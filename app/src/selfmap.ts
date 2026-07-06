import type { SimFixture } from "./store";

/** SELF-MAPPING — the mesh locates itself on the tree (Elliot's staged sync spec).
 *
 *  The problem: ~118 physical lights announce MAC ids on the ESP-NOW mesh, but
 *  nobody knows which MAC hangs WHERE. The 3-D model (fixtures.json) says where
 *  lights are SUPPOSED to be — but the real install will drift from the model.
 *
 *  The staged answer (a confidence ladder, each stage refines the last):
 *   1. MESH 2-D  — every node reports per-neighbor RSSI (Ben's net_bench: ESP-NOW
 *      exposes RSSI per packet; ±8–17 dB per-board spread ⇒ RSSI is a TOPOLOGY
 *      signal, not a tape measure). We embed the pairwise-distance matrix in 2-D
 *      (weighted stress majorization / SMACOF).
 *   2. ToF VERTICAL — each lantern's downward VL53 ranger reports height above
 *      ground (the presence sensor moonlights as an altimeter). That fixes the
 *      vertical coordinate absolutely.
 *   3. MODEL PRIOR — the embedded cloud is aligned to the model's slot cloud
 *      (anchor-based similarity transform when ≥2 lights are already confirmed;
 *      otherwise PCA axes × 4 reflection/flip candidates, best total cost wins),
 *      then MAC→slot assignment (greedy-global, role-restricted) with a
 *      per-fixture CONFIDENCE = margin over the runner-up slot.
 *   4. INSTALL CONFIRM — the installer solo-flashes ONLY the low-confidence
 *      lights and confirms/corrects (AutoCal + CommissioningPanel flow).
 *   5. PHOTOGRAMMETRY — solo-step capture triangulates true positions, residuals
 *      checked, map LOCKED.
 *
 *  The point of stages 1–3 is NOT centimetres — it's slot IDENTITY. Fusion should
 *  shrink the installer's manual pass from "all 118" to "the uncertain ~dozen". */

// ── stage ladder ──────────────────────────────────────────────────────────────
export type MapStage =
  | "heard" // MAC announced on the mesh
  | "ranged" // has an RSSI neighbor table
  | "placed" // embedded in the 2-D plan
  | "height" // ToF vertical fix attached
  | "hypothesis" // matched to a model slot (unconfirmed)
  | "confirmed" // installer verified (solo flash → tap)
  | "locked"; // photogrammetry residual accepted; frozen

export const STAGE_ORDER: MapStage[] = ["heard", "ranged", "placed", "height", "hypothesis", "confirmed", "locked"];
export function stageRank(s: MapStage): number { return STAGE_ORDER.indexOf(s); }

// ── RSSI ⇄ distance (log-distance path-loss) ─────────────────────────────────
/** Bench-anchored defaults: net_bench saw ≈-25…-33 dBm co-located and ≈-70 dBm
 *  far/obstructed with the cliff near -90 dBm. txRefDbm = RSSI at 1 m. */
export interface RssiModel { txRefDbm: number; pathLossExp: number }
export const DEFAULT_RSSI_MODEL: RssiModel = { txRefDbm: -40, pathLossExp: 2.2 };

export function distanceToRssi(d: number, m: RssiModel = DEFAULT_RSSI_MODEL): number {
  return m.txRefDbm - 10 * m.pathLossExp * Math.log10(Math.max(0.1, d));
}
export function rssiToDistance(rssi: number, m: RssiModel = DEFAULT_RSSI_MODEL): number {
  return Math.pow(10, (m.txRefDbm - rssi) / (10 * m.pathLossExp));
}

// ── survey data (what the wire carries — see syncproto.ts) ───────────────────
export interface RssiRow { mac: string; med: number; n: number }
export interface SurveyNode {
  mac: string;
  role: string; // the node KNOWS its hardware role (burned in flash at build time)
  rows: RssiRow[]; // per-neighbor median RSSI
  tofHeightM: number | null; // downward ToF height above ground (m), null = no fix
}

// ── deterministic PRNG (no Math.random — sim must be reproducible) ───────────
export function lcg(seed: number): () => number {
  let s = (seed >>> 0) || 1;
  return () => {
    s = (Math.imul(s, 1664525) + 1013904223) >>> 0;
    return s / 4294967296;
  };
}

/** Gaussian-ish (sum of 3 uniforms, mean 0, ~unit variance). */
function gauss(rnd: () => number): number {
  return (rnd() + rnd() + rnd() - 1.5) * 2;
}

// ── SIMULATED SURVEY (the sim side of the mirror; real hardware replaces this) ─
export interface SimSurveyOpts {
  placementJitterM: number; // how far the REAL install drifts from the model
  boardOffsetDb: number; // per-board RSSI bias spread (net_bench saw 8–17 dB)
  pingNoiseDb: number; // per-median residual noise after n pings
  maxRangeM: number; // beyond this the packet is lost (≈ -90 dBm floor)
  tofSigmaM: number; // ToF height noise
  tofDropout: number; // fraction of nodes whose ToF has no ground return
  seed: number;
}
export const DEFAULT_SIM_SURVEY: SimSurveyOpts = {
  placementJitterM: 0.35,
  boardOffsetDb: 8,
  pingNoiseDb: 2,
  maxRangeM: 30,
  tofSigmaM: 0.08,
  tofDropout: 0.05,
  seed: 20260705,
};

