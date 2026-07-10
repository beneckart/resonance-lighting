import { useEffect, useRef, useState } from "react";
import { useTwin, CA_RULES, TRIGGER_COLOR_MODES, type PatternId, type TriggerColorMode } from "./store";
import { ThemePicker } from "./ThemePicker";
import { Widget } from "./Widget";
import { getLifeRules, setLifeRules, getCaParams, setCaParams, type LifeRules, type CaParams } from "./field";

// editable Game-of-Life rule presets (graph has ~6 neighbours, not a grid's 8)
const LIFE_PRESETS: Record<string, Partial<LifeRules>> = {
  // DEFAULT — Conway dynamics on the mesh: survive 2-3 exactly as Conway, birth
  // scaled 8-grid→6-graph (3/8 ≈ 2/6). Games run ~76 generations (median) with
  // real still-lifes + oscillators, then the watchdog deals a fresh 4-9 seed.
  "Conway B2/S23": { bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true },
  "classic B3/S23": { bLo: 3, bHi: 3, sLo: 2, sHi: 3, pure: true },
  "organic churn": { bLo: 2, bHi: 3, sLo: 1, sHi: 3, pure: false },
};
// v2: default became Conway-mesh pure — the old persisted key would pin every
// existing device to the churn rules and nobody would see the fix
const RULES_KEY = "ca.liferules.v2";

function ParamSlider({ label, v, min, max, step, on, hint }: { label: string; v: number; min: number; max: number; step: number; on: (v: number) => void; hint?: string }) {
  return (
    <div style={{ margin: "4px 0" }}>
      <div style={{ fontSize: 10, color: "#8aa0bb" }}>{label}</div>
      <input type="range" min={min} max={max} step={step} value={v} onChange={(e) => on(+e.target.value)} style={{ width: "100%", accentColor: "#5b8cff" }} title={hint} />
    </div>
  );
}

// log-mapped speed slider: u∈0..1 → speed 0.03..4 (exponential), so HALF the travel
// is the slow zone. Life gen-period ≈ 2.0·speed^-1.15 (shown live in the label).
const SPD_MIN = 0.03, SPD_MAX = 4;
const uToSpeed = (u: number) => SPD_MIN * Math.pow(SPD_MAX / SPD_MIN, u);
const speedToU = (s: number) => Math.log(Math.max(SPD_MIN, Math.min(SPD_MAX, s)) / SPD_MIN) / Math.log(SPD_MAX / SPD_MIN);
const genPeriod = (s: number) => Math.min(120, 1.0 * Math.pow(Math.max(0.02, s), -1.15)); // MUST match field.ts TICK (baseline 1 s/turn)
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
  life: { name: "Game of Life", blurb: "Conway on the light mesh · games end, fresh seeds deal in", emoji: "🌱", hue: 0.05 },
  ripples: { name: "Excitable", blurb: "waves ripple out & fade · Greenberg-Hastings", emoji: "💫", hue: 0.55 },
  organism: { name: "Reaction-Diffusion", blurb: "blobs drift, split & merge · Gray-Scott", emoji: "🫧", hue: 0.5 },
  living: { name: "Firefly Sync", blurb: "fireflies fall into travelling waves · Kuramoto", emoji: "✨", hue: 0.12 },
};
const CM_LABEL: Record<TriggerColorMode, string> = { fixed: "one colour", random: "random / touch", cycle: "cycle" };

