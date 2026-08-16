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

function clientName(): string {
  const ua = navigator.userAgent;
  const dev = /iPhone|iPad/.test(ua) ? "iphone" : /Android/.test(ua) ? "android"
    : /Windows/.test(ua) ? "windows" : /Mac/.test(ua) ? "mac" : "device";
  return `twin-${dev}`;
}

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
        useTwin.getState().solarPanelsCharging(Number(m.payload.count ?? 0), "fleet");
      } else if (m.kind === "err") {
        console.warn("[cambium]", m.payload.msg);
      } else if (m.kind === "open") {
        // (re)armed — reconnect also lands here, so drive survives daemon restarts
        bridge.send({ kind: "drive", on: true, client: clientName() });
        // Explicit DIRECT program lease: streamed frames carry only a
        // micro-lease, which LOSES to any explicit program lease left on a
        // fixture (runtime.cpp:84) — an Interactive/GoL session hours ago
        // silently ate every frame on the bench 2026-08-15. hardCut so the
        // takeover is visible immediately.
        bridge.send({ kind: "program", programId: 3, leaseS: 600, hardCut: true });
      } else if (m.kind === "lease") {
        // another device holds the pen: frames are dropped server-side.
        // Surface it — silence here is the "haunted controls" bug.
        if (m.payload.granted === false) {
          console.warn("[cambium] drive DENIED — driver:", m.payload.name, "ttl", m.payload.ttl_s);
          window.dispatchEvent(new CustomEvent("resonance:cambium-lease", { detail: m.payload }));
        }
      }
    });

    bridge
      .connect()
      .then(() => {
        if (cancelled) { bridge.disconnect(); return; }
        timer = setInterval(() => {
          const states = telemetry.states;
          if (!states.length || !bridge.connected()) return;
          // Omit near-black lights from the frame: per the wire contract,
          // absence means "no opinion — stay autonomous", so idle lights hold
          // their red listening beacon instead of being commanded dark
          // (Elliot 2026-08-15: "they were not staying in the default red
          // state"). An explicit blackout still sends black to everyone.
          const blackout = useTwin.getState().control.blackout;
          const fx = states
            .map((s) => ({ id: s.id, rgb: s.rgb }))
            .filter((s) => blackout || (s.rgb[0] + s.rgb[1] + s.rgb[2]) > 0.02);
          if (fx.length) bridge.sendFrame(fx);
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
