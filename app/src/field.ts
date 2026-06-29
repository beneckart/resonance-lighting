import type { SimFixture } from "./store";

/** DECENTRALISED "LIVING" FIELD — each light decides its own state from its
 *  NEIGHBOURS, with no central pattern. Three coupled processes, all SLOW + organic
 *  (no strobe):
 *   1. Firefly synchronisation (Kuramoto): each light is a phase oscillator nudged
 *      toward its neighbours → spontaneous travelling waves of soft flashes.
 *   2. Neighbour colour diffusion: each light drifts its hue toward the circular
 *      mean of its neighbours' hues (+ a slow global rotation) → coherent, evolving
 *      colour fields rather than noise.
 *   3. Drifting attractors: slow-moving focal points that brighten + tint nearby
 *      lights → the "points of focus" the show moves around.
 *  State persists across frames (this is a simulation, not a stateless pattern). */
export interface Attractor { x: number; y: number; z: number; hue: number; }

const TAU = Math.PI * 2;
let phase = new Float32Array(0); // 0..1 firefly oscillator
let hueF = new Float32Array(0); // 0..1 per-light hue (diffuses to neighbours)
let tAcc = 0; // wall-clock accumulator (drives the slow spatial colour source)
let n0 = -1;
export const fieldOut = { bri: new Float32Array(0), hue: new Float32Array(0) };

function reseed(n: number, fixtures: SimFixture[]) {
  phase = new Float32Array(n);
  hueF = new Float32Array(n);
  fieldOut.bri = new Float32Array(n);
  fieldOut.hue = new Float32Array(n);
  for (let i = 0; i < n; i++) {
    phase[i] = fixtures[i].rnd; // desynced start → emerges into sync
    hueF[i] = (fixtures[i].heightT * 0.4 + fixtures[i].rnd * 0.3) % 1; // gentle spatial seed
  }
  n0 = n;
}

/** Advance the living field one step and write fieldOut.bri/hue (both 0..1). */
export function updateField(fixtures: SimFixture[], dt: number, speed: number, attractors: Attractor[]) {
  const n = fixtures.length;
  if (n0 !== n) reseed(n, fixtures);
  const dts = Math.min(0.05, Math.max(0, dt));
  tAcc += dts;
  const rate = dts * (0.5 + speed); // organic, slow
  const K = 0.25; // weak local coupling → PARTIAL sync (travelling waves, not a tree-wide strobe)

  for (let i = 0; i < n; i++) {
    const f = fixtures[i];
    const nb = f.neighbors;
    const L = nb.length || 1;

    // 1. firefly phase nudged toward neighbours — mean pull → partial sync; a
    //    bottom→top frequency gradient makes the flash waves sweep UP the tree
    let coupling = 0;
    for (let k = 0; k < nb.length; k++) coupling += Math.sin((phase[nb[k]] - phase[i]) * TAU);
    coupling /= L;
    const omega = (0.10 + 0.05 * (f.rnd - 0.5)) * (1 + 0.5 * f.heightT);
    phase[i] += rate * (omega + K * coupling);
    if (phase[i] >= 1) phase[i] -= 1; else if (phase[i] < 0) phase[i] += 1;
    // soft flash envelope (gaussian around the wrap point) — NEVER a hard strobe
    const dmin = Math.min(phase[i], 1 - phase[i]);
    const env = Math.exp(-(dmin * dmin) / (0.18 * 0.18));

    // 2. hue diffuses toward neighbours' circular mean + slow global rotation
    let cx = 0, cy = 0;
    for (let k = 0; k < nb.length; k++) { cx += Math.cos(hueF[nb[k]] * TAU); cy += Math.sin(hueF[nb[k]] * TAU); }
    const target = Math.atan2(cy, cx) / TAU; // [-0.5,0.5]
    let dh = target - hueF[i];
    dh -= Math.round(dh); // shortest way around the colour wheel
    // a slowly-drifting SPATIAL source keeps the field colour-VARIED across the tree
    // (pure neighbour diffusion alone collapses everything to ONE colour over time)
    const src = f.pos[0] * 0.014 + f.pos[2] * 0.014 + f.heightT * 0.25 + tAcc * 0.025;
    let ds = (src - Math.floor(src)) - hueF[i];
    ds -= Math.round(ds);
    hueF[i] += rate * (0.5 * dh + 0.35 * ds); // diffuse toward neighbours + toward the source
    hueF[i] -= Math.floor(hueF[i]);

    // 3. attractors: brighten + pull hue toward the focal point's colour
    let aBoost = 0, ax = 0, ay = 0, aw = 0;
    for (const a of attractors) {
      const dx = f.pos[0] - a.x, dy = f.pos[1] - a.y, dz = f.pos[2] - a.z;
      const w = 1 / (1 + (dx * dx + dy * dy + dz * dz) * 0.012);
      aBoost += w;
      ax += Math.cos(a.hue * TAU) * w; ay += Math.sin(a.hue * TAU) * w; aw += w;
    }
    aBoost = Math.min(1, aBoost);
    let hue = hueF[i];
    if (aw > 0.15) {
      const aHue = Math.atan2(ay, ax) / TAU;
      let dha = aHue - hue; dha -= Math.round(dha);
      hue = (hue + dha * 0.4 * aBoost + 1) % 1; // near a focus, lean to its colour
    }

    fieldOut.bri[i] = Math.min(1, 0.10 + 0.7 * env + 0.55 * aBoost);
    fieldOut.hue[i] = hue;
  }
}

