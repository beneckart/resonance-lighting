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

// ── COLOUR THEME constraint for ALL the CA engines (Elliot: the picked theme —
//    love, ocean, … — must hold across Game of Life, Excitable, the lot). The
//    life engine themes its BIRTHS; free-running engines (living, organism,
//    ripples) pass their computed hue through this map, which pulls it to the
//    nearest theme anchor while keeping most of the local variation.
let fieldThemeHues: number[] | null = null;
export function setFieldTheme(hues: number[] | null) {
  fieldThemeHues = hues && hues.length ? hues : null;
}
export function themeMapHue(h: number, hues?: number[] | null): number {
  const t = hues === undefined ? fieldThemeHues : hues; // undefined = global theme · null = unconstrained
  if (!t || !t.length) return h;
  let best = t[0], bd = Infinity;
  for (const a of t) { let d = Math.abs(a - h) % 1; d = Math.min(d, 1 - d); if (d < bd) { bd = d; best = a; } }
  let off = h - best; off -= Math.round(off); // signed shortest offset
  return ((best + off * 0.22) % 1 + 1) % 1; // compress toward the anchor, keep life
}

const TAU = Math.PI * 2;

// ── EDITABLE RULE PARAMS for Excitable / Reaction-Diffusion / Firefly (Elliot:
//    "the ability to control the rules"). Game of Life already has B/S; these
//    give the other three the same tunability. Safe ranges enforced by the UI. ──
export interface CaParams {
  ghKappa: number;   // Excitable: refractory length (wave cool-down); higher = slower re-fire
  ghSeed: number;    // Excitable: spontaneous ignition rate (0 = only taps/presence)
  rdFeed: number;    // Reaction-Diffusion: feed F
  rdKill: number;    // Reaction-Diffusion: kill k
  ffCouple: number;  // Firefly: sync strength K (0 = no sync, chaotic; high = tree-wide pulse)
  ffRate: number;    // Firefly: base flash rate
}
let caParams: CaParams = { ghKappa: 10, ghSeed: 0.0025, rdFeed: 0.025, rdKill: 0.06, ffCouple: 0.25, ffRate: 0.10 };
export function setCaParams(p: Partial<CaParams>) { caParams = { ...caParams, ...p }; }
export function getCaParams(): CaParams { return { ...caParams }; }

// ── INTERACTIVE-REST excitation (shared by Firefly + Reaction-Diffusion) ──
let excite = new Float32Array(0); // 0..1 per-fixture liveliness; taps raise, decays
let interactiveRest = true;       // true = quiet at rest, wake on touch (interactive); false = free-run (show)
export function setInteractiveRest(on: boolean) { interactiveRest = on; }
function bumpExcite(indices: number[], amp = 1) {
  for (const i of indices) if (i >= 0 && i < excite.length) excite[i] = Math.min(1, excite[i] + amp);
}
// decay + gentle neighbour spread, once per frame
function decayExcite(fixtures: SimFixture[], dt: number) {
  const n = excite.length; if (!n) return;
  const d = Math.exp(-dt / 3.2); // ~3 s half-life-ish
  const nxt = new Float32Array(n);
  for (let i = 0; i < n; i++) {
    let sp = 0; const nb = fixtures[i].neighbors;
    for (let k = 0; k < nb.length; k++) sp += excite[nb[k]];
    nxt[i] = Math.min(1, excite[i] * d + (nb.length ? sp / nb.length : 0) * 0.06 * (1 - d));
  }
  excite = nxt;
}
let phase = new Float32Array(0); // 0..1 firefly oscillator
let hueF = new Float32Array(0); // 0..1 per-light hue (diffuses to neighbours)
let tAcc = 0; // wall-clock accumulator (drives the slow spatial colour source)
let n0 = -1;
export const fieldOut = { bri: new Float32Array(0), hue: new Float32Array(0) };

