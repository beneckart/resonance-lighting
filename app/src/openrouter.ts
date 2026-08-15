import { PATTERN_IDS, ELEMENT_MODES, useTwin } from "./store";
import { SHOWS } from "./shows";
import { THEMES } from "./themes";
import { isCommandLike, parseScript } from "./command";
import { interpret, type Interpretation } from "./llm";

/** OPENROUTER OPERATOR — the optional remote brain behind the SAME seam the
 *  offline interpreter already occupies (Elliot 2026-08-15: "an AI Claude
 *  through OpenRouter ... run the controller through voice without a code
 *  change").
 *
 *  The charter line in llm.ts always anticipated this: "the command console IS
 *  the LLM's tool surface — an external LLM would emit these same grammar
 *  strings." So this file adds a second interpreter, not a second control path.
 *  Natural language in, `command.ts` grammar lines out, `runScript` runs them.
 *  Nothing downstream knows or cares which interpreter produced the lines.
 *
 *  THREE PROPERTIES THIS FILE MUST NEVER GIVE UP:
 *
 *  1. NO CODE, ONLY VOCABULARY. The model returns command-grammar lines, which
 *     are shape-gated by `isCommandLike` and then parsed by the same
 *     fail-safe `runCommandStr` a human's typing goes through. The model cannot
 *     reach the tree with anything the controller could not already say.
 *  2. THE KEY NEVER ENTERS THE REPO. It lives in localStorage on the operator's
 *     own device, pasted into the app. It is never committed, never logged, and
 *     never put in an error message (see `redact`).
 *  3. OFFLINE ALWAYS WINS OVER BROKEN. Playa network is Starlink and it drops.
 *     Every failure — no key, no network, rate limit, junk output, zero usable
 *     lines — falls back to the deterministic `interpret()` rather than
 *     surfacing an error. A voice command must never die because the sky did. */

const KEY_STORAGE = "resonance.openrouter.key";
const MODEL_STORAGE = "resonance.openrouter.model";
export const DEFAULT_MODEL = "anthropic/claude-sonnet-4.5";
export const ENDPOINT = "https://openrouter.ai/api/v1/chat/completions";

/** localStorage is absent in tests/SSR — every accessor tolerates that. */
function store(): Storage | null {
  try {
    return typeof localStorage === "undefined" ? null : localStorage;
  } catch {
    return null; // Safari private mode throws on access
  }
}

export function loadKey(): string | null {
  return store()?.getItem(KEY_STORAGE) ?? null;
}

export function saveKey(key: string): void {
  const s = store();
  if (!s) return;
  const t = key.trim();
  if (t) s.setItem(KEY_STORAGE, t);
  else s.removeItem(KEY_STORAGE);
}

export function clearKey(): void {
  store()?.removeItem(KEY_STORAGE);
}

export function loadModel(): string {
  return store()?.getItem(MODEL_STORAGE) || DEFAULT_MODEL;
}

export function saveModel(model: string): void {
  const s = store();
  if (!s) return;
  const t = model.trim();
  if (t) s.setItem(MODEL_STORAGE, t);
  else s.removeItem(MODEL_STORAGE);
}

export function remoteConfigured(): boolean {
  return !!loadKey();
}

/** A key must never reach a log line, a toast, or a thrown message. Any error
 *  text that transits this module goes through here first. */
export function redact(text: string): string {
  return text.replace(/sk-or-[A-Za-z0-9_\-]+/g, "sk-or-***");
}

/** The grammar, stated once, for the model. Pattern ids are injected from the
 *  live registries rather than retyped, so a pattern added to the app is a
 *  pattern the operator can ask for on the same commit. */
export function grammarPrompt(): string {
  const patterns = [...PATTERN_IDS, ...ELEMENT_MODES].join(", ");
  const shows = SHOWS.map((s) => `${s.id} (${s.vibe})`).join(" · ");
  const themes = THEMES.map((t) => t.id).join(", ");
  // the operator's saved custom modes, live from the store — the AI can recall
  // a mode Elliot saved five minutes ago without any code or prompt change
  let cueNames = "";
  try {
    cueNames = useTwin.getState().cues.map((c) => c.name).join(", ");
  } catch { /* store not initialised (tests) — fine, section stays empty */ }
  return [
    "You are the lighting operator AI for the Resonance Tree — a 10 m bamboo art",
    "installation of 130 solar LED lanterns at Burning Man (72 hanging downlights in",
    "3 rings, 24 perimeter, 18 chandelier crown, 16 uplights). You are talking to the",
    "digital twin; when real-drive is armed the physical tree mirrors it exactly.",
    "",
    "Translate the operator's request into lines of this command grammar. Output ONLY",
    "command lines, one per line. No prose, no explanation, no markdown, no code fences.",
    "",
    "LOOK & FEEL (globals):",
    "  pattern <id>      speed <0..3>      bri <0..1>      sat <0..1>      hue <0..1>",
    `  pattern ids: ${patterns}`,
    `  theme <id>   — colour mood for the living patterns; ids: ${themes}`,
    "",
    "TARGETED (specific lights):",
    "  <target> off | on | color <name|#hex>",
    "  target = all | zone <low|mid|high> | range <a-b> | every <n>",
    "         | fixture <id> | light <n | n,n,n | a-b>",
    "",
    "SHOWS (5-min authored arcs):",
    `  show <id> | show stop   — ids: ${shows}`,
    "",
    "GAME OF LIGHT (the interactive presence experience):",
    "  gol arm   — tree goes dark in standby; first visitor tap ignites it",
    "  gol end",
    "",
    "CUSTOM MODES (no code required — compose a look, then save it):",
    "  cue save <name>   — saves the CURRENT look as a named mode",
    "  cue <name>        — recalls it",
    cueNames ? `  saved modes right now: ${cueNames}` : "  (no saved modes yet)",
    "",
    "FLEET OPS (the real lanterns):",
    "  blink             — every real lantern blinks (roll call)",
    "  blink <mac>       — one lantern blinks, e.g. blink F2BE20 (locate a light)",
    "",
    "STANDALONE:  clear (release overrides)    off (blackout)    on",
    "",
    "SEQUENCING:",
    "  wait <seconds>    — pause a script; later lines run on a timer. A new",
    "                      request from the operator cancels any pending script",
    "                      instantly, so iterate freely.",
    "",
    "EXAMPLES",
    "  'all red, slowly become purple, then dark, then start the ripple game'",
    "    ->  all color red",
    "        wait 6",
    "        all color purple",
    "        wait 6",
    "        off",
    "        wait 2",
    "        clear",
    "        theme random",
    "        pattern ripples",
    "  'make it feel like a slow sunrise'   ->  pattern rising",
    "                                           speed 0.5",
    "                                           all color orange",
    "  'campfire mood but save it for later'->  pattern ember",
    "                                           theme ember",
    "                                           cue save campfire",
    "  'play the sun show'                  ->  show solarray",
    "  'which light is F2BE20?'             ->  blink F2BE20",
    "  'do a roll call'                     ->  blink",
    "  'set up the tap game'                ->  gol arm",
    "  'kill everything'                    ->  off",
    "",
    "Compose freely: a described look usually wants a pattern + theme/colour +",
    "speed + brightness together. If the request implies no lighting action at",
    "all, output nothing.",
  ].join("\n");
}

