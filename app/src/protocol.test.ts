import { describe, it, expect } from "vitest";
import { encodeFixture, buildShowFrame } from "./protocol";
import type { Control, SimFixture } from "./store";

const ctrl = {
  pattern: "spectrum", brightness: 1, hue: 0.5, sat: 1, speed: 1, master: 1,
} as unknown as Control;

const fx = (id: string): SimFixture => ({
  id, name: id, role: "canopy", zone: "mid", pos: [0, 0, 0], norm: [0.5, 0.5, 0.5],
  seqT: 0, seq: 0, heightT: 0.5, rnd: 0.5, beamDeg: 120, lumens: 450,
});

describe("protocol v1", () => {
  it("encodes control params (not pixels): bri/hue 0..255", () => {
    const p = encodeFixture(ctrl, fx("F000"));
    expect(p).toEqual({ id: "F000", pattern: "spectrum", bri: 255, hue: 128 });
  });
  it("override off → pattern off, bri 0", () => {
    expect(encodeFixture(ctrl, fx("F001"), { mode: "off" })).toEqual({ id: "F001", pattern: "off", bri: 0, hue: 0 });
  });
  it("override color → static rgb 0..255", () => {
    const p = encodeFixture(ctrl, fx("F002"), { mode: "color", rgb: [1, 0, 0] });
    expect(p.pattern).toBe("static");
    expect(p.rgb).toEqual([255, 0, 0]);
  });
  it("buildShowFrame is channel-pinned + epoched + per-fixture", () => {
    const f = buildShowFrame(ctrl, [fx("F000"), fx("F001")], { 1: { mode: "off" } }, 11, 7);
    expect(f.proto).toBe(1);
    expect(f.channel).toBe(11);
    expect(f.epoch).toBe(7);
    expect(f.fixtures.length).toBe(2);
    expect(f.fixtures[1].pattern).toBe("off");
  });
});
