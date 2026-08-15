import { useEffect, useMemo, useRef, useState, useSyncExternalStore } from "react";
import { PATTERN_IDS, ELEMENT_MODES, useTwin, type PatternId, type UiMode } from "./store";
import {
  fleetCensusNow, fleetConnected, fleetIdentify, fleetRegistry, fleetVersion, subscribeFleet,
} from "./fleetlink";
import { litState, GAUGE_FAULT_MV } from "./macregistry";
import { SHOWS } from "./shows";
import { startMic, startTrack, stopAudio } from "./audio";
import { interpretRemote, remoteConfigured, loadKey, saveKey, loadModel, saveModel, DEFAULT_MODEL } from "./openrouter";
import { listenOnce, voiceSupported, type ListenHandle } from "./voice";
import { ThemePicker } from "./ThemePicker";
import { loadCalibration, resolveFixtureId } from "./calibration";
import { nameFor, setName } from "./names";
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

type TabId = "tree" | UiMode | "command" | "locate" | "settings";
const MODES: { id: TabId; icon: string; label: string }[] = [
  { id: "tree", icon: "🌳", label: "Tree" },
  { id: "command", icon: "🎛", label: "Command" },
  { id: "locate", icon: "🔎", label: "Locate" },
  { id: "interactive", icon: "🌱", label: "Interactive" },
  { id: "lightshow", icon: "🎬", label: "Shows" },
  { id: "sound", icon: "🎵", label: "Sound" },
  { id: "calibrate", icon: "🔧", label: "Calibrate" },
  { id: "settings", icon: "⚙️", label: "Settings" },
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
  const triggerAt = useTwin((s) => s.triggerAt);
  const demoLock = useTwin((s) => s.demoLock);
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
  // Game of Light lifecycle (mobile parity with InteractivityPanel, 08-15)
  const golPhase = useTwin((s) => s.gol.phase);
  const armGol = useTwin((s) => s.armGol);
  const golSetPhase = useTwin((s) => s.golSetPhase);
  const golFirstVisitor = useTwin((s) => s.golFirstVisitor);
  const caTheme = useTwin((s) => s.caTheme);
  const setCaTheme = useTwin((s) => s.setCaTheme);
  // AI operator settings. Hooks live here, ABOVE the early return — a sheet
  // effect placed below one crashed this component on 08-14 ("fewer hooks than
  // expected") and there is no reason to relearn that.
  const [aiKey, setAiKey] = useState(() => loadKey() ?? "");
  const [aiModel, setAiModel] = useState(() => loadModel());
  const [aiSaved, setAiSaved] = useState(false);
  const [voiceState, setVoiceState] = useState<"idle" | "listening" | "typing">("idle");
  const [voiceText, setVoiceText] = useState("");
  const [voiceNote, setVoiceNote] = useState<string | null>(null);
  const voiceHandle = { current: null as ListenHandle | null };

  /** transcript (spoken or typed) → interpret → commands → the tree.
   *
   *  Two interpreters, one seam: with an OpenRouter key present the remote model
   *  reads the sentence, otherwise — or on ANY failure — the deterministic
   *  offline interpreter does. interpretRemote never throws and never returns
   *  empty-handed, so there is deliberately no error branch here. It DOES report
   *  which brain answered (🤖 remote / ⚙️ offline), because an operator who
   *  can't tell whether the AI is live is being asked to trust a black box. */
  const speakToTree = async (text: string) => {
    const t = text.trim();
    if (!t) return;
    setVoiceState("idle");
    setVoiceText("");
    if (remoteConfigured()) setVoiceNote("🤖 thinking…");

    const r = await interpretRemote(t, { timeoutMs: 12000 });
    if (r.commands.length) runScript(r.commands.join("\n"));

    const badge = r.source === "openrouter" ? "🤖" : "⚙️";
    setVoiceNote(
      r.commands.length ? `${badge} ${r.note}` : `didn't catch a light command in “${t}”`,
    );
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
        // zIndex 60, NOT 16. SidePanel is zIndex 40 and fills the right half in
        // dock mode, so at right:14 this rendered UNDERNEATH it and was
        // unclickable on every dock surface — measured 2026-08-15, blocked by a
        // DIFFERENT element on each one ("🕯 Intimate", a description div, a span
        // reading "0.02"), which is the signature of stacking order rather than
        // one bad overlap. 60 is what every sibling floater already uses
        // (🗂 dock, 🔦 BEACON, 🌑 BLACKOUT, ✨ clean view).
        position: "fixed", bottom: 14, right: 14, zIndex: 60, padding: "10px 14px", borderRadius: 12,
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
          <span style={{ fontWeight: 800, color: "#e7ecf6", textShadow: "0 1px 8px #000", flex: 1 }}>🌳 Resonance{demoLock ? " · demo" : ""}</span>
          {!demoLock && <><button onClick={() => set({ blackout: !ctrl.blackout })} style={{
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
          }}>✕</button></>}
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
          <span style={{ fontSize: 16, fontWeight: 700, letterSpacing: 0.3, flex: 1 }}>🌳 Resonance{demoLock ? " · demo" : ""}</span>
          {!demoLock && <button onClick={() => set({ blackout: !ctrl.blackout })} style={{
            minHeight: 40, padding: "0 12px", borderRadius: 12, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: ctrl.blackout ? "1.5px solid #ff5b6e" : "1.5px solid #3a2a30",
            background: ctrl.blackout ? "#ff5b6e22" : "#141a26", color: ctrl.blackout ? "#ffd7dc" : "#9fb0c7",
          }}>🌑 blackout</button>}
          {!demoLock && <button onClick={() => set({ beaconPreempt: !ctrl.beaconPreempt })} style={{
            minHeight: 40, padding: "0 12px", borderRadius: 12, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: ctrl.beaconPreempt ? "1.5px solid #fff" : "1.5px solid #3a3a2a",
            background: ctrl.beaconPreempt ? "#ffffff22" : "#141a26", color: ctrl.beaconPreempt ? "#fff" : "#9fb0c7",
          }}>🔦 beacon</button>}
          {!demoLock && <button onClick={() => setOpen(false)} aria-label="full console" style={{
            width: 40, height: 40, borderRadius: 20, border: "1px solid #283549",
            background: "#141a26", color: "#9fb0c7", fontSize: 18, cursor: "pointer",
          }}>✕</button>}
        </div>

        {/* ── mode content ── */}
        {tab === "command" && <CommandPage />}
        {tab === "locate" && <LocatePage />}
        {tab === "settings" && (
          <>
            <div style={microLabel}>Settings · this device</div>
            <Section id="ai" label={`🤖 AI operator (OpenRouter key lives on THIS device) ${remoteConfigured() ? "· 🤖 Claude connected" : "· offline"}`}>
              <div style={{ color: "#7e8ea6", fontSize: 12, lineHeight: 1.5 }}>
                Talk to the tree in plain language. Without a key it uses the built-in
                offline interpreter — which keeps working when the network doesn't.
                The key is stored on <b>this device only</b>; it is never sent anywhere
                but OpenRouter, and never leaves with the app.
              </div>
              <input
                type="password"
                value={aiKey}
                placeholder="OpenRouter API key (sk-or-…)"
                autoComplete="off"
                spellCheck={false}
                onChange={(e) => { setAiKey(e.target.value); setAiSaved(false); }}
                style={{
                  width: "100%", padding: "12px 10px", marginTop: 8, borderRadius: 10,
                  border: "1px solid #2b3a52", background: "#0d1420", color: "#dbe6f5",
                  fontSize: 16, // 16px stops iOS Safari zooming the whole page on focus
                }}
              />
              <input
                type="text"
                value={aiModel}
                placeholder={DEFAULT_MODEL}
                autoComplete="off"
                spellCheck={false}
                onChange={(e) => { setAiModel(e.target.value); setAiSaved(false); }}
                style={{
                  width: "100%", padding: "10px", marginTop: 8, borderRadius: 10,
                  border: "1px solid #2b3a52", background: "#0d1420", color: "#7e8ea6",
                  fontSize: 16,
                }}
              />
              <div style={{ display: "flex", gap: 10, marginTop: 10 }}>
                <Toggle
                  on={aiSaved}
                  label={aiSaved ? "✓ saved" : "save"}
                  accent={AMBER}
                  onClick={() => {
                    saveKey(aiKey);
                    saveModel(aiModel);
                    setAiSaved(true);
                    window.setTimeout(() => setAiSaved(false), 2500);
                  }}
                />
                <Toggle
                  on={false}
                  label="forget key"
                  onClick={() => { saveKey(""); setAiKey(""); setAiSaved(false); }}
                />
              </div>
            </Section>

            <Section id="set-bridge" label="📡 Bridge & fleet">
              <div style={{ color: "#7e8ea6", fontSize: 12, lineHeight: 1.6 }}>
                Daemon: same-origin <code>/cambium</code> proxy (default) ·
                <code>?cambium=0</code> in the URL disables · drive-real is ON by
                default — the tree mirrors this twin when the daemon answers.
              </div>
              <div style={{ display: "flex", gap: 8 }}>
                <Toggle on={useTwin.getState().net.driveReal} accent="#ff5b6e"
                  label={useTwin.getState().net.driveReal ? "📡 driving real" : "drive real: off"}
                  onClick={() => useTwin.getState().setNet({ driveReal: !useTwin.getState().net.driveReal })} />
              </div>
              <div style={{ display: "flex", gap: 8, marginTop: 6 }}>
                {[1, 2, 4, 8].map((hz) => (
                  <Toggle key={hz} on={false} label={`hb ${hz} Hz`}
                    onClick={() => { void fetch(`/cambium/debug/rate?hz=${hz}`).catch(() => {}); }} />
                ))}
              </div>
              <div style={{ color: "#7e8ea6", fontSize: 11 }}>
                heartbeat-rate buttons broadcast NB_SET_RATE to awake lanterns
              </div>
            </Section>
            <Section id="set-about" label="ⓘ About the light data">
              <div style={{ color: "#7e8ea6", fontSize: 12, lineHeight: 1.7 }}>
                <b>unmapped</b> — that lantern's MAC has no slot in the model yet,
                so height/role are unknown until the MAC↔slot join (Calibrate →
                commissioning, or Justin's Constellate camera sweep).<br />
                <b>presence & neighbor events</b> — reserved-but-unsent in Ben's
                firmware (NB_EVENT); "what it does when someone walks under"
                cannot be shown until that ships.<br />
                <b>knock</b> — needs the striker fitted AND the lantern awake in
                dev profile; the 433 remote bypasses all of this in hardware.
              </div>
            </Section>
            <Section id="set-dev" label="🛠 Heavy-dev mode (use with charged batteries)">
              <div style={{ display: "flex", gap: 8 }}>
                <Toggle on={false} accent="#3ddc97" label="stay-awake + OTA on"
                  onClick={() => { void fetch("/cambium/debug/maint?on=1").catch(() => {}); }} />
                <Toggle on={false} label="resume normal"
                  onClick={() => { void fetch("/cambium/debug/maint?on=0").catch(() => {}); }} />
              </div>
              <div style={{ color: "#7e8ea6", fontSize: 11, lineHeight: 1.5 }}>
                Stay-awake joins lanterns to shared WiFi with the OTA endpoint up
                (Ben's maintenance mode). WiFi costs battery — turn it off when done.
              </div>
            </Section>
          </>
        )}

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
            {/* full dials here too (Elliot 08-15: "for Light Show mode are there
                still controls for speed brightness etc") — master alone wasn't it */}
            <BigSlider label="master" v={ctrl.master} min={0} max={1} step={0.01} on={(v) => set({ master: v })} />
            <BigSlider label="brightness" v={ctrl.brightness} min={0} max={1} step={0.01} on={(v) => set({ brightness: v })} />
            <BigSlider label="speed" v={ctrl.speed} min={0} max={3} step={0.01} on={(v) => set({ speed: v })} />
            <BigSlider label="hue" v={ctrl.hue} min={0} max={1} step={0.01} on={(v) => set({ hue: v })} />
          </>
        )}

        {tab === "interactive" && (
          <>
            <div style={{ display: "flex", gap: 10 }}>
              <Toggle on={false} label="✨ ping the tree" onClick={() => pingPresence()} accent={AMBER} />
              <Toggle on={ctrl.aiPilot} label="🤖 AI pilot" onClick={() => set({ aiPilot: !ctrl.aiPilot })} accent="#9b6bff" />
            </div>

            {/* GAME OF LIGHT — the ignition lifecycle was desktop-only until
                08-15 (Elliot: "why doesn't interactive show the game of life").
                The pads alone set pattern=life with gol.phase stuck at "off" —
                the bare CA field, not the experience. This arms the real thing. */}
            <Section id="gol" label={`Game of Light · ${
              golPhase === "off" ? "not armed" :
              golPhase === "standby" ? "🌙 standby — waiting for first visitor" :
              golPhase === "live" ? "🟢 LIVE" : "⚡ igniting…"}`}>
              <div style={{ display: "flex", gap: 8 }}>
                {golPhase === "off"
                  ? <Toggle on={false} accent="#3ddc97" label="▶ Arm (standby)" onClick={() => armGol()} />
                  : <Toggle on={true} accent="#3ddc97" label="⏹ end" onClick={() => golSetPhase("off")} />}
                {golPhase === "standby" && (
                  <Toggle on={false} accent="#5b8cff" label="👤 sim first visitor"
                    onClick={() => golFirstVisitor(fixtures.length ? (Math.random() * fixtures.length) | 0 : 0)} />
                )}
              </div>
              <div style={{ color: "#7e8ea6", fontSize: 12, lineHeight: 1.5 }}>
                Arm it and the tree goes dark, waiting. The first tap ignites it —
                then every tap seeds life that spreads light to light.
              </div>
              {/* INSTANT sim (Elliot 08-15: "still not seeing the game of life
                  simulator" — Arm leads to a dark waiting tree, which reads as
                  nothing; these start the CA visibly RUNNING right now) */}
              <div style={{ ...microLabel, marginTop: 4 }}>or run the simulation right now</div>
              <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(94px, 1fr))", gap: 8 }}>
                {(["life", "ripples", "organism", "living", "chains"] as PatternId[]).map((rule) => (
                  <Pad key={rule} active={ctrl.pattern === rule} accent="#3ddc97" label={rule === "life" ? "🧬 life" : rule}
                    onClick={() => {
                      set({ pattern: rule, blackout: false });
                      // seed immediately so it's visibly ALIVE — an empty CA
                      // field looks identical to a broken one
                      const n = fixtures.length;
                      if (n > 0) for (let k = 0; k < 4; k++) triggerAt((Math.random() * n) | 0);
                    }} />
                ))}
              </div>
              <ThemePicker value={caTheme} onPick={setCaTheme} />
            </Section>
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

// ─────────────────────────────────────────────────────────────────────────────
// 🎛 COMMAND — the simple test surface (Elliot 08-15: "a simple command mode
// for testing. On off, color, brightness, blink").
//
// Every button routes through runScript's grammar — the same path typing,
// voice, and the AI operator use — so commands hit the SIM unconditionally,
// and hit the REAL fleet exactly when 📡 drive is armed. One frame-pump owner,
// always. BLINK is the exception by design: identify is a bounded targeted
// packet, not a frame stream, so it reaches the real lanterns even with drive
// off — that's what makes it the locate aid.
// ─────────────────────────────────────────────────────────────────────────────

const CMD_COLORS = ["red", "orange", "gold", "green", "cyan", "blue", "purple", "magenta", "white"] as const;

function useFleet(): number {
  return useSyncExternalStore(subscribeFleet, fleetVersion, fleetVersion);
}

function CommandPage() {
  useFleet(); // re-render on fleet traffic (connection chip + heard count)
  const runScript = useTwin((s) => s.runScript);
  const ctrl = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const driveReal = useTwin((s) => s.net.driveReal);
  const setNet = useTwin((s) => s.setNet);
  // BLINK acknowledgement. Declared with the other hooks, ABOVE every early
  // return — a sheet effect placed below one crashed this file on 08-14
  // ("fewer hooks than expected") and there is no reason to relearn that.
  const [cmdNote, setCmdNote] = useState<string | null>(null);
  const c = fleetCensusNow();
  const connected = fleetConnected();

  return (
    <>
      {/* one slim status row (Elliot 08-15: "more spatially efficient…
          Driving Real on by default, a little button we turn off only when
          we need it") — the pill is small BECAUSE it is usually on */}
      <div style={{ display: "flex", gap: 8, alignItems: "center", fontSize: 12 }}>
        <span style={{ color: connected ? "#7ee08c" : "#7e8ea6", flex: 1 }}>
          {connected ? `📡 ${c.heardWindow} lights (5 min)` : "○ no bridge — sim only"}
        </span>
        <button onClick={() => setNet({ driveReal: !driveReal })} aria-label="toggle drive real" style={{
          padding: "4px 10px", borderRadius: 999, fontSize: 11, fontWeight: 700, cursor: "pointer",
          border: `1px solid ${driveReal ? "#ff5b6e" : "#2b3a52"}`,
          background: driveReal ? "#ff5b6e22" : "#141a26",
          color: driveReal ? "#ff9aa6" : "#7e8ea6", touchAction: "manipulation",
        }}>{driveReal ? "● REAL" : "○ sim only"}</button>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 8 }}>
        <Pad active={false} accent="#7ee08c" label="⚡ ON" onClick={() => runScript("on")} />
        <Pad active={false} accent="#ff5b6e" label="⭘ OFF" onClick={() => runScript("off")} />
        {/* BLINK is the one pad on this page with NO sim path: ON/OFF/colours all
            route through runScript() and drive the twin unconditionally, while
            this emits a wire-only identify packet. With no daemon socket it was a
            total no-op AND said nothing — measured 2026-08-15: render movement
            exactly 0 across 8 samples, zero feedback, zero errors, while the OFF
            pad beside it moved luminance 109.8 → 3.1. That reads as "Blink is
            broken" when the truth is "there is nothing to blink". Say so. */}
        <Pad active={false} accent="#ffb454" label="✨ BLINK" onClick={() => {
          const sent = fleetIdentify(null, 3);
          setCmdNote(sent ? `blink sent to ${c.heardWindow || "the"} lantern(s) · 3 s` : "no bridge — nothing to blink");
          window.setTimeout(() => setCmdNote(null), 3000);
        }} />
      </div>
      {cmdNote && (
        <div style={{ fontSize: 12, color: cmdNote.startsWith("no bridge") ? "#ffb4b4" : "#7ee08c" }}>{cmdNote}</div>
      )}

      <div style={{ display: "grid", gridTemplateColumns: "repeat(9, 1fr)", gap: 6 }}>
        {CMD_COLORS.map((col) => (
          <button key={col} aria-label={`all ${col}`} onClick={() => runScript(`all color ${col}`)}
            style={{
              height: 44, borderRadius: 10, cursor: "pointer", border: "1.5px solid #2b3a52",
              background: col, touchAction: "manipulation",
            }} />
        ))}
      </div>

      <BigSlider label="brightness" v={ctrl.brightness} min={0} max={1} step={0.01} on={(v) => set({ brightness: v })} />
      <div style={{ display: "flex", gap: 8 }}>
        <Toggle on={false} label="clear overrides" onClick={() => runScript("clear")} />
      </div>
    </>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// 🔎 LOCATE — fleet status, told honestly (Elliot 08-15: "how many are on
// right now, what their battery level is" + "reads the status of all the
// lights"). Numbers come with their listening window because the count is a
// FUNCTION OF LISTEN TIME (measured: 14@25s → 23@20min on the same fleet);
// battery leads with millivolts because soc_pct reported 1% and 100% at
// near-identical voltage on the live fleet. Tap a row → that lantern blinks.
// ─────────────────────────────────────────────────────────────────────────────

function LocatePage() {
  useFleet();
  const c = fleetCensusNow();
  const connected = fleetConnected();
  const reg = fleetRegistry();
  const now = Date.now();
  const rows = Object.values(reg.records).sort((a, b) => b.lastSeenMs - a.lastSeenMs);
  const [detail, setDetail] = useState<string | null>(null); // mac → LightSheet

  const age = (ms: number) => {
    const s = Math.max(0, Math.round((now - ms) / 1000));
    return s < 90 ? `${s}s` : s < 3600 ? `${Math.round(s / 60)}m` : `${Math.round(s / 3600)}h`;
  };
  const litDot = (r: (typeof rows)[number]) => {
    const s = litState(r);
    return s === "lit" ? "🟡" : s === "dormant" ? "⚫" : "◌";
  };

  if (!connected && rows.length === 0) {
    return (
      <div style={{ color: "#7e8ea6", fontSize: 13, lineHeight: 1.6 }}>
        ○ No bridge connection — nothing heard yet.<br />
        The app dials the cambium daemon automatically (same-origin <code>/cambium</code>).
        If the CoreS3 bridge is unplugged there is no radio and this page stays empty:
        that is the truth, not a bug.
      </div>
    );
  }

  return (
    <>
      <div style={{ fontSize: 13, color: "#dbe6f5", lineHeight: 1.7 }}>
        <b>{c.heardRecent}</b> heard in the last minute · <b>{c.heardWindow}</b> in 5 min ·{" "}
        <b>{c.total}</b> total this session
        {c.listeningMs > 0 && <span style={{ color: "#7e8ea6" }}> (listening {Math.round(c.listeningMs / 60000)} min — the count grows with listen time)</span>}
        <br />
        🟡 <b>{c.lit}</b> lit · ⚫ <b>{c.dormant}</b> dormant · ◌ <b>{c.unknown}</b> unknown ·
        ⚡ <b>{c.charging}</b> charging
        <br />
        battery {c.medianMv !== null ? <>median <b>{c.medianMv} mV</b> ({c.minMv}–{c.maxMv})</> : "—"}
        {c.belowLedsOff > 0 && <span style={{ color: "#ffb454" }}> · {c.belowLedsOff} at/under the 2950 mV LEDs-off floor</span>}
        {c.gaugeFaults.length > 0 && <span style={{ color: "#ff5b6e" }}> · gauge fault: {c.gaugeFaults.join(", ")}</span>}
      </div>
      <div style={{ color: "#7e8ea6", fontSize: 11 }}>tap a light for its control screen</div>
      {detail && <LightSheet mac={detail} onClose={() => setDetail(null)} />}
      <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
        {rows.map((r) => (
          <button key={r.mac} onClick={() => setDetail(r.mac)}
            style={{
              display: "flex", gap: 10, alignItems: "baseline", padding: "9px 10px", borderRadius: 10,
              border: `1px solid ${detail === r.mac ? "#ffb454" : "#1c2740"}`,
              background: detail === r.mac ? "#ffb45422" : "#0d1420", color: "#dbe6f5",
              fontFamily: "ui-monospace, monospace", fontSize: 12, cursor: "pointer", touchAction: "manipulation",
            }}>
            <span>{litDot(r)}</span>
            <b style={{ minWidth: 62 }}>{nameFor(r.mac) ?? r.mac}</b>
            <span style={{ minWidth: 70 }}>
              {r.battMv > 0 && r.battMv < GAUGE_FAULT_MV ? "gauge⚠" : `${r.battMv} mV`}
              {r.battMa > 0 ? " ⚡" : r.battMa < 0 ? " ▾" : ""}
            </span>
            <span style={{ color: "#7e8ea6" }}>tier {r.powerTier ?? "?"} · prog {r.program ?? "?"}</span>
            <span style={{ marginLeft: "auto", color: "#7e8ea6" }}>{age(r.lastSeenMs)}</span>
          </button>
        ))}
      </div>
    </>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// 💡 LIGHT SHEET — one lantern's full control + status screen (Elliot 08-15:
// "when I click on a light in locate I should get a separate screen: what it
// is doing, what it is running, ring the bell..."). Everything here is HONEST:
// each field states its source, and what the radio does not report yet is
// said out loud rather than guessed (the trust rule from the AI manual §5).
// ─────────────────────────────────────────────────────────────────────────────

const LIFE_LABEL: Record<number, string> = {
  0: "0 · boot/park", 1: "1 · dormant (lights off, conserving)",
  2: "2 · waking", 3: "3 · ALIVE — running its show",
};
const TIER_LABEL: Record<number, string> = {
  0: "0 · full power", 1: "1 · dimmed (ADR-0023: ~3.00 V floor)",
  2: "2 · LEDs off, OTA windows (~2.95 V)", 3: "3 · deep conservation (~2.90 V)",
};

function LightSheet({ mac, onClose }: { mac: string; onClose: () => void }) {
  useFleet();
  const fixtures = useTwin((s) => s.fixtures);
  const selectLight = useTwin((s) => s.selectLight);
  // one per-light surface at a time: the tree's tap-chip editor closes when
  // this sheet opens (they collided in Elliot's 08-15 screenshot)
  useEffect(() => { selectLight(null); }, [selectLight]);
  const r = fleetRegistry().records[mac];
  const [note, setNote] = useState<string | null>(null);
  if (!r) return null;

  // MAC↔slot join (Calibrate commissioning owns this; often still unmapped)
  const slot = resolveFixtureId(loadCalibration(), mac);
  const fx = slot ? fixtures.find((f) => f.id === slot) : undefined;
  const age = Math.max(0, Math.round((Date.now() - r.lastSeenMs) / 1000));
  const lit = litState(r);

  const act = async (label: string, path: string) => {
    setNote(`${label}…`);
    try {
      const res = await fetch(path);
      setNote(res.ok ? `${label} ✓ sent` : `${label} failed (${res.status})`);
    } catch {
      setNote(`${label} failed — no daemon`);
    }
    window.setTimeout(() => setNote(null), 4000);
  };
  const solid = (rr: number, gg: number, bb: number) =>
    act("colour", `/cambium/debug/solid?id=${mac}&r=${rr}&g=${gg}&b=${bb}&w=0`);

  const row = (k: string, v: React.ReactNode) => (
    <div style={{ display: "flex", justifyContent: "space-between", gap: 10, fontSize: 13, lineHeight: 1.8 }}>
      <span style={{ color: "#7e8ea6" }}>{k}</span><span style={{ textAlign: "right" }}>{v}</span>
    </div>
  );

  return (
    <div style={{
      // OPAQUE — 08-15 Elliot ("not the aesthetic I want"): the translucent
      // sheet let the page underneath bleed through and collide with the
      // tap-chip editor. One surface, one moment.
      //
      // HALF-HEIGHT — 08-15 Elliot ("I would prefer it only take half the
      // screen and we can scroll down to see the data below"). Was inset:0,
      // i.e. full-screen, which hid the tree you are trying to locate. Now it
      // follows the same bottom-sheet geometry as the main mode sheet above:
      // anchored over the tab bar so the tabs stay reachable, rounded top so it
      // reads as a sheet rather than a page, and the overflow lives INSIDE it —
      // that is what makes the data below scrollable instead of clipped.
      position: "fixed", left: 0, right: 0, bottom: "calc(64px + env(safe-area-inset-bottom))",
      height: "50vh", zIndex: 260, background: "#0b0f17",
      borderTop: "1px solid #23304a", borderRadius: "18px 18px 0 0",
      display: "flex", flexDirection: "column", padding: "14px 16px 20px",
      overflowY: "auto", overscrollBehavior: "contain", WebkitOverflowScrolling: "touch",
      color: "#e7ecf6",
    }}>
      <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
        <span style={{ fontSize: 22 }}>{lit === "lit" ? "🟡" : lit === "dormant" ? "⚫" : "◌"}</span>
        <b style={{ fontSize: 20 }}>{nameFor(mac) ?? mac}</b>
        <span style={{ color: "#7e8ea6", fontFamily: "ui-monospace, monospace", fontSize: 12 }}>
          {nameFor(mac) ? mac : ""}{slot ? ` · slot ${slot}` : ""}
        </span>
        <button onClick={onClose} aria-label="close light sheet" style={{
          marginLeft: "auto", width: 40, height: 40, borderRadius: 20, fontSize: 16,
          border: "1px solid #2b3a52", background: "#141a26", color: "#dbe6f5", cursor: "pointer",
        }}>✕</button>
      </div>

      {note && <div style={{ margin: "8px 0", color: AMBER, fontSize: 13 }}>{note}</div>}

      <div style={{ ...microLabel, marginTop: 14 }}>Live from its heartbeat</div>
      {row("state", r.lifeState !== null ? (LIFE_LABEL[r.lifeState] ?? String(r.lifeState)) : "not reported")}
      {row("behavior program", r.program !== null ? (r.program === 0 ? "0 · none (parked)" : `program ${r.program}`) : "not reported")}
      {row("power protocol", r.powerTier !== null ? (TIER_LABEL[r.powerTier] ?? String(r.powerTier)) : "not reported")}
      {row("battery", r.battMv > 0 && r.battMv < GAUGE_FAULT_MV ? "⚠ gauge fault" :
        <>{r.battMv} mV {r.battMa > 0 ? `· charging +${r.battMa} mA ⚡` : r.battMa < 0 ? `· draining ${r.battMa} mA` : ""}</>)}
      {row("last heard", `${age < 90 ? `${age}s` : `${Math.round(age / 60)}m`} ago · ${r.hbCount} heartbeats · ${r.reboots} reboots`)}
      {row("radio", r.dlRssi !== 0 ? `${r.dlRssi} dBm at the lantern` : "—")}

      {fx ? (
        <>
          <div style={{ ...microLabel, marginTop: 14 }}>From the model</div>
          {row("role", fx.role)}
          {row("hang height", `${fx.pos[1].toFixed(2)} m above ground (design)`)}
          {row("zone", `${fx.zone} · light #${fx.num}`)}
        </>
      ) : (
        // compact: the full explanation lives in Settings → About the data
        row("slot", <span style={{ color: "#7e8ea6" }}>unmapped · ⓘ Settings</span>)
      )}

      <div style={{ ...microLabel, marginTop: 14 }}>Name</div>
      <input
        defaultValue={nameFor(mac) ?? ""}
        placeholder="name this lantern… (e.g. Mario)"
        onBlur={(e) => { setName(mac, e.target.value); }}
        style={{
          width: "100%", padding: "10px", marginTop: 4, borderRadius: 10, fontSize: 16,
          border: "1px solid #2b3a52", background: "#0d1420", color: "#dbe6f5",
        }}
      />

      <div style={{ ...microLabel, marginTop: 16 }}>Do things to THIS lantern</div>
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 8, marginTop: 6 }}>
        {/* report what ACTUALLY happened — this said "blink sent (3 s)" even when
            no packet left the browser, which is the worst possible answer during
            a locate walk. */}
        <Pad active={false} accent={AMBER} label="✨ blink" onClick={() => {
          const sent = fleetIdentify(mac, 3);
          setNote(sent ? "blink sent (3 s)" : "no bridge — blink NOT sent");
          window.setTimeout(() => setNote(null), 3000);
        }} />
        <Pad active={false} accent="#c8a24a" label="🔔 knock (bell)" onClick={() => act("knock", `/cambium/debug/knock?mac=${mac}&ms=40`)} />
      </div>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(6, 1fr)", gap: 6, marginTop: 8 }}>
        {([["red",255,0,0],["orange",255,120,0],["green",0,255,0],["cyan",0,200,255],["blue",0,0,255],["white",255,255,255]] as const).map(([name, rr, gg, bb]) => (
          <button key={name} aria-label={`solid ${name}`} onClick={() => solid(rr, gg, bb)} style={{
            height: 40, borderRadius: 10, border: "1.5px solid #2b3a52", cursor: "pointer",
            background: `rgb(${rr},${gg},${bb})`, touchAction: "manipulation",
          }} />
        ))}
      </div>
      <div style={{ display: "flex", gap: 8, marginTop: 8 }}>
        <Toggle on={false} label="⭘ dark" onClick={() => act("dark", `/cambium/debug/solid?id=${mac}&r=0&g=0&b=0&w=0`)} />
      </div>
      <div style={{ color: "#7e8ea6", fontSize: 11, marginTop: 8, lineHeight: 1.5 }}>
        Colour/dark go straight to the real lantern (daemon direct-frame — no
        drive toggle needed) and it falls back to its own behavior ~3 s after
        the last frame. Knock fires only if the lantern's daytime-surplus power
        gate allows it (ADR-0030) and a solenoid is fitted.
      </div>
    </div>
  );
}
