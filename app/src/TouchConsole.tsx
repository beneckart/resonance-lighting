import { useEffect, useMemo, useRef, useState } from "react";
import { PATTERN_IDS, ELEMENT_MODES, useTwin, type PatternId, type UiMode } from "./store";
import { SHOWS } from "./shows";
import { startMic, startTrack, stopAudio } from "./audio";
import { interpret } from "./llm";
import { listenOnce, voiceSupported, type ListenHandle } from "./voice";
import { asset } from "./fixtures";

/** TOUCH CONSOLE v4 — Elliot 08-14 19:14Z: "menu of controls with the tree
 *  above… automatically set the menu at the bottom based on the size and
 *  resize the tree to fit… change the menu and customize it."
 *  🌳 Tree keeps its own full-screen tab. MODE tabs open a bottom sheet that
 *  sizes itself to its CONTENT (max 62%); a ResizeObserver writes the sheet's
 *  real height to --sheet-h and the canvas container RESIZES to the space
 *  above — the tree re-frames, it isn't covered. Customize v1: every section
 *  collapses on header tap, persisted per device. Tab bar scrolls; safety
 *  rides every screen; ✕ drops to desktop. */

const AMBER = "#ffb454";
const PADS = [...PATTERN_IDS, ...ELEMENT_MODES] as PatternId[];

/** phone-detect at mount: coarse pointer OR narrow viewport → land in touch */
function isPhoneLike(): boolean {
  if (typeof window === "undefined" || !window.matchMedia) return false;
  return window.matchMedia("(max-width: 767px)").matches ||
    (window.matchMedia("(pointer: coarse)").matches && window.matchMedia("(max-width: 1023px)").matches);
}

type TabId = "tree" | UiMode;
const MODES: { id: TabId; icon: string; label: string }[] = [
  { id: "tree", icon: "🌳", label: "Tree" },
  { id: "interactive", icon: "🌱", label: "Interactive" },
  { id: "lightshow", icon: "🎬", label: "Shows" },
  { id: "sound", icon: "🎵", label: "Sound" },
  { id: "calibrate", icon: "🔧", label: "Calibrate" },
];

const sheet: React.CSSProperties = {
  // content-sized bottom sheet: the tree above RESIZES to fit (not covered)
  position: "fixed", left: 0, right: 0, bottom: "calc(64px + env(safe-area-inset-bottom))", zIndex: 200,
  maxHeight: "62%",
  background: "linear-gradient(180deg, rgba(10,13,20,0.97) 0%, #0f1320 14%)",
  borderTop: "1px solid #23304a", borderRadius: "18px 18px 0 0",
  color: "#e7ecf6", font: "14px -apple-system, ui-sans-serif, system-ui, sans-serif",
  padding: "10px 14px calc(64px + env(safe-area-inset-bottom))",
  overflowY: "auto", display: "flex", flexDirection: "column", gap: 12,
};

