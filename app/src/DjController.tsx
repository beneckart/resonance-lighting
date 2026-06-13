import { useEffect, useRef, useState } from "react";
import { PATTERN_IDS, ELEMENT_MODES, useTwin, type PatternId } from "./store";
import { startMic, startTrack, stopAudio, audioFeatures, listAudioInputs, type AudioInput } from "./audio";

/** A real-feeling virtual DJ controller (C): two spinning platters, crossfader,
 *  3-band EQ, tempo, transport + a performance-pad grid. Wired to the twin store
 *  and the live audio engine — platters spin at the detected BPM and the beat
 *  LED + pads pulse on transients. Hardware controllers come later; this is the
 *  on-screen surface for now. */

const ACCENT = "#5b8cff";
const PADS: PatternId[] = [...PATTERN_IDS, ...ELEMENT_MODES]; // 12 performance pads

const console_: React.CSSProperties = {
  position: "fixed",
  bottom: 0,
  left: "50%",
  transform: "translateX(-50%)",
  width: "min(880px, 96vw)",
  padding: "6px 16px 8px",
  maxHeight: "48vh",
  overflowY: "auto",
  background: "linear-gradient(180deg,#10141d 0%,#0a0d14 100%)",
  border: "1px solid #1d2735",
  borderBottom: "none",
  borderRadius: "12px 12px 0 0",
  color: "#cdd6e4",
  font: "11px ui-monospace, SFMono-Regular, monospace",
  boxShadow: "0 -8px 28px rgba(0,0,0,0.55)",
  display: "grid",
  gridTemplateColumns: "1fr auto 1fr",
  gap: 10,
  alignItems: "start",
  zIndex: 20,
  backdropFilter: "blur(8px)",
};

/** Touch/mouse draggable knob — drag up/down (or scroll) to turn. Pointer capture
 *  + touchAction:none so it works on tablets without stealing the scroll. */
function Knob({ v, label, on, color = ACCENT }: { v: number; label: string; on: (v: number) => void; color?: string }) {
  const deg = -135 + v * 270; // 0..1 → -135..135°
  const drag = useRef<{ y: number; v: number } | null>(null);
  const clamp = (x: number) => Math.max(0, Math.min(1, x));
  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 3, flex: 1 }}>
      <div
        onPointerDown={(e) => { (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId); drag.current = { y: e.clientY, v }; }}
        onPointerMove={(e) => { if (drag.current) on(clamp(drag.current.v + (drag.current.y - e.clientY) / 140)); }}
        onPointerUp={(e) => { (e.currentTarget as HTMLElement).releasePointerCapture?.(e.pointerId); drag.current = null; }}
        onWheel={(e) => on(clamp(v - Math.sign(e.deltaY) * 0.06))}
        onDoubleClick={() => on(0)}
        style={{
          width: 38, height: 38, borderRadius: "50%", touchAction: "none",
          background: "radial-gradient(circle at 50% 38%,#1c2433,#0c111a)",
          border: `1px solid #2a3a52`, position: "relative", cursor: "ns-resize",
        }}
        title={`${label} — drag up/down (double-click 0)`}
      >
        <div style={{
          position: "absolute", left: "50%", top: 4, width: 2, height: 13,
          background: color, transformOrigin: "50% 15px", transform: `translateX(-50%) rotate(${deg}deg)`,
          borderRadius: 2, boxShadow: `0 0 5px ${color}`,
        }} />
      </div>
      <span style={{ color: "#7e8ea6", fontSize: 9 }}>{label} {Math.round(v * 100)}</span>
    </div>
  );
}

