import type { HbFrame, EvtFrame } from "./bridge";

/** MAC REGISTRY — the persistent ledger of every physical light ever heard.
 *
 *  Each light's MAC (3-byte compact id, NbHeader.src_id) is its unique
 *  communication identity for life. This registry maps and LOGS them:
 *  first/last heard, liveness, battery, the rules state it reported, reboots
 *  (uptime going backwards), and an event log of every edge. The calibration
 *  map (calibration.ts) says WHERE a MAC hangs; this registry says WHO exists
 *  and HOW it's doing. Pure functions — the panel owns the object and drives
 *  updates from bridge frames. */

export interface MacRecord {
  mac: string;
  firstSeenMs: number; // registry clock (ms since session start / bridge connect)
  lastSeenMs: number;
  hbCount: number;
  evtCount: number;
  lastSeq: number;
  lost: number; // seq gaps observed (uplink PDR proxy)
  reboots: number; // uptime regressions
  uptimeMs: number;
  battMv: number;
  /** signed: negative = discharging, positive = charging. The SIGN is the most
   *  actionable number on the panel — "2927 mV" alone cannot distinguish a fleet
   *  recovering in sun from one draining in a shed. */
  battMa: number;
  soc: number;
  caState: number;
  mode: number;
  dlRssi: number; // survey signal: bridge frames' RSSI at this node
  /** Ben's lifecycle triple + choreo intensity. null = the fixture has not told
   *  us, which is NOT the same as zero (see HbFrame). */
  lifeState: number | null;
  program: number | null;
  powerTier: number | null;
  intensity: number | null;
}

export interface MacEvent {
  atMs: number;
  mac: string;
  kind: string; // "first_heard" | "tap" | "state" | "boot" | "fault" | "identify_ack" | "offline" | "online"
  value: number;
}

export interface Registry {
  records: Record<string, MacRecord>;
  events: MacEvent[]; // newest last, capped
  offline: Record<string, boolean>;
}

export const EVENT_CAP = 500;
/** offline = no heartbeat for 3 periods at 2 Hz + jitter headroom */
export const OFFLINE_AFTER_MS = 2500;

export function emptyRegistry(): Registry {
  return { records: {}, events: [], offline: {} };
}

function pushEvent(reg: Registry, e: MacEvent) {
  reg.events.push(e);
  if (reg.events.length > EVENT_CAP) reg.events.splice(0, reg.events.length - EVENT_CAP);
}

export function applyHeartbeat(reg: Registry, hb: HbFrame, nowMs: number): Registry {
  const prev = reg.records[hb.mac];
  const rec: MacRecord = prev ?? {
    mac: hb.mac, firstSeenMs: nowMs, lastSeenMs: nowMs, hbCount: 0, evtCount: 0,
    lastSeq: hb.seq - 1, lost: 0, reboots: 0, uptimeMs: 0, battMv: 0, battMa: 0, soc: 0,
    caState: 0, mode: 0, dlRssi: 0,
    lifeState: null, program: null, powerTier: null, intensity: null,
  };
  if (!prev) pushEvent(reg, { atMs: nowMs, mac: hb.mac, kind: "first_heard", value: 0 });
  if (prev && hb.uptimeMs < rec.uptimeMs) {
    rec.reboots += 1;
    pushEvent(reg, { atMs: nowMs, mac: hb.mac, kind: "boot", value: rec.reboots });
  }
  const gap = hb.seq - rec.lastSeq - 1;
  if (prev && gap > 0 && gap < 1000) rec.lost += gap;
  rec.lastSeq = hb.seq;
  rec.lastSeenMs = nowMs;
  rec.hbCount += 1;
  rec.uptimeMs = hb.uptimeMs;
  rec.battMv = hb.battMv;
  rec.battMa = hb.battMa;
  rec.soc = hb.soc;
  rec.caState = hb.caState;
  rec.mode = hb.mode;
  rec.dlRssi = hb.dlRssi;
  // A heartbeat that omits a field must not ERASE what we already knew — the
  // fleet reports these sparsely, and "??" flickering on screen every other
  // beat is worse than a slightly stale number.
  if (hb.lifeState !== undefined && hb.lifeState !== null) rec.lifeState = hb.lifeState;
  if (hb.program !== undefined && hb.program !== null) rec.program = hb.program;
  if (hb.powerTier !== undefined && hb.powerTier !== null) rec.powerTier = hb.powerTier;
  reg.records[hb.mac] = rec;
  if (reg.offline[hb.mac]) {
    reg.offline[hb.mac] = false;
    pushEvent(reg, { atMs: nowMs, mac: hb.mac, kind: "online", value: 0 });
  }
  return reg;
}

