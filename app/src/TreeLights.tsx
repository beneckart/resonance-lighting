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
    const ctrl = useTwin.getState().control;
    const audio = updateAudio();
    for (let i = 0; i < fixtures.length; i++) {
      litFor(t, fixtures[i], ctrl, audio, lit);
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
