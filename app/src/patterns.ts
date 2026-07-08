import { Color } from "three";
import type { AudioFeatures } from "./audio";
import type { Control, SimFixture } from "./store";
import { quantizedStep } from "./beat";
import { themeMapHue } from "./field";

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

// addressable-light helper: fold ANY number into 1..72. Elliot's rule — last two
// digits, then wrap the 73..99 remainder: 101→1, 112→12, 7747→47, 89→17.
export const into72 = (n: number) => { const m = n % 100 || 100; return m > 72 ? m - 72 : m; };
// Fibonacci numbers folded into 1..72 — a generative "interesting" light sequence
const FIB72: number[] = (() => {
  const out: number[] = [];
  let a = 1, b = 1;
  for (let i = 0; i < 24; i++) { out.push(into72(a)); const n = a + b; a = b; b = n; }
  return out;
})();

// tiny 8×8 1-bit images (8 bytes each, MSB = left pixel) to "play" on the canopy —
// mapped by azimuth × height, scrolling round the trunk (the ≤100-byte graphics idea)
const GLYPHS: number[][] = [
  [0x00, 0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18], // heart
  [0x10, 0x38, 0xFE, 0x7C, 0x38, 0x7C, 0x6C, 0xC6], // star
  [0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C], // smiley
  [0x18, 0x3C, 0x7E, 0xFF, 0x18, 0x18, 0x18, 0x18], // arrow up
];

/** Base-hue MOTION: returns the effective hue for this frame given the picked
 *  hue + the colour-cycle mode. Most patterns build their palette off this hue,
 *  so it tints whatever pattern is running.
 *   rainbow — sweep through ALL hues continuously
 *   group   — drift ±0.13 (≈3 adjacent families: warm red/orange/yellow, etc.)
 *   shade   — drift ±0.045 (the shades of just the one picked colour) */
