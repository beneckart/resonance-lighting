// Web Audio reactivity — mic or a loaded song → frequency features the patterns read.
export interface AudioFeatures {
  active: boolean;
  level: number; // overall 0..1
  bass: number;
  mid: number;
  treble: number;
  beat: number; // transient flux 0..1
}

export const audioFeatures: AudioFeatures = {
  active: false, level: 0, bass: 0, mid: 0, treble: 0, beat: 0,
};

let ctx: AudioContext | null = null;
let analyser: AnalyserNode | null = null;
let freq: Uint8Array | null = null;
let audioEl: HTMLAudioElement | null = null;
let prevBass = 0;

async function ensureCtx() {
  if (!ctx) {
    ctx = new AudioContext();
    analyser = ctx.createAnalyser();
    analyser.fftSize = 1024;
    analyser.smoothingTimeConstant = 0.8;
    freq = new Uint8Array(analyser.frequencyBinCount);
  }
  if (ctx.state === "suspended") await ctx.resume();
}

export async function startMic() {
  await ensureCtx();
  const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
  ctx!.createMediaStreamSource(stream).connect(analyser!);
  audioFeatures.active = true;
}

export async function startFile(file: File) {
  await ensureCtx();
  if (audioEl) audioEl.pause();
  audioEl = new Audio(URL.createObjectURL(file));
  audioEl.loop = true;
  const src = ctx!.createMediaElementSource(audioEl);
  src.connect(analyser!);
  src.connect(ctx!.destination); // so you also hear it
  await audioEl.play();
  audioFeatures.active = true;
}

export function stopAudio() {
  audioEl?.pause();
  audioFeatures.active = false;
  audioFeatures.level = audioFeatures.bass = audioFeatures.mid = audioFeatures.treble = audioFeatures.beat = 0;
}

export function updateAudio(): AudioFeatures {
  if (!analyser || !freq || !audioFeatures.active) return audioFeatures;
  const data = freq;
  // cast sidesteps the TS5.7 Uint8Array<ArrayBuffer> vs ArrayBufferLike generic mismatch
  analyser.getByteFrequencyData(data as unknown as Uint8Array<ArrayBuffer>);
  const n = data.length;
  const band = (a: number, b: number) => {
    let s = 0;
    for (let i = a; i < b; i++) s += data[i];
    return s / ((b - a) * 255);
  };
  const bass = band(0, Math.floor(n * 0.08));
  const mid = band(Math.floor(n * 0.08), Math.floor(n * 0.4));
  const treble = band(Math.floor(n * 0.4), n);
  const flux = Math.max(0, bass - prevBass);
  prevBass = bass * 0.92 + prevBass * 0.08;
  audioFeatures.bass = bass;
  audioFeatures.mid = mid;
  audioFeatures.treble = treble;
  audioFeatures.level = (bass + mid + treble) / 3;
  audioFeatures.beat = Math.min(1, flux * 6);
  return audioFeatures;
}
