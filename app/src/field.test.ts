import { describe, it, expect, beforeEach } from "vitest";
import { updateLife, seedLife, seedRandomCluster, setLifeState, lifeOut, clearLife, setLifeRules, getLifeRules, setFieldTheme, themeMapHue } from "./field";
import type { SimFixture } from "./store";
import { setPiece, resetPiano, updatePiano, keyBri, keyHue } from "./piano";

/** ALGORITHMIC CONSISTENCY CHECK — Game of Life on the neighbour graph (Elliot:
 *  "algorithmic check on the Game of Life to ensure consistency"). Exercises the
 *  engine's invariants: dark-at-rest really stays dark, nodes persist, ttl expires,
 *  brightness leaks decay, hues stay valid, the field never freezes or explodes. */

// a ring of n fixtures, each neighbouring its ±1..±3 — a graph like the real tree's k-NN
function ringFixtures(n: number): SimFixture[] {
  const fx: SimFixture[] = [];
  for (let i = 0; i < n; i++) {
    const a = (i / n) * Math.PI * 2 - Math.PI;
    fx.push({
      id: `F${i}`, name: `f${i}`, role: "downlight", zone: "mid",
      pos: [Math.cos(a) * 10, 2, Math.sin(a) * 10], norm: [0.5, 0.5, 0.5],
      seqT: i / n, seq: i, heightT: 0.5, ring: 1, quadrant: ((i * 4 / n) | 0) % 4, azimuth: a,
      num: i + 1, radialT: 0.8, rnd: (Math.sin(i * 127.1) * 43758.5453) % 1 < 0 ? ((Math.sin(i * 127.1) * 43758.5453) % 1) + 1 : (Math.sin(i * 127.1) * 43758.5453) % 1,
      neighbors: [(i + 1) % n, (i + n - 1) % n, (i + 2) % n, (i + n - 2) % n, (i + 3) % n, (i + n - 3) % n],
      beamDeg: 120, lumens: 450,
    });
  }
  return fx;
}

// run the engine for `secs` sim-seconds in small steps
function run(fx: SimFixture[], secs: number, speed: number, step = 0.05) {
  for (let t = 0; t < secs; t += step) updateLife(fx, step, speed);
}

const N = 60;
let fx: SimFixture[];

beforeEach(() => {
  fx = ringFixtures(N);
  // force a reseed by running once at a different count, then at N
  updateLife(ringFixtures(N + 1), 0.01, 1);
  setLifeState({ ambient: true, nodes: [], palette: "warm" });
  updateLife(fx, 0.01, 1); // reseeds for N
});

