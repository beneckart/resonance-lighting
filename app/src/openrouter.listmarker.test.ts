import { describe, it, expect } from "vitest";
import { stripListMarkerForTest as strip } from "./openrouter";
import { isCommandLike } from "./command";

/**
 * Regression for the complexity-correlated silent fallback (Elliot 2026-08-15:
 * "the simple commands worked but the more complex ones did not").
 *
 * `isCommandLike` tests only the first word, so ONE bare command line passes and
 * a numbered/bulleted list — the natural shape of a model's answer to a
 * compositional request — fails every line, empties `commands`, and drops the
 * whole turn to the offline interpreter with no visible reason.
 */
describe("stripListMarker", () => {
  const cmd = "all pattern spiral";

  it("a bare command already passed the gate — that is why simple requests worked", () => {
    expect(isCommandLike(cmd)).toBe(true);
  });

  it.each([
    ["1. ", "numbered with a dot"],
    ["2) ", "numbered with a paren"],
    ["- ", "hyphen bullet"],
    ["* ", "asterisk bullet"],
    ["• ", "unicode bullet"],
    ["Step 1: ", "step prefix"],
    ["  3. ", "indented numbered"],
  ])("%s (%s) is dropped by the gate but survives after stripping", (prefix) => {
    const line = prefix + cmd;
    expect(isCommandLike(line)).toBe(false); // the bug
    expect(isCommandLike(strip(line))).toBe(true); // the fix
  });

  it("a whole multi-step answer survives, not just the first line", () => {
    const modelAnswer = [
      "1. all pattern warmcool",
      "2. all speed 0.4",
      "3. all bri 0.6",
    ];
    const kept = modelAnswer.map(strip).filter(isCommandLike);
    expect(kept).toEqual(["all pattern warmcool", "all speed 0.4", "all bri 0.6"]);
  });

  it("does NOT loosen the vocabulary gate — prose is still rejected", () => {
    // stripping a marker must never turn non-vocabulary into a runnable command
    for (const prose of [
      "1. Sure! Here's how I'd do that:",
      "- First we set the mood",
      "* rm -rf /",
      "2) console.log('hi')",
    ]) {
      expect(isCommandLike(strip(prose))).toBe(false);
    }
  });

  it("leaves an unmarked line untouched", () => {
    expect(strip(cmd)).toBe(cmd);
    expect(strip("light 1,7,17 color blue")).toBe("light 1,7,17 color blue");
  });

  it("does not eat a hyphen that is part of the command", () => {
    // "range 0-23" must survive — the marker pattern requires trailing whitespace
    expect(strip("range 0-23 color red")).toBe("range 0-23 color red");
  });
});