const COLLAPSE_KEY = "touch.collapsed";
function loadCollapsed(): Record<string, boolean> {
  try { return JSON.parse(localStorage.getItem(COLLAPSE_KEY) ?? "{}"); } catch { return {}; }
}

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
  const [tab, setTab] = useState<TabId>("tree"); // land on the tree, per the sketch
  const setTouchOpen = useTwin((s) => s.setTouchOpen);
  useEffect(() => { setTouchOpen(open); return () => setTouchOpen(false); }, [open, setTouchOpen]);
  const ctrl = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const setUiMode = useTwin((s) => s.setUiMode);
  const playShow = useTwin((s) => s.playShow);
  const activeShow = useTwin((s) => s.activeShow);
  const fixtures = useTwin((s) => s.fixtures);
  const calSolo = useTwin((s) => s.calSolo);
  const pingPresence = useTwin((s) => s.pingPresence);
  const selectedLight = useTwin((s) => s.selectedLight);
  const selectLight = useTwin((s) => s.selectLight);
  const setLightOverride = useTwin((s) => s.setLightOverride);
  const overrides = useTwin((s) => s.overrides);
  const cues = useTwin((s) => s.cues);
  const addCue = useTwin((s) => s.addCue);
  const recallCue = useTwin((s) => s.recallCue);
  const deleteCue = useTwin((s) => s.deleteCue);
  const [modeName, setModeName] = useState("");
  const [editModes, setEditModes] = useState(false);
  const runScript = useTwin((s) => s.runScript);
  const [voiceState, setVoiceState] = useState<"idle" | "listening" | "typing">("idle");
  const [voiceText, setVoiceText] = useState("");
  const [voiceNote, setVoiceNote] = useState<string | null>(null);
  const voiceHandle = { current: null as ListenHandle | null };

  /** transcript (spoken or typed) → interpret → commands → the tree */
  const speakToTree = (text: string) => {
    const t = text.trim();
    if (!t) return;
    const r = interpret(t);
    if (r.commands.length) runScript(r.commands.join("\n"));
    setVoiceNote(r.commands.length ? `✓ ${r.note}` : `didn't catch a light command in “${t}”`);
    setVoiceText("");
    setVoiceState("idle");
    window.setTimeout(() => setVoiceNote(null), 5000);
  };

  const startVoice = () => {
    if (!voiceSupported()) { setVoiceState("typing"); return; } // fallback: typed input
    setVoiceState("listening");
    setVoiceText("");
    voiceHandle.current = listenOnce({
      onPartial: (t) => setVoiceText(t),
      onFinal: (t) => speakToTree(t),
      onError: (e) => { setVoiceNote(`🎤 ${e} — type it instead`); setVoiceState("typing"); },
      onEnd: () => setVoiceState((v) => (v === "listening" ? "idle" : v)),
    });
  };
  const [calIdx, setCalIdx] = useState(0);
  const [audioSrc, setAudioSrc] = useState<"off" | "mic" | "track">("off");
  const [collapsed, setCollapsed] = useState<Record<string, boolean>>(loadCollapsed);
  const toggleSection = (k: string) => setCollapsed((c) => {
    const n = { ...c, [k]: !c[k] };
    try { localStorage.setItem(COLLAPSE_KEY, JSON.stringify(n)); } catch { /* fine */ }
    return n;
  });
  /** customize v1 (Elliot: "change the menu and customize it"): every section
   *  header is a tap-to-collapse toggle, remembered per device */
  const Section = ({ id, label, children }: { id: string; label: string; children: React.ReactNode }) => (
    <>
      <button onClick={() => toggleSection(id)} style={{
        ...microLabel, background: "none", border: "none", textAlign: "left", padding: 0,
        cursor: "pointer", display: "flex", justifyContent: "space-between", width: "100%",
      }}>{label}<span>{collapsed[id] ? "▸" : "▾"}</span></button>
      {!collapsed[id] && children}
    </>
  );
  const calId = useMemo(() => fixtures[calIdx]?.id ?? "—", [fixtures, calIdx]);

  // ALL hooks live above the early return — the sheet-height effect sat below
  // it and crashed the component with "fewer hooks than expected" the moment
  // ✕ closed the console (launch-QA find, console errors 21:53Z).
  const sheetEl = useRef<HTMLDivElement | null>(null);
  useEffect(() => {
    const root = document.documentElement;
    const el = sheetEl.current;
    if (!open || tab === "tree" || !el) {
      root.style.setProperty("--sheet-h", "0px");
      return;
    }
    const apply = () => root.style.setProperty("--sheet-h", `${el.getBoundingClientRect().height + 64}px`);
    apply();
    const ro = new ResizeObserver(apply);
    ro.observe(el);
    return () => { ro.disconnect(); root.style.setProperty("--sheet-h", "0px"); };
  }, [open, tab, collapsed]);

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

  const SWATCHES: { name: string; rgb: [number, number, number] }[] = [
    { name: "red", rgb: [1, 0.05, 0.05] }, { name: "orange", rgb: [1, 0.45, 0.05] },
    { name: "amber", rgb: [1, 0.71, 0.33] }, { name: "green", rgb: [0.1, 1, 0.3] },
    { name: "cyan", rgb: [0.1, 0.9, 1] }, { name: "blue", rgb: [0.15, 0.35, 1] },
    { name: "purple", rgb: [0.6, 0.2, 1] }, { name: "pink", rgb: [1, 0.35, 0.75] },
    { name: "white", rgb: [1, 1, 1] },
  ];
  const selId = selectedLight !== null ? (fixtures[selectedLight]?.id ?? "?") : null;
  const selOv = selectedLight !== null ? overrides[selectedLight] : undefined;

  // sheet height → --sheet-h CSS var; the canvas container in App resizes to
  // the space above (the tree RE-FRAMES to fit — Elliot 19:14Z — not covered).
  // Effect-managed (a callback-ref ResizeObserver leaked here: --sheet-h stuck
  // at the old height after the sheet unmounted — canvas 257px on the Tree tab,
  // caught by measurement 08-14). v3's live-peek is gone: with the tree
  // visibly above, the canvas IS the feedback.
  return (
    <>
      {tab === "tree" && (
        <div style={{
          position: "fixed", top: "max(8px, env(safe-area-inset-top))", left: 8, right: 8, zIndex: 205,
          display: "flex", gap: 8, alignItems: "center",
        }}>
          <span style={{ fontWeight: 800, color: "#e7ecf6", textShadow: "0 1px 8px #000", flex: 1 }}>🌳 Resonance</span>
          <button onClick={() => set({ blackout: !ctrl.blackout })} style={{
            minHeight: 40, padding: "0 12px", borderRadius: 12, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: ctrl.blackout ? "1.5px solid #ff5b6e" : "1.5px solid #3a2a30",
            background: ctrl.blackout ? "#2a1016" : "rgba(16,22,34,0.85)", color: ctrl.blackout ? "#ffd7dc" : "#cdd6e4",
          }}>🌑</button>
          <button onClick={() => set({ beaconPreempt: !ctrl.beaconPreempt })} style={{
            minHeight: 40, padding: "0 12px", borderRadius: 12, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: ctrl.beaconPreempt ? "1.5px solid #fff" : "1.5px solid #3a3a2a",
            background: ctrl.beaconPreempt ? "#333" : "rgba(16,22,34,0.85)", color: "#fff",
          }}>🔦</button>
          <button onClick={() => setOpen(false)} aria-label="full console" style={{
            width: 40, height: 40, borderRadius: 20, border: "1px solid #283549",
            background: "rgba(16,22,34,0.85)", color: "#9fb0c7", fontSize: 16, cursor: "pointer",
          }}>✕</button>
        </div>
      )}
      {selectedLight !== null && (
        <div style={{
          position: "fixed", left: 8, right: 8, bottom: "calc(58% + 8px)", zIndex: 205,
          background: "rgba(10,13,20,0.95)", border: `1px solid ${AMBER}55`, borderRadius: 14,
          padding: "8px 10px", display: "flex", flexDirection: "column", gap: 8,
          color: "#e7ecf6", font: "13px -apple-system, ui-sans-serif, system-ui, sans-serif",
        }}>
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <span style={{ fontWeight: 800, color: AMBER }}>💡 {selId}</span>
            <span style={{ color: "#7e8ea6", fontSize: 11, flex: 1 }}>
              {selOv ? (selOv.mode === "off" ? "held OFF" : "held to a color") : "following the show"}
            </span>
            <button onClick={() => { setLightOverride(selectedLight, null); }} style={{
              minHeight: 34, padding: "0 10px", borderRadius: 10, border: "1px solid #2a3a52",
              background: "#141a26", color: "#9fb0c7", cursor: "pointer", fontSize: 12,
            }}>↩ release</button>
            <button onClick={() => selectLight(null)} aria-label="close light editor" style={{
              width: 34, height: 34, borderRadius: 17, border: "1px solid #283549",
              background: "#141a26", color: "#9fb0c7", cursor: "pointer",
            }}>✕</button>
          </div>
          <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
            {SWATCHES.map((c) => (
              <button key={c.name} aria-label={`light color ${c.name}`}
                onClick={() => setLightOverride(selectedLight, { mode: "color", rgb: c.rgb })}
                style={{
                  width: 34, height: 34, borderRadius: 17, cursor: "pointer",
                  border: "2px solid #0a0d14", outline: "1px solid #2a3a52",
                  background: `rgb(${c.rgb.map((v) => Math.round(v * 255)).join(",")})`,
                }} />
            ))}
            <button onClick={() => setLightOverride(selectedLight, { mode: "off" })} style={{
              minHeight: 34, padding: "0 12px", borderRadius: 17, cursor: "pointer",
              border: "1px solid #3a2a30", background: "#141a26", color: "#ff8fa0", fontWeight: 700, fontSize: 12,
            }}>off</button>
          </div>
          {selOv?.mode === "color" && (
            <div style={{ display: "flex", gap: 12 }}>
              <label style={{ flex: 1, color: "#9fb0c7", fontSize: 12 }}>
                fade · {Math.round(((selOv.bri ?? 1)) * 100)}%
                <input type="range" min={0.05} max={1} step={0.05} value={selOv.bri ?? 1}
                  onChange={(e) => setLightOverride(selectedLight, { ...selOv, bri: Number(e.target.value) })}
                  style={{ width: "100%", height: 30, accentColor: AMBER }} />
              </label>
              <label style={{ flex: 1, color: "#9fb0c7", fontSize: 12 }}>
                motion · {(selOv.pulse ?? 0).toFixed(1)} Hz
                <input type="range" min={0} max={3} step={0.1} value={selOv.pulse ?? 0}
                  onChange={(e) => setLightOverride(selectedLight, { ...selOv, pulse: Number(e.target.value) })}
                  style={{ width: "100%", height: 30, accentColor: AMBER }} />
              </label>
            </div>
          )}
        </div>
      )}
      {tab !== "tree" && <div ref={sheetEl} style={sheet}>
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
        {tab === "lightshow" && (
          <>
            <Section id="shows" label="Light shows · tap to play, tap again to stop">
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(104px, 1fr))", gap: 8 }}>
              {SHOWS.map((s) => (
                <Pad key={s.id} active={activeShow === s.id} accent={AMBER}
                  label={activeShow === s.id ? `■ ${s.name}` : s.name}
                  onClick={() => playShow(activeShow === s.id ? null : s.id)} />
              ))}
            </div>
            </Section>
            <BigSlider label="master" v={ctrl.master} min={0} max={1} step={0.01} on={(v) => set({ master: v })} />
          </>
        )}

        {tab === "interactive" && (
          <>
            <div style={{ display: "flex", gap: 10 }}>
              <Toggle on={false} label="✨ ping the tree" onClick={() => pingPresence()} accent={AMBER} />
              <Toggle on={ctrl.aiPilot} label="🤖 AI pilot" onClick={() => set({ aiPilot: !ctrl.aiPilot })} accent="#9b6bff" />
            </div>
            <Section id="mymodes" label="My modes · save the look you just made">
            <div style={{ display: "flex", gap: 6, flexWrap: "wrap", alignItems: "center" }}>
              {cues.map((c) => (
                <span key={c.id} style={{ display: "inline-flex", alignItems: "center", gap: 4 }}>
                  <button onClick={() => recallCue(c.id)} style={{
                    minHeight: 40, padding: "0 14px", borderRadius: 12, cursor: "pointer", fontWeight: 600, fontSize: 13,
                    border: `1.5px solid ${AMBER}66`, background: "#1c1610", color: "#ffe2b0",
                  }}>★ {c.name}</button>
                  {editModes && (
                    <button aria-label={`delete mode ${c.name}`} onClick={() => deleteCue(c.id)} style={{
                      width: 28, height: 28, borderRadius: 14, border: "1px solid #3a2a30",
                      background: "#141a26", color: "#ff8fa0", cursor: "pointer", fontSize: 12,
                    }}>✕</button>
                  )}
                </span>
              ))}
              {cues.length > 0 && (
                <button onClick={() => setEditModes(!editModes)} style={{
                  minHeight: 40, padding: "0 10px", borderRadius: 12, cursor: "pointer", fontSize: 12,
                  border: "1px solid #283549", background: "#141a26", color: "#9fb0c7",
                }}>{editModes ? "done" : "edit"}</button>
              )}
            </div>
            <div style={{ display: "flex", gap: 6 }}>
              <input value={modeName} onChange={(e) => setModeName(e.target.value)} placeholder="name this mode…"
                style={{ flex: 1, minHeight: 44, borderRadius: 12, border: "1px solid #283549", background: "#0d1119", color: "#e7ecf6", padding: "0 12px", fontSize: 14 }} />
              <button onClick={() => { addCue(modeName || "my mode"); setModeName(""); }} style={{
                minHeight: 44, padding: "0 16px", borderRadius: 12, cursor: "pointer", fontWeight: 700,
                border: `1.5px solid ${AMBER}`, background: `${AMBER}22`, color: "#fff", fontSize: 14,
              }}>💾 save</button>
            </div>
            </Section>
            <div style={{ color: "#7e8ea6", fontSize: 12 }}>💡 Tap any light on the tree above to take it over — color it or hold it off while the show plays around it.</div>
            <Section id="patterns" label="Patterns">
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(94px, 1fr))", gap: 8 }}>
              {PADS.map((p) => (
                <Pad key={p} active={ctrl.pattern === p} label={p} onClick={() => set({ pattern: p })} />
              ))}
            </div>
            </Section>
            <Section id="dials" label="Dials">
              <BigSlider label="speed" v={ctrl.speed} min={0} max={3} step={0.01} on={(v) => set({ speed: v })} />
              <BigSlider label="brightness" v={ctrl.brightness} min={0} max={1} step={0.01} on={(v) => set({ brightness: v })} />
              <BigSlider label="hue" v={ctrl.hue} min={0} max={1} step={0.01} on={(v) => set({ hue: v })} />
            </Section>
          </>
        )}

        {tab === "sound" && (
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

        {tab === "calibrate" && (
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
      </div>}

      {/* ── voice: mic FAB above the tab bar + live transcript / result chip ── */}
      {(voiceState !== "idle" || voiceNote) && (
        <div style={{
          position: "fixed", left: 12, right: 76, bottom: "calc(72px + env(safe-area-inset-bottom))", zIndex: 215,
          background: "rgba(10,13,20,0.95)", border: `1px solid ${AMBER}55`, borderRadius: 12,
          padding: "8px 12px", color: "#e7ecf6", fontSize: 13,
        }}>
          {voiceState === "listening" && <span>🎤 {voiceText || "listening…"}</span>}
          {voiceState === "typing" && (
            <form onSubmit={(e) => { e.preventDefault(); speakToTree(voiceText); }} style={{ display: "flex", gap: 6 }}>
              <input autoFocus value={voiceText} onChange={(e) => setVoiceText(e.target.value)}
                placeholder="tell the tree… e.g. everything red and slow"
                style={{ flex: 1, minHeight: 38, borderRadius: 10, border: "1px solid #283549", background: "#0d1119", color: "#e7ecf6", padding: "0 10px" }} />
              <button type="submit" style={{ minHeight: 38, padding: "0 12px", borderRadius: 10, border: `1px solid ${AMBER}`, background: `${AMBER}22`, color: "#fff", cursor: "pointer" }}>go</button>
            </form>
          )}
          {voiceState === "idle" && voiceNote && <span>{voiceNote}</span>}
        </div>
      )}
      <button aria-label="voice command" onClick={() => (voiceState === "listening" ? voiceHandle.current?.stop() : startVoice())} style={{
        position: "fixed", right: 12, bottom: "calc(72px + env(safe-area-inset-bottom))", zIndex: 215,
        width: 56, height: 56, borderRadius: 28, cursor: "pointer",
        border: `1.5px solid ${voiceState === "listening" ? "#ff5b6e" : AMBER}`,
        background: voiceState === "listening" ? "#ff5b6e22" : `${AMBER}22`,
        color: "#fff", fontSize: 24,
        boxShadow: voiceState === "listening" ? "0 0 22px #ff5b6ecc" : `0 0 14px ${AMBER}66`,
      }}>🎤</button>

      {/* ── mode tab bar: four lanterns, active one is LIT ── */}
      <nav style={{
        position: "fixed", left: 0, right: 0, bottom: 0, zIndex: 210,
        display: "flex", gap: 6, padding: "8px 10px calc(8px + env(safe-area-inset-bottom))",
        background: "rgba(8,10,16,0.96)", borderTop: "1px solid #1c2740",
        overflowX: "auto", // tabs scroll as modes grow (Elliot's sketch)
      }}>
        {MODES.map((m) => {
          const active = tab === m.id;
          return (
            <button key={m.id} onClick={() => {
              setTab(m.id);
              // sync uiMode for the modes whose desktop semantics are safe;
              // "interactive" on desktop FORCES pattern→"life" (quiet CA world),
              // which read as "the tree does not respond" (Elliot, measured
              // 08-14: luminance 125→123 on tab tap) — on mobile the pads rule.
              if (m.id === "lightshow" || m.id === "sound" || m.id === "calibrate") setUiMode(m.id);
            }} aria-label={`mode ${m.label}`} style={{
              flex: "1 0 72px", minHeight: 52, borderRadius: 14, cursor: "pointer",
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
