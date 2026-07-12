import { create } from "zustand";
import { blenderToThree, type FixturesDoc } from "./fixtures";
import { runCommandStr, parseScript, type Override } from "./command";
import { makeCue, loadCues, saveCues, type Cue } from "./cues";
import type { Ripple } from "./interaction";
import { clearLife, seedLife, seedRandomCluster, exciteRipples, exciteOrganism, exciteField, themeMapHue, setFieldTheme, setLifeState, setLifeRules, getLifeRules, type LifeRules } from "./field";

// Game-of-Light swaps the life rules to organic while armed; the player's own
// rules are snapshotted here and restored on disarm.
let preGolRules: LifeRules | null = null;
import { themeById, themeHue } from "./themes";
import { loadFixtures, makeTestGridDoc } from "./fixtures";
import { recEvent } from "./flightrec";
import { DEFAULT_SENSORS, type Sensors } from "./sensors";

export type PatternId =
  | "solid" | "breathe" | "chase" | "ripple" | "sparkle" | "sequence" | "spectrum" | "tricolor"
  | "spiral" | "godray" | "rising" | "planewipe" | "warmcool" | "bloom" | "firefly" | "ca" | "hero" | "plasma"
  | "chromatic" | "rings" | "fibonacci" | "sweep" | "living" | "piano" | "ripples" | "organism"
  | "aurora" | "chladni" | "glyph" | "interference" | "lissajous" | "shockwave" | "hurricane"
  | "life"
  | "wind" | "ember" | "rain" | "beacon";
export const PATTERN_IDS: PatternId[] = [
  "solid", "breathe", "chase", "ripple", "sparkle", "sequence", "spectrum", "tricolor",
  "spiral", "godray", "rising", "planewipe", "warmcool", "bloom", "firefly", "ca", "hero", "plasma", "chromatic", "rings", "fibonacci", "sweep", "living", "piano", "ripples", "organism", "life", "aurora", "chladni", "glyph", "interference", "lissajous", "shockwave", "hurricane",
];
/** Decentralised cellular-automata rules (Ben's BACKGROUND.md mesh spec): each light
 *  runs a simple local rule over its pre-baked neighbour list. The "interactivity mode"
 *  vocabulary — the tree lives on its own + reacts to presence pokes. */
export const CA_RULES: PatternId[] = ["life", "ripples", "organism", "living"];
/** Element / environmental modes (dossier PART 7). */
export const ELEMENT_MODES: PatternId[] = ["wind", "ember", "rain", "beacon"];

export type SeqMode = "fill" | "single" | "snake" | "groups" | "everyN" | "allOn" | "allOff";
export const SEQ_MODES: SeqMode[] = ["fill", "single", "snake", "groups", "everyN", "allOn", "allOff"];

export type VizMode = "lanterns" | "orbs" | "wire";
export const VIZ_MODES: VizMode[] = ["lanterns", "orbs", "wire"];

/** Base-hue MOTION on top of the picked colour (works with any pattern):
 *  off    — hold the picked colour
 *  rainbow— sweep continuously through ALL colours
 *  group  — drift through the adjacent FAMILY band (warm red/orange/yellow,
 *           cool blue/purple/pink) around the picked colour
 *  shade  — drift through the SHADES of just the picked colour (all the reds…) */
export type ColorCycle = "off" | "rainbow" | "group" | "shade" | "independent";
export const COLOR_CYCLES: ColorCycle[] = ["off", "rainbow", "group", "shade", "independent"];

/** Group "Mode" (Elliot's sketch): linear = pattern runs in spatial order;
 *  random = each light fires in a shuffled order (seeded per fixture). */
export type LightOrder = "linear" | "random";
export const LIGHT_ORDERS: LightOrder[] = ["linear", "random"];

/** Default look for a freshly-activated group (the sketch's per-group controls). */
export const DEFAULT_GROUP_CONTROL: Partial<Control> = {
  pattern: "chase", hue: 0.6, sat: 1, colorCycle: "off", reverse: false, speed: 1, brightness: 1, order: "linear",
};

/** A scene LAYER: a subset of light numbers (1..72) running its OWN control
 *  (pattern / colour / direction / speed) on top of the base. This is the engine
 *  behind per-group independent looks — the group panel and LLM-driven grouping
 *  both just push layers. A fixture matched by a later layer wins (last-write). */
export interface SceneLayer {
  id: string;
  nums: number[]; // light numbers 1..72 this layer drives
  control: Partial<Control>;
}

/** TRIGGER RESPONSE (interactivity rules editor): what a "sensor firing at a spot"
 *  does. A touch/click on the tree fires a sensor at the nearest fixture; this rule
 *  says what happens — which CA runs, the reaction colour, how bright, how far it
 *  spreads across neighbours. Colour can be fixed, random per touch, or cycling. */
export type TriggerColorMode = "fixed" | "random" | "cycle";
export const TRIGGER_COLOR_MODES: TriggerColorMode[] = ["fixed", "random", "cycle"];
export interface TriggerRule {
  rule: PatternId; // CA the tree runs / a touch kicks
  hue: number; // reaction colour 0..1 (used when colorMode = "fixed")
  intensity: number; // 0..2.5 brightness of the reaction (used when briRange off)
  spread: number; // 0.3..2 how far/fast the disturbance rolls (radius/speed + Life hops)
  duration: number; // seconds the triggered lights stay ON before fading (Life: cell ttl)
  colorMode: TriggerColorMode;
  // CONSTRAINTS (Elliot): "always a random colour but NEVER the same as the last;
  // always a different brightness, but within a certain range"
  noRepeatColor: boolean; // successive reactions must differ by ≥ MIN_HUE_DIST on the wheel
  briRange: boolean; // randomize each reaction's brightness within [briLo, briHi]
  briLo: number;
  briHi: number;
}
export const DEFAULT_TRIGGER_RULE: TriggerRule = {
  rule: "life", hue: 0.05, intensity: 1.6, spread: 1, duration: 4, colorMode: "cycle",
  noRepeatColor: true, briRange: false, briLo: 0.8, briHi: 1.8,
};
/** Minimum hue-wheel distance between successive reactions when noRepeatColor is on. */
export const MIN_HUE_DIST = 0.15;

