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
  soc: number;
  caState: number;
  mode: number;
  dlRssi: number; // survey signal: bridge frames' RSSI at this node
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
    lastSeq: hb.seq - 1, lost: 0, reboots: 0, uptimeMs: 0, battMv: 0, soc: 0,
    caState: 0, mode: 0, dlRssi: 0,
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
  rec.soc = hb.soc;
  rec.caState = hb.caState;
  rec.mode = hb.mode;
  rec.dlRssi = hb.dlRssi;
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
