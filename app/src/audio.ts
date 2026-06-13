// Web-Audio reactivity engine (B / the spine). Mic, file, or the test track →
// spectral-flux onset detection + interval-based BPM + asymmetric attack/release
// envelope followers (bass/mid/treble) + percentile-ish AGC. Per docs/research/12.
import { BeatTracker } from "./beat";

export type Section = "ambient" | "groove" | "build" | "peak";

export interface AudioFeatures {
  active: boolean;
  level: number; // AGC-normalized overall 0..1
  bass: number;
  mid: number;
  treble: number;
  beat: number; // onset envelope 0..1 (decays) — drives flashes
  onset: boolean; // true the frame an onset fires
  bpm: number; // detected tempo
  drop: number; // drop burst 0..1 (decays) — quiet build → spike
  section: Section; // live song-section tier (rekordbox-phrase analog)
  beatPhase: number; // PLL phase within the beat 0..1 (0 = downbeat) — predictive grid
  beatPulse: number; // 1 at the beat, decays toward next — on-grid swell for motion
}

export const audioFeatures: AudioFeatures = {
  active: false, level: 0, bass: 0, mid: 0, treble: 0, beat: 0, onset: false, bpm: 0, drop: 0, section: "ambient", beatPhase: 0, beatPulse: 0,
};

/** Classify the live song SECTION from level dynamics (doc 16 P0-2): a drop or
 *  loud passage = peak; a rising-from-quiet trend = build; steady mid = groove;
 *  quiet = ambient. Pure + testable. */
export function classifySection(level: number, pastMin: number, drop: number): Section {
  if (drop > 0.4 || level > 0.72) return "peak";
  if (level > 0.4 && level - pastMin > 0.28) return "build";
  if (level > 0.22) return "groove";
  return "ambient";
}

let ctx: AudioContext | null = null;
let analyser: AnalyserNode | null = null;
let freq: Uint8Array | null = null;
let prevSpec: Float32Array | null = null;
let audioEl: HTMLAudioElement | null = null;
// 3-band EQ (the DJ deck actually filters the sound). Boost-only to match the
// light-zone EQ semantics: knob 0 = neutral (0 dB), knob 1 = +12 dB on that band.
let eqLowNode: BiquadFilterNode | null = null;
let eqMidNode: BiquadFilterNode | null = null;
let eqHighNode: BiquadFilterNode | null = null;

/** EQ knob (0..1) → filter gain in dB (boost-only: 0 dB .. +12 dB). Pure. */
export function eqDbForKnob(knob: number): number {
  return Math.max(0, Math.min(1, knob)) * 12;
}

// envelope + onset + bpm state
let envBass = 0, envMid = 0, envTreble = 0, envLevel = 0;
let agcMax = 0.05;
let lastT = 0;
let lastDrop = -10;
const levelHist: number[] = [];
const tracker = new BeatTracker();

const now = () => performance.now() / 1000;
const alphaFor = (tauMs: number, dt: number) => 1 - Math.exp(-dt / (tauMs / 1000));

async function ensureCtx() {
  if (!ctx) {
    ctx = new AudioContext();
    analyser = ctx.createAnalyser();
    analyser.fftSize = 1024;
    analyser.smoothingTimeConstant = 0.2; // low → our own asymmetric smoothing keeps transients
    freq = new Uint8Array(analyser.frequencyBinCount);
    prevSpec = new Float32Array(analyser.frequencyBinCount);
    // 3-band EQ filter chain
    eqLowNode = ctx.createBiquadFilter();
    eqLowNode.type = "lowshelf";
    eqLowNode.frequency.value = 220;
    eqMidNode = ctx.createBiquadFilter();
    eqMidNode.type = "peaking";
    eqMidNode.frequency.value = 1200;
    eqMidNode.Q.value = 1;
    eqHighNode = ctx.createBiquadFilter();
    eqHighNode.type = "highshelf";
    eqHighNode.frequency.value = 3800;
  }
  if (ctx.state === "suspended") await ctx.resume();
}

/** Set the live EQ band gains from the control knobs (0..1 each). Boost-only. */
export function setEqGains(low: number, mid: number, high: number) {
  if (eqLowNode) eqLowNode.gain.value = eqDbForKnob(low);
  if (eqMidNode) eqMidNode.gain.value = eqDbForKnob(mid);
  if (eqHighNode) eqHighNode.gain.value = eqDbForKnob(high);
}

function connect(src: AudioNode, toDestination: boolean) {
  // src → low → mid → high → analyser (tap) ; high → destination (playback)
  src.connect(eqLowNode!);
  eqLowNode!.connect(eqMidNode!);
  eqMidNode!.connect(eqHighNode!);
  eqHighNode!.connect(analyser!);
  if (toDestination) eqHighNode!.connect(ctx!.destination);
  audioFeatures.active = true;
}

export interface AudioInput {
  id: string;
  label: string;
}

/** Pure: filter a MediaDeviceInfo list down to selectable audio inputs (mic,
 *  line-in, booth-out…). Labels are blank until permission is granted, so fall
 *  back to a short id stub. */
export function audioInputsFrom(devices: { kind: string; deviceId: string; label?: string }[]): AudioInput[] {
  return devices
    .filter((d) => d.kind === "audioinput")
    .map((d, i) => ({ id: d.deviceId, label: d.label || `input ${i + 1} (${d.deviceId.slice(0, 6) || "default"})` }));
}

