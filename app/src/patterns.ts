import { Color } from "three";
import type { AudioFeatures } from "./audio";
import type { Control, SimFixture } from "./store";

export interface Lit {
  r: number;
  g: number;
  b: number;
}

const col = new Color();
const frac = (x: number) => x - Math.floor(x);
const clamp01 = (x: number) => Math.min(1, Math.max(0, x));
const smooth = (e0: number, e1: number, x: number) => {
  const t = clamp01((x - e0) / (e1 - e0));
  return t * t * (3 - 2 * t);
};

/** Compute a fixture's REPORTED color (brightness pre-applied) from the commanded
 *  control + audio + time. This is the firmware stand-in; the renderer only shows
 *  the result (the mirror rule). */
export function litFor(t: number, f: SimFixture, c: Control, audio: AudioFeatures, out: Lit) {
  const sp = c.speed;
  let bri = c.brightness;
  let hue = c.hue;
  const sat = c.sat;

  switch (c.pattern) {
    case "solid":
      break;
    case "breathe":
      bri *= 0.25 + 0.75 * (0.5 + 0.5 * Math.sin(t * sp * 1.5));
      break;
    case "chase": {
      // a lit band travels AROUND the tree (azimuth order)
      const head = frac(t * sp * 0.25);
      let d = Math.abs(f.seqT - head);
      d = Math.min(d, 1 - d); // wrap-around
      bri *= 1 - smooth(0.0, 0.12, d);
      break;
    }
    case "ripple": {
      const dx = f.norm[0] - 0.5;
      const dy = f.norm[1] - 0.5;
      const dz = f.norm[2] - 0.5;
      const r = Math.sqrt(dx * dx + dy * dy + dz * dz);
      bri *= 0.2 + 0.8 * (0.5 + 0.5 * Math.sin(r * 10 - t * sp * 3));
      hue = frac(hue + r * 0.5);
      break;
    }
    case "sparkle": {
      const ph = frac(t * sp * 0.5 + f.rnd);
      bri *= 0.08 + Math.pow(0.5 + 0.5 * Math.sin(ph * Math.PI * 2), 6);
      hue = frac(hue + f.rnd * 0.12);
      break;
    }
  }

  if (audio.active) {
    bri *= 0.4 + 0.6 * audio.level; // level → overall brightness
    bri *= 1 + 0.5 * audio.bass; // bass swell
    bri += 0.45 * audio.beat; // beat flash
    hue = frac(hue + audio.treble * 0.15); // highs shift hue
  }

  bri = clamp01(bri);
  col.setHSL(frac(hue), sat, 0.5);
  out.r = col.r * bri;
  out.g = col.g * bri;
  out.b = col.b * bri;
}
