import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { act } from "react";
import { createRoot, type Root } from "react-dom/client";
import { AppBoundary } from "./AppBoundary";

/**
 * An error boundary you have never seen catch anything is not a safety net, it
 * is a comment. These tests actually throw inside a child and assert the app
 * degrades to a recoverable screen rather than a white one — plus that SAFE MODE
 * clears the UI state that can rot while preserving the two things an operator
 * cannot cheaply re-enter in the field (the API key and the calibration map).
 */

function Boom(): never { throw new Error("kaboom from a panel"); }

let host: HTMLDivElement;
let root: Root;
let errSpy: ReturnType<typeof vi.spyOn>;

beforeEach(() => {
  host = document.createElement("div");
  document.body.appendChild(host);
  root = createRoot(host);
  // React logs caught render errors; keep the suite output readable
  errSpy = vi.spyOn(console, "error").mockImplementation(() => {});
});

afterEach(() => {
  act(() => root.unmount());
  host.remove();
  errSpy.mockRestore();
  localStorage.clear();
});

describe("AppBoundary", () => {
  it("renders its children when nothing throws", () => {
    act(() => { root.render(<AppBoundary><p>the console</p></AppBoundary>); });
    expect(host.textContent).toContain("the console");
  });

  it("catches a child render error instead of white-screening", () => {
    act(() => { root.render(<AppBoundary><Boom /></AppBoundary>); });
    expect(host.textContent).toContain("The controller hit an error");
    // and it must offer a way OUT, not just an apology
    const labels = Array.from(host.querySelectorAll("button")).map((b) => b.textContent || "");
    expect(labels.some((l) => /reload/i.test(l))).toBe(true);
    expect(labels.some((l) => /safe mode/i.test(l))).toBe(true);
  });

  it("surfaces the underlying message so it can be reported", () => {
    act(() => { root.render(<AppBoundary><Boom /></AppBoundary>); });
    expect(host.textContent).toContain("kaboom from a panel");
  });

  it("SAFE MODE clears rot-prone UI state but preserves key + calibration", () => {
    localStorage.setItem("ui.mode", "sound");
    localStorage.setItem("dock.order.v2.lightshow", "[\"broken\"]");
    localStorage.setItem("touch.collapsed", "{}");
    localStorage.setItem("resonance.openrouter.key", "KEEP-ME");
    localStorage.setItem("resonance.calibration", "KEEP-ME-TOO");

    act(() => { root.render(<AppBoundary><Boom /></AppBoundary>); });

    // jsdom has no navigation; stub reload so the handler can run to completion
    const reload = vi.fn();
    Object.defineProperty(window, "location", {
      value: { ...window.location, reload }, writable: true, configurable: true,
    });

    const safe = Array.from(host.querySelectorAll("button")).find((b) => /safe mode/i.test(b.textContent || ""));
    expect(safe, "safe-mode button exists").toBeTruthy();
    act(() => { safe!.dispatchEvent(new MouseEvent("click", { bubbles: true })); });

    expect(localStorage.getItem("ui.mode")).toBeNull();
    expect(localStorage.getItem("dock.order.v2.lightshow")).toBeNull();
    expect(localStorage.getItem("touch.collapsed")).toBeNull();
    // the two things worth protecting
    expect(localStorage.getItem("resonance.openrouter.key")).toBe("KEEP-ME");
    expect(localStorage.getItem("resonance.calibration")).toBe("KEEP-ME-TOO");
    expect(reload).toHaveBeenCalled();
  });
});
