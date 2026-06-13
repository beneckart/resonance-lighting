import { create } from "zustand";
import { blenderToThree, type FixturesDoc } from "./fixtures";

export type PatternId = "solid" | "breathe" | "chase" | "ripple" | "sparkle";
export const PATTERN_IDS: PatternId[] = ["solid", "breathe", "chase", "ripple", "sparkle"];

export interface SimFixture {
  id: string;
  name: string;
  role: string;
  zone: string;
  pos: [number, number, number]; // three-space (Y-up)
  norm: [number, number, number]; // normalized 0..1 within the fixture bbox
  seqT: number; // 0..1 order AROUND the tree (by azimuth) — for chases/snakes
  heightT: number; // 0..1 by height (low→high)
  rnd: number; // stable per-fixture random 0..1 — for sparkle/jitter
}

export interface Control {
  pattern: PatternId;
  brightness: number; // 0..1 master
  hue: number; // 0..1
  sat: number; // 0..1
  speed: number; // 0..3
}

interface TwinState {
  fixtures: SimFixture[];
  source: string;
  control: Control;
  center: [number, number, number];
  size: number;
  init: (doc: FixturesDoc) => void;
  set: (p: Partial<Control>) => void;
}

export const useTwin = create<TwinState>((setState) => ({
  fixtures: [],
  source: "",
  center: [0, 0, 0],
  size: 10,
  control: { pattern: "breathe", brightness: 0.9, hue: 0.08, sat: 0.85, speed: 1 },
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
        heightT: norm[1],
        rnd: rndOf(i),
      };
    });
    setState({ fixtures, center, size, source: doc.meta.source.split(":")[1] ?? doc.meta.source });
  },
  set: (p) => setState((s) => ({ control: { ...s.control, ...p } })),
}));