export function applyEvent(reg: Registry, evt: EvtFrame, nowMs: number): Registry {
  const rec = reg.records[evt.mac];
  if (rec) {
    rec.lastSeenMs = nowMs;
    rec.evtCount += 1;
    rec.lastSeq = Math.max(rec.lastSeq, evt.seq);
    if (evt.event === "state" || evt.event === "tap") rec.caState = evt.value;
    if (evt.intensity !== undefined) rec.intensity = evt.intensity;
  }
  pushEvent(reg, { atMs: nowMs, mac: evt.mac, kind: evt.event, value: evt.value });
  return reg;
}

/** liveness sweep — call periodically; flags nodes that went quiet */
export function sweepOffline(reg: Registry, nowMs: number, afterMs = OFFLINE_AFTER_MS): string[] {
  const newlyOffline: string[] = [];
  for (const rec of Object.values(reg.records)) {
    const quiet = nowMs - rec.lastSeenMs > afterMs;
    if (quiet && !reg.offline[rec.mac]) {
      reg.offline[rec.mac] = true;
      pushEvent(reg, { atMs: nowMs, mac: rec.mac, kind: "offline", value: 0 });
      newlyOffline.push(rec.mac);
    }
  }
  return newlyOffline;
}

export function onlineCount(reg: Registry): { online: number; total: number } {
  const total = Object.keys(reg.records).length;
  const off = Object.values(reg.offline).filter(Boolean).length;
  return { online: total - off, total };
}

// ── LOCATE: the honest fleet census ─────────────────────────────────────────
//
// WHY THIS EXISTS INSTEAD OF onlineCount(). OFFLINE_AFTER_MS is 2500 ms — three
// periods at the MockBridge's 2 Hz. Real fixtures do not behave like that.
// Measured on the live fleet 2026-08-15: heartbeats arrive around 0.21 Hz,
// staggered, and a fixture at power tier 3 goes minutes between beats. Against
// the real fleet a 2.5 s window marks nearly everything offline forever, and
// cambium's own `online` flag does exactly that — it read false for fixtures
// that had reported seconds earlier.
//
// The deeper trap: THE COUNT IS A FUNCTION OF LISTEN TIME. Successive censuses
// of the same untouched fleet gave 14 (25 s), 15 (90 s), 17 (300 s), 23 (~20
// min). No number here is "the fleet size" — it is "what we have heard, over
// this long". So the census reports WINDOWS and how long it has been listening,
// and refuses to imply completeness. A single number would be a lie with a
// timestamp on it.

export const HEARD_RECENT_MS = 60_000;
export const HEARD_WINDOW_MS = 300_000;

/** ADR-0023 "Standard" tier (Ben, 2026-07-07), the production default.
 *
 *  CAVEAT, stated because acting on it wrongly costs cells: these are specified
 *  UNDER FULL HEX LOAD (~860 mA) and were derived on the 6 Ah 32700. The large
 *  downlights now carry 33140 15 Ah cells, and Ben's ADR says in terms to
 *  re-derive before trusting them there. Treat as guidance, not a verdict. */
export const LFP_DIM_MV = 3000;
export const LFP_LEDS_OFF_MV = 2950;
export const LFP_SLEEP_MV = 2900;
/** Below this, a reading is not a flat battery — it is a broken/absent gauge.
 *  Measured: fixture 9F26BC reported 121 mV while heartbeating normally. */
export const GAUGE_FAULT_MV = 1500;

export type LitState = "lit" | "dormant" | "unknown";

/** Is this light ON? Answered from the most direct evidence available, and
 *  deliberately returning "unknown" rather than guessing "off".
 *
 *  Reporting an unknown fixture as dark is how a real outage hides: the screen
 *  looks the same whether a light is off or silent, so the operator stops
 *  believing the screen. */
export function litState(rec: MacRecord): LitState {
  if (rec.intensity !== null && rec.intensity > 0) return "lit";
  if (rec.lifeState === 3) return "lit"; // running its show
  if (rec.lifeState === 1 && (rec.program ?? 0) === 0) return "dormant";
  if (rec.powerTier !== null && rec.powerTier >= 3 && rec.lifeState === 1) return "dormant";
  return "unknown";
}

