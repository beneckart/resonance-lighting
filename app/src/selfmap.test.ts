import { describe, expect, it } from "vitest";
import { readFileSync } from "node:fs";
import type { SimFixture } from "./store";
import {
  DEFAULT_RSSI_MODEL, distanceToRssi, embed2d, heightSeedAssignment, lcg,
  macFromNum, manualQueue, pcaCandidates, rssiToDistance, scoreAgainstTruth,
  similarityFromPairs, simulateSurvey, slotsFromFixtures, solveMapping, stageRank,
} from "./selfmap";
import type { Anchor } from "./selfmap";
import { blenderToThree } from "./fixtures";

/** Deliberately NASTY synthetic layout: three concentric near-symmetric rings.
 *  Symmetry is the worst case for self-mapping — rotations of a ring are
 *  near-equivalent hypotheses, so this stresses the anchor/confirm machinery.
 *  (The real export is asymmetric and easier; see the real-tree test below.) */
function makeRingFixtures(): SimFixture[] {
  const out: SimFixture[] = [];
  let num = 1;
  const push = (role: string, x: number, y: number, z: number) => {
    const i = num;
    out.push({
      id: `F${String(i - 1).padStart(3, "0")}`, name: `t${i}`, role, zone: "test",
      pos: [x, y, z], norm: [0, 0, 0], seqT: 0, seq: i - 1, num: i, heightT: 0,
      ring: 0, quadrant: 0, azimuth: 0, radialT: 0, rnd: (i * 0.37) % 1,
      neighbors: [], beamDeg: 90, lumens: 400,
    } as SimFixture);
    num += 1;
  };
  const rings: [number, number, number][] = [[8, 14, 12], [14, 10, 16], [20, 7, 20]]; // [radius, height, count]
  rings.forEach(([r, h, count], ri) => {
    for (let k = 0; k < count; k++) {
      const a = (k / count) * Math.PI * 2 + ri * 0.4;
      const wob = 1 + 0.18 * Math.sin(k * 2.7 + ri); // slight asymmetry
      push("downlight", Math.cos(a) * r * wob, h + Math.sin(k * 1.3) * 1.5, Math.sin(a) * r * wob);
    }
  });
  for (let k = 0; k < 8; k++) {
    const a = (k / 8) * Math.PI * 2;
    push("chandelier", Math.cos(a) * 2.5, 22 + (k % 3), Math.sin(a) * 2.5);
  }
  for (let k = 0; k < 10; k++) {
    const a = (k / 10) * Math.PI * 2 + 0.9;
    push("uplight", Math.cos(a) * 11 * (1 + 0.15 * Math.cos(k * 2.1)), 0.6, Math.sin(a) * 11);
  }
  return out;
}

/** The real Blender export (118 fixtures, ~100 m spread) as sim fixtures. */
function realFixtures(): SimFixture[] {
  const doc = JSON.parse(readFileSync("public/fixtures.json", "utf8"));
  return doc.fixtures.map((f: { fixture_id: string; role: string; position: [number, number, number] }, i: number) => ({
    id: f.fixture_id, name: f.fixture_id, role: f.role, zone: "x",
    pos: blenderToThree(f.position), norm: [0, 0, 0], seqT: 0, seq: i, num: i + 1, heightT: 0,
    ring: 0, quadrant: 0, azimuth: 0, radialT: 0, rnd: (i * 0.37) % 1, neighbors: [], beamDeg: 90, lumens: 400,
  } as SimFixture));
}

describe("rssi model", () => {
  it("distance→rssi→distance round-trips", () => {
    for (const d of [1, 3, 8, 20]) {
      expect(rssiToDistance(distanceToRssi(d))).toBeCloseTo(d, 6);
    }
  });
  it("bench sanity: near is loud, far approaches the floor", () => {
    expect(distanceToRssi(1)).toBeGreaterThan(-45);
    expect(distanceToRssi(30, DEFAULT_RSSI_MODEL)).toBeLessThan(-70);
  });
});

describe("lcg", () => {
  it("is deterministic and in [0,1)", () => {
    const a = lcg(7), b = lcg(7);
    for (let i = 0; i < 100; i++) {
      const v = a();
      expect(v).toBe(b());
      expect(v).toBeGreaterThanOrEqual(0);
      expect(v).toBeLessThan(1);
    }
  });
});

