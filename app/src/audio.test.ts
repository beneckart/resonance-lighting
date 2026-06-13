import { describe, it, expect } from "vitest";
import { eqDbForKnob, audioInputsFrom } from "./audio";

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

describe("audioInputsFrom — source/device picker (#5)", () => {
  it("keeps only audioinput devices + labels them (fallback when label blank)", () => {
    const got = audioInputsFrom([
      { kind: "audioinput", deviceId: "abc123def", label: "DJ Booth Out" },
      { kind: "videoinput", deviceId: "cam1", label: "Camera" },
      { kind: "audioinput", deviceId: "xyz789ghi", label: "" }, // pre-permission: blank label
      { kind: "audiooutput", deviceId: "spk1", label: "Speakers" },
    ]);
    expect(got).toHaveLength(2);
    expect(got[0]).toEqual({ id: "abc123def", label: "DJ Booth Out" });
    expect(got[1].id).toBe("xyz789ghi");
    expect(got[1].label).toContain("input 2");
  });
});
