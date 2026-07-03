// UNITY sound — a bright ascending "fanfare" shimmer for community mode. Self-contained
// WebAudio (its own context, lazily created). Autoplay policy: the context unlocks on
// the visitor taps that precede Unity; in the real install the audio bus is armed.
let ctx: AudioContext | null = null;

function ensure(): AudioContext | null {
  try {
    if (!ctx) ctx = new (window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext)();
    if (ctx.state === "suspended") void ctx.resume();
    return ctx;
  } catch { return null; }
}

const midiToFreq = (m: number) => 440 * Math.pow(2, (m - 69) / 12);

/** A rising shimmer chord — a warm pentatonic arpeggio that blooms and rings out,
 *  signalling the whole community lit the tree. ~3s. */
export function playUnityFanfare() {
  const c = ensure();
  if (!c) return;
  const now = c.currentTime;
  const bus = c.createGain();
  bus.gain.value = 0.0001;
  bus.gain.exponentialRampToValueAtTime(0.5, now + 0.08);
  bus.gain.exponentialRampToValueAtTime(0.0001, now + 3.2);
  const rev = c.createBiquadFilter(); rev.type = "lowpass"; rev.frequency.value = 5000;
  bus.connect(rev); rev.connect(c.destination);
  // rising major-pentatonic sweep, two octaves, shimmering
  const notes = [60, 62, 64, 67, 69, 72, 74, 76, 79, 81, 84];
  notes.forEach((m, i) => {
    const t = now + i * 0.09;
    const f = midiToFreq(m);
    const g = c.createGain();
    g.gain.setValueAtTime(0.0001, t);
    g.gain.exponentialRampToValueAtTime(0.22, t + 0.02);
    g.gain.exponentialRampToValueAtTime(0.0001, t + 1.4);
    for (const [mult, amp, type] of [[1, 1, "triangle"], [2, 0.3, "sine"], [3, 0.12, "sine"]] as [number, number, OscillatorType][]) {
      const o = c.createOscillator(); o.type = type; o.frequency.value = f * mult;
      const og = c.createGain(); og.gain.value = amp; o.connect(og); og.connect(g);
      o.start(t); o.stop(t + 1.5);
    }
    g.connect(bus);
  });
}
