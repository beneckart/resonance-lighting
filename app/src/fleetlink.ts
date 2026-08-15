import { CambiumBridge } from "./cambium";
import {
  applyEvent, applyHeartbeat, emptyRegistry, fleetCensus,
  type FleetCensus, type Registry,
} from "./macregistry";

/** FLEET LINK — the app's ALWAYS-ON telemetry ear on the real fleet.
 *
 *  Elliot 2026-08-15: "the simulator now uses the real lights by default" +
 *  "bridge mode … for locating that reads the status of all the lights".
 *  This is that: one app-level cambium connection, dialed at boot, feeding one
 *  shared MAC registry that Locate/Command surfaces read. Before this, fleet
 *  telemetry only existed while the desktop FleetPanel was open with its
 *  private bridge — close the panel, lose the ear.
 *
 *  READ-ONLY BY DESIGN. This link never sends frames: the 📡 drive toggle
 *  (RealDriveDriver) stays the ONE frame-pump owner. Two senders on one fleet
 *  is the collision this codebase has been burned by; an always-on link that
 *  also transmitted would arm that footgun on every phone that opens the app.
 *  The only non-telemetry message allowed out is identify (blink) — a bounded,
 *  targeted locate aid, not a frame stream.
 *
 *  Module singleton, not React state: the registry must accumulate across
 *  tab/page changes and be readable from non-component code (command handlers).
 *  Components subscribe via useSyncExternalStore on (version). */

interface FleetLinkState {
  connected: boolean;
  /** when the current listening session began (for census honesty) */
  startedMs: number;
  version: number; // bumped on every frame — the subscription key
}

const registry: Registry = emptyRegistry();
const state: FleetLinkState = { connected: false, startedMs: 0, version: 0 };
const subs = new Set<() => void>();
let bridge: CambiumBridge | null = null;

function bump() {
  state.version++;
  for (const s of subs) s();
}

export function fleetRegistry(): Registry {
  return registry;
}

export function fleetConnected(): boolean {
  return state.connected;
}

export function fleetVersion(): number {
  return state.version;
}

export function fleetCensusNow(now = Date.now()): FleetCensus {
  return fleetCensus(registry, now, state.startedMs || undefined);
}

export function subscribeFleet(fn: () => void): () => void {
  subs.add(fn);
  return () => subs.delete(fn);
}

/** blink one lantern (or all, mac=null) — the locate aid. Rides the telemetry
 *  link because identify is bounded and targeted, unlike a frame stream. */
export function fleetIdentify(mac: string | null, seconds = 3): void {
  // seam contract (bridge.ts IdentifyDown): {kind, mac: string|null, seconds} —
  // CambiumBridge translates null-mac to the omitted-field wire form itself
  bridge?.send({ kind: "identify", mac, seconds });
}

/** Dial the daemon and start feeding the registry. Idempotent. */
export function startFleetLink(url?: string): void {
  if (bridge) return; // already listening
  if (CambiumBridge.disabledByLocation()) return; // ?cambium=0 — operator said no
  const b = new CambiumBridge({ url: url ?? CambiumBridge.urlFromLocation() });
  bridge = b;
  state.startedMs = Date.now();
  b.onUp((f) => {
    if (f.kind === "hb") applyHeartbeat(registry, f, Date.now());
    else applyEvent(registry, f, Date.now());
    bump();
  });
  b.onMeta((m) => {
    // the bridge emits open/close meta; established-session reconnect is its
    // machinery, we just mirror socket truth for the UI
    if (m.kind === "open" && !state.connected) { state.connected = true; bump(); }
    if (m.kind === "close" && state.connected) { state.connected = false; bump(); }
  });
  const dial = () => {
    if (bridge !== b) return; // stopped meanwhile
    void b.connect().catch(() => {
      // A FAILED INITIAL dial does not self-retry (the bridge only fights for
      // once-established sessions — deliberate, see cambium.ts). But real-by-
      // default means a phone opened before the daemon boots should still find
      // the fleet eventually, so we re-dial gently. 30 s: patient enough to be
      // invisible, cheap enough to never matter. A daemon-less deploy just
      // stays quiet at one attempt every 30 s.
      if (bridge === b) setTimeout(dial, 30_000);
    });
  };
  dial();
}

export function stopFleetLink(): void {
  bridge?.disconnect();
  bridge = null;
  state.connected = false;
  bump();
}
