import { useEffect } from "react";
import { useTwin } from "./store";
import { nextCueIndex } from "./cues";

/**
 * Cue timeline (F2): when playing, steps through saved cues in order, holding each for
 * stepSecs, looping. Recalls each cue via the store. Renders nothing.
 */
export function TimelineDriver() {
  const playing = useTwin((s) => s.timeline.playing);
  const stepSecs = useTwin((s) => s.timeline.stepSecs);

  useEffect(() => {
    if (!playing) return;
    if (useTwin.getState().cues.length === 0) return;
    let i = -1;
    const tick = () => {
      const cues = useTwin.getState().cues;
      if (cues.length === 0) return;
      i = nextCueIndex(i, cues.length);
      useTwin.getState().recallCue(cues[i].id);
    };
    tick();
    const id = window.setInterval(tick, Math.max(1000, stepSecs * 1000));
    return () => clearInterval(id);
  }, [playing, stepSecs]);

  return null;
}
