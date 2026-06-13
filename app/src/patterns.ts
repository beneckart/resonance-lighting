import { Color } from "three";
import type { AudioFeatures } from "./audio";
import type { Control, SimFixture } from "./store";
import { beatStepMs } from "./beat";

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
export function litFor(t: number, f: SimFixture, c: Control, audio: AudioFeatures, n: number, out: Lit) {
  // jog-wheel direction: reverse the around-the-tree motion. `t` drives temporal
  // motion; `tt` is the directional time used by azimuthal/orbiting patterns.
  const dir = c.reverse ? -1 : 1;
  const tt = t * dir;
  const sp = c.speed;
  let bri = c.brightness;
  let hue = c.hue;
  let sat = c.sat;

  switch (c.pattern) {
    case "solid":
      break;
    case "sequence": {
      const N = Math.max(1, n);
      // snap the step to the beat when synced + a tempo is detected
      const stepMs =
        c.syncToBeat && audio.bpm > 0 ? beatStepMs(audio.bpm, c.beatDiv) : c.stepMs;
      const step = Math.floor((t * 1000) / Math.max(20, stepMs));
      const r = f.seq;
      const mod = (a: number, m: number) => ((a % m) + m) % m;
      let on = false;
      switch (c.seqMode) {
        case "allOn":
          on = true;
          break;
        case "allOff":
          on = false;
          break;
        case "single": // one light travels around the tree
          on = mod(step, N) === r;
          break;
        case "everyN": // every 2nd / 4th, phase animates
          on = mod(r, c.everyN) === mod(step, c.everyN);
          break;
        case "groups": { // consecutive blocks of groupSize light in sequence
          const G = Math.max(1, c.groupSize);
          const groups = Math.ceil(N / G);
          on = Math.floor(r / G) === mod(step, groups);
          break;
        }
        case "snake": { // two heads sweep OUT from a center point, then back
          const center = Math.floor(N / 2);
          const half = Math.max(1, Math.floor(N / 2));
          const period = 2 * half;
          const p = mod(step, period);
          const off = p <= half ? p : period - p; // 0..half..0
          on = r === mod(center + off, N) || r === mod(center - off, N);
          break;
        }
        case "fill":
        default: { // fill on one-after-another, then recede ("move back"), then dark
          const cycle = 2 * N;
          const p = mod(step, cycle);
          on = p < N ? r <= p : r > p - N;
          break;
        }
      }
      bri = on ? bri : c.seqMode === "allOff" ? 0 : bri * 0.03;
      break;
    }
    case "breathe":
      bri *= 0.25 + 0.75 * (0.5 + 0.5 * Math.sin(t * sp * 1.5));
      break;
    case "chase": {
      // a lit band travels AROUND the tree (azimuth order)
      const head = frac(tt * sp * 0.25);
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
    case "spectrum": {
      // full-spectrum rainbow wrapped around the tree, slowly rotating
      hue = frac(f.seqT + tt * sp * 0.08);
      break;
    }
    case "tricolor": {
      // TRICOLOR ORBIT (the "three-colour dancing" projection): three triad
      // hues (120° apart) in crisp azimuthal sectors that ORBIT the tree, each
      // sector pulsing out of phase + driven by its own frequency band so the
      // three colours dance against each other.
      const orbit = tt * sp * 0.18; // whole triad rotates around the trunk
      const slot = Math.floor(frac(f.seqT + orbit) * 3) % 3; // sector 0/1/2
      hue = frac(c.hue + slot / 3);
      sat = Math.max(sat, 0.92); // keep the three colours vivid + distinct
      bri *= 0.5 + 0.5 * Math.sin(t * sp * 2.4 - slot * 2.0944); // phase-offset pulse
      if (audio.active) {
        const band = slot === 0 ? audio.bass : slot === 1 ? audio.mid : audio.treble;
        bri *= 0.55 + 0.85 * band; // each colour dances to its own band
      }
      break;
    }
    case "chromatic": {
      // CHROMATIC TRICOLOR (Elliot's ask): three distinct colours moving OUTWARD
      // from ONE source point — the crown (the chandelier hangs there). Each
      // triad hue is a travelling wavefront; the three are phase-offset by 1/3 so
      // they separate as they propagate, reading as three colours streaming out
      // from a single origin. u = radial distance from the source (0 at crown).
      const u = 1 - f.heightT; // crown(=1) → base(=0): distance from the top source
      const flow = tt * sp * 0.22; // wavefronts travel outward over time
      const width = 0.26; // band thickness
      let best = -1, bestHue = hue, bestBri = 0;
      for (let k = 0; k < 3; k++) {
        const w = frac(flow + k / 3); // this colour's wavefront position
        let d = Math.abs(u - w);
        d = Math.min(d, 1 - d); // wrap so the ring is continuous
        let inten = smooth(width, 0, d); // bright when the fixture sits on the front
        if (audio.active) {
          const bandK = k === 0 ? audio.bass : k === 1 ? audio.mid : audio.treble;
          inten *= 0.5 + 0.9 * bandK; // each colour pulses to its own band
        }
        if (inten > best) { best = inten; bestHue = frac(c.hue + k / 3); bestBri = inten; }
      }
      hue = bestHue;
      sat = Math.max(sat, 0.95); // keep the three colours vivid + distinct
      bri *= 0.12 + 0.95 * bestBri; // dark between wavefronts → the bands read crisply
      break;
    }
    case "spiral": {
      // rainbow barber-pole spiralling UP the tree (azimuth + height), rotating
      hue = frac(f.seqT + f.heightT * 1.5 + tt * sp * 0.1);
      break;
    }
    case "godray": {
      // four tight light SHAFTS sweeping around the tree (rotating azimuth sectors)
      const g = frac(f.seqT * 4 - tt * sp * 0.4);
      bri *= 0.08 + 0.92 * Math.pow(0.5 + 0.5 * Math.cos(g * 6.283), 8);
      break;
    }
    case "rising": {
      // a band of light climbs trunk → canopy ("rising sap")
      const wave = frac(t * sp * 0.25);
      const d = Math.abs(f.heightT - wave);
      bri *= 0.08 + 0.92 * Math.max(0, 1 - d * 5);
      hue = frac(hue + f.heightT * 0.1);
      break;
    }
    case "planewipe": {
      // a flat sheet of light sweeps through the volume at a rotating angle
      const ang = t * sp * 0.3;
      const proj = (f.norm[0] - 0.5) * Math.cos(ang) + (f.norm[2] - 0.5) * Math.sin(ang);
      const sweep = -0.7 + 1.4 * frac(t * sp * 0.15);
      bri *= 0.08 + 0.92 * Math.max(0, 1 - Math.abs(proj - sweep) * 5);
      break;
    }
    case "warmcool": {
      // warm trunk / cool canopy with a breathing split boundary (reads as depth)
      const split = 0.5 + 0.3 * Math.sin(t * sp * 0.5);
      const w = smooth(split - 0.15, split + 0.15, f.heightT);
      hue = frac(0.08 + w * 0.5); // amber → cyan
      sat = Math.max(sat, 0.7);
      break;
    }
    case "bloom": {
      // soft radial bloom from the trunk outward (saturation swells with audio)
      const dx = f.norm[0] - 0.5, dz = f.norm[2] - 0.5;
      const rad = Math.sqrt(dx * dx + dz * dz);
      bri *= 0.35 + 0.65 * (0.5 + 0.5 * Math.sin(t * sp * 1.2 - rad * 6));
      hue = frac(hue + rad * 0.15);
      break;
    }
    case "firefly": {
      // fireflies: sparse warm blinks at per-fixture phase that snap together on the beat
      const sync = audio.active ? audio.beat : 0;
      const ph = frac(t * sp * 0.25 + f.rnd * (1 - sync));
      const blink = Math.pow(0.5 + 0.5 * Math.sin(ph * 6.283), 8);
      bri *= 0.04 + 0.96 * Math.max(blink, sync * 0.6);
      hue = frac(0.12 + f.rnd * 0.03); // warm yellow-green
      break;
    }
    case "ca": {
      // cellular field: interfering sine lattices → shifting blobs (GoL / reaction-diffusion feel)
      const v = Math.sin(f.norm[0] * 8 + t * sp) * Math.sin(f.norm[2] * 8 - t * sp * 0.7) * Math.sin(f.heightT * 6 + t * sp * 0.4);
      bri *= 0.1 + 0.9 * smooth(-0.1, 0.4, v);
      hue = frac(hue + v * 0.2);
      break;
    }
    case "hero": {
      // SOUND-REACTIVE HERO — the whole tree as one organism (Radial Sonic
      // Runway): an energy wavefront radiates from the trunk outward, brightness
      // driven by the live audio (bass swell + beat flash + drop burst come from
      // the global audio layer below); idles with a slow breath when silent.
      const dx = f.norm[0] - 0.5, dz = f.norm[2] - 0.5;
      const rad = Math.sqrt(dx * dx + dz * dz) * 2; // 0..~1.4 outward from trunk
      const e = audio.active ? audio.level : 0.5 + 0.5 * Math.sin(t * sp); // energy
      const wave = 0.5 + 0.5 * Math.sin(rad * 5 - t * sp * 3 - e * 4); // outward-travelling front
      bri *= 0.15 + 0.85 * wave * (0.4 + 0.9 * e);
      hue = frac(c.hue + rad * 0.25 + (audio.active ? audio.bass * 0.12 : 0));
      sat = Math.max(sat, 0.85);
      break;
    }
    case "plasma": {
      // classic PLASMA field — the GLSL fragment-shader math, evaluated per
      // fixture on the CPU (Tier-1 #3 first piece): interfering sine waves over
      // position + a radial term → flowing hue + brightness.
      const x = f.norm[0] * 6, z = f.norm[2] * 6;
      const v =
        Math.sin(x + t * sp) +
        Math.sin(z * 0.8 - t * sp * 0.7) +
        Math.sin((x + z) * 0.5 + t * sp * 0.5) +
        Math.sin(Math.hypot(x - 3, z - 3) - t * sp); // ~ -4..4
      hue = frac(c.hue + (v + 4) / 8);
      bri *= 0.45 + 0.55 * (0.5 + 0.5 * Math.sin(v * 1.5 + t * sp));
      sat = Math.max(sat, 0.85);
      break;
    }
    // --- element modes (D4) ---
    case "wind": {
      const w = Math.sin(f.seqT * 6.283 * 2 - t * sp * 0.9) * (0.6 + 0.4 * Math.sin(t * sp * 0.3));
      bri *= 0.45 + 0.4 * (0.5 + 0.5 * w);
      hue = frac(0.33 + 0.06 * w); // cool green-teal sway
      break;
    }
    case "ember": {
      hue = frac(0.03 + f.rnd * 0.05); // deep warm orange/red
      const flick = (0.5 + 0.5 * Math.sin(t * sp * 6 + f.rnd * 30)) * (0.6 + 0.4 * Math.sin(t * sp * 2.3 + f.rnd * 10));
      bri *= 0.22 + 0.6 * flick;
      break;
    }
    case "rain": {
      hue = frac(0.55 + 0.05 * Math.sin(t + f.rnd * 6)); // silver-blue
      const fall = frac(t * sp * 0.45 + f.rnd);
      const d = Math.abs(1 - f.heightT - fall);
      bri *= 0.08 + 0.92 * Math.max(0, 1 - d * 6);
      break;
    }
    case "beacon": {
      const head = frac(t * sp * 0.3);
      let d = Math.abs(f.seqT - head);
      d = Math.min(d, 1 - d);
      const swept = 1 - smooth(0.0, 0.18, d);
      sat *= 0.15; // near-white whiteout safety beam
      bri *= 0.15 + 0.95 * swept;
      break;
    }
  }

  if (audio.active) {
    bri *= 0.4 + 0.6 * audio.level; // level → overall brightness
    bri *= 1 + 0.5 * audio.bass; // bass swell
    bri += 0.45 * audio.beat; // beat flash (reactive — fires AT the onset)
    bri += 0.18 * (audio.beatPulse || 0); // PLL on-grid swell (predictive — pumps ON the beat, fills gaps between onsets)
    bri = bri * (1 - audio.drop) + audio.drop; // DROP → burst all to full
    hue = frac(hue + audio.treble * 0.15); // highs shift hue
  }

  bri = clamp01(bri);
  col.setHSL(frac(hue), sat, 0.5);
  out.r = col.r * bri;
  out.g = col.g * bri;
  out.b = col.b * bri;
}
