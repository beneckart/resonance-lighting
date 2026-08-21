import { PATTERN_IDS, ELEMENT_MODES, type PatternId } from "./store";

/** LLM operator (PRD #31): natural language → command-console commands.
 *  The command console IS the LLM's tool surface (charter) — an external LLM
 *  would emit these same grammar strings. This is a deterministic, offline,
 *  testable interpreter that maps everyday phrasing onto the command.ts grammar
 *  so the NL→light path works without a network call (and is the contract the
 *  AI-VJ / smart-sound mode drives). */

export interface Interpretation {
  commands: string[]; // command-console lines, run via runScript
  note: string;
}

// CSS-valid colour names only (three.Color must parse them in command.ts)
const COLORS = [
  "red", "green", "blue", "white", "cyan", "magenta", "yellow", "orange",
  "purple", "pink", "violet", "gold", "teal", "turquoise", "crimson", "indigo",
];

const ALL_PATTERNS = [...PATTERN_IDS, ...ELEMENT_MODES] as string[];

// soft synonyms → a real pattern id
const PATTERN_SYNONYMS: [RegExp, PatternId][] = [
  [/\brainbow\b/, "spectrum"],
  [/\btwinkle|stars?|glitter\b/, "sparkle"],
  [/\bcomet|chase|run(ning)?\b/, "chase"],
  [/\bpulse|pulsing|heartbeat\b/, "breathe"],
  [/\bwave|ripple|water\b/, "ripple"],
  [/\bshaft|god ?ray|beam(s)?\b/, "godray"],
  [/\bspiral|barber\b/, "spiral"],
  [/\bfire|flame|ember|lava\b/, "ember"],
  [/\brain|drizzle|storm\b/, "rain"],
  [/\bbreeze|wind|sway\b/, "wind"],
  [/\brise|rising|climb|sap\b/, "rising"],
  [/\bsweep|wipe|plane\b/, "planewipe"],
  [/\bwarm.?cool|depth\b/, "warmcool"],
  [/\bthree colou?r|tri.?colou?r|triad\b/, "tricolor"],
];

const has = (s: string, re: RegExp) => re.test(s);

/** Resolve the addressing target from natural language → command grammar token. */
export function targetFor(s: string): string {
  if (has(s, /\b(top|canopy|crown|upper|high|tips?)\b/)) return "zone high";
  if (has(s, /\b(bottom|trunk|base|lower|low|roots?|ground)\b/)) return "zone low";
  if (has(s, /\b(middle|mid|center|centre|waist)\b/)) return "zone mid";
  if (has(s, /\bevery other\b/)) return "every 2";
  return "all"; // whole tree / everything / unspecified
}

export function interpret(nl: string): Interpretation {
  const s = ` ${nl.toLowerCase().trim()} `;
  const cmds: string[] = [];

  // pattern (explicit id wins, else a synonym)
  let pat: string | null = ALL_PATTERNS.find((p) => has(s, new RegExp(`\\b${p}\\b`))) ?? null;
  if (!pat) for (const [re, id] of PATTERN_SYNONYMS) if (has(s, re)) { pat = id; break; }
  if (pat) cmds.push(`pattern ${pat}`);

  const tgt = targetFor(s);

  // off / blackout (the target goes dark)
  if (has(s, /\b(turn off|blackout|black out|go dark|kill|lights? off|darkness)\b/)) {
    cmds.push(`${tgt} off`);
  }

  // colour
  const color = COLORS.find((c) => has(s, new RegExp(`\\b${c}\\b`)));
  if (color) cmds.push(`${tgt} color ${color}`);

  // speed
  if (has(s, /\b(fast|faster|quick|rapid|energetic|hype|frantic)\b/)) cmds.push("speed 2.4");
  else if (has(s, /\b(slow|slower|gentle|calm|chill|mellow|relax)\b/)) cmds.push("speed 0.5");

  // brightness
  if (has(s, /\b(bright|brighter|full|blast|max|intense)\b/)) cmds.push("bri 1");
  else if (has(s, /\b(dim|dimmer|softer|faint|subtle)\b/)) cmds.push("bri 0.4");

  // saturation
  if (has(s, /\b(vivid|saturated|vibrant|punchy|deep)\b/)) cmds.push("sat 1");
  else if (has(s, /\b(pastel|washed|pale|desaturated|muted)\b/)) cmds.push("sat 0.45");

  const note = cmds.length
    ? `interpreted → ${cmds.join(" · ")}`
    : `no actionable lighting intent found`;
  return { commands: cmds, note };
}
