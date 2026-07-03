import { useContext, useEffect, useRef, useState } from "react";
import { useTwin } from "./store";
import { DockCtx } from "./Widget";
import { audioFeatures } from "./audio";
import { decideLook, energyOf, type AiDecision } from "./aivj";
import { phraseSeconds } from "./autovj";

/** The visible AI auto-pilot (PRD #32): a panel that shows the live audio digest
 *  + the AI's current decision, and — when ON — drives the show by re-deciding
 *  the look each phrase (and instantly on a drop) via the audio→light policy. */
const ACCENT = "#9b6bff";
const panel: React.CSSProperties = {
  position: "fixed", top: 12, right: 12, width: 212, padding: "10px 12px",
  background: "rgba(12,10,20,0.86)", border: "1px solid #2a2350", borderRadius: 10,
  color: "#dcd6f4", font: "11px ui-monospace, SFMono-Regular, monospace",
  backdropFilter: "blur(6px)", zIndex: 15,
};

function Meter({ label, v, color }: { label: string; v: number; color: string }) {
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 6, margin: "2px 0" }}>
      <span style={{ width: 34, color: "#8a82a8", fontSize: 9 }}>{label}</span>
      <div style={{ flex: 1, height: 6, background: "#1a1630", borderRadius: 3, overflow: "hidden" }}>
        <div style={{ width: `${Math.round(Math.min(1, v) * 100)}%`, height: "100%", background: color }} />
      </div>
    </div>
  );
}

export function AiPilot() {
  const ai = useTwin((s) => s.control.aiPilot);
  const set = useTwin((s) => s.set);
  const [dec, setDec] = useState<AiDecision | null>(null);
  const [live, setLive] = useState({ bass: 0, mid: 0, treble: 0, level: 0, drop: 0, bpm: 0, active: false, section: "ambient" as string });
  const lastDrop = useRef(0);

  // drive the show while ON: re-decide each phrase + instantly on a drop edge
  useEffect(() => {
    if (!ai) return;
    let timer: number;
    const apply = () => {
      const d = decideLook(audioFeatures);
      setDec(d);
      set({ pattern: d.pattern, hue: d.hue, speed: d.speed });
    };
    const tick = () => {
      apply();
      const secs = phraseSeconds(audioFeatures.bpm, 4);
      timer = window.setTimeout(tick, Math.max(2500, secs * 1000));
    };
    tick();
    // fast poll for a drop edge → immediate re-decide
    const dropWatch = window.setInterval(() => {
      if (audioFeatures.active && audioFeatures.drop > 0.5 && performance.now() - lastDrop.current > 1200) {
        lastDrop.current = performance.now();
        apply();
      }
    }, 120);
    return () => { clearTimeout(timer); clearInterval(dropWatch); };
  }, [ai, set]);

  // live meters (always, so you can see the digest even before turning AI on)
  useEffect(() => {
    let raf = 0;
    const loop = () => {
      const a = audioFeatures;
      setLive({ bass: a.bass, mid: a.mid, treble: a.treble, level: a.level, drop: a.drop, bpm: a.bpm, active: a.active, section: a.section });
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, []);

  const docked = useContext(DockCtx);
  return (
    <div style={docked ? { ...panel, position: "static", width: "100%", marginBottom: 8, boxSizing: "border-box" as const } : panel}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <span style={{ color: ACCENT, fontWeight: 700 }}>🤖 AI auto-pilot</span>
        <button
          onClick={() => set({ aiPilot: !ai })}
          style={{
            padding: "3px 8px", borderRadius: 5, cursor: "pointer", fontSize: 10,
            border: ai ? `1px solid ${ACCENT}` : "1px solid #2a2350",
            background: ai ? "#2a1f55" : "#140f24", color: ai ? "#e7deff" : "#8a82a8",
            boxShadow: ai ? `0 0 8px ${ACCENT}` : "none",
          }}
        >
          {ai ? "ON" : "OFF"}
        </button>
      </div>
      <div style={{ fontSize: 9, color: "#8a82a8", margin: "2px 0 6px" }}>
        smart sound → light · {live.active ? `${Math.round(live.bpm) || "–"} BPM · ${live.section}` : "no audio (idle looks)"}
      </div>
      <Meter label="bass" v={live.bass} color="#ff7a59" />
      <Meter label="mid" v={live.mid} color="#ffd166" />
      <Meter label="treble" v={live.treble} color="#5be0ff" />
      <Meter label="level" v={live.level} color="#3ddc97" />
      <Meter label="drop" v={live.drop} color="#ff5b6e" />
      <div style={{ marginTop: 6, paddingTop: 6, borderTop: "1px solid #2a2350", fontSize: 10 }}>
        <div style={{ color: "#8a82a8", fontSize: 9 }}>decision · energy {energyOf(live as never).toFixed(2)}</div>
        <div style={{ color: ai ? "#e7deff" : "#6a6288" }}>{dec ? dec.reason : "turn ON to let the AI drive"}</div>
      </div>
    </div>
  );
}
