import { buildShowFrame, type ShowFrame } from "./protocol";
import type { Control, SimFixture } from "./store";
import type { Cue } from "./cues";

/** A compiled show: a deterministic per-fixture keyframe timeline (Protocol-v1
 *  frames, params-not-pixels) that the cortex can replay WITHOUT the browser.
 *  PRD #29 / F2 — the "Show Compiler" handoff to the mesh. */
export interface ShowKeyframe {
  tMs: number; // start time on the timeline
  name: string; // source cue name
  frame: ShowFrame; // the param frame to broadcast at tMs
}

export interface ShowDoc {
  meta: {
    schema: "resonance.show/0.1";
    channel: number;
    stepMs: number;
    durationMs: number;
    fixtures: number;
    keyframes: number;
  };
  keyframes: ShowKeyframe[];
}

/** Compile a cue list into a keyframe timeline. Each cue → one keyframe spaced
 *  stepMs apart; each keyframe is a Protocol-v1 ShowFrame built from the base
 *  control merged with the cue's stored control. Epoch increments per keyframe
 *  so the mesh can detect ordering. Pure + deterministic. */
export function compileShow(
  cues: Cue[],
  fixtures: SimFixture[],
  baseControl: Control,
  channel: number,
  stepMs: number
): ShowDoc {
  const keyframes: ShowKeyframe[] = cues.map((c, i) => ({
    tMs: i * stepMs,
    name: c.name,
    frame: buildShowFrame({ ...baseControl, ...c.control }, fixtures, {}, channel, i),
  }));
  return {
    meta: {
      schema: "resonance.show/0.1",
      channel,
      stepMs,
      durationMs: keyframes.length * stepMs,
      fixtures: fixtures.length,
      keyframes: keyframes.length,
    },
    keyframes,
  };
}

/** Serialize a ShowDoc to pretty JSON (the file the cortex ingests). */
export function showToJson(doc: ShowDoc): string {
  return JSON.stringify(doc, null, 2);
}

export interface ShowValidation {
  ok: boolean;
  errors: string[];
}

/** Validate a (re-imported) ShowDoc before the cortex replays it — the ingest
 *  gate that closes the compile→export→import round-trip. Pure. */
export function validateShowDoc(doc: unknown): ShowValidation {
  const errors: string[] = [];
  const d = doc as Partial<ShowDoc> | null;
  if (!d || typeof d !== "object") return { ok: false, errors: ["doc is not an object"] };
  if (!d.meta || typeof d.meta !== "object") errors.push("missing meta");
  else {
    if (d.meta.schema !== "resonance.show/0.1") errors.push(`unexpected schema ${d.meta.schema}`);
    if (typeof d.meta.channel !== "number") errors.push("meta.channel not a number");
    if (typeof d.meta.stepMs !== "number") errors.push("meta.stepMs not a number");
  }
  if (!Array.isArray(d.keyframes)) {
    errors.push("keyframes not an array");
  } else {
    d.keyframes.forEach((kf, i) => {
      if (typeof kf?.tMs !== "number") errors.push(`keyframes[${i}].tMs not a number`);
      const fr = kf?.frame;
      if (!fr || fr.proto !== 1) errors.push(`keyframes[${i}].frame.proto must be 1`);
      else if (!Array.isArray(fr.fixtures)) errors.push(`keyframes[${i}].frame.fixtures not an array`);
    });
    // tMs should be monotonically non-decreasing (replay order)
    for (let i = 1; i < d.keyframes.length; i++) {
      if (d.keyframes[i]?.tMs < d.keyframes[i - 1]?.tMs) {
        errors.push(`keyframes[${i}].tMs goes backwards`);
        break;
      }
    }
  }
  return { ok: errors.length === 0, errors };
}
