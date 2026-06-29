import { useState } from "react";
import { Widget } from "./Widget";
import {
  useTwin, COLOR_CYCLES, LIGHT_ORDERS, DEFAULT_GROUP_CONTROL,
  type PatternId, type ColorCycle, type LightOrder,
} from "./store";

// the per-group "Pattern" choices (movement-ish) — a curated subset of the full list
const GROUP_PATTERNS: PatternId[] = ["sweep", "chase", "spiral", "rings", "sparkle", "fibonacci", "solid", "breathe"];
const SWATCHES = [
  { n: "red", h: 0, s: 1 }, { n: "orange", h: 0.07, s: 1 }, { n: "yellow", h: 0.15, s: 1 },
  { n: "green", h: 0.33, s: 1 }, { n: "cyan", h: 0.5, s: 1 }, { n: "blue", h: 0.62, s: 1 },
  { n: "purple", h: 0.78, s: 1 }, { n: "pink", h: 0.9, s: 0.85 }, { n: "white", h: 0, s: 0 },
];
const LABELS: Record<string, string> = {
  ring1: "Ring 1", ring2: "Ring 2", ring3: "Ring 3", uplights: "Uplights", chandelier: "Chandelier", all: "All",
};
const labelOf = (k: string) => LABELS[k] ?? k;

// "1,4,7" or "1-24" or "1,7,17-20" → sorted unique number[]
function parseNums(s: string): number[] {
  const out = new Set<number>();
  for (const part of s.split(",")) {
    const p = part.trim();
    const r = p.match(/^(\d+)-(\d+)$/);
    if (r) { for (let k = +r[1]; k <= +r[2]; k++) out.add(k); }
    else if (/^\d+$/.test(p)) out.add(+p);
  }
  return [...out].sort((a, b) => a - b);
}

const row: React.CSSProperties = { display: "flex", flexWrap: "wrap", gap: 4, margin: "6px 0" };
function btn(active: boolean): React.CSSProperties {
  return {
    flex: "1 0 auto", padding: "5px 7px", borderRadius: 6, cursor: "pointer", fontSize: 11,
    border: active ? "1px solid #5b8cff" : "1px solid #2a3a52",
    background: active ? "#21345e" : "#121a26", color: active ? "#dce6ff" : "#9fb0c7", textAlign: "center",
  };
}
const lbl: React.CSSProperties = { marginTop: 10, opacity: 0.7, fontSize: 11 };

