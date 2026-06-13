import { describe, it, expect } from "vitest";
import { eqDbForKnob } from "./audio";

describe("eqDbForKnob — DJ EQ knob → filter gain (boost-only)", () => {
  it("0 = neutral (0 dB), 1 = +12 dB, linear between", () => {
    expect(eqDbForKnob(0)).toBe(0);
    expect(eqDbForKnob(1)).toBe(12);
    expect(eqDbForKnob(0.5)).toBe(6);
  });
  it("clamps out-of-range knobs (never cuts, never over-boosts)", () => {
    expect(eqDbForKnob(-1)).toBe(0);
    expect(eqDbForKnob(2)).toBe(12);
  });
});
