import type { Control } from "./store";

/** A timed LIGHT SHOW: a sequence of cues that drive the base look + per-group
 *  layers over time. The ShowPlayer steps through cues by elapsed seconds; the
 *  twin's built-in colour/brightness slew handles the cross-fades. Three shows,
 *  each ~5 min, exploring different patterns / rhythms / colours / directions /
 *  groups — "the lighting god playing with every control." */
export interface ShowLayer { group: string; control: Partial<Control>; }
export interface ShowCue { at: number; note: string; base?: Partial<Control>; layers?: ShowLayer[]; }
export interface LightShow { id: string; name: string; vibe: string; durationS: number; cues: ShowCue[]; }

// ── 🌅 AWAKENING — organic, slow, breathing. A tree waking from night to bloom ──
const AWAKENING: LightShow = {
  id: "awakening", name: "🌅 Awakening", vibe: "organic · slow · warm→cool", durationS: 300,
  cues: [
    { at: 0, note: "dormant — deep night", base: { pattern: "solid", hue: 0.62, sat: 0.7, brightness: 0.05, colorCycle: "off", order: "linear", reverse: false, strobe: false, speed: 0.3, master: 1 } },
    { at: 18, note: "first warmth rises the trunk", base: { pattern: "rising", hue: 0.08, sat: 0.9, brightness: 0.38, speed: 0.3 } },
    { at: 45, note: "rings ripple outward", base: { pattern: "solid", hue: 0.08, sat: 0.8, brightness: 0.18 }, layers: [
      { group: "ring1", control: { pattern: "ripple", hue: 0.06, speed: 0.3, brightness: 0.7 } },
      { group: "ring2", control: { pattern: "ripple", hue: 0.11, speed: 0.3, brightness: 0.7, reverse: true } },
      { group: "ring3", control: { pattern: "ripple", hue: 0.16, speed: 0.3, brightness: 0.7 } },
    ] },
    { at: 80, note: "the canopy blooms green", base: { pattern: "bloom", hue: 0.3, sat: 0.8, brightness: 0.65, speed: 0.4, colorCycle: "group" } },
    { at: 120, note: "breathing canopy + crown", base: { pattern: "breathe", hue: 0.33, sat: 0.7, brightness: 0.7, speed: 0.4, colorCycle: "off" }, layers: [
      { group: "uplights", control: { pattern: "solid", hue: 0.33, sat: 0.7, brightness: 0.5 } },
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.6, speed: 0.3 } },
    ] },
    { at: 162, note: "counter-rotating rings", base: { pattern: "warmcool", brightness: 0.6, speed: 0.3 }, layers: [
      { group: "ring1", control: { pattern: "rings", hue: 0.3, speed: 0.4, brightness: 0.8 } },
      { group: "ring2", control: { pattern: "rings", hue: 0.45, speed: 0.4, reverse: true, brightness: 0.8 } },
      { group: "ring3", control: { pattern: "rings", hue: 0.55, speed: 0.4, brightness: 0.8 } },
    ] },
    { at: 210, note: "full warmcool drift", base: { pattern: "warmcool", hue: 0.4, sat: 0.7, brightness: 0.82, speed: 0.3, colorCycle: "group" } },
    { at: 252, note: "evening settles cool", base: { pattern: "ripple", hue: 0.6, sat: 0.7, brightness: 0.5, speed: 0.25, colorCycle: "off" } },
    { at: 286, note: "return to rest", base: { pattern: "breathe", hue: 0.62, sat: 0.7, brightness: 0.16, speed: 0.3 } },
  ],
};

