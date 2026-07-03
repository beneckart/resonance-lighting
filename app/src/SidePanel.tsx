import { useRef, useState, type ReactNode } from "react";
import { useTwin, UI_MODES, type UiMode } from "./store";
import { DockCtx } from "./Widget";
import { Controls } from "./Controls";
import { GroupPanel } from "./GroupPanel";
import { ShowsPanel } from "./ShowsPanel";
import { InteractivityPanel } from "./InteractivityPanel";
import { DataLog } from "./DataLog";
import { AiPilot } from "./AiPilot";
import { CommissioningPanel } from "./CommissioningPanel";

/** THE DOCK — Elliot's split-screen layout: tree on the left, ONE organized,
 *  scrollable panel on the right. Pick the MODE first; the panel shows only that
 *  mode's controls, grouped by type, each section collapsible and drag-to-reorder
 *  (order persists). "Float" returns to the old free-floating widgets. */

const MODE_META: Record<UiMode, { label: string; emoji: string; blurb: string; accent: string }> = {
  interactive: { label: "Interactive", emoji: "🌱", blurb: "the tree is REACTIVE — people trigger it; you only set the rules", accent: "#3ddc97" },
  lightshow: { label: "Light Show", emoji: "🎬", blurb: "you drive it — whole tree, a group, or a single light; build a show or play one", accent: "#5b8cff" },
  sound: { label: "Sound", emoji: "🎵", blurb: "the tree is reactive to MUSIC — DJ decks, AI-VJ, audio-reactive", accent: "#ff7b54" },
  calibrate: { label: "Calibrate", emoji: "🔧", blurb: "commissioning, positions, testing & health", accent: "#c8a24a" },
};

// sections per mode — keys are stable ids for order persistence
const SECTIONS: Record<UiMode, { key: string; el: ReactNode }[]> = {
  interactive: [
    { key: "rules", el: <InteractivityPanel /> },
    { key: "datalog", el: <DataLog /> },
  ],
  lightshow: [
    { key: "scope", el: <GroupPanel /> }, // whole tree · custom groups · single lights
    { key: "controls", el: <Controls /> }, // sliders: colour · brightness · speed · patterns
    { key: "shows", el: <ShowsPanel /> }, // pre-designed shows + piano (build-your-own: cues)
  ],
  sound: [
    { key: "aivj", el: <AiPilot /> },
    { key: "controls", el: <Controls /> }, // EQ/strobe/auto-VJ toggles live here too
  ],
  calibrate: [
    { key: "commission", el: <CommissioningPanel /> },
    { key: "datalog", el: <DataLog /> },
  ],
};

function loadOrder(mode: UiMode, keys: string[]): string[] {
  try {
    const raw = localStorage.getItem("dock.order." + mode);
    if (raw) {
      const saved = JSON.parse(raw) as string[];
      const valid = saved.filter((k) => keys.includes(k));
      return [...valid, ...keys.filter((k) => !valid.includes(k))];
    }
  } catch { /* ignore */ }
  return keys;
}

