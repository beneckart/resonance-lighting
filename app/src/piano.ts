import type { SimFixture } from "./store";

/** THE CANOPY AS A PIANO. The ~72 canopy ("high") lights are a 6-octave keyboard
 *  (MIDI 36..107, by azimuth). A selectable PIECE plays; each sounding note lights
 *  its key with a piano attack/decay envelope, octave doubling (more of the canopy
 *  lit), and colour by voice (bass indigo / arpeggio blue→cyan / melody gold). */
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

interface PNote { midi: number; t: number; dur: number; vel: number; voice: number } // voice 0 arp / 1 bass / 2 melody
interface Piece { id: string; name: string; notes: PNote[]; len: number }

// build a monophonic line from [midi, beats] pairs (midi 0 = rest)
function line(seq: [number, number][], q: number, voice: number, vel: number): { notes: PNote[]; len: number } {
  let t = 0; const notes: PNote[] = [];
  for (const [midi, beats] of seq) { if (midi > 0) notes.push({ midi, t, dur: beats * q * 0.92, vel, voice }); t += beats * q; }
  return { notes, len: t };
}

// ── Moonlight Sonata (1st mvt opening) — triplet arpeggio + bass + melody ──
function moonlight(): Piece {
  const TRI = 0.34, BAR = 12 * TRI;
  const A = [56, 61, 64], B = [57, 61, 64], C = [57, 62, 66], D = [56, 60, 64], E = [55, 59, 64], F = [54, 57, 61], G = [51, 56, 59];
  const BARS: { g: number[][]; bass: number[] }[] = [
    { g: [A, A, A, A], bass: [37, 49] }, { g: [A, A, A, A], bass: [37, 49] }, { g: [B, B, C, C], bass: [45, 57] }, { g: [D, D, A, A], bass: [44, 56] },
    { g: [A, A, A, A], bass: [37, 49] }, { g: [A, A, A, A], bass: [37, 49] }, { g: [B, B, C, C], bass: [45, 57] }, { g: [E, E, A, A], bass: [44, 56] },
    { g: [A, A, A, A], bass: [37, 49] }, { g: [B, B, C, C], bass: [45, 57] }, { g: [D, D, A, A], bass: [44, 56] }, { g: [A, A, A, A], bass: [37, 49] },
    { g: [F, F, F, F], bass: [42, 54] }, { g: [G, G, G, G], bass: [44, 56] }, { g: [A, A, A, A], bass: [37, 49] }, { g: [D, D, A, A], bass: [44, 56] },
  ];
  const notes: PNote[] = [];
  for (let b = 0; b < BARS.length; b++) {
    for (let grp = 0; grp < 4; grp++) for (let i = 0; i < 3; i++) notes.push({ midi: BARS[b].g[grp][i], t: b * BAR + grp * 3 * TRI + i * TRI, dur: TRI * 1.1, vel: 0.45, voice: 0 });
    for (const bm of BARS[b].bass) notes.push({ midi: bm, t: b * BAR, dur: BAR * 0.98, vel: 0.6, voice: 1 });
  }
  const m = (midi: number, bar: number, off: number, dur: number, vel = 0.95): PNote => ({ midi, t: bar * BAR + off, dur, vel, voice: 2 });
  notes.push(m(68, 4, 0, 3.2), m(68, 5, 0, 1.7), m(69, 5, 1.9, 1), m(68, 6, 0, 1.7), m(66, 6, 1.9, 1), m(64, 7, 0, 2.6),
    m(64, 8, 0, 3), m(63, 9, 0, 1.6), m(61, 9, 1.9, 1.4), m(61, 10, 0, 3), m(59, 11, 0, 2.6),
    m(61, 12, 0, 3), m(59, 13, 0, 1.6), m(56, 13, 1.9, 1.4), m(56, 14, 0, 3), m(56, 15, 0, 2.6));
  return { id: "moonlight", name: "Moonlight", notes, len: BARS.length * BAR };
}

