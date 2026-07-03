import { describe, it, expect } from "vitest";
import { buildPlan, judgeColor, tofSample, tofHistogram } from "./autocal";
import type { SimFixture } from "./store";

const fx = (num: number, role = "downlight"): SimFixture => ({
  id: `F${num}`, name: `f${num}`, role, zone: "mid", pos: [num, 2 + num * 0.1, 0], norm: [0.5, 0.5, 0.5],
  seqT: 0, seq: num - 1, heightT: 0.5, ring: 1, quadrant: 0, azimuth: 0, num, radialT: 0.5, rnd: (num * 0.37) % 1,
  neighbors: [], beamDeg: 120, lumens: 450,
});

describe("auto-calibration engine", () => {
  it("plans group-by-group, one light at a time, each colour, no duplicates", () => {
    const fixtures = [fx(1), fx(2), fx(3), fx(4)];
    const groups = { ring1: [2, 1], chandelier: [3], all: [1, 2, 3, 4] };
    const plan = buildPlan(fixtures, groups, ["red", "white"]);
    // ring1 sorted (1,2) then chandelier (3) then leftover 4 as "other"
    expect(plan.map((s) => `${s.num}:${s.color}`)).toEqual([
      "1:red", "1:white", "2:red", "2:white", "3:red", "3:white", "4:red", "4:white",
    ]);
    expect(plan[0].group).toBe("ring1");
    expect(plan[4].group).toBe("chandelier");
    expect(plan[6].group).toBe("other");
  });

  it("judges reported colour: pass on match (any brightness), fail on wrong/dead", () => {
    expect(judgeColor([1, 0, 0], [0.6, 0, 0]).ok).toBe(true); // dimmer but right colour
    expect(judgeColor([1, 0, 0], [0.5, 0.5, 0.5]).ok).toBe(false); // wrong colour
    expect(judgeColor([1, 0, 0], [0.01, 0, 0]).ok).toBe(false); // essentially dark
    expect(judgeColor([1, 0, 0], null).ok).toBe(false); // no report (dead)
    expect(judgeColor([1, 1, 1], [0.8, 0.8, 0.75]).ok).toBe(true); // white within tol
  });

  it("ToF sampling + histogram: errors centred near zero, all fixtures binned", () => {
    const fixtures = Array.from({ length: 40 }, (_, i) => fx(i + 1));
    const groundY = Math.min(...fixtures.map((f) => f.pos[1]));
    const errs = fixtures.map((f) => tofSample(f, groundY).errCm);
    const hist = tofHistogram(errs);
    expect(hist.reduce((a, b) => a + b.n, 0)).toBe(40); // every sample lands in a bin
    const mean = errs.reduce((a, b) => a + b, 0) / errs.length;
    expect(Math.abs(mean)).toBeLessThan(4); // centred near zero (small sensor noise)
  });
});
