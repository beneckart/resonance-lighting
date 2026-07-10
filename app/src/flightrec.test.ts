import { describe, it, expect, beforeEach } from "vitest";
import { recReset, recEvent, recKeyframe, decodeKeyframe, flagBug, recSummary } from "./flightrec";

describe("flight recorder — interactive-mode black box (doc 18C)", () => {
  beforeEach(() => recReset());

  it("records events and freezes only the flag window", () => {
    recEvent("trigger", { idx: 12, hue: 0.5 });
    recEvent("mode", { to: "interactive" });
    const log = flagBug("test note", 118, 120_000);
    expect(log.events.length).toBe(2);
    expect(log.note).toBe("test note");
    expect(log.fixtures).toBe(118);
    expect(log.version).toBe(1);
  });

  it("keyframes round-trip through the packed encoding within quantization", () => {
    const n = 118;
    const bri = new Float32Array(n).map((_, i) => (i % 16) / 15);
    const hue = new Float32Array(n).map((_, i) => (i / n));
    recKeyframe(bri, hue);
    const log = flagBug("", n);
    expect(log.keyframes.length).toBe(1);
    const dec = decodeKeyframe(log.keyframes[0], n);
    for (let i = 0; i < n; i++) {
      expect(Math.abs(dec.bri[i] - bri[i])).toBeLessThanOrEqual(1 / 15 + 1e-6);
      expect(Math.abs(dec.hue[i] - hue[i])).toBeLessThanOrEqual(1 / 255 + 1e-6);
    }
  });

  it("keyframe capture self-throttles to ~2 Hz", () => {
    const bri = new Float32Array(10), hue = new Float32Array(10);
    for (let k = 0; k < 50; k++) recKeyframe(bri, hue); // burst in one tick
    expect(flagBug("", 10).keyframes.length).toBe(1);
  });

  it("event ring never exceeds its bound", () => {
    for (let k = 0; k < 5000; k++) recEvent("mark", { g: k });
    const s = recSummary();
    expect(s.events).toBeLessThanOrEqual(4096);
    expect(s.byKind.mark).toBeLessThanOrEqual(4096);
  });

  it("summary counts by kind", () => {
    recEvent("trigger", {});
    recEvent("trigger", {});
    recEvent("theme", { id: "love" });
    const s = recSummary();
    expect(s.byKind.trigger).toBe(2);
    expect(s.byKind.theme).toBe(1);
  });
});
