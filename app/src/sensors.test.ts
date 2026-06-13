import { describe, it, expect } from "vitest";
import { applyEnv, windSway, DEFAULT_SENSORS, type Sensors } from "./sensors";
import type { Control } from "./store";

const base = { pattern: "solid", brightness: 1, hue: 0.5, sat: 1, speed: 1 } as unknown as Control;
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
    const empty = applyEnv(base, sens({ crowd: 0, ambient: 0 })).brightness;
    const packed = applyEnv(base, sens({ crowd: 1, ambient: 0 })).brightness;
    expect(packed).toBeGreaterThan(empty);
    expect(applyEnv(base, sens({ ambient: 1 })).brightness).toBeLessThan(empty + 0.01);
  });
  it("does not mutate the input control", () => {
    const c = { ...base };
    applyEnv(c, DEFAULT_SENSORS);
    expect(c.hue).toBe(0.5);
  });
});

describe("windSway", () => {
  it("is zero in still air and grows with wind", () => {
    expect(windSway(sens({ windKph: 0 }), 1, 0.3)).toBe(0);
    expect(Math.abs(windSway(sens({ windKph: 60 }), 1, 0.3))).toBeGreaterThan(0);
  });
});
