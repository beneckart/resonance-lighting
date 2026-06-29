// PIANO AUDIO — synthesises the piece's notes with Web Audio so you actually HEAR
// the music (output goes to the system audio device, e.g. a Bluetooth speaker).
// Triggered from the same piano clock as the lights → audio + lights stay in sync.
let ctx: AudioContext | null = null;
let master: GainNode | null = null;
let soundOn = false;

/** Enable/disable + (lazily) create the AudioContext. MUST be called from a user
 *  gesture (a button click) the first time, or the browser blocks audio. */
export function setPianoSound(on: boolean) {
  soundOn = on;
  if (on) {
    if (!ctx) {
      ctx = new AudioContext();
      master = ctx.createGain();
      master.gain.value = 0.32;
      const comp = ctx.createDynamicsCompressor(); // tame stacked notes
      master.connect(comp);
      comp.connect(ctx.destination);
    }
    void ctx.resume();
  }
}
export function isPianoSound() { return soundOn && !!ctx; }

const midiToFreq = (m: number) => 440 * Math.pow(2, (m - 69) / 12);

/** Play one note: a short piano-ish tone (triangle + octave partial, fast attack,
 *  exponential decay scaled to the note's duration). */
export function playPianoNote(midi: number, vel: number, dur: number) {
  if (!soundOn || !ctx || !master) return;
  const now = ctx.currentTime;
  const freq = midiToFreq(midi);
  const peak = Math.min(0.5, 0.12 + vel * 0.5);
  const decay = Math.max(0.35, dur * 0.9);

  const g = ctx.createGain();
  g.gain.setValueAtTime(0.0001, now);
  g.gain.exponentialRampToValueAtTime(peak, now + 0.006);   // percussive attack
  g.gain.exponentialRampToValueAtTime(0.0001, now + decay); // ring out
  g.connect(master);

  const o1 = ctx.createOscillator(); o1.type = "triangle"; o1.frequency.value = freq;
  const o2 = ctx.createOscillator(); o2.type = "sine"; o2.frequency.value = freq * 2;
  const g2 = ctx.createGain(); g2.gain.value = 0.28; o2.connect(g2); g2.connect(g);
  o1.connect(g);
  o1.start(now); o2.start(now);
  const stop = now + decay + 0.05;
  o1.stop(stop); o2.stop(stop);
}
