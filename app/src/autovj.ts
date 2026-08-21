import type { PatternId, VizMode } from "./store";

export interface Look {
  pattern: PatternId;
  visualizer: VizMode;
  hue: number;
}

/** The auto-VJ's curated look vocabulary (Resolume "Bag" model). */
export const LOOKS: Look[] = [
  { pattern: "spectrum", visualizer: "lanterns", hue: 0.0 },
  { pattern: "chase", visualizer: "orbs", hue: 0.55 },
  { pattern: "ripple", visualizer: "lanterns", hue: 0.7 },
  { pattern: "tricolor", visualizer: "lanterns", hue: 0.1 },
  { pattern: "sparkle", visualizer: "wire", hue: 0.85 },
  { pattern: "sequence", visualizer: "lanterns", hue: 0.3 },
  { pattern: "breathe", visualizer: "orbs", hue: 0.5 },
];

/** Shuffle-bag: every item plays once before any repeats (no immediate repeats). */
export class ShuffleBag<T> {
  private bag: number[] = [];
  constructor(private items: T[]) {}
  next(rand: () => number = Math.random): T {
    if (this.items.length === 0) throw new Error("empty bag");
    if (this.bag.length === 0) this.bag = this.items.map((_, i) => i);
    const k = Math.floor(rand() * this.bag.length);
    const idx = this.bag.splice(k, 1)[0];
    return this.items[idx];
  }
}

/** Phrase length in seconds = bars × 4 beats × beat-period (fallback when no tempo). */
export function phraseSeconds(bpm: number, bars: number): number {
  const beat = bpm > 0 ? 60 / bpm : 0.5;
  return beat * 4 * bars;
}
