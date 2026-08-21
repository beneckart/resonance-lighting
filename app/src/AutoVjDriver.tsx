import { useEffect } from "react";
import { useTwin } from "./store";
import { ShuffleBag, LOOKS, phraseSeconds } from "./autovj";
import { audioFeatures } from "./audio";

/**
 * Auto-VJ driver (D): when enabled, switches the look (pattern + visualizer + hue)
 * on each phrase boundary — phrase length derived from the detected BPM (× autoBars),
 * or a fixed fallback when there's no audio. Shuffle-bag so every look plays once
 * before repeating. Renders nothing; just drives the store.
 */
export function AutoVj() {
  const auto = useTwin((s) => s.control.autoVj);
  const bars = useTwin((s) => s.control.autoBars);

  useEffect(() => {
    if (!auto) return;
    const bag = new ShuffleBag(LOOKS);
    let timer: number;
    const tick = () => {
      const look = bag.next();
      useTwin.getState().set({ pattern: look.pattern, visualizer: look.visualizer, hue: look.hue });
      const secs = phraseSeconds(audioFeatures.bpm, bars);
      timer = window.setTimeout(tick, Math.max(2500, secs * 1000));
    };
    tick();
    return () => clearTimeout(timer);
  }, [auto, bars]);

  return null;
}
