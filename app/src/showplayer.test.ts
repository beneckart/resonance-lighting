import { describe, it, expect } from "vitest";
import { cueTime } from "./ShowPlayer";
import type { ShowCue } from "./shows";

/** Regression guard for the crash that hit EVERY show on its final cue:
 *  the continuous-interp block calls cueTime(cues, i+1), and on the last cue
 *  that's cueTime(cues, length) — which must return a sane bound, never throw
 *  (the old code indexed cues[k] before its k>=length guard). */
const cues: ShowCue[] = [
  { at: 0, note: "a" },
  { at: 10, note: "b" },
  { at: 30, note: "c" }, // last
];
const noJit = () => 0;

describe("ShowPlayer cueTime — bounds-safe", () => {
  it("never throws for any k around the boundaries (this was the crash)", () => {
    for (let k = -5; k <= cues.length + 5; k++) {
      expect(() => cueTime(cues, k, noJit)).not.toThrow();
    }
  });

  it("the final-cue lookup at(i+1) = cueTime(cues, length) returns a bound past the last cue", () => {
    const past = cueTime(cues, cues.length, noJit); // the exact call that crashed
    expect(past).toBeGreaterThan(cues[cues.length - 1].at); // > 30, so span stays positive
  });

  it("clamps below 0 to the first cue", () => {
    expect(cueTime(cues, -1, noJit)).toBe(cues[0].at);
    expect(cueTime(cues, 0, noJit)).toBe(cues[0].at);
  });

  it("returns the cue's own time with no jitter", () => {
    expect(cueTime(cues, 1, noJit)).toBe(10);
    expect(cueTime(cues, 2, noJit)).toBe(30);
  });

  it("jitter stays within ±6% of the gap to the next cue", () => {
    const maxJit = () => 1; // extreme jitter
    const t = cueTime(cues, 1, maxJit); // gap to next = 30-10 = 20; ±6% = ±1.2
    expect(t).toBeGreaterThanOrEqual(10);
    expect(t).toBeLessThanOrEqual(10 + 0.06 * 20 + 1e-9);
  });

  it("single-cue show never throws (degenerate)", () => {
    const one: ShowCue[] = [{ at: 0, note: "only" }];
    for (let k = -2; k <= 3; k++) expect(() => cueTime(one, k, noJit)).not.toThrow();
  });
});
