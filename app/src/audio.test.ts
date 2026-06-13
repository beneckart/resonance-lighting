import { describe, it, expect } from "vitest";
import { eqDbForKnob, audioInputsFrom, classifySection, spectralCentroid, centroidToHue } from "./audio";

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

describe("classifySection — live song section (P0-2)", () => {
  it("drop or loud → peak; rising-from-quiet → build; mid → groove; quiet → ambient", () => {
    expect(classifySection(0.5, 0.4, 0.9)).toBe("peak"); // drop
    expect(classifySection(0.8, 0.7, 0)).toBe("peak"); // loud
    expect(classifySection(0.5, 0.1, 0)).toBe("build"); // rose from quiet
    expect(classifySection(0.35, 0.3, 0)).toBe("groove");
    expect(classifySection(0.1, 0.05, 0)).toBe("ambient");
  });
});

describe("spectralCentroid — timbre brightness (P0-4)", () => {
  it("0 when silent; ~0 when energy is all in the lowest bin; ~1 in the highest", () => {
    expect(spectralCentroid([0, 0, 0, 0])).toBe(0); // no energy
    expect(spectralCentroid([10, 0, 0, 0])).toBe(0); // all in bin 0 → centroid 0
    expect(spectralCentroid([0, 0, 0, 10])).toBeCloseTo(1, 5); // all in top bin
  });
  it("a bright spectrum has a higher centroid than a bassy one", () => {
    const bassy = spectralCentroid([10, 8, 2, 0, 0, 0, 0, 0]);
    const bright = spectralCentroid([0, 0, 0, 0, 0, 2, 8, 10]);
    expect(bright).toBeGreaterThan(bassy);
    expect(bassy).toBeLessThan(0.5);
    expect(bright).toBeGreaterThan(0.5);
  });
});

describe("centroidToHue — timbre → colour", () => {
  it("dark → warm amber, bright → cool blue, monotonic + in range", () => {
    expect(centroidToHue(0)).toBeCloseTo(0.02, 5); // warm
    expect(centroidToHue(1)).toBeCloseTo(0.58, 5); // cool
    expect(centroidToHue(0.5)).toBeGreaterThan(centroidToHue(0));
    expect(centroidToHue(2)).toBeCloseTo(0.58, 5); // clamps
    expect(centroidToHue(-1)).toBeGreaterThanOrEqual(0.02);
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
