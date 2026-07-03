import { useEffect } from "react";
import { useTwin } from "./store";

/** Presence sensor → ripple wavefronts. Real hardware (Ben's PRESENCE_SENSING doc):
 *  a per-lantern downward ToF lidar "eye" (VL53L1X class) is the primary candidate —
 *  PIR is RULED OUT (a swaying lantern self-triggers on its own moving warm scene) —
 *  with mmWave (LD2410/LD2420) and mesh-RSSI attenuation as parallel channels. Here
 *  `sensors.motion` (0..1) sets the spawn rate so a passer-by sends a wave of light
 *  pulsing through the tree. Renders nothing. */
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
