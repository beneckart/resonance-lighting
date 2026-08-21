/** FLEET RULES — behavior scripts the fixtures run THEMSELVES.
 *
 *  Elliot's ask (2026-07-05): write scripts/rules and give them to the fleet —
 *  "flash the updates on how we want them to behave in different conditions
 *  and modes."
 *
 *  The design honors the two hard constraints of Ben's architecture:
 *   1. RULES ARE DATA, NOT FIRMWARE (ADR 0010): pushing behavior is a small
 *      control-plane broadcast + flash write on the node — not an OTA. OTA
 *      stays for code; rules change nightly without touching code.
 *   2. RULES RUN LOCALLY: a fixture evaluates its rule table against its OWN
 *      inputs (clock, battery, its ToF presence, sound) every tick — the
 *      radio is NOT needed to behave, only to change the behavior. A tree
 *      whose cortex dies keeps following the last rules it was given.
 *
 *  Size discipline: a compiled rule set must fit ONE ESP-NOW frame (250 B) so
 *  "flash the fleet" is literally a single broadcast (+ per-node ack). The
 *  compiler enforces MAX_BYTES.
 *
 *  The text DSL (one rule per line, first match wins, last line = default):
 *      when hour >= 22 and soc < 30 -> pattern=ember bri=40
 *      when presence > 0            -> pattern=ripple bri=255 speed=3
 *      ->  pattern=breathe bri=120
 */

// ── vocabulary (shared enum with firmware — append-only, never renumber) ─────

export const SENSORS = {
  hour: 0, // local clock 0..23 (fleet epoch-synced)
  soc: 1, // this node's OWN battery %, 0..100
  presence: 2, // this node's ToF presence level 0..10
  sound: 3, // sound level 0..10 (mic/aux where fitted)
  supply: 4, // charger says supply valid (0/1) — daylight proxy
  mode: 5, // global mode byte last broadcast
} as const;
export type SensorName = keyof typeof SENSORS;

export const OPS = { "<": 0, ">": 1, "=": 2, "<=": 3, ">=": 4 } as const;
export type OpName = keyof typeof OPS;

/** Pattern ids the fixtures know (append-only; mirrors firmware pattern/). */
export const PATTERNS = {
  off: 0, static: 1, breathe: 2, ember: 3, ripple: 4,
  sparkle: 5, rainbow: 6, chase: 7, beacon: 8,
} as const;
export type PatternName = keyof typeof PATTERNS;

export interface Condition { sensor: SensorName; op: OpName; value: number }
export interface Action { pattern: PatternName; bri: number; hue?: number; speed?: number }
export interface Rule { when: Condition[]; then: Action } // when=[] → always true (default)
export interface RuleSet { version: 1; epoch: number; rules: Rule[] }

export const MAX_BYTES = 240; // one ESP-NOW frame with header room
export const MAX_RULES = 16;

// ── parser (the operator-facing DSL) ─────────────────────────────────────────

export interface ParseResult { ok: boolean; ruleset?: RuleSet; errors: string[] }

