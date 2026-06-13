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
