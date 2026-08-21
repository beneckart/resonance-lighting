import { describe, it, expect, beforeEach } from "vitest";
import { makeCue, loadCues, saveCues, nextCueIndex, type Cue } from "./cues";
import type { Control } from "./store";

const ctrl = { pattern: "ripple", brightness: 0.7, hue: 0.4 } as unknown as Control;

describe("cues", () => {
  beforeEach(() => localStorage.clear());

  it("makeCue snapshots the control + names it", () => {
    const c = makeCue("sunset", ctrl, 1000);
    expect(c.name).toBe("sunset");
    expect(c.control).toEqual(ctrl);
    expect(c.control).not.toBe(ctrl); // a copy
    expect(c.id).toContain("cue-1000");
  });

  it("auto-names when blank", () => {
    expect(makeCue("", ctrl, 1).name).toMatch(/^cue /);
  });

  it("save/load round-trips via localStorage", () => {
    const cues: Cue[] = [makeCue("a", ctrl, 2), makeCue("b", ctrl, 3)];
    saveCues(cues);
    const back = loadCues();
    expect(back.length).toBe(2);
    expect(back[0].name).toBe("a");
    expect(back[1].control.pattern).toBe("ripple");
  });

  it("loadCues is [] when empty", () => {
    expect(loadCues()).toEqual([]);
  });

  it("nextCueIndex wraps", () => {
    expect(nextCueIndex(0, 3)).toBe(1);
    expect(nextCueIndex(2, 3)).toBe(0);
    expect(nextCueIndex(0, 0)).toBe(0);
  });
});
