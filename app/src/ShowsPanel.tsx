import { useEffect, useState } from "react";
import { useTwin } from "./store";
import { SHOWS, showById } from "./shows";
import { resetPiano } from "./piano";

const panel: React.CSSProperties = {
  position: "fixed", top: 12, left: 344, width: 208, padding: "10px 12px",
  background: "rgba(10,14,20,0.86)", border: "1px solid #1d2735", borderRadius: 10,
  color: "#cdd6e4", font: "11px ui-monospace, SFMono-Regular, monospace", backdropFilter: "blur(6px)", zIndex: 44,
};
const fmt = (s: number) => `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, "0")}`;

export function ShowsPanel() {
  const activeShow = useTwin((s) => s.activeShow);
  const startedAt = useTwin((s) => s.showStartedAt);
  const playShow = useTwin((s) => s.playShow);
  const set = useTwin((s) => s.set);
  const setTod = useTwin((s) => s.setTimeOfDay);
  const pianoOn = useTwin((s) => s.control.pattern === "piano");
  const [, setTick] = useState(0);
  useEffect(() => {
    if (!activeShow) return;
    const id = setInterval(() => setTick((x) => x + 1), 400);
    return () => clearInterval(id);
  }, [activeShow]);

  const show = showById(activeShow);
  const elapsed = show ? Math.min(show.durationS, performance.now() / 1000 - startedAt) : 0;
  let cueNote = "";
  if (show) {
    for (const c of show.cues) if (c.at <= elapsed) cueNote = c.note;
  }

  return (
    <div style={panel}>
      <div style={{ fontWeight: 700, fontSize: 13, color: "#eef3fb", marginBottom: 6 }}>🎬 Light Shows</div>
      {SHOWS.map((s) => {
        const on = activeShow === s.id;
        return (
          <button key={s.id} onClick={() => playShow(on ? null : s.id)}
            style={{ display: "block", width: "100%", textAlign: "left", margin: "4px 0", padding: "6px 8px", borderRadius: 7, cursor: "pointer",
              border: on ? "1px solid #5b8cff" : "1px solid #2a3a52", background: on ? "#21345e" : "#121a26", color: on ? "#dce6ff" : "#9fb0c7" }}>
            <div style={{ fontWeight: 700 }}>{on ? "⏹ " : "▶ "}{s.name}</div>
            <div style={{ fontSize: 9.5, opacity: 0.7 }}>{s.vibe} · 5 min</div>
          </button>
        );
      })}
      <button onClick={() => {
        if (pianoOn) { set({ pattern: "solid" }); }
        else { playShow(null); resetPiano(); set({ pattern: "piano", brightness: 0.95, sat: 0.6, colorCycle: "off", reverse: false }); setTod(0); }
      }}
        style={{ display: "block", width: "100%", textAlign: "left", margin: "4px 0", padding: "6px 8px", borderRadius: 7, cursor: "pointer",
          border: pianoOn ? "1px solid #5b8cff" : "1px solid #2a3a52", background: pianoOn ? "#21345e" : "#121a26", color: pianoOn ? "#dce6ff" : "#9fb0c7" }}>
        <div style={{ fontWeight: 700 }}>{pianoOn ? "⏹ " : "🎹 "}Moonlight Sonata</div>
        <div style={{ fontSize: 9.5, opacity: 0.7 }}>the canopy plays the keys · 72-key piano</div>
      </button>
      {show && (
        <div style={{ marginTop: 6 }}>
          <div style={{ height: 4, background: "#1a2233", borderRadius: 3, overflow: "hidden" }}>
            <div style={{ height: "100%", width: `${(elapsed / show.durationS) * 100}%`, background: "#5b8cff" }} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", fontSize: 9.5, opacity: 0.7, marginTop: 3 }}>
            <span>{fmt(elapsed)} / {fmt(show.durationS)}</span><span>↻ loops</span>
          </div>
          <div style={{ fontSize: 10, color: "#9fc0ff", marginTop: 3 }}>▸ {cueNote}</div>
        </div>
      )}
    </div>
  );
}
