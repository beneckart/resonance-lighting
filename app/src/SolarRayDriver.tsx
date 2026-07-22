import { useEffect } from "react";
import { useTwin } from "./store";

/** SOLAR RAY auto-trigger (Ben + Elliot, 2026-07-21).
 *
 *  "Activate when the last solar panel receives its last juice from the sun" —
 *  the SOLAR HANDOFF: as the real sun sets and the final panel stops harvesting,
 *  the tree takes over AS the sun (pattern "solarray": chandelier core, warm
 *  rays radiating outward). Edge detection + guards live in the store action
 *  (solarPanelsCharging); this driver only reports the harvesting count.
 *
 *  SIM SOURCE (today): the ambient-daylight sensor — panels harvest while
 *  ambient > 0, so dragging daylight to zero IS sunset.
 *  FLEET SOURCE (Justin's two-way bridge, when it lands): count the registry's
 *  nodes reporting battMa > 0 (charging) and call
 *  useTwin.getState().solarPanelsCharging(count) from the frame consumer —
 *  same action, real telemetry, no other changes needed. */
export function SolarRayDriver() {
  const ambient = useTwin((s) => s.sensors.ambient);
  const notify = useTwin((s) => s.solarPanelsCharging);
  useEffect(() => {
    notify(ambient > 0.02 ? 1 : 0); // sim: "the sun is up" = at least one panel harvesting
  }, [ambient, notify]);
  return null;
}
