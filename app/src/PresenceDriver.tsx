import { useEffect } from "react";
import { useTwin } from "./store";

/** Presence / motion sensor → ripple wavefronts. On real hardware a PIR/mmWave
 *  fires on movement; here `sensors.motion` (0..1) sets the spawn rate so a
 *  passer-by sends a wave of light pulsing through the tree. Renders nothing. */
export function PresenceDriver() {
  const motion = useTwin((s) => s.sensors.motion);
  useEffect(() => {
    if (motion < 0.08) return;
    const ping = useTwin.getState().pingPresence;
    const interval = Math.max(280, (1.6 - motion) * 1000); // more motion → faster ripples
    ping(); // immediate wavefront on motion onset
    const id = window.setInterval(() => ping(), interval);
    return () => clearInterval(id);
  }, [motion]);
  return null;
}
