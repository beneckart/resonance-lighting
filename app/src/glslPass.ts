// GLSL pattern RT pass — step 2 of RESEARCH-GLSL-RUNTIME Approach B.
//
// Packs the 118 fixture positions into an N×1 DataTexture, runs one pattern
// fragment-shader pass into an N×1 render target (each texel = one fixture's
// RGB), and reads the texels back to drive instanceColor. This interim version
// uses a tiny one-shot readback (N×1 = 118 px ≈ sub-ms — the doc only warns
// against LARGE per-frame readbacks); the no-readback ideal (InstancedMesh
// samples the RT in its own shader) is the next refinement.
//
// Opt-in (control.glslMode): when OFF the proven CPU twin is untouched.

import {
  DataTexture, RGBAFormat, FloatType, UnsignedByteType, NearestFilter,
  WebGLRenderTarget, Scene, OrthographicCamera, Mesh, PlaneGeometry, ShaderMaterial,
  type WebGLRenderer,
} from "three";
import type { SimFixture } from "./store";
import type { AudioFeatures } from "./audio";
import type { Control } from "./store";
import { buildFragmentShader, GLSL_PATTERNS } from "./glslRuntime";

/** Pack fixture positions into an N×4 (RGBA) Float array, normalized to ~[-1,1]
 *  about the cloud centre so patterns are scale-independent (this normalized p
 *  is the shared contract input for CPU referee + GPU + future ESP32). Pure. */
export function packPositions(fixtures: SimFixture[]): Float32Array {
  const n = fixtures.length;
  const data = new Float32Array(n * 4);
  if (n === 0) return data;
  let cx = 0, cy = 0, cz = 0;
  for (const f of fixtures) { cx += f.pos[0]; cy += f.pos[1]; cz += f.pos[2]; }
  cx /= n; cy /= n; cz /= n;
  let ext = 1e-3;
  for (const f of fixtures) {
    ext = Math.max(ext, Math.abs(f.pos[0] - cx), Math.abs(f.pos[1] - cy), Math.abs(f.pos[2] - cz));
  }
  const inv = 1 / ext;
  for (let i = 0; i < n; i++) {
    const p = fixtures[i].pos;
    data[i * 4 + 0] = (p[0] - cx) * inv;
    data[i * 4 + 1] = (p[1] - cy) * inv;
    data[i * 4 + 2] = (p[2] - cz) * inv;
    data[i * 4 + 3] = 1;
  }
  return data;
}

/** Assemble the normalized uniform bus from the live control + audio. Pure. */
export function passUniforms(t: number, c: Control, a: AudioFeatures) {
  return {
    uTime: t,
    uSpeed: c.speed,
    uHue: c.hue,
    uSat: c.sat,
    uDensity: 1,
    uBass: a.active ? a.bass : 0,
    uMid: a.active ? a.mid : 0,
    uTreble: a.active ? a.treble : 0,
    uOnset: a.active ? a.beat : 0,
    uBPM: a.bpm,
    uBeatPhase: a.beatPhase,
  };
}

const VERT = `varying vec2 vUv; void main(){ vUv = uv; gl_Position = vec4(position.xy, 0.0, 1.0); }`;

export interface GlslPass {
  patternId: string;
  /** Render the pattern into the RT and read the N×4 RGBA bytes back (0..255). */
  renderRead: (gl: WebGLRenderer, t: number, c: Control, a: AudioFeatures) => Uint8Array;
  setPattern: (id: string) => void;
  dispose: () => void;
}

/** Build the RT pass for a fixture set. Three.js objects are constructed here
 *  but no GL work happens until renderRead() runs inside the R3F frame. */
export function createGlslPass(fixtures: SimFixture[], patternId = "radialPulse"): GlslPass {
  const n = Math.max(1, fixtures.length);
  const posTex = new DataTexture(packPositions(fixtures) as unknown as Float32Array<ArrayBuffer>, n, 1, RGBAFormat, FloatType);
  posTex.needsUpdate = true;

  const rt = new WebGLRenderTarget(n, 1, {
    format: RGBAFormat, type: UnsignedByteType, minFilter: NearestFilter, magFilter: NearestFilter,
    depthBuffer: false, stencilBuffer: false,
  });

  const uniforms: Record<string, { value: unknown }> = {
    uPos: { value: posTex },
    uTime: { value: 0 }, uSpeed: { value: 1 }, uHue: { value: 0.1 }, uSat: { value: 0.9 }, uDensity: { value: 1 },
    uBass: { value: 0 }, uMid: { value: 0 }, uTreble: { value: 0 }, uOnset: { value: 0 }, uBPM: { value: 0 }, uBeatPhase: { value: 0 },
  };

  const build = (id: string) =>
    new ShaderMaterial({ vertexShader: VERT, fragmentShader: buildFragmentShader((GLSL_PATTERNS[id] ?? GLSL_PATTERNS.radialPulse).glsl), uniforms });

  const scene = new Scene();
  const cam = new OrthographicCamera(-1, 1, 1, -1, 0, 1);
  let mat = build(patternId);
  const quad = new Mesh(new PlaneGeometry(2, 2), mat);
  scene.add(quad);

  const buf = new Uint8Array(n * 4);
  let curId = patternId;

  return {
    get patternId() { return curId; },
    setPattern(id: string) {
      if (id === curId) return;
      curId = id;
      mat.dispose();
      mat = build(id);
      quad.material = mat;
    },
    renderRead(gl, t, c, a) {
      const u = passUniforms(t, c, a);
      for (const key in u) uniforms[key].value = (u as Record<string, number>)[key];
      const prev = gl.getRenderTarget();
      gl.setRenderTarget(rt);
      gl.render(scene, cam);
      gl.readRenderTargetPixels(rt, 0, 0, n, 1, buf);
      gl.setRenderTarget(prev);
      return buf;
    },
    dispose() {
      posTex.dispose(); rt.dispose(); mat.dispose(); quad.geometry.dispose();
    },
  };
}
