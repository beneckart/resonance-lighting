import { describe, it, expect } from "vitest";
import { packPositions, passUniforms } from "./glslPass";
import type { SimFixture } from "./store";
import type { AudioFeatures } from "./audio";
import type { Control } from "./store";

const fx = (pos: [number, number, number]): SimFixture => ({
  id: "f", name: "f", role: "canopy", zone: "mid", pos, norm: [0, 1, 0],
  seqT: 0, seq: 0, heightT: 0.5, ring: 1, num: 1, radialT: 0.5, rnd: 0.5, neighbors: [], beamDeg: 120, lumens: 400,
});

describe("packPositions", () => {
  it("returns N×4 RGBA floats, w=1", () => {
    const d = packPositions([fx([0, 0, 0]), fx([2, 2, 2])]);
    expect(d.length).toBe(8);
    expect(d[3]).toBe(1);
    expect(d[7]).toBe(1);
  });
  it("centres the cloud at the origin + normalizes to ~[-1,1]", () => {
    // symmetric cloud about (5,5,5), extent 5 → ends map to ±1, centre to 0
    const d = packPositions([fx([0, 0, 0]), fx([10, 10, 10]), fx([5, 5, 5])]);
    expect(d[0]).toBeCloseTo(-1, 5); // min corner
    expect(d[4]).toBeCloseTo(1, 5);  // max corner
    expect(d[8]).toBeCloseTo(0, 5);  // centre
    for (let i = 0; i < d.length; i++) expect(Math.abs(d[i])).toBeLessThanOrEqual(1.0000001);
  });
  it("empty fixture set → empty array (no divide-by-zero)", () => {
    expect(packPositions([]).length).toBe(0);
  });
});

describe("passUniforms", () => {
  const ctrl = { speed: 1.5, hue: 0.4, sat: 0.8 } as unknown as Control;
  const aud = (p: Partial<AudioFeatures>): AudioFeatures => ({
    active: true, level: 0, bass: 0, mid: 0, treble: 0, beat: 0, onset: false, bpm: 124, drop: 0,
    section: "groove", beatPhase: 0.25, beatPulse: 0, centroid: 0, beatTime: 0, ...p,
  });
  it("maps control + audio onto the GLSL uniform names", () => {
    const u = passUniforms(3, ctrl, aud({ bass: 0.6, mid: 0.4, treble: 0.2, beat: 0.5 }));
    expect(u.uTime).toBe(3);
    expect(u.uSpeed).toBe(1.5);
    expect(u.uHue).toBe(0.4);
    expect(u.uBass).toBe(0.6);
    expect(u.uBPM).toBe(124);
    expect(u.uBeatPhase).toBe(0.25);
  });
  it("zeroes audio uniforms when inactive (silent → no false reactivity)", () => {
    const u = passUniforms(0, ctrl, aud({ active: false, bass: 0.9, treble: 0.9 }));
    expect(u.uBass).toBe(0);
    expect(u.uTreble).toBe(0);
  });
});
