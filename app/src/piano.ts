import type { SimFixture } from "./store";

/** THE CANOPY AS A PIANO. The ~72 canopy ("high") lights are laid out by azimuth
 *  as a 6-octave keyboard (MIDI 36..107), one light per key. A score plays; each
 *  sounding note lights its key with a piano attack/decay envelope. DYNAMICS:
 *   • octave doubling — a note also lights its pitch in neighbouring octaves
 *     (falling off), so far more of the canopy illuminates and chords read big.
 *   • colour by VOICE + velocity — bass = deep indigo, arpeggio = blue→cyan by
 *     pitch, the melody pops warm gold. Brightness tracks note velocity.
 *  keyBri/keyHue/keySat[midi] hold each key's live state; TreeLights renders them. */
const BASE_MIDI = 36;

let fixKey = new Int16Array(0);
let mappedN = -1;
export const keyBri = new Float32Array(128);
export const keyHue = new Float32Array(128);
export const keySat = new Float32Array(128);
let t0 = -1;

export function mapKeys(fixtures: SimFixture[]) {
  const n = fixtures.length;
  fixKey = new Int16Array(n).fill(-1);
  const canopy = fixtures.map((f, i) => ({ i, f })).filter((x) => x.f.zone === "high");
  canopy.sort((a, b) => a.f.seqT - b.f.seqT);
  canopy.forEach((x, k) => { fixKey[x.i] = BASE_MIDI + Math.min(71, k); });
  mappedN = n;
}
export function fixtureMidi(fixtures: SimFixture[], i: number): number {
  if (mappedN !== fixtures.length) mapKeys(fixtures);
  return fixKey[i];
}

// voice: 0 = arpeggio, 1 = bass, 2 = melody
interface PNote { midi: number; t: number; dur: number; vel: number; voice: number }
const TRI = 0.34;
const BAR = 12 * TRI;
const A = [56, 61, 64], B = [57, 61, 64], C = [57, 62, 66], D = [56, 60, 64], E = [55, 59, 64];
const F = [54, 57, 61], G = [51, 56, 59];
const BARS: { g: number[][]; bass: number[] }[] = [
  { g: [A, A, A, A], bass: [37, 49] }, { g: [A, A, A, A], bass: [37, 49] },
  { g: [B, B, C, C], bass: [45, 57] }, { g: [D, D, A, A], bass: [44, 56] },
  { g: [A, A, A, A], bass: [37, 49] }, { g: [A, A, A, A], bass: [37, 49] },
  { g: [B, B, C, C], bass: [45, 57] }, { g: [E, E, A, A], bass: [44, 56] },
  { g: [A, A, A, A], bass: [37, 49] }, { g: [B, B, C, C], bass: [45, 57] },
  { g: [D, D, A, A], bass: [44, 56] }, { g: [A, A, A, A], bass: [37, 49] },
  { g: [F, F, F, F], bass: [42, 54] }, { g: [G, G, G, G], bass: [44, 56] },
  { g: [A, A, A, A], bass: [37, 49] }, { g: [D, D, A, A], bass: [44, 56] },
];
export const SCORE_LEN = BARS.length * BAR;

const NOTES: PNote[] = (() => {
  const out: PNote[] = [];
  for (let b = 0; b < BARS.length; b++) {
    const bar = BARS[b];
    for (let grp = 0; grp < 4; grp++) {
      const trio = bar.g[grp];
      for (let i = 0; i < 3; i++) out.push({ midi: trio[i], t: b * BAR + grp * 3 * TRI + i * TRI, dur: TRI * 1.1, vel: 0.45, voice: 0 });
    }
    for (const bm of bar.bass) out.push({ midi: bm, t: b * BAR, dur: BAR * 0.98, vel: 0.6, voice: 1 });
  }
  const m = (midi: number, bar: number, off: number, dur: number, vel = 0.95): PNote => ({ midi, t: bar * BAR + off, dur, vel, voice: 2 });
  out.push(
    m(68, 4, 0, 3.2), m(68, 5, 0, 1.7), m(69, 5, 1.9, 1.0), m(68, 6, 0, 1.7), m(66, 6, 1.9, 1.0), m(64, 7, 0, 2.6),
    m(64, 8, 0, 3.0), m(63, 9, 0, 1.6), m(61, 9, 1.9, 1.4), m(61, 10, 0, 3.0), m(59, 11, 0, 2.6),
    m(61, 12, 0, 3.0), m(59, 13, 0, 1.6), m(56, 13, 1.9, 1.4), m(56, 14, 0, 3.0), m(56, 15, 0, 2.6),
  );
  return out;
})();

function colorFor(voice: number, midi: number): [number, number] { // [hue, sat]
  if (voice === 1) return [0.74, 0.7];            // bass — deep indigo
  if (voice === 2) return [0.10, 0.4];            // melody — warm gold, whiter (pops)
  const frac = (midi - 36) / 71;                  // arpeggio — blue→cyan by pitch
  return [0.62 - frac * 0.18, 0.6];
}

export function resetPiano() { t0 = -1; }

/** Advance the piano and refresh keyBri/keyHue/keySat. now = performance.now()/1000. */
export function updatePiano(now: number) {
  if (t0 < 0) t0 = now;
  const tt = (now - t0) % SCORE_LEN;
  keyBri.fill(0);
  const OCT = [0.3, 0.5, 0.72, 1, 0.72, 0.5, 0.3]; // octave-doubling falloff for o = -3..+3
  for (const nt of NOTES) {
    const dt = tt - nt.t;
    if (dt < 0 || dt > nt.dur + 1.4) continue;
    const env = dt < 0.025 ? dt / 0.025 : Math.exp(-(dt - 0.025) / Math.max(0.28, nt.dur * 0.7));
    const base = nt.vel * env;
    for (let o = -3; o <= 3; o++) {
      const mm = nt.midi + o * 12;
      if (mm < 36 || mm > 107) continue;
      const v = base * OCT[o + 3];
      if (v > keyBri[mm]) {
        keyBri[mm] = v;
        const [h, s] = colorFor(nt.voice, nt.midi);
        keyHue[mm] = h; keySat[mm] = s;
      }
    }
  }
}