describe("similarityFromPairs", () => {
  it("recovers a known rotation+scale+translation", () => {
    const th = 0.7, s = 1.8, tx = 4, ty = -2;
    const src: [number, number][] = [[0, 0], [3, 1], [-2, 5], [1, -4]];
    const dst = src.map(([x, y]): [number, number] => [
      s * (Math.cos(th) * x - Math.sin(th) * y) + tx,
      s * (Math.sin(th) * x + Math.cos(th) * y) + ty,
    ]);
    const xf = similarityFromPairs(src, dst)!;
    expect(xf).not.toBeNull();
    src.forEach((p, i) => {
      const q: [number, number] = [
        xf.scale * (xf.rot[0][0] * p[0] + xf.rot[0][1] * p[1]) + xf.t[0],
        xf.scale * (xf.rot[1][0] * p[0] + xf.rot[1][1] * p[1]) + xf.t[1],
      ];
      expect(q[0]).toBeCloseTo(dst[i][0], 4);
      expect(q[1]).toBeCloseTo(dst[i][1], 4);
    });
  });
  it("recovers a reflected mapping", () => {
    const src: [number, number][] = [[0, 0], [4, 0], [0, 3], [2, 2]];
    const dst = src.map(([x, y]): [number, number] => [x, -y]); // mirror
    const xf = similarityFromPairs(src, dst)!;
    src.forEach((p, i) => {
      const q = [
        xf.scale * (xf.rot[0][0] * p[0] + xf.rot[0][1] * p[1]) + xf.t[0],
        xf.scale * (xf.rot[1][0] * p[0] + xf.rot[1][1] * p[1]) + xf.t[1],
      ];
      expect(q[0]).toBeCloseTo(dst[i][0], 3);
      expect(q[1]).toBeCloseTo(dst[i][1], 3);
    });
  });
  it("returns null with fewer than 2 pairs", () => {
    expect(similarityFromPairs([[1, 1]], [[2, 2]])).toBeNull();
  });
});

describe("embed2d", () => {
  it("recovers a square from exact pairwise distances (up to similarity)", () => {
    const pts: [number, number][] = [[0, 0], [10, 0], [10, 10], [0, 10], [5, 5]];
    const macs = pts.map((_, i) => `M${i}`);
    const nodes = pts.map((p, i) => ({
      mac: macs[i], role: "downlight",
      rows: pts.map((q, j) => ({ mac: macs[j], med: distanceToRssi(Math.hypot(p[0] - q[0], p[1] - q[1]) || 0.1), n: 24 }))
        .filter((_, j) => j !== i),
      tofHeightM: null,
    }));
    const xy = embed2d(nodes, DEFAULT_RSSI_MODEL, 400, 3);
    // pairwise distances preserved (embedding is unaligned, so compare distances)
    for (let i = 0; i < pts.length; i++) {
      for (let j = i + 1; j < pts.length; j++) {
        const want = Math.hypot(pts[i][0] - pts[j][0], pts[i][1] - pts[j][1]);
        const a = xy.get(macs[i])!, b = xy.get(macs[j])!;
        const got = Math.hypot(a[0] - b[0], a[1] - b[1]);
        expect(Math.abs(got - want)).toBeLessThan(want * 0.15 + 0.5);
      }
    }
  });
});

describe("pcaCandidates", () => {
  it("produces 4 candidates covering flips", () => {
    const src: [number, number][] = [[0, 0], [5, 1], [2, 6], [-3, 2]];
    const dst = src.map(([x, y]): [number, number] => [-y * 2 + 1, x * 2 - 3]);
    expect(pcaCandidates(src, dst)).toHaveLength(4);
  });
});

describe("heightSeedAssignment", () => {
  it("puts nearly every node at the right LEVEL from ToF height alone", () => {
    const fixtures = makeRingFixtures();
    const sim = simulateSurvey(fixtures);
    const slots = slotsFromFixtures(fixtures);
    const seeded = heightSeedAssignment(sim.nodes, slots, new Map());
    const truthById = new Map(sim.truth.map((t) => [t.mac, t.fixtureId]));
    const slotH = new Map(slots.map((s) => [s.fixtureId, s.h]));
    let levelOk = 0;
    sim.nodes.forEach((n, i) => {
      const s = seeded[i];
      if (!s) return;
      const trueH = slotH.get(truthById.get(n.mac)!)!;
      if (Math.abs(s.h - trueH) < 2.5) levelOk += 1;
    });
    expect(levelOk / sim.nodes.length).toBeGreaterThanOrEqual(0.9);
  });
});

