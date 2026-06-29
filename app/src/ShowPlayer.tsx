import { useEffect, useRef } from "react";
import { useTwin } from "./store";
import { showById } from "./shows";

/** Drives a running light show: every 250ms it finds the current cue by elapsed
 *  seconds and applies it (base look + per-group layers). Loops at the end. The
 *  twin's colour/brightness slew turns the discrete cue changes into cross-fades. */
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
      if (elapsed > show.durationS) { // loop the show
        useTwin.setState({ showStartedAt: performance.now() / 1000 });
        elapsed = 0;
        lastCue.current = -1;
      }
      let idx = 0;
      for (let i = 0; i < show.cues.length; i++) {
        if (show.cues[i].at <= elapsed) idx = i; else break;
      }
      if (idx === lastCue.current) return;
      lastCue.current = idx;
      const cue = show.cues[idx];
      if (cue.base) st.set(cue.base);
      st.clearLayers();
      if (cue.layers) for (const ly of cue.layers) {
        const nums = st.namedGroups[ly.group] ?? [];
        if (nums.length) st.setLayer(`show:${ly.group}`, nums, ly.control);
      }
    };

    apply();
    const iv = setInterval(apply, 250);
    return () => clearInterval(iv);
  }, [activeShow]);

  return null;
}
