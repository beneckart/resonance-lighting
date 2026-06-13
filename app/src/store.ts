import { create } from "zustand";
import { blenderToThree, type FixturesDoc } from "./fixtures";
import { runCommandStr, parseScript, type Override } from "./command";
import { makeCue, loadCues, saveCues, type Cue } from "./cues";
import type { Ripple } from "./interaction";
import { DEFAULT_SENSORS, type Sensors } from "./sensors";

export type PatternId =
  | "solid" | "breathe" | "chase" | "ripple" | "sparkle" | "sequence" | "spectrum" | "tricolor"
  | "spiral" | "godray" | "rising" | "planewipe" | "warmcool"
  | "wind" | "ember" | "rain" | "beacon";
export const PATTERN_IDS: PatternId[] = [
  "solid", "breathe", "chase", "ripple", "sparkle", "sequence", "spectrum", "tricolor",
  "spiral", "godray", "rising", "planewipe", "warmcool",
];
/** Element / environmental modes (dossier PART 7). */
export const ELEMENT_MODES: PatternId[] = ["wind", "ember", "rain", "beacon"];

export type SeqMode = "fill" | "single" | "snake" | "groups" | "everyN" | "allOn" | "allOff";
export const SEQ_MODES: SeqMode[] = ["fill", "single", "snake", "groups", "everyN", "allOn", "allOff"];

export type VizMode = "lanterns" | "orbs" | "wire";
export const VIZ_MODES: VizMode[] = ["lanterns", "orbs", "wire"];

export interface SimFixture {
  id: string;
  name: string;
  role: string;
  zone: string;
  pos: [number, number, number]; // three-space (Y-up)
  norm: [number, number, number]; // normalized 0..1 within the fixture bbox
  seqT: number; // 0..1 order AROUND the tree (by azimuth) — for chases/snakes
  seq: number; // integer rank 0..N-1 around the tree — for the sequencer
  heightT: number; // 0..1 by height (low→high)
  rnd: number; // stable per-fixture random 0..1 — for sparkle/jitter
  beamDeg: number; // beam cone angle (deg) from fixtures.json
  lumens: number; // lumens_max from fixtures.json (beam photometrics)
}

export interface Control {
  pattern: PatternId;
  brightness: number; // 0..1 master
  hue: number; // 0..1
  sat: number; // 0..1
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
}

export const useTwin = create<TwinState>((setState, get) => ({
  fixtures: [],
  source: "",
  center: [0, 0, 0],
  size: 10,
  overrides: {},
  cmdLog: [],
  view: { mock: false, monitor: false, deadCount: 6 },
  monitorStats: { reporting: 0, dead: 0, stale: 0 },
  net: { channel: 11, driveReal: false },
  cues: loadCues(),
  timeline: { playing: false, stepSecs: 8 },
  ripples: [],
  guest: false,
  sensors: DEFAULT_SENSORS,
  control: {
    pattern: "sequence",
    brightness: 0.9,
    hue: 0.08,
    sat: 0.85,
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
        heightT: norm[1],
        rnd: rndOf(i),
        beamDeg: f.beam_deg ?? 120,
        lumens: f.lumens_max ?? 450,
      };
    });
    setState({ fixtures, center, size, source: doc.meta.source.split(":")[1] ?? doc.meta.source });
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
}));