// ── EXCITABLE MEDIA (Greenberg-Hastings) — legible ripple waves spreading across
//    neighbours and fading. Slow CA tick + smoothed brightness = no strobe. ──
const KAPPA = 10; // 0 = resting, 1 = excited, 2..KAPPA-1 = refractory tail
let cell = new Int8Array(0);
let ghAcc = 0;
let ghN = -1;
export const rippleOut = { bri: new Float32Array(0), age: new Float32Array(0) };

export function updateRipples(fixtures: SimFixture[], dt: number, speed: number) {
  const n = fixtures.length;
  if (ghN !== n) { cell = new Int8Array(n); rippleOut.bri = new Float32Array(n); rippleOut.age = new Float32Array(n); ghN = n; }
  ghAcc += Math.min(0.1, Math.max(0, dt)) * (0.6 + speed);
  const TICK = 0.45; // seconds per CA step (slow, organic)
  if (ghAcc >= TICK) {
    ghAcc -= TICK;
    const next = new Int8Array(n);
    for (let i = 0; i < n; i++) {
      const c = cell[i];
      if (c === 1) next[i] = 2;                       // excited → refractory
      else if (c >= 2) next[i] = c + 1 >= KAPPA ? 0 : c + 1; // refractory countdown → resting
      else {                                          // resting: excite if a neighbour is excited
        let ex = 0;
        const nb = fixtures[i].neighbors;
        for (let k = 0; k < nb.length; k++) if (cell[nb[k]] === 1) ex++;
        next[i] = ex >= 1 ? 1 : (Math.random() < 0.0025 ? 1 : 0); // + rare spontaneous seed
      }
    }
    cell = next;
  }
  const ease = Math.min(1, Math.max(0, dt) * 5); // smooth brightness (no hard on/off)
  for (let i = 0; i < n; i++) {
    const c = cell[i];
    const target = c === 1 ? 1 : c >= 2 ? Math.max(0, 1 - (c - 1) / (KAPPA - 1)) : 0;
    rippleOut.bri[i] += (target - rippleOut.bri[i]) * ease;
    rippleOut.age[i] = c >= 2 ? (c - 1) / (KAPPA - 1) : 0; // 0 at wavefront → 1 at tail
  }
}
