import { useLayoutEffect, useRef } from "react";
import { useFrame } from "@react-three/fiber";
import { Color, InstancedMesh, Object3D } from "three";
import { useTwin } from "./store";
import { litFor, type Lit } from "./patterns";
import { updateAudio } from "./audio";

const dummy = new Object3D();
const col = new Color();
const lit: Lit = { r: 0, g: 0, b: 0 };
const GAIN = 2.6; // push bright fixtures into HDR so Bloom makes them glow like lanterns

/**
 * Renders ALL fixtures as one InstancedMesh.
 * - The pattern engine (litFor) + overrides compute each fixture's COMMANDED render.
 * - A mock heartbeat transport (G1) turns that into REPORTED state: when enabled,
 *   each fixture only "reports" its color at a jittered ~0.6–1.2s interval (held in
 *   between → visible staleness), and `deadCount` fixtures stop reporting entirely.
 * - The render shows REPORTED state only (the mirror rule). Monitor mode (F3) tints
 *   dead fixtures red and the store publishes reporting/dead/stale counts.
 * Swap this mock transport for the real ESP-NOW heartbeat and nothing else changes.
 */
export function TreeLights() {
  const fixtures = useTwin((s) => s.fixtures);
  const size = useTwin((s) => s.size) * 0.012;
  const ref = useRef<InstancedMesh>(null);

  // reported-state buffers (the "what the tree last told us" layer)
  const repR = useRef<Float32Array>(new Float32Array(0));
  const repG = useRef<Float32Array>(new Float32Array(0));
  const repB = useRef<Float32Array>(new Float32Array(0));
  const lastReport = useRef<Float32Array>(new Float32Array(0));
  const statsAt = useRef(0);

  useLayoutEffect(() => {
    const mesh = ref.current;
    if (!mesh) return;
    const n = fixtures.length;
    repR.current = new Float32Array(n);
    repG.current = new Float32Array(n);
    repB.current = new Float32Array(n);
    lastReport.current = new Float32Array(n).fill(-1e9);
    fixtures.forEach((f, i) => {
      dummy.position.set(f.pos[0], f.pos[1], f.pos[2]);
      dummy.scale.setScalar(1);
      dummy.updateMatrix();
      mesh.setMatrixAt(i, dummy.matrix);
      mesh.setColorAt(i, col.setRGB(0, 0, 0));
    });
    mesh.instanceMatrix.needsUpdate = true;
    if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
  }, [fixtures]);

  useFrame((state) => {
    const mesh = ref.current;
    const n = fixtures.length;
    if (!mesh || n === 0) return;
    const t = state.clock.elapsedTime;
    const st = useTwin.getState();
    const { control: ctrl, overrides, view } = st;
    const audio = updateAudio();
    let dead = 0;
    let stale = 0;

    for (let i = 0; i < n; i++) {
      const f = fixtures[i];
      // commanded render
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
        // never updates → frozen / no signal
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
        col.setRGB(0.5, 0.0, 0.0); // "no signal" marker
      } else {
        col.setRGB(repR.current[i] * GAIN, repG.current[i] * GAIN, repB.current[i] * GAIN);
      }
      mesh.setColorAt(i, col);
    }
    if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;

    if (t - statsAt.current > 0.5) {
      statsAt.current = t;
      st.setMonitorStats({ reporting: n - dead - stale, dead, stale });
    }
  });

  if (fixtures.length === 0) return null;
  return (
    <instancedMesh
      ref={ref}
      args={[undefined as never, undefined as never, fixtures.length]}
      key={fixtures.length}
    >
      <sphereGeometry args={[size, 12, 12]} />
      <meshBasicMaterial toneMapped={false} />
    </instancedMesh>
  );
}