// ── ⚡ IGNITION — rhythmic, explosive, beat-driven. Build → drop → chaos → climax ──
const IGNITION: LightShow = {
  id: "ignition", name: "⚡ Ignition", vibe: "energetic · fast · strobe + rainbow", durationS: 300,
  cues: [
    { at: 0, note: "tension — a single chase races", base: { pattern: "chase", hue: 0, sat: 1, brightness: 0.7, speed: 1.2, colorCycle: "off", order: "linear", reverse: false, strobe: false, master: 1 } },
    { at: 22, note: "opposing chases by ring", base: { pattern: "solid", brightness: 0.05 }, layers: [
      { group: "ring1", control: { pattern: "chase", hue: 0, sat: 1, speed: 1.3, brightness: 1 } },
      { group: "ring2", control: { pattern: "sweep", hue: 0.5, sat: 1, speed: 1.3, brightness: 1, reverse: true } },
      { group: "ring3", control: { pattern: "chase", hue: 0, sat: 1, speed: 1.3, brightness: 1, reverse: true } },
    ] },
    { at: 48, note: "tricolor orbit, fast", base: { pattern: "tricolor", sat: 1, brightness: 0.85, speed: 1.8 } },
    { at: 72, note: "THE DROP — golden strobe", base: { pattern: "solid", hue: 0.09, sat: 0.55, brightness: 1, strobe: true, speed: 2 } },
    { at: 80, note: "rainbow chase chaos", base: { pattern: "chase", brightness: 0.9, speed: 1.8, colorCycle: "rainbow", strobe: false } },
    { at: 105, note: "per-light random fire", base: { pattern: "sparkle", brightness: 0.92, speed: 1.6, colorCycle: "independent", order: "random" } },
    { at: 135, note: "sweeps flip directions", base: { pattern: "solid", brightness: 0.05 }, layers: [
      { group: "ring1", control: { pattern: "sweep", speed: 2, colorCycle: "rainbow", brightness: 1 } },
      { group: "ring2", control: { pattern: "sweep", speed: 2, reverse: true, colorCycle: "rainbow", brightness: 1 } },
      { group: "ring3", control: { pattern: "sweep", speed: 2, colorCycle: "rainbow", brightness: 1 } },
      { group: "chandelier", control: { pattern: "fibonacci", speed: 1.8, colorCycle: "rainbow", brightness: 1 } },
    ] },
    { at: 170, note: "counter-rings build", base: { pattern: "rings", sat: 1, brightness: 0.9, speed: 1.6, colorCycle: "rainbow" } },
    { at: 210, note: "CLIMAX — everything", base: { pattern: "tricolor", sat: 1, brightness: 1, speed: 2.2, strobe: false }, layers: [
      { group: "chandelier", control: { pattern: "sparkle", colorCycle: "independent", order: "random", speed: 2, brightness: 1 } },
    ] },
    { at: 255, note: "strobe peak", base: { pattern: "chase", brightness: 1, speed: 2.4, colorCycle: "rainbow", strobe: true } },
    { at: 290, note: "snap to a golden pulse", base: { pattern: "breathe", hue: 0.09, sat: 0.55, brightness: 1, speed: 2, strobe: false } },
  ],
};

// ── 🌌 COSMOS — hypnotic, spatial, geometric. Void → galaxies → starfield → collapse ──
const COSMOS: LightShow = {
  id: "cosmos", name: "🌌 Cosmos", vibe: "hypnotic · spatial · deep cool", durationS: 300,
  cues: [
    { at: 0, note: "the void — one star at the crown", base: { pattern: "solid", hue: 0.7, sat: 0.8, brightness: 0.02, colorCycle: "off", order: "linear", reverse: false, strobe: false, speed: 0.4, master: 1 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.5, speed: 0.3 } },
    ] },
    { at: 24, note: "spiral genesis", base: { pattern: "spiral", hue: 0.72, sat: 0.9, brightness: 0.5, speed: 0.4 } },
    { at: 55, note: "counter-rotating galaxies + godrays", base: { pattern: "godray", hue: 0.7, sat: 0.8, brightness: 0.4, speed: 0.5 }, layers: [
      { group: "ring1", control: { pattern: "rings", hue: 0.66, speed: 0.5, brightness: 0.85 } },
      { group: "ring2", control: { pattern: "rings", hue: 0.78, speed: 0.5, reverse: true, brightness: 0.85 } },
      { group: "ring3", control: { pattern: "rings", hue: 0.85, speed: 0.5, brightness: 0.85 } },
    ] },
    { at: 92, note: "nebula plasma", base: { pattern: "plasma", hue: 0.78, sat: 0.85, brightness: 0.6, speed: 0.4, colorCycle: "group" } },
    { at: 130, note: "chromatic streams from the crown", base: { pattern: "chromatic", sat: 0.9, brightness: 0.6, speed: 0.5, colorCycle: "off" }, layers: [
      { group: "chandelier", control: { pattern: "solid", hue: 0.06, sat: 0.6, brightness: 0.7 } },
    ] },
    { at: 168, note: "starfield twinkle", base: { pattern: "sparkle", hue: 0.7, sat: 0.9, brightness: 0.7, speed: 0.6, colorCycle: "independent", order: "random" } },
    { at: 205, note: "all spiral, cool rainbow", base: { pattern: "spiral", sat: 0.9, brightness: 0.8, speed: 0.7, colorCycle: "group", hue: 0.66 } },
    { at: 245, note: "convergence — one rotation", base: { pattern: "rings", sat: 0.9, brightness: 0.92, speed: 0.85, reverse: false, colorCycle: "off", hue: 0.75 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.7, speed: 0.5 } },
    ] },
    { at: 286, note: "collapse back to the star", base: { pattern: "solid", hue: 0.7, sat: 0.8, brightness: 0.03 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.6, speed: 0.3 } },
    ] },
  ],
};

