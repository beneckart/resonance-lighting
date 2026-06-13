import { describe, it, expect } from "vitest";
import { rippleIntensity } from "./interaction";

describe("rippleIntensity", () => {
  it("peaks when the wavefront reaches the fixture", () => {
    // front = age*speed = 1*10 = 10; a fixture exactly at dist 10 is fully lit
    expect(rippleIntensity(10, 1, 10, 4)).toBeCloseTo(1 * (1 - 0.5), 5); // fade at age1 = 0.5
  });
  it("is 0 outside the band", () => {
    expect(rippleIntensity(50, 1, 10, 4)).toBe(0); // far from front (10)
  });
  it("is 0 before trigger + after fade", () => {
    expect(rippleIntensity(0, -1, 10, 4)).toBe(0);
    expect(rippleIntensity(10, 3, 10, 4)).toBe(0); // age 3 → fade clamped to 0
  });
  it("front advances with age", () => {
    // at age 2 the front is at 20; a fixture at 20 sees the band (but faded)
    expect(rippleIntensity(20, 1.9, 10, 5)).toBeGreaterThan(0);
  });
});
