import { describe, it, expect, beforeEach } from "vitest";
import { litFor, type Lit } from "./patterns";
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
      const f = fx({ azimuth: (i / 40) * Math.PI * 2 - Math.PI, radialT: (i % 10) / 10, rnd: (i * 0.37) % 1 });
      for (const t of [0, 2.7, 9.1]) {
        const o = lit(f, t);
        expect(o.b).toBeLessThanOrEqual(o.r + 1e-6); // blue never beats red
      }
    }
  });

  it("rays are directional: on-ray fixtures outshine between-ray sky", () => {
    // sample many azimuths at fixed radius/time; the ray structure ⇒ big contrast
    const levels: number[] = [];
    for (let i = 0; i < 96; i++) {
      const f = fx({ azimuth: (i / 96) * Math.PI * 2 - Math.PI, radialT: 0.6 });
      const o = lit(f, 3.3);
      levels.push(Math.max(o.r, o.g, o.b));
    }
    const mx = Math.max(...levels), mn = Math.min(...levels);
    expect(mx).toBeGreaterThan(0.25); // rays visibly lit
    expect(mn).toBeLessThan(mx * 0.35); // dark sky between rays
  });

  it("packets travel OUTWARD along a ray (peak radius advances with time)", () => {
    // fix an azimuth; find the brightest radius at t, then at t+dt — it should move out
    const peakR = (t: number): number => {
      let best = 0, bestR = 0;
      for (let r = 0.05; r <= 1; r += 0.01) {
        // stay ON the (curled, precessing) ray at each radius: azimuth tracks the spine
        const f = fx({ azimuth: 0.9 * r + t * 0.11, radialT: r });
        const o = lit(f, t);
        const v = Math.max(o.r, o.g, o.b);
        if (v > best) { best = v; bestR = r; }
      }
      return bestR;
    };
    const t0 = 2.0, dt = 0.35; // small step so the packet doesn't wrap
    const r0 = peakR(t0), r1 = peakR(t0 + dt);
    const forward = (r1 - r0 + 1) % 1; // cyclic outward distance
    expect(forward).toBeGreaterThan(0.0);
    expect(forward).toBeLessThan(0.5); // moved outward, not a wrap-around artifact
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

    it("sun up then down → Solar Ray activates", () => {
      useTwin.getState().solarPanelsCharging(1); // dawn: panels harvesting
      expect(useTwin.getState().control.pattern).toBe("solid"); // nothing yet
      useTwin.getState().solarPanelsCharging(0); // sunset: last panel done
      expect(useTwin.getState().control.pattern).toBe("solarray");
    });

    it("does not refire while it stays dark", () => {
      useTwin.getState().solarPanelsCharging(1);
      useTwin.getState().solarPanelsCharging(0);
      useTwin.setState({ control: { ...useTwin.getState().control, pattern: "chase" } }); // operator moved on
      useTwin.getState().solarPanelsCharging(0); // still night
      expect(useTwin.getState().control.pattern).toBe("chase");
    });

    it("never steals from a running show", () => {
      useTwin.getState().solarPanelsCharging(1);
      useTwin.setState({ activeShow: "performance" });
      useTwin.getState().solarPanelsCharging(0);
      expect(useTwin.getState().control.pattern).toBe("solid");
      useTwin.setState({ activeShow: null });
    });

    it("respects blackout", () => {
      useTwin.getState().solarPanelsCharging(1);
      useTwin.setState({ control: { ...useTwin.getState().control, blackout: true } });
      useTwin.getState().solarPanelsCharging(0);
      expect(useTwin.getState().control.pattern).toBe("solid");
    });
  });
});
