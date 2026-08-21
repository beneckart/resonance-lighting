import { describe, it, expect } from "vitest";
import { ccToControl } from "./midi";

describe("ccToControl", () => {
  it("maps known CCs to normalized control changes", () => {
    expect(ccToControl(7, 127)).toEqual({ brightness: 1 });
    expect(ccToControl(1, 64)).toEqual({ hue: 64 / 127 });
    expect(ccToControl(2, 0)).toEqual({ sat: 0 });
    expect(ccToControl(3, 127)).toEqual({ speed: 3 });
    expect(ccToControl(4, 127)).toEqual({ master: 1 });
    expect(ccToControl(5, 127)).toEqual({ xfade: 1 });
  });
  it("returns null for unmapped CCs", () => {
    expect(ccToControl(99, 64)).toBeNull();
  });
});
