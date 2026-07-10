import { useEffect, useState } from "react";
import { telemetry } from "./telemetry";
import { flagBug, recSummary } from "./flightrec";
import { Widget } from "./Widget";

// nearest readable colour name (research: show "blue", not "#1e90ff")
const NAMED: [string, [number, number, number]][] = [
  ["red", [1, 0, 0]], ["orange", [1, 0.5, 0]], ["amber", [1, 0.75, 0.2]], ["yellow", [1, 1, 0]],
  ["green", [0, 1, 0]], ["cyan", [0, 1, 1]], ["blue", [0.1, 0.3, 1]], ["purple", [0.5, 0, 1]],
  ["magenta", [1, 0, 1]], ["pink", [1, 0.45, 0.7]], ["white", [1, 1, 1]],
];
function colorName(r: number, g: number, b: number): string {
  const m = Math.max(r, g, b) || 1;
  const nr = r / m, ng = g / m, nb = b / m;
  let best = "—", bd = Infinity;
  for (const [n, c] of NAMED) {
    const d = (nr - c[0]) ** 2 + (ng - c[1]) ** 2 + (nb - c[2]) ** 2;
    if (d < bd) { bd = d; best = n; }
  }
  return best;
}
const hex = (r: number, g: number, b: number) =>
  "#" + [r, g, b].map((v) => Math.round(Math.min(1, Math.max(0, v)) * 255).toString(16).padStart(2, "0")).join("");

export function DataLog() {
  const [, setTick] = useState(0);
  const [showAll, setShowAll] = useState(false);
  useEffect(() => {
    const id = setInterval(() => setTick((x) => x + 1), 250); // poll the module-level snapshot ~4 Hz
    return () => clearInterval(id);
  }, []);

  const states = telemetry.states;
  const lit = states.filter((s) => s.bri > 0.02);
  const rows = (showAll ? states : lit).slice().sort((a, b) => a.num - b.num);

  return (
    <Widget id="datalog" title="📟 data log — what the lights are doing" x={12} y={430} w={300} h={280}>
      {/* 🐞 FLIGHT RECORDER (doc 18C): always recording a rolling window of
          inputs + output keyframes; flagging freezes the last 2 min into a
          downloadable repro the twin can replay. */}
      <div style={{ display: "flex", gap: 6, alignItems: "center", marginBottom: 6 }}>
        <button onClick={() => {
          const note = window.prompt("What looked wrong? (goes into the bug file)") ?? "";
          const log = flagBug(note, telemetry.states.length);
          const blob = new Blob([JSON.stringify(log)], { type: "application/json" });
          const a = document.createElement("a");
          a.href = URL.createObjectURL(blob);
          a.download = `tree-bug-${new Date().toISOString().slice(0, 16).replace(/[T:]/g, "-")}.json`;
          document.body.appendChild(a); a.click();
          setTimeout(() => { a.remove(); URL.revokeObjectURL(a.href); }, 5000);
        }}
          title="freeze the last 2 minutes (every tap, mode change + what every light did) into a bug file"
          style={{ flex: 1, padding: "6px 8px", borderRadius: 8, cursor: "pointer", fontWeight: 700, fontSize: 11,
            border: "1.5px solid #ffb74d", background: "#2a1d0c", color: "#ffd9a0" }}>
          🐞 flag a bug — save last 2 min
        </button>
        <span style={{ fontSize: 9, color: "#7a8ba3" }} title="events + output keyframes in the rolling black box">
          ⏺ {recSummary().events}ev · {recSummary().keyframes}kf
        </span>
      </div>
      <div style={{ display: "flex", justifyContent: "space-between", fontSize: 10, opacity: 0.7, marginBottom: 4 }}>
        <span><b style={{ color: "#7fe0a0" }}>{lit.length}</b> lit / {states.length} lights</span>
        <button onClick={() => setShowAll((v) => !v)}
          style={{ padding: "1px 7px", borderRadius: 5, cursor: "pointer", border: "1px solid #2a3a52", background: "#0d141e", color: "#9fb0c7", fontSize: 10 }}>
          {showAll ? "lit only" : "show all"}
        </button>
      </div>
      {rows.length === 0 && <div style={{ opacity: 0.5 }}>all dark — no lights on</div>}
      {rows.map((s) => {
        const pct = Math.round(s.bri * 100);
        const name = s.bri > 0.02 ? colorName(s.rgb[0], s.rgb[1], s.rgb[2]) : "off";
        return (
          <div key={s.id} style={{ display: "flex", alignItems: "center", gap: 7, lineHeight: 1.55 }}>
            <span style={{ width: 11, height: 11, borderRadius: 3, flex: "0 0 auto",
              background: hex(s.rgb[0], s.rgb[1], s.rgb[2]), border: "1px solid #2a3a52" }} />
            <span style={{ color: "#8fa3bf" }}>L{String(s.num).padStart(3, " ")}</span>
            <span style={{ color: s.bri > 0.02 ? "#dce6ff" : "#566", minWidth: 34 }}>{s.bri > 0.02 ? `${pct}%` : "off"}</span>
            <span style={{ opacity: 0.85 }}>{name}</span>
          </div>
        );
      })}
    </Widget>
  );
}
