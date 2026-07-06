import { describe, expect, it } from "vitest";
import {
  compileRules, decodeRules, evalRules, MAX_BYTES, parseRules, PATTERNS,
  RULE_PRESETS, type SensorInputs,
} from "./rules";
import { MockBridge, type UpFrame } from "./bridge";

const INPUTS: SensorInputs = { hour: 21, soc: 80, presence: 0, sound: 0, supply: 0, mode: 0 };

describe("parseRules (the DSL)", () => {
  it("parses conditions, actions, defaults and comments", () => {
    const r = parseRules([
      "# night program",
      "when hour >= 22 and soc < 30 -> pattern=ember bri=40",
      "when presence > 0 -> pattern=ripple bri=255 speed=3",
      "-> pattern=breathe bri=120",
    ].join("\n"));
    expect(r.ok).toBe(true);
    expect(r.ruleset!.rules).toHaveLength(3);
    expect(r.ruleset!.rules[0].when).toHaveLength(2);
    expect(r.ruleset!.rules[2].when).toHaveLength(0); // default
  });
  it("reports useful errors with line numbers", () => {
    const r = parseRules([
      "when hour >= 22 pattern=ember", // missing ->
      "when windspeed > 5 -> pattern=off", // unknown sensor
      "when soc < 20 -> pattern=disco", // unknown pattern
      "when soc < 20 -> bri=40", // missing pattern
    ].join("\n"));
    expect(r.ok).toBe(false);
    expect(r.errors).toHaveLength(5); // 4 line errors + "no rules survived"
    expect(r.errors[0]).toContain("line 1");
    expect(r.errors[1]).toContain("windspeed");
    expect(r.errors[2]).toContain("disco");
    expect(r.errors[3]).toContain("needs pattern");
    expect(r.errors[4]).toContain("no rules");
  });
  it("rejects an empty program", () => {
    expect(parseRules("# nothing\n").ok).toBe(false);
  });
  it("every shipped preset parses", () => {
    for (const [name, text] of Object.entries(RULE_PRESETS)) {
      const r = parseRules(text);
      expect(r.ok, `${name}: ${r.errors.join("; ")}`).toBe(true);
    }
  });
});

describe("compile / decode (the wire bytes)", () => {
  it("round-trips exactly", () => {
    const r = parseRules(RULE_PRESETS["night-saver"], 7);
    const bytes = compileRules(r.ruleset!);
    const back = decodeRules(bytes);
    expect(back).toEqual(r.ruleset);
  });
  it("negative values survive the i16 encoding", () => {
    const r = parseRules("when soc > -5 -> pattern=off bri=0");
    const back = decodeRules(compileRules(r.ruleset!));
    expect(back.rules[0].when[0].value).toBe(-5);
  });
  it("a full program fits ONE ESP-NOW frame", () => {
    for (const text of Object.values(RULE_PRESETS)) {
      expect(compileRules(parseRules(text).ruleset!).length).toBeLessThanOrEqual(MAX_BYTES);
    }
  });
  it("parser rejects a program too big for one frame", () => {
    const big = Array.from({ length: 16 }, () =>
      "when hour >= 1 and soc < 99 and presence = 0 and sound < 9 -> pattern=ember bri=40",
    ).join("\n");
    const r = parseRules(big);
    expect(r.ok).toBe(false);
    expect(r.errors.join(" ")).toContain("one ESP-NOW frame");
  });
});

describe("evalRules (what firmware runs)", () => {
  const rs = parseRules(RULE_PRESETS["night-saver"]).ruleset!;
  it("first match wins", () => {
    const hit = evalRules(rs, { ...INPUTS, hour: 3, presence: 0 });
    expect(hit!.index).toBe(0);
    expect(hit!.action.pattern).toBe("ember");
  });
  it("low battery beats presence order-dependently (as written)", () => {
    const hit = evalRules(rs, { ...INPUTS, soc: 10, presence: 5 });
    expect(hit!.action.pattern).toBe("ember"); // soc rule is earlier
  });
  it("falls through to the default", () => {
    const hit = evalRules(rs, { ...INPUTS });
    expect(hit!.action.pattern).toBe("breathe");
    expect(hit!.index).toBe(rs.rules.length - 1);
  });
  it("returns null when nothing matches and there is no default", () => {
    const nodefault = parseRules("when sound > 5 -> pattern=chase bri=255").ruleset!;
    expect(evalRules(nodefault, INPUTS)).toBeNull();
  });
});

describe("fleet integration (one broadcast reprograms everyone)", () => {
  const SPECS = Array.from({ length: 6 }, (_, i) => ({ mac: `M${i}`, role: "downlight" }));
  it("nodes run the rules locally; env changes flip behavior with instant events", async () => {
    const b = new MockBridge(SPECS, 3);
    await b.connect();
    const frames: UpFrame[] = [];
    b.onUp((f) => frames.push(f));
    const rs = parseRules(RULE_PRESETS["night-saver"], 42).ruleset!;
    b.send({ kind: "ruleset", epoch: 42, bytes: [...compileRules(rs)] });
    b.env.hour = 21;
    b.tick(300);
    // everyone settles on the default (breathe)
    const hb1 = frames.filter((f) => f.kind === "hb");
    b.tick(1000);
    const settled = frames.filter((f) => f.kind === "hb").slice(-3);
    expect(settled.every((f) => f.kind === "hb" && f.caState === PATTERNS.breathe)).toBe(true);
    // a person shows up → ripple, announced by instant events (not polling)
    const before = frames.length;
    b.env.presence = 5;
    b.tick(100);
    const evts = frames.slice(before).filter((f) => f.kind === "evt" && f.event === "state");
    expect(evts.length).toBe(6); // every node switched, instantly
    expect(evts.every((f) => f.kind === "evt" && f.value === PATTERNS.ripple)).toBe(true);
    void hb1;
  });
});