/** TOP-LEVEL OPERATOR MODES (Elliot's taxonomy). Pick the mode FIRST; the side panel
 *  shows only that mode's controls:
 *   interactive — the tree is REACTIVE (presence sensors); you only set the rules
 *   lightshow   — you drive it: scope (whole tree → group → single light), sliders,
 *                 patterns, pre-designed shows or build-your-own with music
 *   sound       — the tree is reactive to SOUND (DJ / AI-VJ / audio-reactive)
 *   calibrate   — commissioning, photogrammetry, testing, health */
export type UiMode = "interactive" | "lightshow" | "sound" | "calibrate";
export const UI_MODES: UiMode[] = ["interactive", "lightshow", "sound", "calibrate"];

/** GAME OF LIGHT lifecycle. The tree senses its first visitor → ignites (all off → a
 *  quick flourish → off) → goes LIVE. In live mode the tree is dark at rest and each
 *  visitor who activates a sensor drops a persistent NODE (a live Game-of-Life source
 *  in its quadrant's colour). When nodes ring the whole tree → UNITY (community mode). */
export type GolPhase = "off" | "standby" | "off1" | "flash" | "off2" | "live";
export const GOL_PHASES: GolPhase[] = ["off", "standby", "off1", "flash", "off2", "live"];
/** 4 quadrant colours around the trunk — visitors light their side in these hues. */
export const QUADRANT_HUES = [0.02, 0.13, 0.55, 0.83]; // red · amber · teal · magenta
export interface GolState {
  phase: GolPhase;
  t0: number; // performance.now()/1000 at the current phase start
  ambient: boolean; // true = always-alive field; false (live default) = dark at rest, react to visitors
  nodes: number[]; // persistent visitor-node fixture indices
  unity: boolean; // community/Unity effect active
  unityT0: number;
}
export const DEFAULT_GOL: GolState = { phase: "off", t0: 0, ambient: true, nodes: [], unity: false, unityT0: 0 };

/** MODE-ENTRY ceremony (Elliot): picking an interactive rule first takes the tree
 *  DARK (blank field), runs a short themed flourish so folks nearby KNOW the tree
 *  is entering this mode, goes dark again, then the chosen automaton starts from
 *  an empty board. Driven by IgnitionDriver, same as Game-of-Light ignition. */
export type AnnouncePhase = "idle" | "dark" | "flourish" | "settle";
export interface AnnounceState { phase: AnnouncePhase; target: PatternId; t0: number }
export const DEFAULT_ANNOUNCE: AnnounceState = { phase: "idle", target: "life", t0: 0 };

export interface SimFixture {
  id: string;
  name: string;
  role: string;
  zone: string;
  pos: [number, number, number]; // three-space (Y-up)
  norm: [number, number, number]; // normalized 0..1 within the fixture bbox
  seqT: number; // 0..1 order AROUND the tree (by azimuth) — for chases/snakes
  seq: number; // integer rank 0..N-1 around the tree — for the sequencer
  num: number; // addressable light number 1..72 (each light individually addressable)
  heightT: number; // 0..1 by height (low→high)
  ring: number; // concentric ring index 0=inner 1=mid 2=outer (by radial distance from trunk)
  quadrant: number; // 0..3 azimuth quadrant around the trunk (Game-of-Light regions)
  azimuth: number; // -PI..PI angle around the trunk (for Unity ring detection)
  radialT: number; // 0..1 normalized horizontal distance from the trunk axis (in/out)
  rnd: number; // stable per-fixture random 0..1 — for sparkle/jitter
  neighbors: number[]; // indices of nearest fixtures — decentralised/neighbour-coupled patterns
  beamDeg: number; // beam cone angle (deg) from fixtures.json
  lumens: number; // lumens_max from fixtures.json (beam photometrics)
  aim?: [number, number, number]; // three-space cast direction (schema 0.2; optional)
}

export interface Control {
  pattern: PatternId;
  brightness: number; // 0..1 master
  hue: number; // 0..1
  sat: number; // 0..1
  colorCycle: ColorCycle; // base-hue motion (off/rainbow/group/shade), any pattern
  order: LightOrder; // group "Mode": linear (spatial) vs random firing order
  speed: number; // 0..3
  // sequencer (H1)
  seqMode: SeqMode;
  stepMs: number; // step delay (Elliot's 0.2s default)
  groupSize: number; // 24 / 36 / 72 / ...
  everyN: number; // 2 / 4 / ...
  syncToBeat: boolean; // snap sequencer step to detected BPM
  beatDiv: number; // 1=quarter, 2=eighth
  visualizer: VizMode; // render style (A7)
  // DJ controller (C)
  xfade: number; // 0=look A, 1=look B
  djPatternB: PatternId; // look B pattern
  djHueB: number; // look B hue
  eqLow: number; // bass→low-zone gain 0..1
  eqMid: number; // mid→mid-zone gain
  eqHigh: number; // treble→high-zone gain
  master: number; // final intensity 0..1
  strobe: boolean;
  strobeHz: number;
  // auto-VJ (D)
  autoVj: boolean;
  autoBars: number; // phrase length in bars between look changes
  aiPilot: boolean; // AI-VJ: audio-digest → auto-pick looks (smart sound→light)
  beaconPreempt: boolean; // safety preempt: force full white over everything
  blackout: boolean; // safety preempt: force all-off (wins over beacon)
  reverse: boolean; // jog-wheel direction: reverse the around-the-tree motion
  audioSpeed: boolean; // auto-drive motion speed from the music (energy/BPM/drop)
  autoBalance: boolean; // sense ambient daylight → auto-boost drive to stay readable
  glslMode: boolean; // opt-in: drive fixtures from the GPU GLSL pattern pass (vs CPU litFor)
  glslPattern: string; // active GLSL pattern id (glslRuntime registry)
}

