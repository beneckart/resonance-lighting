import type { SimFixture } from "./store";

/** THE CANOPY AS A PIANO. The ~72 canopy ("high") lights are laid out by azimuth
 *  as a linear 72-key keyboard (6 octaves, MIDI 36..107); each light = one key.
 *  A score (Moonlight Sonata 1st mvt, opening) plays: when a note sounds, its key's
 *  light rises with a piano attack and falls with a decay envelope. keyBri[midi]
 *  holds each key's live 0..1 level; TreeLights renders it on the mapped fixtures. */
const BASE_MIDI = 36; // C2 → key 0

let fixKey = new Int16Array(0); // per-fixture MIDI note, -1 if not a key
let mappedN = -1;
export const keyBri = new Float32Array(128); // live brightness per MIDI note
let t0 = -1; // performance-time origin of the current playthrough

export function mapKeys(fixtures: SimFixture[]) {
  const n = fixtures.length;
  fixKey = new Int16Array(n).fill(-1);
  const canopy = fixtures.map((f, i) => ({ i, f })).filter((x) => x.f.zone === "high");
  canopy.sort((a, b) => a.f.seqT - b.f.seqT); // around the tree = along the keyboard
  canopy.forEach((x, k) => { fixKey[x.i] = BASE_MIDI + Math.min(71, k); });
  mappedN = n;
}
export function fixtureMidi(fixtures: SimFixture[], i: number): number {
  if (mappedN !== fixtures.length) mapKeys(fixtures);
  return fixKey[i];
}

// ── Moonlight Sonata, 1st movement (Adagio sostenuto), opening — encoded score ──
interface PNote { midi: number; t: number; dur: number; vel: number; }
const TRI = 0.34;            // seconds per triplet-eighth (slow, ~Adagio)
const BAR = 12 * TRI;        // 12 triplets per bar
// arpeggio trios (RH ostinato) + per-bar bass octaves
const A = [56, 61, 64];      // c#m : G#3 C#4 E4
const B = [57, 61, 64];      // a–c#–e
const C = [57, 62, 66];      // a–d–f#
const D = [56, 60, 64];      // g#–B#(C)–e  (dominant colour)
const E = [55, 59, 64];      // g–b–e turnaround
const BARS: { g: number[][]; bass: number[] }[] = [
  { g: [A, A, A, A], bass: [37, 49] },
  { g: [A, A, A, A], bass: [37, 49] },
  { g: [B, B, C, C], bass: [45, 57] },
  { g: [D, D, A, A], bass: [44, 56] },
  { g: [A, A, A, A], bass: [37, 49] },
  { g: [A, A, A, A], bass: [37, 49] },
  { g: [B, B, C, C], bass: [45, 57] },
  { g: [E, E, A, A], bass: [44, 56] },
];
export const SCORE_LEN = BARS.length * BAR;

const NOTES: PNote[] = (() => {
  const out: PNote[] = [];
  for (let b = 0; b < BARS.length; b++) {
    const bar = BARS[b];
    for (let grp = 0; grp < 4; grp++) {
      const trio = bar.g[grp];
      for (let i = 0; i < 3; i++) out.push({ midi: trio[i], t: b * BAR + grp * 3 * TRI + i * TRI, dur: TRI * 1.1, vel: 0.45 });
    }
    for (const bm of bar.bass) out.push({ midi: bm, t: b * BAR, dur: BAR * 0.98, vel: 0.6 });
  }
  // the famous melody enters at bar 4, sustained over the triplets
  const M: PNote[] = [
    { midi: 68, t: 4 * BAR, dur: 3.2, vel: 0.95 },                 // G#4
    { midi: 68, t: 5 * BAR, dur: 1.7, vel: 0.9 },                  // G#4
    { midi: 69, t: 5 * BAR + 1.9, dur: 1.0, vel: 0.9 },            // A4
    { midi: 68, t: 6 * BAR, dur: 1.7, vel: 0.9 },                  // G#4
    { midi: 66, t: 6 * BAR + 1.9, dur: 1.0, vel: 0.85 },           // F#4
    { midi: 64, t: 7 * BAR, dur: 2.6, vel: 0.9 },                  // E4
  ];
  return out.concat(M);
})();

export function resetPiano() { t0 = -1; }

/** Advance the piano and refresh keyBri. `now` = performance.now()/1000. */
export function updatePiano(now: number) {
  if (t0 < 0) t0 = now;
  const tt = (now - t0) % SCORE_LEN;
  keyBri.fill(0);
  for (const nt of NOTES) {
    const dt = tt - nt.t;
    if (dt < 0 || dt > nt.dur + 1.4) continue;
    // piano envelope: fast attack, exponential decay (longer notes ring longer)
    const env = dt < 0.025 ? dt / 0.025 : Math.exp(-(dt - 0.025) / Math.max(0.28, nt.dur * 0.7));
    const v = nt.vel * env;
    if (v > keyBri[nt.midi]) keyBri[nt.midi] = v;
  }
}
