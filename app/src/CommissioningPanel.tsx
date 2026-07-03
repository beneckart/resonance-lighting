import { useContext, useMemo, useState } from "react";
import { useTwin } from "./store";
import { DockCtx } from "./Widget";
import {
  loadCalibration, saveCalibration, assign, resolveMac, progress, type CalibrationMap,
} from "./calibration";

/** Commissioning UI (Elliot: "calibration built in"): bind each physical fixture
 *  (MAC-derived ID) to its fixtures.json slot. Flow: 🔦 identify flashes the
 *  fixture in the twin so an installer sees which it is, then assign its MAC.
 *  On real hw the MACs arrive from the ESP-NOW heartbeat; here you can type one
 *  or use a sim MAC. calibration.ts holds the (tested) 1:1 map logic. */
const ACCENT = "#3ddc97";
const panel: React.CSSProperties = {
  position: "fixed", top: 12, right: 12, width: 248, maxHeight: "92vh", overflowY: "auto",
  padding: "10px 12px", background: "rgba(8,16,12,0.9)", border: "1px solid #1d3a2a",
  borderRadius: 10, color: "#dceee4", font: "11px ui-monospace, SFMono-Regular, monospace",
  backdropFilter: "blur(6px)", zIndex: 18,
};

let simN = 0;
const simMac = () => { simN += 1; return `SIM${simN.toString(16).toUpperCase().padStart(3, "0")}`; };

export function CommissioningPanel() {
  const docked = useContext(DockCtx);
  const [open, setOpen] = useState(false);
  const fixtures = useTwin((s) => s.fixtures);
  const runCommand = useTwin((s) => s.runCommand);
  const [map, setMap] = useState<CalibrationMap>(() => loadCalibration());
  const [mac, setMac] = useState("");

  const prog = useMemo(() => progress(map, fixtures), [map, fixtures]);

  const commit = (next: CalibrationMap) => { saveCalibration(next); setMap(next); };

  const flash = (id: string) => {
    runCommand(`fixture ${id} color #ffffff`);
    window.setTimeout(() => runCommand("clear"), 1100);
  };

  if (!open && !docked) {
    return (
      <button onClick={() => setOpen(true)} style={{
        position: "fixed", bottom: 14, left: 14, zIndex: 17, padding: "7px 11px", borderRadius: 10,
        border: "1px solid #1d3a2a", background: "rgba(8,16,12,0.85)", color: "#9fc7b0",
        font: "12px ui-monospace, monospace", cursor: "pointer", backdropFilter: "blur(6px)",
      }}>📍 commission</button>
    );
  }

  return (
    <div style={docked ? { ...panel, position: "static", width: "100%", maxHeight: "none", marginBottom: 8, boxSizing: "border-box" as const } : panel}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <span style={{ color: ACCENT, fontWeight: 700 }}>📍 Commissioning</span>
        {!docked && <button onClick={() => setOpen(false)} style={{ border: "1px solid #1d3a2a", background: "#0c1812", color: "#9fc7b0", borderRadius: 6, cursor: "pointer", padding: "2px 8px" }}>✕</button>}
      </div>
      <div style={{ fontSize: 9.5, color: "#7ea692", margin: "4px 0" }}>
        bind each physical fixture (MAC) → its slot · {prog.assigned}/{prog.total} done
      </div>
      <div style={{ height: 6, background: "#11221a", borderRadius: 3, overflow: "hidden", margin: "0 0 6px" }}>
        <div style={{ width: `${Math.round(prog.pct * 100)}%`, height: "100%", background: ACCENT }} />
      </div>
      <div style={{ display: "flex", gap: 4, marginBottom: 6 }}>
        <input value={mac} placeholder="MAC (or sim)" onChange={(e) => setMac(e.target.value.toUpperCase())}
          style={{ flex: 1, minWidth: 0, padding: "4px 6px", borderRadius: 6, border: "1px solid #1d3a2a", background: "#0b1611", color: "#dceee4", font: "11px ui-monospace, monospace" }} />
        <button onClick={() => setMac(simMac())} style={{ border: "1px solid #1d3a2a", background: "#0c1812", color: "#9fc7b0", borderRadius: 6, cursor: "pointer", padding: "0 8px" }}>sim</button>
      </div>
      {fixtures.slice(0, 200).map((f) => {
        const boundMac = resolveMac(map, f.id);
        return (
          <div key={f.id} style={{ display: "flex", alignItems: "center", gap: 4, padding: "3px 0", borderTop: "1px solid #11221a" }}>
            <span style={{ width: 42, color: boundMac ? ACCENT : "#7ea692" }}>{f.id}</span>
            <span style={{ flex: 1, fontSize: 9.5, color: "#6a8a78", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
              {boundMac ?? `${f.role}·${f.zone}`}
            </span>
            <button onClick={() => flash(f.id)} title="flash this fixture" style={{ border: "1px solid #2a3a52", background: "#10131a", color: "#cdd6e4", borderRadius: 5, cursor: "pointer", padding: "2px 5px" }}>🔦</button>
            <button
              onClick={() => commit(assign(map, mac || simMac(), f.id, new Date().toISOString()))}
              title="assign the MAC above to this fixture"
              style={{ border: `1px solid ${ACCENT}55`, background: "#0c1812", color: ACCENT, borderRadius: 5, cursor: "pointer", padding: "2px 6px" }}
            >bind</button>
          </div>
        );
      })}
    </div>
  );
}
