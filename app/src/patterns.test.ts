import { describe, it, expect } from "vitest";
import { litFor, type Lit } from "./patterns";
import { PATTERN_IDS, ELEMENT_MODES, type Control, type SimFixture } from "./store";
import type { AudioFeatures } from "./audio";

const fx = (seq: number): SimFixture => ({
  id: `F${seq}`, name: `F${seq}`, role: "canopy", zone: "mid",
  pos: [1, 2, 3], norm: [0.3, 0.6, 0.8], seqT: seq / 10, seq,
  heightT: 0.6, rnd: 0.42, beamDeg: 120, lumens: 450,
});
const ctrl = (pattern: string): Control => ({
  pattern, brightness: 0.9, hue: 0.1, sat: 0.85, speed: 1,
  seqMode: "fill", stepMs: 200, groupSize: 24, everyN: 2, syncToBeat: false, beatDiv: 1,
  visualizer: "lanterns", xfade: 0, djPatternB: "ripple", djHueB: 0.6,
  eqLow: 0, eqMid: 0, eqHigh: 0, master: 1, strobe: false, strobeHz: 10, autoVj: false, autoBars: 8,
} as unknown as Control);
const silent: AudioFeatures = {
  active: false, level: 0, bass: 0, mid: 0, treble: 0, beat: 0, onset: false, bpm: 0, drop: 0, section: "ambient", beatPhase: 0, beatPulse: 0, centroid: 0, beatTime: 0,
};
const loud: AudioFeatures = { ...silent, active: true, level: 0.8, bass: 0.7, mid: 0.5, treble: 0.4, beat: 1, onset: true, bpm: 124 };

const ALL = [...PATTERN_IDS, ...ELEMENT_MODES];

describe("litFor — every pattern produces valid colour", () => {
  it("covers all pattern + element ids", () => {
    expect(ALL).toContain("spiral");
    expect(ALL).toContain("godray");
    expect(ALL).toContain("warmcool");
  });

  for (const p of ALL) {
    it(`${p}: finite RGB in [0,1] across time, silent + loud`, () => {
      const out: Lit = { r: 0, g: 0, b: 0 };
      for (const audio of [silent, loud]) {
        for (const t of [0, 0.37, 1.2, 3.9, 12.5]) {
          for (let s = 0; s < 6; s++) {
            litFor(t, fx(s), ctrl(p), audio, 6, out);
            for (const c of [out.r, out.g, out.b]) {
              expect(Number.isFinite(c)).toBe(true);
              expect(c).toBeGreaterThanOrEqual(0);
              expect(c).toBeLessThanOrEqual(1.000001);
            }
          }
        }
      }
    });
  }
});