function reseed(n: number, fixtures: SimFixture[]) {
  phase = new Float32Array(n);
  hueF = new Float32Array(n);
  excite = new Float32Array(n);
  fieldOut.bri = new Float32Array(n);
  fieldOut.hue = new Float32Array(n);
  for (let i = 0; i < n; i++) {
    phase[i] = fixtures[i].rnd; // desynced start → emerges into sync
    hueF[i] = (fixtures[i].heightT * 0.4 + fixtures[i].rnd * 0.3) % 1; // gentle spatial seed
  }
  n0 = n;
}

/** A touch KICKS the firefly field: the touched lights' oscillators jump to the
 *  flash point → an immediate bright pulse that then re-synchronises outward —
 *  the crowd's tap visibly perturbs the swarm (Elliot: clearly see the response). */
export function exciteField(indices: number[]) {
  for (const i of indices) if (i >= 0 && i < phase.length) phase[i] = 0.99;
  bumpExcite(indices, 1);
}

/** Advance the living field one step and write fieldOut.bri/hue (both 0..1). */
export function updateField(fixtures: SimFixture[], dt: number, speed: number, attractors: Attractor[]) {
  const n = fixtures.length;
  if (n0 !== n) reseed(n, fixtures);
  const dts = Math.min(0.05, Math.max(0, dt));
  tAcc += dts;
  const rate = dts * Math.max(0.05, speed) * 1.5; // multiplicative: the dial reaches GLACIAL (0.05×) and fast (6×); ×1.5 keeps speed-1 parity
  decayExcite(fixtures, dts);
  const K = caParams.ffCouple; // coupling → sync strength (editable)

  for (let i = 0; i < n; i++) {
    const f = fixtures[i];
    const nb = f.neighbors;
    const L = nb.length || 1;

    // 1. firefly phase nudged toward neighbours — mean pull → partial sync; a
    //    bottom→top frequency gradient makes the flash waves sweep UP the tree
    let coupling = 0;
    for (let k = 0; k < nb.length; k++) coupling += Math.sin((phase[nb[k]] - phase[i]) * TAU);
    coupling /= L;
    const omega = (caParams.ffRate + 0.05 * (f.rnd - 0.5)) * (1 + 0.5 * f.heightT);
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

    // INTERACTIVE REST: the flashing lives only where excitation is (taps/presence);
    // at rest a faint breath so the tree isn't dead. Free-run mode = old always-on show.
    const gate = interactiveRest ? Math.max(0.05, excite[i]) : 1;
    fieldOut.bri[i] = Math.min(1, (0.10 + 0.7 * env) * gate + 0.55 * aBoost);
    fieldOut.hue[i] = themeMapHue(hue);
  }
}

// ── EXCITABLE MEDIA (Greenberg-Hastings) — legible ripple waves spreading across
//    neighbours and fading. Slow CA tick + smoothed brightness = no strobe. ──
// KAPPA (refractory length) is now caParams.ghKappa (editable)
let cell = new Int8Array(0);
let ghAcc = 0;
let ghN = -1;
export const rippleOut = { bri: new Float32Array(0), age: new Float32Array(0) };

/** Excite the medium at these fixtures (a touch/sensor) — the wave rolls out
 *  from there by the GH rule. Without this the mode only self-seeds rarely and
 *  can sit dark for many seconds, which reads as "broken". */
export function exciteRipples(indices: number[]) {
  for (const i of indices) if (i >= 0 && i < cell.length && cell[i] === 0) cell[i] = 1;
}