export function parseRules(text: string, epoch = 1): ParseResult {
  const errors: string[] = [];
  const rules: Rule[] = [];
  const lines = text.split("\n");
  lines.forEach((raw, li) => {
    const line = raw.replace(/#.*$/, "").trim();
    if (!line) return;
    const n = li + 1;
    const arrow = line.indexOf("->");
    if (arrow < 0) { errors.push(`line ${n}: missing "->"`); return; }
    const head = line.slice(0, arrow).trim();
    const tail = line.slice(arrow + 2).trim();
    // conditions
    const when: Condition[] = [];
    if (head) {
      if (!head.startsWith("when")) { errors.push(`line ${n}: rules start with "when" (or bare "->" for the default)`); return; }
      const conds = head.slice(4).split(/\band\b/);
      for (const c of conds) {
        const m = c.trim().match(/^(\w+)\s*(<=|>=|<|>|=)\s*(-?\d+)$/);
        if (!m) { errors.push(`line ${n}: can't read condition "${c.trim()}"`); return; }
        const [, sensor, op, value] = m;
        if (!(sensor in SENSORS)) { errors.push(`line ${n}: unknown sensor "${sensor}" (know: ${Object.keys(SENSORS).join(", ")})`); return; }
        when.push({ sensor: sensor as SensorName, op: op as OpName, value: parseInt(value, 10) });
      }
    }
    // action
    const action: Partial<Action> = {};
    for (const kv of tail.split(/\s+/)) {
      if (!kv) continue;
      const m = kv.match(/^(\w+)=([\w-]+)$/);
      if (!m) { errors.push(`line ${n}: can't read "${kv}" (want key=value)`); return; }
      const [, k, v] = m;
      if (k === "pattern") {
        if (!(v in PATTERNS)) { errors.push(`line ${n}: unknown pattern "${v}" (know: ${Object.keys(PATTERNS).join(", ")})`); return; }
        action.pattern = v as PatternName;
      } else if (k === "bri" || k === "hue" || k === "speed") {
        const num = parseInt(v, 10);
        if (Number.isNaN(num) || num < 0 || num > 255) { errors.push(`line ${n}: ${k} must be 0..255`); return; }
        action[k] = num;
      } else {
        errors.push(`line ${n}: unknown key "${k}" (know: pattern, bri, hue, speed)`);
        return;
      }
    }
    if (action.pattern === undefined) { errors.push(`line ${n}: action needs pattern=…`); return; }
    if (action.bri === undefined) action.bri = 255;
    if (action.hue === undefined) action.hue = 0;
    if (action.speed === undefined) action.speed = 1;
    rules.push({ when, then: action as Action });
  });
  if (!rules.length) errors.push("no rules — write at least a default line: -> pattern=breathe bri=120");
  if (rules.length > MAX_RULES) errors.push(`${rules.length} rules — max ${MAX_RULES}`);
  if (!errors.length) {
    const bytes = compileRules({ version: 1, epoch, rules }).length;
    if (bytes > MAX_BYTES) errors.push(`compiled to ${bytes} B — must fit one ESP-NOW frame (${MAX_BYTES} B). Fewer/simpler rules.`);
  }
  return errors.length ? { ok: false, errors } : { ok: true, ruleset: { version: 1, epoch, rules }, errors: [] };
}

// ── compiler (the wire bytes a node stores in flash) ─────────────────────────
// header: ver u8 · epoch u16le · count u8
// rule:   condCount u8 · [sensor u8 · op u8 · value i16le]* · pattern u8 · bri u8 · hue u8 · speed u8

export function compileRules(rs: RuleSet): Uint8Array {
  const out: number[] = [rs.version, rs.epoch & 0xff, (rs.epoch >> 8) & 0xff, rs.rules.length];
  for (const r of rs.rules) {
    out.push(r.when.length);
    for (const c of r.when) {
      out.push(SENSORS[c.sensor], OPS[c.op], c.value & 0xff, (c.value >> 8) & 0xff);
    }
    out.push(PATTERNS[r.then.pattern], r.then.bri, r.then.hue ?? 0, r.then.speed ?? 1);
  }
  return new Uint8Array(out);
}

export function decodeRules(bytes: Uint8Array): RuleSet {
  const sensorNames = Object.keys(SENSORS) as SensorName[];
  const opNames = Object.keys(OPS) as OpName[];
  const patternNames = Object.keys(PATTERNS) as PatternName[];
  let i = 0;
  const version = bytes[i++] as 1;
  const epoch = bytes[i++] | (bytes[i++] << 8);
  const count = bytes[i++];
  const rules: Rule[] = [];
  for (let r = 0; r < count; r++) {
    const nc = bytes[i++];
    const when: Condition[] = [];
    for (let c = 0; c < nc; c++) {
      const sensor = sensorNames[bytes[i++]];
      const op = opNames[bytes[i++]];
      let value = bytes[i++] | (bytes[i++] << 8);
      if (value > 0x7fff) value -= 0x10000;
      when.push({ sensor, op, value });
    }
    const pattern = patternNames[bytes[i++]];
    const bri = bytes[i++];
    const hue = bytes[i++];
    const speed = bytes[i++];
    rules.push({ when, then: { pattern, bri, hue, speed } });
  }
  return { version, epoch, rules };
}

// ── evaluator (EXACTLY what firmware runs each tick — first match wins) ──────

export type SensorInputs = Record<SensorName, number>;

function holds(c: Condition, inputs: SensorInputs): boolean {
  const v = inputs[c.sensor];
  switch (c.op) {
    case "<": return v < c.value;
    case ">": return v > c.value;
    case "=": return v === c.value;
    case "<=": return v <= c.value;
    case ">=": return v >= c.value;
  }
}

/** returns the matched rule index + action (null if nothing matches and no default) */
export function evalRules(rs: RuleSet, inputs: SensorInputs): { index: number; action: Action } | null {
  for (let i = 0; i < rs.rules.length; i++) {
    if (rs.rules[i].when.every((c) => holds(c, inputs))) return { index: i, action: rs.rules[i].then };
  }
  return null;
}

// ── presets (starting points for the operator) ───────────────────────────────

export const RULE_PRESETS: Record<string, string> = {
  "night-saver": [
    "# conserve late at night unless people are there",
    "when hour >= 1 and hour < 6 and presence = 0 -> pattern=ember bri=30",
    "when soc < 20 -> pattern=ember bri=25            # protect the battery",
    "when presence > 0 -> pattern=ripple bri=255 speed=3",
    "-> pattern=breathe bri=140",
  ].join("\n"),
  "party": [
    "when sound > 6 -> pattern=chase bri=255 speed=5",
    "when sound > 2 -> pattern=sparkle bri=220 speed=3",
    "when soc < 15 -> pattern=ember bri=40             # even parties respect the cell",
    "-> pattern=rainbow bri=200 speed=2",
  ].join("\n"),
  "storm-safe": [
    "# daylight/no-charge day: minimum draw, visible status",
    "when supply = 1 -> pattern=off bri=0              # daylight — sleep the LEDs",
    "when soc < 40 -> pattern=ember bri=20",
    "-> pattern=breathe bri=80",
  ].join("\n"),
};
