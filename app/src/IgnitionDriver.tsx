import { useEffect } from "react";
import { useTwin } from "./store";
import { seedLife } from "./field";

/** GAME OF LIGHT ignition. When the tree senses its first visitor (a tap in standby),
 *  it runs a short cinematic to announce interactive mode, exactly as Elliot described:
 *    off1  — all lights snap OFF (it noticed you)
 *    flash — a quick flourish blooms up from the trunk (entering interactive mode)
 *    off2  — everything goes dark again
 *    live  — interactive: dark at rest, visitors drop nodes that play Game of Life
 *  Timings are wall-clock; the store owns blackout per phase, this just advances it. */
export function IgnitionDriver() {
  const phase = useTwin((s) => s.gol.phase);
  const golSetPhase = useTwin((s) => s.golSetPhase);
  useEffect(() => {
    if (phase === "off1") {
      const t = setTimeout(() => golSetPhase("flash"), 700);
      return () => clearTimeout(t);
    }
    if (phase === "flash") {
      // flourish: a bright colour-cycling bloom RISING up the whole tree (bottom→top),
      // three waves in three colours, so it clearly reads as "entering interactive mode".
      const fx = useTwin.getState().fixtures;
      const timers: ReturnType<typeof setTimeout>[] = [];
      if (fx.length) {
        const byH = fx.map((f, i) => ({ i, h: f.heightT })).sort((a, b) => a.h - b.h);
        const third = Math.ceil(byH.length / 3) || 1;
        const wave = (band: { i: number }[], hue: number, delay: number) =>
          timers.push(setTimeout(() => band.forEach((o) => seedLife([o.i], { hops: 1, hue, bri: 2.5, ttl: 1.4 })), delay));
        wave(byH.slice(0, third), 0.02, 0);          // trunk/base → red
        wave(byH.slice(third, 2 * third), 0.12, 380); // mid → amber
        wave(byH.slice(2 * third), 0.83, 760);        // canopy → magenta
      }
      timers.push(setTimeout(() => golSetPhase("off2"), 2200));
      return () => timers.forEach(clearTimeout);
    }
    if (phase === "off2") {
      const t = setTimeout(() => golSetPhase("live"), 600);
      return () => clearTimeout(t);
    }
  }, [phase, golSetPhase]);
  return null;
}
