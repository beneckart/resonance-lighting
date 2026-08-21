import { describe, it, expect } from "vitest";
import { interpret, targetFor } from "./llm";

describe("LLM operator — interpret(NL) → commands", () => {
  it("maps target words to zones", () => {
    expect(targetFor("light up the canopy")).toBe("zone high");
    expect(targetFor("warm the trunk")).toBe("zone low");
    expect(targetFor("the middle ring")).toBe("zone mid");
    expect(targetFor("every other light")).toBe("every 2");
    expect(targetFor("the whole tree")).toBe("all");
  });

  it("'make the canopy pulse blue and fast' → pattern + zoned colour + speed", () => {
    const r = interpret("make the canopy pulse blue and fast");
    expect(r.commands).toContain("pattern breathe");
    expect(r.commands).toContain("zone high color blue");
    expect(r.commands).toContain("speed 2.4");
  });

  it("'slow rainbow over the whole tree' → spectrum + all + slow", () => {
    const r = interpret("slow rainbow over the whole tree");
    expect(r.commands).toContain("pattern spectrum");
    expect(r.commands).toContain("speed 0.5");
  });

  it("'turn off the top' → zoned blackout", () => {
    expect(interpret("turn off the top").commands).toContain("zone high off");
  });

  it("'dim vivid red fire' → ember + colour + bri + sat", () => {
    const r = interpret("dim vivid red fire");
    expect(r.commands).toContain("pattern ember");
    expect(r.commands).toContain("all color red");
    expect(r.commands).toContain("bri 0.4");
    expect(r.commands).toContain("sat 1");
  });

  it("recognises explicit pattern ids + the three-colour synonym", () => {
    expect(interpret("run godray").commands).toContain("pattern godray");
    expect(interpret("do the three colour dance").commands).toContain("pattern tricolor");
  });

  it("no intent → empty commands with a note", () => {
    const r = interpret("hello there");
    expect(r.commands).toHaveLength(0);
    expect(r.note).toMatch(/no actionable/);
  });

  it("only emits CSS-valid colour names (command.ts must parse them)", () => {
    // 'amber' is not CSS-valid and must NOT be emitted
    expect(interpret("amber glow").commands.some((c) => c.includes("amber"))).toBe(false);
  });
});
