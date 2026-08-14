import { useEffect, useMemo, useState } from "react";
import { PATTERN_IDS, ELEMENT_MODES, useTwin, type PatternId, type UiMode } from "./store";
import { SHOWS } from "./shows";
import { startMic, startTrack, stopAudio } from "./audio";
import { asset } from "./fixtures";

/** TOUCH CONSOLE v2 — "the tree in your hand" (Elliot 08-14: full mobile
 *  redesign; all four operator modes; intuitive).
 *
 *  A phone LANDS here (isPhoneLike → open by default). The live 3-D tree stays
 *  visible in the top of the screen; controls live in a bottom sheet with the
 *  SAME four modes as the desktop tabs — Interactive · Shows · Sound ·
 *  Calibrate — so the mental model never forks. BLACKOUT/BEACON ride every
 *  mode (safety is never a tab away). Signature: the active mode tab glows
 *  lantern-amber, the tree's own light; everything else stays quiet.
 *  ✕ drops to the full desktop UI. */

const AMBER = "#ffb454";
const PADS = [...PATTERN_IDS, ...ELEMENT_MODES] as PatternId[];

/** phone-detect at mount: coarse pointer OR narrow viewport → land in touch */
function isPhoneLike(): boolean {
  if (typeof window === "undefined" || !window.matchMedia) return false;
  return window.matchMedia("(max-width: 767px)").matches ||
    (window.matchMedia("(pointer: coarse)").matches && window.matchMedia("(max-width: 1023px)").matches);
}

const MODES: { id: UiMode; icon: string; label: string }[] = [
  { id: "interactive", icon: "🌱", label: "Interactive" },
  { id: "lightshow", icon: "🎬", label: "Shows" },
  { id: "sound", icon: "🎵", label: "Sound" },
  { id: "calibrate", icon: "🔧", label: "Calibrate" },
];

const sheet: React.CSSProperties = {
  // bottom sheet: the tree stays alive above it
  position: "fixed", left: 0, right: 0, bottom: 0, height: "58%", zIndex: 200,
  background: "linear-gradient(180deg, rgba(10,13,20,0.92) 0%, #0f1320 18%)",
  backdropFilter: "blur(10px)", borderTop: "1px solid #23304a",
  borderRadius: "18px 18px 0 0",
  color: "#e7ecf6", font: "14px -apple-system, ui-sans-serif, system-ui, sans-serif",
  padding: "10px 14px calc(64px + env(safe-area-inset-bottom))",
  overflowY: "auto", display: "flex", flexDirection: "column", gap: 12,
};

const microLabel: React.CSSProperties = {
  color: "#7e8ea6", fontSize: 11, letterSpacing: 1.2, textTransform: "uppercase",
};

function Toggle({ on, label, onClick, accent = "#5b8cff" }: { on: boolean; label: string; onClick: () => void; accent?: string }) {
  return (
    <button onClick={onClick} style={{
      flex: 1, minHeight: 52, borderRadius: 14, cursor: "pointer", fontSize: 15, fontWeight: 600,
      border: on ? `1.5px solid ${accent}` : "1.5px solid #283549",
      background: on ? `${accent}22` : "#141a26", color: on ? "#fff" : "#9fb0c7",
      boxShadow: on ? `0 0 16px ${accent}66` : "none", touchAction: "manipulation",
    }}>{label}</button>
  );
}

function BigSlider({ label, v, min, max, step, on }: { label: string; v: number; min: number; max: number; step: number; on: (v: number) => void }) {
  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", color: "#9fb0c7", fontSize: 13, marginBottom: 4 }}>
        <span>{label}</span><span>{v.toFixed(2)}</span>
      </div>
      <input type="range" min={min} max={max} step={step} value={v} onChange={(e) => on(Number(e.target.value))}
        style={{ width: "100%", height: 38, accentColor: AMBER, touchAction: "manipulation" }} />
    </div>
  );
}

function Pad({ active, label, onClick, accent = "#5b8cff" }: { active: boolean; label: string; onClick: () => void; accent?: string }) {
  return (
    <button aria-label={`touchpad ${label}`} onClick={onClick} style={{
      minHeight: 56, borderRadius: 14, cursor: "pointer", fontSize: 13, fontWeight: 600, textTransform: "capitalize",
      border: active ? `1.5px solid ${accent}` : "1.5px solid #283549",
      background: active ? `${accent}26` : "#141a26", color: active ? "#fff" : "#9fb0c7",
      boxShadow: active ? `0 0 14px ${accent}55` : "none", touchAction: "manipulation",
    }}>{label}</button>
  );
}

