// IES-ish beam photometrics: the same lumens spread over a narrower cone reads
// brighter (higher candela); over a wider cone, dimmer. Gives the beams a physically
// grounded look from each fixture's lumens_max + beam_deg.

/** Approx peak intensity (candela-ish) = lumens / cone solid angle. */
export function beamIntensity(lumens: number, beamDeg: number): number {
  const half = (Math.min(170, Math.max(5, beamDeg)) / 2) * (Math.PI / 180);
  const solidAngle = 2 * Math.PI * (1 - Math.cos(half));
  return lumens / Math.max(1e-4, solidAngle);
}

/** Intensity relative to a reference fixture (~1 at 450lm / 120°). */
export function relativeBeam(lumens: number, beamDeg: number, refLumens = 450, refDeg = 120): number {
  return beamIntensity(lumens, beamDeg) / beamIntensity(refLumens, refDeg);
}