describe("Game of Life — algorithmic consistency", () => {
  it("outputs stay in range: bri finite 0..1.4+, hue always in [0,1)", () => {
    run(fx, 20, 2);
    for (let i = 0; i < N; i++) {
      expect(Number.isFinite(lifeOut.bri[i])).toBe(true);
      expect(lifeOut.bri[i]).toBeGreaterThanOrEqual(0);
      expect(lifeOut.bri[i]).toBeLessThanOrEqual(1.45);
      expect(lifeOut.hue[i]).toBeGreaterThanOrEqual(0);
      expect(lifeOut.hue[i]).toBeLessThan(1.0001);
    }
  });

  it("ambient field NEVER freezes or goes extinct (perpetual churn)", () => {
    run(fx, 10, 1);
    const snap1 = Array.from(lifeOut.bri);
    run(fx, 10, 1);
    const snap2 = Array.from(lifeOut.bri);
    const changed = snap1.filter((v, i) => Math.abs(v - snap2[i]) > 0.05).length;
    expect(changed).toBeGreaterThan(2); // still evolving after 20s
    expect(snap2.some((v) => v > 0.3)).toBe(true); // not extinct
  });

  it("dark-at-rest: ambient off + no nodes → the tree goes fully dark and STAYS dark", () => {
    setLifeRules({ bLo: 2, bHi: 3, sLo: 1, sHi: 3, pure: false }); // burn-out physics = ORGANIC rules (pure exempts it by spec)
    setLifeState({ ambient: false, nodes: [] });
    run(fx, 60, 2); // fatigue guarantees burn-out; give it a realistic settle window
    const totalBri = Array.from(lifeOut.bri).reduce((a, b) => a + b, 0);
    expect(totalBri).toBeLessThan(0.5); // essentially dark everywhere
    // and a fresh touch WAKES it (fatigue resets) — the tree still responds
    seedLife([10], { hops: 2, hue: 0.5, bri: 1.2, ttl: 2 });
    run(fx, 1, 2);
    expect(lifeOut.bri[10]).toBeGreaterThan(0.4);
    setLifeRules({ bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true }); // restore the Conway-mesh default
  });

  it("a visitor NODE keeps its region alive in dark-at-rest (patterns emanate)", () => {
    setLifeState({ ambient: false, nodes: [{ i: 7, hue: 0.55 }] });
    run(fx, 20, 1);
    expect(lifeOut.bri[7]).toBeGreaterThan(0.5); // the node itself is lit
    expect(Math.abs(lifeOut.hue[7] - 0.55)).toBeLessThan(0.02); // in its colour
    // its neighbourhood shows life; far side of the ring is dark
    const near = fx[7].neighbors.reduce((a, j) => a + lifeOut.bri[j], 0);
    const far = [37, 38, 39].reduce((a, j) => a + lifeOut.bri[j], 0);
    expect(near).toBeGreaterThan(far);
  });

  it("time-on (ttl): a held cell stays lit its full duration, then fades", () => {
    setLifeState({ ambient: false, nodes: [] });
    run(fx, 25, 2); // clear the field
    seedLife([20], { hops: 0, hue: 0.3, bri: 1.2, ttl: 3 });
    run(fx, 1.5, 0.03); // glacial speed → the RULE won't tick; only ttl holds it
    expect(lifeOut.bri[20]).toBeGreaterThan(0.5); // still held on at 1.5s
    run(fx, 6, 0.03); // past the 3s ttl (+ fade time)
    expect(lifeOut.bri[20]).toBeLessThan(0.2); // released and faded
  });

  it("brightness never leaks: a bright touch decays back toward normal after death", () => {
    seedLife([5], { hops: 0, hue: 0.1, bri: 2.5, ttl: 0.5 });
    run(fx, 30, 2); // many generations after the hold expires
    // wherever cell 5 is now (alive or dead), its render bri must be ≤ ~1.4 cap and
    // not stuck at the 2.5 tap boost
    expect(lifeOut.bri[5]).toBeLessThanOrEqual(1.45);
  });

  it("random palette produces a genuinely multicoloured population", () => {
    setLifeState({ palette: "random" });
    run(fx, 15, 2);
    const hues = [];
    for (let i = 0; i < N; i++) if (lifeOut.bri[i] > 0.3) hues.push(lifeOut.hue[i]);
    const bins = new Set(hues.map((h) => Math.floor(h * 8)));
    expect(bins.size).toBeGreaterThanOrEqual(3); // several distinct colour families
  });

  it("speed scales the generation rate (fast churns, glacial barely moves)", () => {
    setLifeState({ ambient: true, palette: "warm" });
    run(fx, 5, 3);
    const fastSnap = Array.from(lifeOut.bri);
    run(fx, 5, 3);
    const fastChanged = fastSnap.filter((v, i) => Math.abs(v - lifeOut.bri[i]) > 0.05).length;
    // now glacial: 0.03 → ~2min/gen → in 5s the CELLS can't tick at all
    run(fx, 5, 0.03);
    const slowSnap = Array.from(lifeOut.bri);
    run(fx, 5, 0.03);
    const slowChanged = slowSnap.filter((v, i) => Math.abs(v - lifeOut.bri[i]) > 0.05).length;
    expect(fastChanged).toBeGreaterThan(slowChanged);
  });
});

describe("Conway-mesh default + new-game watchdog (Elliot 2026-07-08)", () => {
  it("default rules are pure Conway-mesh B2/S23", () => {
    expect(getLifeRules()).toEqual({ bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true });
  });

  it("seedRandomCluster lights a 4-9 cluster on a blank board", () => {
    setLifeRules({ bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true });
    setLifeState({ ambient: true, nodes: [], palette: "warm" });
    clearLife();
    seedRandomCluster(fx);
    updateLife(fx, 0.01, 0.03); // glacial speed → no generation ticks yet, just render
    const lit = Array.from({ length: N }, (_, i) => i).filter((i) => lifeOut.bri[i] > 0.01 || true);
    void lit;
    // count directly via a full-speed single frame render pass
    let alive = 0;
    for (let i = 0; i < N; i++) if (lifeOut.bri[i] > 0) alive++;
    expect(alive).toBeGreaterThanOrEqual(4);
    expect(alive).toBeLessThanOrEqual(9);
  });

  it("pure+ambient: an ended game auto-deals a fresh 4-9 seed (never stays dark)", () => {
    setLifeRules({ bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true });
    setLifeState({ ambient: true, nodes: [], palette: "warm" });
    clearLife(); // extinct board — the watchdog must revive it
    run(fx, 10, 1); // ~5 generations at speed 1
    let alive = 0;
    for (let i = 0; i < N; i++) if (lifeOut.bri[i] > 0.02) alive++;
    expect(alive).toBeGreaterThan(0);
  });

  it("pure+ambient: a frozen still-life is detected and replaced within a few generations", () => {
    setLifeRules({ bLo: 9, bHi: 9, sLo: 0, sHi: 8, pure: true }); // births impossible, nothing dies → instant still-life
    setLifeState({ ambient: true, nodes: [], palette: "warm" });
    clearLife();
    seedLife([0, 1, 2, 3, 4]);
    // stagnation (repeat hash) must trigger a clear + fresh 4-9 reseed every ~5
    // generations — over many generations at least one pair of snapshots differs
    const snaps: number[][] = [];
    for (let k = 0; k < 4; k++) { run(fx, 25, 2); snaps.push(Array.from(lifeOut.bri)); }
    let anyDiff = false;
    for (let a = 0; a < snaps.length; a++) for (let b = a + 1; b < snaps.length; b++) {
      if (snaps[a].some((v, i) => Math.abs(v - snaps[b][i]) > 0.05)) anyDiff = true;
    }
    expect(anyDiff).toBe(true);
    setLifeRules({ bLo: 2, bHi: 2, sLo: 2, sHi: 3, pure: true }); // restore default
  });
});

