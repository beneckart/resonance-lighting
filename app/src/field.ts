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

// ── GAME OF LIFE on the neighbour graph — Ben's "Game of Life variants" (BACKGROUND.md).
//    A TRUE decentralised automaton: each light is a cell that reads its k-nearest
//    neighbours (the pre-baked flash neighbour list) and lives/dies by a simple count
//    rule (graph-tuned for ~6 neighbours, not an 8-grid). Rendered with a glow tail so
//    it reads organic, NEVER strobey.
//
//    PERPETUAL: a plain GoL on a sparse graph freezes into a still-life within seconds.
//    A steady churn (spontaneous births + isolated deaths, rate rides `speed`) keeps it
//    evolving CONTINUALLY — it never settles, so clicks always land on a living field.
//
//    PER-CELL RESPONSE: each cell carries its own HUE, BRIGHTNESS and TIME-ON (ttl). A
//    click/sensor seeds a blob tagged with the trigger rule's colour/brightness/duration;
//    those "held" cells stay lit for their ttl (immune to rule-death), propagate to
//    neighbours, then fade — so a touch's colour blooms and rolls out through the mesh.
const BORN_LO = 2, BORN_HI = 3;   // graph-tuned "B2-3"
const SURV_LO = 1, SURV_HI = 3;   // graph-tuned "S1-3"
const LIFE_AGE_MAX = 12;          // ticks a cell counts up while alive
let lifeCell = new Int8Array(0);  // 0 dead, 1..LIFE_AGE_MAX alive (age in ticks)
let lifeHue = new Float32Array(0);// per-cell hue 0..1
let lifeBri = new Float32Array(0);// per-cell brightness multiplier (default 1)
let lifeTtl = new Float32Array(0);// per-cell "held on" seconds remaining (0 = rule governs)
let lifeGlow = new Float32Array(0); // smoothed render brightness (fades on death)
let lifeAcc = 0;
let lifeN = -1;
export const lifeOut = { bri: new Float32Array(0), hue: new Float32Array(0) };
export interface SeedOpts { hops?: number; hue?: number; bri?: number; ttl?: number }
let lifeSeeds: { i: number; o: SeedOpts }[] = []; // pending pokes → birth next frame
let baseHueFn: (i: number) => number = () => 0.05;

/** Presence/sensor poke: birth a blob at these fixtures and TAG it with a response —
 *  `hue` (reaction colour), `bri` (brightness), `ttl` (seconds the light stays on),
 *  `hops` (how far the blob spreads across neighbours). The Game of Life then carries
 *  the disturbance onward. Omitted fields fall back to the living field's defaults. */
export function seedLife(indices: number[], opts: SeedOpts = {}) {
  for (const i of indices) lifeSeeds.push({ i, o: opts });
}

function reseedLife(n: number, fixtures: SimFixture[]) {
  lifeCell = new Int8Array(n); lifeHue = new Float32Array(n);
  lifeBri = new Float32Array(n).fill(1); lifeTtl = new Float32Array(n);
  lifeGlow = new Float32Array(n);
  lifeOut.bri = new Float32Array(n); lifeOut.hue = new Float32Array(n);
  baseHueFn = (i: number) => (0.02 + fixtures[i].heightT * 0.10 + fixtures[i].rnd * 0.04) % 1; // warm ambers/reds
  for (let i = 0; i < n; i++) { if (fixtures[i].rnd < 0.16) lifeCell[i] = 1; lifeHue[i] = baseHueFn(i); }
  lifeN = n;
}