interface TwinState {
  fixtures: SimFixture[];
  source: string;
  control: Control;
  center: [number, number, number];
  size: number;
  overrides: Record<number, Override>; // per-fixture command overrides (H3)
  cmdLog: string[];
  // truth-loop (G1/F3): mock heartbeat transport + monitor view
  view: { mock: boolean; monitor: boolean; deadCount: number };
  monitorStats: { reporting: number; dead: number; stale: number };
  net: { channel: number; driveReal: boolean }; // ESP-NOW control-plane (E/I8)
  cues: Cue[]; // saved looks (F1)
  timeline: { playing: boolean; stepSecs: number }; // cue timeline (F2)
  ripples: Ripple[]; // presence→ripple interactions
  triggerRule: TriggerRule; // interactivity rules editor: what a sensor-firing does
  caTheme: string; // interactive-mode colour THEME (themes.ts) — "" = free palette
  gol: GolState; // Game-of-Light lifecycle (ignition · nodes · quadrants · Unity)
  announce: AnnounceState; // CA mode-entry ceremony (dark → flourish → blank start)
  uiMode: UiMode; // which operator mode the side panel shows (persisted)
  dock: boolean; // split-screen dock layout (tree left · one organized panel right)
  // GROUPS ABOVE MODES (Elliot): each group can run its OWN mode simultaneously —
  // canopy interactive while the chandelier follows sound. "follow" = ride the base.
  groupModes: Record<string, UiMode | "follow">;
  selectedScope: string; // which group the panel is dialling in ("all" = whole tree)
  guest: boolean; // guest-DJ scoped mode (C3)
  sensors: Sensors; // environmental inputs (crowd/motion/temp/wind/daylight)
  cameraPreset: "hero" | "top"; // hero 3/4 vs top-down projection view
  cinematic: boolean; // hide all UI panels for a clean show/beauty view
  timeOfDay: number; // 0 = night, 0.5 = dusk, 1 = day (scene ambient/background)
  layers: SceneLayer[]; // per-group/subset looks composed over the base control
  // GROUPS (Elliot's panel): named light-number sets + each group's saved look + which are live
  namedGroups: Record<string, number[]>; // group name → light numbers (presets + custom)
  groupControls: Record<string, Partial<Control>>; // group name → its Pattern/Color/Direction/Speed/Mode
  groupActive: Record<string, boolean>; // which groups are currently driving their layer
  selectedGroup: string; // the group the panel is editing
  activeShow: string | null; // running timed light show (shows.ts), or null
  showStartedAt: number; // performance.now()/1000 when the show began (for elapsed)
  showSeed: number; // per-RUN seed → each playthrough varies (hue rotation, speed, cue jitter); reset on start + loop
  init: (doc: FixturesDoc) => void;
  loadLayout: (which: "tree" | "grid", seed?: number) => void; // testing rig ⇄ the real tree
  set: (p: Partial<Control>) => void;
  runCommand: (cmd: string) => void;
  runScript: (text: string) => void;
  setView: (p: Partial<{ mock: boolean; monitor: boolean; deadCount: number }>) => void;
  setMonitorStats: (s: { reporting: number; dead: number; stale: number }) => void;
  setNet: (p: Partial<{ channel: number; driveReal: boolean }>) => void;
  addCue: (name: string) => void;
  recallCue: (id: string) => void;
  deleteCue: (id: string) => void;
  setTimeline: (p: Partial<{ playing: boolean; stepSecs: number }>) => void;
  pingPresence: (origin?: [number, number, number]) => void;
  triggerAt: (idx: number, origin?: [number, number, number]) => void; // fire a sensor at a fixture
  setTriggerRule: (p: Partial<TriggerRule>) => void;
  setCaTheme: (id: string) => void; // pick a colour theme for interactive mode
  armGol: () => void; // enter standby: dark, waiting for the first visitor
  golSetPhase: (p: GolPhase) => void; // ignition state machine (driven by IgnitionDriver)
  enterCa: (r: PatternId) => void; // pick a CA rule WITH the entry ceremony (dark → announce → blank)
  announceSetPhase: (p: AnnouncePhase) => void; // ceremony state machine (IgnitionDriver)
  golFirstVisitor: (idx: number) => void; // first visitor sensed → begin ignition
  addNode: (idx: number) => void; // a visitor activates a sensor → persistent node
  clearNodes: () => void;
  setGolAmbient: (b: boolean) => void;
  setUnity: (on: boolean) => void;
  setUiMode: (m: UiMode) => void;
  setDock: (b: boolean) => void;
  setGroupMode: (group: string, m: UiMode | "follow") => void; // per-group mode routing
  resetAllOff: () => void; // BLACKOUT: stop every mode/show/group and go dark (a reset, not a hold)
  setSelectedScope: (g: string) => void;
  /** Auto-calibration solo-light: ALL fixtures off; `test` lights exactly one in
   *  the given colour. null = end of sequence, clear every override. */
  calSolo: (test: { idx: number; rgb: [number, number, number] } | null) => void;
  setGuest: (b: boolean) => void;
  setSensors: (p: Partial<Sensors>) => void;
  setCameraPreset: (c: "hero" | "top") => void;
  setCinematic: (b: boolean) => void;
  setTimeOfDay: (t: number) => void;
  setLayer: (id: string, nums: number[], control: Partial<Control>) => void;
  removeLayer: (id: string) => void;
  clearLayers: () => void;
  defineGroup: (name: string, nums: number[]) => void;
  deleteGroup: (name: string) => void;
  selectGroup: (name: string) => void;
  setGroupControl: (name: string, partial: Partial<Control>) => void;
  groupThemes: Record<string, string>; // per-group colour theme id ("" / absent = follow global)
  setGroupTheme: (name: string, id: string) => void;
  toggleGroupActive: (name: string, on: boolean) => void;
  playShow: (id: string | null) => void;
}

