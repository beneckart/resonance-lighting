/** STALE-UNITS GUARD for context glbs.
 *
 *  The 2026-08-13 units re-cut (blender export 0.4.1, ×0.1 to true meters) was
 *  applied to fixtures.json and NEVER to the companion glbs. Measured 08-15:
 *  tree-context.glb spans 100.66 m and chandelier.glb 15 m against a 10.05 m
 *  fixture cloud — a ratio of 10.02, i.e. exactly the old units. The twin had
 *  been rendering true-scale fixtures inside a 10× tree for two days, which is
 *  what every "camera is inside the forest" symptom actually was. Nobody
 *  perception-checked RELATIVE scale after the re-cut; each file looked fine
 *  alone.
 *
 *  This guard corrects exactly that one defect and nothing else:
 *  - ratio > STALE_RATIO → the glb is in old units → scale by ×0.1.
 *  - otherwise → leave it alone.
 *
 *  Deliberately NOT a general auto-fit (scale = fixtures/glb): the structure
 *  legitimately extends past the lights (canopy, guys — Ed's 22 m guy-spread
 *  trap), so a continuous fit would silently shrink a CORRECT re-export by
 *  ~2×. A binary known-defect correction cannot: when blender ships the glbs
 *  re-cut to true meters, the ratio drops under the threshold and this becomes
 *  a no-op on the same commit. Re-export requested from blender 2026-08-15. */

export const STALE_RATIO = 5;
export const OLD_UNITS_SCALE = 0.1;

export function staleUnitsScale(glbSpan: number, fixtureSpan: number): number {
  if (!Number.isFinite(glbSpan) || !Number.isFinite(fixtureSpan)) return 1;
  if (glbSpan <= 0 || fixtureSpan <= 0) return 1;
  return glbSpan / fixtureSpan > STALE_RATIO ? OLD_UNITS_SCALE : 1;
}
