import { Color } from "three";
import { PATTERN_IDS, type Control, type PatternId, type SimFixture } from "./store";

export interface Override {
  mode: "color" | "off";
  rgb?: [number, number, number];
}

export interface CmdResult {
  control?: Partial<Control>;
  setOverrides?: { idx: number[]; op: Override };
  clear?: boolean;
  msg: string;
}

const clampNum = (v: string, a: number, b: number) => {
  const n = parseFloat(v);
  return isNaN(n) ? a : Math.min(b, Math.max(a, n));
};

const parseColor = (tok: string): [number, number, number] | null => {
  try {
    const c = new Color(tok);
    return [c.r, c.g, c.b];
  } catch {
    return null;
  }
};

const GLOBALS: Record<string, (v: string) => Partial<Control> | null> = {
  pattern: (v) => (PATTERN_IDS.includes(v as PatternId) ? { pattern: v as PatternId } : null),
  hue: (v) => ({ hue: clampNum(v, 0, 1) }),
  bri: (v) => ({ brightness: clampNum(v, 0, 1) }),
  brightness: (v) => ({ brightness: clampNum(v, 0, 1) }),
  sat: (v) => ({ sat: clampNum(v, 0, 1) }),
  speed: (v) => ({ speed: clampNum(v, 0, 3) }),
};

/**
 * Parse a free-form lighting command into a state mutation. Grammar:
 *   clear | on | off
 *   all <pattern|hue|bri|sat|speed> <value>        (global)
 *   hue 0.5 / speed 2 / pattern sequence            (global shorthand)
 *   <target> <color #hex|name | on | off>           (per-fixture override)
 *     target = all | zone <low|mid|high> | range <a-b> | every <n> | fixture <id|seq>
 *            | light <n|list|range>   (by addressable number 1..72: "light 1,7,17")
 * Addressing uses azimuth order (seq) for range/every — "range 0-23" = first 24 around the tree.
 * `light` addresses by the 1..72 number: "light 7 color blue", "light 1,7,17 color red".
 */
export function runCommandStr(cmd: string, fixtures: SimFixture[]): CmdResult {
  const t = cmd.trim().toLowerCase().split(/\s+/).filter(Boolean);
  if (!t.length) return { msg: "" };

  if (t[0] === "clear") return { clear: true, msg: "overrides cleared" };
  if (t[0] === "off" && t.length === 1)
    return { setOverrides: { idx: fixtures.map((_, i) => i), op: { mode: "off" } }, msg: "all off" };
  if (t[0] === "on" && t.length === 1) return { clear: true, msg: "all on (overrides cleared)" };

  // global shorthand: "hue 0.5", "all hue 0.5"
  if (t[0] === "all" && t.length >= 3 && GLOBALS[t[1]]) {
    const patch = GLOBALS[t[1]](t[2]);
    if (patch) return { control: patch, msg: `all ${t[1]} ${t[2]}` };
  }
  if (GLOBALS[t[0]] && t.length >= 2) {
    const patch = GLOBALS[t[0]](t[1]);
    if (patch) return { control: patch, msg: `${t[0]} ${t[1]}` };
  }

  // targeted overrides
  const withIdx = fixtures.map((f, i) => ({ f, i }));
  let idx: number[] | null = null;
  let ti = 0;
  let label = "";
  if (t[0] === "all") {
    idx = withIdx.map((x) => x.i);
    ti = 1;
    label = "all";
  } else if (t[0] === "zone" && t[1]) {
    idx = withIdx.filter((x) => x.f.zone === t[1]).map((x) => x.i);
    ti = 2;
    label = `zone ${t[1]}`;
  } else if (t[0] === "range" && t[1]) {
    const m = t[1].match(/^(\d+)-(\d+)$/);
    if (m) {
      const a = +m[1];
      const b = +m[2];
      idx = withIdx.filter((x) => x.f.seq >= a && x.f.seq <= b).map((x) => x.i);
      ti = 2;
      label = `range ${t[1]}`;
    }
  } else if (t[0] === "every" && t[1]) {
    const nn = parseInt(t[1], 10);
    if (nn >= 1) {
      idx = withIdx.filter((x) => x.f.seq % nn === 0).map((x) => x.i);
      ti = 2;
      label = `every ${nn}`;
    }
  } else if (t[0] === "fixture" && t[1]) {
    idx = withIdx.filter((x) => x.f.id.toLowerCase() === t[1] || String(x.f.seq) === t[1]).map((x) => x.i);
    ti = 2;
    label = `fixture ${t[1]}`;
  } else if ((t[0] === "light" || t[0] === "lights" || t[0] === "num") && t[1]) {
    // address by ADDRESSABLE NUMBER 1..72 — supports lists + ranges:
    //   "light 7" · "light 1,7,17" · "light 1-24" · "light 1,7,17-20"
    const wanted = new Set<number>();
    for (const part of t[1].split(",")) {
      const r = part.match(/^(\d+)-(\d+)$/);
      if (r) { for (let k = +r[1]; k <= +r[2]; k++) wanted.add(k); }
      else if (/^\d+$/.test(part)) wanted.add(+part);
    }
    idx = withIdx.filter((x) => wanted.has(x.f.num)).map((x) => x.i);
    ti = 2;
    label = `light ${t[1]}`;
  }

  if (idx) {
    const act = t[ti];
    if (act === "off") return { setOverrides: { idx, op: { mode: "off" } }, msg: `${label} off (${idx.length})` };
    if (act === "on")
      return { setOverrides: { idx, op: { mode: "color", rgb: [1, 1, 1] } }, msg: `${label} on (${idx.length})` };
    if (act === "color" && t[ti + 1]) {
      const rgb = parseColor(t[ti + 1]);
      if (rgb) return { setOverrides: { idx, op: { mode: "color", rgb } }, msg: `${label} ${t[ti + 1]} (${idx.length})` };
    }
  }
  return { msg: `? unrecognized — try: light 1,7,17 color blue · range 0-23 color #00aaff · zone high off · clear` };
}

/** Split a multi-line command script (the LLM's output) into runnable commands.
 *  Trims, drops blank lines and `#` comments. */
export function parseScript(text: string): string[] {
  return text
    .split(/\r?\n/)
    .map((l) => l.trim())
    .filter((l) => l.length > 0 && !l.startsWith("#"));
}
