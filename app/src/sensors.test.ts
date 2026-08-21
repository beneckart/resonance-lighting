import { describe, it, expect } from "vitest";
import { applyEnv, windSway, autoBalanceGain, DEFAULT_SENSORS, type Sensors } from "./sensors";
import type { Control } from "./store";

const base = { pattern: "solid", brightness: 1, hue: 0.5, sat: 1, speed: 1, master: 1, autoBalance: false } as unknown as Control;
const sens = (p: Partial<Sensors>): Sensors => ({ ...DEFAULT_SENSORS, ...p });

describe("applyEnv", () => {
  it("cold biases hue cooler, hot biases warmer", () => {
    const cold = applyEnv(base, sens({ tempC: 0 })).hue; // +cool
    const hot = applyEnv(base, sens({ tempC: 40 })).hue; // -warm
    expect(cold).toBeGreaterThan(0.5);
    expect(hot).toBeLessThan(0.5);
  });
  it("wind speeds up animation, capped", () => {
    expect(applyEnv(base, sens({ windKph: 0 })).speed).toBeCloseTo(1, 5);
    expect(applyEnv(base, sens({ windKph: 45 })).speed).toBeCloseTo(2, 5);
    expect(applyEnv(base, sens({ windKph: 999 })).speed).toBeLessThanOrEqual(2.2);
  });
  it("crowd raises brightness, daylight washes it down", () => {
    const dim = { ...base, brightness: 0.6 } as typeof base; // headroom so crowd can raise it within the [0,1] clamp
    const empty = applyEnv(dim, sens({ crowd: 0, ambient: 0 })).brightness;
    const packed = applyEnv(dim, sens({ crowd: 1, ambient: 0 })).brightness;
    expect(packed).toBeGreaterThan(empty);
    expect(empty).toBeCloseTo(0.6, 5); // zero crowd = IDENTITY (no rest-state dimming)
    expect(applyEnv(dim, sens({ ambient: 1 })).brightness).toBeLessThan(empty + 0.01);
  });
  it("does not mutate the input control", () => {
    const c = { ...base };
    applyEnv(c, DEFAULT_SENSORS);
    expect(c.hue).toBe(0.5);
  });
});

describe("autoBalanceGain — ambient-light compensation", () => {
  it("night = no boost (1.0), full sun ≈ 2.0, monotonic, capped", () => {
    expect(autoBalanceGain(0)).toBeCloseTo(1, 5);
    expect(autoBalanceGain(1)).toBeCloseTo(2, 5);
    expect(autoBalanceGain(0.5)).toBeGreaterThan(autoBalanceGain(0));
    expect(autoBalanceGain(1)).toBeGreaterThan(autoBalanceGain(0.5));
    expect(autoBalanceGain(5)).toBeLessThanOrEqual(2.2); // clamps
  });
  it("exactly inverts the dayWash so day master ≈ night master (balanced)", () => {
    // dayWash = 1 − 0.5·ambient; autoBalanceGain is its inverse → product ≈ 1
    for (const a of [0, 0.3, 0.6, 1]) {
      expect(autoBalanceGain(a) * (1 - 0.5 * a)).toBeCloseTo(1, 5);
    }
  });
  it("applyEnv boosts master in daylight when on, leaves it when off", () => {
    const on = { ...base, autoBalance: true };
    expect(applyEnv(on, sens({ ambient: 1 })).master).toBeCloseTo(2, 5);
    expect(applyEnv(on, sens({ ambient: 0 })).master).toBeCloseTo(1, 5); // night unchanged
    expect(applyEnv(base, sens({ ambient: 1 })).master).toBeCloseTo(1, 5); // off = no boost
  });
});

describe("windSway", () => {
  it("is zero in still air and grows with wind", () => {
    expect(windSway(sens({ windKph: 0 }), 1, 0.3)).toBe(0);
    expect(Math.abs(windSway(sens({ windKph: 60 }), 1, 0.3))).toBeGreaterThan(0);
  });
});