export interface RemoteOptions {
  apiKey?: string;
  model?: string;
  /** injectable for tests — defaults to global fetch */
  fetchImpl?: typeof fetch;
  /** abort the call after this many ms; the offline interpreter takes over */
  timeoutMs?: number;
}

export interface RemoteResult extends Interpretation {
  /** which interpreter actually produced `commands` — surfaced in the UI so the
   *  operator always knows whether the tree is being driven by the remote model
   *  or the offline fallback. Never guess this from configuration state. */
  source: "openrouter" | "offline";
  /** populated when the remote path was attempted and failed */
  error?: string;
}

function offline(nl: string, error?: string): RemoteResult {
  const local = interpret(nl);
  return { ...local, source: "offline", ...(error ? { error: redact(error) } : {}) };
}

/** Pull the assistant text out of an OpenRouter chat completion, tolerating the
 *  shape drift between providers rather than assuming one. */
function extractText(payload: unknown): string {
  const p = payload as { choices?: Array<{ message?: { content?: unknown } }> };
  const c = p?.choices?.[0]?.message?.content;
  if (typeof c === "string") return c;
  // some providers return content as an array of parts
  if (Array.isArray(c)) {
    return c
      .map((part) => (typeof part === "string" ? part : (part as { text?: string })?.text ?? ""))
      .join("");
  }
  return "";
}

/** Strip the markdown fence a model adds despite being told not to. Cheap, and
 *  it rescues an otherwise perfectly good answer. */
function stripFences(text: string): string {
  return text
    .replace(/^\s*```[a-zA-Z]*\s*/, "")
    .replace(/```\s*$/, "")
    .trim();
}

/** test seam — the fence stripper is internal, but it is the one piece of
 *  model-output handling worth pinning independently of a fake fetch. */
export const stripFencesForTest = stripFences;

/** Natural language → command lines, via OpenRouter, with the offline
 *  interpreter as the floor. This function does not throw. */
export async function interpretRemote(nl: string, opts: RemoteOptions = {}): Promise<RemoteResult> {
  const apiKey = opts.apiKey ?? loadKey();
  if (!apiKey) return offline(nl);

  const doFetch = opts.fetchImpl ?? (typeof fetch !== "undefined" ? fetch : null);
  if (!doFetch) return offline(nl, "no fetch available");

  const controller = typeof AbortController !== "undefined" ? new AbortController() : null;
  const timer =
    controller && opts.timeoutMs
      ? setTimeout(() => controller.abort(), opts.timeoutMs)
      : null;

  try {
    const res = await doFetch(ENDPOINT, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${apiKey}`,
        "Content-Type": "application/json",
        "HTTP-Referer": "https://resonanceart.github.io/resonance-lighting/",
        "X-Title": "Resonance Tree Lighting",
      },
      body: JSON.stringify({
        model: opts.model ?? loadModel(),
        temperature: 0,
        messages: [
          { role: "system", content: grammarPrompt() },
          { role: "user", content: nl },
        ],
      }),
      ...(controller ? { signal: controller.signal } : {}),
    });

    if (!res.ok) {
      // 401 bad key · 402 out of credit · 429 rate limit — all fall back
      return offline(nl, `openrouter http ${res.status}`);
    }

    const text = stripFences(extractText(await res.json()));
    // THE GATE: only lines that are vocabulary we own survive.
    const commands = parseScript(text).filter(isCommandLike);

    if (!commands.length) {
      // The model answered, but with nothing usable (prose, refusal, empty).
      // The offline interpreter may still find intent in the same sentence.
      return offline(nl, "model returned no usable commands");
    }

    return {
      commands,
      note: `openrouter → ${commands.join(" · ")}`,
      source: "openrouter",
    };
  } catch (e) {
    const msg = e instanceof Error ? e.message : String(e);
    return offline(nl, msg);
  } finally {
    if (timer) clearTimeout(timer);
  }
}