function Deck({ side }: { side: "A" | "B" }) {
  const control = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const platter = useRef<HTMLDivElement>(null);
  const angle = useRef(0);
  const jog = useRef<{ a: number; t: number } | null>(null); // last pointer angle+time while scrubbing
  const dragging = useRef(false);
  const pattern = side === "A" ? control.pattern : control.djPatternB;

  useEffect(() => {
    let raf = 0;
    let last = 0;
    const loop = (t: number) => {
      const dt = last ? (t - last) / 1000 : 0;
      last = t;
      if (!dragging.current) {
        // idle/auto spin (BPM-driven) when not being scrubbed by the operator
        const bpm = audioFeatures.active && audioFeatures.bpm ? audioFeatures.bpm : 33.3;
        const beatBoost = audioFeatures.active ? 1 + audioFeatures.beat * 0.8 : 1;
        const cur = useTwin.getState().control;
        const sign = cur.reverse ? -1 : 1;
        angle.current = (angle.current + dt * (bpm / 60) * 360 * 0.25 * beatBoost * sign) % 360;
      }
      if (platter.current) platter.current.style.transform = `rotate(${angle.current}deg)`;
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, []);

  // jog-wheel scrub: drag AROUND the platter → spin the lights around the tree.
  // angular velocity → speed; direction → reverse; pointer pressure → intensity.
  const angOf = (e: React.PointerEvent) => {
    const r = (e.currentTarget as HTMLElement).getBoundingClientRect();
    return Math.atan2(e.clientY - (r.top + r.height / 2), e.clientX - (r.left + r.width / 2));
  };
  const onJogDown = (e: React.PointerEvent) => {
    (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    dragging.current = true;
    jog.current = { a: angOf(e), t: e.timeStamp };
  };
  const onJogMove = (e: React.PointerEvent) => {
    if (!dragging.current || !jog.current) return;
    const a = angOf(e);
    let d = a - jog.current.a;
    if (d > Math.PI) d -= 2 * Math.PI; else if (d < -Math.PI) d += 2 * Math.PI; // shortest arc
    const dt = Math.max(1, e.timeStamp - jog.current.t) / 1000;
    jog.current = { a, t: e.timeStamp };
    angle.current = (angle.current + (d * 180) / Math.PI) % 360; // platter follows the finger
    const press = e.pressure > 0 ? 0.5 + e.pressure : 1; // touch pressure → intensity (1 on non-pressure)
    const vel = (d / dt) * press; // rad/s, signed
    set({ speed: Math.max(0.05, Math.min(3, Math.abs(vel) * 0.5)), reverse: vel < 0 });
  };
  const onJogUp = (e: React.PointerEvent) => {
    (e.currentTarget as HTMLElement).releasePointerCapture?.(e.pointerId);
    dragging.current = false;
    jog.current = null;
  };

  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6 }}>
      <div style={{ display: "flex", justifyContent: "space-between", width: "100%", alignItems: "center" }}>
        <span style={{ color: ACCENT, fontWeight: 700 }}>DECK {side}</span>
        <span style={{ color: "#7e8ea6" }}>{pattern}</span>
      </div>
      {/* spinning jog wheel — drag around it to spin the lights round the tree */}
      <div
        onPointerDown={onJogDown}
        onPointerMove={onJogMove}
        onPointerUp={onJogUp}
        title="spin to drive speed + direction of the lights around the tree"
        style={{
          width: 60, height: 60, borderRadius: "50%", position: "relative",
          background: "radial-gradient(circle at 50% 50%,#161d2b 0%,#0b0f17 70%)",
          border: "2px solid #243049", boxShadow: "inset 0 0 18px rgba(0,0,0,0.7)",
          touchAction: "none", cursor: "grab",
        }}
      >
        <div ref={platter} style={{ position: "absolute", inset: 6, borderRadius: "50%" }}>
          <div style={{
            position: "absolute", left: "50%", top: 4, width: 3, height: "46%",
            background: `linear-gradient(${ACCENT},transparent)`, transform: "translateX(-50%)", borderRadius: 3,
          }} />
          <div style={{
            position: "absolute", inset: "30%", borderRadius: "50%",
            border: "1px dashed #2a3a52",
          }} />
        </div>
        <div style={{
          position: "absolute", left: "50%", top: "50%", width: 16, height: 16, borderRadius: "50%",
          transform: "translate(-50%,-50%)", background: "#1c2433", border: "1px solid #34456a",
        }} />
      </div>
      {/* tempo for A = global speed; pitch slider feel */}
      <input
        type="range" min={0} max={3} step={0.01} value={control.speed}
        onChange={(e) => set({ speed: Number(e.target.value) })}
        style={{ width: 60, accentColor: ACCENT }}
        title="tempo / speed"
      />
      <span style={{ color: "#7e8ea6", fontSize: 9 }}>tempo {control.speed.toFixed(2)}×</span>
    </div>
  );
}

