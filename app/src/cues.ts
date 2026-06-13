import type { Control } from "./store";

export interface Cue {
  id: string;
  name: string;
  control: Control; // a full snapshot of the look
}

let counter = 0;

/** Capture the current control state as a named cue. */
export function makeCue(name: string, control: Control, seed = Date.now()): Cue {
  const n = ++counter;
  return { id: `cue-${seed}-${n}`, name: name.trim() || `cue ${n}`, control: { ...control } };
}

const KEY = "resonance.cues";

export function loadCues(): Cue[] {
  try {
    const raw = typeof localStorage !== "undefined" ? localStorage.getItem(KEY) : null;
    return raw ? (JSON.parse(raw) as Cue[]) : [];
  } catch {
    return [];
  }
}

export function saveCues(cues: Cue[]): void {
  try {
    if (typeof localStorage !== "undefined") localStorage.setItem(KEY, JSON.stringify(cues));
  } catch {
    /* ignore quota / unavailable */
  }
}
