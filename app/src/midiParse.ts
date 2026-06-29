// Minimal Standard MIDI File parser → note events {midi, t(sec), dur(sec), vel(0..1)}.
// Lets us play the TRUE full score of a public-domain piece by dropping a .mid file
// in (far more accurate than hand-typed notes). Handles format 0/1, tempo maps,
// running status, note on/off pairing. (SMPTE division is uncommon for these — we
// assume ticks-per-quarter.)
export interface MidiNote { midi: number; t: number; dur: number; vel: number }

export function parseMidi(buf: ArrayBuffer): { notes: MidiNote[]; len: number } {
  const d = new DataView(buf);
  let p = 0;
  const u8 = () => d.getUint8(p++);
  const u16 = () => { const v = d.getUint16(p); p += 2; return v; };
  const u32 = () => { const v = d.getUint32(p); p += 4; return v; };
  const vlq = () => { let v = 0, b; do { b = u8(); v = (v << 7) | (b & 0x7f); } while (b & 0x80); return v; };

  if (u32() !== 0x4d546864) throw new Error("not a MIDI file");
  u32(); u16(); // header length, format
  const ntracks = u16();
  const division = u16();

  const tempos: { tick: number; uspq: number }[] = [];
  const raw: { tickOn: number; tickOff: number; midi: number; vel: number }[] = [];

  for (let tr = 0; tr < ntracks; tr++) {
    if (u32() !== 0x4d54726b) { const len = u32(); p += len; continue; }
    const len = u32();
    const end = p + len; // read length FIRST (p + u32() would use the pre-advance p)
    let tick = 0, status = 0;
    const pending: Record<number, { tick: number; vel: number }[]> = {};
    while (p < end) {
      tick += vlq();
      let s = u8();
      if (s < 0x80) { p--; s = status; }
      else if (s < 0xf0) status = s; // running status applies ONLY to channel messages
      const type = s & 0xf0;
      if (s === 0xff) { // meta — always consume exactly its declared length
        const meta = u8(); const mlen = vlq(); const mend = p + mlen;
        if (meta === 0x51 && mlen >= 3) tempos.push({ tick, uspq: (d.getUint8(p) << 16) | (d.getUint8(p + 1) << 8) | d.getUint8(p + 2) });
        p = mend;
      } else if (s === 0xf0 || s === 0xf7) { p += vlq(); }
      else if (type === 0x90) {
        const midi = u8(), vel = u8();
        if (vel > 0) (pending[midi] || (pending[midi] = [])).push({ tick, vel });
        else { const a = pending[midi]; if (a?.length) { const on = a.shift()!; raw.push({ tickOn: on.tick, tickOff: tick, midi, vel: on.vel }); } }
      } else if (type === 0x80) {
        const midi = u8(); u8();
        const a = pending[midi]; if (a?.length) { const on = a.shift()!; raw.push({ tickOn: on.tick, tickOff: tick, midi, vel: on.vel }); }
      } else if (type === 0xc0 || type === 0xd0) { u8(); }
      else { u8(); u8(); }
    }
    p = end;
  }

  tempos.sort((a, b) => a.tick - b.tick);
  const tickToSec = (tk: number) => {
    let sec = 0, last = 0, uspq = 500000;
    for (const te of tempos) { if (te.tick >= tk) break; sec += (te.tick - last) * (uspq / 1e6) / division; last = te.tick; uspq = te.uspq; }
    return sec + (tk - last) * (uspq / 1e6) / division;
  };

  const notes: MidiNote[] = raw.map((n) => {
    const t = tickToSec(n.tickOn);
    return { midi: n.midi, t, dur: Math.max(0.08, tickToSec(n.tickOff) - t), vel: n.vel / 127 };
  }).sort((a, b) => a.t - b.t);
  const len = notes.reduce((m, n) => Math.max(m, n.t + n.dur), 0);
  return { notes, len };
}