export function DjController() {
  const control = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const [beatLed, setBeatLed] = useState(false);
  const [audioOn, setAudioOn] = useState(false);
  const ledRef = useRef(0);
  const waveRef = useRef<HTMLCanvasElement>(null);
  const [inputs, setInputs] = useState<AudioInput[]>([]);
  const [deviceId, setDeviceId] = useState("");
  const [collapsed, setCollapsed] = useState(false);

  useEffect(() => { listAudioInputs().then(setInputs).catch(() => {}); }, []);

  // scrolling waveform on the deck — live audio level history, tinted by treble,
  // with a red tick on each beat (idles with a faint pulse when no audio)
  useEffect(() => {
    const cv = waveRef.current;
    const g = cv?.getContext("2d");
    if (!cv || !g) return;
    const W = cv.width, H = cv.height;
    const hist = new Float32Array(W);
    let raf = 0;
    const loop = () => {
      const a = audioFeatures;
      hist.copyWithin(0, 1);
      hist[W - 1] = a.active ? a.level : 0.05 + 0.03 * (0.5 + 0.5 * Math.sin(performance.now() / 380));
      g.fillStyle = "#0b0f17";
      g.fillRect(0, 0, W, H);
      g.beginPath();
      g.moveTo(0, H);
      for (let x = 0; x < W; x++) g.lineTo(x, H - hist[x] * H);
      g.lineTo(W, H);
      g.closePath();
      const hue = a.active ? 210 - a.treble * 170 : 210;
      g.fillStyle = `hsla(${hue},80%,56%,0.75)`;
      g.fill();
      if (a.active && a.beat > 0.6) { g.fillStyle = "rgba(255,91,110,0.6)"; g.fillRect(W - 2, 0, 2, H); }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, []);

  // beat LED pulse from the live audio engine
  useEffect(() => {
    let raf = 0;
    const loop = () => {
      const now = performance.now();
      if (audioFeatures.active && audioFeatures.beat > 0.5 && now - ledRef.current > 120) {
        ledRef.current = now;
        setBeatLed(true);
        setTimeout(() => setBeatLed(false), 90);
      }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, []);

  const padBtn = (p: PatternId) => {
    const active = control.pattern === p;
    return (
      <button
        key={p}
        aria-label={`pad ${p}`}
        onClick={() => set({ pattern: p })}
        style={{
          padding: "4px 3px", borderRadius: 5, cursor: "pointer", fontSize: 9.5,
          border: active ? `1px solid ${ACCENT}` : "1px solid #233149",
          background: active ? "#21345e" : "linear-gradient(180deg,#151d2b,#0d131d)",
          color: active ? "#dce6ff" : "#9fb0c7",
          boxShadow: active ? `0 0 8px ${ACCENT}` : "none",
          textTransform: "uppercase", letterSpacing: 0.3,
        }}
      >
        {p}
      </button>
    );
  };

  const toggle = (on: boolean, label: string, fn: () => void, color = ACCENT) => (
    <button
      onClick={fn}
      style={{
        flex: 1, padding: "6px 4px", borderRadius: 5, cursor: "pointer", fontSize: 9.5,
        border: on ? `1px solid ${color}` : "1px solid #233149",
        background: on ? "#21345e" : "#0d131d", color: on ? "#dce6ff" : "#9fb0c7",
        boxShadow: on ? `0 0 8px ${color}` : "none",
      }}
    >
      {label}
    </button>
  );

  // collapsed: a slim bar with just an expand button (so the tree is unobscured)
  if (collapsed) {
    return (
      <button
        onClick={() => setCollapsed(false)}
        title="show DJ console"
        style={{
          position: "fixed", bottom: 0, left: "50%", transform: "translateX(-50%)", zIndex: 20,
          padding: "5px 16px", borderRadius: "10px 10px 0 0", cursor: "pointer",
          border: "1px solid #1d2735", borderBottom: "none", background: "rgba(16,20,29,0.92)",
          color: "#9fb0c7", font: "11px ui-monospace, monospace", backdropFilter: "blur(8px)",
        }}
      >🎛 DJ ▲</button>
    );
  }

  return (
    <div style={console_}>
      <button
        onClick={() => setCollapsed(true)}
        title="collapse DJ console"
        style={{
          position: "absolute", top: 6, right: 8, zIndex: 1, width: 22, height: 22, borderRadius: 6,
          border: "1px solid #2a3a52", background: "#141a26", color: "#9fb0c7", cursor: "pointer", fontSize: 11,
        }}
      >▾</button>
      <Deck side="A" />

      {/* center: mixer — EQ, crossfader, transport, pads */}
      <div style={{ width: "min(420px, 50vw)", display: "flex", flexDirection: "column", gap: 5 }}>
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
          <span style={{ color: "#7e8ea6" }}>MIXER</span>
          <span style={{ display: "flex", alignItems: "center", gap: 6 }}>
            <span style={{
              width: 9, height: 9, borderRadius: "50%",
              background: beatLed ? "#ff5b6e" : "#33211f",
              boxShadow: beatLed ? "0 0 8px #ff5b6e" : "none", transition: "background 60ms",
            }} />
            <span style={{ color: "#7e8ea6", fontSize: 9 }}>
              {audioFeatures.active ? `${Math.round(audioFeatures.bpm) || "–"} BPM` : "no audio"}
            </span>
          </span>
        </div>

        {/* scrolling waveform */}
        <canvas ref={waveRef} width={420} height={34}
          style={{ width: "100%", height: 34, borderRadius: 6, display: "block", border: "1px solid #1d2735" }} />

        {/* 3-band EQ */}
        <div style={{ display: "flex", gap: 6 }}>
          <Knob v={control.eqLow} label="LOW" on={(v) => set({ eqLow: v })} color="#ff7a59" />
          <Knob v={control.eqMid} label="MID" on={(v) => set({ eqMid: v })} color="#ffd166" />
          <Knob v={control.eqHigh} label="HIGH" on={(v) => set({ eqHigh: v })} color="#5be0ff" />
          <Knob v={control.master} label="MASTER" on={(v) => set({ master: v })} />
        </div>

        {/* crossfader A↔B */}
        <div>
          <div style={{ display: "flex", justifyContent: "space-between", color: "#7e8ea6", fontSize: 9 }}>
            <span>A</span><span>crossfade</span><span>B</span>
          </div>
          <input
            type="range" min={0} max={1} step={0.01} value={control.xfade}
            onChange={(e) => set({ xfade: Number(e.target.value) })}
            style={{ width: "100%", accentColor: ACCENT }}
          />
        </div>

        {/* transport */}
        <div style={{ display: "flex", gap: 5 }}>
          {toggle(control.syncToBeat, "🥁 SYNC", () => set({ syncToBeat: !control.syncToBeat }))}
          {toggle(control.strobe, "⚡ STROBE", () => set({ strobe: !control.strobe }), "#ff5b6e")}
          {toggle(control.autoVj, "🤖 AUTO", () => set({ autoVj: !control.autoVj }), "#9b6bff")}
          {toggle(control.audioSpeed, "🎵 RX-SPD", () => set({ audioSpeed: !control.audioSpeed }), "#3ddc97")}
          {toggle(audioOn, audioOn ? "■ STOP" : "▶ TRACK", () => {
            if (audioOn) { stopAudio(); setAudioOn(false); }
            else { startTrack("/audio/test-beat-124bpm.wav"); setAudioOn(true); }
          }, "#3ddc97")}
          {toggle(false, "🎤 MIC", () => { startMic(deviceId || undefined); setAudioOn(true); })}
        </div>

        {/* audio source / input device picker (#5) */}
        <div style={{ display: "flex", gap: 5, alignItems: "center" }}>
          <span style={{ color: "#7e8ea6", fontSize: 9 }}>🎚 source</span>
          <select
            value={deviceId}
            onChange={(e) => setDeviceId(e.target.value)}
            onMouseDown={() => listAudioInputs().then(setInputs).catch(() => {})}
            style={{ flex: 1, minWidth: 0, padding: "3px 5px", borderRadius: 5, fontSize: 9.5, border: "1px solid #233149", background: "#0d131d", color: "#9fb0c7" }}
          >
            <option value="">default mic / line-in</option>
            {inputs.map((d) => <option key={d.id} value={d.id}>{d.label}</option>)}
          </select>
        </div>

        {/* performance pads — all patterns + element modes */}
        <div style={{ display: "grid", gridTemplateColumns: "repeat(6,1fr)", gap: 4 }}>
          {PADS.map(padBtn)}
        </div>
      </div>

      <Deck side="B" />
    </div>
  );
}
