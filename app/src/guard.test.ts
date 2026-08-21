import { describe, it, expect } from "vitest";
import { guestClamp } from "./guard";
import type { Control } from "./store";

const base = { brightness: 1, master: 1, strobe: true, hue: 0.5, pattern: "solid" } as unknown as Control;

describe("guestClamp", () => {
  it("caps brightness + master and disables strobe", () => {
    const g = guestClamp(base);
    expect(g.brightness).toBe(0.8);
    expect(g.master).toBe(0.7);
    expect(g.strobe).toBe(false);
  });
  it("leaves values already within caps", () => {
    const g = guestClamp({ ...base, brightness: 0.3, master: 0.5, strobe: false });
    expect(g.brightness).toBe(0.3);
    expect(g.master).toBe(0.5);
  });
  it("preserves other fields (hue/pattern)", () => {
    const g = guestClamp(base);
    expect(g.hue).toBe(0.5);
    expect(g.pattern).toBe("solid");
  });
});
