import { describe, it, expect } from "vitest";
import { validateFixturesDoc, blenderToThree, auditFixtures, type FixturesDoc } from "./fixtures";

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
  it("downlight aim Z-down → -Y, uplight aim Z-up → +Y", () => {
    expect(blenderToThree([0, 0, -1])).toEqual([0, -1, -0]);
    expect(blenderToThree([0, 0, 1])).toEqual([0, 1, -0]);
  });
});

const adoc = (fixtures: unknown[]): FixturesDoc => ({
  meta: { source: "t", exported: "t", up_axis: "Z", units: "m", count: fixtures.length, bbox: { min: [0, 0, 0], max: [1, 1, 1] }, schema: "resonance.fixtures/0.3" },
  fixtures: fixtures as never,
});
const af = (id: string, role: string, zone: string, aim?: number[]) => ({
  fixture_id: id, name: id, role, position: [0, 0, 0], zone, led_type: "rgbw_4w", lumens_max: 450, beam_deg: 120, design_color: [1, 1, 1], ...(aim ? { aim } : {}),
});

describe("auditFixtures", () => {
  it("counts roles/zones + flags mis-aimed fixtures (downlight up / uplight down)", () => {
    const a = auditFixtures(adoc([
      af("F0", "downlight", "low", [0, 0, -1]),  // ok: down
      af("F1", "uplight", "mid", [0, 0, 1]),     // ok: up
      af("F2", "uplight", "high", [0, 0, -1]),   // BAD: uplight aiming down
      af("F3", "chandelier", "high"),            // no aim
    ]));
    expect(a.byRole).toEqual({ downlight: 1, uplight: 2, chandelier: 1 });
    expect(a.byZone.high).toBe(2);
    expect(a.withAim).toBe(3);
    expect(a.warnings).toHaveLength(1);
    expect(a.warnings[0]).toContain("F2");
  });
});
