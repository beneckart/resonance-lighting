import { useState } from "react";
import { useTwin, CA_RULES, TRIGGER_COLOR_MODES, type PatternId, type TriggerColorMode } from "./store";
import { setLifeState } from "./field";
import { Widget } from "./Widget";

// log-mapped speed slider: u∈0..1 → speed 0.03..4 (exponential), so HALF the travel
// is the slow zone. Life gen-period ≈ 2.0·speed^-1.15 (shown live in the label).
const SPD_MIN = 0.03, SPD_MAX = 4;
const uToSpeed = (u: number) => SPD_MIN * Math.pow(SPD_MAX / SPD_MIN, u);
const speedToU = (s: number) => Math.log(Math.max(SPD_MIN, Math.min(SPD_MAX, s)) / SPD_MIN) / Math.log(SPD_MAX / SPD_MIN);
const genPeriod = (s: number) => Math.min(120, 2.0 * Math.pow(Math.max(0.02, s), -1.15));
const fmtPeriod = (p: number) => (p >= 60 ? `${(p / 60).toFixed(1)}min` : p >= 10 ? `${Math.round(p)}s` : `${p.toFixed(1)}s`);

/** INTERACTIVITY MODE — the tree lives on its own DECENTRALISED rules (Ben's
 *  BACKGROUND.md mesh spec: each light runs a simple local rule over its pre-baked
 *  neighbour list) and reacts to presence. Sibling to Light-Show mode: shows are
 *  authored/central, this is emergent/local + interactive.
 *
 *  TAP THE TREE to fire a "motion sensor" at that spot — the RULES EDITOR below says
 *  what happens (reaction colour, intensity, how far it spreads across neighbours,
 *  which CA runs). Many taps/touches fire at once. For Game of Life a tap also
 *  births live cells there, so the disturbance propagates hop-by-hop through the mesh. */
const RULE_META: Record<string, { name: string; blurb: string; emoji: string; hue: number }> = {
  life: { name: "Game of Life", blurb: "cells born & die by neighbour count", emoji: "🌱", hue: 0.05 },
  ripples: { name: "Excitable", blurb: "waves ripple out & fade · Greenberg-Hastings", emoji: "💫", hue: 0.55 },
  organism: { name: "Reaction-Diffusion", blurb: "blobs drift, split & merge · Gray-Scott", emoji: "🫧", hue: 0.5 },
  living: { name: "Firefly Sync", blurb: "fireflies fall into travelling waves · Kuramoto", emoji: "✨", hue: 0.12 },
};
const CM_LABEL: Record<TriggerColorMode, string> = { fixed: "one colour", random: "random / touch", cycle: "cycle" };

