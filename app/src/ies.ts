/** Minimal IESNA LM-63 photometric parser — enough to drive the twin's cone
 *  geometry from the real baked fixture profile (blender-architect's
 *  downlight.ies). We extract the beam angle (to 50% peak) and field angle
 *  (to ~10% peak) so the SpotLight cone matches the physical downlight. */
export interface IESProfile {
  vertAngles: number[]; // degrees from nadir (0 = straight down)
  candelas: number[]; // candela at each vertical angle
  peak: number; // max candela
  beamDeg: number; // FULL beam angle (to 50% peak)
  fieldDeg: number; // FULL field angle (to 10% peak)
}

/** Interpolate the vertical angle at which candela falls to `frac`×peak. */
function angleAtFraction(angles: number[], cd: number[], peak: number, frac: number): number {
  const target = peak * frac;
  for (let i = 1; i < cd.length; i++) {
    if (cd[i] <= target) {
      const a0 = angles[i - 1], a1 = angles[i];
      const c0 = cd[i - 1], c1 = cd[i];
      if (c0 === c1) return a1;
      const t = (c0 - target) / (c0 - c1); // 0..1 between i-1 and i
      return a0 + t * (a1 - a0);
    }
  }
  return angles[angles.length - 1];
}

export function parseIES(text: string): IESProfile {
  const lines = text.split(/\r?\n/);
  const tiltIdx = lines.findIndex((l) => l.toUpperCase().startsWith("TILT="));
  if (tiltIdx < 0) throw new Error("IES: no TILT line");
  // flatten all numbers after the TILT line
  const nums: number[] = [];
  for (let i = tiltIdx + 1; i < lines.length; i++) {
    for (const tok of lines[i].trim().split(/\s+/)) {
      const n = parseFloat(tok);
      if (!isNaN(n)) nums.push(n);
    }
  }
  const nVert = Math.round(nums[3]);
  const nHoriz = Math.round(nums[4]);
  const headerLen = 10 + 3; // 10 count fields + 3 ballast/watts fields
  const vertAngles = nums.slice(headerLen, headerLen + nVert);
  const candStart = headerLen + nVert + nHoriz;
  const candelas = nums.slice(candStart, candStart + nVert);
  const peak = Math.max(...candelas, 1e-6);
  const beamDeg = 2 * angleAtFraction(vertAngles, candelas, peak, 0.5);
  const fieldDeg = 2 * angleAtFraction(vertAngles, candelas, peak, 0.1);
  return { vertAngles, candelas, peak, beamDeg, fieldDeg };
}
