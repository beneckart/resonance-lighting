import { useLayoutEffect, useMemo, useRef } from "react";
import { useFrame } from "@react-three/fiber";
import { AdditiveBlending, Color, ConeGeometry, InstancedMesh, Object3D, Quaternion, Vector3 } from "three";
import { useTwin } from "./store";
import { litFor, type Lit } from "./patterns";
import { updateAudio } from "./audio";
import { strobeGate, eqGain, lerp } from "./dj";
import { rippleIntensity } from "./interaction";
import { guestClamp } from "./guard";
import { relativeBeam } from "./photometry";
import { applyEnv } from "./sensors";
import { easeGroundTint } from "./groundtint";

const dummy = new Object3D();
const col = new Color();
// beam-aim helpers: orient each cast cone along its real direction
const DOWN = new Vector3(0, -1, 0);
const aimV = new Vector3();
const aimQ = new Quaternion();
const lit: Lit = { r: 0, g: 0, b: 0 };
const litB: Lit = { r: 0, g: 0, b: 0 };
const GAIN = 1.65;
const DEG = Math.PI / 180;
// Tight "ray" cone: a narrow reference half-angle gives crisp light shafts
// instead of a washy flood. Per-fixture beamDeg is mapped against this below.
const RAY_HALF = 15 * DEG;
const refTan = Math.tan(RAY_HALF);

/** All lights as one InstancedMesh of lanterns + (optionally) one of beam cones.
 *  Render style (A7) is the `visualizer` mode: lanterns / orbs / wire.
 *  Tick = firmware stand-in (mock heartbeat, G1); render shows REPORTED state. */