export function TouchConsole() {
  const [open, setOpen] = useState(isPhoneLike);
  const setTouchOpen = useTwin((s) => s.setTouchOpen);
  useEffect(() => { setTouchOpen(open); return () => setTouchOpen(false); }, [open, setTouchOpen]);
  const ctrl = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const uiMode = useTwin((s) => s.uiMode);
  const setUiMode = useTwin((s) => s.setUiMode);
  const playShow = useTwin((s) => s.playShow);
  const activeShow = useTwin((s) => s.activeShow);
  const fixtures = useTwin((s) => s.fixtures);
  const calSolo = useTwin((s) => s.calSolo);
  const pingPresence = useTwin((s) => s.pingPresence);
  const [calIdx, setCalIdx] = useState(0);
  const [audioSrc, setAudioSrc] = useState<"off" | "mic" | "track">("off");
  const calId = useMemo(() => fixtures[calIdx]?.id ?? "—", [fixtures, calIdx]);

  if (!open) {
    return (
      <button onClick={() => setOpen(true)} style={{
        position: "fixed", bottom: 14, right: 14, zIndex: 16, padding: "10px 14px", borderRadius: 12,
        border: "1px solid #2a3a52", background: "rgba(16,22,34,0.9)", color: "#cdd6e4",
        font: "13px ui-monospace, monospace", cursor: "pointer", backdropFilter: "blur(6px)",
      }}>📱 touch</button>
    );
  }

  const solo = (idx: number) => {
    const i = ((idx % fixtures.length) + fixtures.length) % fixtures.length;
    setCalIdx(i);
    calSolo({ idx: i, rgb: [1, 1, 1] });
  };

  return (
    <>
      <div style={sheet}>
        {/* header: title + safety pair + exit — present in EVERY mode */}
        <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
          <span style={{ fontSize: 16, fontWeight: 700, letterSpacing: 0.3, flex: 1 }}>🌳 Resonance</span>
          <button onClick={() => set({ blackout: !ctrl.blackout })} style={{
            minHeight: 40, padding: "0 12px", borderRadius: 12, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: ctrl.blackout ? "1.5px solid #ff5b6e" : "1.5px solid #3a2a30",
            background: ctrl.blackout ? "#ff5b6e22" : "#141a26", color: ctrl.blackout ? "#ffd7dc" : "#9fb0c7",
          }}>🌑 blackout</button>
          <button onClick={() => set({ beaconPreempt: !ctrl.beaconPreempt })} style={{
            minHeight: 40, padding: "0 12px", borderRadius: 12, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: ctrl.beaconPreempt ? "1.5px solid #fff" : "1.5px solid #3a3a2a",
            background: ctrl.beaconPreempt ? "#ffffff22" : "#141a26", color: ctrl.beaconPreempt ? "#fff" : "#9fb0c7",
          }}>🔦 beacon</button>
          <button onClick={() => setOpen(false)} aria-label="full console" style={{
            width: 40, height: 40, borderRadius: 20, border: "1px solid #283549",
            background: "#141a26", color: "#9fb0c7", fontSize: 18, cursor: "pointer",
          }}>✕</button>
        </div>

        {/* ── mode content ── */}
        {uiMode === "lightshow" && (
          <>
            <div style={microLabel}>Light shows · tap to play, tap again to stop</div>
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(104px, 1fr))", gap: 8 }}>
              {SHOWS.map((s) => (
                <Pad key={s.id} active={activeShow === s.id} accent={AMBER}
                  label={activeShow === s.id ? `■ ${s.name}` : s.name}
                  onClick={() => playShow(activeShow === s.id ? null : s.id)} />
              ))}
            </div>
            <BigSlider label="master" v={ctrl.master} min={0} max={1} step={0.01} on={(v) => set({ master: v })} />
          </>
        )}

        {uiMode === "interactive" && (
          <>
            <div style={{ display: "flex", gap: 10 }}>
              <Toggle on={false} label="✨ ping the tree" onClick={() => pingPresence()} accent={AMBER} />
              <Toggle on={ctrl.aiPilot} label="🤖 AI pilot" onClick={() => set({ aiPilot: !ctrl.aiPilot })} accent="#9b6bff" />
            </div>
            <div style={microLabel}>Patterns</div>
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(94px, 1fr))", gap: 8 }}>
              {PADS.map((p) => (
                <Pad key={p} active={ctrl.pattern === p} label={p} onClick={() => set({ pattern: p })} />
              ))}
            </div>
            <BigSlider label="speed" v={ctrl.speed} min={0} max={3} step={0.01} on={(v) => set({ speed: v })} />
            <BigSlider label="brightness" v={ctrl.brightness} min={0} max={1} step={0.01} on={(v) => set({ brightness: v })} />
            <BigSlider label="hue" v={ctrl.hue} min={0} max={1} step={0.01} on={(v) => set({ hue: v })} />
          </>
        )}

        {uiMode === "sound" && (
          <>
            <div style={microLabel}>Audio source</div>
            <div style={{ display: "flex", gap: 10 }}>
              <Toggle on={audioSrc === "mic"} label="🎤 mic"
                onClick={() => { if (audioSrc === "mic") { stopAudio(); setAudioSrc("off"); } else { startMic().then(() => setAudioSrc("mic")).catch(console.error); } }} />
              <Toggle on={audioSrc === "track"} label="🎶 test track"
                onClick={() => { if (audioSrc === "track") { stopAudio(); setAudioSrc("off"); } else { void startTrack(asset("/audio/test-beat-124bpm.wav")).then(() => setAudioSrc("track")).catch(console.error); } }} />
              <Toggle on={ctrl.syncToBeat} label="🥁 beat" onClick={() => set({ syncToBeat: !ctrl.syncToBeat })} accent={AMBER} />
            </div>
            <BigSlider label="master" v={ctrl.master} min={0} max={1} step={0.01} on={(v) => set({ master: v })} />
            <BigSlider label="crossfade A↔B" v={ctrl.xfade} min={0} max={1} step={0.01} on={(v) => set({ xfade: v })} />
            <div style={{ color: "#7e8ea6", fontSize: 12 }}>
              The lights follow whatever the phone hears — hold it toward the music.
            </div>
          </>
        )}

        {uiMode === "calibrate" && (
          <>
            <div style={microLabel}>Solo one light · walk the tree, confirm what you see</div>
            <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
              <button onClick={() => solo(calIdx - 1)} style={{ minWidth: 64, minHeight: 64, fontSize: 24, borderRadius: 16, border: "1.5px solid #283549", background: "#141a26", color: "#cdd6e4", cursor: "pointer" }}>◀</button>
              <div style={{ flex: 1, textAlign: "center" }}>
                <div style={{ fontSize: 28, fontWeight: 800, color: AMBER }}>{calId}</div>
                <div style={{ color: "#7e8ea6", fontSize: 12 }}>{fixtures.length ? `${calIdx + 1} / ${fixtures.length} · only this light is on` : "no fixtures loaded"}</div>
              </div>
              <button onClick={() => solo(calIdx + 1)} style={{ minWidth: 64, minHeight: 64, fontSize: 24, borderRadius: 16, border: "1.5px solid #283549", background: "#141a26", color: "#cdd6e4", cursor: "pointer" }}>▶</button>
            </div>
            <div style={{ display: "flex", gap: 10 }}>
              <Toggle on={false} label="⏹ all lights back on" onClick={() => calSolo(null)} accent="#3ddc97" />
            </div>
            <div style={{ color: "#7e8ea6", fontSize: 12 }}>
              With the real fleet connected this same stepper drives the physical
              identify-blink — what you solo here blinks on the tree.
            </div>
          </>
        )}
      </div>

      {/* ── mode tab bar: four lanterns, active one is LIT ── */}
      <nav style={{
        position: "fixed", left: 0, right: 0, bottom: 0, zIndex: 210,
        display: "flex", gap: 6, padding: "8px 10px calc(8px + env(safe-area-inset-bottom))",
        background: "rgba(8,10,16,0.96)", borderTop: "1px solid #1c2740",
      }}>
        {MODES.map((m) => {
          const active = uiMode === m.id;
          return (
            <button key={m.id} onClick={() => setUiMode(m.id)} aria-label={`mode ${m.label}`} style={{
              flex: 1, minHeight: 52, borderRadius: 14, cursor: "pointer",
              display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 2,
              border: "none", background: "transparent",
              color: active ? AMBER : "#7e8ea6", fontWeight: active ? 700 : 500, fontSize: 11,
              textShadow: active ? `0 0 18px ${AMBER}` : "none", touchAction: "manipulation",
            }}>
              <span style={{ fontSize: 22, filter: active ? `drop-shadow(0 0 10px ${AMBER})` : "grayscale(0.4)" }}>{m.icon}</span>
              {m.label}
            </button>
          );
        })}
      </nav>
    </>
  );
}
