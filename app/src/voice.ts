/** VOICE INPUT — the microphone in front of the LLM operator (Elliot 08-14:
 *  "describe what we want and the system transcribes and makes those items
 *  happen"). Wraps the browser's SpeechRecognition; the transcript feeds the
 *  SAME deterministic interpret() → runScript path the typed operator uses,
 *  so voice works fully offline once the words are text. iOS Safari 14.5+ and
 *  Chrome expose webkitSpeechRecognition; where absent, callers fall back to
 *  the typed input (voiceSupported()). */

type RecognitionLike = {
  lang: string;
  continuous: boolean;
  interimResults: boolean;
  start(): void;
  stop(): void;
  abort(): void;
  onresult: ((ev: { results: ArrayLike<{ 0: { transcript: string }; isFinal: boolean }> }) => void) | null;
  onerror: ((ev: { error?: string }) => void) | null;
  onend: (() => void) | null;
};

type RecognitionCtor = new () => RecognitionLike;

function ctor(): RecognitionCtor | null {
  if (typeof window === "undefined") return null;
  const w = window as unknown as { SpeechRecognition?: RecognitionCtor; webkitSpeechRecognition?: RecognitionCtor };
  return w.SpeechRecognition ?? w.webkitSpeechRecognition ?? null;
}

export function voiceSupported(): boolean {
  return ctor() !== null;
}

export interface ListenHandle { stop(): void }

/** One utterance: partials stream to onPartial, the final transcript to
 *  onFinal (once), errors to onError. Auto-ends on silence. */
export function listenOnce(cb: {
  onPartial?: (text: string) => void;
  onFinal: (text: string) => void;
  onError?: (err: string) => void;
  onEnd?: () => void;
  make?: RecognitionCtor; // injectable for tests
}): ListenHandle | null {
  const C = cb.make ?? ctor();
  if (!C) { cb.onError?.("speech recognition unavailable on this browser"); return null; }
  const rec = new C();
  rec.lang = "en-US";
  rec.continuous = false;
  rec.interimResults = true;
  let finalSent = false;
  rec.onresult = (ev) => {
    let text = "";
    let isFinal = false;
    for (let i = 0; i < ev.results.length; i++) {
      text += ev.results[i][0].transcript;
      if (ev.results[i].isFinal) isFinal = true;
    }
    if (isFinal && !finalSent) { finalSent = true; cb.onFinal(text.trim()); }
    else cb.onPartial?.(text.trim());
  };
  rec.onerror = (ev) => cb.onError?.(ev.error ?? "speech error");
  rec.onend = () => cb.onEnd?.();
  try { rec.start(); } catch (e) { cb.onError?.(String(e)); return null; }
  return { stop: () => { try { rec.stop(); } catch { /* already stopped */ } } };
}
