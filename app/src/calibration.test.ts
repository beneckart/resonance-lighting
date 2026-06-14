import { describe, it, expect } from "vitest";
import { emptyMap, assign, resolveMac, resolveFixtureId, unassignedFixtures, progress, identifyCommand } from "./calibration";
import type { SimFixture } from "./store";

const fx = (id: string): SimFixture => ({
  id, name: id, role: "downlight", zone: "mid", pos: [0, 0, 0], norm: [0.5, 0.5, 0.5],
  seqT: 0, seq: 0, heightT: 0.5, ring: 1, radialT: 0.5, rnd: 0.5, beamDeg: 120, lumens: 450,
});
const fixtures = [fx("F000"), fx("F001"), fx("F002")];
const T = "2026-06-13T00:00:00Z";

describe("calibration / commissioning", () => {
  it("assigns MAC → fixtureId and resolves both ways", () => {
    const m = assign(emptyMap(), "A1B2C3", "F001", T);
    expect(resolveFixtureId(m, "A1B2C3")).toBe("F001");
    expect(resolveMac(m, "F001")).toBe("A1B2C3");
  });

  it("is 1:1 — reassigning a MAC or fixtureId replaces the old mapping", () => {
    let m = assign(emptyMap(), "AAA", "F000", T);
    m = assign(m, "AAA", "F002", T); // same MAC, new slot
    expect(resolveMac(m, "F000")).toBeNull();
    expect(resolveFixtureId(m, "AAA")).toBe("F002");
    m = assign(m, "BBB", "F002", T); // same slot, new MAC
    expect(resolveMac(m, "F002")).toBe("BBB");
    expect(resolveFixtureId(m, "AAA")).toBeNull();
  });

  it("tracks commissioning progress + remaining fixtures", () => {
    let m = emptyMap();
    expect(progress(m, fixtures).pct).toBe(0);
    m = assign(m, "X1", "F000", T);
    m = assign(m, "X2", "F001", T);
    const p = progress(m, fixtures);
    expect(p.assigned).toBe(2);
    expect(p.remaining).toBe(1);
    expect(unassignedFixtures(m, fixtures).map((f) => f.id)).toEqual(["F002"]);
  });

  it("identify command carries the mapped MAC (or null when uncommissioned)", () => {
    const m = assign(emptyMap(), "MAPPED", "F000", T);
    expect(identifyCommand(m, "F000").mac).toBe("MAPPED");
    expect(identifyCommand(m, "F002").mac).toBeNull(); // cortex must sweep candidates
    expect(identifyCommand(m, "F000").kind).toBe("identify");
  });
});
