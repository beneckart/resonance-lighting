// fixtures.json contract (resonance.fixtures/0.1) — the neutral geometry handoff.

/** Base-aware asset URL: the app normally serves from "/", but the published
 *  copy on the Resonance Network site lives under "/lighting/" (vite
 *  --base=/lighting/). Every runtime fetch/loader path goes through here so
 *  ONE build flag relocates the whole app. */
export function asset(p: string): string {
  const base = import.meta.env.BASE_URL ?? "/";
  return base.replace(/\/$/, "") + p;
}
export interface Fixture {
  fixture_id: string;
  name: string;
  role: string;
  position: [number, number, number]; // Blender world coords (Z-up)
  zone: string;
  led_type: string;
  lumens_max: number;
  beam_deg: number;
  design_color: [number, number, number];
  aim?: [number, number, number]; // schema 0.2: cast direction, Blender Z-up (optional)
}

export interface FixturesDoc {
  meta: {
    source: string;
    exported: string;
    up_axis: string;
    units: string;
    count: number;
    bbox: { min: [number, number, number]; max: [number, number, number] };
    schema: string;
  };
  fixtures: Fixture[];
}

/** Blender (Z-up, x,y,z) → three.js (Y-up, x, z, -y). Matches glTF export_yup. */
export function blenderToThree(p: [number, number, number]): [number, number, number] {
  return [p[0], p[2], -p[1]];
}

export async function loadFixtures(url = asset("/fixtures.json")): Promise<FixturesDoc> {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`fixtures.json ${res.status}`);
  return res.json();
}

export interface ValidationResult {
  ok: boolean;
  errors: string[];
}

/** Validate a fixtures.json against the resonance.fixtures/0.1 contract (G2). Pure;
 *  used to gate a swapped-in Grasshopper export. */
export function validateFixturesDoc(doc: unknown): ValidationResult {
  const errors: string[] = [];
  const d = doc as Partial<FixturesDoc> | null;
  if (!d || typeof d !== "object") return { ok: false, errors: ["doc is not an object"] };
  if (!d.meta || typeof d.meta !== "object") errors.push("missing meta");
  else {
    if (typeof d.meta.count !== "number") errors.push("meta.count is not a number");
    if (!d.meta.bbox || !Array.isArray(d.meta.bbox.min) || !Array.isArray(d.meta.bbox.max))
      errors.push("missing/invalid meta.bbox");
  }
  if (!Array.isArray(d.fixtures)) errors.push("fixtures is not an array");
  else if (d.fixtures.length === 0) errors.push("fixtures is empty");
  else {
    d.fixtures.forEach((f, i) => {
      if (!f || typeof f.fixture_id !== "string") errors.push(`fixtures[${i}].fixture_id missing`);
      if (!Array.isArray(f?.position) || f.position.length !== 3 || f.position.some((n) => typeof n !== "number"))
        errors.push(`fixtures[${i}].position must be [x,y,z] numbers`);
      if (typeof f?.beam_deg !== "number") errors.push(`fixtures[${i}].beam_deg missing`);
      if (typeof f?.zone !== "string") errors.push(`fixtures[${i}].zone missing`);
      if (f?.aim !== undefined && (!Array.isArray(f.aim) || f.aim.length !== 3 || f.aim.some((n) => typeof n !== "number")))
        errors.push(`fixtures[${i}].aim must be [x,y,z] numbers`); // schema 0.2
    });
  }
  return { ok: errors.length === 0, errors };
}

export interface FixtureAudit {
  byRole: Record<string, number>;
  byZone: Record<string, number>;
  withAim: number;
  warnings: string[];
}

/** Data-quality audit of a fixtures doc — counts by role/zone + aim-sanity
 *  (downlights must aim DOWN, uplights UP in Blender Z-up). Catches regressions
 *  as the export evolves (0.3 is a procedural first pass). Pure. */
export function auditFixtures(doc: FixturesDoc): FixtureAudit {
  const byRole: Record<string, number> = {};
  const byZone: Record<string, number> = {};
  const warnings: string[] = [];
  let withAim = 0;
  for (const f of doc.fixtures) {
    byRole[f.role] = (byRole[f.role] ?? 0) + 1;
    byZone[f.zone] = (byZone[f.zone] ?? 0) + 1;
    if (f.aim) {
      withAim++;
      const z = f.aim[2]; // Blender up axis
      if (f.role === "downlight" && z > -0.3) warnings.push(`${f.fixture_id}: downlight aim not pointing down (z=${z.toFixed(2)})`);
      if (f.role === "uplight" && z < 0.3) warnings.push(`${f.fixture_id}: uplight aim not pointing up (z=${z.toFixed(2)})`);
    }
  }
  return { byRole, byZone, withAim, warnings };
}

/** TEST-GRID layout (Elliot 2026-07-08): 7×7 = 49 lights hung across a 20×20 m
 *  square at DIFFERENT heights and spacings — a lab rig to test whether the
 *  lights can orient themselves in space (self-map) and pick the right identity
 *  on the 3-D layout. The TREE is the actual piece; this is testing mode only.
 *  Deterministic per seed → "re-jitter" gives a fresh hanging arrangement. */
export function makeTestGridDoc(seed = 1): FixturesDoc {
  const rnd = (i: number) => { const x = Math.sin(i * 127.1 + seed * 311.7) * 43758.5453; return x - Math.floor(x); };
  const fixtures: Fixture[] = [];
  const pitch = 20 / 6; // nominal 7-across over 20 m
  for (let r = 0; r < 7; r++) for (let c = 0; c < 7; c++) {
    const i = r * 7 + c;
    const jx = (rnd(i * 2 + 1) - 0.5) * 1.6; // per-light spacing jitter
    const jy = (rnd(i * 2 + 2) - 0.5) * 1.6;
    fixtures.push({
      fixture_id: `G${String(i).padStart(2, "0")}`,
      name: `Grid ${r + 1}-${c + 1}`,
      role: "downlight",
      position: [-10 + c * pitch + jx, -10 + r * pitch + jy, 2.2 + rnd(i * 3 + 7) * 2.8], // hang heights 2.2–5 m
      zone: "grid",
      led_type: "rgbw",
      lumens_max: 450,
      beam_deg: 120,
      design_color: [1, 0.85, 0.6],
    });
  }
  const xs = fixtures.map((f) => f.position[0]), ys = fixtures.map((f) => f.position[1]), zs = fixtures.map((f) => f.position[2]);
  return {
    meta: {
      source: "testgrid", exported: new Date().toISOString(), up_axis: "Z", units: "m", count: fixtures.length,
      bbox: { min: [Math.min(...xs), Math.min(...ys), Math.min(...zs)], max: [Math.max(...xs), Math.max(...ys), Math.max(...zs)] },
      schema: "resonance.fixtures/0.3",
    },
    fixtures,
  };
}
