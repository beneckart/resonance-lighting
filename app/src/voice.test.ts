/** voice.ts — the mic in front of the LLM operator. Tested with an injected
 *  fake SpeechRecognition: partials stream, final fires once, errors surface,
 *  and the unsupported browser path degrades loudly (both answers proven). */
import { describe, expect, it } from "vitest";
import { listenOnce } from "./voice";

type ResultEvent = { results: ArrayLike<{ 0: { transcript: string }; isFinal: boolean }> };

class FakeRec {
  static last: FakeRec | null = null;
  lang = ""; continuous = true; interimResults = false;
  started = 0; stopped = 0;
  onresult: ((ev: ResultEvent) => void) | null = null;
  onerror: ((ev: { error?: string }) => void) | null = null;
  onend: (() => void) | null = null;
  constructor() { FakeRec.last = this; }
  start() { this.started += 1; }
  stop() { this.stopped += 1; this.onend?.(); }
  abort() { /* noop */ }
  // test drivers
  emit(parts: { text: string; final: boolean }[]) {
    this.onresult?.({ results: parts.map((p) => ({ 0: { transcript: p.text }, isFinal: p.final })) });
  }
}

describe("listenOnce", () => {
  it("streams partials, fires final exactly once, configures one-utterance mode", () => {
    const partials: string[] = []; const finals: string[] = [];
    const h = listenOnce({
      onPartial: (t) => partials.push(t),
      onFinal: (t) => finals.push(t),
      make: FakeRec as never,
    });
    const rec = FakeRec.last!;
    expect(h).not.toBeNull();
    expect(rec.started).toBe(1);
    expect(rec.continuous).toBe(false);
    expect(rec.interimResults).toBe(true);
    rec.emit([{ text: "make everything ", final: false }]);
    rec.emit([{ text: "make everything red", final: false }]);
    rec.emit([{ text: "make everything red and slow", final: true }]);
    rec.emit([{ text: "make everything red and slow", final: true }]); // dup final: ignored
    expect(partials).toEqual(["make everything", "make everything red"]);
    expect(finals).toEqual(["make everything red and slow"]);
  });

  it("surfaces recognition errors", () => {
    const errs: string[] = [];
    listenOnce({ onFinal: () => {}, onError: (e) => errs.push(e), make: FakeRec as never });
    FakeRec.last!.onerror?.({ error: "not-allowed" });
    expect(errs).toEqual(["not-allowed"]);
  });

  it("unsupported browser: returns null AND reports — silence is not a fallback", () => {
    const errs: string[] = [];
    // no `make`, and jsdom has no SpeechRecognition → the unsupported path
    const h = listenOnce({ onFinal: () => {}, onError: (e) => errs.push(e) });
    expect(h).toBeNull();
    expect(errs[0]).toMatch(/unavailable/);
  });
});
