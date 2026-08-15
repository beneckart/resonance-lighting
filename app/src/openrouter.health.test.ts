import { describe, it, expect, beforeEach } from "vitest";
import { checkRemote, recordAiTurn, aiLogSnapshot, clearAiLog, subscribeAiLog } from "./openrouter";

/**
 * The AI operator's diagnostic channel (Elliot 2026-08-15: "we need to enable
 * the AI built into the app to allow communication so we know if it is working
 * or if there is any errors").
 *
 * A diagnostic nobody has watched misdiagnose is not a diagnostic. Each branch
 * below is a failure an operator can actually hit on the playa, and the test
 * asserts it gets the RIGHT name — "no credit" and "bad key" want different
 * actions, and reporting either as a generic failure wastes a trip up the tree.
 */

const KEY = ["sk", "or", "v1"].join("-") + "-" + "PROBE".repeat(4);

function fakeFetch(status: number, body: unknown): typeof fetch {
  return (async () => ({
    ok: status >= 200 && status < 300,
    status,
    json: async () => body,
  })) as unknown as typeof fetch;
}

const reply = (text: string) => ({ choices: [{ message: { content: text } }] });

describe("checkRemote", () => {
  it("reports no-key without making a request", async () => {
    const h = await checkRemote({ apiKey: undefined, fetchImpl: fakeFetch(200, reply("all color blue")) });
    // apiKey undefined falls through to loadKey(); jsdom localStorage is empty here
    expect(h.code).toBe("no-key");
    expect(h.ok).toBe(false);
    expect(h.latencyMs).toBeNull(); // never went out
  });

  it("reports ok and the commands when the whole path works", async () => {
    const h = await checkRemote({ apiKey: KEY, fetchImpl: fakeFetch(200, reply("all color blue")) });
    expect(h.ok).toBe(true);
    expect(h.code).toBe("ok");
    expect(h.commands).toEqual(["all color blue"]);
    expect(h.latencyMs).not.toBeNull();
  });

  it.each([
    [401, "bad-key"],
    [403, "bad-key"],
    [402, "no-credit"],
    [429, "rate-limited"],
    [404, "model-missing"],
    [500, "no-network"],
    [503, "no-network"],
  ])("maps HTTP %i to %s", async (status, code) => {
    const h = await checkRemote({ apiKey: KEY, fetchImpl: fakeFetch(status, {}) });
    expect(h.code).toBe(code);
    expect(h.ok).toBe(false);
    expect(h.detail.length).toBeGreaterThan(0); // always actionable text
  });

  it("distinguishes bad-output from bad-key — the failure Elliot actually hit", async () => {
    // a 200 with prose is a WORKING key whose answer the shape gate drops;
    // calling that "bad key" would send the operator to re-paste a fine key
    const h = await checkRemote({ apiKey: KEY, fetchImpl: fakeFetch(200, reply("Sure! I'd make it blue.")) });
    expect(h.code).toBe("bad-output");
    expect(h.ok).toBe(false);
  });

  it("accepts a numbered-list answer — the list-marker fix applies to the probe too", async () => {
    const h = await checkRemote({
      apiKey: KEY,
      fetchImpl: fakeFetch(200, reply("1. all color blue\n2. all bri 0.8")),
    });
    expect(h.ok).toBe(true);
    expect(h.commands).toEqual(["all color blue", "all bri 0.8"]);
  });

  it("reports no-network when fetch throws, and never leaks the key", async () => {
    const throwing = (async () => { throw new Error(`connect failed using ${KEY}`); }) as unknown as typeof fetch;
    const h = await checkRemote({ apiKey: KEY, fetchImpl: throwing });
    expect(h.code).toBe("no-network");
    expect(h.detail).not.toContain(KEY);
    expect(h.detail).toContain("sk-or-***");
  });
});

describe("AI activity log", () => {
  beforeEach(() => clearAiLog());

  it("records newest-first and notifies subscribers", () => {
    let notified = 0;
    const off = subscribeAiLog(() => { notified++; });
    recordAiTurn({ at: 1, said: "blue", source: "openrouter", commands: ["all color blue"] });
    recordAiTurn({ at: 2, said: "off", source: "offline", commands: ["off"] });
    expect(aiLogSnapshot()[0].said).toBe("off");
    expect(notified).toBe(2);
    off();
  });

  it("redacts a key that reaches it through an error string", () => {
    recordAiTurn({ at: 3, said: "x", source: "offline", commands: [], error: `boom ${KEY}` });
    expect(aiLogSnapshot()[0].error).not.toContain(KEY);
  });

  it("stays bounded so a long show can't grow it without limit", () => {
    for (let i = 0; i < 60; i++) {
      recordAiTurn({ at: i, said: `s${i}`, source: "offline", commands: [] });
    }
    expect(aiLogSnapshot().length).toBeLessThanOrEqual(25);
    expect(aiLogSnapshot()[0].said).toBe("s59"); // newest kept
  });
});
