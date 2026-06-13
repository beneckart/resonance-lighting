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
