import { describe, it, expect, beforeEach } from "vitest";
import { litFor, assignSunRays, SUN_RAY_COUNT, type Lit } from "./patterns";
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
      const f = fx({ sunRay: i % 3 === 0 ? i % 12 : undefined, radialT: (i % 10) / 10, ring: i % 3, rnd: (i * 0.37) % 1 });
      for (const t of [0, 2.7, 9.1]) {
        const o = lit(f, t);
        expect(o.b).toBeLessThanOrEqual(o.r + 1e-6); // blue never beats red
      }
    }
  });

  const level = (f: SimFixture, t: number) => { const o = lit(f, t); return Math.max(o.r, o.g, o.b); };

  it("a ray light varies CONTINUOUSLY over time — not a few discrete on/off levels", () => {
    const f = fx({ sunRay: 0, ring: 1, radialT: 0.6 });
    const vals = new Set<number>();
    for (let i = 0; i < 60; i++) vals.add(+level(f, i * 0.1).toFixed(2));
    expect(vals.size).toBeGreaterThan(15); // a smooth wave, many distinct brightnesses (not ≤6 frames)
  });

  it("energy flows OUTWARD along the ray: the bright crest's radius advances with time", () => {
    // dense sampling along one ray; find the peak-brightness radius at t and t+dt
    const peakR = (t: number): number => {
      let best = -1, bestR = 0;
      for (let r = 0.15; r <= 1; r += 0.01) {
        const v = level(fx({ sunRay: 0, ring: 1, radialT: r }), t);
        if (v > best) { best = v; bestR = r; }
      }
      return bestR;
    };
    const t0 = 1.0, dt = 0.2; // small step so the crest doesn't wrap a whole cycle
    const forward = (peakR(t0 + dt) - peakR(t0) + 1) % 1; // cyclic outward distance
    expect(forward).toBeGreaterThan(0.0);
    expect(forward).toBeLessThan(0.5); // moved outward, not a wrap artifact
    // and reverse runs it inward
    const rc = { ...ctrl, reverse: true } as Control;
    const peakRrev = (t: number): number => {
      let best = -1, bestR = 0;
      for (let r = 0.15; r <= 1; r += 0.01) {
        const out: Lit = { r: 0, g: 0, b: 0 };
        litFor(t, fx({ sunRay: 0, ring: 1, radialT: r }), rc, AUDIO, 118, out);
        const v = Math.max(out.r, out.g, out.b);
        if (v > best) { best = v; bestR = r; }
      }
      return bestR;
    };
    const back = (peakRrev(t0) - peakRrev(t0 + dt) + 1) % 1; // inward distance
    expect(back).toBeGreaterThan(0.0);
    expect(back).toBeLessThan(0.5);
  });

  it("a lit ray never fully blacks out (its shape stays visible as energy flows)", () => {
    const f = fx({ sunRay: 0, ring: 1, radialT: 0.55 });
    let mn = Infinity;
    for (let i = 0; i < 80; i++) mn = Math.min(mn, level(f, i * 0.13));
    expect(mn).toBeGreaterThan(0.05); // faint baseline glow — the ray line persists
  });

  it("lights outside the stencil are HARD OFF at all times (ray definition)", () => {
    const between = fx({ sunRay: undefined, ring: 1, radialT: 0.6 }); // not part of the drawing
    for (let i = 0; i < 30; i++) {
      const o = lit(between, i * 0.2);
      expect(Math.max(o.r, o.g, o.b)).toBeLessThan(0.001); // fully off, not dim
    }
  });

  it("assignSunRays stencils EXACTLY one light per ring per ray — most of the tree stays dark", () => {
    // simulate the real tree: 3 rings × 26 evenly spaced downlights
    const fleet: SimFixture[] = [];
    for (let ring = 0; ring < 3; ring++)
      for (let i = 0; i < 26; i++)
        fleet.push(fx({ azimuth: (i / 26) * Math.PI * 2 - Math.PI + ring * 0.04, ring, radialT: 0.3 + ring * 0.3 }));
    assignSunRays(fleet);
    const assigned = fleet.filter((f) => f.sunRay !== undefined);
    expect(assigned.length).toBe(3 * SUN_RAY_COUNT); // 36 lights draw the sun
    expect(fleet.length - assigned.length).toBe(78 - 36); // 42 lights NEVER light
    // each ray has exactly one light per ring
    for (let k = 0; k < SUN_RAY_COUNT; k++)
      for (let ring = 0; ring < 3; ring++)
        expect(fleet.filter((f) => f.sunRay === k && f.ring === ring).length).toBe(1);
  });

  it("colour follows the energy: the bright crest is golden, the dim trough is deep red", () => {
    // sample one light across time; its most golden moment (peak brightness)
    // must be more yellow than its dimmest moment (Ben's watt colourbar).
    const f = fx({ sunRay: 0, ring: 1, radialT: 0.55 });
    let bright: Lit = { r: 0, g: 0, b: 0 }, dim: Lit = { r: 9, g: 9, b: 9 };
    for (let i = 0; i < 80; i++) {
      const o = lit(f, i * 0.13);
      if (Math.max(o.r, o.g, o.b) > Math.max(bright.r, bright.g, bright.b)) bright = o;
      if (Math.max(o.r, o.g, o.b) < Math.max(dim.r, dim.g, dim.b)) dim = o;
    }
    const goldenness = (o: Lit) => o.g / Math.max(1e-6, o.r); // yellow ⇒ high green/red
    expect(goldenness(bright)).toBeGreaterThan(goldenness(dim) + 0.08);
    expect(dim.r).toBeGreaterThan(dim.b); // even the trough stays warm (red, never blue)
  });

  it("chandelier core is lit at all times (the sun never goes out)", () => {
    for (let i = 0; i < 20; i++) {
      const o = lit(fx({ role: "chandelier", zone: "crown", radialT: 0.05 }), i * 0.31);
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