/** Enumerate available audio inputs for the source picker (#5). */
export async function listAudioInputs(): Promise<AudioInput[]> {
  if (!navigator.mediaDevices?.enumerateDevices) return [];
  return audioInputsFrom(await navigator.mediaDevices.enumerateDevices());
}

/** Start from a mic / line-in / booth-out. Pass a deviceId from listAudioInputs
 *  to pick a specific input (e.g. the DJ booth output), else the default. */
export async function startMic(deviceId?: string) {
  await ensureCtx();
  const audio = deviceId ? { deviceId: { exact: deviceId } } : true;
  const stream = await navigator.mediaDevices.getUserMedia({ audio });
  connect(ctx!.createMediaStreamSource(stream), false);
}

export async function startFile(file: File) {
  await ensureCtx();
  if (audioEl) audioEl.pause();
  audioEl = new Audio(URL.createObjectURL(file));
  audioEl.loop = true;
  connect(ctx!.createMediaElementSource(audioEl), true);
  await audioEl.play();
}

export async function startTrack(url: string) {
  await ensureCtx();
  if (audioEl) audioEl.pause();
  audioEl = new Audio(url);
  audioEl.loop = true;
  audioEl.crossOrigin = "anonymous";
  connect(ctx!.createMediaElementSource(audioEl), true);
  await audioEl.play();
}

export function stopAudio() {
  audioEl?.pause();
  audioFeatures.active = false;
  audioFeatures.level = audioFeatures.bass = audioFeatures.mid = audioFeatures.treble = 0;
  audioFeatures.beat = 0;
  audioFeatures.onset = false;
}

export function updateAudio(): AudioFeatures {
  audioFeatures.onset = false;
  if (!analyser || !freq || !prevSpec || !audioFeatures.active) {
    audioFeatures.beat *= 0.85;
    audioFeatures.drop *= 0.9;
    return audioFeatures;
  }
  const data = freq;
  analyser.getByteFrequencyData(data as unknown as Uint8Array<ArrayBuffer>);
  const n = data.length;
  const t = now();
  const dt = lastT ? Math.min(0.1, Math.max(0.001, t - lastT)) : 0.016;
  lastT = t;

  const band = (a: number, b: number) => {
    let s = 0;
    for (let i = a; i < b; i++) s += data[i];
    return s / ((b - a) * 255);
  };
  const rawBass = band(0, Math.floor(n * 0.08));
  const rawMid = band(Math.floor(n * 0.08), Math.floor(n * 0.4));
  const rawTreble = band(Math.floor(n * 0.4), n);
  const rawLevel = (rawBass + rawMid + rawTreble) / 3;

  // asymmetric envelope followers (fast attack / slow release)
  const follow = (env: number, x: number, atk: number, rel: number) =>
    env + alphaFor(x > env ? atk : rel, dt) * (x - env);
  envBass = follow(envBass, rawBass, 10, 250);
  envMid = follow(envMid, rawMid, 6, 180);
  envTreble = follow(envTreble, rawTreble, 3, 120);
  envLevel = follow(envLevel, rawLevel, 8, 400);

  // AGC (slow-decaying running max)
  agcMax = Math.max(agcMax * 0.999, envLevel, 0.02);

  // spectral flux over the KICK/BASS band only → locks onsets to the beat
  // (full-spectrum flux also fires on hats/chords and confuses the tempo)
  let flux = 0;
  const fluxBins = Math.max(4, Math.floor(n * 0.1));
  for (let i = 0; i < n; i++) {
    const v = data[i] / 255;
    if (i < fluxBins) {
      const d = v - prevSpec[i];
      if (d > 0) flux += d;
    }
    prevSpec[i] = v;
  }
  flux /= fluxBins;
  const bt = tracker.push(flux, t);
  audioFeatures.onset = bt.onset;
  audioFeatures.bpm = bt.bpm;
  audioFeatures.beatPhase = bt.phase;
  // on-grid swell: peaks at the downbeat (phase 0) and falls off toward the next
  // beat — a smooth, PREDICTIVE pulse that doesn't wait for an onset to fire
  audioFeatures.beatPulse = bt.bpm > 0 ? Math.pow(1 - bt.phase, 2) : 0;
  if (bt.onset) audioFeatures.beat = 1;
  else audioFeatures.beat *= 0.86;

  audioFeatures.bass = envBass;
  audioFeatures.mid = envMid;
  audioFeatures.treble = envTreble;
  audioFeatures.level = Math.min(1, envLevel / agcMax);

  // drop detection: quiet build (window min low) → current spike
  levelHist.push(audioFeatures.level);
  if (levelHist.length > 40) levelHist.shift();
  const past = levelHist.slice(0, -6);
  const pastMin = past.length ? Math.min(...past) : 1;
  if (audioFeatures.level > 0.55 && pastMin < 0.18 && t - lastDrop > 2) {
    lastDrop = t;
    audioFeatures.drop = 1;
  } else {
    audioFeatures.drop *= 0.9;
  }

  audioFeatures.section = classifySection(audioFeatures.level, pastMin, audioFeatures.drop);
  return audioFeatures;
}
