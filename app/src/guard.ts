import type { Control } from "./store";

/**
 * Guest-DJ scope (C3): when a controller is handed to a guest, clamp the show so it
 * stays safe — cap brightness + master, and force strobe off (no whiteout/blackout
 * abuse). Pure; applied to the effective control each frame.
 */
export function guestClamp(c: Control): Control {
  return {
    ...c,
    brightness: Math.min(c.brightness, 0.8),
    master: Math.min(c.master, 0.7),
    strobe: false,
  };
}
