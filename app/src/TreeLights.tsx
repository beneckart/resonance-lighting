import { useLayoutEffect, useRef } from "react";
import { useFrame } from "@react-three/fiber";
import { Color, InstancedMesh, Object3D } from "three";
import { useTwin } from "./store";
import { litFor, type Lit } from "./patterns";
import { updateAudio } from "./audio";

const dummy = new Object3D();
const col = new Color();
const lit: Lit = { r: 0, g: 0, b: 0 };

/**
 * Renders ALL fixtures as one InstancedMesh. The useFrame tick is the firmware
 * stand-in: it computes each fixture's REPORTED color (litFor) and writes it to the
 * instance — the render shows reported state only. Swap this tick for a real
 * heartbeat feed (G1) and the mirror is unchanged.
 */
export function TreeLights() {
  const fixtures = useTwin((s) => s.fixtures);
  const size = useTwin((s) => s.size) * 0.006;
  const ref = useRef<InstancedMesh>(null);

  useLayoutEffect(() => {
    const mesh = ref.current;
    if (!mesh) return;
    fixtures.forEach((f, i) => {
      dummy.position.set(f.pos[0], f.pos[1], f.pos[2]);
      dummy.scale.setScalar(1);
      dummy.updateMatrix();
      mesh.setMatrixAt(i, dummy.matrix);
    });
    mesh.instanceMatrix.needsUpdate = true;
    fixtures.forEach((_, i) => mesh.setColorAt(i, col.setRGB(0, 0, 0)));
    if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
  }, [fixtures]);

  useFrame((state) => {
    const mesh = ref.current;
    if (!mesh || fixtures.length === 0) return;
    const t = state.clock.elapsedTime;
    const st = useTwin.getState();
    const ctrl = st.control;
    const overrides = st.overrides;
    const audio = updateAudio();
    const n = fixtures.length;
    for (let i = 0; i < n; i++) {
      litFor(t, fixtures[i], ctrl, audio, n, lit);
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
      col.setRGB(lit.r, lit.g, lit.b);
      mesh.setColorAt(i, col);
    }
    if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
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