export function macFromNum(num: number): string {
  // stable fake compact MAC per light number (sim stand-in for the real last-3-bytes id)
  const h = (Math.imul(num + 7, 2654435761) >>> 8) & 0xffffff;
  return h.toString(16).toUpperCase().padStart(6, "0");
}

export interface SimSurveyResult {
  nodes: SurveyNode[];
  /** ground truth for scoring the pipeline (sim-only; real hardware has none) */
  truth: { mac: string; fixtureId: string; truePos: [number, number, number] }[];
  groundY: number;
}

/** Synthesize what the real mesh would report: true positions = model + jitter,
 *  RSSI from true distances + per-board bias + noise, ToF from true height. */
export function simulateSurvey(fixtures: SimFixture[], opts: Partial<SimSurveyOpts> = {}): SimSurveyResult {
  const o = { ...DEFAULT_SIM_SURVEY, ...opts };
  const rnd = lcg(o.seed);
  const groundY = Math.min(...fixtures.map((f) => f.pos[1])) - 0.5;
  const truePos: [number, number, number][] = fixtures.map((f) => [
    f.pos[0] + gauss(rnd) * o.placementJitterM,
    Math.max(groundY + 0.2, f.pos[1] + gauss(rnd) * o.placementJitterM * 0.6),
    f.pos[2] + gauss(rnd) * o.placementJitterM,
  ]);
  const boardOffset = fixtures.map(() => gauss(rnd) * (o.boardOffsetDb / 2));
  const nodes: SurveyNode[] = fixtures.map((f, i) => {
    const rows: RssiRow[] = [];
    for (let j = 0; j < fixtures.length; j++) {
      if (j === i) continue;
      const d = Math.hypot(
        truePos[i][0] - truePos[j][0],
        truePos[i][1] - truePos[j][1],
        truePos[i][2] - truePos[j][2],
      );
      if (d > o.maxRangeM) continue;
      const rssi = distanceToRssi(d) + boardOffset[i] + boardOffset[j] + gauss(rnd) * o.pingNoiseDb;
      if (rssi < -90) continue; // below the floor — packet never heard
      rows.push({ mac: macFromNum(fixtures[j].num), med: Math.round(rssi * 10) / 10, n: 24 });
    }
    const hasTof = rnd() > o.tofDropout;
    return {
      mac: macFromNum(f.num),
      role: f.role,
      rows,
      tofHeightM: hasTof ? Math.max(0.1, truePos[i][1] - groundY + gauss(rnd) * o.tofSigmaM) : null,
    };
  });
  return {
    nodes,
    truth: fixtures.map((f, i) => ({ mac: macFromNum(f.num), fixtureId: f.id, truePos: truePos[i] })),
    groundY,
  };
}

// ── 2-D embedding (weighted SMACOF stress majorization) ──────────────────────
/** Embed nodes in the horizontal plane from pairwise RSSI-derived distances.
 *
 *  KEY FUSION STEP: RSSI measures the 3-D range, but the plan we're solving is
 *  2-D — a crown light 20 m above a ground light reads "20 m away" even when
 *  they hang on the same vertical. Each node's ToF height lets us project the
 *  measured range onto the plane first: dxy = √(d² − Δh²). Without that the
 *  vertical span of the tree poisons the whole embedding.
 *
 *  Near pairs get higher weight (RSSI is most trustworthy up close). SMACOF is
 *  restarted from several seeds; lowest weighted stress wins. Output is
 *  topology-correct but in an ARBITRARY frame (rotation/reflection/translation/
 *  scale unknown) — alignment to the model happens later. */
export interface EmbedGeometry {
  xy: Map<string, [number, number]>;
  macs: string[];
  /** bias-corrected 3-D ranges, indexed like `macs` (0 = never heard) */
  d3: number[][];
  /** best-known height above ground per mac (ToF, or fleet median fallback) */
  heightOf: (mac: string) => number;
}

export interface EmbedOpts {
  iters?: number;
  seed?: number;
  restarts?: number;
  biasRounds?: number;
  /** macs pinned to known plan positions (confirmed anchors) — they drag the
   *  whole embedding into the MODEL frame during SMACOF */
  pins?: Map<string, [number, number]>;
  /** warm-start positions (e.g. the slot positions of a candidate assignment);
   *  when given, replaces the random multi-restart */
  init?: Map<string, [number, number]>;
}

