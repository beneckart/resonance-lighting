import { useState } from "react";
import { PATTERN_IDS, ELEMENT_MODES, useTwin, type PatternId } from "./store";
import { SHOWS } from "./shows";

/** iPad-OS style TOUCH console (Elliot ask): a togglable fullscreen overlay with
 *  big touch targets — pattern pads + master/brightness + crossfader + AI/camera/
 *  strobe toggles — for driving the tree from a tablet at the install. Works at
 *  375px (phone) up through tablet/desktop.
 *
 *  MOBILE-FIRST (Elliot 08-14, "way too clunky" on the phone): a phone now
 *  LANDS in this console instead of the desktop panel stack — open defaults to
 *  true on coarse-pointer/narrow screens. ✕ drops to the full UI. Also grew
 *  the two things a phone operator needs most: the Light Shows row and the
 *  big BLACKOUT/BEACON pair. */
const PADS = [...PATTERN_IDS, ...ELEMENT_MODES] as PatternId[];

/** phone-detect at mount: coarse pointer OR narrow viewport → land in touch */
function isPhoneLike(): boolean {
  if (typeof window === "undefined" || !window.matchMedia) return false;
  return window.matchMedia("(max-width: 767px)").matches ||
    (window.matchMedia("(pointer: coarse)").matches && window.matchMedia("(max-width: 1023px)").matches);
}

const sheet: React.CSSProperties = {
  position: "fixed", inset: 0, zIndex: 200, // above every desktop float (clean-view/record/beacon pills)
  background: "linear-gradient(160deg,#0a0d14 0%,#0f1320 100%)",
  color: "#e7ecf6", font: "14px -apple-system, ui-sans-serif, system-ui, sans-serif",
  padding: "max(16px, env(safe-area-inset-top)) 16px 16px", overflowY: "auto",
  display: "flex", flexDirection: "column", gap: 14,
};

function Toggle({ on, label, onClick, accent = "#5b8cff" }: { on: boolean; label: string; onClick: () => void; accent?: string }) {
  return (
    <button onClick={onClick} style={{
      flex: 1, minHeight: 56, borderRadius: 14, cursor: "pointer", fontSize: 15, fontWeight: 600,
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
        style={{ width: "100%", height: 38, accentColor: "#5b8cff", touchAction: "manipulation" }} />
    </div>
  );
}

export function TouchConsole() {
  const [open, setOpen] = useState(isPhoneLike);
  const ctrl = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const cameraPreset = useTwin((s) => s.cameraPreset);
  const setCameraPreset = useTwin((s) => s.setCameraPreset);
  const playShow = useTwin((s) => s.playShow);
  const activeShow = useTwin((s) => s.activeShow);

  if (!open) {
    return (
      <button onClick={() => setOpen(true)} style={{
        position: "fixed", bottom: 14, right: 14, zIndex: 16, padding: "10px 14px", borderRadius: 12,
        border: "1px solid #2a3a52", background: "rgba(16,22,34,0.9)", color: "#cdd6e4",
        font: "13px ui-monospace, monospace", cursor: "pointer", backdropFilter: "blur(6px)",
      }}>📱 touch</button>
    );
  }

  return (
    <div style={sheet}>
      <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
        <span style={{ fontSize: 18, fontWeight: 700, letterSpacing: 0.3 }}>🌳 Resonance · Touch Console</span>
        <button onClick={() => setOpen(false)} style={{
          width: 44, height: 44, borderRadius: 22, border: "1px solid #283549",
          background: "#141a26", color: "#9fb0c7", fontSize: 20, cursor: "pointer",
        }}>✕</button>
      </div>

      <div style={{ color: "#7e8ea6", fontSize: 12 }}>LIGHT SHOWS</div>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(104px, 1fr))", gap: 8 }}>
        {SHOWS.map((s) => {
          const active = activeShow === s.id;
          return (
            <button key={s.id} aria-label={`show ${s.id}`} onClick={() => playShow(active ? null : s.id)} style={{
              minHeight: 60, borderRadius: 14, cursor: "pointer", fontSize: 13, fontWeight: 600,
              border: active ? "1.5px solid #ffb454" : "1.5px solid #283549",
              background: active ? "#3d2c12" : "#141a26", color: active ? "#ffe2b0" : "#9fb0c7",
              boxShadow: active ? "0 0 16px #ffb45466" : "none", touchAction: "manipulation",
            }}>{active ? `■ ${s.name}` : s.name}</button>
          );
        })}
      </div>

      <div style={{ display: "flex", gap: 10 }}>
        <Toggle on={ctrl.blackout} label="🌑 BLACKOUT" onClick={() => set({ blackout: !ctrl.blackout })} accent="#ff5b6e" />
        <Toggle on={ctrl.beaconPreempt} label="🔦 BEACON" onClick={() => set({ beaconPreempt: !ctrl.beaconPreempt })} accent="#ffd166" />
      </div>

      <div style={{ color: "#7e8ea6", fontSize: 12 }}>PATTERN</div>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(94px, 1fr))", gap: 8 }}>
        {PADS.map((p) => {
          const active = ctrl.pattern === p;
          return (
            <button key={p} aria-label={`touchpad ${p}`} onClick={() => set({ pattern: p })} style={{
              minHeight: 60, borderRadius: 14, cursor: "pointer", fontSize: 13, fontWeight: 600, textTransform: "capitalize",
              border: active ? "1.5px solid #5b8cff" : "1.5px solid #283549",
              background: active ? "#21345e" : "#141a26", color: active ? "#dce6ff" : "#9fb0c7",
              boxShadow: active ? "0 0 16px #5b8cff66" : "none", touchAction: "manipulation",
            }}>{p}</button>
          );
        })}
      </div>

      <div style={{ display: "flex", gap: 10 }}>
        <Toggle on={ctrl.aiPilot} label="🤖 AI" onClick={() => set({ aiPilot: !ctrl.aiPilot })} accent="#9b6bff" />
        <Toggle on={ctrl.strobe} label="⚡ Strobe" onClick={() => set({ strobe: !ctrl.strobe })} accent="#ff5b6e" />
        <Toggle on={ctrl.syncToBeat} label="🥁 Sync" onClick={() => set({ syncToBeat: !ctrl.syncToBeat })} />
      </div>
      <div style={{ display: "flex", gap: 10 }}>
        <Toggle on={cameraPreset === "hero"} label="Hero cam" onClick={() => setCameraPreset("hero")} accent="#3ddc97" />
        <Toggle on={cameraPreset === "top"} label="⬇ Top-down" onClick={() => setCameraPreset("top")} accent="#3ddc97" />
      </div>

      <BigSlider label="MASTER" v={ctrl.master} min={0} max={1} step={0.01} on={(v) => set({ master: v })} />
      <BigSlider label="brightness" v={ctrl.brightness} min={0} max={1} step={0.01} on={(v) => set({ brightness: v })} />
      <BigSlider label="hue" v={ctrl.hue} min={0} max={1} step={0.01} on={(v) => set({ hue: v })} />
      <BigSlider label="speed" v={ctrl.speed} min={0} max={3} step={0.01} on={(v) => set({ speed: v })} />
      <BigSlider label="crossfade A↔B" v={ctrl.xfade} min={0} max={1} step={0.01} on={(v) => set({ xfade: v })} />
    </div>
  );
}
