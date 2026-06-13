// Pure, hardware-independent beat tracker: feed it per-frame spectral-flux + a
// timestamp, get back onset + an interval-median BPM. Decoupled from Web-Audio so
// it can be unit-tested against a synthetic impulse train (see beat.test.ts).
/** Beat-synced sequencer step (ms). division 1 = quarter notes, 2 = eighths. */
export function beatStepMs(bpm: number, division = 1): number {
  if (bpm <= 0 || division <= 0) return 0;
  return 60000 / (bpm * division);
}

export class BeatTracker {
  fluxAvg = 0;
  lastOnsetT = -1;
  intervals: number[] = [];
  bpm = 0;
  onset = false;

  /** @param flux spectral flux this frame (≥0). @param t seconds. */
  push(flux: number, t: number): { onset: boolean; bpm: number } {
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
    return { onset: this.onset, bpm: this.bpm };
  }
}