// last trigger-reaction memory for the "never the same as the last" constraints
let lastTriggerHue = -1;
let lastTriggerBri = -1;

export const useTwin = create<TwinState>((setState, get) => ({
  fixtures: [],
  source: "",
  center: [0, 0, 0],
  size: 10,
  overrides: {},
  layers: [],
  groupThemes: {},
  namedGroups: {},
  groupControls: {},
  groupActive: {},
  selectedGroup: "ring1",
  activeShow: null,
  showStartedAt: 0,
  showSeed: 0,
  cmdLog: [],
  view: { mock: false, monitor: false, deadCount: 6 },
  monitorStats: { reporting: 0, dead: 0, stale: 0 },
  net: { channel: 11, driveReal: false },
  cues: loadCues(),
  timeline: { playing: false, stepSecs: 8 },
  ripples: [],
  triggerRule: DEFAULT_TRIGGER_RULE,
  caTheme: (typeof localStorage !== "undefined" && localStorage.getItem("ca.theme")) || "ember",
  gol: DEFAULT_GOL,
  announce: DEFAULT_ANNOUNCE,
  uiMode: (typeof localStorage !== "undefined" && (localStorage.getItem("ui.mode") as UiMode)) || "lightshow",
  dock: typeof localStorage !== "undefined" ? localStorage.getItem("ui.dock") !== "0" : true,
  groupModes: {},
  selectedScope: "all",
  guest: false,
  sensors: DEFAULT_SENSORS,
  cameraPreset: "hero",
  cinematic: false,
  // ?tod=<0..1> deep-links a time of day (0 night … 1 day) — handy for previewing
  // the install in daylight (structure + fixture bodies read against a lit sky).
  timeOfDay: (() => {
    if (typeof location === "undefined") return 0;
    const t = new URLSearchParams(location.search).get("tod");
    return t == null ? 0 : Math.max(0, Math.min(1, parseFloat(t) || 0));
  })(),
  control: {
    pattern: "sequence",
    brightness: 0.9,
    hue: 0.08,
    sat: 0.85,
    colorCycle: "off",
    order: "linear",
    speed: 1,
    seqMode: "fill",
    stepMs: 200,
    groupSize: 24,
    everyN: 2,
    syncToBeat: false,
    beatDiv: 1,
    visualizer: "lanterns",
    xfade: 0,
    djPatternB: "ripple",
    djHueB: 0.6,
    eqLow: 0,
    eqMid: 0,
    eqHigh: 0,
    master: 1,
    strobe: false,
    strobeHz: 10,
    autoVj: false,
    autoBars: 8,
    aiPilot: false,
    beaconPreempt: false,
    blackout: false,
    reverse: false,
    audioSpeed: false,
    autoBalance: true, // on by default: boosts only as daylight rises, night unchanged
    glslMode: typeof location !== "undefined" && new URLSearchParams(location.search).has("glsl"),
    glslPattern: (typeof location !== "undefined" && new URLSearchParams(location.search).get("glslp")) || "radialPulse",
  },
  loadLayout: (which, seed = 1) => {
    if (which === "grid") { get().init(makeTestGridDoc(seed)); return; }
    loadFixtures().then((doc) => get().init(doc)).catch(() => { /* keep current */ });
  },
  init: (doc) => {
    const raw = doc.fixtures.map((f) => blenderToThree(f.position));
    const min: [number, number, number] = [Infinity, Infinity, Infinity];
    const max: [number, number, number] = [-Infinity, -Infinity, -Infinity];
    for (const p of raw)
      for (let i = 0; i < 3; i++) {
        min[i] = Math.min(min[i], p[i]);
        max[i] = Math.max(max[i], p[i]);
      }
    const span: [number, number, number] = [max[0] - min[0], max[1] - min[1], max[2] - min[2]];
    const size = Math.max(span[0], span[1], span[2]) || 10;
    const center: [number, number, number] = [
      (min[0] + max[0]) / 2,
      (min[1] + max[1]) / 2,
      (min[2] + max[2]) / 2,
    ];
    // azimuth order around the trunk (Y axis) → seqT, so a chase travels around the tree
    const angle = (p: [number, number, number]) =>
      Math.atan2(p[2] - center[2], p[0] - center[0]); // -PI..PI
    const order = raw
      .map((p, i) => ({ i, a: angle(p) }))
      .sort((u, v) => u.a - v.a)
      .map((o) => o.i);
    const rankOf = new Array<number>(raw.length);
    order.forEach((idx, rank) => (rankOf[idx] = rank));
    const denom = Math.max(1, raw.length - 1);
    // concentric RINGS: horizontal distance from the trunk axis → ring index by
    // value-tertiles (inner/mid/outer), + a normalized radial 0..1 (in/out).
    const radiusOf = (p: [number, number, number]) => Math.hypot(p[0] - center[0], p[2] - center[2]);
    const radii = raw.map(radiusOf);
    const maxR = Math.max(...radii, 1e-3);
    const sortedR = [...radii].sort((a, b) => a - b);
    const t1 = sortedR[Math.floor(radii.length / 3)] ?? maxR / 3;
    const t2 = sortedR[Math.floor((2 * radii.length) / 3)] ?? (2 * maxR) / 3;
    const ringOf = (r: number) => (r <= t1 ? 0 : r <= t2 ? 1 : 2);
    // deterministic per-fixture pseudo-random
    const rndOf = (i: number) => {
      const x = Math.sin(i * 127.1 + 311.7) * 43758.5453;
      return x - Math.floor(x);
    };

    const fixtures: SimFixture[] = doc.fixtures.map((f, i) => {
      const p = raw[i];
      const norm: [number, number, number] = [
        span[0] ? (p[0] - min[0]) / span[0] : 0.5,
        span[1] ? (p[1] - min[1]) / span[1] : 0.5,
        span[2] ? (p[2] - min[2]) / span[2] : 0.5,
      ];
      return {
        id: f.fixture_id,
        name: f.name,
        role: f.role,
        zone: f.zone,
        pos: p,
        norm,
        seqT: rankOf[i] / denom,
        seq: rankOf[i],
        num: rankOf[i] + 1, // unique addressable number 1..N for EVERY light (by azimuth)
        heightT: norm[1],
        ring: ringOf(radii[i]),
        quadrant: (Math.floor(((angle(p) + Math.PI) / (2 * Math.PI)) * 4) % 4 + 4) % 4,
        azimuth: angle(p),
        radialT: radii[i] / maxR,
        rnd: rndOf(i),
        neighbors: [], // filled below once all positions exist
        beamDeg: f.beam_deg ?? 120,
        lumens: f.lumens_max ?? 450,
        // schema 0.2: real per-fixture aim (Blender Z-up) → three-space direction
        aim: f.aim ? blenderToThree(f.aim) : undefined,
      };
    });
    // k-nearest neighbours per fixture (3D) — the substrate for decentralised
    // "living" patterns where each light decides from what its neighbours do.
    // Bake 12 (distance-sorted); the RULES use the first `neighbourK` of them,
    // so the operator can widen/narrow the neighbourhood to fit the geometry.
    const KN = 12;
    for (let i = 0; i < fixtures.length; i++) {
      const p = fixtures[i].pos;
      const d = fixtures.map((g, j) => ({ j, d2: (g.pos[0] - p[0]) ** 2 + (g.pos[1] - p[1]) ** 2 + (g.pos[2] - p[2]) ** 2 }));
      d.sort((a, b) => a.d2 - b.d2);
      fixtures[i].neighbors = d.slice(1, KN + 1).map((x) => x.j);
    }

    // seed preset GROUPS (Elliot's panel): 3 concentric rings of downlights split by
    // radius + uplights + chandelier + all. Real ring IDs arrive with the Blender
    // export; until then split downlights into radial thirds of ~equal count.
    const downs = fixtures.filter((f) => f.role === "downlight").sort((a, b) => a.radialT - b.radialT);
    const third = Math.ceil(downs.length / 3) || 1;
    const numsOf = (arr: SimFixture[]) => arr.map((f) => f.num).sort((a, b) => a - b);
    const namedGroups: Record<string, number[]> = {
      ring1: numsOf(downs.slice(0, third)),
      ring2: numsOf(downs.slice(third, 2 * third)),
      ring3: numsOf(downs.slice(2 * third)),
      uplights: numsOf(fixtures.filter((f) => f.role === "uplight")),
      chandelier: numsOf(fixtures.filter((f) => f.role === "chandelier")),
      all: numsOf(fixtures),
    };
    setState({ fixtures, center, size, namedGroups, source: doc.meta.source.split(":")[1] ?? doc.meta.source });
  },
  set: (p) => setState((s) => ({
    // picking any pattern/mode RELEASES a latched blackout (Elliot: blackout is
    // "all off", not a hold — clicking a mode must visibly run it)
    control: { ...s.control, ...p, ...(p.pattern != null && p.blackout === undefined && s.control.blackout ? { blackout: false } : null) },
  })),
  runCommand: (cmd) => {
    const r = runCommandStr(cmd, get().fixtures);
    setState((s) => {
      const next: Partial<TwinState> = {};
      if (r.control) next.control = { ...s.control, ...r.control };
      if (r.clear) next.overrides = {};
      if (r.setOverrides) {
        const o = { ...s.overrides };
        for (const i of r.setOverrides.idx) o[i] = r.setOverrides.op;
        next.overrides = o;
      }
      next.cmdLog = [r.msg, ...s.cmdLog].filter(Boolean).slice(0, 6);
      return next;
    });
  },
  runScript: (text) => {
    for (const cmd of parseScript(text)) get().runCommand(cmd);
  },
  setView: (p) => setState((s) => ({ view: { ...s.view, ...p } })),
  setMonitorStats: (s) => setState({ monitorStats: s }),
  setNet: (p) => setState((s) => ({ net: { ...s.net, ...p } })),
  addCue: (name) =>
    setState((s) => {
      const cues = [...s.cues, makeCue(name, s.control)];
      saveCues(cues);
      return { cues };
    }),
  recallCue: (id) =>
    setState((s) => {
      const c = s.cues.find((x) => x.id === id);
      return c ? { control: { ...s.control, ...c.control } } : {};
    }),
  deleteCue: (id) =>
    setState((s) => {
      const cues = s.cues.filter((x) => x.id !== id);
      saveCues(cues);
      return { cues };
    }),
  setTimeline: (p) => setState((s) => ({ timeline: { ...s.timeline, ...p } })),
  pingPresence: (origin) =>
    setState((s) => {
      // pick the fixture at/nearest the ping (or a random one for the ambient motion sim)
      let idx = -1;
      if (origin && s.fixtures.length) {
        let bd = Infinity;
        for (let i = 0; i < s.fixtures.length; i++) {
          const p = s.fixtures[i].pos;
          const d = (p[0] - origin[0]) ** 2 + (p[1] - origin[1]) ** 2 + (p[2] - origin[2]) ** 2;
          if (d < bd) { bd = d; idx = i; }
        }
      } else if (s.fixtures.length) {
        // ambient motion sim: a passer-by is a person ON THE GROUND — in Game-of-Light
        // live mode pick a random OUTER DOWNLIGHT (canopy edge), never a crown fixture
        if (s.gol.phase === "live") {
          const walkable = s.fixtures.map((f, i) => ({ f, i })).filter((x) => x.f.role === "downlight" && x.f.radialT >= 0.4);
          if (walkable.length) idx = walkable[Math.floor(Math.random() * walkable.length)].i;
        }
        if (idx < 0) idx = Math.floor(Math.random() * s.fixtures.length);
      }
      const o = idx >= 0 ? s.fixtures[idx].pos : (origin ?? [0, 0, 0]);
      const t0 = performance.now() / 1000;
      const ripples = [...s.ripples.filter((r) => t0 - r.t0 < 3), { x: o[0], y: o[1], z: o[2], t0 }].slice(-16);
      // in Game of Life, a presence ping also seeds a small living blob there —
      // EXCEPT during Unity, whose celebration pings are ripple-only (seeding every
      // 0.33s for 10s would leave the field over-grown after the celebration ends)
      if (idx >= 0 && s.control.pattern === "life" && !s.gol.unity) seedLife([idx], { hops: 2 });
      // a passer-by also WAKES the free-running fields at that spot (interactive rest)
      if (idx >= 0) {
        if (s.control.pattern === "ripples") exciteRipples([idx]);
        if (s.control.pattern === "organism") exciteOrganism([idx, ...(s.fixtures[idx].neighbors ?? [])]);
        if (s.control.pattern === "living") exciteField([idx, ...(s.fixtures[idx].neighbors ?? [])]);
      }
      return { ripples };
    }),
  // fire a SENSOR at fixture `idx` (a touch/click on the tree): push a colour+
  // intensity wavefront per the trigger rule, and — if the active CA is Game of
  // Life — birth cells there so the disturbance propagates across neighbour hops.
  // Many simultaneous touches just append; the wave/seed engines handle overlap.
  triggerAt: (idx, origin) => setState((s) => {
    const f = s.fixtures[idx];
    if (!f) return {};
    recEvent("trigger", { idx, num: f.num, rule: s.triggerRule.rule });
    const o = origin ?? f.pos;
    const tr = s.triggerRule;
    const theme = themeById(s.caTheme);
    const themed = s.caTheme !== "random" && theme.hues.length > 0;
    let hue = tr.hue;
    if (tr.colorMode === "random") {
      const r = ((idx * 0.147 + performance.now() * 0.00013) % 1 + 1) % 1;
      // themed reactions draw from the THEME's anchors (no-repeat picks a different one)
      hue = themed ? themeHue(theme, r, tr.noRepeatColor ? lastTriggerHue : -1) : r;
    } else if (tr.colorMode === "cycle") {
      const r = ((s.ripples.length * 0.11 + performance.now() * 0.00003) % 1 + 1) % 1;
      hue = themed ? themeHue(theme, r, tr.noRepeatColor ? lastTriggerHue : -1) : r;
    }
    // CONSTRAINT: never the same colour as the last reaction. Themed draws already
    // avoided the last anchor; free draws walk the wheel by the golden ratio.
    if (!themed && tr.noRepeatColor && tr.colorMode !== "fixed" && lastTriggerHue >= 0) {
      let guard = 0;
      const dist = (a: number, b: number) => { let d = Math.abs(a - b) % 1; return Math.min(d, 1 - d); };
      while (dist(hue, lastTriggerHue) < MIN_HUE_DIST && guard++ < 8) hue = (hue + 0.381966) % 1;
    }
    lastTriggerHue = hue;
    // CONSTRAINT: a different brightness each time, within the configured range
    let intensity = tr.intensity;
    if (tr.briRange) {
      const lo = Math.min(tr.briLo, tr.briHi), hi = Math.max(tr.briLo, tr.briHi);
      intensity = lo + Math.random() * (hi - lo);
      if (Math.abs(intensity - lastTriggerBri) < (hi - lo) * 0.2) intensity = lo + (hi - lo) - (intensity - lo); // reflect → different from last
      lastTriggerBri = intensity;
    }
    const t0 = performance.now() / 1000;
    const ripples = [
      ...s.ripples.filter((r) => t0 - r.t0 < 3),
      { x: o[0], y: o[1], z: o[2], t0, hue, intensity, spread: tr.spread },
    ].slice(-16); // keep more for multi-touch
    // Game of Life: seed live cells at the sensor, TAGGED with the rule's colour /
    // brightness / time-on; spread grows the birth blob. The field carries it onward.
    if (s.control.pattern === "life") {
      seedLife([idx], { hops: Math.max(1, Math.round(tr.spread * 2)), hue, bri: intensity, ttl: tr.duration });
    }
    // every CA answers a touch with a VISIBLE local response that its own
    // dynamics then carry onward (Elliot: Excitable/RD/Firefly must be
    // interactive, not autonomous shows) — a 1-hop blob reads clearly
    const blob = [idx, ...(f.neighbors ?? [])];
    if (s.control.pattern === "ripples") exciteRipples(blob);
    if (s.control.pattern === "organism") exciteOrganism(blob);
    if (s.control.pattern === "living") exciteField(blob);
    return { ripples };
  }),
  setTriggerRule: (p) => setState((s) => ({ triggerRule: { ...s.triggerRule, ...p } })),
  // COLOUR THEME (interactive mode): the field's births/drift and every touch
  // reaction stay inside the theme's colour world. "ember"/"random" map to the
  // engine's built-ins; everything else feeds its hue anchors as a theme.
  setCaTheme: (id) => {
    recEvent("theme", { id });
    try { localStorage.setItem("ca.theme", id); } catch { /* fine */ }
    const t = themeById(id);
    if (id === "random") setLifeState({ palette: "random" });
    else if (id === "ember") setLifeState({ palette: "warm" });
    else setLifeState({ palette: "theme", themeHues: t.hues });
    // the OTHER engines (living / organism / ripples) pull their hues into the
    // theme's world through this map — one theme, every mode
    setFieldTheme(id === "random" ? null : t.hues);
    setState((s) => ({ caTheme: id, control: { ...s.control, sat: t.sat, hue: t.hues[0] ?? s.control.hue } }));
  },
  // ── CA MODE-ENTRY ceremony: dark → themed flourish → dark → fresh start
  //    (Game of Life lands on a 4-9-light seed cluster, not a blank board) ──
  enterCa: (r) => {
    recEvent("rule", { rule: r });
    const s = get();
    const style = {
      pattern: r as PatternId, colorCycle: "off" as const, order: "linear" as const, reverse: false,
      strobe: false, beaconPreempt: false, master: 1, brightness: 0.95,
    };
    // Game of Light armed = it owns the lifecycle; announcing would fight it.
    // Mid-ceremony re-picks just retarget the ceremony's landing rule.
    if (s.gol.phase !== "off") { s.set({ ...style, blackout: false }); return; }
    if (s.announce.phase !== "idle") { setState((st) => ({ announce: { ...st.announce, target: r } })); return; }
    clearLife(); // the automaton must start from an EMPTY board
    setState((st) => ({
      control: { ...st.control, ...style, pattern: "life", blackout: true }, // life renders the flourish
      announce: { phase: "dark", target: r, t0: performance.now() / 1000 },
    }));
  },
  announceSetPhase: (p) => setState((s) => {
    if (p === "idle") {
      // ceremony over → land on the chosen rule: field wiped, and for Game of
      // Life a fresh 4-9-light cluster is dealt immediately (Elliot: no blank start)
      clearLife();
      if (s.announce.target === "life") seedRandomCluster(s.fixtures);
      return {
        announce: { ...s.announce, phase: "idle", t0: performance.now() / 1000 },
        control: { ...s.control, pattern: s.announce.target, blackout: false },
      };
    }
    return {
      announce: { ...s.announce, phase: p, t0: performance.now() / 1000 },
      control: { ...s.control, blackout: p !== "flourish" },
    };
  }),

  // ── GAME OF LIGHT lifecycle ──
  armGol: () => {
    recEvent("arm", { on: true });
    // the visitor lifecycle NEEDS burn-out physics (waves must die back to dark),
    // which pure mode exempts by spec — run armed sessions on the organic rules
    // and hand the player's own rules back on disarm
    preGolRules = getLifeRules();
    setLifeRules({ bLo: 2, bHi: 3, sLo: 1, sHi: 3, pure: false });
    setLifeState({ nodes: [], ambient: false });
    setState((s) => ({
      control: { ...s.control, pattern: "life", blackout: true, beaconPreempt: false },
      gol: { ...DEFAULT_GOL, phase: "standby", ambient: false, t0: performance.now() / 1000 },
    }));
  },
  golSetPhase: (p) => setState((s) => {
    // "off" = DISARM: back to a normal always-alive field, lights back on, nodes gone.
    // (A previous bug left the tree blacked out + dark-at-rest after disarming.)
    if (p === "off") {
      if (preGolRules) { setLifeRules(preGolRules); preGolRules = null; } // restore the player's rules
      setLifeState({ ambient: true, nodes: [] });
      return { gol: { ...DEFAULT_GOL, phase: "off" }, control: { ...s.control, blackout: false } };
    }
    const dark = p === "standby" || p === "off1" || p === "off2";
    const ambient = p === "live" ? false : s.gol.ambient;
    setLifeState({ ambient });
    return { gol: { ...s.gol, phase: p, ambient, t0: performance.now() / 1000 }, control: { ...s.control, blackout: dark } };
  }),
  golFirstVisitor: (idx) => setState((s) => {
    if (s.gol.phase !== "standby") return {};
    void idx; // the ignition flourish blooms from the trunk; the visitor's node is placed once LIVE
    return { gol: { ...s.gol, phase: "off1", t0: performance.now() / 1000 }, control: { ...s.control, blackout: true } };
  }),
  addNode: (idx) => setState((s) => {
    const f = s.fixtures[idx]; if (!f) return {};
    if (s.gol.nodes.includes(idx)) return {}; // already a node
    const nodes = [...s.gol.nodes, idx].slice(-32);
    // node colours live INSIDE the picked theme (identity when Wild) — armed
    // "live" mode was ignoring the theme and reading washed-out (Elliot)
    setLifeState({ nodes: nodes.map((i) => ({ i, hue: themeMapHue(QUADRANT_HUES[s.fixtures[i].quadrant] ?? 0.05) })) });
    seedLife([idx], { hops: 1, hue: themeMapHue(QUADRANT_HUES[f.quadrant] ?? 0.05), bri: 1.15, ttl: 0 });
    return { gol: { ...s.gol, nodes } };
  }),
  clearNodes: () => { setLifeState({ nodes: [] }); setState((s) => ({ gol: { ...s.gol, nodes: [] } })); },
  setGolAmbient: (b) => { setLifeState({ ambient: b }); setState((s) => ({ gol: { ...s.gol, ambient: b } })); },
  setUnity: (on) => setState((s) => ({ gol: { ...s.gol, unity: on, unityT0: on ? performance.now() / 1000 : s.gol.unityT0 } })),
  // switching operator mode reshapes the panel AND nudges the engine into that world:
  // interactive → a CA rule runs (the tree reacts, you set rules); leaving interactive
  // disarms Game of Light so a latched dark phase can't strand the next mode.
  setUiMode: (m) => {
    recEvent("mode", { to: m });
    try { localStorage.setItem("ui.mode", m); } catch { /* fine */ }
    const s = get();
    if (m === "interactive") {
      if (!(CA_RULES as PatternId[]).includes(s.control.pattern)) s.set({ pattern: "life", strobe: false, blackout: false });
    } else if (s.gol.phase !== "off") {
      s.golSetPhase("off");
    }
    // entering ANY mode turns the lights back on (Elliot: blackout is a reset,
    // not a permanent hold — you leave it by choosing a mode)
    setState((q) => ({ uiMode: m, control: { ...q.control, blackout: false } }));
  },
  setDock: (b) => {
    try { localStorage.setItem("ui.dock", b ? "1" : "0"); } catch { /* fine */ }
    setState({ dock: b });
  },
  // BLACKOUT = reset to all-off: stop shows, disarm Game of Light, drop every
  // group layer, clear the field, go dark. Entering a mode afterwards lights the
  // tree back up (setUiMode/setGroupMode/playShow/set all lift blackout).
  resetAllOff: () => setState((s) => {
    clearLife();
    return {
      activeShow: null,
      layers: [],
      gol: { ...DEFAULT_GOL },
      groupModes: {},
      control: { ...s.control, blackout: true, beaconPreempt: false },
    };
  }),
  // PER-GROUP MODE ROUTING — maps a group's mode onto the layer engine so different
  // regions run different worlds at once (canopy interactive + chandelier on sound):
  //   interactive → that group's layer runs the Game of Life (the shared field
  //                 renders on its members; taps there seed it)
  //   sound       → its layer runs the audio-reactive hero look
  //   lightshow   → its saved Group look drives it (GroupPanel controls)
  //   follow      → no layer; the group rides the whole-tree base
  setGroupMode: (group, m) => {
    const s = get();
    const nums = group === "all" ? s.fixtures.map((f) => f.num) : (s.namedGroups[group] ?? []);
    if (group === "all") { s.setUiMode(m === "follow" ? s.uiMode : (m as UiMode)); setState((q) => ({ groupModes: { ...q.groupModes, all: m } })); return; }
    if (m === "interactive") s.setLayer(group, nums, { pattern: "life", brightness: 0.95, colorCycle: "off", strobe: false });
    else if (m === "sound") s.setLayer(group, nums, { pattern: "hero", audioSpeed: true, brightness: 0.95 });
    else if (m === "lightshow") { s.toggleGroupActive(group, true); }
    else s.removeLayer(group); // follow the base
    setState((q) => ({ groupModes: { ...q.groupModes, [group]: m }, selectedScope: group, control: { ...q.control, blackout: false } }));
  },
  setSelectedScope: (g) => setState({ selectedScope: g }),
  calSolo: (test) => setState((s) => {
    if (!test) return { overrides: {} }; // sequence done → normal output
    const o: Record<number, Override> = {};
    for (let i = 0; i < s.fixtures.length; i++) o[i] = { mode: "off" };
    o[test.idx] = { mode: "color", rgb: test.rgb };
    return { overrides: o };
  }),
  setGuest: (b) => setState({ guest: b }),
  setSensors: (p) => setState((s) => ({ sensors: { ...s.sensors, ...p } })),
  setCameraPreset: (c) => setState({ cameraPreset: c }),
  setCinematic: (b) => setState({ cinematic: b }),
  setTimeOfDay: (t) => setState((s) => ({ timeOfDay: Math.max(0, Math.min(1, t)), sensors: { ...s.sensors, ambient: Math.max(0, Math.min(1, t)) } })),
  setLayer: (id, nums, control) => setState((s) => ({ layers: [...s.layers.filter((l) => l.id !== id), { id, nums, control }] })),
  removeLayer: (id) => setState((s) => ({ layers: s.layers.filter((l) => l.id !== id) })),
  clearLayers: () => setState({ layers: [] }),
  // GROUPS — the panel's create/select/edit/activate. Activating or editing an
  // active group (re)derives its layer so the look applies immediately.
  defineGroup: (name, nums) => setState((s) => ({ namedGroups: { ...s.namedGroups, [name]: nums }, selectedGroup: name })),
  deleteGroup: (name) => setState((s) => {
    const ng = { ...s.namedGroups }; delete ng[name];
    const gc = { ...s.groupControls }; delete gc[name];
    const ga = { ...s.groupActive }; delete ga[name];
    return { namedGroups: ng, groupControls: gc, groupActive: ga, layers: s.layers.filter((l) => l.id !== name) };
  }),
  selectGroup: (name) => setState({ selectedGroup: name }),
  setGroupTheme: (name, id) => setState((s) => ({ groupThemes: { ...s.groupThemes, [name]: id } })),
  setGroupControl: (name, partial) => setState((s) => {
    const ctl = { ...DEFAULT_GROUP_CONTROL, ...s.groupControls[name], ...partial };
    const groupControls = { ...s.groupControls, [name]: ctl };
    const nums = s.namedGroups[name] ?? [];
    const layers = s.groupActive[name]
      ? [...s.layers.filter((l) => l.id !== name), { id: name, nums, control: ctl }]
      : s.layers;
    return { groupControls, layers };
  }),
  playShow: (id) => setState((s) => {
    recEvent("show", { id });
    if (!id) return { activeShow: null, layers: [] }; // stop → drop show layers
    // starting a SHOW exits the Game-of-Light lifecycle — otherwise a blackout
    // latched by an armed/dark GoL phase silently renders the whole show black
    if (s.gol.phase !== "off") {
      setLifeState({ ambient: true, nodes: [] });
      return { activeShow: id, showStartedAt: performance.now() / 1000, showSeed: Math.random(), gol: { ...DEFAULT_GOL }, control: { ...s.control, blackout: false } };
    }
    return { activeShow: id, showStartedAt: performance.now() / 1000, showSeed: Math.random(), control: { ...s.control, blackout: false } };
  }),
  toggleGroupActive: (name, on) => setState((s) => {
    const ctl = { ...DEFAULT_GROUP_CONTROL, ...s.groupControls[name] };
    const nums = s.namedGroups[name] ?? [];
    return {
      groupActive: { ...s.groupActive, [name]: on },
      groupControls: { ...s.groupControls, [name]: ctl },
      layers: on ? [...s.layers.filter((l) => l.id !== name), { id: name, nums, control: ctl }] : s.layers.filter((l) => l.id !== name),
    };
  }),
}));

// LLM / external control hook: expose the store so an operator (or Claude driving
// the page) can issue commands from outside the UI — e.g.
//   window.twin.getState().runCommand("light 1,7,17 color blue")
//   window.twin.getState().runScript("light 1 color blue\nlight 7 color red")
//   window.twin.getState().set({ pattern: "spiral", speed: 0.6 })
// This is the Path-1 "Claude as the LLM" channel; harmless in the browser app.
if (typeof window !== "undefined") (window as unknown as { twin: typeof useTwin }).twin = useTwin;