export function updateRipples(fixtures: SimFixture[], dt: number, speed: number) {
  const n = fixtures.length;
  if (ghN !== n) { cell = new Int8Array(n); rippleOut.bri = new Float32Array(n); rippleOut.age = new Float32Array(n); ghN = n; }
  ghAcc += Math.min(0.1, Math.max(0, dt)) * Math.max(0.05, speed) * 1.6; // multiplicative dial (glacial→fast), speed-1 parity with the old 0.6+1
  const TICK = 0.45; // seconds per CA step (slow, organic)
  if (ghAcc >= TICK) {
    ghAcc -= TICK;
    const next = new Int8Array(n);
    for (let i = 0; i < n; i++) {
      const c = cell[i];
      const KAPPA = Math.max(3, Math.round(caParams.ghKappa));
      if (c === 1) next[i] = 2;                       // excited → refractory
      else if (c >= 2) next[i] = c + 1 >= KAPPA ? 0 : c + 1; // refractory countdown → resting
      else {                                          // resting: excite if a neighbour is excited
        let ex = 0;
        const nb = fixtures[i].neighbors;
        for (let k = 0; k < nb.length; k++) if (cell[nb[k]] === 1) ex++;
        next[i] = ex >= 1 ? 1 : (Math.random() < caParams.ghSeed ? 1 : 0); // + spontaneous seed (editable)
      }
    }
    cell = next;
  }
  const ease = Math.min(1, Math.max(0, dt) * 5); // smooth brightness (no hard on/off)
  const KA = Math.max(3, Math.round(caParams.ghKappa));
  for (let i = 0; i < n; i++) {
    const c = cell[i];
    const target = c === 1 ? 1 : c >= 2 ? Math.max(0, 1 - (c - 1) / (KA - 1)) : 0;
    rippleOut.bri[i] += (target - rippleOut.bri[i]) * ease;
    rippleOut.age[i] = c >= 2 ? (c - 1) / (KA - 1) : 0; // 0 at wavefront → 1 at tail
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
// EDITABLE rule (Elliot: "to accurately get the game of life patterns to work we
// need to be able to edit the rules").
// `pure` = TEXTBOOK mode: no churn, no old-age, no refractory, no fatigue, no
// extinction guard — exact birth/survive counts on the neighbour graph, so seeded
// patterns evolve by the rule and NOTHING else (playgameoflife.com semantics).
// DEFAULT = Conway adapted to the mesh: survive 2-3 exactly as Conway; birth
// scaled from the 8-neighbour grid to our 6-neighbour graph (3/8 ≈ 2/6 → B2).
// Measured on the real 118-fixture graph from 4-9-light seeds: literal B3/S23
// dies in ~4 generations (median); B2/S23 gives median-76-generation games with
// genuine still-lifes and period-2 oscillators — Conway DYNAMICS, mesh topology.
export interface LifeRules { bLo: number; bHi: number; sLo: number; sHi: number; pure: boolean }
let lifeRules: LifeRules = { bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true };
export function setLifeRules(r: Partial<LifeRules>) { lifeRules = { ...lifeRules, ...r }; }
export function getLifeRules(): LifeRules { return { ...lifeRules }; }
const LIFE_AGE_MAX = 12;          // ticks a cell counts up while alive
let lifeCell = new Int8Array(0);  // 0 dead, 1..LIFE_AGE_MAX alive (age in ticks)
let lifeRefr = new Int8Array(0);  // refractory countdown after death (dark-at-rest: waves burn out)
let lifeFatigue = 0;              // generations since the last stimulation (dark-at-rest decay pressure)
let lifeHue = new Float32Array(0);// per-cell hue 0..1
let lifeBri = new Float32Array(0);// per-cell brightness multiplier (default 1)
let lifeTtl = new Float32Array(0);// per-cell "held on" seconds remaining (0 = rule governs)
let lifeGlow = new Float32Array(0); // smoothed render brightness (fades on death)
let lifeAcc = 0;
let lifeN = -1;
export const lifeOut = { bri: new Float32Array(0), hue: new Float32Array(0) };
export interface SeedOpts { hops?: number; hue?: number; bri?: number; ttl?: number }
let lifeSeeds: { i: number; o: SeedOpts }[] = []; // pending pokes → birth next frame
// STAGED births: ring k of a seed blob is born at generation gen+k, so a touch
// visibly TRICKLES outward hop-by-hop (Elliot: "obvious how they trickle through")
let lifePending: { gen: number; i: number; hue?: number; bri?: number; ttl?: number }[] = [];
let baseHueFn: (i: number) => number = () => 0.05;
// ── Game-of-Light state: persistent visitor NODES + ambient mode ──
//    A node = a person activating a sensor: a PERMANENT live source that each
//    generation keeps itself + a rotating pair of neighbours alive, so Game-of-Life
//    patterns continually emanate from it and interact with the other nodes. `ambient`
//    off = tree is DARK at rest and only lights where visitors are (nodes/taps).
let lifeNodes: { i: number; hue: number }[] = [];
let lifeAmbient = true;
let lifePalette: "warm" | "random" | "theme" = "warm"; // birth colours: warm base · random · a colour THEME
let lifeThemeHues: number[] = []; // theme anchors (when palette === "theme")
let lifeGen = 0;
// nearest theme anchor to a hue (circular) — drift target inside the theme
function nearestThemeHue(h: number): number {
  let best = h, bd = Infinity;
  for (const a of lifeThemeHues) { let d = Math.abs(a - h) % 1; d = Math.min(d, 1 - d); if (d < bd) { bd = d; best = a; } }
  return best;
}
const themeBirthHue = (i: number, gen: number) => {
  if (!lifeThemeHues.length) return rndHue(i, gen);
  const base = lifeThemeHues[Math.floor(rndHue(i, gen) * lifeThemeHues.length) % lifeThemeHues.length];
  return ((base + (rndHue(i + 313, gen) - 0.5) * 0.05) % 1 + 1) % 1;
};
export function setLifeState(o: { nodes?: { i: number; hue: number }[]; ambient?: boolean; palette?: "warm" | "random" | "theme"; themeHues?: number[] }) {
  if (o.nodes) lifeNodes = o.nodes;
  if (o.ambient != null) lifeAmbient = o.ambient;
  if (o.themeHues) lifeThemeHues = o.themeHues;
  if (o.palette && (o.palette !== lifePalette || o.themeHues)) {
    lifePalette = o.palette;
    // switching palette: instantly RE-COLOUR the living population so the change
    // reads immediately (otherwise neighbour-inheritance keeps the old blend)
    if (o.palette === "random") for (let i = 0; i < lifeHue.length; i++) { if (lifeCell[i] > 0) lifeHue[i] = rndHue(i, lifeGen + i); }
    else if (o.palette === "theme") for (let i = 0; i < lifeHue.length; i++) { if (lifeCell[i] > 0) lifeHue[i] = themeBirthHue(i, lifeGen + i); }
  }
}
// deterministic per-(fixture, generation) random hue — a fresh colour for each birth
const rndHue = (i: number, gen: number) => { const x = Math.sin(i * 71.7 + gen * 13.13) * 43758.5453; return x - Math.floor(x); };

/** Presence/sensor poke: birth a blob at these fixtures and TAG it with a response —
 *  `hue` (reaction colour), `bri` (brightness), `ttl` (seconds the light stays on),
 *  `hops` (how far the blob spreads across neighbours). The Game of Life then carries
 *  the disturbance onward. Omitted fields fall back to the living field's defaults. */
/** Wipe the field to BLANK (all dead, nothing pending) — the mode-entry contract:
 *  the tree goes dark and the automaton starts from an empty board. */
export function clearLife() {
  lifeCell.fill(0); lifeRefr.fill(0); lifeTtl.fill(0); lifeGlow.fill(0);
  lifeBri.fill(1);
  lifeSeeds = []; lifePending = []; lifeFatigue = 0;
  lifeOut.bri.fill(0);
}

export function seedLife(indices: number[], opts: SeedOpts = {}) {
  for (const i of indices) lifeSeeds.push({ i, o: opts });
  lifeFatigue = 0; // fresh stimulation → the field wakes up fully again
}

// ── NEW-GAME watchdog (pure mode) ── a Conway game ENDS: extinction, a still-
// life, or a small oscillator. In ambient pure mode the tree then deals a fresh
// hand — a random CLUSTER of 4-9 lights (Elliot: "doesn't have to start from
// blank — it can start from any 4-9 lights on"). Scattered singles would just
// die under B2/S23, so the seed is one BFS neighbourhood, like drawing a small
// pattern on playgameoflife.com and pressing play.
let lifeHist: number[] = []; // recent generation hashes → cycle detection
let lifeStagnant = 0;
function lifeStateHash(): number {
  let h = 2166136261;
  for (let i = 0; i < lifeCell.length; i++) if (lifeCell[i] > 0) { h ^= i + 1; h = Math.imul(h, 16777619); }
  return h >>> 0;
}
export function seedRandomCluster(fixtures: SimFixture[], count?: number) {
  const n = fixtures.length;
  if (!n || lifeCell.length !== n) return;
  const c = count ?? 4 + Math.floor(Math.random() * 6); // 4-9 lights on
  const c0 = Math.floor(Math.random() * n);
  const order = [c0];
  const seen = new Set<number>([c0]);
  for (let q = 0; q < order.length && order.length < 14; q++)
    for (const j of fixtures[order[q]].neighbors) if (!seen.has(j)) { seen.add(j); order.push(j); }
  for (const i of order.slice(0, c)) {
    lifeCell[i] = 1; lifeRefr[i] = 0;
    lifeHue[i] = lifePalette === "random" ? rndHue(i, lifeGen + i) : lifePalette === "theme" ? themeBirthHue(i, lifeGen + i) : baseHueFn(i);
  }
  lifeHist = []; lifeStagnant = 0;
}

function reseedLife(n: number, fixtures: SimFixture[]) {
  lifeCell = new Int8Array(n); lifeRefr = new Int8Array(n); lifeHue = new Float32Array(n);
  lifeBri = new Float32Array(n).fill(1); lifeTtl = new Float32Array(n);
  lifeGlow = new Float32Array(n);
  lifeOut.bri = new Float32Array(n); lifeOut.hue = new Float32Array(n);
  baseHueFn = (i: number) => (0.02 + fixtures[i].heightT * 0.10 + fixtures[i].rnd * 0.04) % 1; // warm ambers/reds
  for (let i = 0; i < n; i++) { if (lifeAmbient && fixtures[i].rnd < 0.16) lifeCell[i] = 1; lifeHue[i] = baseHueFn(i); }
  lifeN = n;
}

export function updateLife(fixtures: SimFixture[], dt: number, speed: number) {
  const n = fixtures.length;
  if (lifeN !== n) reseedLife(n, fixtures);
  const dts = Math.min(0.1, Math.max(0, dt));

  // drain sensor pokes → schedule the blob STAGED: the tapped cell births NOW, each
  // neighbour ring births one GENERATION later — so the touch visibly trickles
  // outward hop-by-hop at the field's own pace instead of appearing all at once.
  if (lifeSeeds.length) {
    for (const { i, o } of lifeSeeds) {
      if (i < 0 || i >= n) continue;
      const hops = Math.max(0, Math.round(o.hops ?? 1));
      const tag = (c: number, atten: number) => {
        lifeCell[c] = Math.max(1, lifeCell[c]); lifeRefr[c] = 0;
        if (o.hue != null) lifeHue[c] = o.hue;
        if (o.bri != null) lifeBri[c] = o.bri * atten;
        if (o.ttl != null && o.ttl > 0) lifeTtl[c] = Math.max(lifeTtl[c], o.ttl * (0.6 + 0.4 * atten));
      };
      tag(i, 1); // the touched light answers IMMEDIATELY
      let frontier = [i]; const seen = new Set<number>([i]);
      for (let h = 0; h < hops; h++) {
        const nextFront: number[] = [];
        const atten = 1 - (h + 1) / (hops + 1); // outer ring a touch dimmer/shorter
        for (const c of frontier) for (const j of fixtures[c].neighbors) {
          if (seen.has(j)) continue; seen.add(j); nextFront.push(j);
          lifePending.push({ gen: lifeGen + h + 1, i: j, hue: o.hue, bri: o.bri != null ? o.bri * atten : undefined, ttl: o.ttl != null && o.ttl > 0 ? o.ttl * (0.6 + 0.4 * atten) : undefined });
        }
        frontier = nextFront;
      }
    }
    lifeSeeds = [];
  }
  // staged ring births whose generation has arrived
  if (lifePending.length) {
    const due = lifePending.filter((p) => p.gen <= lifeGen);
    if (due.length) {
      lifePending = lifePending.filter((p) => p.gen > lifeGen);
      for (const p of due) {
        if (p.i < 0 || p.i >= n) continue;
        lifeCell[p.i] = Math.max(1, lifeCell[p.i]); lifeRefr[p.i] = 0;
        if (p.hue != null) lifeHue[p.i] = p.hue;
        if (p.bri != null) lifeBri[p.i] = p.bri;
        if (p.ttl != null) lifeTtl[p.i] = Math.max(lifeTtl[p.i], p.ttl);
      }
    }
  }

  // count down "held on" cells every frame → gives the rules editor a real TIME-ON knob
  for (let i = 0; i < n; i++) if (lifeTtl[i] > 0) {
    lifeTtl[i] -= dts;
    if (lifeTtl[i] <= 0) { lifeTtl[i] = 0; lifeCell[i] = 0; } // held time elapsed → let it fade
  }

  // generation clock — POWER-LAW so the dial truly reaches glacial. BASELINE
  // (speed 1) = ONE SECOND per turn (Elliot: Conway-ish, tuned in fractions of
  // a second): 0.25 ≈ 4.9s · 1 = 1s · 2.5 ≈ 0.35s · 4 ≈ 0.2s · slider min ≈ 90s.
  // Callers pass the RAW dial value (not wind-inflated env speed).
  const TICK = Math.min(120, 1.0 * Math.pow(Math.max(0.02, speed), -1.15));
  lifeAcc += dts;
  let ticked = false;
  while (lifeAcc >= TICK) {
    lifeAcc -= TICK; ticked = true; lifeGen++;
    if (!lifeAmbient) lifeFatigue++;
    const next = new Int8Array(n);
    const nextHue = new Float32Array(n);
    // dark-at-rest (ambient off): NO spontaneous churn — only nodes/taps light the tree
    const churn = lifeAmbient && !lifeRules.pure ? 0.004 + 0.020 * Math.min(1.5, speed) : 0;
    let live = 0;
    for (let i = 0; i < n; i++) {
      const nb = fixtures[i].neighbors;
      let a = 0, cx = 0, cy = 0;
      for (let k = 0; k < nb.length; k++) if (lifeCell[nb[k]] > 0) { a++; const hh = lifeHue[nb[k]] * TAU; cx += Math.cos(hh); cy += Math.sin(hh); }
      const alive = lifeCell[i] > 0;
      const held = lifeTtl[i] > 0; // click-held cells ignore the rule until their time is up
      let born = held || (alive ? a >= lifeRules.sLo && a <= lifeRules.sHi : a >= lifeRules.bLo && a <= lifeRules.bHi);
      // perpetual churn: rare spontaneous birth, rare isolated death → continual motion
      if (!born && !alive && Math.random() < churn) born = true;
      if (born && alive && !held && a <= 1 && !lifeRules.pure && Math.random() < 0.06) born = false;
      // DARK-AT-REST (ambient off): a disturbance must BURN OUT, not self-sustain
      // (caught by field.test — plain B2-3/S1-3 oscillates forever on this graph):
      //  · cells die of OLD AGE (age cap), and
      //  · dead cells are REFRACTORY for a few generations (excitable-medium rule) so
      //    the wave rolls outward and leaves quiet darkness behind it.
      if (!lifeAmbient && !held && !lifeRules.pure) {
        if (born && alive && lifeCell[i] >= LIFE_AGE_MAX - 1) born = false; // old age
        if (born && !alive && lifeRefr[i] > 0) born = false; // still refractory
        // FATIGUE: without fresh stimulation the death pressure rises each generation,
        // guaranteeing extinction on ANY topology (a rotating ring-wave otherwise laps
        // forever). Any seedLife (a touch / node placement) resets it to zero.
        if (born && Math.random() < Math.min(0.85, lifeFatigue * 0.02)) born = false;
      }
      if (born) {
        next[i] = Math.min(LIFE_AGE_MAX, (alive ? lifeCell[i] : 0) + 1);
        // colour: keep own if alive/held; else inherit neighbours' mean (a patch KEEPS
        // its colour as it spreads); else a fresh birth takes the palette's colour —
        // "warm" = the amber base field, "random" = every new birth its own random hue
        // (a multicoloured population whose patches then diffuse into each other).
        let hue = alive || held ? lifeHue[i] : (a > 0 ? (Math.atan2(cy, cx) / TAU + 1) % 1
          : lifePalette === "random" ? rndHue(i, lifeGen)
          : lifePalette === "theme" ? themeBirthHue(i, lifeGen)
          : baseHueFn(i));
        // warm re-converges to the amber base · random keeps DIVERGING (jitter beats
        // neighbour-blend) · theme drifts toward its NEAREST theme anchor, so the
        // field evolves freely but always stays inside the picked colour world
        if (!held && lifePalette === "warm") { let d = baseHueFn(i) - hue; d -= Math.round(d); hue = (hue + d * 0.05 + 1) % 1; }
        else if (!held && lifePalette === "theme") { let d = nearestThemeHue(hue) - hue; d -= Math.round(d); hue = (hue + d * 0.12 + (rndHue(i, lifeGen) - 0.5) * 0.03 + 1) % 1; }
        else if (!held) hue = (hue + (rndHue(i, lifeGen) - 0.5) * 0.10 + 1) % 1;
        nextHue[i] = hue;
        live++;
      } else {
        next[i] = 0; nextHue[i] = lifeHue[i];
        if (alive) lifeRefr[i] = 5; // just died → refractory (dark-at-rest burn-out)
        else if (lifeRefr[i] > 0) lifeRefr[i]--;
        // decay any tap-boosted brightness back to normal while dead — otherwise a
        // bright touch leaves a permanent hot-spot at this fixture forever
        lifeBri[i] += (1 - lifeBri[i]) * 0.35;
      }
    }
    // extinction guard only when the tree should stay alive on its own (ambient)
    if (lifeAmbient && !lifeRules.pure && live < 4) for (let s = 0; s < 8; s++) { const j = (Math.random() * n) | 0; next[j] = 1; nextHue[j] = lifePalette === "random" ? rndHue(j, lifeGen + s) : lifePalette === "theme" ? themeBirthHue(j, lifeGen + s) : baseHueFn(j); }
    // PERSISTENT NODES (visitors): keep each node + a rotating pair of its neighbours
    // alive so Game-of-Life patterns emanate from every person and interact. The node
    // itself is a bright anchor in its quadrant's colour.
    for (const nd of lifeNodes) {
      if (nd.i < 0 || nd.i >= n) continue;
      next[nd.i] = Math.max(1, next[nd.i]); nextHue[nd.i] = nd.hue; lifeBri[nd.i] = 1.35;
      const nb = fixtures[nd.i].neighbors;
      if (nb.length) for (let e = 0; e < 2; e++) {
        const j = nb[(lifeGen + e) % nb.length];
        next[j] = Math.max(1, next[j]); nextHue[j] = nd.hue;
      }
    }
    lifeCell = next; lifeHue = nextHue;
    // pure + ambient + no visitor nodes: when THIS game ends, deal the next one.
    // (Dark-at-rest and Game-of-Light own their lifecycles; churn mode never ends.)
    if (lifeRules.pure && lifeAmbient && !lifeNodes.length) {
      const h = lifeStateHash();
      lifeStagnant = live === 0 || lifeHist.includes(h) ? lifeStagnant + 1 : 0;
      lifeHist.push(h);
      if (lifeHist.length > 6) lifeHist.shift();
      if (live === 0 || lifeStagnant >= 4) {
        // a stuck still-life must not accumulate as permanent lit sculpture —
        // clear everything not tap-held, then seed the fresh 4-9 cluster
        for (let i = 0; i < n; i++) if (lifeTtl[i] <= 0) lifeCell[i] = 0;
        seedRandomCluster(fixtures);
      }
    }
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

/** A touch INJECTS chemistry into the reaction: a fresh blob of activator at the
 *  tapped lights that then drifts/splits by Gray-Scott dynamics — the tap grows
 *  a living blob instead of doing nothing (Elliot: RD must be interactive). */
export function exciteOrganism(indices: number[]) {
  for (const i of indices) if (i >= 0 && i < gv.length) { gv[i] = 0.55; gu[i] = 0.25; }
  bumpExcite(indices, 1);
}

let gsAcc = 0; // fractional evolution steps (dial-scaled) carried across frames
export function updateOrganism(fixtures: SimFixture[], dt: number, speed: number) {
  const n = fixtures.length;
  if (gsN !== n) {
    gu = new Float32Array(n).fill(1); gv = new Float32Array(n);
    su = new Float32Array(n); sv = new Float32Array(n);
    organismOut.bri = new Float32Array(n); organismOut.hue = new Float32Array(n);
    for (let s = 0; s < 8; s++) { const i = Math.floor(Math.random() * n); gv[i] = 0.5; gu[i] = 0.25; }
    gsN = n;
  }
  const Du = 0.5, Dv = 0.25, F = caParams.rdFeed, k = caParams.rdKill; // feed/kill editable (Du/Dv fixed for stability)
  // MULTIPLICATIVE dial: ~480 steps/s at speed 1 (the old 8-per-frame at 60 fps),
  // a step every few frames at the glacial end, capped so fast can't hitch the CPU
  gsAcc += Math.min(0.1, Math.max(0, dt)) * Math.min(720, Math.max(10, speed * 480)); // band 10..720 steps/s — fast can't over-evolve the spots into a frozen steady state
  const steps = Math.min(24, Math.floor(gsAcc));
  gsAcc -= steps;
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
  if (excite.length === n) decayExcite(fixtures, Math.min(0.1, Math.max(0, dt)));
  // the reaction can go EXTINCT (v → 0 everywhere) — the organism must live on:
  // quietly re-inject a few spots so the mode never fades to permanent black
  let vsum = 0;
  for (let i = 0; i < n; i++) vsum += gv[i];
  if (vsum < 0.05) for (let sd = 0; sd < 8; sd++) { const i = Math.floor(Math.random() * n); gv[i] = 0.5; gu[i] = 0.25; }
  for (let i = 0; i < n; i++) {
    const v = gv[i];
    const gate = interactiveRest ? Math.max(0.04, excite[i]) : 1; // quiet at rest, blob blooms where touched
    organismOut.bri[i] = Math.min(1, v * 2.6 * gate);
    organismOut.hue[i] = themeMapHue((0.62 - Math.min(1, v / (gu[i] + v + 1e-3)) * 0.42 + 1) % 1); // ratio → blue→green, pulled into the theme
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
