import { Soundfont } from "smplr";

// PIANO AUDIO — plays a REAL sampled acoustic grand piano (recorded notes, via
// smplr) so it sounds like an actual piano, not a chiptune. Until the samples
// finish loading it falls back to a warmer additive synth. Output goes to the
// system audio device (route to a Bluetooth speaker). Triggered from the same
// piano clock as the lights → audio + lights stay in sync.
let ctx: AudioContext | null = null;
let master: GainNode | null = null;   // synth bus
let out: GainNode | null = null;      // master bus → speakers + recording tap
let recDest: MediaStreamAudioDestinationNode | null = null;
let soundOn = false;
let piano: Soundfont | null = null;
let pianoReady = false;

/** A live audio MediaStream of the piano output — for the in-app screen recorder
 *  (so the recording captures the music). Null until sound is enabled. */
export function getPianoAudioStream(): MediaStream | null { return recDest?.stream ?? null; }

export function pianoLoading() { return !!piano && !pianoReady; }
export function pianoSampled() { return pianoReady; }

export function setPianoSound(on: boolean) {
  soundOn = on;
  if (on) {
    if (!ctx) {
      ctx = new AudioContext();
      out = ctx.createGain();                       // master bus
      out.connect(ctx.destination);                 // → speakers
      recDest = ctx.createMediaStreamDestination();
      out.connect(recDest);                          // → recording tap
      master = ctx.createGain();
      master.gain.value = 0.5;
      const comp = ctx.createDynamicsCompressor();
      master.connect(comp);
      comp.connect(out);                             // synth → master bus
      try {
        piano = new Soundfont(ctx, { instrument: "acoustic_grand_piano", volume: 100, destination: out }); // sampled → master bus
        piano.load.then(() => { pianoReady = true; }).catch(() => { pianoReady = false; });
      } catch { piano = null; }
    }
    void ctx.resume();
  }
}
export function isPianoSound() { return soundOn && !!ctx; }

const midiToFreq = (m: number) => 440 * Math.pow(2, (m - 69) / 12);

/** Play one note: the sampled grand piano if loaded, else a warm additive synth. */
export function playPianoNote(midi: number, vel: number, dur: number) {
  if (!soundOn || !ctx || !master) return;
  if (pianoReady && piano) {
    piano.start({ note: midi, velocity: Math.max(18, Math.round(vel * 110)), duration: Math.max(0.25, dur) });
    return;
  }
  // ── fallback synth (warmer than a single triangle: 4 partials + lowpass) ──
  const now = ctx.currentTime;
  const freq = midiToFreq(midi);
  const peak = Math.min(0.45, 0.1 + vel * 0.45);
  const decay = Math.max(0.35, dur * 0.9);
  const g = ctx.createGain();
  g.gain.setValueAtTime(0.0001, now);
  g.gain.exponentialRampToValueAtTime(peak, now + 0.006);
  g.gain.exponentialRampToValueAtTime(0.0001, now + decay);
  const lp = ctx.createBiquadFilter();
  lp.type = "lowpass";
  lp.frequency.setValueAtTime(Math.min(8000, freq * 8), now);
  lp.frequency.exponentialRampToValueAtTime(Math.max(600, freq * 2.5), now + decay); // timbre darkens as it decays
  g.connect(lp); lp.connect(master);
  const parts: [number, number, OscillatorType][] = [[1, 1, "triangle"], [2, 0.4, "sine"], [3, 0.18, "sine"], [4, 0.08, "sine"]];
  const stop = now + decay + 0.05;
  for (const [mult, amp, type] of parts) {
    const o = ctx.createOscillator(); o.type = type; o.frequency.value = freq * mult;
    const og = ctx.createGain(); og.gain.value = amp; o.connect(og); og.connect(g);
    o.start(now); o.stop(stop);
  }
}
