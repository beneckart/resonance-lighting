export interface Ripple {
  x: number;
  y: number;
  z: number;
  t0: number; // performance.now()/1000 at trigger
  // TRIGGER RESPONSE (rules editor): what the "sensor firing here" looks like.
  hue?: number; // reaction colour 0..1 (undefined = don't tint, just brighten)
  intensity?: number; // brightness-boost multiplier at the wavefront (default ~2.2)
  spread?: number; // radius/speed scale — how far/fast the disturbance rolls (default 1)
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
