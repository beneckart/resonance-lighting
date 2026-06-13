// GLSL pattern runtime — Approach B from RESEARCH-GLSL-RUNTIME-AND-ART.md.
//
// The pattern contract is a GLSL `vec3 pattern(vec3 p, float t)` body. Each
// pattern ships that body (the source of truth for the GPU render-to-texture
// pass — step 2) PLUS a CPU port that reads the SAME uniforms. The CPU port is
// the "golden-frame referee": it's headless-testable here, and it's how the
// twin, Ben's ESP32 render3D, and a future C++→WASM build can be proven to
// agree on `pattern(p,t)`. The wire carries {pattern_id, params}, never pixels;
// fixtures.json is the single coordinate source for every backend.
//
// This module is self-contained and does NOT touch the live render path yet —
// step 2 wires the RT pass + InstancedMesh sampling. Build beside the twin.

export type Vec3 = [number, number, number];

/** Normalized uniform bus shared by every pattern + every backend. Mirrors the
 *  uniforms listed in the research doc (uTime/uSpeed/… + audio). */
export interface PatternUniforms {
  speed: number;
  hue: number; // 0..1 base hue
  sat: number; // 0..1
  density: number; // spatial frequency scaler
  bass: number;
  mid: number;
  treble: number;
  onset: number; // 0..1 beat flash
  bpm: number;
  beatPhase: number; // 0..1 PLL phase
}

export const DEFAULT_UNIFORMS: PatternUniforms = {
  speed: 1, hue: 0.1, sat: 0.9, density: 1,
  bass: 0, mid: 0, treble: 0, onset: 0, bpm: 0, beatPhase: 0,
};

export interface GlslPattern {
  id: string;
  /** GLSL body of `vec3 pattern(vec3 p, float t)` (returns 0..1 RGB). */
  glsl: string;
  /** CPU port of the SAME math — the golden-frame referee + headless reference. */
  cpu: (p: Vec3, t: number, u: PatternUniforms) => Vec3;
}

const clamp01 = (x: number) => Math.min(1, Math.max(0, x));
const fract = (x: number) => x - Math.floor(x);

/** HSV→RGB, identical math to the GLSL `hsv2rgb` injected into the shader, so
 *  the CPU referee and the GPU pass agree bit-for-bit (modulo float precision). */
export function hsv2rgb(h: number, s: number, v: number): Vec3 {
  // identical to GLSL_HSV2RGB: phase-offset hue ramp, then mix(white, k, s)·v
  const ramp = (off: number) => clamp01(Math.abs(fract(h + off) * 6 - 3) - 1);
  const k: Vec3 = [ramp(0), ramp(4 / 6), ramp(2 / 6)];
  const mix = (c: number) => (1 - s) + s * c; // mix(1.0, c, s)
  return [clamp01(mix(k[0]) * v), clamp01(mix(k[1]) * v), clamp01(mix(k[2]) * v)];
}

/** The GLSL hsv2rgb source injected into every fragment shader (matches the JS
 *  above — standard IQ-style hue ramp). */
export const GLSL_HSV2RGB = `vec3 hsv2rgb(float h, float s, float v){
  vec3 k = clamp(abs(fract(vec3(h)+vec3(0.0,4.0/6.0,2.0/6.0))*6.0-3.0)-1.0, 0.0, 1.0);
  return mix(vec3(1.0), k, s) * v;
}`;

// ---- pattern registry -------------------------------------------------------

const radialPulse: GlslPattern = {
  id: "radialPulse",
  glsl: `
    float r = length(p);
    float wave = sin(r * uDensity * 6.0 - t * uSpeed * 3.0);
    float v = 0.5 + 0.5 * wave;
    v *= 0.6 + 0.8 * uBass;          // bass swells the ring
    v += 0.4 * uOnset;               // flash on the beat
    return hsv2rgb(fract(uHue + r * 0.1), uSat, clamp(v, 0.0, 1.0));`,
  cpu: (p, t, u) => {
    const r = Math.hypot(p[0], p[1], p[2]);
    const wave = Math.sin(r * u.density * 6 - t * u.speed * 3);
    let v = 0.5 + 0.5 * wave;
    v *= 0.6 + 0.8 * u.bass;
    v += 0.4 * u.onset;
    return hsv2rgb(fract(u.hue + r * 0.1), u.sat, clamp01(v));
  },
};

const spectrumBands: GlslPattern = {
  id: "spectrumBands",
  glsl: `
    float band = floor(fract(p.y * 0.15 + t * uSpeed * 0.05) * 3.0); // 0/1/2 by height
    float lvl = band < 1.0 ? uBass : (band < 2.0 ? uMid : uTreble);
    float v = 0.35 + 0.9 * lvl;
    return hsv2rgb(fract(uHue + band / 3.0), uSat, clamp(v, 0.0, 1.0));`,
  cpu: (p, t, u) => {
    const band = Math.floor(fract(p[1] * 0.15 + t * u.speed * 0.05) * 3);
    const lvl = band < 1 ? u.bass : band < 2 ? u.mid : u.treble;
    const v = 0.35 + 0.9 * lvl;
    return hsv2rgb(fract(u.hue + band / 3), u.sat, clamp01(v));
  },
};

export const GLSL_PATTERNS: Record<string, GlslPattern> = {
  radialPulse,
  spectrumBands,
};

/** Evaluate a pattern's CPU referee. Returns black for an unknown id. */
export function evalPattern(id: string, p: Vec3, t: number, u: PatternUniforms = DEFAULT_UNIFORMS): Vec3 {
  const pat = GLSL_PATTERNS[id];
  if (!pat) return [0, 0, 0];
  return pat.cpu(p, t, u);
}

/** Wrap a pattern's GLSL body into the full fragment shader for the RT pass
 *  (step 2): each output texel = one fixture's RGB, sampled from the position
 *  texture uPos at vUv. No readback — the visible InstancedMesh samples the
 *  same target by index. Pure string build (testable without a GL context). */
export function buildFragmentShader(body: string): string {
  return `precision highp float;
varying vec2 vUv;
uniform sampler2D uPos;     // N×1 fixture positions (xyz)
uniform float uTime, uSpeed, uHue, uSat, uDensity;
uniform float uBass, uMid, uTreble, uOnset, uBPM, uBeatPhase;
${GLSL_HSV2RGB}
vec3 pattern(vec3 p, float t){
${body}
}
void main(){
  vec3 p = texture2D(uPos, vUv).xyz;
  gl_FragColor = vec4(pattern(p, uTime), 1.0);
}`;
}