export function embedGeometry(
  nodes: SurveyNode[],
  model: RssiModel = DEFAULT_RSSI_MODEL,
  opts: EmbedOpts = {},
): EmbedGeometry {
  const n = nodes.length;
  // small clouds fold easily from a bad start (and are cheap) — restart more
  const { iters = 300, seed = 42, restarts = n < 20 ? 12 : 4, biasRounds = 4, pins, init } = opts;
  const idx = new Map(nodes.map((s, i) => [s.mac, i]));
  // fallback height for nodes without a ToF fix: fleet median
  const hs = nodes.map((s) => s.tofHeightM).filter((h): h is number => h !== null).sort((a, b) => a - b);
  const medH = hs.length ? hs[Math.floor(hs.length / 2)] : 0;
  const hOf = (s: SurveyNode) => (s.tofHeightM ?? medH);
  // symmetrized RAW 3-D range per pair (dB bias not yet removed)
  const D3: number[][] = Array.from({ length: n }, () => Array(n).fill(0));
  for (let i = 0; i < n; i++) {
    for (const row of nodes[i].rows) {
      const j = idx.get(row.mac);
      if (j === undefined || j === i) continue;
      const d3 = rssiToDistance(row.med, model);
      D3[i][j] = D3[j][i] = D3[i][j] > 0 ? (D3[i][j] + d3) / 2 : d3;
    }
  }
  // per-node multiplicative range bias: a board reading b dB hot shrinks ALL its
  // ranges by 10^(b/10n) — d_meas(i,j) = d_true·s_i·s_j. We estimate log-factors
  // β jointly with the embedding: embed → per-node median log-residual → correct
  // ranges → re-embed. (This is the sim rehearsal of real per-board RSSI
  // self-calibration; net_bench saw boards reading 8–17 dB apart.)
  const beta: number[] = Array(n).fill(0);
  const D: number[][] = Array.from({ length: n }, () => Array(n).fill(0));
  const W: number[][] = Array.from({ length: n }, () => Array(n).fill(0));
  const buildTargets = () => {
    for (let i = 0; i < n; i++) {
      for (let j = 0; j < n; j++) {
        if (D3[i][j] <= 0) { D[i][j] = 0; W[i][j] = 0; continue; }
        const d3c = D3[i][j] * Math.pow(10, beta[i] + beta[j]); // bias-corrected 3-D range
        const dh = hOf(nodes[i]) - hOf(nodes[j]);
        const d = Math.sqrt(Math.max(0.25, d3c * d3c - dh * dh)); // ToF-projected plan distance
        D[i][j] = d;
        W[i][j] = 1 / Math.max(1, d * d); // near pairs dominate
      }
    }
  };
  const pinIdx: [number, [number, number]][] = [];
  if (pins) {
    nodes.forEach((s, i) => {
      const p = pins.get(s.mac);
      if (p) pinIdx.push([i, p]);
    });
  }
  const applyPins = (X: [number, number][]) => {
    for (const [i, p] of pinIdx) X[i] = [p[0], p[1]];
  };
  const smacof = (X: [number, number][], rounds: number): [number, number][] => {
    applyPins(X);
    for (let it = 0; it < rounds; it++) {
      const Xn: [number, number][] = X.map(() => [0, 0]);
      const Wi: number[] = Array(n).fill(0);
      for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
          if (j === i || W[i][j] === 0) continue;
          const dx = X[i][0] - X[j][0];
          const dy = X[i][1] - X[j][1];
          const cur = Math.max(1e-6, Math.hypot(dx, dy));
          const s = D[i][j] / cur;
          Xn[i][0] += W[i][j] * (X[j][0] + dx * s);
          Xn[i][1] += W[i][j] * (X[j][1] + dy * s);
          Wi[i] += W[i][j];
        }
      }
      for (let i = 0; i < n; i++) {
        if (Wi[i] === 0) continue;
        X[i] = [Xn[i][0] / Wi[i], Xn[i][1] / Wi[i]];
      }
      applyPins(X);
    }
    return X;
  };
  const stressOf = (X: [number, number][]): number => {
    let stress = 0;
    for (let i = 0; i < n; i++) {
      for (let j = i + 1; j < n; j++) {
        if (W[i][j] === 0) continue;
        const cur = Math.hypot(X[i][0] - X[j][0], X[i][1] - X[j][1]);
        stress += W[i][j] * (cur - D[i][j]) * (cur - D[i][j]);
      }
    }
    return stress;
  };
  // zeroth-order β from row medians: a hot board reads EVERY partner nearer, so
  // its median log-range sits below the fleet's — half the offset is its own.
  // Only meaningful on fleet-sized surveys; on small clouds the layout itself
  // drives row medians and this would poison exact data.
  if (n >= 20) {
    const rowMed: number[] = Array(n).fill(0);
    for (let i = 0; i < n; i++) {
      const logs = [];
      for (let j = 0; j < n; j++) if (D3[i][j] > 0) logs.push(Math.log10(D3[i][j]));
      logs.sort((a, b) => a - b);
      rowMed[i] = logs.length ? logs[Math.floor(logs.length / 2)] : 0;
    }
    const fleetMed = [...rowMed].sort((a, b) => a - b)[Math.floor(n / 2)] ?? 0;
    for (let i = 0; i < n; i++) beta[i] = 0.5 * (fleetMed - rowMed[i]);
  }
  // initial embed: warm start when given, else multi-restart
  buildTargets();
  const meds = D.flat().filter((d) => d > 0).sort((a, b) => a - b);
  const R = (meds[Math.floor(meds.length / 2)] ?? 5) * 0.8;
  let X: [number, number][] | null = null;
  if (init) {
    const rnd = lcg(seed);
    const X0: [number, number][] = nodes.map((s) => {
      const p = init.get(s.mac);
      // tiny deterministic jitter so coincident warm-start points can separate
      return p ? [p[0] + (rnd() - 0.5) * 0.2, p[1] + (rnd() - 0.5) * 0.2] : [(rnd() - 0.5) * R, (rnd() - 0.5) * R];
    });
    X = smacof(X0, iters);
  } else {
    let bestStress = Infinity;
    for (let r = 0; r < restarts; r++) {
      const rnd = lcg(seed + r * 7919);
      const X0: [number, number][] = Array.from({ length: n }, () => {
        const a = rnd() * Math.PI * 2;
        const rad = R * (0.3 + rnd() * 0.7);
        return [Math.cos(a) * rad, Math.sin(a) * rad];
      });
      const Xr = smacof(X0, iters);
      const st = stressOf(Xr);
      if (st < bestStress) { bestStress = st; X = Xr; }
    }
  }
  // alternate: estimate per-node bias from the embedding, rebuild targets, refine
  for (let round = 0; round < biasRounds; round++) {
    for (let i = 0; i < n; i++) {
      const resids: number[] = [];
      for (let j = 0; j < n; j++) {
        if (j === i || D3[i][j] <= 0) continue;
        const dxyEmb = Math.hypot(X![i][0] - X![j][0], X![i][1] - X![j][1]);
        const dh = hOf(nodes[i]) - hOf(nodes[j]);
        const d3Emb = Math.hypot(dxyEmb, dh);
        const d3Cur = D3[i][j] * Math.pow(10, beta[i] + beta[j]);
        resids.push(Math.log10(Math.max(0.1, d3Emb)) - Math.log10(Math.max(0.1, d3Cur)));
      }
      if (!resids.length) continue;
      resids.sort((a, b) => a - b);
      beta[i] += 0.6 * resids[Math.floor(resids.length / 2)];
    }
    // zero-mean the bias vector: a uniform β shift is a global rescale, which is
    // unobservable here (alignment owns scale) and otherwise feeds back into
    // collapse — the embedding shrinks, every β chases it, repeat.
    const meanB = beta.reduce((s, b) => s + b, 0) / n;
    for (let i = 0; i < n; i++) beta[i] -= meanB;
    buildTargets();
    X = smacof(X!, Math.max(60, Math.floor(iters / 3)));
  }
  const d3corr: number[][] = Array.from({ length: n }, (_, i) =>
    Array.from({ length: n }, (_, j) => (D3[i][j] > 0 ? D3[i][j] * Math.pow(10, beta[i] + beta[j]) : 0)),
  );
  return {
    xy: new Map(nodes.map((s) => [s.mac, X![idx.get(s.mac)!] as [number, number]])),
    macs: nodes.map((s) => s.mac),
    d3: d3corr,
    heightOf: (mac: string) => {
      const i = idx.get(mac);
      return i === undefined ? medH : hOf(nodes[i]);
    },
  };
}

