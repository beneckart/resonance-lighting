export interface Ripple {
  x: number;
  y: number;
  z: number;
  t0: number; // performance.now()/1000 at trigger
}

/**
 * Brightness contribution of an expanding ripple wavefront at a fixture `dist` away,
 * `age` seconds after the trigger. The lit band is at radius = age*speed; the whole
 * ripple fades over ~2s. This is the mesh-choreography: presence at one point → a wave
 * rolling outward across the tree (PRESENCE_SENSING doc).
 */
export function rippleIntensity(dist: number, age: number, speed: number, width: number): number {
  if (age < 0) return 0;
  const front = age * speed;
  const band = Math.max(0, 1 - Math.abs(dist - front) / width);
  const fade = Math.max(0, 1 - age * 0.5);
  return band * band * fade;
}