// HOW EACH MODE WORKS — Elliot: "showing the rules for how each interactive mode
// is working would be super useful." Plain-language mechanism + the live rule.
const RULE_EXPLAIN: Record<string, { rule: string; how: string; onTouch: string; rest: string }> = {
  life: {
    rule: "", // built live from K + birth/survive below
    how: "A Game of Life REDESIGNED for the tree's geometry: instead of Conway's 8-neighbour square grid, each light counts its K nearest lights in 3-D. Survival 2–3 keeps Conway's feel; birth is tuned to K (at K=6, born on 2 — Conway's grid birth-3 just dies out on a sparse mesh). Widen K above and raise the birth count to match.",
    onTouch: "a touch is born as live cells that then evolve by this rule and spread hop-by-hop.",
    rest: "when a game ends (dies out or freezes) a fresh 4–9-light seed is dealt in.",
  },
  ripples: {
    rule: "Each light is RESTING, EXCITED, or COOLING. A resting light EXCITES next turn if any of its K nearest lights is excited. An excited light then COOLS for a few turns before it can fire again.",
    how: "Excitable medium (Greenberg-Hastings) — the rule that models nerve impulses and forest fires. Waves can only travel forward (the cool-down stops them backing up), so they roll outward and fade.",
    onTouch: "a touch excites the medium there; the wave spreads one hop per turn and fades behind it.",
    rest: "dark and quiet until someone touches or walks by.",
  },
  organism: {
    rule: "Each light holds two virtual chemicals. It DIFFUSES them toward its K nearest lights and they REACT (one feeds on the other). Where the second chemical builds up, the light glows.",
    how: "Reaction-Diffusion (Gray-Scott) — the maths behind leopard spots and coral. Blobs grow, drift, split and merge. Tune feed/kill below for spots ↔ stripes ↔ churn.",
    onTouch: "a touch injects a fresh blob of chemical that blooms then drifts away.",
    rest: "a faint breath; the chemistry only shows where people are.",
  },
  living: {
    rule: "Each light has a flash timer. Every tick it nudges its timer toward the average of its K nearest lights' timers, so they drift INTO STEP and flash together in travelling waves.",
    how: "Firefly synchronisation (Kuramoto) — how real fireflies end up blinking in unison. Sync strength below sets how strongly neighbours pull together (scattered ↔ whole-tree pulse).",
    onTouch: "a touch flashes that region and re-triggers the sync wave outward from it.",
    rest: "a faint breath; the swarm only lights where it's stirred.",
  },
};

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
  const caTheme = useTwin((s) => s.caTheme);
  const setCaTheme = useTwin((s) => s.setCaTheme);
  const announce = useTwin((s) => s.announce);
  const enterCa = useTwin((s) => s.enterCa);
  const [rules, setRulesUi] = useState<LifeRules>(() => getLifeRules());
  const [cap, setCapUi] = useState<CaParams>(() => getCaParams());
  const applyCap = (p: Partial<CaParams>) => { setCaParams(p); const n = getCaParams(); setCapUi(n); try { localStorage.setItem("ca.params.v1", JSON.stringify(n)); } catch { /* fine */ } };
  // sync the engine with the persisted theme + life rules once on mount
  useEffect(() => {
    setCaTheme(useTwin.getState().caTheme);
    try {
      const raw = localStorage.getItem(RULES_KEY);
      if (raw) { const r = JSON.parse(raw) as LifeRules; setLifeRules(r); setRulesUi(getLifeRules()); }
      const rawP = localStorage.getItem("ca.params.v1");
      if (rawP) { setCaParams(JSON.parse(rawP) as CaParams); setCapUi(getCaParams()); }
    } catch { /* fine */ }
  }, [setCaTheme]);
  const applyRules = (p: Partial<LifeRules>) => {
    setLifeRules(p);
    const next = getLifeRules();
    setRulesUi(next);
    try { localStorage.setItem(RULES_KEY, JSON.stringify(next)); } catch { /* fine */ }
  };
  const active = control.pattern;
  const isCA = (CA_RULES as PatternId[]).includes(active);
  const PHASE_LABEL: Record<string, string> = {
    off: "— not armed —", standby: "🌙 standby · waiting for first visitor",
    off1: "○ sensed — going dark", flash: "✷ ignition flourish", off2: "○ dark", live: "🟢 LIVE · interactive",
  };

  const pickRule = (r: PatternId) => {
    // ENTRY CEREMONY (Elliot): dark → themed flourish ("entering this mode") →
    // dark → Game of Life starts from a fresh 4-9-light seed. store.enterCa owns it.
    if (caTheme === "random") set({ sat: 0.85, hue: RULE_META[r]?.hue ?? control.hue });
    enterCa(r);
    setTr({ rule: r });
    setTod(0); // night — the living field reads best against black
  };
  const pokeRandom = () => {
    if (!isCA) pickRule(tr.rule);
    if (fixtures.length) trigger((Math.random() * fixtures.length) | 0);
  };
  // ONE PERSON WALKING THROUGH triggers MANY lights (Elliot): a virtual visitor
  // strolls ~a third of the way around the outer canopy, firing the nearest
  // sensor as they pass — each footfall seeds cells that then LIVE by the rule.
  const walkRef = useRef<number[]>([]);
  const simWalk = () => {
    if (!isCA) pickRule(tr.rule);
    walkRef.current.forEach((t) => clearTimeout(t));
    walkRef.current = [];
    const st = useTwin.getState();
    const outer = st.fixtures.map((f, i) => ({ f, i })).filter((x) => x.f.role === "downlight" && x.f.radialT >= 0.4);
    if (!outer.length) return;
    const a0 = Math.random() * Math.PI * 2;
    const dir = Math.random() < 0.5 ? 1 : -1;
    const STEPS = 9;
    for (let k = 0; k < STEPS; k++) {
      const az = a0 + dir * (k / STEPS) * (Math.PI * 2 * 0.38); // ~a third of the circle
      let best = outer[0].i, bd = Infinity;
      for (const x of outer) {
        let d = Math.abs(x.f.azimuth - ((az + Math.PI) % (Math.PI * 2)) + Math.PI) % (Math.PI * 2);
        d = Math.min(d, Math.PI * 2 - d);
        if (d < bd) { bd = d; best = x.i; }
      }
      walkRef.current.push(window.setTimeout(() => useTwin.getState().triggerAt(best), k * 850));
    }
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
        {/* Unity/community mode has NO button in the main flow — it only triggers
            ORGANICALLY when visitors form a chain of lit nodes all the way around.
            Demo/testing shortcuts live in the collapsed 🧪 drawer at the bottom. */}
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

      {/* HOW THE ACTIVE MODE WORKS — the actual rule, in plain language */}
      {isCA && RULE_EXPLAIN[active] && (
        <div style={{ margin: "2px 0 8px", padding: "8px 9px", borderRadius: 8, background: "#0c1620", border: "1px solid #21323f" }}>
          <div style={{ fontSize: 10, fontWeight: 700, color: "#7fd0b0", marginBottom: 4 }}>ⓘ The rule — how each light reacts to its neighbours</div>
          <div style={{ fontSize: 10.5, color: "#dfe9f5", lineHeight: 1.45, fontWeight: 600, padding: "5px 7px", background: "#101c14", borderRadius: 6, border: "1px solid #234a30" }}>{active === "life" ? `Each light counts its ${cap.neighbourK} nearest lights every turn. LIT: stays lit if ${rules.sLo}${rules.sHi !== rules.sLo ? "–" + rules.sHi : ""} are lit, else turns off. DARK: turns on if ${rules.bLo}${rules.bHi !== rules.bLo ? "–" + rules.bHi : ""} are lit.` : RULE_EXPLAIN[active].rule}</div>
          <div style={{ fontSize: 9.5, color: "#9fb0c7", lineHeight: 1.4, marginTop: 5 }}>{RULE_EXPLAIN[active].how}</div>
          <div style={{ fontSize: 9.5, color: "#8fb9a6", marginTop: 4 }}><b style={{ color: "#b7f5db" }}>on touch:</b> {RULE_EXPLAIN[active].onTouch}</div>
          <div style={{ fontSize: 9.5, color: "#8aa0bb", marginTop: 2 }}><b>at rest:</b> {RULE_EXPLAIN[active].rest}</div>
        </div>
      )}

      {/* ⏱ TURN SPEED — how fast each light triggers the next; drives EVERY rule.
          Front and centre (Elliot could not find it below the fold). */}
      <div style={{ marginTop: 8, padding: "7px 8px", borderRadius: 8, background: "#0e1826", border: "1px solid #1d2f28" }}>
        <div style={{ fontWeight: 700, color: "#f0d890", fontSize: 11.5, marginBottom: 4 }}>
          ⏱ TURN SPEED · one turn every {fmtPeriod(genPeriod(control.speed))}
        </div>
        <div style={{ display: "flex", gap: 4, marginBottom: 4 }}>
          {([["🐢 slow", 0.25], ["▶ baseline", 1], ["⚡ fast", 2.5], ["🚀 turbo", 4]] as [string, number][]).map(([lb, v]) => {
            const on = Math.abs(control.speed - v) < 0.03;
            return (
              <button key={lb} onClick={() => set({ speed: v })}
                style={{ flex: 1, padding: "5px 2px", borderRadius: 6, cursor: "pointer", fontSize: 10, fontWeight: 700,
                  border: on ? "1.5px solid #f0d890" : "1px solid #2a3a52", background: on ? "#2a2410" : "#121a26", color: on ? "#f0d890" : "#9fb0c7" }}>
                {lb}
              </button>
            );
          })}
        </div>
        <input type="range" min={0} max={1} step={0.005} value={speedToU(control.speed)}
          onChange={(e) => set({ speed: uToSpeed(+e.target.value) })}
          style={{ width: "100%", accentColor: "#5b8cff" }} />
      </div>

      <div style={{ display: "flex", gap: 6, marginTop: 8 }}>
        <button onClick={pokeRandom}
          title="fire a sensor at a random spot (or just tap the tree in the 3D view)"
          style={{ flex: 1, padding: "9px 8px", borderRadius: 9, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: "1.5px solid #3ddc97", background: "#12402f", color: "#b7f5db", boxShadow: "0 0 14px #3ddc9744" }}>
          👋 Poke a spot
        </button>
        <button onClick={simWalk}
          title="ONE person walking under the canopy — the nearest sensor fires as they pass; every footfall's cells then live by the rule"
          style={{ flex: 1, padding: "9px 8px", borderRadius: 9, cursor: "pointer", fontWeight: 700, fontSize: 12,
            border: "1.5px solid #5b8cff", background: "#16264a", color: "#cfe0ff", boxShadow: "0 0 14px #5b8cff44" }}>
          🚶 Sim a walk-through
        </button>
      </div>

      {/* ── the running field itself ── */}
      <div style={{ marginTop: 10, paddingTop: 8, borderTop: "1px solid #1d2735" }}>
        <div style={{ fontWeight: 700, color: "#eef3fb", marginBottom: 6 }}>🌿 The field</div>
        {isCA && (
          <div style={{ marginBottom: 8, padding: "7px 8px", borderRadius: 8, background: "#0e1620", border: "1px solid #21323f" }}>
            <div style={{ fontSize: 10.5, fontWeight: 700, color: "#9fd0ff" }}>△ Geometry · each light reacts to its {cap.neighbourK} nearest lights</div>
            <div style={{ fontSize: 9, color: "#7a8ba3", margin: "1px 0 3px" }}>the tree is a 3-D mesh (not a grid) — this sets the neighbourhood every rule below counts over</div>
            <input type="range" min={3} max={12} step={1} value={cap.neighbourK}
              onChange={(e) => applyCap({ neighbourK: +e.target.value })} style={{ width: "100%", accentColor: "#5b8cff" }} />
          </div>
        )}
        {announce.phase !== "idle" && (
          <div style={{ fontSize: 10.5, color: "#f0d890", margin: "2px 0 6px" }}>
            ✷ entering {RULE_META[announce.target]?.name ?? announce.target} — dark → flourish → fresh seed…
          </div>
        )}
        {active === "life" && (
          <Row label={`Life rule · of ${cap.neighbourK} neighbours: born ${rules.bLo}${rules.bHi !== rules.bLo ? "-" + rules.bHi : ""} · survive ${rules.sLo}${rules.sHi !== rules.sLo ? "-" + rules.sHi : ""}${rules.pure ? " · pure" : ""}`}>
            <div style={{ display: "flex", gap: 4, marginBottom: 4 }}>
              {Object.entries(LIFE_PRESETS).map(([name, p]) => {
                const on = rules.bLo === (p.bLo ?? rules.bLo) && rules.bHi === (p.bHi ?? rules.bHi) && rules.sLo === (p.sLo ?? rules.sLo) && rules.sHi === (p.sHi ?? rules.sHi) && rules.pure === (p.pure ?? rules.pure);
                return (
                  <button key={name} onClick={() => applyRules(p)}
                    style={{ flex: 1, padding: "4px 2px", borderRadius: 6, cursor: "pointer", fontSize: 9,
                      border: on ? "1px solid #3ddc97" : "1px solid #2a3a52", background: on ? "#10362a" : "#121a26", color: on ? "#a9f0d4" : "#9fb0c7" }}>
                    {name}
                  </button>
                );
              })}
            </div>
            <div style={{ display: "flex", gap: 8, alignItems: "center", fontSize: 10, color: "#8aa0bb", flexWrap: "wrap" }}>
              <span>birth</span>
              <Stepper v={rules.bLo} set={(v) => applyRules({ bLo: v, bHi: Math.max(v, rules.bHi) })} />
              <span>–</span>
              <Stepper v={rules.bHi} set={(v) => applyRules({ bHi: v, bLo: Math.min(v, rules.bLo) })} />
              <span style={{ marginLeft: 6 }}>survive</span>
              <Stepper v={rules.sLo} set={(v) => applyRules({ sLo: v, sHi: Math.max(v, rules.sHi) })} />
              <span>–</span>
              <Stepper v={rules.sHi} set={(v) => applyRules({ sHi: v, sLo: Math.min(v, rules.sLo) })} />
            </div>
            <label style={{ display: "flex", alignItems: "center", gap: 6, marginTop: 4, fontSize: 10, color: "#8aa0bb", cursor: "pointer" }}>
              <input type="checkbox" checked={rules.pure} onChange={(e) => applyRules({ pure: e.target.checked })} />
              pure (textbook) — exact rule only: no churn, ageing or burn-out
            </label>
          </Row>
        )}
        {active === "ripples" && (
          <Row label="Excitable rules">
            <ParamSlider label={`wave cool-down · ${cap.ghKappa} steps`} v={cap.ghKappa} min={3} max={20} step={1} on={(v) => applyCap({ ghKappa: v })} hint="how long a light rests before it can flash again — longer = slower, more spaced waves" />
            <ParamSlider label={`self-ignite · ${cap.ghSeed <= 0.0002 ? "off (taps only)" : (cap.ghSeed * 1000).toFixed(1) + "‰"}`} v={cap.ghSeed} min={0} max={0.02} step={0.0005} on={(v) => applyCap({ ghSeed: v })} hint="0 = the tree waits for people; higher = it sparks its own waves" />
          </Row>
        )}
        {active === "organism" && (
          <Row label="Reaction-Diffusion rules">
            <ParamSlider label={`feed · ${cap.rdFeed.toFixed(3)}`} v={cap.rdFeed} min={0.01} max={0.06} step={0.001} on={(v) => applyCap({ rdFeed: v })} hint="how fast new material grows — shifts spots ↔ stripes ↔ churn" />
            <ParamSlider label={`kill · ${cap.rdKill.toFixed(3)}`} v={cap.rdKill} min={0.045} max={0.07} step={0.001} on={(v) => applyCap({ rdKill: v })} hint="how fast blobs dissolve — tune with feed for different textures" />
            <button onClick={() => applyCap({ rdFeed: 0.025, rdKill: 0.06 })} style={{ marginTop: 4, padding: "3px 8px", borderRadius: 6, cursor: "pointer", fontSize: 9, border: "1px solid #2a3a52", background: "#121a26", color: "#9fb0c7" }}>reset (gentle spots)</button>
          </Row>
        )}
        {active === "living" && (
          <Row label="Firefly Sync rules">
            <ParamSlider label={`sync strength · ${cap.ffCouple.toFixed(2)}`} v={cap.ffCouple} min={0} max={0.8} step={0.02} on={(v) => applyCap({ ffCouple: v })} hint="0 = each light flashes alone (scattered); high = the whole tree pulses together" />
            <ParamSlider label={`flash rate · ${cap.ffRate.toFixed(2)}`} v={cap.ffRate} min={0.03} max={0.3} step={0.01} on={(v) => applyCap({ ffRate: v })} hint="how often the fireflies flash" />
          </Row>
        )}
        <Row label="Colour theme — the mood the field lives in">
          <ThemePicker value={caTheme} onPick={setCaTheme} />
        </Row>
        <Row label={`Brightness · ${Math.round(control.brightness * 100)}%`}>
          <input type="range" min={0.1} max={1} step={0.02} value={control.brightness}
            onChange={(e) => set({ brightness: +e.target.value })} style={{ width: "100%" }} />
        </Row>
      </div>

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
        {tr.colorMode !== "fixed" && (
          <label style={{ display: "flex", alignItems: "center", gap: 6, marginBottom: 8, fontSize: 10, color: "#8aa0bb", cursor: "pointer" }}>
            <input type="checkbox" checked={tr.noRepeatColor} onChange={(e) => setTr({ noRepeatColor: e.target.checked })} />
            never the same colour as the last
          </label>
        )}
        <label style={{ display: "flex", alignItems: "center", gap: 6, marginBottom: 4, fontSize: 10, color: "#8aa0bb", cursor: "pointer" }}>
          <input type="checkbox" checked={tr.briRange} onChange={(e) => setTr({ briRange: e.target.checked })} />
          different brightness each time, within a range
        </label>
        {tr.briRange ? (
          <Row label={`Brightness range · ${tr.briLo.toFixed(1)}× – ${tr.briHi.toFixed(1)}×`}>
            <div style={{ display: "flex", gap: 6 }}>
              <input type="range" min={0.2} max={2.5} step={0.1} value={tr.briLo}
                onChange={(e) => setTr({ briLo: +e.target.value })} style={{ flex: 1 }} />
              <input type="range" min={0.2} max={2.5} step={0.1} value={tr.briHi}
                onChange={(e) => setTr({ briHi: +e.target.value })} style={{ flex: 1 }} />
            </div>
          </Row>
        ) : (
          <Row label={`Brightness · ${tr.intensity.toFixed(1)}×`}>
            <input type="range" min={0.2} max={2.5} step={0.1} value={tr.intensity}
              onChange={(e) => setTr({ intensity: +e.target.value })} style={{ width: "100%" }} />
          </Row>
        )}
        <Row label={`Time on · ${tr.duration.toFixed(1)}s`}>
          <input type="range" min={0.5} max={15} step={0.5} value={tr.duration}
            onChange={(e) => setTr({ duration: +e.target.value })} style={{ width: "100%" }} />
        </Row>
        <Row label={`Spread · ${tr.spread.toFixed(1)}${control.pattern === "life" ? ` · ${Math.max(1, Math.round(tr.spread * 2))} hops` : ""}`}>
          <input type="range" min={0.3} max={2} step={0.1} value={tr.spread}
            onChange={(e) => setTr({ spread: +e.target.value })} style={{ width: "100%" }} />
        </Row>
      </div>

      {/* ── 🧪 test & demo drawer — collapsed, OUT of the operator flow. Unity itself
          only ever triggers organically (a real chain of nodes around the tree). ── */}
      <details style={{ marginTop: 10, borderTop: "1px solid #1d2735", paddingTop: 6 }}>
        <summary style={{ cursor: "pointer", fontSize: 10, color: "#6f8a7e" }}>🧪 test & demo</summary>
        <div style={{ display: "flex", gap: 4, marginTop: 6 }}>
          <button onClick={() => { const st = useTwin.getState(); const byQ: Record<number, number[]> = { 0: [], 1: [], 2: [], 3: [] }; st.fixtures.forEach((f, i) => { if (f.role === "downlight" && f.radialT >= 0.45) byQ[f.quadrant]?.push(i); }); [0, 1, 2, 3].forEach((q) => { const a = byQ[q]; if (a.length) { st.addNode(a[(a.length * 0.3) | 0]); st.addNode(a[(a.length * 0.7) | 0]); } }); }}
            style={btn("#5a4a6a", "#171022", "#b8a0d0")}>sim ring (unity test)</button>
          <button onClick={pokeRandom} style={btn("#2a3a52", "#141a26", "#9fb0c7")}>poke random</button>
        </div>
      </details>
    </Widget>
  );
}

function btn(border: string, bg: string, color: string): React.CSSProperties {
  return { flex: 1, padding: "5px 6px", borderRadius: 6, cursor: "pointer", fontSize: 10.5, fontWeight: 700, border: `1px solid ${border}`, background: bg, color };
}

function Stepper({ v, set }: { v: number; set: (v: number) => void }) {
  const b: React.CSSProperties = { width: 18, height: 18, lineHeight: "14px", padding: 0, borderRadius: 5, border: "1px solid #2a3a52", background: "#121a26", color: "#9fb0c7", cursor: "pointer", fontSize: 11 };
  return (
    <span style={{ display: "inline-flex", gap: 2, alignItems: "center" }}>
      <button style={b} onClick={() => set(Math.max(0, v - 1))}>−</button>
      <b style={{ color: "#cfeede", minWidth: 10, textAlign: "center" }}>{v}</b>
      <button style={b} onClick={() => set(Math.min(8, v + 1))}>+</button>
    </span>
  );
}

function Row({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <label style={{ display: "block", marginBottom: 8 }}>
      <div style={{ fontSize: 10, color: "#8aa0bb", marginBottom: 2 }}>{label}</div>
      {children}
    </label>
  );
}
