import { describe, it, expect } from "vitest";
import { evalPattern, GLSL_PATTERNS, DEFAULT_UNIFORMS, type Vec3 } from "./glslRuntime";

// GOLDEN-FRAME REFEREE (RESEARCH-GLSL-RUNTIME): the CPU port is the reference
// that the GPU pass + Ben's ESP32 render3D must reproduce. These tests LOCK the
// reference at documented fixed inputs — any drift in the pattern math (here or,
// once transliterated, on hardware) is caught. Expected values are derived from
// the published formula, not snapshotted.

const round4 = (c: Vec3): Vec3 => c.map((x) => Math.round(x * 1e4) / 1e4) as Vec3;

describe("golden-frame referee — locked CPU reference", () => {
  it("radialPulse at the origin, default uniforms — derived exact", () => {
    // r=0 → wave=sin(0)=0 → v=0.5; ×(0.6+0.8·bass=0.6)=0.3; +0.4·onset(0)=0.3.
    // hue=fract(0.1+0)=0.1; hsv2rgb(0.1,0.9,0.3) → R=0.1+0.9·1, G=0.1+0.9·0.6,
    // B=0.1+0.9·0, all ×v(0.3) = [0.3, 0.192, 0.03].
    const c = round4(evalPattern("radialPulse", [0, 0, 0], 0, DEFAULT_UNIFORMS));
    expect(c).toEqual([0.3, 0.192, 0.03]);
  });

  it("chromaticCrown at the crown (y=1) at t=0 — on the k=0 wavefront", () => {
    // u = 1-(1·0.5+0.5) = 0; flow=0; k=0 → w=0 → d=0 → smoothstep(0.26,0,0)=1;
    // inten=1·(0.5+0)≈0.5 → bv≈0.5, hue=base 0.1. value=0.12+0.95·0.5005≈0.5955.
    const c = evalPattern("chromaticCrown", [0, 1, 0], 0, DEFAULT_UNIFORMS);
    // k=0 wins → warm base hue (R dominant), mid band lit
    expect(c[0]).toBeGreaterThan(c[2]); // reddish (hue ~0.1), not blue
    expect(c[0] + c[1] + c[2]).toBeGreaterThan(0.3); // on the wavefront → lit
  });

  it("every pattern is deterministic (same input → identical output)", () => {
    const u = { ...DEFAULT_UNIFORMS, bass: 0.5, mid: 0.3, treble: 0.2, speed: 1.7 };
    for (const id of Object.keys(GLSL_PATTERNS)) {
      const a = evalPattern(id, [0.3, -0.4, 0.6], 2.5, u);
      const b = evalPattern(id, [0.3, -0.4, 0.6], 2.5, u);
      expect(a).toEqual(b);
    }
  });

  it("spectrumBands routes each height band to its own audio band", () => {
    // band by p.y: with bass loud + mid/treble silent, a band-0 fixture is bright
    const loudBass = { ...DEFAULT_UNIFORMS, bass: 1, mid: 0, treble: 0 };
    // sample y values that land in band 0 (fract(y·0.15)·3 < 1)
    let foundBright = false;
    for (let y = 0; y < 7; y += 0.3) {
      const band = Math.floor(((y * 0.15) % 1) * 3);
      const c = evalPattern("spectrumBands", [0, y, 0], 0, loudBass);
      const lum = c[0] + c[1] + c[2];
      if (band === 0) { expect(lum).toBeGreaterThan(0.3); foundBright = true; }
    }
    expect(foundBright).toBe(true);
  });
});
