import { useEffect, useMemo, useRef, useState } from "react";
import { useTwin } from "./store";
import { Widget } from "./Widget";
import { MockBridge, SerialBridge, type BridgeLink, type UpFrame } from "./bridge";
import {
  applyEvent, applyHeartbeat, exportCsv, loadRegistry,
  onlineCount, saveRegistry, sweepOffline, uplinkPdr, type Registry,
} from "./macregistry";
import { macFromNum } from "./selfmap";
import { loadCalibration, saveCalibration, assign as assignEntry, resolveFixtureId } from "./calibration";
import { loadFixtures, validateFixturesDoc } from "./fixtures";

/** FLEET — the live two-way bridge console (Elliot's 2026-07-05 spec).
 *
 *  Left side of the bridge: this panel (the controller). Right side: the fleet,
 *  reached through the bridge PowerFeather. Today the fleet is the MockBridge
 *  (same seam, tick-driven); plug the real bridge in over USB and the Serial
 *  button drives the identical code path.
 *
 *  What it proves on screen:
 *   - every MAC that ever speaks is REGISTERED and LOGGED (macregistry)
 *   - fleet state arrives with NO polling: heartbeats (≤ ~650 ms staleness)
 *     + instant edge events ("tap" a light and watch the row flash NOW)
 *   - MANUAL ADJUSTMENTS: any light can be re-slotted by hand at any time;
 *     a manual assignment is installer truth — it becomes an anchor for the
 *     next self-map solve
 *   - battery cost of knowing all this: zero extra duty (it rides the
 *     heartbeat Ben's 46 h soak already validated) */

const ACCENT = "#3aa4c8";
const UI_HZ = 2; // table refresh; events bypass this and flash immediately