export function SidePanel() {
  const uiMode = useTwin((s) => s.uiMode);
  const setUiMode = useTwin((s) => s.setUiMode);
  const setDock = useTwin((s) => s.setDock);
  const namedGroups = useTwin((s) => s.namedGroups);
  const groupModes = useTwin((s) => s.groupModes);
  const scope = useTwin((s) => s.selectedScope);
  const setScope = useTwin((s) => s.setSelectedScope);
  const setGroupMode = useTwin((s) => s.setGroupMode);
  const defs = SECTIONS[uiMode];
  const [order, setOrder] = useState<string[]>(() => loadOrder(uiMode, defs.map((d) => d.key)));
  const [prevMode, setPrevMode] = useState(uiMode);
  if (prevMode !== uiMode) { // mode switched → load that mode's saved order
    setPrevMode(uiMode);
    setOrder(loadOrder(uiMode, defs.map((d) => d.key)));
  }
  const [dragKey, setDragKey] = useState<string | null>(null);
  const dragRef = useRef<string | null>(null); // synchronous mirror of dragKey (pointer events outrun state)
  const rowRefs = useRef<Record<string, HTMLDivElement | null>>({});

  const commitOrder = (next: string[]) => {
    setOrder(next);
    try { localStorage.setItem("dock.order." + uiMode, JSON.stringify(next)); } catch { /* ignore */ }
  };

  // pointer drag-to-reorder: grab a section's ⠿ grip, drag past a neighbour's
  // midpoint to swap. Touch-friendly (pointer events + capture).
  const onGripDown = (key: string) => (e: React.PointerEvent) => {
    dragRef.current = key;
    setDragKey(key);
    try { (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId); } catch { /* fine — capture is best-effort */ }
  };
  const onGripMove = (key: string) => (e: React.PointerEvent) => {
    if (dragRef.current !== key) return;
    const idx = order.indexOf(key);
    const y = e.clientY;
    if (idx > 0) {
      const above = rowRefs.current[order[idx - 1]];
      if (above && y < above.getBoundingClientRect().top + above.offsetHeight / 2) {
        const next = [...order]; next[idx] = next[idx - 1]; next[idx - 1] = key; commitOrder(next); return;
      }
    }
    if (idx < order.length - 1) {
      const below = rowRefs.current[order[idx + 1]];
      if (below && y > below.getBoundingClientRect().top + below.offsetHeight / 2) {
        const next = [...order]; next[idx] = next[idx + 1]; next[idx + 1] = key; commitOrder(next);
      }
    }
  };
  const onGripUp = () => { dragRef.current = null; setDragKey(null); };

  const meta = MODE_META[uiMode];
  const ordered = order.map((k) => defs.find((d) => d.key === k)).filter(Boolean) as { key: string; el: ReactNode }[];

  return (
    <div style={{ position: "fixed", top: 0, right: 0, bottom: 0, width: "50%", zIndex: 40, display: "flex", flexDirection: "column", background: "rgba(7,10,15,0.97)", borderLeft: "1px solid #1d2735", backdropFilter: "blur(8px)" }}>
      {/* ── SCOPE FIRST (Elliot: groups sit ABOVE modes) — pick who you're driving:
            the whole tree or one group. Each group can hold a DIFFERENT mode. ── */}
      <div style={{ padding: "10px 12px 8px", borderBottom: "1px solid #16202e" }}>
        <div style={{ display: "flex", gap: 4, flexWrap: "wrap", marginBottom: 8 }}>
          {["all", ...Object.keys(namedGroups).filter((g) => g !== "all")].map((g) => {
            const on = scope === g;
            const gm = groupModes[g];
            const dotColor = gm === "interactive" ? "#3ddc97" : gm === "sound" ? "#ff7b54" : gm === "lightshow" ? "#5b8cff" : null;
            return (
              <button key={g} onClick={() => setScope(g)}
                style={{ padding: "6px 10px", minHeight: 32, borderRadius: 8, cursor: "pointer", fontSize: 11, fontWeight: 700,
                  border: on ? "1.5px solid #cdd6e4" : "1px solid #2a3a52", background: on ? "#1a2434" : "#0c121c",
                  color: on ? "#eef3fb" : "#8aa0bb", font: "11px ui-monospace, monospace" }}>
                {g === "all" ? "🌳 whole tree" : g}
                {dotColor && <span style={{ marginLeft: 5, color: dotColor }}>●</span>}
              </button>
            );
          })}
        </div>
        {/* mode tabs — assign the SELECTED scope's mode */}
        <div style={{ display: "flex", gap: 6 }}>
          {UI_MODES.filter((m) => scope === "all" || m !== "calibrate").map((m) => {
            const mm = MODE_META[m];
            const on = scope === "all" ? uiMode === m : groupModes[scope] === m;
            return (
              <button key={m} onClick={() => (scope === "all" ? setUiMode(m) : setGroupMode(scope, m))}
                style={{ flex: 1, minHeight: 44, padding: "6px 4px", borderRadius: 10, cursor: "pointer", fontWeight: 700, fontSize: 12,
                  border: on ? `1.5px solid ${mm.accent}` : "1px solid #2a3a52", background: on ? "#101c2a" : "#0c121c",
                  color: on ? mm.accent : "#8aa0bb", font: "12px ui-monospace, monospace" }}>
                <div style={{ fontSize: 16 }}>{mm.emoji}</div>
                {mm.label}
              </button>
            );
          })}
          {scope !== "all" && (
            <button onClick={() => setGroupMode(scope, "follow")}
              title="this group follows whatever the whole tree does"
              style={{ flex: 0.7, minHeight: 44, padding: "6px 4px", borderRadius: 10, cursor: "pointer", fontWeight: 700, fontSize: 11,
                border: !groupModes[scope] || groupModes[scope] === "follow" ? "1.5px solid #8aa0bb" : "1px solid #2a3a52",
                background: "#0c121c", color: "#8aa0bb", font: "11px ui-monospace, monospace" }}>
              <div style={{ fontSize: 14 }}>↩</div>
              follow
            </button>
          )}
        </div>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginTop: 6 }}>
          <span style={{ fontSize: 10, color: "#7a8ba3", font: "10px ui-monospace, monospace" }}>
            {scope === "all" ? meta.blurb : `assign ${scope} its own mode — it runs ALONGSIDE the rest of the tree`}
          </span>
          <button onClick={() => setDock(false)} title="switch to free-floating widgets"
            style={{ padding: "4px 9px", borderRadius: 7, cursor: "pointer", fontSize: 10.5, border: "1px solid #2a3a52", background: "#0c121c", color: "#8aa0bb", font: "10.5px ui-monospace, monospace" }}>
            ⧉ float
          </button>
        </div>
      </div>
      {/* ── the mode's sections — scrollable, collapsible, drag ⠿ to reorder ── */}
      <div style={{ flex: 1, overflowY: "auto", padding: "10px 12px 90px", overscrollBehavior: "contain", WebkitOverflowScrolling: "touch" }}>
        <DockCtx.Provider value={true}>
          {ordered.map(({ key, el }) => (
            <div key={key} ref={(r) => { rowRefs.current[key] = r; }}
              style={{ display: "flex", gap: 6, alignItems: "stretch", opacity: dragKey === key ? 0.75 : 1 }}>
              <div onPointerDown={onGripDown(key)} onPointerMove={onGripMove(key)} onPointerUp={onGripUp}
                title="drag to reorder"
                style={{ flex: "0 0 22px", display: "flex", alignItems: "center", justifyContent: "center", marginBottom: 8, borderRadius: 8, cursor: "grab", color: "#46577a", background: dragKey === key ? "#16202e" : "transparent", touchAction: "none", userSelect: "none", fontSize: 13 }}>
                ⠿
              </div>
              <div style={{ flex: 1, minWidth: 0 }}>{el}</div>
            </div>
          ))}
        </DockCtx.Provider>
      </div>
    </div>
  );
}
