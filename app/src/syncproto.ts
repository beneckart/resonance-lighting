import type { SurveyNode, MapStage, RssiRow } from "./selfmap";

/** SYNC PROTOCOL v1.1 — the CALIBRATION frames of the control plane (the seam
 *  Ben's firmware implements; the sim implements the same shapes today).
 *
 *  Everything here honors the architecture contract: CONTROL PARAMETERS ONLY,
 *  low-rate, broadcast where possible (ADR 0004/0010). Survey data rides the
 *  existing ~1 Hz heartbeat — calibration adds NO new radio duty beyond a
 *  slightly fatter heartbeat while a survey session is open.
 *
 *  The staged flow these frames carry (docs/research/16-SYNC-CALIBRATION-PROTOCOL.md):
 *   stage 1  cal_survey → nodes measure per-neighbor RSSI → cal_rssi reports
 *   stage 2  cal_tof reports (downward ranger; height where a ground return exists)
 *   stage 3  cortex solves (selfmap.ts) → cal_assign HYPOTHESES pushed to flash
 *   stage 4  identify/flash + installer confirm → cal_assign stage:"confirmed"
 *   stage 5  photogrammetry residuals accepted → cal_lock freezes the map
 */

// ── frames ────────────────────────────────────────────────────────────────────

/** Downlink broadcast: open a survey session. Every node starts counting
 *  packets it hears (it ALREADY hears heartbeats — the survey just turns on
 *  bookkeeping) and reports its table in the heartbeat for `durationS`. */
export interface SurveyCmd {
  proto: 1;
  kind: "cal_survey";
  session: number; // survey session id (epoch-like, monotonic)
  durationS: number; // how long nodes keep survey bookkeeping on
  minPings: number; // report a neighbor only after this many packets heard
}

/** Uplink (heartbeat rider): one node's RSSI table. `rows` carries the MEDIAN
 *  dBm per heard neighbor — medians, not means, so a walking body or a rain
 *  squall doesn't poison the survey. */
export interface RssiReport {
  proto: 1;
  kind: "cal_rssi";
  session: number;
  mac: string; // compact id (last 3 MAC bytes) — the node's stable self-id
  role: string; // the node KNOWS its hardware role (burned in flash at build)
  rows: RssiRow[];
}

/** Uplink (heartbeat rider): downward ToF self-location sample. `heightM` is
 *  the range to the nearest surface BELOW (VL53 ceiling ≈ 4 m) — that's true
 *  height-above-ground only for low fixtures or clear drops; the solver treats
 *  it as a lower-bound/quality-gated signal via `clear`. */
export interface TofReport {
  proto: 1;
  kind: "cal_tof";
  session: number;
  mac: string;
  heightM: number | null; // null = no return in range
  sigmaM: number; // sample spread over the session
  clear: boolean; // true = return looks like ground (stable, planar), not foliage
}

/** Downlink unicast: tell a node WHICH fixture slot it is (it stores this in
 *  flash — Ben's architecture already reserves position/neighbor-list flash
 *  storage). Repeated sends upgrade the stage; never downgrade silently. */
export interface AssignCmd {
  proto: 1;
  kind: "cal_assign";
  mac: string;
  fixtureId: string;
  stage: Extract<MapStage, "hypothesis" | "confirmed" | "locked">;
  pos: [number, number, number]; // model-frame position (three-space, Y-up, m)
  confidence: number; // 0..1 at time of assignment
}

/** Uplink ack: node confirms it stored the assignment. */
export interface AssignAck {
  proto: 1;
  kind: "cal_ack";
  mac: string;
  fixtureId: string;
  stage: string;
}

/** Downlink broadcast: freeze the map. Nodes at stage < confirmed keep their
 *  hypothesis but flag it; the cortex refuses to lock while any node is
 *  unassigned unless `force`. */
export interface LockCmd {
  proto: 1;
  kind: "cal_lock";
  session: number;
  mapVersion: number;
  mapHash: string; // hash of the full mac→fixtureId table (drift detection)
  force: boolean;
}

export type CalFrame = SurveyCmd | RssiReport | TofReport | AssignCmd | AssignAck | LockCmd;

