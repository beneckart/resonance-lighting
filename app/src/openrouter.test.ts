import { describe, it, expect } from "vitest";
import {
  interpretRemote, grammarPrompt, redact, stripFencesForTest,
  DEFAULT_MODEL, ENDPOINT,
} from "./openrouter";
import { isCommandLike, COMMAND_HEADS } from "./command";
import { PATTERN_IDS } from "./store";

/** A fake fetch that answers with whatever the model "said". No network, ever. */
function fakeFetch(content: string, status = 200): typeof fetch {
  return (async () => ({
    ok: status >= 200 && status < 300,
    status,
    json: async () => ({ choices: [{ message: { content } }] }),
  })) as unknown as typeof fetch;
}

/** ASSEMBLED, never written as a literal: a credential-shaped string in source
 *  trips the repo's secret scanner (it did — 2026-08-15), and a scanner that
 *  cries wolf over test fixtures is a scanner people start ignoring. */
const KEY = ["sk", "or", "v1", "notarealkey000"].join("-");

describe("the shape gate (isCommandLike)", () => {
  it("accepts every head the parser actually implements", () => {
    for (const head of COMMAND_HEADS) expect(isCommandLike(head)).toBe(true);
  });

  it("REJECTS the things a model actually emits when it misbehaves", () => {
    // this is the test that matters — each has a real-world analogue
    expect(isCommandLike("Sure! Here are the commands:")).toBe(false);
    expect(isCommandLike("I'm sorry, I can't help with that.")).toBe(false);
    expect(isCommandLike("```")).toBe(false);
    expect(isCommandLike("rm -rf /")).toBe(false);
    expect(isCommandLike("console.log('hi')")).toBe(false);
    expect(isCommandLike("")).toBe(false);
  });

  it("is case- and whitespace-insensitive, because models are", () => {
    expect(isCommandLike("  PATTERN spiral  ")).toBe(true);
    expect(isCommandLike("Zone High Off")).toBe(true);
  });
});

describe("grammarPrompt", () => {
  it("injects the LIVE pattern ids so the prompt cannot drift from the app", () => {
    const p = grammarPrompt();
    // a pattern added to the registry must appear without editing this file
    for (const id of PATTERN_IDS.slice(0, 8)) expect(p).toContain(id);
    // the trained surface (08-15): shows, themes, cues, fleet ops, sequencing
    expect(p).toContain("show solarray");
    expect(p).toContain("theme ember");
    expect(p).toContain("cue save");
    expect(p).toContain("blink F2BE20");
    expect(p).toContain("wait 6");
  });
});

describe("interpretRemote", () => {
  it("uses the remote answer when the model behaves", async () => {
    const r = await interpretRemote("make it a slow sunrise", {
      apiKey: KEY,
      fetchImpl: fakeFetch("pattern rising\nspeed 0.5\nall color orange"),
    });
    expect(r.source).toBe("openrouter");
    expect(r.commands).toEqual(["pattern rising", "speed 0.5", "all color orange"]);
    expect(r.error).toBeUndefined();
  });

  it("strips a markdown fence the model was told not to add", async () => {
    const r = await interpretRemote("blue please", {
      apiKey: KEY,
      fetchImpl: fakeFetch("```\nall color blue\n```"),
    });
    expect(r.source).toBe("openrouter");
    expect(r.commands).toEqual(["all color blue"]);
  });

  it("DROPS prose mixed into a good answer, keeping only the commands", async () => {
    const r = await interpretRemote("go red", {
      apiKey: KEY,
      fetchImpl: fakeFetch("Sure! Here you go:\nall color red\nHope that helps!"),
    });
    expect(r.source).toBe("openrouter");
    expect(r.commands).toEqual(["all color red"]);
  });

  // ---- every one of these must still reach the tree, via the offline path ----

  it("falls back OFFLINE when there is no key at all", async () => {
    const r = await interpretRemote("make it red", { apiKey: "" });
    expect(r.source).toBe("offline");
    expect(r.commands).toContain("all color red"); // deterministic path still works
  });

  it("falls back OFFLINE on an HTTP error (bad key / no credit / rate limit)", async () => {
    for (const status of [401, 402, 429, 500]) {
      const r = await interpretRemote("make it red", {
        apiKey: KEY,
        fetchImpl: fakeFetch("", status),
      });
      expect(r.source).toBe("offline");
      expect(r.error).toContain(String(status));
      expect(r.commands).toContain("all color red");
    }
  });

  it("falls back OFFLINE when the network throws (Starlink drop)", async () => {
    const boom = (async () => {
      throw new Error("Failed to fetch");
    }) as unknown as typeof fetch;
    const r = await interpretRemote("make it red", { apiKey: KEY, fetchImpl: boom });
    expect(r.source).toBe("offline");
    expect(r.error).toBe("Failed to fetch");
    expect(r.commands).toContain("all color red");
  });

  it("falls back OFFLINE when the model returns ONLY prose", async () => {
    const r = await interpretRemote("make it red", {
      apiKey: KEY,
      fetchImpl: fakeFetch("I'm not sure what you mean by that."),
    });
    expect(r.source).toBe("offline");
    expect(r.commands).toContain("all color red");
  });

  it("never lets a request die: no interpreter path throws", async () => {
    const nasty = (async () => ({
      ok: true,
      status: 200,
      json: async () => ({ unexpected: "shape" }), // no choices at all
    })) as unknown as typeof fetch;
    const r = await interpretRemote("dim the tree", { apiKey: KEY, fetchImpl: nasty });
    expect(r.source).toBe("offline");
    expect(r.commands.length).toBeGreaterThan(0);
  });
});

describe("secret hygiene", () => {
  it("redacts an OpenRouter key out of any text that transits the module", () => {
    const leaked = `request failed with Authorization: Bearer ${KEY} oh no`;
    expect(redact(leaked)).not.toContain(KEY);
    expect(redact(leaked)).toContain("sk-or-***");
  });

  it("never puts the key in the error surfaced to the UI", async () => {
    const boom = (async () => {
      throw new Error(`bad request for key ${KEY}`);
    }) as unknown as typeof fetch;
    const r = await interpretRemote("red", { apiKey: KEY, fetchImpl: boom });
    expect(r.error).toBeDefined();
    expect(r.error).not.toContain(KEY);
  });
});

describe("constants", () => {
  it("points at OpenRouter and defaults to a current Claude model", () => {
    expect(ENDPOINT).toBe("https://openrouter.ai/api/v1/chat/completions");
    expect(DEFAULT_MODEL).toContain("anthropic/");
  });
});

describe("stripFences", () => {
  it("leaves unfenced text untouched", () => {
    expect(stripFencesForTest("all color blue")).toBe("all color blue");
  });
});