export function InteractivityPanel() {
  const control = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const tr = useTwin((s) => s.triggerRule);
  const setTr = useTwin((s) => s.setTriggerRule);
  const trigger = useTwin((s) => s.triggerAt);
  const fixtures = useTwin((s) => s.fixtures);
  const setTod = useTwin((s) => s.setTimeOfDay);
  const gol = useTwin((s) => s.gol);
  const armGol = useTwin((s) => s.armGol);
  const golSetPhase = useTwin((s) => s.golSetPhase);
  const clearNodes = useTwin((s) => s.clearNodes);
  const setGolAmbient = useTwin((s) => s.setGolAmbient);
  const [palette, setPalette] = useState<"warm" | "random">("warm");
  const active = control.pattern;
  const isCA = (CA_RULES as PatternId[]).includes(active);
  const PHASE_LABEL: Record<string, string> = {
    off: "— not armed —", standby: "🌙 standby · waiting for first visitor",
    off1: "○ sensed — going dark", flash: "✷ ignition flourish", off2: "○ dark", live: "🟢 LIVE · interactive",
  };

  const pickRule = (r: PatternId) => {
    set({
      pattern: r, colorCycle: "off", order: "linear", reverse: false,
      strobe: false, blackout: false, beaconPreempt: false, master: 1,
      brightness: 0.95, sat: 0.85, hue: RULE_META[r]?.hue ?? control.hue,
    });
    setTr({ rule: r });
    setTod(0); // night — the living field reads best against black
  };
  const pokeRandom = () => {
    if (!isCA) pickRule(tr.rule);
    if (fixtures.length) trigger((Math.random() * fixtures.length) | 0);
  };

  return (
    <Widget id="interactivity" title="🌱 Interactivity" x={568} y={12} w={244} h={560} accent="#3ddc97">
      <div style={{ fontSize: 10.5, color: "#8fb9a6", lineHeight: 1.35, marginBottom: 8 }}>
        The tree lives on its own <b>local rules</b> — each light decides from its
        neighbours. <b style={{ color: "#b7f5db" }}>Tap the tree</b> to fire a sensor there;
        many touches at once. The rules below say what a touch does.
      </div>

      {/* ── GAME OF LIGHT lifecycle: arm → first visitor → ignite → live nodes ── */}
      <div style={{ padding: "7px 8px", borderRadius: 8, background: gol.unity ? "#2a1040" : "#0e1826", border: `1px solid ${gol.unity ? "#b060ff" : "#1d2f28"}`, marginBottom: 8 }}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
          <span style={{ fontWeight: 700, color: "#cfeede" }}>🎇 Game of Light</span>
          <span style={{ fontSize: 9.5, color: gol.phase === "live" ? "#7af0c0" : "#8aa0bb" }}>{PHASE_LABEL[gol.phase]}</span>
        </div>
        {gol.unity && <div style={{ fontSize: 11, fontWeight: 700, color: "#e0a0ff", marginTop: 4, textAlign: "center" }}>🌈 UNITY — community mode!</div>}
        <div style={{ display: "flex", gap: 4, marginTop: 6 }}>
          {gol.phase === "off"
            ? <button onClick={armGol} style={btn("#3ddc97", "#12402f", "#b7f5db")}>▶ Arm (standby)</button>
            : <button onClick={() => golSetPhase("off")} style={btn("#5a3a3a", "#1a1016", "#ff8fa0")}>⏹ Disarm</button>}
          {gol.phase === "standby" && <button onClick={() => useTwin.getState().golFirstVisitor(fixtures.length ? (Math.random() * fixtures.length) | 0 : 0)} style={btn("#5b8cff", "#21345e", "#dce6ff")}>👤 Sim first visitor</button>}
        </div>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginTop: 6 }}>
          <span style={{ fontSize: 10, color: "#8aa0bb" }}>nodes (visitors): <b style={{ color: "#cfeede" }}>{gol.nodes.length}</b></span>
          <div style={{ display: "flex", gap: 4 }}>
            <button onClick={() => setGolAmbient(!gol.ambient)} title="ambient field vs dark-at-rest" style={btn(gol.ambient ? "#c8a24a" : "#2a3a52", gol.ambient ? "#2a2410" : "#141a26", gol.ambient ? "#f0d890" : "#9fb0c7")}>{gol.ambient ? "☀ ambient" : "🌙 dark-rest"}</button>
            <button onClick={clearNodes} style={btn("#2a3a52", "#141a26", "#9fb0c7")}>clear</button>
          </div>
        </div>
        {/* demo: drop a ring of OUTER-downlight nodes all the way around → Unity.
            (ring detection is sensor-real: only ground-reachable outer downlights count) */}
        <button onClick={() => { const st = useTwin.getState(); const byQ: Record<number, number[]> = { 0: [], 1: [], 2: [], 3: [] }; st.fixtures.forEach((f, i) => { if (f.role === "downlight" && f.radialT >= 0.45) byQ[f.quadrant]?.push(i); }); [0, 1, 2, 3].forEach((q) => { const a = byQ[q]; if (a.length) { st.addNode(a[(a.length * 0.3) | 0]); st.addNode(a[(a.length * 0.7) | 0]); } }); }}
          style={{ ...btn("#b060ff", "#2a1040", "#e0b0ff"), width: "100%", marginTop: 6 }}>
          🌈 Sim community ring → Unity
        </button>
      </div>

      {(CA_RULES as PatternId[]).map((r) => {
        const m = RULE_META[r];
        const on = active === r;
        return (
          <button key={r} onClick={() => pickRule(r)}
            style={{ display: "block", width: "100%", textAlign: "left", margin: "4px 0", padding: "6px 8px", borderRadius: 7, cursor: "pointer",
              border: on ? "1px solid #3ddc97" : "1px solid #2a3a52", background: on ? "#10362a" : "#121a26", color: on ? "#a9f0d4" : "#9fb0c7" }}>
            <div style={{ fontWeight: 700 }}>{on ? "● " : "○ "}{m.emoji} {m.name}</div>
            <div style={{ fontSize: 9.5, opacity: 0.7 }}>{m.blurb}</div>
          </button>
        );
      })}

      <button onClick={pokeRandom}
        title="fire a sensor at a random spot (or just tap the tree in the 3D view)"
        style={{ width: "100%", marginTop: 8, padding: "9px 8px", borderRadius: 9, cursor: "pointer", fontWeight: 700, fontSize: 12.5,
          border: "1.5px solid #3ddc97", background: "#12402f", color: "#b7f5db", boxShadow: "0 0 14px #3ddc9744" }}>
        👋 Poke a random spot
      </button>

      {/* ── RULES EDITOR: what a sensor firing DOES ── */}
      <div style={{ marginTop: 12, paddingTop: 8, borderTop: "1px solid #1d2735" }}>
        <div style={{ fontWeight: 700, color: "#eef3fb", marginBottom: 6 }}>⚙ When a sensor fires…</div>

        <Row label="Colour">
          <div style={{ display: "flex", gap: 4 }}>
            {TRIGGER_COLOR_MODES.map((cm) => (
              <button key={cm} onClick={() => setTr({ colorMode: cm })}
                style={{ flex: 1, padding: "4px 2px", borderRadius: 6, cursor: "pointer", fontSize: 9.5,
                  border: tr.colorMode === cm ? "1px solid #3ddc97" : "1px solid #2a3a52", background: tr.colorMode === cm ? "#10362a" : "#121a26", color: tr.colorMode === cm ? "#a9f0d4" : "#9fb0c7" }}>
                {CM_LABEL[cm]}
              </button>
            ))}
          </div>
        </Row>
        {tr.colorMode === "fixed" && (
          <Row label={`Reaction hue · ${tr.hue.toFixed(2)}`}>
            <input type="range" min={0} max={1} step={0.01} value={tr.hue}
              onChange={(e) => setTr({ hue: +e.target.value })}
              style={{ width: "100%", accentColor: `hsl(${tr.hue * 360},85%,55%)` }} />
          </Row>
        )}
        <Row label={`Brightness · ${tr.intensity.toFixed(1)}×`}>
          <input type="range" min={0.2} max={2.5} step={0.1} value={tr.intensity}
            onChange={(e) => setTr({ intensity: +e.target.value })} style={{ width: "100%" }} />
        </Row>
        <Row label={`Time on · ${tr.duration.toFixed(1)}s`}>
          <input type="range" min={0.5} max={15} step={0.5} value={tr.duration}
            onChange={(e) => setTr({ duration: +e.target.value })} style={{ width: "100%" }} />
        </Row>
        <Row label={`Spread · ${tr.spread.toFixed(1)}${control.pattern === "life" ? ` · ${Math.max(1, Math.round(tr.spread * 2))} hops` : ""}`}>
          <input type="range" min={0.3} max={2} step={0.1} value={tr.spread}
            onChange={(e) => setTr({ spread: +e.target.value })} style={{ width: "100%" }} />
        </Row>
      </div>

      {/* ── the running field itself ── */}
      <div style={{ marginTop: 10, paddingTop: 8, borderTop: "1px solid #1d2735", opacity: isCA ? 1 : 0.4, pointerEvents: isCA ? "auto" : "none" }}>
        <div style={{ fontWeight: 700, color: "#eef3fb", marginBottom: 6 }}>🌿 The field</div>
        <Row label={`Speed · one generation every ${fmtPeriod(genPeriod(control.speed))}`}>
          {/* LOG-scaled: half the slider travel covers the SLOW zone (2min → ~4s/gen) */}
          <input type="range" min={0} max={1} step={0.005} value={speedToU(control.speed)}
            onChange={(e) => set({ speed: uToSpeed(+e.target.value) })} style={{ width: "100%" }} />
        </Row>
        <Row label="Population colours">
          <div style={{ display: "flex", gap: 4 }}>
            <button onClick={() => { setPalette("warm"); setLifeState({ palette: "warm" }); }}
              style={btn(palette === "warm" ? "#c8a24a" : "#2a3a52", palette === "warm" ? "#2a2410" : "#141a26", palette === "warm" ? "#f0d890" : "#9fb0c7")}>🔥 warm</button>
            <button onClick={() => { setPalette("random"); setLifeState({ palette: "random" }); }}
              style={btn(palette === "random" ? "#b060ff" : "#2a3a52", palette === "random" ? "#22103a" : "#141a26", palette === "random" ? "#dfb8ff" : "#9fb0c7")}>🎲 random</button>
          </div>
        </Row>
        <Row label={`Base hue · ${control.hue.toFixed(2)}`}>
          <input type="range" min={0} max={1} step={0.01} value={control.hue}
            onChange={(e) => set({ hue: +e.target.value })}
            style={{ width: "100%", accentColor: `hsl(${control.hue * 360},80%,55%)` }} />
        </Row>
        <Row label={`Brightness · ${Math.round(control.brightness * 100)}%`}>
          <input type="range" min={0.1} max={1} step={0.02} value={control.brightness}
            onChange={(e) => set({ brightness: +e.target.value })} style={{ width: "100%" }} />
        </Row>
      </div>
    </Widget>
  );
}

function btn(border: string, bg: string, color: string): React.CSSProperties {
  return { flex: 1, padding: "5px 6px", borderRadius: 6, cursor: "pointer", fontSize: 10.5, fontWeight: 700, border: `1px solid ${border}`, background: bg, color };
}

function Row({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <label style={{ display: "block", marginBottom: 8 }}>
      <div style={{ fontSize: 10, color: "#8aa0bb", marginBottom: 2 }}>{label}</div>
      {children}
    </label>
  );
}
