import { describe, it, expect } from "vitest";
import { BeatTracker, beatStepMs, quantizePhase, quantizedStep } from "./beat";

describe("beatStepMs", () => {
  it("maps BPM → step ms (quarter + eighth)", () => {
    expect(beatStepMs(124)).toBeCloseTo(483.87, 1);
    expect(beatStepMs(124, 2)).toBeCloseTo(241.94, 1);
    expect(beatStepMs(140)).toBeCloseTo(428.57, 1);
    expect(beatStepMs(0)).toBe(0);
  });
});

// OBJECTIVE proof the detector math is correct, independent of audio hardware:
// feed a synthetic 124-BPM kick impulse train and assert the detected BPM locks on.
describe("BeatTracker", () => {
  it("detects 124 BPM from a 124-BPM impulse train", () => {
    const bt = new BeatTracker();
    const fps = 60;
    const dur = 14; // seconds
    const period = 60 / 124; // 0.4839s
    let nextBeat = 0.5; // first kick after a short lead-in
    for (let f = 0; f < fps * dur; f++) {
      const t = f / fps;
      let flux = 0.002; // steady noise floor
      if (t >= nextBeat) {
        flux = 0.06; // a kick transient
        nextBeat += period;
      }
      bt.push(flux, t);
    }
    expect(bt.bpm).toBeGreaterThanOrEqual(120);
    expect(bt.bpm).toBeLessThanOrEqual(128);
  });

  it("detects ~90 BPM from a 90-BPM impulse train", () => {
    const bt = new BeatTracker();
    const fps = 60;
    const period = 60 / 90; // 0.667s
    let nextBeat = 0.5;
    for (let f = 0; f < fps * 16; f++) {
      const t = f / fps;
      let flux = 0.002;
      if (t >= nextBeat) {
        flux = 0.06;
        nextBeat += period;
      }
      bt.push(flux, t);
    }
    expect(bt.bpm).toBeGreaterThanOrEqual(86);
    expect(bt.bpm).toBeLessThanOrEqual(94);
  });

  it("phase PLL locks near the downbeat on a steady train", () => {
    const bt = new BeatTracker();
    const fps = 60;
    const period = 60 / 120; // 0.5s
    let nextBeat = 0.5;
    const phasesAtBeat: number[] = [];
    for (let f = 0; f < fps * 16; f++) {
      const t = f / fps;
      let flux = 0.002;
      let isBeat = false;
      if (t >= nextBeat) { flux = 0.06; nextBeat += period; isBeat = true; }
      const r = bt.push(flux, t);
      // after lock-in, sample the phase at the kick frames — should be near 0/1
      if (isBeat && t > 8) phasesAtBeat.push(r.phase);
    }
    expect(bt.phase).toBeGreaterThanOrEqual(0);
    expect(bt.phase).toBeLessThan(1);
    // every locked beat-frame phase sits near a beat boundary (0 or 1), not mid-beat
    for (const p of phasesAtBeat) {
      const dist = Math.min(p, 1 - p);
      expect(dist).toBeLessThan(0.2);
    }
  });

  it("quantizedStep advances 1 per 1/division beat (grid-locked)", () => {
    // division 1 (quarter): step increments once per whole beat
    expect(quantizedStep(0.0, 1)).toBe(0);
    expect(quantizedStep(0.99, 1)).toBe(0);
    expect(quantizedStep(1.0, 1)).toBe(1);
    expect(quantizedStep(3.5, 1)).toBe(3);
    // division 2 (eighths): two steps per beat
    expect(quantizedStep(0.5, 2)).toBe(1);
    expect(quantizedStep(2.0, 2)).toBe(4);
    // division 4 (sixteenths): four steps per beat
    expect(quantizedStep(1.0, 4)).toBe(4);
  });

  it("BeatTracker.beats is monotonic + ~tracks elapsed beats on a steady train", () => {
    const bt = new BeatTracker();
    const fps = 60, period = 60 / 120; // 120 BPM → 0.5s/beat
    let nextBeat = 0.5, last = 0;
    for (let f = 0; f < fps * 10; f++) {
      const t = f / fps;
      let flux = 0.002;
      if (t >= nextBeat) { flux = 0.06; nextBeat += period; }
      const r = bt.push(flux, t);
      expect(r.beats).toBeGreaterThanOrEqual(last - 0.2); // monotonic (PLL nudge is small)
      last = r.beats;
    }
    // ~10s at 120 BPM ≈ 20 beats (allow PLL slack)
    expect(bt.beats).toBeGreaterThan(16);
    expect(bt.beats).toBeLessThan(24);
  });

  it("quantizePhase snaps to the division grid", () => {
    expect(quantizePhase(0.06, 1)).toBe(0);
    expect(quantizePhase(0.46, 1)).toBe(0); // nearest quarter is the downbeat
    expect(quantizePhase(0.6, 1)).toBe(0); // 0.6 rounds to 1 → wraps to 0
    expect(quantizePhase(0.3, 2)).toBeCloseTo(0.5, 5); // nearest eighth
    expect(quantizePhase(0.2, 4)).toBeCloseTo(0.25, 5); // nearest sixteenth
  });
});
