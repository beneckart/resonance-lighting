import type { SimFixture } from "./store";

/** COMMISSIONING / CALIBRATION (the bridge to the real lights).
 *
 *  Self-IDENTIFY: each physical fixture's ESP32 self-IDs by its WiFi MAC — a
 *  stable per-node ID (firmware/ARCHITECTURE.md: "MAC-derived fixture ID"). On
 *  boot it announces that MAC on the ESP-NOW mesh.
 *
 *  Self-LOCATE: the hardware CANNOT know its position — ESP-NOW RSSI is only an
 *  approximate topology signal ("not exact distance"), and there's no per-fixture
 *  GPS. So position comes from a one-time COMMISSIONING pass that maps each MAC
 *  to a fixtures.json slot (its Blender-known position/role/aim). This is the
 *  same model Chromatik/LX uses — fixture positions are AUTHORED (the model
 *  file), never auto-sensed.
 *
 *  Flow: twin sends an IDENTIFY (flash) to a fixtureId → cortex resolves it to a
 *  MAC (or, when unmapped, the installer flashes candidates) → installer sees
 *  which physical light blinks → taps the matching slot → assign(mac→fixtureId).
 *  Protocol-v1 frames stay fixtureId-addressed; the cortex holds this map to
 *  translate fixtureId ⇄ MAC on the wire. */

export type EntryStage = "hypothesis" | "confirmed" | "locked";
export type EntryMethod = "manual" | "mesh" | "photo";

export interface FixtureMapEntry {
  mac: string; // e.g. "A1B2C3" (compactIdFromMac: last 3 MAC bytes)
  fixtureId: string; // fixtures.json fixture_id, e.g. "F012"
  at: string; // ISO timestamp of assignment
  // v2 (staged sync protocol) — optional so v1 entries stay valid:
  stage?: EntryStage; // where on the confidence ladder this entry sits
  confidence?: number; // 0..1 at assignment time (1 for manual confirms)
  method?: EntryMethod; // what produced it (mesh solve / installer tap / photo)
}

export interface CalibrationMap {
  version: 2;
  entries: FixtureMapEntry[];
}

const KEY = "resonance.calibration.v1"; // storage key kept stable across versions

export function emptyMap(): CalibrationMap {
  return { version: 2, entries: [] };
}

export function loadCalibration(): CalibrationMap {
  try {
    const raw = localStorage.getItem(KEY);
    if (!raw) return emptyMap();
    const m = JSON.parse(raw) as { version?: number; entries?: FixtureMapEntry[] };
    if (!m || !Array.isArray(m.entries)) return emptyMap();
    // migrate v1 → v2: hand-commissioned entries are installer-confirmed truth
    const entries = m.entries.map((e) =>
      e.stage ? e : { ...e, stage: "confirmed" as const, confidence: 1, method: "manual" as const });
    return { version: 2, entries };
  } catch {
    return emptyMap();
  }
}

export function saveCalibration(m: CalibrationMap) {
  try {
    localStorage.setItem(KEY, JSON.stringify(m));
  } catch {
    /* storage unavailable — non-fatal */
  }
}

/** Assign a MAC to a fixtureId (idempotent: replaces any prior mapping of either
 *  the MAC or the fixtureId so each maps 1:1). Pure — returns a new map.
 *  Default provenance is an installer tap (confirmed/manual); the mesh solver
 *  passes its own stage/confidence/method. */
export function assign(
  map: CalibrationMap,
  mac: string,
  fixtureId: string,
  at: string,
  meta: { stage: EntryStage; confidence: number; method: EntryMethod } = { stage: "confirmed", confidence: 1, method: "manual" },
): CalibrationMap {
  const entries = map.entries.filter((e) => e.mac !== mac && e.fixtureId !== fixtureId);
  entries.push({ mac, fixtureId, at, ...meta });
  return { version: 2, entries };
}

/** Promote every entry to `locked` (photogrammetry residuals accepted). */
export function lockAll(map: CalibrationMap): CalibrationMap {
  return { version: 2, entries: map.entries.map((e) => ({ ...e, stage: "locked" as const })) };
}

/** Entries the installer should walk with a flash-and-confirm pass. */
export function unconfirmedEntries(map: CalibrationMap): FixtureMapEntry[] {
  return map.entries.filter((e) => (e.stage ?? "confirmed") === "hypothesis")
    .sort((a, b) => (a.confidence ?? 0) - (b.confidence ?? 0));
}

export function resolveFixtureId(map: CalibrationMap, mac: string): string | null {
  return map.entries.find((e) => e.mac === mac)?.fixtureId ?? null;
}

export function resolveMac(map: CalibrationMap, fixtureId: string): string | null {
  return map.entries.find((e) => e.fixtureId === fixtureId)?.mac ?? null;
}

/** Fixtures still needing a physical light assigned (commissioning progress). */
export function unassignedFixtures(map: CalibrationMap, fixtures: SimFixture[]): SimFixture[] {
  const mapped = new Set(map.entries.map((e) => e.fixtureId));
  return fixtures.filter((f) => !mapped.has(f.id));
}

export interface CalibrationProgress {
  total: number;
  assigned: number;
  remaining: number;
  pct: number;
}

export function progress(map: CalibrationMap, fixtures: SimFixture[]): CalibrationProgress {
  const total = fixtures.length;
  const ids = new Set(fixtures.map((f) => f.id));
  const assigned = map.entries.filter((e) => ids.has(e.fixtureId)).length;
  return { total, assigned, remaining: total - assigned, pct: total ? assigned / total : 0 };
}

/** The IDENTIFY command the twin/cortex broadcasts so an installer can SEE which
 *  physical fixture is which (it flashes). Addressed by fixtureId on Protocol-v1;
 *  if not yet mapped, the cortex sweeps unmapped MACs one at a time. */
export interface IdentifyCommand {
  proto: 1;
  kind: "identify";
  fixtureId: string;
  mac: string | null; // null until commissioned → cortex sweeps candidates
  flashHz: number;
}

export function identifyCommand(map: CalibrationMap, fixtureId: string, flashHz = 4): IdentifyCommand {
  return { proto: 1, kind: "identify", fixtureId, mac: resolveMac(map, fixtureId), flashHz };
}
