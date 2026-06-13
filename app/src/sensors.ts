import type { Control } from "./store";

/** Environmental sensors (on-playa inputs). On real hardware these arrive from
 *  PIR/mmWave (crowd + motion), a temp probe, and an anemometer (wind); here
 *  they're simulated via the sensor panel so the show can be tuned to them. */
export interface Sensors {
  crowd: number; // 0..1 estimated crowd density around the tree (mmWave/PIR count)
  motion: number; // 0..1 instantaneous motion energy (someone moving past)
  tempC: number; // ambient temperature °C
  windKph: number; // wind speed km/h (anemometer)
  ambient: number; // 0..1 ambient daylight (0 = night, 1 = full sun)
}

export const DEFAULT_SENSORS: Sensors = {
  crowd: 0.4,
  motion: 0,
  tempC: 20,
  windKph: 8,
  ambient: 0,
};

const clamp = (x: number, lo: number, hi: number) => Math.min(hi, Math.max(lo, x));
const frac = (x: number) => x - Math.floor(x);

/** Auto-balance gain (Elliot: "sense the lighting from the piece and adapt the
 *  levels to balance automatically"). Daylight washes the look out perceptually
 *  (dayWash = 1 − 0.5·ambient); this gain is its INVERSE, so driving master by
 *  it compensates → the tree reads the same at noon as at night. Night
 *  (ambient 0) → 1.0 (unchanged); full sun (ambient 1) → 2.0. Capped. Pure. */
export function autoBalanceGain(ambient: number): number {
  const a = clamp(ambient, 0, 1);
  return clamp(1 / (1 - 0.5 * a), 1, 2.2);
}

/** Fold the live sensors into the commanded control: temperature biases hue
 *  warm/cool, wind speeds up motion, crowd raises energy, daylight washes the
 *  look down — and (when autoBalance is on) auto-boosts master to compensate
 *  for that wash so the tree stays readable. Pure — returns a new Control. */
export function applyEnv(c: Control, s: Sensors): Control {
  // temperature → warm/cool hue bias: cold pushes toward blue (0.6), hot toward amber (0.08)
  const tempShift = clamp((20 - s.tempC) / 40, -0.5, 0.5); // +cool / -warm
  const hue = frac(c.hue + tempShift * 0.18);
  // wind → faster animation (gusts liven the sway), capped
  const speed = c.speed * clamp(1 + s.windKph / 45, 1, 2.2);
  // crowd → more energy; daylight → washed out (perceptually dimmer)
  const crowdGain = 0.7 + 0.45 * clamp(s.crowd, 0, 1);
  const dayWash = 1 - 0.5 * clamp(s.ambient, 0, 1);
  const brightness = clamp(c.brightness * crowdGain * dayWash, 0, 1);
  // AUTO-BALANCE: drive master harder as ambient rises to punch through daylight
  const master = clamp(c.master * (c.autoBalance ? autoBalanceGain(s.ambient) : 1), 0, 2.2);
  return { ...c, hue, speed, brightness, master };
}

/** Wind-driven extra sway amount (0..~1) layered on top of pattern motion. */
export function windSway(s: Sensors, t: number, phase: number): number {
  const amp = clamp(s.windKph / 60, 0, 1);
  return amp * (0.5 + 0.5 * Math.sin(t * (0.6 + amp) + phase * 6.283));
}
