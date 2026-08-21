import { describe, it, expect } from "vitest";
import { ShuffleBag, phraseSeconds, LOOKS } from "./autovj";

// deterministic LCG for reproducible shuffle tests
function lcg(seed: number) {
  let s = seed >>> 0;
  return () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296);
}

describe("autovj", () => {
  it("phraseSeconds: 124bpm × 8 bars ≈ 15.48s; falls back when no tempo", () => {
    expect(phraseSeconds(124, 8)).toBeCloseTo(15.48, 1);
    expect(phraseSeconds(0, 8)).toBeCloseTo(16, 5); // 0.5s/beat fallback
  });

  it("ShuffleBag plays every item once before repeating", () => {
    const items = [0, 1, 2, 3, 4];
    const bag = new ShuffleBag(items);
    const rand = lcg(42);
    const firstPass = items.map(() => bag.next(rand));
    expect([...firstPass].sort((a, b) => a - b)).toEqual(items); // a full permutation
    const secondPass = items.map(() => bag.next(rand));
    expect([...secondPass].sort((a, b) => a - b)).toEqual(items);
  });

  it("LOOKS are non-empty and well-formed", () => {
    expect(LOOKS.length).toBeGreaterThan(3);
    for (const l of LOOKS) {
      expect(typeof l.pattern).toBe("string");
      expect(l.hue).toBeGreaterThanOrEqual(0);
    }
  });
});