export function TreeLights() {
  const fixtures = useTwin((s) => s.fixtures);
  const treeSize = useTwin((s) => s.size);
  const center = useTwin((s) => s.center);
  const viz = useTwin((s) => s.control.visualizer);
  const dotSize = treeSize * (viz === "orbs" ? 0.022 : viz === "wire" ? 0.009 : 0.012);
  const beamH = treeSize * 0.38;
  const showBeams = viz !== "wire";

  const lightRef = useRef<InstancedMesh>(null);
  const beamRef = useRef<InstancedMesh>(null);

  const beamGeom = useMemo(() => {
    const g = new ConeGeometry(beamH * refTan, beamH, 18, 1, true);
    g.translate(0, -beamH / 2, 0);
    return g;
  }, [beamH]);

  const repR = useRef<Float32Array>(new Float32Array(0));
  const repG = useRef<Float32Array>(new Float32Array(0));
  const repB = useRef<Float32Array>(new Float32Array(0));
  // displayed (smoothed) colour — a low-pass slew toward the reported value so
  // every transition (pattern / cue / crossfade / colour) eases instead of cutting
  const dispR = useRef<Float32Array>(new Float32Array(0));
  const dispG = useRef<Float32Array>(new Float32Array(0));
  const dispB = useRef<Float32Array>(new Float32Array(0));
  const lastReport = useRef<Float32Array>(new Float32Array(0));
  const statsAt = useRef(0);

  useLayoutEffect(() => {
    const lm = lightRef.current;
    if (!lm) return;
    const bm = beamRef.current;
    const n = fixtures.length;
    repR.current = new Float32Array(n);
    repG.current = new Float32Array(n);
    repB.current = new Float32Array(n);
    dispR.current = new Float32Array(n);
    dispG.current = new Float32Array(n);
    dispB.current = new Float32Array(n);
    lastReport.current = new Float32Array(n).fill(-1e9);
    fixtures.forEach((f, i) => {
      dummy.position.set(f.pos[0], f.pos[1], f.pos[2]);
      dummy.rotation.set(0, 0, 0);
      dummy.scale.setScalar(dotSize);
      dummy.updateMatrix();
      lm.setMatrixAt(i, dummy.matrix);
      lm.setColorAt(i, col.setRGB(0, 0, 0));
      if (bm) {
        // map the fixture's real beam angle to a TIGHT visual ray: clamp wide
        // floods (≤70°) and cap width at 1.6× the reference so rays stay crisp.
        const s = Math.max(0.5, Math.min(1.6, Math.tan(Math.min(70, Math.max(12, f.beamDeg)) * 0.5 * DEG) / refTan));
        // real cast geometry: use the fixture's true aim (schema 0.2) when the
        // Blender export provides it; else fall back to the heuristic (canopy
        // downlights aim DOWN + fan radially OUTWARD from the trunk).
        if (f.aim) {
          aimV.set(f.aim[0], f.aim[1], f.aim[2]).normalize();
        } else {
          const rx = f.pos[0] - center[0], rz = f.pos[2] - center[2];
          const rl = Math.hypot(rx, rz) || 1;
          aimV.set((rx / rl) * 0.5, -1, (rz / rl) * 0.5).normalize();
        }
        aimQ.setFromUnitVectors(DOWN, aimV);
        dummy.quaternion.copy(aimQ);
        dummy.scale.set(s, 1, s);
        dummy.updateMatrix();
        bm.setMatrixAt(i, dummy.matrix);
        bm.setColorAt(i, col.setRGB(0, 0, 0));
        dummy.quaternion.identity(); // reset for next light-mesh write
      }
    });
    lm.instanceMatrix.needsUpdate = true;
    if (lm.instanceColor) lm.instanceColor.needsUpdate = true;
    if (bm) {
      bm.instanceMatrix.needsUpdate = true;
      if (bm.instanceColor) bm.instanceColor.needsUpdate = true;
    }
  }, [fixtures, dotSize]);

  useFrame((state, delta) => {
    const lm = lightRef.current;
    const n = fixtures.length;
    if (!lm || n === 0) return;
    // low-pass slew factor: ~110ms time-constant → soft transitions, clamped
    const k = Math.min(1, 1 - Math.exp(-Math.max(0, delta) / 0.11));
    const bm = beamRef.current;
    const t = state.clock.elapsedTime;
    const st = useTwin.getState();
    const { overrides, view } = st;
    // fold live environmental sensors (crowd/temp/wind/daylight) into the look
    const ctrl = applyEnv(st.guest ? guestClamp(st.control) : st.control, st.sensors);
    const audio = updateAudio();
    // DJ controller (C): crossfade to look B, master intensity, strobe gate
    const xfade = ctrl.xfade;
    const cb = xfade > 0.001 ? { ...ctrl, pattern: ctrl.djPatternB, hue: ctrl.djHueB } : null;
    const mg = ctrl.master * (ctrl.strobe ? strobeGate(t, ctrl.strobeHz) : 1);
    const ripples = st.ripples;
    const nowS = performance.now() / 1000;
    const rSpeed = treeSize * 0.55;
    const rWidth = treeSize * 0.06;
    let dead = 0;
    let stale = 0;
    let sumR = 0, sumG = 0, sumB = 0; // aggregate for the ground tint

    for (let i = 0; i < n; i++) {
      const f = fixtures[i];
      litFor(t, f, ctrl, audio, n, lit);
      if (cb) {
        litFor(t, f, cb, audio, n, litB);
        lit.r = lerp(lit.r, litB.r, xfade);
        lit.g = lerp(lit.g, litB.g, xfade);
        lit.b = lerp(lit.b, litB.b, xfade);
      }
      const ov = overrides[i];
      if (ov) {
        if (ov.mode === "off") {
          lit.r = lit.g = lit.b = 0;
        } else if (ov.rgb) {
          const m = ctrl.brightness;
          lit.r = ov.rgb[0] * m;
          lit.g = ov.rgb[1] * m;
          lit.b = ov.rgb[2] * m;
        }
      }
      // EQ→zone gain × master × strobe
      const g = mg * eqGain(f.zone, ctrl.eqLow, ctrl.eqMid, ctrl.eqHigh, audio);
      lit.r *= g; lit.g *= g; lit.b *= g;

      // presence→ripple: brighten as a wavefront passes
      if (ripples.length) {
        let boost = 0;
        for (const rp of ripples) {
          const dx = f.pos[0] - rp.x, dy = f.pos[1] - rp.y, dz = f.pos[2] - rp.z;
          const b = rippleIntensity(Math.sqrt(dx * dx + dy * dy + dz * dz), nowS - rp.t0, rSpeed, rWidth);
          if (b > boost) boost = b;
        }
        if (boost > 0) { const m = 1 + 2.2 * boost; lit.r *= m; lit.g *= m; lit.b *= m; }
      }

      const isDead = view.mock && f.seq < view.deadCount;
      if (!view.mock) {
        repR.current[i] = lit.r; repG.current[i] = lit.g; repB.current[i] = lit.b;
        lastReport.current[i] = t;
      } else if (isDead) {
        dead++;
      } else {
        const interval = 0.6 + f.rnd * 0.6;
        if (t - lastReport.current[i] >= interval) {
          repR.current[i] = lit.r; repG.current[i] = lit.g; repB.current[i] = lit.b;
          lastReport.current[i] = t;
        }
        if (t - lastReport.current[i] > 1.3) stale++;
      }

      if (view.monitor && isDead) {
        lm.setColorAt(i, col.setRGB(0.5, 0, 0));
        if (bm) bm.setColorAt(i, col.setRGB(0, 0, 0));
      } else {
        // ease displayed colour toward the reported value (smooth all transitions)
        dispR.current[i] += (repR.current[i] - dispR.current[i]) * k;
        dispG.current[i] += (repG.current[i] - dispG.current[i]) * k;
        dispB.current[i] += (repB.current[i] - dispB.current[i]) * k;
        const r = dispR.current[i], g = dispG.current[i], b = dispB.current[i];
        sumR += r; sumG += g; sumB += b;
        lm.setColorAt(i, col.setRGB(r * GAIN, g * GAIN, b * GAIN));
        if (bm) {
          // IES-ish: scale beam by the fixture's photometric intensity (lumens/beam angle)
          const bs = 0.55 * Math.min(2.5, Math.max(0.3, relativeBeam(f.lumens, f.beamDeg)));
          bm.setColorAt(i, col.setRGB(r * bs, g * bs, b * bs));
        }
      }
    }
    if (lm.instanceColor) lm.instanceColor.needsUpdate = true;
    if (bm?.instanceColor) bm.instanceColor.needsUpdate = true;

    // feed the live aggregate colour to the ground projection (real coloured
    // shapes on the floor): normalized hue + average luminance for intensity
    const inv = 1 / n;
    const ar = sumR * inv, ag = sumG * inv, ab = sumB * inv;
    const level = Math.min(1, (ar + ag + ab) / 3 * GAIN * 1.6);
    const mx = Math.max(ar, ag, ab, 1e-4);
    easeGroundTint(ar / mx, ag / mx, ab / mx, level, k);

    if (t - statsAt.current > 0.5) {
      statsAt.current = t;
      st.setMonitorStats({ reporting: n - dead - stale, dead, stale });
    }
  });

  if (fixtures.length === 0) return null;
  return (
    <group>
      <instancedMesh
        ref={beamRef}
        args={[undefined as never, undefined as never, fixtures.length]}
        key={`beam${fixtures.length}`}
        visible={showBeams}
      >
        <primitive object={beamGeom} attach="geometry" />
        <meshBasicMaterial transparent blending={AdditiveBlending} opacity={0.06} depthWrite={false} toneMapped={false} />
      </instancedMesh>
      <instancedMesh ref={lightRef} args={[undefined as never, undefined as never, fixtures.length]} key={`led${fixtures.length}`}>
        <sphereGeometry args={[1, 12, 12]} />
        <meshBasicMaterial toneMapped={false} wireframe={viz === "wire"} />
      </instancedMesh>
    </group>
  );
}
