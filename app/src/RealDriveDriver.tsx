/** REAL-DRIVE DRIVER — the twin's rendered colors, onto the physical tree.
 *
 *  Headless (renders nothing). Watches the existing net.driveReal toggle
 *  ("📡 drive real" in Controls); while armed it connects a WsBridge to the
 *  cambium middleware daemon and every 200 ms ships the telemetry snapshot —
 *  the SAME "what each light is ACTUALLY doing" data the DataLog shows,
 *  written post-blackout/tameWhite by the render loop — as a "frame".
 *  Cambium clamps, extracts the W channel per fixture class, batches into
 *  NB_DIRECT_FRAME packets and paces to the fleet's 10 Hz render cap.
 *
 *  Uplink: "charging" frames (nodes with battMa > 0) feed
 *  store.solarPanelsCharging(), so the Solar Ray handoff fires off REAL
 *  panels once hardware is connected.
 *
 *  Failure doctrine: if we vanish (tab close, wifi drop), cambium goes
 *  silent and the fleet's own ladder takes over (1 s hold → 3 s autonomous
 *  fallback, never blank). No cleanup we could do beats that, but we still
 *  send drive-off on a tidy unmount.
 */

import { useEffect, useRef } from "react";
import { WsBridge } from "./bridge";
import { telemetry } from "./telemetry";
import { useTwin } from "./store";

const SEND_MS = 200; // telemetry updates at ~5.5 Hz; the fleet renders at 10 Hz

export function RealDriveDriver() {
  const driveReal = useTwin((s) => s.net.driveReal);
  const seq = useRef(0);

  useEffect(() => {
    if (!driveReal) return;
    const bridge = new WsBridge(WsBridge.urlFromLocation());
    let timer: ReturnType<typeof setInterval> | undefined;
    let cancelled = false;

    const unsub = bridge.onUp((f) => {
      if (f.kind === "charging") useTwin.getState().solarPanelsCharging(f.count);
      else if (f.kind === "err") console.warn("[cambium]", f.msg);
    });

    bridge
      .connect()
      .then(() => {
        if (cancelled) { bridge.disconnect(); return; }
        bridge.send({ kind: "drive", on: true });
        timer = setInterval(() => {
          const states = telemetry.states;
          if (!states.length) return;
          bridge.send({
            kind: "frame",
            seq: ++seq.current,
            fixtures: states.map((s) => ({ id: s.id, rgb: s.rgb })),
          });
        }, SEND_MS);
        console.info("[cambium] drive real: streaming to", bridge.transport);
      })
      .catch((e) => {
        console.warn("[cambium]", String(e));
        useTwin.getState().setNet({ driveReal: false }); // disarm: nothing listening
      });

    return () => {
      cancelled = true;
      if (timer) clearInterval(timer);
      bridge.send({ kind: "drive", on: false });
      unsub();
      bridge.disconnect();
    };
  }, [driveReal]);

  return null;
}
