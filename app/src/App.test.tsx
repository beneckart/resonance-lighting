import { describe, it, expect } from "vitest";
import { App } from "./App";

// Smoke test: the App component is defined and is a function component.
// (R3F's <Canvas> needs WebGL, which jsdom lacks — full render is verified
// visually via the Playwright screenshot harness, not here.)
describe("App", () => {
  it("is a defined component", () => {
    expect(typeof App).toBe("function");
  });
});