// ── 🌳 PERFORMANCE — the Bellagio-structured 18-min living show ──────────────
// Five-act dramatic arc, climax at the golden section (~61.8% ≈ 11:08), partial-
// release ratcheting, one signature motif (the spiral) recurring transformed, a
// cool-night→warm-dawn→spectrum→warm colour journey, the decentralised "living"
// engine as its heartbeat. Slow + organic, NO strobe; resolves to rest, not black,
// so each hourly run is one breath. (Research: WET/Bellagio + Freytag + φ-climax.)
const PERFORMANCE: LightShow = {
  id: "performance", name: "🌳 Performance", vibe: "18-min living arc · Bellagio-structured", durationS: 1080,
  cues: [
    // Phase 0 — darkness / the held breath
    { at: 0, note: "night — first stars", base: { pattern: "sparkle", hue: 0.62, sat: 0.6, brightness: 0.16, colorCycle: "independent", order: "random", reverse: false, strobe: false, speed: 0.5, master: 1 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.5, speed: 0.3 } },
    ] },
    // Phase 1 — stars emerge (density accelerates; desynced twinkle)
    { at: 90, note: "stars begin to emerge", base: { pattern: "sparkle", hue: 0.6, sat: 0.6, brightness: 0.3, colorCycle: "independent", order: "random", speed: 0.55 } },
    { at: 185, note: "the sky fills in", base: { pattern: "living", hue: 0.58, sat: 0.7, brightness: 0.42, colorCycle: "off", speed: 0.6 } },
    // Phase 2 — first breath / the signature motif wakes
    { at: 270, note: "first breath — a slow spiral wakes", base: { pattern: "spiral", hue: 0.55, sat: 0.7, brightness: 0.33, colorCycle: "off", speed: 0.4 } },
    { at: 350, note: "a global colour breath toward dawn", base: { pattern: "spiral", hue: 0.12, sat: 0.78, brightness: 0.42, colorCycle: "off", speed: 0.4 } },
    // Phase 3 — rising action / nested swells, baseline ratchets up
    { at: 420, note: "the tree comes alive", base: { pattern: "living", hue: 0.1, sat: 0.85, brightness: 0.5, colorCycle: "off", speed: 0.5 } },
    { at: 485, note: "swell — ripples answer (call & response)", base: { pattern: "ripple", hue: 0.16, sat: 0.82, brightness: 0.62, colorCycle: "group", speed: 0.5 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.7, speed: 0.4 } },
    ] },
    { at: 545, note: "partial release — baseline higher than before", base: { pattern: "living", hue: 0.32, sat: 0.85, brightness: 0.54, colorCycle: "off", speed: 0.55 } },
    { at: 605, note: "rings converge on a moving focus", base: { pattern: "rings", hue: 0.5, sat: 0.9, brightness: 0.74, colorCycle: "group", speed: 0.6 }, layers: [
      { group: "ring1", control: { pattern: "rings", hue: 0.45, speed: 0.6, brightness: 0.88 } },
      { group: "ring2", control: { pattern: "rings", hue: 0.6, speed: 0.6, reverse: true, brightness: 0.88 } },
      { group: "ring3", control: { pattern: "rings", hue: 0.75, speed: 0.6, brightness: 0.88 } },
    ] },
    // Phase 4 — CLIMAX at φ (~668s): full spectrum bloom, everything converges
    { at: 668, note: "✦ CLIMAX — full-spectrum bloom", base: { pattern: "bloom", hue: 0.0, sat: 1, brightness: 0.97, colorCycle: "group", speed: 0.7 }, layers: [
      { group: "chandelier", control: { pattern: "solid", hue: 0.06, sat: 0.6, brightness: 0.7 } },
    ] },
    { at: 712, note: "the living organism at full bloom", base: { pattern: "living", hue: 0.55, sat: 1, brightness: 0.99, colorCycle: "off", speed: 0.7 } },
    // Phase 5 — falling action: the motif unwinds (inversion), spectrum recedes
    { at: 752, note: "falling — the spiral unwinds", base: { pattern: "spiral", hue: 0.1, sat: 0.85, brightness: 0.7, colorCycle: "off", speed: 0.5, reverse: true } },
    { at: 842, note: "the spectrum recedes warm", base: { pattern: "ripple", hue: 0.08, sat: 0.7, brightness: 0.45, colorCycle: "off", speed: 0.4, reverse: false } },
    // Phase 6 — denouement / rest: the exhale, loop back toward darkness
    { at: 875, note: "the exhale", base: { pattern: "living", hue: 0.08, sat: 0.7, brightness: 0.3, colorCycle: "off", speed: 0.35 } },
    { at: 965, note: "settling to embers", base: { pattern: "breathe", hue: 0.07, sat: 0.7, brightness: 0.16, speed: 0.3 } },
    { at: 1045, note: "a few stars remain", base: { pattern: "solid", hue: 0.6, sat: 0.7, brightness: 0.04, speed: 0.3 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.4, speed: 0.25 } },
    ] },
  ],
};

