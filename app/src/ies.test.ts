import { describe, it, expect } from "vitest";
import { parseIES } from "./ies";

// the real blender-baked downlight.ies (app/public/downlight.ies)
const IES = `IESNA:LM-63-2002
[TEST] Resonance downlight (Blender-baked)
[MANUFAC] Resonance Tree
TILT=NONE
1 -1 1 6 1 1 2 0 0 0
1.0 1.0 0.0
0.0 27.5 46.8 55.0 57.0 90.0
0.0
450.0 405.0 225.0 67.5 0.0 0.0
`;

describe("parseIES", () => {
  const p = parseIES(IES);

  it("reads vertical angles + candelas", () => {
    expect(p.vertAngles).toEqual([0, 27.5, 46.8, 55, 57, 90]);
    expect(p.candelas).toEqual([450, 405, 225, 67.5, 0, 0]);
    expect(p.peak).toBe(450);
  });

  it("beam angle = 2× the 50%-peak vertical angle (225cd @ 46.8°)", () => {
    expect(p.beamDeg).toBeCloseTo(93.6, 1);
  });

  it("field angle (10% peak) is wider than the beam, ≤ 2×57°", () => {
    expect(p.fieldDeg).toBeGreaterThan(p.beamDeg);
    expect(p.fieldDeg).toBeLessThanOrEqual(114);
  });
});