/** Back-compat/simple view: just the embedded plan positions. */
export function embed2d(
  nodes: SurveyNode[],
  model: RssiModel = DEFAULT_RSSI_MODEL,
  iters = 300,
  seed = 42,
  restarts?: number,
  biasRounds?: number,
): Map<string, [number, number]> {
  return embedGeometry(nodes, model, { iters, seed, restarts, biasRounds }).xy;
}

// ── alignment to the model frame ─────────────────────────────────────────────
/** Model slots in solver coordinates: XY = horizontal plane (three-space x,z),
 *  H = height above ground. */
export interface Slot { fixtureId: string; role: string; xy: [number, number]; h: number }
export function slotsFromFixtures(fixtures: SimFixture[], groundY?: number): Slot[] {
  const g = groundY ?? Math.min(...fixtures.map((f) => f.pos[1])) - 0.5;
  return fixtures.map((f) => ({ fixtureId: f.id, role: f.role, xy: [f.pos[0], f.pos[2]], h: f.pos[1] - g }));
}

export interface Anchor { mac: string; fixtureId: string }

interface Xform { rot: [[number, number], [number, number]]; t: [number, number]; scale: number }
/** Apply a similarity transform (kept for tooling/visualization). */
export function applyXform(x: Xform, p: [number, number]): [number, number] {
  return [
    x.scale * (x.rot[0][0] * p[0] + x.rot[0][1] * p[1]) + x.t[0],
    x.scale * (x.rot[1][0] * p[0] + x.rot[1][1] * p[1]) + x.t[1],
  ];
}

/** Similarity transform (rotation+reflection+scale+translation) from paired
 *  points (Umeyama / orthogonal-Procrustes in 2-D, closed form). */
export function similarityFromPairs(src: [number, number][], dst: [number, number][]): Xform | null {
  const n = Math.min(src.length, dst.length);
  if (n < 2) return null;
  let sx = 0, sy = 0, dx = 0, dy = 0;
  for (let i = 0; i < n; i++) { sx += src[i][0]; sy += src[i][1]; dx += dst[i][0]; dy += dst[i][1]; }
  sx /= n; sy /= n; dx /= n; dy /= n;
  // cross-covariance a b / c d  (dst·srcᵀ), plus src variance
  let a = 0, b = 0, c = 0, d = 0, varS = 0;
  for (let i = 0; i < n; i++) {
    const ux = src[i][0] - sx, uy = src[i][1] - sy;
    const vx = dst[i][0] - dx, vy = dst[i][1] - dy;
    a += vx * ux; b += vx * uy; c += vy * ux; d += vy * uy;
    varS += ux * ux + uy * uy;
  }
  // closed-form 2-D Procrustes with optional reflection: R maximizing tr(RᵀM)
  // rotation candidate
  const thetaR = Math.atan2(c - b, a + d);
  // reflection candidate (R = rot(theta)·diag(1,-1))
  const thetaF = Math.atan2(c + b, a - d);
  const trR = (a + d) * Math.cos(thetaR) + (c - b) * Math.sin(thetaR);
  const trF = (a - d) * Math.cos(thetaF) + (c + b) * Math.sin(thetaF);
  const useFlip = trF > trR;
  const th = useFlip ? thetaF : thetaR;
  const cosT = Math.cos(th), sinT = Math.sin(th);
  const rot: Xform["rot"] = useFlip
    ? [[cosT, sinT], [sinT, -cosT]]
    : [[cosT, -sinT], [sinT, cosT]];
  const scale = varS > 1e-9 ? Math.max(trF, trR) / varS : 1;
  const s = Math.max(0.2, Math.min(5, scale)); // guard degenerate scales
  const t: [number, number] = [
    dx - s * (rot[0][0] * sx + rot[0][1] * sy),
    dy - s * (rot[1][0] * sx + rot[1][1] * sy),
  ];
  return { rot, t, scale: s };
}

