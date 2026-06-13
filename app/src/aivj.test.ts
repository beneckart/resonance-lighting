import { describe, it, expect } from "vitest";
import { decideLook, energyOf } from "./aivj";
import type { AudioFeatures } from "./audio";

const af = (p: Partial<AudioFeatures>): AudioFeatures => ({
  active: true, level: 0, bass: 0, mid: 0, treble: 0, beat: 0, onset: false, bpm: 120, drop: 0, ...p,
});
const seq = (vals: number[]) => { let i = 0; return () => vals[i++ % vals.length]; };

describe("AI-VJ policy", () => {
  it("silent → gentle idle energy", () => {
    expect(energyOf({ ...af({}), active: false })).toBeCloseTo(0.3, 5);
  });

  it("a DROP always picks the big godray burst at full speed", () => {
    const d = decideLook(af({ drop: 0.9, level: 0.5, bass: 0.5 }), seq([0.1]));
    expect(d.pattern).toBe("godray");
    expect(d.speed).toBeGreaterThan(2);
    expect(d.reason).toMatch(/DROP/);
  });

  it("high energy → a hot pattern", () => {
    const d = decideLook(af({ level: 0.9, bass: 0.9 }), seq([0]));
    expect(["spectrum", "tricolor", "chase", "godray"]).toContain(d.pattern);
    expect(d.speed).toBeGreaterThan(1.7);
  });

  it("low energy → a calm pattern, slow", () => {
    const d = decideLook(af({ level: 0.05, bass: 0.05 }), seq([0]));
    expect(["breathe", "warmcool", "ember", "rain"]).toContain(d.pattern);
    expect(d.speed).toBeLessThan(1);
  });

  it("hue + speed stay in range", () => {
    for (const v of [0, 0.3, 0.7, 0.99]) {
      const d = decideLook(af({ level: v, bass: v, treble: v }), seq([v]));
      expect(d.hue).toBeGreaterThanOrEqual(0);
      expect(d.hue).toBeLessThan(1);
      expect(d.speed).toBeGreaterThan(0);
    }
  });
});
