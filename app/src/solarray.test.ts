import { describe, it, expect, beforeEach } from "vitest";
import { litFor, solarRayStage, type Lit } from "./patterns";
import { solarHandoffFired } from "./sensors";
import { useTwin, type Control, type SimFixture } from "./store";

/** SOLAR RAY (Ben's mode): chandelier = molten core, warm rays radiate outward,
 *  auto-fires at the solar handoff (last panel stops harvesting). */

const ctrl = {
  pattern: "solarray", brightness: 1, hue: 0.5, sat: 1, speed: 1, master: 1,
  colorCycle: "off", reverse: false, stepMs: 200, seqMode: "fill", seqGroup: 24, seqEveryN: 2,
  syncToBeat: false, beatDiv: 1, strobe: false, blackout: false,
} as unknown as Control;

const AUDIO = { active: false, level: 0, bass: 0, treble: 0, beat: 0, beatPulse: 0, drop: 0, bpm: 0, beatTime: 0 } as never;

const fx = (over: Partial<SimFixture>): SimFixture => ({
  id: "F000", name: "t", role: "downlight", zone: "high", pos: [0, 0, 0], norm: [0.5, 0.5, 0.5],
  seqT: 0, seq: 0, num: 1, heightT: 0.5, ring: 1, quadrant: 0, azimuth: 0, radialT: 0.5,
  rnd: 0.5, neighbors: [], beamDeg: 120, lumens: 450, ...over,
});

const lit = (f: SimFixture, t: number): Lit => {
  const out: Lit = { r: 0, g: 0, b: 0 };
  litFor(t, f, ctrl, AUDIO, 118, out);
  return out;
};

const warmness = (o: Lit) => (o.r - o.b); // warm palette ⇒ red channel dominates blue

describe("solarray pattern", () => {
  it("chandelier core is lit, bright and WARM (the molten sun)", () => {
    const core = lit(fx({ role: "chandelier", zone: "crown", radialT: 0.05 }), 1.0);
    const mx = Math.max(core.r, core.g, core.b);
    expect(mx).toBeGreaterThan(0.5); // bright
    expect(core.r).toBeGreaterThan(core.b); // warm: red over blue
    expect(warmness(core)).toBeGreaterThan(0.3);
  });

  it("every fixture renders in the warm red→yellow palette only (never blue/green-dominant)", () => {
    for (let i = 0; i < 40; i++) {
      const f = fx({ azimuth: (i / 40) * Math.PI * 2 - Math.PI, radialT: (i % 10) / 10, ring: i % 3, rnd: (i * 0.37) % 1 });
      for (const t of [0, 2.7, 9.1]) {
        const o = lit(f, t);
        expect(o.b).toBeLessThanOrEqual(o.r + 1e-6); // blue never beats red
      }
    }
  });

  // the frame clock: 0 core · 1 +inner · 2 +middle · 3 +outer · 4-5 pulse
  const tAtStage = (s: number) => s * 0.55 + 0.27; // mid-frame time at speed 1

  it("frame clock steps 0..5 and loops", () => {
    for (let s = 0; s < 6; s++) expect(solarRayStage(tAtStage(s), 1)).toBe(s);
    expect(solarRayStage(tAtStage(6), 1)).toBe(0); // loops
  });

  it("chains ignite inner → middle → outer as the frames advance (the wave)", () => {
    const onRay = (ring: number) => fx({ azimuth: 0, ring, radialT: 0.3 + ring * 0.3 }); // azimuth 0 = a ray spine
    const level = (f: SimFixture, t: number) => { const o = lit(f, t); return Math.max(o.r, o.g, o.b); };
    // frame 1: inner lit, middle+outer still dark
    expect(level(onRay(0), tAtStage(1))).toBeGreaterThan(0.3);
    expect(level(onRay(1), tAtStage(1))).toBeLessThan(0.05);
    expect(level(onRay(2), tAtStage(1))).toBeLessThan(0.05);
    // frame 2: middle joins, outer still dark
    expect(level(onRay(1), tAtStage(2))).toBeGreaterThan(0.3);
    expect(level(onRay(2), tAtStage(2))).toBeLessThan(0.05);
    // frame 3: full sun — all three rings lit
    expect(level(onRay(0), tAtStage(3))).toBeGreaterThan(0.3);
    expect(level(onRay(1), tAtStage(3))).toBeGreaterThan(0.3);
    expect(level(onRay(2), tAtStage(3))).toBeGreaterThan(0.3);
  });

  it("skips the lights in between — off-chain fixtures are HARD OFF in every frame (ray definition)", () => {
    const m = (Math.PI * 2) / 12;
    const between = fx({ azimuth: m / 2, ring: 1, radialT: 0.6 }); // halfway between two ray spines
    for (let s = 0; s < 6; s++) {
      const o = lit(between, tAtStage(s));
      expect(Math.max(o.r, o.g, o.b)).toBeLessThan(0.001); // fully off, not dim
    }
  });

  it("chandelier core is lit in EVERY frame (the sun never goes out)", () => {
    for (let s = 0; s < 6; s++) {
      const o = lit(fx({ role: "chandelier", zone: "crown", radialT: 0.05 }), tAtStage(s));
      expect(Math.max(o.r, o.g, o.b)).toBeGreaterThan(0.25);
    }
  });
});

describe("solar handoff trigger", () => {
  it("fires exactly on the >0 → 0 edge (the last panel loses the sun)", () => {
    expect(solarHandoffFired(1, 0)).toBe(true);
    expect(solarHandoffFired(78, 0)).toBe(true);
    expect(solarHandoffFired(0, 0)).toBe(false); // already dark — no refire
    expect(solarHandoffFired(5, 3)).toBe(false); // sun still up
    expect(solarHandoffFired(0, 4)).toBe(false); // sunrise re-arms, doesn't fire
  });

  describe("store action", () => {
    beforeEach(() => {
      useTwin.setState({
        solarChargingCount: 0, activeShow: null, guest: false,
        control: { ...useTwin.getState().control, pattern: "solid", blackout: false },
      });
    });

    it("sun up then down → the ☀️ Solar Ray SHOW starts", () => {
      useTwin.getState().solarPanelsCharging(1); // dawn: panels harvesting
      expect(useTwin.getState().activeShow).toBe(null); // nothing yet
      useTwin.getState().solarPanelsCharging(0); // sunset: last panel done
      expect(useTwin.getState().activeShow).toBe("solarray-show");
      useTwin.getState().playShow(null);
    });

    it("does not refire while it stays dark", () => {
      useTwin.getState().solarPanelsCharging(1);
      useTwin.getState().solarPanelsCharging(0);
      useTwin.getState().playShow(null); // operator stopped it
      useTwin.getState().solarPanelsCharging(0); // still night
      expect(useTwin.getState().activeShow).toBe(null);
    });

    it("never steals from a running show", () => {
      useTwin.getState().solarPanelsCharging(1);
      useTwin.setState({ activeShow: "performance" });
      useTwin.getState().solarPanelsCharging(0);
      expect(useTwin.getState().activeShow).toBe("performance");
      useTwin.setState({ activeShow: null });
    });

    it("respects blackout", () => {
      useTwin.getState().solarPanelsCharging(1);
      useTwin.setState({ control: { ...useTwin.getState().control, blackout: true } });
      useTwin.getState().solarPanelsCharging(0);
      expect(useTwin.getState().activeShow).toBe(null);
    });
  });
});
