import type { Control } from "./store";

/** Map a MIDI CC (Akai APC-class faders) to a control change. value 0..127. */
export function ccToControl(cc: number, value: number): Partial<Control> | null {
  const v = Math.min(1, Math.max(0, value / 127));
  switch (cc) {
    case 7: return { brightness: v }; // common volume CC
    case 1: return { hue: v }; // mod wheel
    case 2: return { sat: v };
    case 3: return { speed: v * 3 };
    case 4: return { master: v };
    case 5: return { xfade: v };
    default: return null;
  }
}

/* eslint-disable @typescript-eslint/no-explicit-any */
/** Connect to WebMIDI (guarded — unavailable in some browsers/headless). Returns a
 *  status string; routes CC → onCC, note-on → onNote. */
export async function startMidi(
  onCC: (cc: number, value: number) => void,
  onNote: (note: number) => void
): Promise<string> {
  const nav = navigator as any;
  if (typeof nav.requestMIDIAccess !== "function") return "WebMIDI unavailable";
  try {
    const access = await nav.requestMIDIAccess();
    let count = 0;
    access.inputs.forEach((input: any) => {
      count++;
      input.onmidimessage = (e: any) => {
        const [status, d1, d2] = e.data;
        const cmd = status & 0xf0;
        if (cmd === 0xb0) onCC(d1, d2);
        else if (cmd === 0x90 && d2 > 0) onNote(d1);
      };
    });
    return count > 0 ? `MIDI: ${count} input(s)` : "MIDI: no inputs";
  } catch (err) {
    return `MIDI error: ${String(err)}`;
  }
}
