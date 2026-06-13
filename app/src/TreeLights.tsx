import { useLayoutEffect, useMemo, useRef } from "react";
import { useFrame } from "@react-three/fiber";
import { AdditiveBlending, Color, ConeGeometry, InstancedMesh, Object3D } from "three";
import { useTwin } from "./store";
import { litFor, type Lit } from "./patterns";
import { updateAudio } from "./audio";

const dummy = new Object3D();
const col = new Color();
const lit: Lit = { r: 0, g: 0, b: 0 };
const GAIN = 2.1; // push bright fixtures into HDR so Bloom makes them glow like lanterns
const DEG = Math.PI / 180;
const refTan = Math.tan(60 * DEG); // reference half-angle for a 120° beam

/**
 * The tree's lights, as ONE InstancedMesh of glowing lanterns + ONE InstancedMesh of
 * additive volumetric beam cones (downward, sized by each fixture's beam angle, colored
 * by its reported state). The useFrame tick is the firmware stand-in (mock heartbeat
 * transport, G1); the render shows REPORTED state only (the mirror rule).
 */
export function TreeLights() {
  const fixtures = useTwin((s) => s.fixtures);
  const treeSize = useTwin((s) => s.size);
  const dotSize = treeSize * 0.012;
  const beamH = treeSize * 0.22;

  const lightRef = useRef<InstancedMesh>(null);
  const beamRef = useRef<InstancedMesh>(null);

  const beamGeom = useMemo(() => {
    const g = new ConeGeometry(beamH * refTan, beamH, 18, 1, true);
    g.translate(0, -beamH / 2, 0); // apex at fixture, opens downward
    return g;
  }, [beamH]);

  const repR = useRef<Float32Array>(new Float32Array(0));
  const repG = useRef<Float32Array>(new Float32Array(0));
  const repB = useRef<Float32Array>(new Float32Array(0));
  const lastReport = useRef<Float32Array>(new Float32Array(0));
  const statsAt = useRef(0);

  useLayoutEffect(() => {
    const lm = lightRef.current;
    const bm = beamRef.current;
    if (!lm || !bm) return;
    const n = fixtures.length;
    repR.current = new Float32Array(n);
    repG.current = new Float32Array(n);
    repB.current = new Float32Array(n);
    lastReport.current = new Float32Array(n).fill(-1e9);
    fixtures.forEach((f, i) => {
      // lantern
      dummy.position.set(f.pos[0], f.pos[1], f.pos[2]);
      dummy.rotation.set(0, 0, 0);
      dummy.scale.setScalar(1);
      dummy.updateMatrix();
      lm.setMatrixAt(i, dummy.matrix);
      // beam cone — scaled in XZ by the fixture's beam angle
      const s = Math.max(0.25, Math.tan(Math.min(160, Math.max(20, f.beamDeg)) * 0.5 * DEG) / refTan);
      dummy.scale.set(s, 1, s);
      dummy.updateMatrix();
      bm.setMatrixAt(i, dummy.matrix);
      lm.setColorAt(i, col.setRGB(0, 0, 0));
      bm.setColorAt(i, col.setRGB(0, 0, 0));
    });
    lm.instanceMatrix.needsUpdate = true;
    bm.instanceMatrix.needsUpdate = true;
    if (lm.instanceColor) lm.instanceColor.needsUpdate = true;
    if (bm.instanceColor) bm.instanceColor.needsUpdate = true;
  }, [fixtures]);

  useFrame((state) => {
    const lm = lightRef.current;
    const bm = beamRef.current;
    const n = fixtures.length;
    if (!lm || !bm || n === 0) return;
    const t = state.clock.elapsedTime;
    const st = useTwin.getState();
    const { control: ctrl, overrides, view } = st;
    const audio = updateAudio();
    let dead = 0;
    let stale = 0;

    for (let i = 0; i < n; i++) {
      const f = fixtures[i];
      litFor(t, f, ctrl, audio, n, lit);
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

      const isDead = view.mock && f.seq < view.deadCount;
      if (!view.mock) {
        repR.current[i] = lit.r;
        repG.current[i] = lit.g;
        repB.current[i] = lit.b;
        lastReport.current[i] = t;
      } else if (isDead) {
        dead++;
      } else {
        const interval = 0.6 + f.rnd * 0.6;
        if (t - lastReport.current[i] >= interval) {
          repR.current[i] = lit.r;
          repG.current[i] = lit.g;
          repB.current[i] = lit.b;
          lastReport.current[i] = t;
        }
        if (t - lastReport.current[i] > 1.3) stale++;
      }

      if (view.monitor && isDead) {
        col.setRGB(0.5, 0.0, 0.0);
        lm.setColorAt(i, col);
        bm.setColorAt(i, col.setRGB(0, 0, 0));
      } else {
        const r = repR.current[i];
        const g = repG.current[i];
        const b = repB.current[i];
        lm.setColorAt(i, col.setRGB(r * GAIN, g * GAIN, b * GAIN));
        bm.setColorAt(i, col.setRGB(r * 0.55, g * 0.55, b * 0.55));
      }
    }
    if (lm.instanceColor) lm.instanceColor.needsUpdate = true;
    if (bm.instanceColor) bm.instanceColor.needsUpdate = true;

    if (t - statsAt.current > 0.5) {
      statsAt.current = t;
      st.setMonitorStats({ reporting: n - dead - stale, dead, stale });
    }
  });

  if (fixtures.length === 0) return null;
  return (
    <group>
      <instancedMesh ref={beamRef} args={[undefined as never, undefined as never, fixtures.length]} key={`beam${fixtures.length}`}>
        <primitive object={beamGeom} attach="geometry" />
        <meshBasicMaterial transparent blending={AdditiveBlending} opacity={0.07} depthWrite={false} toneMapped={false} />
      </instancedMesh>
      <instancedMesh ref={lightRef} args={[undefined as never, undefined as never, fixtures.length]} key={`led${fixtures.length}`}>
        <sphereGeometry args={[dotSize, 12, 12]} />
        <meshBasicMaterial toneMapped={false} />
      </instancedMesh>
    </group>
  );
}
