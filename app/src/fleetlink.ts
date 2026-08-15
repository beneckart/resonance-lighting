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
 *  link because identify is bounded and targeted, unlike a frame stream.
 *
 *  RETURNS WHETHER IT ACTUALLY WENT OUT. This used to return void, and the
 *  packet could evaporate at two separate layers without a word:
 *    1. `bridge?.send` — optional chain, so no bridge meant a silent no-op;
 *    2. cambium.ts send() early-returns unless the socket is OPEN.
 *  Meanwhile the Locate sheet said "blink sent (3 s)" unconditionally and the
 *  Command pad said nothing at all, so an operator could not tell "the lantern
 *  ignored it" from "it never left the browser" (Elliot 08-15: "why does the
 *  Blink still not work?"). Honest failure is cheap; a lie during a locate walk
 *  costs you a trip up the tree. */
export function fleetIdentify(mac: string | null, seconds = 3): boolean {
  // seam contract (bridge.ts IdentifyDown): {kind, mac: string|null, seconds} —
  // CambiumBridge translates null-mac to the omitted-field wire form itself
  if (!bridge || !bridge.connected()) return false;
  bridge.send({ kind: "identify", mac, seconds });
  return true;
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
  // BOOTSTRAP FROM THE DAEMON'S MEMORY (08-15, Elliot: "why am I only seeing
  // one light now? We had 10 before"): this registry dies with the page, but
  // the daemon has been listening for hours — its /fleet roster is the census
  // a refresh was throwing away. Seed from it once, then live frames take
  // over. Bootstrapped rows are stamped just inside the 5-minute window and
  // OUTSIDE the 60-second one: "the daemon knows this lantern" is honest;
  // "this page heard it seconds ago" would not be.
  const httpBase = (b as unknown as { url?: string })?.url ?? "";
  const fleetUrl = httpBase.replace(/^ws/, "http").replace(/\/ws$/, "/fleet");
  void fetch(fleetUrl)
    .then((r) => (r.ok ? r.json() : null))
    .then((roster: Record<string, { fixture_id?: string | null; telemetry?: Record<string, unknown> | null }> | null) => {
      if (!roster || bridge !== b) return;
      const now = Date.now();
      const num = (v: unknown) => (typeof v === "number" && Number.isFinite(v) ? v : 0);
      const numOrNull = (v: unknown) => (typeof v === "number" && Number.isFinite(v) ? v : null);
      for (const [mac, row] of Object.entries(roster)) {
        if (registry.records[mac]) continue; // live data already beat us
        const t = row.telemetry ?? {};
        applyHeartbeat(registry, {
          kind: "hb", mac: mac.toUpperCase(), seq: 0, uptimeMs: 0,
          battMv: num(t.batt_mv), battMa: num(t.batt_ma), soc: num(t.soc_pct),
          resetReason: 0, caState: num(t.life_state), mode: num(t.mode),
          dlPdrX1000: 0, dlRssi: num(t.dl_rssi),
          lifeState: numOrNull(t.life_state), program: numOrNull(t.program),
          powerTier: numOrNull(t.power_tier),
        }, now - 299_000);
      }
      bump();
    })
    .catch(() => { /* no HTTP roster — live frames alone, as before */ });

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