export function updateLife(fixtures: SimFixture[], dt: number, speed: number) {
  const n = fixtures.length;
  if (lifeN !== n) reseedLife(n, fixtures);
  const dts = Math.min(0.1, Math.max(0, dt));

  // drain sensor pokes → birth a blob out to `hops`, tagged with the response
  if (lifeSeeds.length) {
    for (const { i, o } of lifeSeeds) {
      if (i < 0 || i >= n) continue;
      const hops = Math.max(0, Math.round(o.hops ?? 1));
      let frontier = [i]; const seen = new Set<number>([i]);
      const tag = (c: number, atten: number) => {
        lifeCell[c] = Math.max(1, lifeCell[c]);
        if (o.hue != null) lifeHue[c] = o.hue;
        if (o.bri != null) lifeBri[c] = o.bri * atten;
        if (o.ttl != null && o.ttl > 0) lifeTtl[c] = Math.max(lifeTtl[c], o.ttl * (0.6 + 0.4 * atten));
      };
      tag(i, 1);
      for (let h = 0; h < hops; h++) {
        const nextFront: number[] = [];
        const atten = 1 - (h + 1) / (hops + 1); // outer ring a touch dimmer/shorter
        for (const c of frontier) for (const j of fixtures[c].neighbors) {
          if (seen.has(j)) continue; seen.add(j); nextFront.push(j); tag(j, atten);
        }
        frontier = nextFront;
      }
    }
    lifeSeeds = [];
  }

  // count down "held on" cells every frame → gives the rules editor a real TIME-ON knob
  for (let i = 0; i < n; i++) if (lifeTtl[i] > 0) {
    lifeTtl[i] -= dts;
    if (lifeTtl[i] <= 0) { lifeTtl[i] = 0; lifeCell[i] = 0; } // held time elapsed → let it fade
  }

    const TICK = 0.6 / (0.14 + speed); // generations — speed clearly rides this: slow≈1.8s/gen, fast≈0.19s/gen
  lifeAcc += dts;
  let ticked = false;
  while (lifeAcc >= TICK) {
    lifeAcc -= TICK; ticked = true;
    const next = new Int8Array(n);
    const nextHue = new Float32Array(n);
    const churn = 0.005 + 0.022 * Math.min(1.5, speed); // spontaneous births — calm when slow, busy when fast, never freezes
    let live = 0;
    for (let i = 0; i < n; i++) {
      const nb = fixtures[i].neighbors;
      let a = 0, cx = 0, cy = 0;
      for (let k = 0; k < nb.length; k++) if (lifeCell[nb[k]] > 0) { a++; const hh = lifeHue[nb[k]] * TAU; cx += Math.cos(hh); cy += Math.sin(hh); }
      const alive = lifeCell[i] > 0;
      const held = lifeTtl[i] > 0; // click-held cells ignore the rule until their time is up
      let born = held || (alive ? a >= SURV_LO && a <= SURV_HI : a >= BORN_LO && a <= BORN_HI);
      // perpetual churn: rare spontaneous birth, rare isolated death → continual motion
      if (!born && !alive && Math.random() < churn) born = true;
      if (born && alive && !held && a <= 1 && Math.random() < 0.06) born = false;
      if (born) {
        next[i] = Math.min(LIFE_AGE_MAX, (alive ? lifeCell[i] : 0) + 1);
        // colour: keep own if alive/held, else inherit neighbours' mean (diffuses colour),
        // else base warm. Slow drift back toward base so injected colour eventually fades.
        let hue = alive || held ? lifeHue[i] : (a > 0 ? (Math.atan2(cy, cx) / TAU + 1) % 1 : baseHueFn(i));
        if (!held) { let d = baseHueFn(i) - hue; d -= Math.round(d); hue = (hue + d * 0.05 + 1) % 1; }
        nextHue[i] = hue;
        live++;
      } else { next[i] = 0; nextHue[i] = lifeHue[i]; }
    }
    if (live < 4) for (let s = 0; s < 8; s++) { const j = (Math.random() * n) | 0; next[j] = 1; nextHue[j] = baseHueFn(j); }
    lifeCell = next; lifeHue = nextHue;
  }

  // smoothed render — alive glows up, dead fades out; both snappier at higher speed so
  // the SPEED dial visibly changes the pace (not just the invisible generation clock).
  const up = Math.min(1, dts * (2 + speed * 5));
  const down = Math.min(1, dts * (1 + speed * 1.8));
  for (let i = 0; i < n; i++) {
    const target = lifeCell[i] > 0 ? Math.min(1.4, lifeBri[i]) : 0;
    lifeGlow[i] += (target - lifeGlow[i]) * (lifeCell[i] > 0 ? up : down);
    lifeOut.bri[i] = lifeGlow[i];
    lifeOut.hue[i] = lifeHue[i];
  }
  void ticked;
}

