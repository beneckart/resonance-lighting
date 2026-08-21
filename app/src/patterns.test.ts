import { setFieldTheme } from "./field";
import { applyThemeToLit, tameWhite, WHITE_CAP } from "./patterns";
import { describe, it, expect } from "vitest";
import { litFor, type Lit } from "./patterns";
import { PATTERN_IDS, ELEMENT_MODES, type Control, type SimFixture } from "./store";
import type { AudioFeatures } from "./audio";

const fx = (seq: number): SimFixture => ({
  id: `F${seq}`, name: `F${seq}`, role: "canopy", zone: "mid",
  pos: [1, 2, 3], norm: [0.3, 0.6, 0.8], seqT: seq / 10, seq,
  heightT: 0.6, ring: 1, quadrant: 0, azimuth: 0, num: 1, radialT: 0.5, rnd: 0.42, neighbors: [], beamDeg: 120, lumens: 450,
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

describe("theme funnel + white cap (Elliot 2026-07-08)", () => {
  it("tameWhite caps bright near-white to the low-glow ceiling", () => {
    const o = { r: 1, g: 0.97, b: 0.92 };
    tameWhite(o);
    expect(Math.max(o.r, o.g, o.b)).toBeCloseTo(WHITE_CAP, 5);
    expect(o.g / o.r).toBeCloseTo(0.97, 5); // neutrality preserved, only dimmed
  });
  it("tameWhite leaves colours and dim whites alone", () => {
    const red = { r: 1, g: 0.2, b: 0.2 };
    tameWhite(red);
    expect(red.r).toBe(1);
    const dim = { r: 0.3, g: 0.3, b: 0.3 };
    tameWhite(dim);
    expect(dim.r).toBe(0.3);
  });
  it("applyThemeToLit pulls a saturated colour toward the theme; identity without one; whites untouched", () => {
    setFieldTheme(null);
    const o1 = { r: 1, g: 0.1, b: 0.1 };
    applyThemeToLit(o1);
    expect(o1.r).toBeCloseTo(1, 5); // no theme → untouched
    setFieldTheme([0.5, 0.55, 0.6, 0.65]); // ocean
    const o2 = { r: 1, g: 0.1, b: 0.1 };
    applyThemeToLit(o2);
    expect(o2.b).toBeGreaterThan(o2.r); // pulled into the blue world
    const w = { r: 0.9, g: 0.9, b: 0.9 };
    applyThemeToLit(w);
    expect(w.r).toBeCloseTo(0.9, 5); // whites pass through
    setFieldTheme(null);
  });
});