describe("full pipeline (simulated survey → solve)", () => {
  const fixtures = makeRingFixtures();
  const slots = slotsFromFixtures(fixtures);

  it("is EXACT on noiseless data (machinery sanity)", () => {
    const clean = simulateSurvey(fixtures, { placementJitterM: 0, boardOffsetDb: 0, pingNoiseDb: 0, tofSigmaM: 0, tofDropout: 0 });
    const anchors = [1, 20, 40].map((i) => ({ mac: clean.truth[i].mac, fixtureId: clean.truth[i].fixtureId }));
    const res = solveMapping(clean.nodes, slots, anchors);
    expect(scoreAgainstTruth(res, clean.truth).accuracy).toBe(1);
  });

  it("with 3 anchors survives realistic noise on the nasty symmetric rings", () => {
    // defaults: ±8 dB per-board offsets (net_bench), 0.35 m placement drift, ToF noise
    const sim = simulateSurvey(fixtures);
    const anchors = [1, 20, 40].map((i) => ({ mac: sim.truth[i].mac, fixtureId: sim.truth[i].fixtureId }));
    const res = solveMapping(sim.nodes, slots, anchors);
    const score = scoreAgainstTruth(res, sim.truth);
    expect(score.total).toBe(fixtures.length);
    expect(score.accuracy).toBeGreaterThanOrEqual(0.55);
    expect(res.usedAnchors).toBe(3);
    // and the confidence signal must catch most of what's wrong
    const queue = manualQueue(res);
    expect(queue.length).toBeGreaterThan(0);
    expect(queue.length).toBeLessThan(fixtures.length * 0.6);
  });

  it("ACTIVE CONFIRM LOOP: confirming the lowest-confidence lights converges fast", () => {
    // the install-day workflow: solve → flash-confirm the shakiest 10 → re-solve.
    const sim = simulateSurvey(fixtures);
    const truthById = new Map(sim.truth.map((t) => [t.mac, t.fixtureId]));
    const anchors: Anchor[] = [];
    const anchoredMacs = new Set<string>();
    let accuracy = 0;
    let confirms = 0;
    for (let round = 0; round < 5 && accuracy < 0.98; round++) {
      const res = solveMapping(sim.nodes, slots, anchors);
      accuracy = scoreAgainstTruth(res, sim.truth).accuracy;
      if (accuracy >= 0.98) break;
      const byConf = res.estimates.filter((e) => !anchoredMacs.has(e.mac)).sort((a, b) => a.confidence - b.confidence);
      for (const e of byConf.slice(0, 10)) {
        anchors.push({ mac: e.mac, fixtureId: truthById.get(e.mac)! });
        anchoredMacs.add(e.mac);
        confirms += 1;
      }
    }
    // far fewer manual confirmations than the 66-light fleet
    expect(accuracy).toBeGreaterThanOrEqual(0.98);
    expect(confirms).toBeLessThanOrEqual(40);
  });

  it("is deterministic for a given seed", () => {
    const a = simulateSurvey(fixtures, { seed: 5 });
    const b = simulateSurvey(fixtures, { seed: 5 });
    expect(JSON.stringify(a.nodes[10])).toBe(JSON.stringify(b.nodes[10]));
    const ra = solveMapping(a.nodes, slots, []);
    const rb = solveMapping(b.nodes, slots, []);
    expect(scoreAgainstTruth(ra, a.truth).accuracy).toBe(scoreAgainstTruth(rb, b.truth).accuracy);
  });

  it("ToF dropout nodes still get assigned", () => {
    const sim = simulateSurvey(fixtures, { tofDropout: 0.3, seed: 11 });
    const res = solveMapping(sim.nodes, slots, []);
    const noTof = res.estimates.filter((e) => e.estH === null);
    expect(noTof.length).toBeGreaterThan(0);
    expect(noTof.filter((e) => e.fixtureId).length).toBe(noTof.length);
  });
});

describe("real tree (118-fixture Blender export)", () => {
  it("no anchors + healthy mesh connectivity beats guessing by a wide margin", () => {
    const fixtures = realFixtures();
    const slots = slotsFromFixtures(fixtures);
    // 120 m radio range ≈ fully-connected survey graph (open-air ESP-NOW)
    const sim = simulateSurvey(fixtures, { seed: 7, maxRangeM: 120 });
    const res = solveMapping(sim.nodes, slots, []);
    const score = scoreAgainstTruth(res, sim.truth);
    // 118 slots — random same-role guessing would land ~1-2%
    expect(score.accuracy).toBeGreaterThanOrEqual(0.45);
  });
});

describe("plumbing", () => {
  it("stageRank orders the ladder", () => {
    expect(stageRank("heard")).toBeLessThan(stageRank("hypothesis"));
    expect(stageRank("hypothesis")).toBeLessThan(stageRank("confirmed"));
    expect(stageRank("confirmed")).toBeLessThan(stageRank("locked"));
  });
  it("macFromNum is stable and unique across the fleet", () => {
    const macs = new Set<string>();
    for (let i = 1; i <= 200; i++) macs.add(macFromNum(i));
    expect(macs.size).toBe(200);
    expect(macFromNum(7)).toBe(macFromNum(7));
  });
});
