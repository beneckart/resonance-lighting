import { describe, it, expect } from "vitest";
import { staleUnitsScale, STALE_RATIO, OLD_UNITS_SCALE } from "./units";

describe("staleUnitsScale — the 10× stale-glb guard", () => {
  it("corrects the MEASURED 08-15 defect: 100.66 m glb vs 10.05 m fixtures", () => {
    expect(staleUnitsScale(100.66, 10.05)).toBe(OLD_UNITS_SCALE);
  });

  it("corrects the chandelier case: 15 m glb vs 10.05 m tree — wait, no it must NOT", () => {
    // 15/10.05 = 1.49 — a chandelier glb bigger than its cluster but smaller
    // than the tree is NOT the units defect; scaling it would break a correct
    // export. The guard keys on the tree-sized ratio, so the chandelier is
    // compared against ITS OWN 10× signature via the tree span at call site.
    expect(staleUnitsScale(15, 10.05)).toBe(1);
  });

  it("is a NO-OP on a correct re-export, even one legitimately wider than the lights", () => {
    // Ed's guy-spread trap: structure at 2× the fixture cloud is real geometry
    expect(staleUnitsScale(20, 10)).toBe(1);
    expect(staleUnitsScale(10, 10)).toBe(1);
  });

  it("still fires just past the threshold and not just under it", () => {
    expect(staleUnitsScale(10 * (STALE_RATIO + 0.01), 10)).toBe(OLD_UNITS_SCALE);
    expect(staleUnitsScale(10 * (STALE_RATIO - 0.01), 10)).toBe(1);
  });

  it("never divides by zero or propagates NaN into a render", () => {
    expect(staleUnitsScale(0, 10)).toBe(1);
    expect(staleUnitsScale(100, 0)).toBe(1);
    expect(staleUnitsScale(NaN, 10)).toBe(1);
    expect(staleUnitsScale(100, NaN)).toBe(1);
  });
});
