import { describe, it, expect } from "vitest";
import { strobeGate, eqGain, lerp } from "./dj";
import type { AudioFeatures } from "./audio";

const af = (p: Partial<AudioFeatures>): AudioFeatures => ({
  active: true, level: 0, bass: 0, mid: 0, treble: 0, beat: 0, onset: false, bpm: 0, drop: 0, ...p,
});

describe("dj helpers", () => {
  it("strobeGate toggles at hz", () => {
    expect(strobeGate(0, 10)).toBe(1);
    expect(strobeGate(0.05, 10)).toBe(0); // half a 10Hz cycle later
    expect(strobeGate(0.1, 10)).toBe(1);
  });

  it("eqGain = 1 when slider 0 or audio inactive", () => {
    expect(eqGain("low", 0, 0, 0, af({ bass: 1 }))).toBeCloseTo(1, 5);
    expect(eqGain("low", 1, 1, 1, { ...af({ bass: 1 }), active: false })).toBe(1);
  });

  it("eqGain tracks the zone's band when slider 1", () => {
    expect(eqGain("low", 1, 0, 0, af({ bass: 1 }))).toBeCloseTo(1.75, 5);
    expect(eqGain("high", 0, 0, 1, af({ treble: 0.5 }))).toBeCloseTo(0.95, 5);
    expect(eqGain("mid", 0, 1, 0, af({ mid: 0 }))).toBeCloseTo(0.15, 5);
  });

  it("lerp blends A↔B", () => {
    expect(lerp(0, 10, 0)).toBe(0);
    expect(lerp(0, 10, 1)).toBe(10);
    expect(lerp(0, 10, 0.5)).toBe(5);
  });
});
