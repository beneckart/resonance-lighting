import { useEffect, useRef, useState, type ReactNode } from "react";

/** A draggable / resizable / collapsible panel frame. Wrap any panel's content in
 *  <Widget id title x y w h> and it becomes a movable widget whose position, size,
 *  and collapsed state persist to localStorage by `id`. Drag the title bar to move,
 *  the ◢ corner to resize, the ▾ to hide the body. */
interface WState { x: number; y: number; w: number; h: number; collapsed: boolean }

function load(id: string, def: WState): WState {
  try { const r = localStorage.getItem("widget." + id); if (r) return { ...def, ...JSON.parse(r) }; } catch { /* ignore */ }
  return def;
}
function persist(id: string, s: WState) { try { localStorage.setItem("widget." + id, JSON.stringify(s)); } catch { /* ignore */ } }

export function Widget({ id, title, x, y, w, h = 360, minW = 150, accent = "#5b8cff", children }: {
  id: string; title: string; x: number; y: number; w: number; h?: number; minW?: number; accent?: string; children: ReactNode;
}) {
  const [s, setS] = useState<WState>(() => load(id, { x, y, w, h, collapsed: false }));
  const drag = useRef<{ ox: number; oy: number; px: number; py: number } | null>(null);
  const rez = useRef<{ ow: number; oh: number; px: number; py: number } | null>(null);
  useEffect(() => { persist(id, s); }, [id, s]);

  const onDragDown = (e: React.PointerEvent) => {
    (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    drag.current = { ox: s.x, oy: s.y, px: e.clientX, py: e.clientY };
  };
  const onRezDown = (e: React.PointerEvent) => {
    e.stopPropagation();
    (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    rez.current = { ow: s.w, oh: s.h, px: e.clientX, py: e.clientY };
  };
  const onMove = (e: React.PointerEvent) => {
    if (drag.current) {
      const d = drag.current;
      setS((q) => ({ ...q, x: Math.max(0, d.ox + e.clientX - d.px), y: Math.max(0, d.oy + e.clientY - d.py) }));
    } else if (rez.current) {
      const r = rez.current;
      setS((q) => ({ ...q, w: Math.max(minW, r.ow + e.clientX - r.px), h: Math.max(70, r.oh + e.clientY - r.py) }));
    }
  };
  const onUp = (e: React.PointerEvent) => { (e.currentTarget as HTMLElement).releasePointerCapture?.(e.pointerId); drag.current = null; rez.current = null; };

  return (
    <div style={{ position: "fixed", left: s.x, top: s.y, width: s.w, zIndex: 50, background: "rgba(10,14,20,0.9)", border: "1px solid #1d2735", borderRadius: 10, color: "#cdd6e4", font: "12px ui-monospace, SFMono-Regular, monospace", backdropFilter: "blur(6px)", boxShadow: "0 6px 22px rgba(0,0,0,0.45)" }}>
      <div onPointerDown={onDragDown} onPointerMove={onMove} onPointerUp={onUp}
        style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8, padding: "6px 10px", cursor: "move", touchAction: "none", borderLeft: `3px solid ${accent}`, borderRadius: "10px 0 0 0", borderBottom: s.collapsed ? "none" : "1px solid #16202e" }}>
        <span style={{ fontWeight: 700, fontSize: 12, color: "#eef3fb", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>{title}</span>
        <button onPointerDown={(e) => e.stopPropagation()} onClick={() => setS((q) => ({ ...q, collapsed: !q.collapsed }))} title={s.collapsed ? "show" : "hide"}
          style={{ flex: "0 0 auto", padding: "1px 8px", borderRadius: 5, cursor: "pointer", border: "1px solid #2a3a52", background: "#141a26", color: "#9fb0c7", fontSize: 11 }}>
          {s.collapsed ? "▸" : "▾"}
        </button>
      </div>
      {!s.collapsed && (
        <div style={{ position: "relative" }}>
          <div style={{ height: s.h, overflowY: "auto", overflowX: "hidden", padding: "8px 10px" }}>{children}</div>
          <div onPointerDown={onRezDown} onPointerMove={onMove} onPointerUp={onUp} title="resize"
            style={{ position: "absolute", right: 2, bottom: 2, width: 16, height: 16, cursor: "nwse-resize", color: "#46577a", fontSize: 12, lineHeight: "16px", textAlign: "center", touchAction: "none", userSelect: "none" }}>◢</div>
        </div>
      )}
    </div>
  );
}
