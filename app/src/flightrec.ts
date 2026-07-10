/** FLIGHT RECORDER — the interactive-mode black box (doc 18C).
 *
 *  Elliot: "how is it going to show what the tree is doing and record it for
 *  bug logging and tracking when it is in interactive mode?"
 *
 *  Aviation pattern: record ALWAYS into a bounded ring, persist ON INCIDENT.
 *  A rolling ~30 min of (a) every INPUT — taps, walk footfalls, presence
 *  pings, mode/rule/theme/speed changes, arm/disarm — (b) 2 Hz OUTPUT
 *  keyframes (per-light brightness nibble + hue byte), (c) engine MARKS
 *  (generation ticks, watchdog reseeds). The 🐞 flag freezes the recent
 *  window into a downloadable JSON the twin can replay: deterministic-enough
 *  CA + recorded inputs ⇒ an honest repro instead of "it looked wrong".
 *
 *  Module-level (not zustand) — written from hot paths at frame rate; must
 *  never trigger React renders. Memory budget: EVENTS ≤ 4k entries,
 *  KEYFRAMES ≤ 3600 (30 min @ 2 Hz), ~350 B each ≈ 1.3 MB. */

export interface RecEvent {
  t: number; // ms since session start
  kind: "trigger" | "presence" | "mode" | "rule" | "theme" | "speed" | "arm" | "show" | "mark";
  detail: Record<string, unknown>;
}
export interface RecKeyframe {
  t: number;
  // packed per-light: high nibble = brightness 0..15, low byte in `hue` = hue 0..255
  bri: string; // base64 of Uint8Array(n) nibbles packed 2-per-byte
  hue: string; // base64 of Uint8Array(n)
}
export interface FlightLog {
  version: 1;
  startedAt: string; // ISO wall clock of session start
  flaggedAt: string;
  note: string;
  windowMs: number;
  fixtures: number;
  events: RecEvent[];
  keyframes: RecKeyframe[];
}

const MAX_EVENTS = 4096;
const MAX_KEYFRAMES = 3600; // 30 min @ 2 Hz
const KEYFRAME_MS = 500;

const t0 = () => performance.now();
let sessionStart = t0();
let sessionISO = new Date().toISOString();
let events: RecEvent[] = [];
let keyframes: RecKeyframe[] = [];
let lastKf = -1e9;

export function recReset() {
  sessionStart = t0();
  sessionISO = new Date().toISOString();
  events = [];
  keyframes = [];
  lastKf = -1e9;
}

export function recEvent(kind: RecEvent["kind"], detail: Record<string, unknown> = {}) {
  events.push({ t: Math.round(t0() - sessionStart), kind, detail });
  if (events.length > MAX_EVENTS) events.splice(0, events.length - MAX_EVENTS);
}

const b64 = (u: Uint8Array) => {
  let s = "";
  for (let i = 0; i < u.length; i++) s += String.fromCharCode(u[i]);
  return btoa(s);
};
const unb64 = (s: string) => {
  const raw = atob(s);
  const u = new Uint8Array(raw.length);
  for (let i = 0; i < raw.length; i++) u[i] = raw.charCodeAt(i);
  return u;
};

/** Call from the render loop (any rate) — self-throttles to 2 Hz. */
export function recKeyframe(bri: Float32Array | number[], hue: Float32Array | number[]) {
  const now = t0();
  if (now - lastKf < KEYFRAME_MS) return;
  lastKf = now;
  const n = bri.length;
  const bn = new Uint8Array(Math.ceil(n / 2));
  for (let i = 0; i < n; i++) {
    const q = Math.max(0, Math.min(15, Math.round((bri[i] as number) * 15)));
    if (i % 2 === 0) bn[i >> 1] = q << 4;
    else bn[i >> 1] |= q;
  }
  const hn = new Uint8Array(n);
  for (let i = 0; i < n; i++) hn[i] = Math.max(0, Math.min(255, Math.round(((hue[i] as number) % 1 + 1) % 1 * 255)));
  keyframes.push({ t: Math.round(now - sessionStart), bri: b64(bn), hue: b64(hn) });
  if (keyframes.length > MAX_KEYFRAMES) keyframes.splice(0, keyframes.length - MAX_KEYFRAMES);
}

/** Decode a keyframe back to per-light arrays (replay + tests). */
export function decodeKeyframe(kf: RecKeyframe, n: number): { bri: Float32Array; hue: Float32Array } {
  const bn = unb64(kf.bri), hn = unb64(kf.hue);
  const bri = new Float32Array(n), hue = new Float32Array(n);
  for (let i = 0; i < n; i++) {
    const q = i % 2 === 0 ? bn[i >> 1] >> 4 : bn[i >> 1] & 0xf;
    bri[i] = q / 15;
    hue[i] = (hn[i] ?? 0) / 255;
  }
  return { bri, hue };
}

/** Freeze the last `windowMs` into a downloadable bug log. */
export function flagBug(note: string, fixtures: number, windowMs = 120_000): FlightLog {
  const cut = t0() - sessionStart - windowMs;
  return {
    version: 1,
    startedAt: sessionISO,
    flaggedAt: new Date().toISOString(),
    note,
    windowMs,
    fixtures,
    events: events.filter((e) => e.t >= cut),
    keyframes: keyframes.filter((k) => k.t >= cut),
  };
}

/** Human summary for the session card (shown on disarm / in the panel). */
export function recSummary(): { events: number; keyframes: number; byKind: Record<string, number>; minutes: number } {
  const byKind: Record<string, number> = {};
  for (const e of events) byKind[e.kind] = (byKind[e.kind] ?? 0) + 1;
  return {
    events: events.length,
    keyframes: keyframes.length,
    byKind,
    minutes: Math.round((t0() - sessionStart) / 6000) / 10,
  };
}
