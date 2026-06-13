import { describe, it, expect } from "vitest";
import { beamIntensity, relativeBeam } from "./photometry";

describe("photometry", () => {
  it("relativeBeam ≈ 1 at the reference (450lm / 120°)", () => {
    expect(relativeBeam(450, 120)).toBeCloseTo(1, 5);
  });
  it("narrower beam → brighter; wider → dimmer (same lumens)", () => {
    expect(relativeBeam(450, 60)).toBeGreaterThan(1);
    expect(relativeBeam(450, 160)).toBeLessThan(1);
  });
  it("more lumens → brighter at the same angle", () => {
    expect(beamIntensity(900, 120)).toBeCloseTo(2 * beamIntensity(450, 120), 5);
  });
});
