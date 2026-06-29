import { create } from "zustand";
import { blenderToThree, type FixturesDoc } from "./fixtures";
import { runCommandStr, parseScript, type Override } from "./command";
import { makeCue, loadCues, saveCues, type Cue } from "./cues";
import type { Ripple } from "./interaction";
import { DEFAULT_SENSORS, type Sensors } from "./sensors";

export type PatternId =
  | "solid" | "breathe" | "chase" | "ripple" | "sparkle" | "sequence" | "spectrum" | "tricolor"
  | "spiral" | "godray" | "rising" | "planewipe" | "warmcool" | "bloom" | "firefly" | "ca" | "hero" | "plasma"
  | "chromatic" | "rings" | "fibonacci" | "sweep"
  | "wind" | "ember" | "rain" | "beacon";
export const PATTERN_IDS: PatternId[] = [
  "solid", "breathe", "chase", "ripple", "sparkle", "sequence", "spectrum", "tricolor",
  "spiral", "godray", "rising", "planewipe", "warmcool", "bloom", "firefly", "ca", "hero", "plasma", "chromatic", "rings", "fibonacci", "sweep",
];
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
  radialT: number; // 0..1 normalized horizontal distance from the trunk axis (in/out)
  rnd: number; // stable per-fixture random 0..1 — for sparkle/jitter
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
  init: (doc: FixturesDoc) => void;
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
  toggleGroupActive: (name: string, on: boolean) => void;
  playShow: (id: string | null) => void;
}

export const useTwin = create<TwinState>((setState, get) => ({
  fixtures: [],
  source: "",
  center: [0, 0, 0],
  size: 10,
  overrides: {},
  layers: [],
  namedGroups: {},
  groupControls: {},
  groupActive: {},
  selectedGroup: "ring1",
  activeShow: null,
  showStartedAt: 0,
  cmdLog: [],
  view: { mock: false, monitor: false, deadCount: 6 },
  monitorStats: { reporting: 0, dead: 0, stale: 0 },
  net: { channel: 11, driveReal: false },
  cues: loadCues(),
  timeline: { playing: false, stepSecs: 8 },
  ripples: [],
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
        radialT: radii[i] / maxR,
        rnd: rndOf(i),
        beamDeg: f.beam_deg ?? 120,
        lumens: f.lumens_max ?? 450,
        // schema 0.2: real per-fixture aim (Blender Z-up) → three-space direction
        aim: f.aim ? blenderToThree(f.aim) : undefined,
      };
    });
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
  set: (p) => setState((s) => ({ control: { ...s.control, ...p } })),
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
      const o =
        origin ??
        (s.fixtures.length ? s.fixtures[Math.floor(Math.random() * s.fixtures.length)].pos : [0, 0, 0]);
      const t0 = performance.now() / 1000;
      const ripples = [...s.ripples.filter((r) => t0 - r.t0 < 3), { x: o[0], y: o[1], z: o[2], t0 }].slice(-8);
      return { ripples };
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
  setGroupControl: (name, partial) => setState((s) => {
    const ctl = { ...DEFAULT_GROUP_CONTROL, ...s.groupControls[name], ...partial };
    const groupControls = { ...s.groupControls, [name]: ctl };
    const nums = s.namedGroups[name] ?? [];
    const layers = s.groupActive[name]
      ? [...s.layers.filter((l) => l.id !== name), { id: name, nums, control: ctl }]
      : s.layers;
    return { groupControls, layers };
  }),
  playShow: (id) => setState(() => {
    if (!id) return { activeShow: null, layers: [] }; // stop → drop show layers
    return { activeShow: id, showStartedAt: performance.now() / 1000 };
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
