/** REAL-DRIVE DRIVER — the twin's rendered colors, onto the physical tree.
 *
 *  (Justin's design, cambium-ws-bridge branch; transport swapped onto
 *  CambiumBridge so real-drive gets reconnect/backoff, the identify null-mac
 *  fix, and hb translation for free.)
 *
 *  Headless (renders nothing). Watches the existing net.driveReal toggle
 *  ("📡 drive real" in Controls); while armed it connects to the cambium
 *  middleware daemon and every 200 ms ships the telemetry snapshot — the
 *  SAME "what each light is ACTUALLY doing" data the DataLog shows, written
 *  post-blackout/tameWhite by the render loop — as a "frame". Cambium
 *  clamps, extracts the W channel per fixture class, batches into
 *  NB_DIRECT_FRAME packets and paces to the fleet's 10 Hz render cap.
 *
 *  Uplink: "charging" meta (nodes with battMa > 0) feeds
 *  store.solarPanelsCharging(), so the Solar Ray handoff fires off REAL
 *  panels once hardware is connected.
 *
 *  Failure doctrine: if we vanish (tab close, wifi drop), cambium goes
 *  silent and the fleet's own ladder takes over (1 s hold → 3 s autonomous
 *  fallback, never blank). No cleanup we could do beats that, but we still
 *  send drive-off on a tidy unmount. If the daemon was never there at all,
 *  the toggle disarms — nothing is listening. */

import { useEffect, useRef } from "react";
import { CambiumBridge } from "./cambium";
import { telemetry } from "./telemetry";
import { useTwin } from "./store";

const SEND_MS = 200; // telemetry updates at ~5.5 Hz; the fleet renders at 10 Hz

export function RealDriveDriver() {
  const driveReal = useTwin((s) => s.net.driveReal);
  const bridgeRef = useRef<CambiumBridge | null>(null);

  useEffect(() => {
    if (!driveReal) return;
    const bridge = new CambiumBridge({ url: CambiumBridge.urlFromLocation() });
    bridgeRef.current = bridge;
    let timer: ReturnType<typeof setInterval> | undefined;
    let cancelled = false;

    // night-gate relay: FleetPanel's 🌙 buttons reach the fleet through THIS
    // bridge when the panel has no cambium connection of its own (runbook
    // flow: only the 📡 toggle is armed)
    const onNight = (e: Event) => {
      const mode = (e as CustomEvent<{ mode: 0 | 1 | 2 }>).detail.mode;
      bridge.send({ kind: "night", mode, mac: null });
    };
    window.addEventListener("resonance:cambium-night", onNight);

    const unsubMeta = bridge.onMeta((m) => {
      if (m.kind === "charging") {
        useTwin.getState().solarPanelsCharging(Number(m.payload.count ?? 0));
      } else if (m.kind === "err") {
        console.warn("[cambium]", m.payload.msg);
      } else if (m.kind === "open") {
        // (re)armed — reconnect also lands here, so drive survives daemon restarts
        bridge.send({ kind: "drive", on: true });
      }
    });

    bridge
      .connect()
      .then(() => {
        if (cancelled) { bridge.disconnect(); return; }
        timer = setInterval(() => {
          const states = telemetry.states;
          if (!states.length || !bridge.connected()) return;
          bridge.sendFrame(states.map((s) => ({ id: s.id, rgb: s.rgb })));
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
      window.removeEventListener("resonance:cambium-night", onNight);
      bridge.send({ kind: "drive", on: false });
      unsubMeta();
      bridge.disconnect();
      bridgeRef.current = null;
    };
  }, [driveReal]);

  return null;
}