// ── Für Elise (WoO 59 opening) — the iconic theme (sixteenth = q) ──
function furElise(): Piece {
  const q = 0.17;
  const theme: [number, number][] = [
    [76, 1], [75, 1], [76, 1], [75, 1], [76, 1], [71, 1], [74, 1], [72, 1], [69, 3], // E D# E D# E B D C A
    [60, 1], [64, 1], [69, 1], [71, 3],                                              // C E A B
    [64, 1], [68, 1], [71, 1], [72, 3],                                              // E G# B C
    [64, 1], [0, 1],
  ];
  const seq = [...theme, ...theme];
  const mel = line(seq, q, 2, 0.95);
  // light left-hand roots under the held notes (A / E / A …)
  const bass = line([[0, 9], [45, 3], [0, 1], [40, 3], [0, 1], [45, 3], [0, 5]], q, 1, 0.55);
  const bass2 = { notes: bass.notes.map((n) => ({ ...n, t: n.t + theme.reduce((s, [, b]) => s + b, 0) * q })), len: bass.len };
  return { id: "elise", name: "Für Elise", notes: [...mel.notes, ...bass.notes, ...bass2.notes], len: mel.len };
}

// ── Pachelbel Canon in D — ground bass + arpeggiated triads (loops) ──
function canon(): Piece {
  const q = 0.3; // eighth
  const chords: number[][] = [ // [root-low, triad up ×3] per half-bar (4 eighths)
    [38, 50, 54, 57, 62], [33, 45, 49, 52, 57], [35, 47, 50, 54, 59], [30, 42, 45, 49, 54],
    [31, 43, 47, 50, 55], [38, 50, 54, 57, 62], [31, 43, 47, 50, 55], [33, 45, 49, 52, 57],
  ];
  const notes: PNote[] = [];
  chords.forEach((c, i) => {
    const t = i * 4 * q;
    notes.push({ midi: c[0], t, dur: 4 * q * 0.98, vel: 0.6, voice: 1 }); // ground bass
    for (let k = 0; k < 4; k++) notes.push({ midi: c[1 + (k % 4)], t: t + k * q, dur: q * 1.05, vel: 0.55, voice: 0 }); // arpeggio
  });
  return { id: "canon", name: "Canon in D", notes, len: chords.length * 4 * q };
}

const PIECES: Record<string, Piece> = { moonlight: moonlight(), elise: furElise(), canon: canon() };
export const PIECE_LIST = Object.values(PIECES).map((p) => ({ id: p.id, name: p.name }));
let current = "moonlight";
export function setPiece(id: string) { if (PIECES[id]) { current = id; t0 = -1; } }
export function currentPiece() { return current; }

function colorFor(voice: number, midi: number): [number, number] {
  if (voice === 1) return [0.74, 0.7];
  if (voice === 2) return [0.10, 0.4];
  const frac = (midi - 36) / 71;
  return [0.62 - frac * 0.18, 0.6];
}

export function resetPiano() { t0 = -1; }

export function updatePiano(now: number) {
  if (t0 < 0) t0 = now;
  const piece = PIECES[current];
  const tt = (now - t0) % piece.len;
  keyBri.fill(0);
  const OCT = [0.3, 0.5, 0.72, 1, 0.72, 0.5, 0.3];
  for (const nt of piece.notes) {
    const dt = tt - nt.t;
    if (dt < 0 || dt > nt.dur + 1.4) continue;
    const env = dt < 0.025 ? dt / 0.025 : Math.exp(-(dt - 0.025) / Math.max(0.28, nt.dur * 0.7));
    const base = nt.vel * env;
    for (let o = -3; o <= 3; o++) {
      const mm = nt.midi + o * 12;
      if (mm < 36 || mm > 107) continue;
      const v = base * OCT[o + 3];
      if (v > keyBri[mm]) { keyBri[mm] = v; const [h, s] = colorFor(nt.voice, nt.midi); keyHue[mm] = h; keySat[mm] = s; }
    }
  }
}