describe("editable rules + pure mode + blank start (Elliot 2026-07-06)", () => {
  beforeEach(() => {
    setLifeRules({ bLo: 2, bHi: 3, sLo: 1, sHi: 3, pure: false });
  });

  it("clearLife wipes the board to blank", () => {
    setLifeState({ ambient: true, nodes: [], palette: "warm" });
    run(fx, 3, 2);
    clearLife();
    setLifeState({ ambient: false, nodes: [] }); // no ambient reseed pressure
    setLifeRules({ pure: true }); // no churn either
    updateLife(fx, 0.05, 2);
    expect(Math.max(...lifeOut.bri)).toBeLessThan(0.05);
  });

  it("setLifeRules changes who is born: B-never means seeds never spread", () => {
    clearLife();
    setLifeState({ ambient: false, nodes: [] });
    setLifeRules({ bLo: 9, bHi: 9, sLo: 0, sHi: 8, pure: true }); // births impossible, survival free
    seedLife([10], { hops: 0 });
    run(fx, 6, 2);
    // the seeded cell survives (S0-8) but NOTHING else was ever born (B9)
    const lit = [...lifeOut.bri].map((b, i) => (b > 0.15 ? i : -1)).filter((i) => i >= 0);
    expect(lit).toContain(10);
    expect(lit.length).toBeLessThanOrEqual(1);
  });

  it("pure mode is deathless under a permissive rule; organic mode burns out", () => {
    // PURE + survive-anything: the population can never shrink
    clearLife();
    setLifeState({ ambient: false, nodes: [] });
    setLifeRules({ bLo: 2, bHi: 3, sLo: 0, sHi: 8, pure: true });
    seedLife([5, 6, 7], { hops: 1 });
    run(fx, 8, 2);
    const alivePure = [...lifeOut.bri].filter((b) => b > 0.15).length;
    expect(alivePure).toBeGreaterThan(0);
    // ORGANIC dark-at-rest: fatigue + ageing guarantee the same seeding burns out
    clearLife();
    setLifeRules({ bLo: 2, bHi: 3, sLo: 0, sHi: 8, pure: false });
    seedLife([5, 6, 7], { hops: 1 });
    run(fx, 60, 2);
    const aliveOrganic = [...lifeOut.bri].filter((b) => b > 0.15).length;
    expect(aliveOrganic).toBe(0);
  });

  it("getLifeRules round-trips setLifeRules", () => {
    setLifeRules({ bLo: 3, bHi: 3, sLo: 2, sHi: 3, pure: true });
    expect(getLifeRules()).toEqual({ bLo: 3, bHi: 3, sLo: 2, sHi: 3, pure: true });
  });
});

describe("theme map (one theme, every engine)", () => {
  it("pulls any hue into the theme's world and is identity when unthemed", () => {
    setFieldTheme([0.9, 0.0, 0.8]); // love-ish anchors
    const mapped = themeMapHue(0.45); // teal → must land near an anchor
    const dist = (a: number, b: number) => { const d = Math.abs(a - b) % 1; return Math.min(d, 1 - d); };
    expect(Math.min(dist(mapped, 0.9), dist(mapped, 0.0), dist(mapped, 0.8))).toBeLessThan(0.13);
    setFieldTheme(null);
    expect(themeMapHue(0.45)).toBe(0.45);
  });
});

describe("piano colours follow the picked theme (Elliot 2026-07-08)", () => {
  it("with an ocean theme set, every sounding key's hue lands near a theme anchor", () => {
    const anchors = [0.5, 0.55, 0.6, 0.65]; // ocean
    setFieldTheme(anchors);
    setPiece("moonlight");
    resetPiano();
    for (let t = 0; t <= 12; t += 0.25) updatePiano(t);
    let checked = 0;
    for (let k = 36; k <= 107; k++) if (keyBri[k] > 0.05) {
      checked++;
      const d = Math.min(...anchors.map((a) => { const x = Math.abs(a - keyHue[k]) % 1; return Math.min(x, 1 - x); }));
      expect(d).toBeLessThan(0.12); // themeMapHue keeps 22% of the offset — worst case 0.11
    }
    expect(checked).toBeGreaterThan(3); // the opening bars actually sounded
    setFieldTheme(null);
    setPiece("moonlight");
  });

  it("with no theme (Wild) the warm arc is untouched: hues avoid green/blue", () => {
    setFieldTheme(null);
    setPiece("moonlight");
    resetPiano();
    for (let t = 0; t <= 12; t += 0.25) updatePiano(t);
    for (let k = 36; k <= 107; k++) if (keyBri[k] > 0.05) {
      // warm arc = 0.75..1.15 (wraps): never inside the green/blue band 0.2..0.7
      expect(keyHue[k] > 0.2 && keyHue[k] < 0.7).toBe(false);
    }
  });
});
