import { describe, it, expect } from "vitest";
import { validateFixturesDoc, blenderToThree } from "./fixtures";

const valid = {
  meta: { source: "x", exported: "x", up_axis: "Z", units: "blender", count: 1, bbox: { min: [0, 0, 0], max: [1, 1, 1] }, schema: "resonance.fixtures/0.1" },
  fixtures: [{ fixture_id: "F000", name: "F000", role: "canopy", position: [1, 2, 3], zone: "mid", led_type: "rgbw_4w", lumens_max: 450, beam_deg: 120, design_color: [1, 1, 1] }],
};

describe("validateFixturesDoc", () => {
  it("accepts a valid doc", () => {
    expect(validateFixturesDoc(valid)).toEqual({ ok: true, errors: [] });
  });
  it("rejects non-objects + empties", () => {
    expect(validateFixturesDoc(null).ok).toBe(false);
    expect(validateFixturesDoc({ meta: valid.meta, fixtures: [] }).ok).toBe(false);
  });
  it("flags bad fixture fields", () => {
    const bad = { meta: valid.meta, fixtures: [{ fixture_id: "F0", position: [1, 2], zone: "mid" }] };
    const r = validateFixturesDoc(bad);
    expect(r.ok).toBe(false);
    expect(r.errors.some((e) => e.includes("position"))).toBe(true);
    expect(r.errors.some((e) => e.includes("beam_deg"))).toBe(true);
  });
});

describe("blenderToThree", () => {
  it("Z-up → Y-up: (x,y,z) → (x,z,-y)", () => {
    expect(blenderToThree([1, 2, 3])).toEqual([1, 3, -2]);
  });
});
