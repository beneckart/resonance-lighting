import { centroidToHue, type AudioFeatures } from "./audio";
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

/** Live motion speed from the music (Tier-1 'change speed by set features'):
 *  energy + tempo drive it, the drop spikes it. Pure. Silent → 1 (neutral). */
export function reactiveSpeed(a: AudioFeatures): number {
  if (!a.active) return 1;
  const energy = a.level * 0.6 + a.bass * 0.4;
  const tempo = a.bpm > 0 ? Math.min(1.5, a.bpm / 120) : 1; // 120 BPM → 1.0
  const drop = a.drop * 1.5; // the drop spikes speed
  return Math.max(0.3, Math.min(3, (0.5 + energy * 1.8) * tempo + drop));
}

export function decideLook(a: AudioFeatures, rand: () => number = Math.random): AiDecision {
  // DROP → big energetic burst (instant override, beats section)
  if (a.active && a.drop > 0.4) {
    return { pattern: "godray", hue: rand(), speed: 2.6, reason: `DROP — godray shafts, full energy` };
  }
  const e = energyOf(a);
  // TIMBRE-aware hue (P0-4): the spectral centroid sets the colour (bright/harsh
  // → cool, bassy/warm → amber), with a little randomness for variety. Falls
  // back to a warm idle hue when silent.
  const hue = (a.active ? centroidToHue(a.centroid) * 0.8 + rand() * 0.2 : 0.05 + rand() * 0.2) % 1;
  // SECTION steers the pattern family for the climactic states so the show
  // tracks musical STRUCTURE, not just instantaneous energy. peak forces the
  // hot family; build ramps an upward "rising" sweep as anticipation. groove +
  // ambient fall through to the energy tiers below.
  if (a.active && a.section === "peak") {
    const p = pick(HOT, rand);
    return { pattern: p, hue, speed: 2.2 + Math.min(0.8, a.bass), reason: `PEAK section — ${p}` };
  }
  if (a.active && a.section === "build") {
    return { pattern: "rising", hue, speed: 1.4 + e * 1.2, reason: `BUILD section — rising sweep, ramping` };
  }
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