// ── REACTION-DIFFUSION (Gray-Scott) on the neighbour graph — organic blobs that
//    drift, split and merge. Row-normalised graph Laplacian (mean of neighbours). ──
let gu = new Float32Array(0), gv = new Float32Array(0), su = new Float32Array(0), sv = new Float32Array(0);
let gsN = -1;
export const organismOut = { bri: new Float32Array(0), hue: new Float32Array(0) };

export function updateOrganism(fixtures: SimFixture[], speed: number) {
  const n = fixtures.length;
  if (gsN !== n) {
    gu = new Float32Array(n).fill(1); gv = new Float32Array(n);
    su = new Float32Array(n); sv = new Float32Array(n);
    organismOut.bri = new Float32Array(n); organismOut.hue = new Float32Array(n);
    for (let s = 0; s < 8; s++) { const i = Math.floor(Math.random() * n); gv[i] = 0.5; gu[i] = 0.25; }
    gsN = n;
  }
  const Du = 0.5, Dv = 0.25, F = 0.025, k = 0.06; // stable "gentle spots" regime (low node count → ~1 drifting blob)
  const steps = Math.max(2, Math.round(3 + speed * 5)); // evolution rate rides the speed dial
  for (let s = 0; s < steps; s++) {
    for (let i = 0; i < n; i++) {
      const nb = fixtures[i].neighbors, L = nb.length || 1;
      let lu = 0, lv = 0;
      for (let j = 0; j < nb.length; j++) { lu += gu[nb[j]] - gu[i]; lv += gv[nb[j]] - gv[i]; }
      lu /= L; lv /= L;
      const uvv = gu[i] * gv[i] * gv[i];
      su[i] = Math.min(1, Math.max(0, gu[i] + (Du * lu - uvv + F * (1 - gu[i]))));
      sv[i] = Math.min(1, Math.max(0, gv[i] + (Dv * lv + uvv - (F + k) * gv[i])));
    }
    [gu, su] = [su, gu]; [gv, sv] = [sv, gv];
  }
  for (let i = 0; i < n; i++) {
    const v = gv[i];
    organismOut.bri[i] = Math.min(1, v * 2.6);
    organismOut.hue[i] = (0.62 - Math.min(1, v / (gu[i] + v + 1e-3)) * 0.42 + 1) % 1; // ratio → blue→green
  }
}

// ── LORENZ attractors — two hypnotic two-lobe foci that crawl + hand off, used as
//    the living engine's drifting points of focus. ──
const lz = [{ x: 0.1, y: 0, z: 0 }, { x: -6, y: 2, z: 22 }];
let lzAcc = 0;
export function lorenzFoci(center: [number, number, number], size: number, dt: number): Attractor[] {
  lzAcc += Math.min(0.06, Math.max(0, dt));
  const h = 0.006;
  while (lzAcc > h) {
    lzAcc -= h;
    for (const s of lz) {
      const dx = 10 * (s.y - s.x), dy = s.x * (28 - s.z) - s.y, dz = s.x * s.y - (8 / 3) * s.z;
      s.x += dx * h; s.y += dy * h; s.z += dz * h;
    }
  }
  return lz.map((s, i) => ({
    x: center[0] + (s.x / 22) * size * 0.5,
    y: center[1] + ((s.z - 25) / 28) * size * 0.5,
    z: center[2] + (s.y / 22) * size * 0.5,
    hue: i === 0 ? 0.58 : 0.05,
  }));
}
