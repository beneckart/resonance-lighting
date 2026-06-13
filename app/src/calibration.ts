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

export interface FixtureMapEntry {
  mac: string; // e.g. "A1B2C3" (compactIdFromMac: last 3 MAC bytes)
  fixtureId: string; // fixtures.json fixture_id, e.g. "F012"
  at: string; // ISO timestamp of assignment
}

export interface CalibrationMap {
  version: 1;
  entries: FixtureMapEntry[];
}

const KEY = "resonance.calibration.v1";

export function emptyMap(): CalibrationMap {
  return { version: 1, entries: [] };
}

export function loadCalibration(): CalibrationMap {
  try {
    const raw = localStorage.getItem(KEY);
    if (!raw) return emptyMap();
    const m = JSON.parse(raw) as CalibrationMap;
    return m && Array.isArray(m.entries) ? { version: 1, entries: m.entries } : emptyMap();
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
 *  the MAC or the fixtureId so each maps 1:1). Pure — returns a new map. */
export function assign(map: CalibrationMap, mac: string, fixtureId: string, at: string): CalibrationMap {
  const entries = map.entries.filter((e) => e.mac !== mac && e.fixtureId !== fixtureId);
  entries.push({ mac, fixtureId, at });
  return { version: 1, entries };
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
