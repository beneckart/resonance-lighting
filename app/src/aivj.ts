import type { AudioFeatures } from "./audio";
import type { PatternId } from "./store";

/** AI-VJ policy (PRD #32): reads the live audio digest {level,bass,mid,treble,
 *  beat,drop,bpm} and decides the next LOOK — pattern + hue + speed. This is the
 *  "smart sound → light" brain; the same decision an LLM operator would make,
 *  expressed as a deterministic, testable policy (injectable rand). */
export interface AiDecision {
  pattern: PatternId;
  hue: number;
  speed: number;
  reason: string;
}

const pick = <T,>(arr: T[], rand: () => number): T => arr[Math.floor(rand() * arr.length) % arr.length];

const HOT: PatternId[] = ["spectrum", "tricolor", "chase", "godray"];
const MID: PatternId[] = ["ripple", "spiral", "sequence", "rising"];
const CALM: PatternId[] = ["breathe", "warmcool", "ember", "rain"];

/** Energy 0..1 from the audio digest (or a gentle idle when silent). */
export function energyOf(a: AudioFeatures): number {
  if (!a.active) return 0.3;
  return Math.min(1, a.level * 0.5 + a.bass * 0.5);
}

export function decideLook(a: AudioFeatures, rand: () => number = Math.random): AiDecision {
  // DROP → big energetic burst
  if (a.active && a.drop > 0.4) {
    return { pattern: "godray", hue: rand(), speed: 2.6, reason: `DROP — godray shafts, full energy` };
  }
  const e = energyOf(a);
  // treble bias nudges the hue toward the cool end
  const hue = (a.active ? rand() * 0.85 + a.treble * 0.15 : 0.05 + rand() * 0.2) % 1;
  if (e > 0.66) {
    const p = pick(HOT, rand);
    return { pattern: p, hue, speed: 1.8 + Math.min(1, a.bass), reason: `high energy ${e.toFixed(2)} — ${p}` };
  }
  if (e > 0.33) {
    const p = pick(MID, rand);
    return { pattern: p, hue, speed: 1.1, reason: `mid energy ${e.toFixed(2)} — ${p}` };
  }
  const p = pick(CALM, rand);
  return { pattern: p, hue: 0.05 + rand() * 0.2, speed: 0.6, reason: `calm ${e.toFixed(2)} — ${p}` };
}