export function GroupPanel() {
  const namedGroups = useTwin((s) => s.namedGroups);
  const selected = useTwin((s) => s.selectedGroup);
  const groupControls = useTwin((s) => s.groupControls);
  const groupActive = useTwin((s) => s.groupActive);
  const selectGroup = useTwin((s) => s.selectGroup);
  const setGroupControl = useTwin((s) => s.setGroupControl);
  const toggleGroupActive = useTwin((s) => s.toggleGroupActive);
  const defineGroup = useTwin((s) => s.defineGroup);
  const deleteGroup = useTwin((s) => s.deleteGroup);
  const [newName, setNewName] = useState("");
  const [newNums, setNewNums] = useState("");

  const names = Object.keys(namedGroups);
  const gc = { ...DEFAULT_GROUP_CONTROL, ...(groupControls[selected] ?? {}) };
  const active = !!groupActive[selected];
  const nums = namedGroups[selected] ?? [];
  const isPreset = selected in LABELS;
  const set = (p: Partial<typeof gc>) => setGroupControl(selected, p);

  return (
    <Widget id="groups" title="▤ Groups" x={980} y={12} w={300} h={460}>
      {/* GROUP selector (top row of the sketch) */}
      <div style={{ ...row, marginTop: 8 }}>
        {names.map((g) => (
          <button key={g} onClick={() => selectGroup(g)} style={btn(g === selected)}>
            {groupActive[g] ? "● " : ""}{labelOf(g)}
          </button>
        ))}
      </div>

      {/* custom group creator */}
      <div style={{ display: "flex", gap: 4, marginTop: 4 }}>
        <input value={newName} placeholder="new group" onChange={(e) => setNewName(e.target.value)}
          style={{ flex: 1, minWidth: 0, padding: "4px 6px", borderRadius: 5, border: "1px solid #2a3a52", background: "#0b1119", color: "#dce6ff", font: "11px ui-monospace, monospace" }} />
        <input value={newNums} placeholder="lights e.g. 1,4,7-9" onChange={(e) => setNewNums(e.target.value)}
          style={{ flex: 1.3, minWidth: 0, padding: "4px 6px", borderRadius: 5, border: "1px solid #2a3a52", background: "#0b1119", color: "#dce6ff", font: "11px ui-monospace, monospace" }} />
        <button title="create group" onClick={() => { const ns = parseNums(newNums); if (newName.trim() && ns.length) { defineGroup(newName.trim(), ns); setNewName(""); setNewNums(""); } }}
          style={{ ...btn(false), flex: "0 0 auto" }}>＋</button>
      </div>

      {/* ── selected group editor (the sketch's rows) ── */}
      <div style={{ marginTop: 10, padding: "8px 10px", background: "#0d141e", borderRadius: 8, border: "1px solid #233149" }}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
          <span style={{ fontWeight: 700, color: "#dce6ff" }}>{labelOf(selected)} · {nums.length} lights</span>
          <button onClick={() => toggleGroupActive(selected, !active)} style={{ ...btn(active), flex: "0 0 auto", fontWeight: 700 }}>
            {active ? "● live" : "○ off"}
          </button>
        </div>

        <div style={lbl}>Pattern</div>
        <div style={row}>
          {GROUP_PATTERNS.map((p) => (
            <button key={p} style={btn(gc.pattern === p)} onClick={() => set({ pattern: p })}>{p}</button>
          ))}
        </div>

        <div style={lbl}>Color</div>
        <div style={{ display: "flex", gap: 5, flexWrap: "wrap", alignItems: "center", margin: "6px 0" }}>
          {SWATCHES.map((sw) => {
            const sel = Math.abs((gc.hue ?? 0) - sw.h) < 0.02 && Math.abs((gc.sat ?? 1) - sw.s) < 0.06 && gc.colorCycle === "off";
            return (
              <button key={sw.n} title={sw.n} onClick={() => set({ hue: sw.h, sat: sw.s, colorCycle: "off" })}
                style={{ width: 24, height: 24, borderRadius: "50%", cursor: "pointer", padding: 0,
                  background: sw.s === 0 ? "#f4f6fb" : `hsl(${sw.h * 360},85%,55%)`,
                  border: sel ? "2px solid #fff" : "1px solid #2a3a52" }} />
            );
          })}
        </div>
        <div style={row}>
          {COLOR_CYCLES.map((m: ColorCycle) => (
            <button key={m} style={{ ...btn(gc.colorCycle === m), fontSize: 10 }} onClick={() => set({ colorCycle: m })}>
              {m === "off" ? "● hold" : m === "rainbow" ? "🌈" : m === "group" ? "family" : m === "shade" ? "shades" : "🎲 per-light"}
            </button>
          ))}
        </div>

        <div style={lbl}>Direction</div>
        <div style={row}>
          <button style={btn(!gc.reverse)} onClick={() => set({ reverse: false })}>▶ forward</button>
          <button style={btn(!!gc.reverse)} onClick={() => set({ reverse: true })}>◀ reverse</button>
        </div>

        <div style={lbl}>Speed · {(gc.speed ?? 1).toFixed(2)}</div>
        <input type="range" min={0} max={3} step={0.01} value={gc.speed ?? 1}
          onChange={(e) => set({ speed: parseFloat(e.target.value) })} style={{ width: "100%" }} />

        <div style={lbl}>Mode</div>
        <div style={row}>
          {LIGHT_ORDERS.map((o: LightOrder) => (
            <button key={o} style={btn(gc.order === o)} onClick={() => set({ order: o })}>{o}</button>
          ))}
        </div>

        <div style={{ marginTop: 8, fontSize: 9.5, opacity: 0.5, wordBreak: "break-all" }}>
          lights: {nums.slice(0, 30).join(",")}{nums.length > 30 ? "…" : ""}
        </div>
        {!isPreset && (
          <button onClick={() => deleteGroup(selected)} style={{ ...btn(false), marginTop: 6, color: "#ff8fa0", borderColor: "#5a3a3a" }}>
            delete group
          </button>
        )}
      </div>
    </Widget>
  );
}
