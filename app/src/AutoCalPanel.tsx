import { useEffect, useRef, useState } from "react";
import { useTwin } from "./store";
import { Widget } from "./Widget";
import { telemetry } from "./telemetry";
import { buildPlan, judgeColor, tofSample, tofHistogram, CAL_RGB, type CalColor, type CalStep, type CalVerdict } from "./autocal";

/** AUTO-CALIBRATION & TESTING — all lights off, then each light comes on SOLO
 *  (group by group), gets a quick colour/brightness check against what it REPORTS
 *  (the mirror/heartbeat), goes off, next. Each solo moment doubles as the
 *  photogrammetry capture window, and the ToF rangers' height histogram checks
 *  the model's positions against "reality". */
const COLORS: CalColor[] = ["red", "green", "blue", "white"];

export function AutoCalPanel() {
  const fixtures = useTwin((s) => s.fixtures);
  const calSolo = useTwin((s) => s.calSolo);
  const [running, setRunning] = useState(false);
  const [stepMs, setStepMs] = useState(350);
  const [useColors, setUseColors] = useState<CalColor[]>(["red", "green", "blue", "white"]);
  const [cursor, setCursor] = useState(-1);
  const [verdicts, setVerdicts] = useState<Map<number, CalVerdict>>(new Map());
  const plan = useRef<CalStep[]>([]);
  const timer = useRef<number | null>(null);
  const snapshot = useRef<{ brightness: number; master: number; blackout: boolean; strobe: boolean } | null>(null);

  const stop = (restore = true) => {
    if (timer.current) { clearInterval(timer.current); timer.current = null; }
    calSolo(null);
    if (restore && snapshot.current) useTwin.getState().set(snapshot.current);
    setRunning(false);
  };

  const start = () => {
    const st = useTwin.getState();
    if (st.gol.phase !== "off") st.golSetPhase("off"); // leave Game of Light cleanly
    snapshot.current = { brightness: st.control.brightness, master: st.control.master, blackout: st.control.blackout, strobe: st.control.strobe };
    st.set({ brightness: 1, master: 1, blackout: false, strobe: false }); // full drive for the test
    plan.current = buildPlan(st.fixtures, st.namedGroups, useColors);
    setVerdicts(new Map());
    setCursor(0);
    setRunning(true);
    // CONFIRM-OR-TIMEOUT stepping (heartbeat-correct): light the fixture, then wait
    // for ITS report to confirm the colour — a fast light advances immediately after
    // the min hold; a light whose heartbeat hasn't confirmed by the timeout is a
    // real failure (dead / wrong colour), not a scheduling artifact. Real ESP-NOW
    // heartbeats are ~1 Hz, so a fixed fast cadence would flag healthy lights.
    let i = 0;
    let stepT0 = 0;
    const TIMEOUT_MS = 1800; // > max heartbeat interval (mock: 0.6–1.2s; real: ~1s)
    const record = (s: CalStep, ok: boolean, delta: number) => setVerdicts((v) => {
      const next = new Map(v);
      const cur = next.get(s.num) ?? { num: s.num, id: s.id, group: s.group, ok: true, worstDelta: 0 };
      if (!ok && cur.ok) { cur.ok = false; cur.failedColor = s.color; }
      cur.worstDelta = Math.max(cur.worstDelta, delta);
      next.set(s.num, cur);
      return next;
    });
    const begin = (idx: number) => {
      const s = plan.current[idx];
      calSolo({ idx: s.idx, rgb: CAL_RGB[s.color] });
      setCursor(idx);
      stepT0 = performance.now();
    };
    begin(0);
    timer.current = window.setInterval(() => {
      if (i >= plan.current.length) { stop(); setCursor(plan.current.length); return; }
      const s = plan.current[i];
      const held = performance.now() - stepT0;
      const rep = telemetry.states.find((x) => x.num === s.num);
      const j = judgeColor(CAL_RGB[s.color], rep ? [rep.rgb[0], rep.rgb[1], rep.rgb[2]] : null);
      if (j.ok && held >= stepMs) { record(s, true, j.delta); i += 1; if (i < plan.current.length) begin(i); else { stop(); setCursor(plan.current.length); } }
      else if (held >= Math.max(stepMs, TIMEOUT_MS)) { record(s, false, j.delta); i += 1; if (i < plan.current.length) begin(i); else { stop(); setCursor(plan.current.length); } }
    }, 90);
  };
  useEffect(() => () => stop(false), []); // eslint-disable-line react-hooks/exhaustive-deps

  const cur = cursor >= 0 && cursor < plan.current.length ? plan.current[cursor] : null;
  const done = [...verdicts.values()];
  const fails = done.filter((v) => !v.ok);
  const lightsTotal = new Set(plan.current.map((p) => p.num)).size || fixtures.length;
  const pct = plan.current.length ? Math.min(1, cursor / plan.current.length) : 0;

  // ToF self-location: each lantern's ranger vs the model → cm error histogram
  const groundY = fixtures.length ? Math.min(...fixtures.map((f) => f.pos[1])) : 0;
  const tof = fixtures.map((f) => tofSample(f, groundY));
  const hist = tofHistogram(tof.map((t) => t.errCm));
  const histMax = Math.max(1, ...hist.map((b) => b.n));
  const meanAbs = tof.length ? tof.reduce((a, t) => a + Math.abs(t.errCm), 0) / tof.length : 0;

  const exportReport = () => {
    const blob = new Blob([JSON.stringify({
      when: new Date().toISOString(), stepMs, colors: useColors,
      lights: done, tof: fixtures.map((f, i) => ({ num: f.num, id: f.id, trueH: +tof[i].trueH.toFixed(3), measuredH: +tof[i].measuredH.toFixed(3) })),
      note: "each solo-lit step is the photogrammetry capture window: frame-time ↔ light id",
    }, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a"); a.href = url; a.download = "resonance-calibration-report.json";
    document.body.appendChild(a); a.click(); setTimeout(() => { a.remove(); URL.revokeObjectURL(url); }, 5000);
  };

  return (
    <Widget id="autocal" title="🔬 Auto-calibration & testing" x={12} y={12} w={300} h={430} accent="#c8a24a">
      <div style={{ fontSize: 10.5, color: "#c9b98a", lineHeight: 1.35, marginBottom: 8 }}>
        All lights off → each light comes on <b>solo</b> (group by group), colour +
        brightness checked against what it <b>reports back</b>, then off, next.
        Cameras capture each solo moment (photogrammetry); ToF rangers share their
        height histogram to locate lights in space.
      </div>

      <div style={{ display: "flex", gap: 4, marginBottom: 6 }}>
        {COLORS.map((c) => {
          const on = useColors.includes(c);
          const col = { red: "#ff5b5b", green: "#4be08a", blue: "#5b8cff", white: "#e8ecf4" }[c];
          return (
            <button key={c} disabled={running} onClick={() => setUseColors((u) => on ? u.filter((x) => x !== c) : [...u, c])}
              style={{ flex: 1, padding: "5px 2px", borderRadius: 6, cursor: "pointer", fontSize: 10, fontWeight: 700,
                border: on ? `1.5px solid ${col}` : "1px solid #2a3a52", background: on ? "#151b28" : "#0c121c", color: on ? col : "#5a6a82" }}>
              {c}
            </button>
          );
        })}
      </div>
      <label style={{ display: "block", marginBottom: 8 }}>
        <div style={{ fontSize: 10, color: "#8aa0bb", marginBottom: 2 }}>Min hold per light · {stepMs}ms — each light stays on until its heartbeat CONFIRMS (or times out = fail)</div>
        <input type="range" min={120} max={1500} step={10} value={stepMs} disabled={running}
          onChange={(e) => setStepMs(+e.target.value)} style={{ width: "100%" }} />
      </label>

      {!running
        ? <button onClick={start} disabled={!useColors.length}
            style={{ width: "100%", padding: "10px 8px", borderRadius: 9, cursor: "pointer", fontWeight: 700, fontSize: 13, border: "1.5px solid #c8a24a", background: "#2a2410", color: "#f0d890" }}>
            ▶ Run auto-calibration ({lightsTotal || "…"} lights × {useColors.length} colours)
          </button>
        : <button onClick={() => stop()}
            style={{ width: "100%", padding: "10px 8px", borderRadius: 9, cursor: "pointer", fontWeight: 700, fontSize: 13, border: "1.5px solid #ff5b6e", background: "#2a1016", color: "#ff8fa0" }}>
            ⏹ Abort ({Math.round(pct * 100)}%)
          </button>}

      {(running || done.length > 0) && (
        <div style={{ marginTop: 8 }}>
          <div style={{ height: 6, background: "#1a2233", borderRadius: 3, overflow: "hidden" }}>
            <div style={{ height: "100%", width: `${pct * 100}%`, background: "#c8a24a" }} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", fontSize: 10, marginTop: 3, color: "#8aa0bb" }}>
            <span>{cur ? `▸ ${cur.group} · light ${cur.num} · ${cur.color}` : running ? "…" : "done"}</span>
            <span>✓ {done.filter((v) => v.ok).length} · ✗ {fails.length}</span>
          </div>
          {fails.length > 0 && (
            <div style={{ marginTop: 4, maxHeight: 70, overflowY: "auto", fontSize: 9.5, color: "#ff9aa8" }}>
              {fails.slice(0, 20).map((f) => <div key={f.num}>✗ light {f.num} ({f.group}) — failed {f.failedColor} · Δ{f.worstDelta.toFixed(2)}</div>)}
            </div>
          )}
          {!running && done.length > 0 && (
            <button onClick={exportReport} style={{ width: "100%", marginTop: 6, padding: "6px 8px", borderRadius: 7, cursor: "pointer", fontSize: 11, border: "1px solid #2a3a52", background: "#121a26", color: "#9fb0c7" }}>
              ⬇ export report (JSON — calibration + ToF + photo-sync log)
            </button>
          )}
        </div>
      )}

      {/* ToF self-location: height-error histogram (model vs rangers) */}
      <div style={{ marginTop: 10, paddingTop: 8, borderTop: "1px solid #1d2735" }}>
        <div style={{ fontSize: 10.5, fontWeight: 700, color: "#cfd8e6", marginBottom: 4 }}>
          📡 ToF self-location · mean |err| {meanAbs.toFixed(1)} cm
        </div>
        <div style={{ display: "flex", alignItems: "flex-end", gap: 1, height: 42 }}>
          {hist.map((b) => (
            <div key={b.x0} title={`${b.x0}…${b.x0 + 2} cm: ${b.n}`}
              style={{ flex: 1, height: `${(b.n / histMax) * 100}%`, background: Math.abs(b.x0 + 1) <= 2 ? "#4be08a" : "#3a4a66", borderRadius: "2px 2px 0 0", minHeight: 1 }} />
          ))}
        </div>
        <div style={{ display: "flex", justifyContent: "space-between", fontSize: 9, color: "#5a6a82" }}>
          <span>−12cm</span><span>0</span><span>+12cm</span>
        </div>
        <div style={{ fontSize: 9.5, color: "#7a8ba3", marginTop: 3 }}>
          each lantern's downward ranger reports its height → matched against the
          model, it anchors the photogrammetry solve in real space
        </div>
      </div>
    </Widget>
  );
}
