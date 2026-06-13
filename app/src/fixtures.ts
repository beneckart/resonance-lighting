// fixtures.json contract (resonance.fixtures/0.1) — the neutral geometry handoff.
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

export async function loadFixtures(url = "/fixtures.json"): Promise<FixturesDoc> {
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
