import { describe, it, expect } from "vitest";
import { runCommandStr, parseScript } from "./command";
import type { SimFixture } from "./store";

const mk = (id: string, seq: number, zone: string): SimFixture => ({
  id, name: id, role: "canopy", zone, pos: [0, 0, 0], norm: [0.5, 0.5, 0.5],
  seqT: 0, seq, heightT: 0.5, ring: 1, num: 1, radialT: 0.5, rnd: 0.5, neighbors: [], beamDeg: 120, lumens: 450,
});
const fixtures: SimFixture[] = [
  mk("F000", 0, "low"), mk("F001", 1, "low"), mk("F002", 2, "mid"),
  mk("F003", 3, "high"), mk("F004", 4, "high"),
];

describe("command parser", () => {
  it("global pattern + params", () => {
    expect(runCommandStr("all pattern sequence", fixtures).control).toEqual({ pattern: "sequence" });
    expect(runCommandStr("hue 0.5", fixtures).control).toEqual({ hue: 0.5 });
  });
  it("targeted overrides resolve the right fixtures", () => {
    expect(runCommandStr("zone high off", fixtures).setOverrides?.idx).toEqual([3, 4]);
    expect(runCommandStr("range 0-1 color red", fixtures).setOverrides?.idx).toEqual([0, 1]);
    expect(runCommandStr("every 2 on", fixtures).setOverrides?.idx).toEqual([0, 2, 4]);
    expect(runCommandStr("fixture F002 color #fff", fixtures).setOverrides?.idx).toEqual([2]);
  });
  it("clear + unrecognized", () => {
    expect(runCommandStr("clear", fixtures).clear).toBe(true);
    expect(runCommandStr("florble", fixtures).msg).toContain("unrecognized");
  });
});

describe("parseScript", () => {
  it("splits, trims, drops blanks + comments", () => {
    const script = "all pattern ripple\n\n# a comment\n  hue 0.3  \nzone high off\n";
    expect(parseScript(script)).toEqual(["all pattern ripple", "hue 0.3", "zone high off"]);
  });
});
