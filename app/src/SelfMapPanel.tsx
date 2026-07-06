import { useMemo, useRef, useState } from "react";
import { useTwin } from "./store";
import { Widget } from "./Widget";
import {
  simulateSurvey, slotsFromFixtures, solveMapping, scoreAgainstTruth, manualQueue,
  type Anchor, type SimSurveyResult, type SolveResult, type SurveyNode,
} from "./selfmap";
import { foldSession, lockCmd, mapHash, type RssiReport, type TofReport } from "./syncproto";
import { loadCalibration, saveCalibration, assign as assignEntry, lockAll } from "./calibration";

/** SELF-MAP — the staged sync/calibration protocol, live (Elliot's 2026-07-05 spec).
 *
 *  1 SURVEY   the mesh measures itself (per-neighbor RSSI + downward ToF)
 *  2 SOLVE    fuse mesh topology + ToF heights + the 3-D model prior into a
 *             slot hypothesis per light, each with an honest confidence
 *  3 CONFIRM  flash-and-confirm ONLY the shakiest lights; every confirm is an
 *             anchor that re-orients the next solve (the active loop)
 *  4 LOCK     photogrammetry/manual residuals accepted → map frozen + hashed
 *
 *  Today the survey is SIMULATED (real positions = model + placement drift,
 *  RSSI with ±8 dB per-board bias per Ben's net_bench) but it flows through
 *  the REAL protocol frames (syncproto.ts) — swap the sim for live heartbeats
 *  and nothing else changes. */

const ACCENT = "#c8a24a";

interface RoundLog { round: number; accuracy: number; confirmed: number; queue: number }