export function FleetPanel() {
  const fixtures = useTwin((s) => s.fixtures);
  const calSolo = useTwin((s) => s.calSolo);
  const [, bump] = useState(0); // registry lives in a ref; bump = re-render
  const reg = useRef<Registry>(loadRegistry());
  const bridge = useRef<BridgeLink | null>(null);
  const [connected, setConnected] = useState(false);
  const [transport, setTransport] = useState<"mock" | "serial" | null>(null);
  const [flash, setFlash] = useState<Record<string, number>>({}); // mac → flash-until ts
  const [lastEvtLatency, setLastEvtLatency] = useState<number | null>(null);
  const tapSentAt = useRef<Record<string, number>>({});
  const [calVersion, setCalVersion] = useState(0);
  const [editMac, setEditMac] = useState<string | null>(null);
  void calVersion; // reslot bumps this to force a re-read below
  const calMap = loadCalibration(); // fresh each render — SelfMap writes show up live
  const [hbHz, setHbHz] = useState(2);

  const specs = useMemo(
    () => fixtures.map((f) => ({ mac: macFromNum(f.num), role: f.role })),
    [fixtures],
  );
  const numByMac = useMemo(() => new Map(fixtures.map((f) => [macFromNum(f.num), f.num])), [fixtures]);
  const idxByMac = useMemo(() => new Map(fixtures.map((f, i) => [macFromNum(f.num), i])), [fixtures]);

  const disconnect = () => {
    bridge.current?.disconnect();
    bridge.current = null;
    setConnected(false);
    setTransport(null);
    saveRegistry(reg.current);
  };

  const wire = (b: BridgeLink) => {
    b.onUp((f: UpFrame) => {
      const now = Date.now(); // wall clock — ledger timestamps survive reloads
      if (f.kind === "hb") {
        applyHeartbeat(reg.current, f, now);
      } else {
        applyEvent(reg.current, f, now);
        // INSTANT path: an edge event re-renders immediately, no batching
        setFlash((m) => ({ ...m, [f.mac]: now + 900 }));
        const sent = tapSentAt.current[f.mac];
        if (sent && f.event === "tap") {
          setLastEvtLatency(performance.now() - sent);
          delete tapSentAt.current[f.mac];
        }
        bump((v) => v + 1);
      }
    });
  };

  const connectMock = async () => {
    disconnect();
    const b = new MockBridge(specs, 7);
    wire(b);
    await b.connect();
    bridge.current = b;
    setConnected(true);
    setTransport("mock");
  };

  const connectSerial = async () => {
    disconnect();
    const b = new SerialBridge();
    wire(b);
    await b.connect(); // user picks the plugged-in bridge PowerFeather
    bridge.current = b;
    setConnected(true);
    setTransport("serial");
  };

  // drive: mock tick + offline sweep + batched UI refresh
  useEffect(() => {
    if (!connected) return;
    const t = window.setInterval(() => {
      const b = bridge.current;
      if (b instanceof MockBridge) b.tick(1000 / UI_HZ);
      sweepOffline(reg.current, Date.now());
      const cutoff = Date.now();
      setFlash((m) => {
        const live = Object.entries(m).filter(([, until]) => until > cutoff);
        return live.length === Object.keys(m).length ? m : Object.fromEntries(live);
      });
      bump((v) => v + 1);
    }, 1000 / UI_HZ);
    return () => window.clearInterval(t);
  }, [connected]);
  useEffect(() => () => disconnect(), []); // eslint-disable-line react-hooks/exhaustive-deps

  const identify = (mac: string) => {
    bridge.current?.send({ kind: "identify", mac, seconds: 5 });
    // mirror the locate-blink in the twin (the mirror renders REPORTED intent)
    const idx = idxByMac.get(mac);
    if (idx !== undefined) {
      calSolo({ idx, rgb: [1, 1, 1] });
      window.setTimeout(() => calSolo(null), 1500);
    }
  };

  const tap = (mac: string) => {
    tapSentAt.current[mac] = performance.now();
    (bridge.current as MockBridge | null)?.tap?.(mac);
  };

  /** MANUAL ADJUSTMENT: the human re-slots a light. Installer truth →
   *  confirmed/manual in the map → anchor for the next self-map solve. */
  const reslot = (mac: string, fixtureId: string) => {
    if (!fixtureId) return;
    const next = assignEntry(loadCalibration(), mac, fixtureId, new Date().toISOString());
    saveCalibration(next);
    setCalVersion((v) => v + 1);
    setEditMac(null);
  };

  const applyRate = (hz: number) => {
    setHbHz(hz);
    bridge.current?.send({ kind: "set_rate", hbHz: hz, frameHz: 0 });
  };

  const downloadCsv = () => {
    const blob = new Blob([exportCsv(reg.current)], { type: "text/csv" });
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = `fleet-log-${new Date().toISOString().slice(0, 19)}.csv`;
    a.click();
    URL.revokeObjectURL(a.href);
  };

  const loadBench10 = async () => {
    disconnect();
    const doc = await loadFixtures("/fixtures-bench10.json");
    const v = validateFixturesDoc(doc);
    if (v.ok) useTwin.getState().init(doc);
  };
  const loadTree = async () => {
    disconnect();
    const doc = await loadFixtures("/fixtures.json");
    const v = validateFixturesDoc(doc);
    if (v.ok) useTwin.getState().init(doc);
  };

  const { online, total } = onlineCount(reg.current);
  const now = Date.now();
  const records = Object.values(reg.current.records).sort((a, b) => (numByMac.get(a.mac) ?? 999) - (numByMac.get(b.mac) ?? 999));
  const events = reg.current.events.slice(-6).reverse();

  const btn = (label: string, onClick: () => void, disabled = false, primary = false) => (
    <button onClick={onClick} disabled={disabled}
      style={{ padding: "5px 9px", borderRadius: 8, border: `1px solid ${primary ? ACCENT : "#2a3648"}`,
        background: primary ? "rgba(58,164,200,0.16)" : "#121a26", color: disabled ? "#5a677a" : "#e8eefb",
        font: "11.5px ui-monospace, monospace", cursor: disabled ? "default" : "pointer" }}>
      {label}
    </button>
  );

  return (
    <Widget id="fleet" title="📡 Fleet · two-way bridge" x={16} y={64} w={300} h={560} accent={ACCENT}>
      <div style={{ display: "flex", flexDirection: "column", gap: 8, font: "11.5px ui-monospace, monospace" }}>
        <div style={{ color: "#9fb0c7", lineHeight: 1.45 }}>
          Controller ⇄ <b>bridge PowerFeather</b> ⇄ fleet. State rides the 2 Hz
          heartbeat (zero extra battery duty); edges arrive as instant events.
          Manual re-slot any light — human truth beats the solver.
        </div>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
          {!connected && btn("▶ connect (sim fleet)", connectMock, !fixtures.length, true)}
          {!connected && SerialBridge.available() && btn("🔌 connect USB bridge", connectSerial)}
          {connected && btn("■ disconnect", disconnect, false, true)}
          {btn("⬇ export log CSV", downloadCsv, !total)}
        </div>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap", alignItems: "center" }}>
          {btn("bench-10 layout", loadBench10)}
          {btn("full tree", loadTree)}
          <label style={{ color: "#9fb0c7", display: "flex", gap: 4, alignItems: "center" }}>
            hb
            <select value={hbHz} onChange={(e) => applyRate(+e.target.value)} disabled={!connected}
              style={{ background: "#121a26", color: "#e8eefb", border: "1px solid #2a3648", borderRadius: 6 }}>
              <option value={2}>2 Hz (bench)</option>
              <option value={0.5}>0.5 Hz</option>
              <option value={0.2}>0.2 Hz (conserve)</option>
            </select>
          </label>
        </div>
        {connected && (
          <div style={{ color: "#9fb0c7" }}>
            {transport === "mock" ? "sim fleet" : "USB bridge"} ·{" "}
            <b style={{ color: online === total ? "#3ddc97" : "#ffb454" }}>{online}/{total}</b> online
            {lastEvtLatency !== null && <> · last tap→twin <b style={{ color: "#3ddc97" }}>{lastEvtLatency.toFixed(0)} ms</b></>}
          </div>
        )}
        {/* MANUAL ADJUSTMENT: click any slot cell, pick the true slot here */}
        {editMac && (
          <div style={{ display: "flex", gap: 6, alignItems: "center", border: `1px solid ${ACCENT}`, borderRadius: 8, padding: "5px 8px" }}>
            <span style={{ color: "#e8eefb" }}>re-slot <b>{editMac}</b> →</span>
            <select autoFocus defaultValue={resolveFixtureId(calMap, editMac) ?? ""}
              onChange={(e) => reslot(editMac, e.target.value)}
              style={{ background: "#121a26", color: "#e8eefb", border: "1px solid #2a3648", borderRadius: 6 }}>
              <option value="">pick slot…</option>
              {fixtures
                .filter((f) => f.role === (specs.find((s) => s.mac === editMac)?.role ?? f.role))
                .map((f) => <option key={f.id} value={f.id}>{f.id} · {f.name}</option>)}
            </select>
            <button onClick={() => setEditMac(null)} style={{ background: "none", border: "none", color: "#9fb0c7", cursor: "pointer" }}>✕</button>
          </div>
        )}
        {/* the ledger */}
        {records.length > 0 && (
          <div style={{ maxHeight: 230, overflowY: "auto", overflowX: "auto", border: "1px solid #1d2735", borderRadius: 8 }}>
            <table style={{ width: "100%", borderCollapse: "collapse", font: "10.5px ui-monospace, monospace" }}>
              <thead>
                <tr style={{ color: "#7e8ca1", textAlign: "left", position: "sticky", top: 0, background: "#0d1420" }}>
                  <th style={{ padding: "4px 6px" }}>mac</th><th>slot</th><th>st</th><th>batt</th><th>rssi</th><th>pdr</th><th></th>
                </tr>
              </thead>
              <tbody>
                {records.map((r) => {
                  const off = reg.current.offline[r.mac];
                  const hot = (flash[r.mac] ?? 0) > now;
                  const slot = resolveFixtureId(calMap, r.mac);
                  return (
                    <tr key={r.mac} style={{ borderTop: "1px solid #16202e", background: hot ? "rgba(61,220,151,0.14)" : "transparent", transition: "background 0.4s" }}>
                      <td style={{ padding: "3px 6px", color: off ? "#ff5470" : "#e8eefb" }}>
                        {off ? "○" : "●"} {r.mac}
                      </td>
                      <td onClick={() => setEditMac(r.mac)} title="manual re-slot"
                        style={{ color: slot ? "#e8eefb" : "#7e8ca1", cursor: "pointer", textDecoration: "underline dotted" }}>
                        {slot ?? "—"}
                      </td>
                      <td style={{ color: hot ? "#3ddc97" : "#9fb0c7" }}>{r.caState}</td>
                      <td style={{ color: r.soc < 20 ? "#ff5470" : "#9fb0c7" }}>{(r.battMv / 1000).toFixed(2)}V</td>
                      <td style={{ color: "#9fb0c7" }}>{r.dlRssi}</td>
                      <td style={{ color: uplinkPdr(r) < 0.9 ? "#ffb454" : "#9fb0c7" }}>{Math.round(uplinkPdr(r) * 100)}</td>
                      <td style={{ whiteSpace: "nowrap" }}>
                        <button title="identify (locate-blink)" onClick={() => identify(r.mac)} style={{ background: "none", border: "none", cursor: "pointer" }}>💡</button>
                        {transport === "mock" && <button title="tap the physical light (sim)" onClick={() => tap(r.mac)} style={{ background: "none", border: "none", cursor: "pointer" }}>👆</button>}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}
        {/* instant event feed */}
        {events.length > 0 && (
          <div>
            <div style={{ color: "#eef3fb", fontWeight: 700 }}>events (instant)</div>
            {events.map((e, i) => (
              <div key={i} style={{ color: "#9fb0c7" }}>
                <span style={{ color: "#7e8ca1" }}>{(e.atMs / 1000).toFixed(1)}s</span>{" "}
                {e.mac} <b style={{ color: e.kind === "offline" ? "#ff5470" : e.kind === "tap" ? "#3ddc97" : "#e8eefb" }}>{e.kind}</b>
                {e.kind === "state" || e.kind === "tap" ? ` → ${e.value}` : ""}
              </div>
            ))}
          </div>
        )}
      </div>
    </Widget>
  );
}