/** PCA-candidate transforms when NO anchors exist: center both clouds, scale by
 *  RMS radius, then try principal-axis rotation × {identity, 180°} × {no flip,
 *  flip} — 4 candidates; caller picks the one whose assignment costs least. */
export function pcaCandidates(src: [number, number][], dst: [number, number][]): Xform[] {
  const stats = (pts: [number, number][]) => {
    let mx = 0, my = 0;
    for (const p of pts) { mx += p[0]; my += p[1]; }
    mx /= pts.length; my /= pts.length;
    let xx = 0, xy = 0, yy = 0, r = 0;
    for (const p of pts) {
      const ux = p[0] - mx, uy = p[1] - my;
      xx += ux * ux; xy += ux * uy; yy += uy * uy; r += ux * ux + uy * uy;
    }
    const theta = 0.5 * Math.atan2(2 * xy, xx - yy); // principal axis
    return { mx, my, theta, rms: Math.sqrt(r / pts.length) };
  };
  const S = stats(src), T = stats(dst);
  const scale = S.rms > 1e-9 ? T.rms / S.rms : 1;
  const out: Xform[] = [];
  for (const extra of [0, Math.PI]) {
    for (const flip of [false, true]) {
      const th = T.theta - (flip ? -S.theta : S.theta) + extra;
      const cosT = Math.cos(th), sinT = Math.sin(th);
      const rot: Xform["rot"] = flip
        ? [[cosT, sinT], [sinT, -cosT]]
        : [[cosT, -sinT], [sinT, cosT]];
      const t: [number, number] = [
        T.mx - scale * (rot[0][0] * S.mx + rot[0][1] * S.my),
        T.my - scale * (rot[1][0] * S.mx + rot[1][1] * S.my),
      ];
      out.push({ rot, t, scale });
    }
  }
  return out;
}

// ── assignment (MAC → model slot) ─────────────────────────────────────────────
export interface MapEstimate {
  mac: string;
  fixtureId: string | null; // null = no slot available (surplus node)
  stage: MapStage;
  confidence: number; // 0..1 (margin over runner-up, damped by absolute cost)
  estXY: [number, number]; // aligned solver-frame estimate (three-space x,z)
  estH: number | null; // ToF height (m above ground), null = no fix
  costM: number; // distance to the assigned slot (m) in fused space
}

export interface SolveResult {
  estimates: MapEstimate[];
  stress: number; // mean |estimated - slot| over assigned pairs (m)
  usedAnchors: number;
  candidateIndex: number; // which PCA candidate won (-1 if anchors used)
}

const H_WEIGHT = 4; // vertical is MEASURED (ToF σ≈8 cm); mesh XY is inferred — height dominates

function fusedCost(estXY: [number, number], estH: number | null, slot: Slot): number {
  const dx = estXY[0] - slot.xy[0];
  const dy = estXY[1] - slot.xy[1];
  const dh = estH === null ? 0 : (estH - slot.h) * H_WEIGHT;
  return Math.hypot(dx, dy, dh);
}

// ── 2-opt refinement (the QAP polish) ────────────────────────────────────────
/** The embedding gets the assignment ROUGHLY right; this fixes it against the
 *  thing we actually measured. Objective: how well do the bias-corrected pair
 *  RANGES fit the slot geometry the assignment implies (log-domain, since RSSI
 *  noise is additive in dB)? Swap two lights' slots / move a light to a free
 *  slot whenever that improves the fit; repeat until no move helps. Returns the
 *  per-node margin (how much the objective worsens under the node's best
 *  alternative move) — the honest confidence signal. */
