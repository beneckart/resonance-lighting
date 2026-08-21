import { describe, it, expect } from "vitest";
import { parseMidi } from "./midiParse";

// a hand-built format-0 MIDI: 96 ticks/quarter, tempo 500000µs (0.5s/qtr),
// C4 for a quarter then E4 for a quarter
const BYTES = [
  0x4d, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x60, // MThd, format 0, 1 track, div 96
  0x4d, 0x54, 0x72, 0x6b, 0x00, 0x00, 0x00, 0x1b, // MTrk len 27
  0x00, 0xff, 0x51, 0x03, 0x07, 0xa1, 0x20, // tempo 500000
  0x00, 0x90, 0x3c, 0x64,                   // noteOn C4 (60) v100
  0x60, 0x80, 0x3c, 0x00,                   // +96 noteOff C4
  0x00, 0x90, 0x40, 0x64,                   // noteOn E4 (64)
  0x60, 0x80, 0x40, 0x00,                   // +96 noteOff E4
  0x00, 0xff, 0x2f, 0x00,                   // end of track
];

describe("parseMidi", () => {
  it("parses notes with correct pitch, time and duration", () => {
    const { notes, len } = parseMidi(new Uint8Array(BYTES).buffer);
    expect(notes.length).toBe(2);
    expect(notes[0].midi).toBe(60);
    expect(notes[1].midi).toBe(64);
    expect(notes[0].t).toBeCloseTo(0, 3);
    expect(notes[1].t).toBeCloseTo(0.5, 3);
    expect(notes[0].dur).toBeCloseTo(0.5, 2);
    expect(len).toBeCloseTo(1.0, 2);
  });
});
