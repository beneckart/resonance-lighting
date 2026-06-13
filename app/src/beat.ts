// Pure, hardware-independent beat tracker: feed it per-frame spectral-flux + a
// timestamp, get back onset + an interval-median BPM. Decoupled from Web-Audio so
// it can be unit-tested against a synthetic impulse train (see beat.test.ts).
/** Beat-synced sequencer step (ms). division 1 = quarter notes, 2 = eighths. */
export function beatStepMs(bpm: number, division = 1): number {
  if (bpm <= 0 || division <= 0) return 0;
  return 60000 / (bpm * division);
}

/** Snap a 0..1 beat phase to the nearest sub-division grid line (division 1 =
 *  quarter, 2 = eighth, 4 = sixteenth). Returns 0..1. */
export function quantizePhase(phase: number, division = 1): number {
  if (division <= 0) return phase;
  const p = ((phase % 1) + 1) % 1;
  return Math.round(p * division) / division % 1;
}

/** Quantizer (doc 16 P0-3): a monotonic beats-elapsed value → a grid-locked
 *  STEP index that advances by 1 every 1/division beat. Because beats is PLL-
 *  locked to the downbeat, step boundaries land exactly ON the ¼/⅛/16th grid —
 *  chases/sequencer steps hit on the beat, not floating off wall-clock. Pure. */
export function quantizedStep(beats: number, division = 1): number {
  return Math.floor(beats * Math.max(1, division));
}

export class BeatTracker {
  fluxAvg = 0;
  lastOnsetT = -1;
  intervals: number[] = [];
  bpm = 0;
  onset = false;
  /** Phase within the current beat, 0..1 (0 = the downbeat). PLL-locked to the
   *  detected tempo + corrected on each onset → lets the show land motion/flash
   *  ON the grid (predictive) instead of only reacting AT an onset. */
  phase = 0;
  /** Monotonic beats elapsed since lock — continuous (no wrap). Drives the
   *  Quantizer: floor(beats·div) = a grid-locked step index. */
  beats = 0;
  private lastT = -1;

  /** @param flux spectral flux this frame (≥0). @param t seconds. */
  push(flux: number, t: number): { onset: boolean; bpm: number; phase: number; beats: number } {
    this.fluxAvg += 0.05 * (flux - this.fluxAvg);
    const threshold = this.fluxAvg * 1.12 + 0.0015;
    this.onset = false;
    if (flux > threshold && t - this.lastOnsetT > 0.3) {
      if (this.lastOnsetT > 0) {
        const iv = t - this.lastOnsetT;
        if (iv > 0.25 && iv < 1.2) {
          this.intervals.push(iv);
          if (this.intervals.length > 12) this.intervals.shift();
          const sorted = [...this.intervals].sort((a, b) => a - b);
          const med = sorted[Math.floor(sorted.length / 2)];
          this.bpm = Math.round(60 / med);
        }
      }
      this.lastOnsetT = t;
      this.onset = true;
    }

    // --- phase-locked loop: free-run the phase at the detected tempo, nudge it
    // toward the beat on each onset so it converges + stays locked through gaps.
    if (this.bpm > 0 && this.lastT >= 0) {
      const period = 60 / this.bpm;
      const dt = Math.max(0, t - this.lastT);
      const adv = dt / period;
      this.phase = (this.phase + adv) % 1;
      this.beats += adv; // monotonic, same advance as phase
      if (this.onset) {
        // phase error to the nearest beat, wrapped to [-0.5, 0.5]
        let err = this.phase;
        if (err > 0.5) err -= 1;
        this.phase = ((this.phase - 0.18 * err) % 1 + 1) % 1; // gentle correction
        this.beats -= 0.18 * err; // keep the monotonic counter grid-aligned too
      }
    }
    this.lastT = t;
    return { onset: this.onset, bpm: this.bpm, phase: this.phase, beats: this.beats };
  }
}