function refineAssignment(
  geo: EmbedGeometry,
  nodes: SurveyNode[],
  slots: Slot[],
  assign: (Slot | null)[],
  locked: boolean[],
  maxSweeps = 8,
  kicks = 8,
  seed = 1337,
): { margins: number[]; J: number } {
  const n = nodes.length;
  const slotP = (s: Slot): [number, number, number] => [s.xy[0], s.xy[1], s.h];
  const term = (i: number, j: number, si: Slot, sj: Slot): number => {
    const d = geo.d3[i][j];
    if (d <= 0) return 0;
    const a = slotP(si), b = slotP(sj);
    const ds = Math.max(0.3, Math.hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]));
    const e = Math.log(d) - Math.log(ds);
    return (1 / (1 + d)) * e * e; // near pairs are the trustworthy ones
  };
  const nodeCost = (i: number, si: Slot): number => {
    let c = 0;
    for (let j = 0; j < n; j++) {
      if (j === i) continue;
      const sj = assign[j];
      if (sj) c += term(i, j, si, sj);
    }
    return c;
  };
  const totalCost = (): number => {
    let c = 0;
    for (let i = 0; i < n; i++) {
      const si = assign[i];
      if (!si) continue;
      for (let j = i + 1; j < n; j++) {
        const sj = assign[j];
        if (sj) c += term(i, j, si, sj);
      }
    }
    return c;
  };
  const freeSlots = () => {
    const used = new Set(assign.filter(Boolean).map((s) => s!.fixtureId));
    return slots.filter((s) => !used.has(s.fixtureId));
  };
  const descend = () => {
    for (let sweep = 0; sweep < maxSweeps; sweep++) {
      let improved = false;
      const free = freeSlots();
      for (let i = 0; i < n; i++) {
        if (locked[i] || !assign[i]) continue;
        const si = assign[i]!;
        const baseI = nodeCost(i, si);
        let bestDelta = -1e-9;
        let bestMove: { kind: "swap"; k: number } | { kind: "move"; slot: Slot } | null = null;
        // swaps
        for (let k = 0; k < n; k++) {
          if (k === i || locked[k] || !assign[k]) continue;
          if (nodes[k].role !== nodes[i].role) continue;
          const sk = assign[k]!;
          const before = baseI + nodeCost(k, sk) - term(i, k, si, sk); // (i,k) counted once
          // nodeCost(i,sk) still sees assign[k]=sk (stale) and nodeCost(k,si) sees
          // assign[i]=si — strip those stale cross-terms, add the real post-swap one
          const after = nodeCost(i, sk) - term(i, k, sk, sk) + nodeCost(k, si) - term(i, k, si, si) + term(i, k, sk, si);
          const delta = after - before;
          if (delta < bestDelta) { bestDelta = delta; bestMove = { kind: "swap", k }; }
        }
        // moves to free same-role slots
        for (const s of free) {
          if (s.role !== nodes[i].role) continue;
          const delta = nodeCost(i, s) - baseI;
          if (delta < bestDelta) { bestDelta = delta; bestMove = { kind: "move", slot: s }; }
        }
        if (bestMove) {
          if (bestMove.kind === "swap") {
            const tmp = assign[i];
            assign[i] = assign[bestMove.k];
            assign[bestMove.k] = tmp;
          } else {
            const idx = free.indexOf(bestMove.slot);
            if (idx >= 0) free.splice(idx, 1);
            free.push(assign[i]!);
            assign[i] = bestMove.slot;
          }
          improved = true;
        }
      }
      if (!improved) break;
    }
  };
  descend();
  // iterated local search: kick a few random same-role swaps, re-descend, keep
  // only if the global fit improved — escapes the ring-rotation local minima
  // that plain 2-opt cannot cross downhill
  const rnd = lcg(seed);
  let bestJ = totalCost();
  let bestAssign = assign.slice();
  for (let kick = 0; kick < kicks; kick++) {
    for (let s = 0; s < 3; s++) {
      const i = Math.floor(rnd() * n);
      const k = Math.floor(rnd() * n);
      if (i === k || locked[i] || locked[k] || !assign[i] || !assign[k]) continue;
      if (nodes[i].role !== nodes[k].role) continue;
      const tmp = assign[i];
      assign[i] = assign[k];
      assign[k] = tmp;
    }
    descend();
    const j = totalCost();
    if (j < bestJ - 1e-9) {
      bestJ = j;
      bestAssign = assign.slice();
    } else {
      for (let i = 0; i < n; i++) assign[i] = bestAssign[i];
    }
  }
  // margins: how much worse is each node's best alternative? (0 = shaky, big = solid)
  const margins: number[] = Array(n).fill(0);
  const free = freeSlots();
  for (let i = 0; i < n; i++) {
    if (!assign[i]) { margins[i] = 0; continue; }
    if (locked[i]) { margins[i] = Infinity; continue; }
    const si = assign[i]!;
    const baseI = nodeCost(i, si);
    let minDelta = Infinity;
    for (let k = 0; k < n; k++) {
      if (k === i || !assign[k] || locked[k] || nodes[k].role !== nodes[i].role) continue;
      const sk = assign[k]!;
      const before = baseI + nodeCost(k, sk) - term(i, k, si, sk);
      const after = nodeCost(i, sk) - term(i, k, sk, sk) + nodeCost(k, si) - term(i, k, si, si) + term(i, k, sk, si);
      minDelta = Math.min(minDelta, after - before);
    }
    for (const s of free) {
      if (s.role !== nodes[i].role) continue;
      minDelta = Math.min(minDelta, nodeCost(i, s) - baseI);
    }
    margins[i] = minDelta === Infinity ? 1 : Math.max(0, minDelta);
  }
  return { margins, J: totalCost() };
}

/** Height-seeded initial assignment: within each role, rank nodes by measured
 *  ToF height and slots by model height, then pair rank-for-rank. ToF is the
 *  high-SNR sensor (σ ≈ 8 cm vs metres of RSSI fuzz) — this alone nearly sorts
 *  the fleet into the right ring/level before RSSI says a word about azimuth. */
