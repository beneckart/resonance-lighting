import { useEffect, useState } from "react";
import { useTwin } from "./store";
import { SHOWS, showById } from "./shows";
import { resetPiano, setPiece, PIECE_LIST, currentPiece } from "./piano";
import { setPianoSound } from "./pianoAudio";
import { Widget } from "./Widget";

const fmt = (s: number) => `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, "0")}`;

export function ShowsPanel() {
  const activeShow = useTwin((s) => s.activeShow);
  const startedAt = useTwin((s) => s.showStartedAt);
  const playShow = useTwin((s) => s.playShow);
  const set = useTwin((s) => s.set);
  const setTod = useTwin((s) => s.setTimeOfDay);
  const pianoOn = useTwin((s) => s.control.pattern === "piano");
  const [soundOn, setSoundOn] = useState(false);
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
    <Widget id="shows" title="🎬 Light Shows" x={344} y={12} w={216} h={330}>
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
      <div style={{ marginTop: 8, display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <span style={{ fontWeight: 700, color: "#eef3fb", fontSize: 12 }}>🎹 Piano · 72 keys</span>
        <button onClick={() => { const v = !soundOn; setSoundOn(v); setPianoSound(v); }} title="play sound (to your speaker / Bluetooth)"
          style={{ padding: "2px 8px", borderRadius: 6, cursor: "pointer", fontSize: 11, border: soundOn ? "1px solid #3ddc97" : "1px solid #2a3a52", background: soundOn ? "#10241c" : "#141a26", color: soundOn ? "#7af0c0" : "#9fb0c7" }}>
          {soundOn ? "🔊 sound on" : "🔇 sound"}
        </button>
      </div>
      <div style={{ display: "flex", flexWrap: "wrap", gap: 4, marginTop: 4 }}>
        {PIECE_LIST.map((p) => {
          const on = pianoOn && currentPiece() === p.id;
          return (
            <button key={p.id} onClick={() => { playShow(null); setPiece(p.id); resetPiano(); setPianoSound(true); setSoundOn(true); set({ pattern: "piano", brightness: 0.95, sat: 0.6, colorCycle: "off", reverse: false }); setTod(0); }}
              style={{ flex: "1 0 auto", padding: "5px 8px", borderRadius: 6, cursor: "pointer", fontSize: 11,
                border: on ? "1px solid #5b8cff" : "1px solid #2a3a52", background: on ? "#21345e" : "#121a26", color: on ? "#dce6ff" : "#9fb0c7" }}>
              {on ? "▶ " : ""}{p.name}
            </button>
          );
        })}
        {pianoOn && (
          <button onClick={() => set({ pattern: "solid" })}
            style={{ flex: "1 0 auto", padding: "5px 8px", borderRadius: 6, cursor: "pointer", fontSize: 11, border: "1px solid #5a3a3a", background: "#1a1016", color: "#ff8fa0" }}>⏹ stop</button>
        )}
      </div>
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
    </Widget>
  );
}
