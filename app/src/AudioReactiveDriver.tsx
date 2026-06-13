import { useEffect } from "react";
import { useTwin } from "./store";
import { audioFeatures } from "./audio";
import { reactiveSpeed } from "./aivj";

/** When control.audioSpeed is ON, continuously drives the pattern speed from the
 *  live music (energy + BPM + drop) so the tree moves faster in hot sections and
 *  eases in quiet ones. Off → manual speed (slider/jog) is untouched. Renders
 *  nothing. */
export function AudioReactiveDriver() {
  const on = useTwin((s) => s.control.audioSpeed);
  useEffect(() => {
    if (!on) return;
    const set = useTwin.getState().set;
    const id = window.setInterval(() => {
      if (audioFeatures.active) set({ speed: reactiveSpeed(audioFeatures) });
    }, 80);
    return () => clearInterval(id);
  }, [on]);
  return null;
}