export function heightSeedAssignment(
  nodes: SurveyNode[],
  slots: Slot[],
  anchored: Map<string, string>,
): (Slot | null)[] {
  const slotById = new Map(slots.map((s) => [s.fixtureId, s]));
  const out: (Slot | null)[] = nodes.map((node) => {
    const forced = anchored.get(node.mac);
    return forced ? (slotById.get(forced) ?? null) : null;
  });
  const usedSlots = new Set(out.filter(Boolean).map((s) => s!.fixtureId));
  const roles = new Set(nodes.map((s) => s.role));
  const hs = nodes.map((s) => s.tofHeightM).filter((h): h is number => h !== null).sort((a, b) => a - b);
  const medH = hs.length ? hs[Math.floor(hs.length / 2)] : 0;
  for (const role of roles) {
    const ns = nodes
      .map((node, i) => ({ node, i }))
      .filter(({ node, i }) => node.role === role && out[i] === null)
      .sort((a, b) => (a.node.tofHeightM ?? medH) - (b.node.tofHeightM ?? medH));
    const ss = slots
      .filter((s) => s.role === role && !usedSlots.has(s.fixtureId))
      .sort((a, b) => a.h - b.h);
    for (let k = 0; k < Math.min(ns.length, ss.length); k++) {
      out[ns[k].i] = ss[k];
      usedSlots.add(ss[k].fixtureId);
    }
  }
  return out;
}

/** Band-circular seed — the assignment primitive that matches how a tree is
 *  actually laid out. Per role, slots cluster into HEIGHT BANDS (rings/levels);
 *  ToF height tells each node its band; within a band the mesh embedding gets
 *  the circular ORDER largely right even when absolute positions are metres
 *  off. So: sort band slots by azimuth, sort band nodes by embedded azimuth,
 *  and try every rotation offset × both directions, scoring each candidate
 *  against the measured ranges (J). Anchors in a band force its offset. */
export function circularSeedAssignment(
  geo: EmbedGeometry,
  nodes: SurveyNode[],
  slots: Slot[],
  anchored: Map<string, string>,
): (Slot | null)[] {
  const n = nodes.length;
  const slotById = new Map(slots.map((s) => [s.fixtureId, s]));
  const assign: (Slot | null)[] = nodes.map((node) => {
    const forced = anchored.get(node.mac);
    return forced ? (slotById.get(forced) ?? null) : null;
  });
  const term = (i: number, j: number, si: Slot, sj: Slot): number => {
    const d = geo.d3[i][j];
    if (d <= 0) return 0;
    const ds = Math.max(0.3, Math.hypot(si.xy[0] - sj.xy[0], si.xy[1] - sj.xy[1], si.h - sj.h));
    const e = Math.log(d) - Math.log(ds);
    return (1 / (1 + d)) * e * e;
  };
  const hs = nodes.map((s) => s.tofHeightM).filter((h): h is number => h !== null).sort((a, b) => a - b);
  const medH = hs.length ? hs[Math.floor(hs.length / 2)] : 0;
  const usedSlots = new Set(assign.filter(Boolean).map((s) => s!.fixtureId));
  for (const role of new Set(nodes.map((s) => s.role))) {
    // height bands from the MODEL (split at gaps > 2.5 m); anchored slots stay
    // out of the rotation pool — anchors steer each band's offset SOFTLY via
    // their measured-range cross-terms in the J score below
    const roleSlots = slots.filter((s) => s.role === role && !usedSlots.has(s.fixtureId)).sort((a, b) => a.h - b.h);
    const bands: Slot[][] = [];
    for (const s of roleSlots) {
      const cur = bands[bands.length - 1];
      if (cur && s.h - cur[cur.length - 1].h <= 2.5) cur.push(s);
      else bands.push([s]);
    }
    // nodes of this role, unassigned, by measured height → fill bands by capacity
    const roleNodes = nodes
      .map((node, i) => ({ node, i }))
      .filter(({ node, i }) => node.role === role && assign[i] === null)
      .sort((a, b) => (a.node.tofHeightM ?? medH) - (b.node.tofHeightM ?? medH));
    let cursor = 0;
    for (const band of bands) {
      const members = roleNodes.slice(cursor, cursor + band.length);
      cursor += band.length;
      if (!members.length) continue;
      // sort band slots + band nodes by azimuth (each around its own centroid)
      let cx = 0, cy = 0;
      band.forEach((s) => { cx += s.xy[0]; cy += s.xy[1]; });
      cx /= band.length; cy /= band.length;
      const slotsSorted = band.slice().sort((a, b) => Math.atan2(a.xy[1] - cy, a.xy[0] - cx) - Math.atan2(b.xy[1] - cy, b.xy[0] - cx));
      let ex = 0, ey = 0;
      members.forEach(({ node }) => { const p = geo.xy.get(node.mac)!; ex += p[0]; ey += p[1]; });
      ex /= members.length; ey /= members.length;
      const azOf = ({ node }: { node: SurveyNode }) => {
        const p = geo.xy.get(node.mac)!;
        return Math.atan2(p[1] - ey, p[0] - ex);
      };
      const m = slotsSorted.length;
      // candidate = (direction, offset): node at sorted position k → slot (k + offset) mod m
      let best: { order: { node: SurveyNode; i: number }[]; offset: number } | null = null;
      let bestJ = Infinity;
      for (const dir of [1, -1]) {
        const order = members.slice().sort((a, b) => dir * (azOf(a) - azOf(b)));
        for (let offset = 0; offset < m; offset++) {
          // score candidate against measured ranges (within band + to already-assigned)
          const cand: [number, Slot][] = order.map(({ i }, k) => [i, slotsSorted[(k + offset) % m]]);
          let J = 0;
          for (let a = 0; a < cand.length; a++) {
            const [i, si] = cand[a];
            for (let b = a + 1; b < cand.length; b++) J += term(i, cand[b][0], si, cand[b][1]);
            for (let j = 0; j < n; j++) if (assign[j]) J += term(i, j, si, assign[j]!);
          }
          if (J < bestJ) { bestJ = J; best = { order, offset }; }
        }
      }
      if (best) {
        for (let k = 0; k < best.order.length; k++) {
          const { i } = best.order[k];
          const s = slotsSorted[(k + best.offset) % m];
          assign[i] = s;
          usedSlots.add(s.fixtureId);
        }
      }
    }
  }
  return assign;
}