// ── 🌿 BIOLUMINESCENCE — emergent ambient: the tree's decentralised "living"
// engine drifting between firefly-sync, spreading ripples, and a reaction-diffusion
// organism. Each light decides from its neighbours; slow, no strobe; loops. ──
const BIOLUMINESCENCE: LightShow = {
  id: "bioluminescence", name: "🌿 Bioluminescence", vibe: "emergent · neighbours decide · 10 min", durationS: 600,
  cues: [
    { at: 0, note: "the colony stirs", base: { pattern: "living", hue: 0.62, sat: 0.85, brightness: 0.3, colorCycle: "off", order: "linear", reverse: false, strobe: false, speed: 0.5, master: 1 } },
    { at: 70, note: "ripples spread across neighbours", base: { pattern: "ripples", hue: 0.5, sat: 0.8, brightness: 0.46, speed: 0.6 } },
    { at: 150, note: "an organism drifts in", base: { pattern: "organism", hue: 0.45, sat: 0.85, brightness: 0.56, speed: 0.8 }, layers: [
      { group: "chandelier", control: { pattern: "breathe", hue: 0.06, sat: 0.6, brightness: 0.5, speed: 0.3 } },
    ] },
    { at: 230, note: "the field comes alive", base: { pattern: "living", hue: 0.33, sat: 0.85, brightness: 0.6, colorCycle: "group", speed: 0.6 } },
    { at: 320, note: "waves answer the colony", base: { pattern: "ripples", hue: 0.78, sat: 0.8, brightness: 0.62, speed: 0.7 } },
    { at: 400, note: "a bloom of light", base: { pattern: "living", hue: 0.1, sat: 0.9, brightness: 0.78, colorCycle: "group", speed: 0.7 } },
    { at: 470, note: "ripples recede", base: { pattern: "ripples", hue: 0.55, sat: 0.75, brightness: 0.5, speed: 0.5 } },
    { at: 540, note: "the colony settles", base: { pattern: "living", hue: 0.62, sat: 0.8, brightness: 0.3, colorCycle: "off", speed: 0.45 } },
  ],
};

// ── 🌠 AURORA — geometric/hypnotic: noise curtains, interference shimmer, standing
// waves (Chladni), and a wandering Lissajous orbiter. Slow, cool, meditative. ──
const AURORA: LightShow = {
  id: "aurora-show", name: "🌠 Aurora", vibe: "noise curtains · standing waves · 8 min", durationS: 480,
  cues: [
    { at: 0, note: "noise curtains drift up", base: { pattern: "aurora", hue: 0.45, sat: 0.85, brightness: 0.45, colorCycle: "off", order: "linear", reverse: false, strobe: false, speed: 0.5, master: 1 } },
    { at: 80, note: "interference shimmer", base: { pattern: "interference", hue: 0.55, sat: 0.8, brightness: 0.52, speed: 0.6 } },
    { at: 160, note: "a standing-wave mandala", base: { pattern: "chladni", hue: 0.7, sat: 0.85, brightness: 0.6, speed: 0.5, colorCycle: "group" } },
    { at: 240, note: "an orbiter wanders the canopy", base: { pattern: "lissajous", hue: 0.1, sat: 0.9, brightness: 0.7, speed: 0.6, colorCycle: "off" } },
    { at: 320, note: "aurora returns, fuller", base: { pattern: "aurora", hue: 0.35, sat: 0.9, brightness: 0.72, speed: 0.6, colorCycle: "group" } },
    { at: 410, note: "settle to curtains", base: { pattern: "aurora", hue: 0.55, sat: 0.8, brightness: 0.4, speed: 0.4, colorCycle: "off" } },
  ],
};

export const SHOWS: LightShow[] = [PERFORMANCE, BIOLUMINESCENCE, AURORA, AWAKENING, IGNITION, COSMOS];
export const showById = (id: string | null): LightShow | undefined => SHOWS.find((s) => s.id === id);
