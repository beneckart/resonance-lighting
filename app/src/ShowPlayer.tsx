import { useEffect, useRef } from "react";
import { useTwin, type Control } from "./store";
import { showById } from "./shows";

/** Drives a running light show. Discrete fields (pattern/colorCycle/order/reverse/
 *  strobe + per-group layers) switch at cue boundaries; CONTINUOUS fields
 *  (brightness/hue/sat/speed/master) are INTERPOLATED between cues every tick so a
 *  long show ramps smoothly over minutes (the Bellagio arc) instead of jumping.
 *  Hue lerps the short way around the wheel. Loops at the end. */
export function ShowPlayer() {
  const activeShow = useTwin((s) => s.activeShow);
  const lastCue = useRef(-1);

  useEffect(() => {
    lastCue.current = -1;
    if (!activeShow) return;

    const apply = () => {
      const st = useTwin.getState();
      const show = showById(st.activeShow);
      if (!show) return;
      let elapsed = performance.now() / 1000 - st.showStartedAt;
      if (elapsed > show.durationS) { // loop the show (one breath cycle) — RESEED so each loop differs
        useTwin.setState({ showStartedAt: performance.now() / 1000, showSeed: Math.random() });
        elapsed = 0;
        lastCue.current = -1;
      }
      // ── per-RUN variation (Elliot: shows must never look identical twice) ──
      // a deterministic seed rotates the whole palette, nudges the pace, and
      // jitters cue timing — the show's ARC is preserved, its surface is fresh.
      const seed = st.showSeed || 0.5;
      const hueRot = (seed - 0.5) * 0.24;         // ±0.12 palette shift — fresh but keeps each show's identity (Cosmos stays cool, Ember warm)
      const speedMul = 0.85 + seed * 0.35;       // 0.85–1.2× pace
      const jit = (k: number) => ((Math.sin((k + 1) * 12.9898 + seed * 78.233) * 43758.5453) % 1); // ±per-cue
      const cues = show.cues;
      // jittered cue times (±6% of the gap to the next cue) — cues drift per run
      const at = (k: number) => {
        const base = cues[k].at;
        if (k === 0 || k >= cues.length) return base;
        const gap = (cues[k + 1]?.at ?? base + 8) - base;
        return base + jit(k) * 0.06 * gap;
      };
      let i = 0;
      for (let k = 0; k < cues.length; k++) { if (at(k) <= elapsed) i = k; else break; }

      // discrete fields + layers: apply once when the cue index changes
      if (i !== lastCue.current) {
        lastCue.current = i;
        const cue = cues[i];
        if (cue.base) {
          const { brightness, hue, sat, speed, master, ...discrete } = cue.base;
          void brightness; void hue; void sat; void speed; void master;
          if (Object.keys(discrete).length) st.set(discrete);
        }
        st.clearLayers();
        if (cue.layers) for (const ly of cue.layers) {
          const nums = st.namedGroups[ly.group] ?? [];
          if (nums.length) st.setLayer(`show:${ly.group}`, nums, ly.control);
        }
      }

      // continuous interpolation of the numeric fields between this cue and the next
      const a = cues[i].base as Partial<Control> | undefined;
      const b = (cues[i + 1] ?? cues[i]).base as Partial<Control> | undefined;
      const span = ((at(i + 1) || (at(i) + 1)) - at(i)) || 1;
      const f = Math.min(1, Math.max(0, (elapsed - at(i)) / span));
      const patch: Partial<Control> = {};
      const lin = (key: "brightness" | "sat" | "speed" | "master") => {
        const av = a?.[key], bv = b?.[key];
        if (av != null && bv != null) patch[key] = av + (bv - av) * f;
        else if (av != null) patch[key] = av;
      };
      lin("brightness"); lin("sat"); lin("speed"); lin("master");
      if (patch.speed != null) patch.speed *= speedMul; // per-run pace
      const ah = a?.hue, bh = b?.hue;
      if (ah != null && bh != null) { let d = bh - ah; d -= Math.round(d); patch.hue = ((ah + d * f + hueRot) % 1 + 1) % 1; }
      else if (ah != null) patch.hue = (ah + hueRot) % 1;
      if (Object.keys(patch).length) st.set(patch);
    };

    apply();
    const iv = setInterval(apply, 200);
    return () => clearInterval(iv);
  }, [activeShow]);

  return null;
}
