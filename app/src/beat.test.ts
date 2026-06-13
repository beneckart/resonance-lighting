import { describe, it, expect } from "vitest";
import { BeatTracker, beatStepMs } from "./beat";

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
});