/** Full solve — heights band it, the mesh orders it, the model shapes it,
 *  anchors pin it, 2-opt + kicks polish it:
 *  embed (anchors pinned) → band-circular seed → 2-opt/ILS against measured
 *  ranges → re-embed warm-started from the final assignment for the reported
 *  positions. */
export function solveMapping(
  nodes: SurveyNode[],
  slots: Slot[],
  anchors: Anchor[] = [],
  model: RssiModel = DEFAULT_RSSI_MODEL,
  seed = 42,
): SolveResult {
  const slotById = new Map(slots.map((s) => [s.fixtureId, s]));
  const anchored = new Map<string, string>();
  const pins = new Map<string, [number, number]>();
  for (const a of anchors) {
    const s = slotById.get(a.fixtureId);
    if (s) { pins.set(a.mac, s.xy); anchored.set(a.mac, a.fixtureId); }
  }
  const usedAnchors = pins.size;
  const locked = nodes.map((node) => anchored.has(node.mac));

  // multi-seed: a bad embedding start can strand the whole solve in a poor
  // basin — run a few and keep the assignment the measured ranges like best
  let best: { assign: (Slot | null)[]; margins: number[]; J: number } | null = null;
  for (const s of [seed, seed + 101, seed + 202]) {
    const geo0 = embedGeometry(nodes, model, { seed: s, pins: usedAnchors >= 2 ? pins : undefined });
    const assign = circularSeedAssignment(geo0, nodes, slots, anchored);
    const { margins, J } = refineAssignment(geo0, nodes, slots, assign, locked);
    if (!best || J < best.J) best = { assign, margins, J };
  }
  const { assign, margins } = best!;
  // final embed warm-started from the assignment → the reported twin positions
  const init = new Map<string, [number, number]>();
  nodes.forEach((node, i) => { if (assign[i]) init.set(node.mac, assign[i]!.xy); });
  const g = embedGeometry(nodes, model, { seed, pins: usedAnchors >= 2 ? pins : undefined, init });
  // confidence: margin relative to the fleet's median margin (median lands at 0.5)
  const finite = margins.filter((m, i) => Number.isFinite(m) && assign[i] && !locked[i]).sort((a, b) => a - b);
  const medMargin = Math.max(1e-6, finite[Math.floor(finite.length / 2)] ?? 1e-6);
  const estimates: MapEstimate[] = nodes.map((node, i) => {
    const p = g.xy.get(node.mac) ?? ([0, 0] as [number, number]);
    const slot = assign[i];
    if (!slot) {
      return { mac: node.mac, fixtureId: null, stage: node.rows.length ? "placed" : "heard", confidence: 0, estXY: p, estH: node.tofHeightM, costM: Infinity };
    }
    const conf = locked[i] ? 1 : Math.max(0, Math.min(1, margins[i] / (margins[i] + medMargin)));
    const stage: MapStage = locked[i] ? "confirmed" : "hypothesis";
    return { mac: node.mac, fixtureId: slot.fixtureId, stage, confidence: conf, estXY: p, estH: node.tofHeightM, costM: fusedCost(p, node.tofHeightM, slot) };
  });
  const assigned = estimates.filter((e) => e.fixtureId && e.costM !== Infinity);
  const stress = assigned.length ? assigned.reduce((s, e) => s + e.costM, 0) / assigned.length : 0;
  return { estimates, stress, usedAnchors, candidateIndex: usedAnchors >= 2 ? -1 : 0 };
}

/** Score a solve against sim ground truth: what fraction of MACs landed on their
 *  true slot? (Sim-only — this is the number that tells us whether the staged
 *  protocol will actually save install time.) */
export function scoreAgainstTruth(
  result: SolveResult,
  truth: { mac: string; fixtureId: string }[],
): { total: number; correct: number; accuracy: number; wrong: string[] } {
  const want = new Map(truth.map((t) => [t.mac, t.fixtureId]));
  let correct = 0;
  const wrong: string[] = [];
  let total = 0;
  for (const e of result.estimates) {
    const expect = want.get(e.mac);
    if (!expect) continue;
    total += 1;
    if (e.fixtureId === expect) correct += 1;
    else wrong.push(e.mac);
  }
  return { total, correct, accuracy: total ? correct / total : 0, wrong };
}

/** The install-time payoff metric: how many lights the installer must manually
 *  confirm = wrong assignments they'd HAVE to fix + low-confidence ones they
 *  SHOULD check (below `confFloor`). */
export function manualQueue(result: SolveResult, confFloor = 0.35): MapEstimate[] {
  return result.estimates
    .filter((e) => !e.fixtureId || (e.stage === "hypothesis" && e.confidence < confFloor))
    .sort((p, q) => p.confidence - q.confidence);
}
