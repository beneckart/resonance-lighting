import { describe, it, expect } from "vitest";
import { BeatTracker } from "./beat";

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
