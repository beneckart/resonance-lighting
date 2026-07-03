import type { SimFixture } from "./store";

/** AUTO-CALIBRATION & TESTING (Elliot's spec): all lights OFF, then one by one —
 *  group by group — each light comes on solo, runs a quick colour + brightness
 *  check, goes off, and the sequence moves on. Each solo-lit moment is:
 *   · a CONTROL→REPORT round-trip test (the twin's mirror loop = the ESP-NOW
 *     heartbeat on real hardware): does the light report the colour we sent?
 *   · the PHOTOGRAMMETRY capture window (cameras see exactly ONE light → its
 *     blob can be triangulated; the log ties frame-time → fixture id).
 *   · a ToF SELF-LOCATION sample: each lantern's downward ranger reports its
 *     height histogram; compared to the model it helps locate lights in space. */

export type CalColor = "red" | "green" | "blue" | "white";
export const CAL_RGB: Record<CalColor, [number, number, number]> = {
  red: [1, 0, 0], green: [0, 1, 0], blue: [0, 0, 1], white: [1, 1, 1],
};

export interface CalStep { idx: number; num: number; id: string; group: string; color: CalColor }

/** Build the full plan: groups in canonical order, each group's lights by number,
 *  each light tested through the chosen colours. A light in several groups is
 *  tested only once (its first group). */
export function buildPlan(
  fixtures: SimFixture[],
  namedGroups: Record<string, number[]>,
  colors: CalColor[],
): CalStep[] {
  const order = ["ring1", "ring2", "ring3", "uplights", "chandelier",
    ...Object.keys(namedGroups).filter((g) => !["ring1", "ring2", "ring3", "uplights", "chandelier", "all"].includes(g))];
  const byNum = new Map<number, { f: SimFixture; idx: number }>();
  fixtures.forEach((f, idx) => byNum.set(f.num, { f, idx }));
  const seen = new Set<number>();
  const steps: CalStep[] = [];
  for (const g of order) {
    const nums = (namedGroups[g] ?? []).slice().sort((a, b) => a - b);
    for (const num of nums) {
      if (seen.has(num)) continue;
      const e = byNum.get(num);
      if (!e) continue;
      seen.add(num);
      for (const color of colors) steps.push({ idx: e.idx, num, id: e.f.id, group: g, color });
    }
  }
  // any fixture not in a named group still gets tested (appended as group "other")
  for (const [num, e] of byNum) if (!seen.has(num)) { seen.add(num); for (const color of colors) steps.push({ idx: e.idx, num, id: e.f.id, group: "other", color }); }
  return steps;
}

export interface CalVerdict { num: number; id: string; group: string; ok: boolean; worstDelta: number; failedColor?: CalColor }

/** Judge one light's colour test: reported RGB (from the heartbeat/telemetry)
 *  vs commanded, within tolerance. Dead/stale lights report nothing → fail. */
export function judgeColor(expected: [number, number, number], reported: [number, number, number] | null, tol = 0.18): { ok: boolean; delta: number } {
  if (!reported) return { ok: false, delta: 1 };
  // compare NORMALISED colour (direction, not magnitude) + require real output —
  // master/brightness scaling must not fail an otherwise-correct light
  const mag = Math.max(reported[0], reported[1], reported[2]);
  if (mag < 0.05) return { ok: false, delta: 1 };
  const em = Math.max(expected[0], expected[1], expected[2], 1e-6);
  let delta = 0;
  for (let k = 0; k < 3; k++) delta = Math.max(delta, Math.abs(expected[k] / em - reported[k] / mag));
  return { ok: delta <= tol, delta };
}

/** Simulated ToF self-location sample: the lantern's downward ranger measures its
 *  height above ground (± sensor noise). On real hardware this arrives in the
 *  heartbeat; here it's derived from the model + noise so the pipeline is real. */
export function tofSample(f: SimFixture, groundY: number): { trueH: number; measuredH: number; errCm: number } {
  const trueH = Math.max(0.1, f.pos[1] - groundY);
  const noise = (Math.sin(f.rnd * 987.31) * 0.5 + Math.sin(f.rnd * 131.7)) * 0.5; // deterministic per fixture
  const measuredH = trueH * (1 + noise * 0.02) + noise * 0.03;
  return { trueH, measuredH, errCm: (measuredH - trueH) * 100 };
}

/** Bin ToF errors for the histogram display (cm). */
export function tofHistogram(errsCm: number[], binW = 2, span = 12): { x0: number; n: number }[] {
  const bins: { x0: number; n: number }[] = [];
  for (let x = -span; x < span; x += binW) bins.push({ x0: x, n: 0 });
  for (const e of errsCm) {
    const i = Math.floor((Math.max(-span, Math.min(span - 0.001, e)) + span) / binW);
    if (bins[i]) bins[i].n++;
  }
  return bins;
}