export function SelfMapPanel() {
  const fixtures = useTwin((s) => s.fixtures);
  const [radioRange, setRadioRange] = useState(60); // open-air ESP-NOW reach (m)
  const [drift, setDrift] = useState(0.35); // install placement drift vs model (m)
  const [seed, setSeed] = useState(7);
  const [survey, setSurvey] = useState<SimSurveyResult | null>(null);
  const [nodes, setNodes] = useState<SurveyNode[] | null>(null);
  const [result, setResult] = useState<SolveResult | null>(null);
  const [rounds, setRounds] = useState<RoundLog[]>([]);
  const [locked, setLocked] = useState<string | null>(null);
  const [busy, setBusy] = useState<string | null>(null);
  const anchors = useRef<Anchor[]>([]);

  const slots = useMemo(() => (fixtures.length ? slotsFromFixtures(fixtures) : []), [fixtures]);
  const truthById = useMemo(() => new Map((survey?.truth ?? []).map((t) => [t.mac, t.fixtureId])), [survey]);

  const runSurvey = () => {
    setBusy("surveying…");
    setTimeout(() => {
      try {
      const sim = simulateSurvey(fixtures, { seed, maxRangeM: radioRange, placementJitterM: drift });
      // route through the REAL wire shapes: sim nodes → cal_rssi/cal_tof frames → fold
      const session = seed;
      const rssi: RssiReport[] = sim.nodes.map((n) => ({ proto: 1, kind: "cal_rssi", session, mac: n.mac, role: n.role, rows: n.rows }));
      const tof: TofReport[] = sim.nodes.map((n) => ({ proto: 1, kind: "cal_tof", session, mac: n.mac, heightM: n.tofHeightM, sigmaM: 0.08, clear: n.tofHeightM !== null }));
      setSurvey(sim);
      setNodes(foldSession(rssi, tof));
      setResult(null);
      setRounds([]);
      setLocked(null);
      anchors.current = [];
      } finally { setBusy(null); }
    }, 30);
  };

  const solve = (extra: Anchor[] = []) => {
    if (!nodes || !survey) return;
    setBusy("solving…");
    setTimeout(() => {
      try {
      anchors.current = [...anchors.current, ...extra];
      // MANUAL ADJUSTMENTS feed the solver: any installer-confirmed entry in
      // the calibration map (e.g. re-slotted by hand in the Fleet panel) is an
      // anchor here — human truth always outranks the solver's own hypotheses
      const macsInSurvey = new Set(nodes.map((n) => n.mac));
      const manual = loadCalibration().entries
        .filter((e) => (e.stage === "confirmed" || e.stage === "locked") && macsInSurvey.has(e.mac))
        .map((e) => ({ mac: e.mac, fixtureId: e.fixtureId }));
      const byMac = new Map<string, Anchor>();
      for (const a of anchors.current) byMac.set(a.mac, a);
      for (const a of manual) byMac.set(a.mac, a); // map-confirmed wins on conflict
      anchors.current = [...byMac.values()];
      const res = solveMapping(nodes, slots, anchors.current);
      setResult(res);
      const score = scoreAgainstTruth(res, survey.truth);
      setRounds((r) => [...r, { round: r.length + 1, accuracy: score.accuracy, confirmed: anchors.current.length, queue: manualQueue(res).length }]);
      // persist hypotheses into the calibration map (mesh provenance)
      let map = loadCalibration();
      const at = new Date().toISOString();
      for (const e of res.estimates) {
        if (!e.fixtureId) continue;
        const isAnchor = anchors.current.some((a) => a.mac === e.mac);
        map = assignEntry(map, e.mac, e.fixtureId, at, isAnchor
          ? { stage: "confirmed", confidence: 1, method: "manual" }
          : { stage: "hypothesis", confidence: e.confidence, method: "mesh" });
      }
      saveCalibration(map);
      } finally { setBusy(null); }
    }, 30);
  };

  /** The active loop's confirm step: in the sim the "installer" flash-checks the
   *  N shakiest lights and taps their true slot; on hardware this is the
   *  Commissioning flow (identify-flash → tap) driven by the same queue. */
  const confirmShakiest = (n: number) => {
    if (!result) return;
    const queue = result.estimates
      .filter((e) => !anchors.current.some((a) => a.mac === e.mac))
      .sort((p, q) => p.confidence - q.confidence)
      .slice(0, n);
    const extra: Anchor[] = queue
      .map((e) => ({ mac: e.mac, fixtureId: truthById.get(e.mac)! }))
      .filter((a) => a.fixtureId);
    solve(extra);
  };

  const doLock = () => {
    const map = lockAll(loadCalibration());
    saveCalibration(map);
    const cmd = lockCmd(seed, rounds.length, map.entries);
    setLocked(cmd.mapHash);
  };

  const score = result && survey ? scoreAgainstTruth(result, survey.truth) : null;
  const queue = result ? manualQueue(result) : [];
  const meanNeighbors = nodes ? Math.round(nodes.reduce((s, n) => s + n.rows.length, 0) / Math.max(1, nodes.length)) : 0;
  const tofFixes = nodes ? nodes.filter((n) => n.tofHeightM !== null).length : 0;

  // ── top-down minimap (model slots + estimates + assignment links) ────────────
  const minimap = useMemo(() => {
    if (!slots.length) return null;
    const xs = slots.map((s) => s.xy[0]), ys = slots.map((s) => s.xy[1]);
    const minX = Math.min(...xs), maxX = Math.max(...xs), minY = Math.min(...ys), maxY = Math.max(...ys);
    const W = 252, H = 190, pad = 12;
    const sx = (x: number) => pad + ((x - minX) / Math.max(1e-6, maxX - minX)) * (W - 2 * pad);
    const sy = (y: number) => pad + ((y - minY) / Math.max(1e-6, maxY - minY)) * (H - 2 * pad);
    const slotById = new Map(slots.map((s) => [s.fixtureId, s]));
    const wrong = new Set(score?.wrong ?? []);
    return (
      <svg width={W} height={H} style={{ background: "#0a0f16", border: "1px solid #1d2735", borderRadius: 8 }}>
        {slots.map((s) => (
          <circle key={s.fixtureId} cx={sx(s.xy[0])} cy={sy(s.xy[1])} r={2} fill="none" stroke="#3a4a60" strokeWidth={1} />
        ))}
        {result?.estimates.map((e) => {
          if (!e.fixtureId) return null;
          const s = slotById.get(e.fixtureId)!;
          const conf = e.confidence;
          const col = wrong.has(e.mac) ? "#ff5470" : `hsl(${Math.round(conf * 120)},70%,55%)`;
          return (
            <g key={e.mac}>
              <line x1={sx(e.estXY[0])} y1={sy(e.estXY[1])} x2={sx(s.xy[0])} y2={sy(s.xy[1])} stroke={col} strokeWidth={0.6} opacity={0.5} />
              <circle cx={sx(e.estXY[0])} cy={sy(e.estXY[1])} r={2.2} fill={col} />
            </g>
          );
        })}
      </svg>
    );
  }, [slots, result, score]);

  const btn = (label: string, onClick: () => void, disabled = false, primary = false) => (
    <button onClick={onClick} disabled={disabled || !!busy}
      style={{ padding: "6px 10px", borderRadius: 8, border: `1px solid ${primary ? ACCENT : "#2a3648"}`,
        background: primary ? "rgba(200,162,74,0.18)" : "#121a26", color: disabled ? "#5a677a" : "#e8eefb",
        font: "12px ui-monospace, monospace", cursor: disabled ? "default" : "pointer" }}>
      {label}
    </button>
  );

  return (
    <Widget id="selfmap" title="🛰 Self-Map · staged sync protocol" x={16} y={64} w={290} h={560} accent={ACCENT}>
      <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
        <div style={{ color: "#9fb0c7", lineHeight: 1.45 }}>
          The mesh locates every light on the tree: <b>survey</b> (RSSI + ToF) →{" "}
          <b>solve</b> (fuse with the 3-D model) → <b>confirm</b> only the shaky ones →{" "}
          <b>lock</b>. Sim survey, real protocol frames.
        </div>
        {/* survey knobs */}
        <div style={{ display: "flex", gap: 10, alignItems: "center", flexWrap: "wrap" }}>
          <label style={{ display: "flex", gap: 5, alignItems: "center" }}>
            radio <input type="range" min={30} max={150} step={5} value={radioRange} onChange={(e) => setRadioRange(+e.target.value)} style={{ width: 70 }} /> {radioRange}m
          </label>
          <label style={{ display: "flex", gap: 5, alignItems: "center" }}>
            drift <input type="range" min={0} max={1.5} step={0.05} value={drift} onChange={(e) => setDrift(+e.target.value)} style={{ width: 60 }} /> {drift.toFixed(2)}m
          </label>
        </div>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
          {btn("1 · 📡 survey mesh", () => { setSeed((s) => s + 1); runSurvey(); }, !fixtures.length, true)}
          {btn("2 · 🧠 solve map", () => solve(), !nodes, true)}
          {btn("3 · ✅ confirm 10 shakiest", () => confirmShakiest(10), !result)}
          {btn("4 · 🔒 lock", doLock, !result || (score ? score.accuracy < 0.999 : true))}
        </div>
        {busy && <div style={{ color: ACCENT }}>{busy}</div>}
        {/* survey summary */}
        {nodes && (
          <div style={{ color: "#9fb0c7" }}>
            heard <b style={{ color: "#e8eefb" }}>{nodes.length}</b>/{fixtures.length} lights ·
            ~<b style={{ color: "#e8eefb" }}>{meanNeighbors}</b> neighbors each ·
            ToF fix on <b style={{ color: "#e8eefb" }}>{tofFixes}</b>
          </div>
        )}
        {/* solve summary */}
        {result && score && (
          <>
            <div style={{ display: "flex", gap: 12, flexWrap: "wrap" }}>
              <span>slot accuracy <b style={{ color: score.accuracy > 0.9 ? "#3ddc97" : score.accuracy > 0.6 ? ACCENT : "#ff5470" }}>{Math.round(score.accuracy * 100)}%</b> <span style={{ color: "#5a677a" }}>(sim truth)</span></span>
              <span>confirmed <b style={{ color: "#e8eefb" }}>{anchors.current.length}</b></span>
              <span>check-queue <b style={{ color: "#e8eefb" }}>{queue.length}</b></span>
            </div>
            {minimap}
            {/* active-loop history */}
            {rounds.length > 0 && (
              <div style={{ color: "#9fb0c7" }}>
                {rounds.map((r) => (
                  <div key={r.round}>
                    round {r.round}: <b style={{ color: "#e8eefb" }}>{Math.round(r.accuracy * 100)}%</b> after {r.confirmed} confirms
                  </div>
                ))}
              </div>
            )}
            {/* the installer's walk list */}
            {queue.length > 0 && (
              <div>
                <div style={{ color: "#eef3fb", fontWeight: 700, margin: "2px 0" }}>flash-and-confirm next:</div>
                {queue.slice(0, 8).map((e) => (
                  <div key={e.mac} style={{ display: "flex", justifyContent: "space-between", color: "#9fb0c7" }}>
                    <span>{e.mac} → {e.fixtureId ?? "—"}</span>
                    <span style={{ color: e.confidence < 0.2 ? "#ff5470" : ACCENT }}>{Math.round(e.confidence * 100)}%</span>
                  </div>
                ))}
              </div>
            )}
            {score.accuracy >= 0.999 && !locked && (
              <div style={{ color: "#3ddc97" }}>every light mapped — ready to lock</div>
            )}
          </>
        )}
        {locked && (
          <div style={{ color: "#3ddc97" }}>
            🔒 map LOCKED · hash <b>{locked}</b> — broadcast to the fleet; a hash
            mismatch on reconnect means the map drifted (current: {mapHash(loadCalibration().entries)})
          </div>
        )}
      </div>
    </Widget>
  );
}