// ── builders ──────────────────────────────────────────────────────────────────

export function surveyCmd(session: number, durationS = 120, minPings = 12): SurveyCmd {
  return { proto: 1, kind: "cal_survey", session, durationS, minPings };
}

export function assignCmd(
  mac: string,
  fixtureId: string,
  stage: AssignCmd["stage"],
  pos: [number, number, number],
  confidence: number,
): AssignCmd {
  return { proto: 1, kind: "cal_assign", mac, fixtureId, stage, pos, confidence };
}

/** Deterministic map hash (FNV-1a over the sorted mac→fixtureId table) — both
 *  sides can compute it; a mismatch on reconnect means the map drifted. */
export function mapHash(entries: { mac: string; fixtureId: string }[]): string {
  const canon = entries
    .map((e) => `${e.mac}=${e.fixtureId}`)
    .sort()
    .join(";");
  let h = 0x811c9dc5;
  for (let i = 0; i < canon.length; i++) {
    h ^= canon.charCodeAt(i);
    h = Math.imul(h, 0x01000193) >>> 0;
  }
  return h.toString(16).padStart(8, "0");
}

export function lockCmd(session: number, mapVersion: number, entries: { mac: string; fixtureId: string }[], force = false): LockCmd {
  return { proto: 1, kind: "cal_lock", session, mapVersion, mapHash: mapHash(entries), force };
}

// ── validation (the wire is untrusted — reports come from 100+ radios) ───────

export function isCalFrame(x: unknown): x is CalFrame {
  const f = x as Partial<CalFrame> | null;
  if (!f || typeof f !== "object" || f.proto !== 1 || typeof f.kind !== "string") return false;
  switch (f.kind) {
    case "cal_survey": {
      const s = f as Partial<SurveyCmd>;
      return typeof s.session === "number" && typeof s.durationS === "number" && typeof s.minPings === "number";
    }
    case "cal_rssi": {
      const s = f as Partial<RssiReport>;
      return typeof s.session === "number" && typeof s.mac === "string" && typeof s.role === "string" &&
        Array.isArray(s.rows) && s.rows.every((r) => r && typeof r.mac === "string" && typeof r.med === "number" && typeof r.n === "number");
    }
    case "cal_tof": {
      const s = f as Partial<TofReport>;
      return typeof s.session === "number" && typeof s.mac === "string" &&
        (s.heightM === null || typeof s.heightM === "number") && typeof s.sigmaM === "number" && typeof s.clear === "boolean";
    }
    case "cal_assign": {
      const s = f as Partial<AssignCmd>;
      return typeof s.mac === "string" && typeof s.fixtureId === "string" &&
        (s.stage === "hypothesis" || s.stage === "confirmed" || s.stage === "locked") &&
        Array.isArray(s.pos) && s.pos.length === 3 && typeof s.confidence === "number";
    }
    case "cal_ack": {
      const s = f as Partial<AssignAck>;
      return typeof s.mac === "string" && typeof s.fixtureId === "string" && typeof s.stage === "string";
    }
    case "cal_lock": {
      const s = f as Partial<LockCmd>;
      return typeof s.session === "number" && typeof s.mapVersion === "number" && typeof s.mapHash === "string" && typeof s.force === "boolean";
    }
    default:
      return false;
  }
}

/** Fold a session's RssiReports + TofReports into solver SurveyNodes.
 *  Later reports for the same mac REPLACE earlier ones (nodes re-report as the
 *  session accumulates pings). ToF gating: only `clear` returns become heights;
 *  a canopy fixture staring into leaves reports null height and the solver
 *  falls back to band-capacity logic. */
export function foldSession(rssi: RssiReport[], tof: TofReport[]): SurveyNode[] {
  const byMac = new Map<string, SurveyNode>();
  for (const r of rssi) {
    byMac.set(r.mac, { mac: r.mac, role: r.role, rows: r.rows, tofHeightM: byMac.get(r.mac)?.tofHeightM ?? null });
  }
  for (const t of tof) {
    const node = byMac.get(t.mac);
    if (!node) continue;
    node.tofHeightM = t.clear && t.heightM !== null ? t.heightM : null;
  }
  return [...byMac.values()];
}