export interface FleetCensus {
  total: number; // every mac ever heard this session
  heardRecent: number; // spoke within HEARD_RECENT_MS
  heardWindow: number; // spoke within HEARD_WINDOW_MS
  lit: number;
  dormant: number;
  unknown: number;
  /** null when nothing has ever been heard — not 0, which would read as "0 mV" */
  medianMv: number | null;
  minMv: number | null;
  maxMv: number | null;
  charging: number; // battMa > 0
  belowLedsOff: number; // at/under the ADR-0023 LEDs-off floor
  gaugeFaults: string[]; // macs reporting implausible voltage
  listeningMs: number; // how long this census has been listening — quote it
}

export function fleetCensus(
  reg: Registry,
  nowMs: number,
  /** when this census started listening. `0` is a LEGAL start time (it is what
   *  a fresh session clock reads), so this must be checked for presence, not
   *  truthiness — a `startedMs ? ... : 0` guard silently reported "listening
   *  for 0 ms" for every census begun at t=0, which is the exact case a test
   *  would use. Caught by that test, 2026-08-15. */
  startedMs?: number,
): FleetCensus {
  const recs = Object.values(reg.records);
  const mv = recs
    .map((r) => r.battMv)
    .filter((v) => v >= GAUGE_FAULT_MV)
    .sort((a, b) => a - b);

  let lit = 0, dormant = 0, unknown = 0;
  for (const r of recs) {
    const s = litState(r);
    if (s === "lit") lit++;
    else if (s === "dormant") dormant++;
    else unknown++;
  }

  return {
    total: recs.length,
    heardRecent: recs.filter((r) => nowMs - r.lastSeenMs <= HEARD_RECENT_MS).length,
    heardWindow: recs.filter((r) => nowMs - r.lastSeenMs <= HEARD_WINDOW_MS).length,
    lit,
    dormant,
    unknown,
    medianMv: mv.length ? mv[Math.floor(mv.length / 2)] : null,
    minMv: mv.length ? mv[0] : null,
    maxMv: mv.length ? mv[mv.length - 1] : null,
    charging: recs.filter((r) => r.battMa > 0).length,
    belowLedsOff: mv.filter((v) => v <= LFP_LEDS_OFF_MV).length,
    gaugeFaults: recs.filter((r) => r.battMv > 0 && r.battMv < GAUGE_FAULT_MV).map((r) => r.mac),
    listeningMs: typeof startedMs === "number" ? Math.max(0, nowMs - startedMs) : 0,
  };
}

/** uplink health per node: fraction of sequenced frames actually heard */
export function uplinkPdr(rec: MacRecord): number {
  const heard = rec.hbCount + rec.evtCount;
  return heard + rec.lost > 0 ? heard / (heard + rec.lost) : 1;
}

// ── persistence (the LOG part of "map and log these") ────────────────────────
const KEY = "resonance.macregistry.v1";

export function saveRegistry(reg: Registry) {
  try { localStorage.setItem(KEY, JSON.stringify(reg)); } catch { /* non-fatal */ }
}

export function loadRegistry(): Registry {
  try {
    const raw = localStorage.getItem(KEY);
    if (!raw) return emptyRegistry();
    const r = JSON.parse(raw) as Registry;
    return r && r.records && Array.isArray(r.events) ? r : emptyRegistry();
  } catch {
    return emptyRegistry();
  }
}

/** CSV export — the durable log Elliot can keep per install/test session. */
export function exportCsv(reg: Registry): string {
  const head = "mac,first_seen_ms,last_seen_ms,heartbeats,events,lost,reboots,batt_mv,soc,ca_state,mode,dl_rssi,uplink_pdr,online";
  const rows = Object.values(reg.records)
    .sort((a, b) => a.mac.localeCompare(b.mac))
    .map((r) => [
      r.mac, r.firstSeenMs, r.lastSeenMs, r.hbCount, r.evtCount, r.lost, r.reboots,
      r.battMv, r.soc, r.caState, r.mode, r.dlRssi,
      uplinkPdr(r).toFixed(3), reg.offline[r.mac] ? "0" : "1",
    ].join(","));
  return [head, ...rows].join("\n");
}
