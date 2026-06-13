import { describe, it, expect } from "vitest";
import { compileShow, showToJson } from "./showcompiler";
import type { Cue } from "./cues";
import type { Control, SimFixture } from "./store";

const base = {
  pattern: "solid", brightness: 1, hue: 0.1, sat: 1, speed: 1, master: 1,
} as unknown as Control;

const fx = (id: string): SimFixture => ({
  id, name: id, role: "canopy", zone: "mid", pos: [0, 0, 0], norm: [0.5, 0.5, 0.5],
  seqT: 0, seq: 0, heightT: 0.5, rnd: 0.5, beamDeg: 120, lumens: 450,
});

const cues: Cue[] = [
  { id: "a", name: "intro", control: { pattern: "spectrum", hue: 0.5 } as unknown as Control },
  { id: "b", name: "drop", control: { pattern: "godray", brightness: 1 } as unknown as Control },
];

describe("compileShow", () => {
  const doc = compileShow(cues, [fx("F000"), fx("F001")], base, 11, 8000);

  it("one keyframe per cue, spaced stepMs, epoched", () => {
    expect(doc.keyframes).toHaveLength(2);
    expect(doc.keyframes[0].tMs).toBe(0);
    expect(doc.keyframes[1].tMs).toBe(8000);
    expect(doc.keyframes[0].frame.epoch).toBe(0);
    expect(doc.keyframes[1].frame.epoch).toBe(1);
  });

  it("merges cue control over base + encodes params (not pixels)", () => {
    expect(doc.keyframes[0].frame.fixtures[0].pattern).toBe("spectrum");
    expect(doc.keyframes[1].frame.fixtures[0].pattern).toBe("godray");
    // hue 0.5 → 128/255
    expect(doc.keyframes[0].frame.fixtures[0].hue).toBe(128);
  });

  it("meta is correct + channel-pinned", () => {
    expect(doc.meta.schema).toBe("resonance.show/0.1");
    expect(doc.meta.channel).toBe(11);
    expect(doc.meta.durationMs).toBe(16000);
    expect(doc.meta.fixtures).toBe(2);
    expect(doc.keyframes[0].frame.channel).toBe(11);
  });

  it("serializes to valid JSON round-trip", () => {
    const parsed = JSON.parse(showToJson(doc));
    expect(parsed.meta.keyframes).toBe(2);
    expect(parsed.keyframes[0].frame.proto).toBe(1);
  });

  it("empty cue list → empty, zero-duration show", () => {
    const empty = compileShow([], [fx("F000")], base, 6, 8000);
    expect(empty.keyframes).toHaveLength(0);
    expect(empty.meta.durationMs).toBe(0);
  });
});