export function cycledHue(base: number, mode: Control["colorCycle"], t: number, speed: number, seed = 0): number {
  switch (mode) {
    case "rainbow": return frac(base + t * speed * 0.06);
    case "group": return frac(base + 0.13 * Math.sin(t * speed * 0.5));
    case "shade": return frac(base + 0.045 * Math.sin(t * speed * 0.6));
    // each light decides its OWN colour — its own phase offset + its own drift
    // rate (seeded by the fixture's stable random) so they desync into a field
    case "independent": return frac(base + seed + t * speed * 0.045 * (0.4 + seed));
    default: return base;
  }
}

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
  // colour layer (per-fixture so "independent" lets each light pick its own) —
  // applied to the base hue before the pattern; patterns that add to hue inherit it
  let hue = c.colorCycle && c.colorCycle !== "off" ? cycledHue(c.hue, c.colorCycle, t, sp, f.rnd) : c.hue;
  let sat = c.sat;

  switch (c.pattern) {
    case "solid":
      break;
    case "shockwave": {
      // SHOCKWAVE (Elliot): a physical front — sharp leading edge, decaying wake,
      // one reflected pass — from a NEW random origin every wave, so it never
      // repeats. Distance is measured in normalized cylinder space.
      const period = 6 / Math.max(0.15, sp);
      const k = Math.floor(t / period); // wave index
      const ph = t / period - k; // 0..1 through this wave
      const h1 = frac(Math.sin(k * 127.1) * 43758.5453); // per-wave origin (stable for the wave)
      const h2 = frac(Math.sin(k * 311.7) * 12543.853);
      const oa = h1 * Math.PI * 2 - Math.PI, oh = h2;
      const dx = Math.cos(f.azimuth) * f.radialT - Math.cos(oa) * 0.9;
      const dz = Math.sin(f.azimuth) * f.radialT - Math.sin(oa) * 0.9;
      const dy = (f.heightT - oh) * 1.4;
      const d = Math.sqrt(dx * dx + dz * dz + dy * dy); // 0..~2.6
      const R = 2.7;
      const front = ph * 1.35 * R; // outbound front…
      const refl = 2 * R - front; // …and its reflection off the far edge
      const edge = (r: number, w: number, gain: number) => {
        const lead = Math.exp(-((d - r) * (d - r)) / (w * w)); // sharp leading edge
        const tail = d < r ? Math.exp(-(r - d) * 2.2) * 0.45 : 0; // decaying wake
        return gain * (lead + tail);
      };
      const e = edge(front, 0.14, 1) + (refl < R ? edge(refl, 0.2, 0.5) : 0);
      bri *= Math.min(1.3, e);
      hue = frac(hue + d * 0.06 + k * 0.13); // each wave lands a shifted colour
      break;
    }
    case "hurricane": {
      // HURRICANE (Elliot): a swirling vortex that WANDERS around the tree and
      // slowly reverses spin — non-commensurate drift frequencies mean the path
      // never retraces (no repetitive patterning).
      const w = tt * (0.25 + sp * 0.5);
      const wander = Math.sin(t * 0.071) * 1.7 + Math.sin(t * 0.0233 + 1.7) * 1.1; // eye azimuth
      const eyeH = 0.5 + 0.38 * Math.sin(t * 0.043 + 2.1); // eye height drifts too
      const spin = Math.sin(t * 0.017) >= 0 ? 1 : -1; // slow direction changes
      let da = f.azimuth - wander;
      da = Math.atan2(Math.sin(da), Math.cos(da)); // wrap
      const dh = (f.heightT - eyeH) * 2.2;
      const dEye = Math.sqrt(da * da * 0.8 + dh * dh);
      const arm = Math.sin(da * 3 * spin + f.heightT * 7 - w * 6 * spin + dEye * 4); // spiral arms
      const band = Math.exp(-dEye * 0.9);
      const eye = Math.exp(-dEye * dEye * 6); // calm bright eye at the centre
      bri *= Math.min(1.25, Math.max(0, 0.12 + band * (0.45 + 0.55 * arm) + eye * 0.5));
      hue = frac(hue + dEye * 0.09 + spin * 0.02);
      break;
    }
    case "sequence": {
      const N = Math.max(1, n);
      // QUANTIZER (P0-3): when synced + a tempo is locked, derive the step from
      // the PLL beat grid (quantizedStep) so step boundaries land exactly on the
      // 1/beatDiv grid — the chase/sequence hits ON the beat, not floating off
      // wall-clock. Falls back to the free-running stepMs clock when no audio.
      const synced = c.syncToBeat && audio.active && audio.bpm > 0;
      const step = synced
        ? quantizedStep(audio.beatTime, c.beatDiv)
        : Math.floor((t * 1000) / Math.max(20, c.stepMs));
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
      // a lit band travels AROUND the tree (azimuth order); random Mode shuffles
      // which light fires when (seeded per fixture) instead of going in order
      const head = frac(tt * sp * 0.25);
      const cpos = c.order === "random" ? f.rnd : f.seqT;
      let d = Math.abs(cpos - head);
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
    case "rings": {
      // THREE CONCENTRIC RINGS (Elliot): each ring rotates azimuthally in the
      // OPPOSITE direction to its neighbours, with a radial IN/OUT wave so the
      // light also travels toward + away from the trunk. The deck/jog `reverse`
      // flips tt → flips every ring's direction together (directional control).
      const ringDir = f.ring % 2 === 0 ? 1 : -1;          // opposite concentric spin
      const spin = f.seqT + tt * sp * 0.16 * ringDir;     // each ring spins its own way
      // radial wavefront travelling in/out across the rings over time
      const radial = 0.5 + 0.5 * Math.sin((f.radialT * 2.2 - tt * sp * 0.5) * Math.PI * 2);
      // a bright sector chasing around each ring (azimuthal head)
      const head = 0.5 + 0.5 * Math.cos(spin * Math.PI * 2);
      hue = frac(c.hue + f.ring / 3 + spin * 0.5);        // each ring its own colour band
      sat = Math.max(sat, 0.9);
      bri *= 0.12 + 0.6 * radial + 0.5 * head;            // in/out pulse × rotating head
      if (audio.active) bri *= 0.6 + 0.8 * (f.ring === 0 ? audio.bass : f.ring === 1 ? audio.mid : audio.treble);
      break;
    }
    case "fibonacci": {
      // GENERATIVE: walk the Fibonacci sequence across the numbered lights (1..72).
      // A light glows when its number is the active fib value, with a fading tail,
      // so 1,2,3,5,8,13,21… light up in turn and hop around the tree. Big fib
      // numbers fold back in via into72 (101→1, 112→12, 7747→47).
      const head = tt * sp * 1.1; // fractional step pointer (rides the speed dial)
      const TAIL = 4; // how many recent steps still glow
      const L = FIB72.length;
      let glow = 0;
      for (let kk = 0; kk < TAIL; kk++) {
        const idx = Math.floor(head) - kk;
        if (idx < 0) continue;
        if (FIB72[((idx % L) + L) % L] === f.num) {
          glow = Math.max(glow, 1 - kk / TAIL - frac(head) / TAIL);
        }
      }
      bri *= 0.03 + 0.97 * Math.max(0, glow);
      hue = frac(hue + f.num / 72); // each number its own tint
      break;
    }
    case "sweep": {
      // a band of light travels LEFT→RIGHT across the tree (world X via norm[0]);
      // `reverse` flips it right→left. Hue runs as a gradient across the width, so
      // a blue base (0.6) reads blue→red left→right — the "blue to red" sweep.
      const head = frac(tt * sp * 0.25);
      const x = c.order === "random" ? f.rnd : f.norm[0]; // 0 = left, 1 = right (random shuffles)
      let d = Math.abs((((x - head) % 1) + 1) % 1);
      d = Math.min(d, 1 - d); // wrap so the band re-enters from the left
      bri *= 0.05 + 0.95 * Math.max(0, 1 - d * 7);
      hue = frac(hue + x * 0.4); // colour gradient across the width
      break;
    }
    case "aurora": {
      // Perlin-ish plasma flow → green/violet curtains drifting up the tree (no strobe)
      const n1 = Math.sin(f.norm[0] * 4 + tt * sp * 0.3)
        + Math.sin(f.norm[2] * 5 - tt * sp * 0.22)
        + Math.sin(f.heightT * 7 + tt * sp * 0.18);
      bri *= 0.18 + 0.82 * (0.5 + 0.5 * Math.sin(n1 * 1.3));
      hue = frac(0.45 + 0.22 * Math.sin(n1) + f.heightT * 0.15); // green↔violet, drifts up
      sat = Math.max(sat, 0.8);
      break;
    }
    case "chladni": {
      // standing-wave (Chladni) mandala — antinodes bright; modes morph slowly
      const m = 2 + Math.floor(3 * (0.5 + 0.5 * Math.sin(t * sp * 0.08)));
      const nn = 1 + Math.floor(3 * (0.5 + 0.5 * Math.cos(t * sp * 0.06)));
      const az = f.seqT * Math.PI * 2;
      const val = Math.abs(Math.sin(m * az) * Math.sin(nn * Math.PI * f.heightT));
      bri *= 0.05 + 0.95 * Math.pow(val, 1.4);
      break;
    }
    case "glyph": {
      // tiny 8×8 1-bit images mapped onto the canopy (azimuth × height), scrolling
      const g = GLYPHS[Math.floor(t * sp * 0.04) % GLYPHS.length];
      const u = frac(f.seqT + t * sp * 0.03); // scroll around the trunk
      const col = Math.min(7, Math.floor(u * 8));
      const row = Math.min(7, Math.floor((1 - f.heightT) * 8));
      const on = (g[row] >> (7 - col)) & 1;
      bri *= on ? 1 : 0.025;
      break;
    }
    case "interference": {
      // two wave sources orbiting the trunk → ripple-tank interference shimmer
      const s1x = 0.5 + 0.35 * Math.sin(tt * sp * 0.2), s1y = 0.6, s1z = 0.5 + 0.35 * Math.cos(tt * sp * 0.17);
      const s2x = 0.5 + 0.35 * Math.cos(tt * sp * 0.14), s2y = 0.4, s2z = 0.5 + 0.35 * Math.sin(tt * sp * 0.22);
      const d1 = Math.hypot(f.norm[0] - s1x, f.norm[1] - s1y, f.norm[2] - s1z);
      const d2 = Math.hypot(f.norm[0] - s2x, f.norm[1] - s2y, f.norm[2] - s2z);
      const w = Math.sin(d1 * 22 - tt * sp * 1.2) + Math.sin(d2 * 22 - tt * sp * 1.2);
      bri *= 0.12 + 0.88 * Math.pow(0.5 + 0.25 * w, 1.5);
      hue = frac(hue + 0.08 * w);
      break;
    }
    case "lissajous": {
      // a light orbiter traces a Lissajous figure through the canopy (Gaussian glow)
      const ph = tt * sp * 0.3;
      const px = 0.5 + 0.42 * Math.sin(ph * 2), py = 0.5 + 0.42 * Math.sin(ph * 3 + 1), pz = 0.5 + 0.42 * Math.sin(ph * 2.5);
      const d2 = (f.norm[0] - px) ** 2 + (f.norm[1] - py) ** 2 + (f.norm[2] - pz) ** 2;
      bri *= 0.02 + 0.98 * Math.exp(-d2 * 12); // near-dark except the moving glow
      hue = frac(hue + ph * 0.05);
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
      // rainbow barber-pole spiralling UP the tree: azimuth + height form a spiral
      // coordinate; bright stripes wrap up it and rotate, so the spiral is VISIBLE
      // (was hue-only before → just a flat colour wash with no readable motion).
      const travel = tt * sp * 0.09;              // slow continuous wrap around the trunk
      const sway = 0.35 * Math.sin(t * sp * 0.45); // AND a back-and-forth sweep
      const s = frac(f.seqT + f.heightT * 1.5 - travel - sway); // spiral phase 0..1
      const arms = 3; // bright stripes wrapping the trunk
      const band = Math.pow(0.5 + 0.5 * Math.cos(frac(s * arms) * 6.283), 5);
      bri *= 0.12 + 0.9 * band; // dark between arms → the spiral reads crisply
      hue = frac(hue + s); // rainbow runs along the spiral
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

// pull an RGB colour into the active theme's hue world (identity when no theme).
// Whites/greys pass through untouched — the theme constrains COLOUR, not shade.
const themeCol = new Color();
const themeHsl = { h: 0, s: 0, l: 0 };
export function applyThemeToLit(o: Lit, hues?: number[] | null) {
  const mx = Math.max(o.r, o.g, o.b);
  if (mx <= 0.001) return;
  const k = mx > 1 ? 1 / mx : 1; // getHSL needs 0..1; keep the overdrive scale
  themeCol.setRGB(o.r * k, o.g * k, o.b * k).getHSL(themeHsl);
  if (themeHsl.s < 0.08) return; // near-white stays white (tameWhite handles level)
  const h2 = themeMapHue(themeHsl.h, hues);
  if (h2 === themeHsl.h) return; // no theme active — free
  themeCol.setHSL(h2, themeHsl.s, themeHsl.l);
  o.r = themeCol.r / k; o.g = themeCol.g / k; o.b = themeCol.b / k;
}

// near-white above this level gets scaled DOWN to it (Elliot: "white low
// brightness can be ok, but not full brightness") — the beacon is exempt.
export const WHITE_CAP = 0.5;
export function tameWhite(o: Lit) {
  const mx = Math.max(o.r, o.g, o.b);
  if (mx <= WHITE_CAP) return; // already a low glow
  const mn = Math.min(o.r, o.g, o.b);
  if ((mx - mn) / mx > 0.3) return; // clearly a colour, not white
  const k = WHITE_CAP / mx;
  o.r *= k; o.g *= k; o.b *= k;
}
