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
      if (elapsed > show.durationS) { // loop the show (one breath cycle)
        useTwin.setState({ showStartedAt: performance.now() / 1000 });
        elapsed = 0;
        lastCue.current = -1;
      }
      const cues = show.cues;
      let i = 0;
      for (let k = 0; k < cues.length; k++) { if (cues[k].at <= elapsed) i = k; else break; }

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
      const span = ((cues[i + 1]?.at ?? cues[i].at + 1) - cues[i].at) || 1;
      const f = Math.min(1, Math.max(0, (elapsed - cues[i].at) / span));
      const patch: Partial<Control> = {};
      const lin = (key: "brightness" | "sat" | "speed" | "master") => {
        const av = a?.[key], bv = b?.[key];
        if (av != null && bv != null) patch[key] = av + (bv - av) * f;
        else if (av != null) patch[key] = av;
      };
      lin("brightness"); lin("sat"); lin("speed"); lin("master");
      const ah = a?.hue, bh = b?.hue;
      if (ah != null && bh != null) { let d = bh - ah; d -= Math.round(d); patch.hue = ((ah + d * f) % 1 + 1) % 1; }
      else if (ah != null) patch.hue = ah;
      if (Object.keys(patch).length) st.set(patch);
    };

    apply();
    const iv = setInterval(apply, 200);
    return () => clearInterval(iv);
  }, [activeShow]);

  return null;
}
