import { describe, it, expect, vi } from "vitest";
import { useTwin } from "./store";

describe("runScript sequencer — wait + preemption (08-15)", () => {
  it("executes lines after `wait` on a timer, not immediately", () => {
    vi.useFakeTimers();
    const st = useTwin.getState();
    st.runScript("bri 0.1\nwait 5\nbri 0.9");
    expect(useTwin.getState().control.brightness).toBeCloseTo(0.1);
    vi.advanceTimersByTime(4900);
    expect(useTwin.getState().control.brightness).toBeCloseTo(0.1); // still waiting
    vi.advanceTimersByTime(200);
    expect(useTwin.getState().control.brightness).toBeCloseTo(0.9);
    vi.useRealTimers();
  });

  it("a NEW script preempts a pending one — the operator's latest word wins", () => {
    vi.useFakeTimers();
    const st = useTwin.getState();
    st.runScript("bri 0.1\nwait 5\nbri 0.9");
    st.runScript("bri 0.5"); // Elliot changes his mind mid-sequence
    vi.advanceTimersByTime(10_000);
    // the old script's tail (bri 0.9) must NEVER fire
    expect(useTwin.getState().control.brightness).toBeCloseTo(0.5);
    vi.useRealTimers();
  });

  it("multi-step timed sequence lands each step in order (the walk-under brief)", () => {
    vi.useFakeTimers();
    const st = useTwin.getState();
    st.runScript("all color red\nwait 2\nall color purple\nwait 2\noff\nwait 1\nclear\npattern ripples");
    vi.advanceTimersByTime(1900);
    vi.advanceTimersByTime(200); // t=2.1 → purple applied
    vi.advanceTimersByTime(2000); // t=4.1 → off
    vi.advanceTimersByTime(1000); // t=5.1 → clear + ripples
    expect(useTwin.getState().control.pattern).toBe("ripples");
    expect(Object.keys(useTwin.getState().overrides).length).toBe(0);
    vi.useRealTimers();
  });
});
