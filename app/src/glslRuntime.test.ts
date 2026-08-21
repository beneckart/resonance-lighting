import { describe, it, expect } from "vitest";
import {
  hsv2rgb, evalPattern, buildFragmentShader, GLSL_PATTERNS, DEFAULT_UNIFORMS,
  type Vec3,
} from "./glslRuntime";

describe("hsv2rgb (GLSL-parity CPU port)", () => {
  it("primary hues land on the right RGB corners", () => {
    expect(hsv2rgb(0, 1, 1)).toEqual([1, 0, 0]); // red
    const g = hsv2rgb(1 / 3, 1, 1); expect(g[1]).toBeCloseTo(1, 5); expect(g[0]).toBeCloseTo(0, 5);
    const b = hsv2rgb(2 / 3, 1, 1); expect(b[2]).toBeCloseTo(1, 5); expect(b[1]).toBeCloseTo(0, 5);
  });
  it("saturation 0 = white·v (greyscale), value scales", () => {
    expect(hsv2rgb(0.4, 0, 1)).toEqual([1, 1, 1]);
    expect(hsv2rgb(0.4, 0, 0.5)).toEqual([0.5, 0.5, 0.5]);
  });
});

describe("evalPattern — CPU referee", () => {
  const pts: Vec3[] = [[0, 0, 0], [1, 2, 3], [-5, 10, 2], [0.3, -0.7, 0.1]];
  for (const id of Object.keys(GLSL_PATTERNS)) {
    it(`${id}: finite RGB in [0,1] across space + time, silent + loud`, () => {
      for (const u of [DEFAULT_UNIFORMS, { ...DEFAULT_UNIFORMS, bass: 0.9, mid: 0.7, treble: 0.5, onset: 1, speed: 2 }]) {
        for (const t of [0, 0.5, 4.2, 17.9]) {
          for (const p of pts) {
            const c = evalPattern(id, p, t, u);
            for (const ch of c) {
              expect(Number.isFinite(ch)).toBe(true);
              expect(ch).toBeGreaterThanOrEqual(0);
              expect(ch).toBeLessThanOrEqual(1.0000001);
            }
          }
        }
      }
    });
  }
  it("unknown id → black", () => {
    expect(evalPattern("nope", [0, 0, 0], 0)).toEqual([0, 0, 0]);
  });
  it("radialPulse responds to bass (louder bass → brighter)", () => {
    const quiet = evalPattern("radialPulse", [0.5, 0, 0], 0, { ...DEFAULT_UNIFORMS, bass: 0 });
    const loud = evalPattern("radialPulse", [0.5, 0, 0], 0, { ...DEFAULT_UNIFORMS, bass: 1 });
    const lum = (c: Vec3) => c[0] + c[1] + c[2];
    expect(lum(loud)).toBeGreaterThan(lum(quiet));
  });
});

describe("buildFragmentShader — RT pass wrapper (step 2 scaffold)", () => {
  it("wraps the body in the pattern contract + samples uPos at vUv, no readback", () => {
    const src = buildFragmentShader(GLSL_PATTERNS.radialPulse.glsl);
    expect(src).toContain("vec3 pattern(vec3 p, float t)");
    expect(src).toContain("texture2D(uPos, vUv)");
    expect(src).toContain("gl_FragColor");
    expect(src).toContain("uBeatPhase"); // audio uniforms present
    expect(src).toContain(GLSL_PATTERNS.radialPulse.glsl.trim().slice(0, 24));
  });
  it("every registered pattern builds a shader containing its body", () => {
    for (const pat of Object.values(GLSL_PATTERNS)) {
      const src = buildFragmentShader(pat.glsl);
      expect(src).toContain("void main()");
      expect(src.length).toBeGreaterThan(pat.glsl.length);
    }
  });
});
