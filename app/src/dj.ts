import type { AudioFeatures } from "./audio";

/** Strobe gate: 0/1 square wave at `hz` (full on-off cycles per second). */
export function strobeGate(t: number, hz: number): number {
  return Math.floor(t * hz * 2) % 2 === 0 ? 1 : 0;
}

/** EQ→light: a per-zone brightness multiplier. slider 0 → 1 (no audio effect);
 *  slider 1 → the zone tracks its band (low→bass, mid→mid, high→treble). */
export function eqGain(
  zone: string,
  low: number,
  mid: number,
  high: number,
  a: AudioFeatures
): number {
  if (!a.active) return 1;
  const slider = zone === "low" ? low : zone === "high" ? high : mid;
  const band = zone === "low" ? a.bass : zone === "high" ? a.treble : a.mid;
  return (1 - slider) + slider * (0.15 + band * 1.6);
}

export const lerp = (a: number, b: number, x: number) => a + (b - a) * x;
