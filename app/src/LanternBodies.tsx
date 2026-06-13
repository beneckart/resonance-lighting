import { useLayoutEffect, useMemo, useRef } from "react";
import { useGLTF } from "@react-three/drei";
import {
  type BufferGeometry, InstancedMesh, Mesh, Object3D, Quaternion, Vector3,
} from "three";
import { mergeGeometries } from "three/examples/jsm/utils/BufferGeometryUtils.js";
import { useTwin } from "./store";

/** The REAL downlight lantern body (blender-architect's downlight_lantern.glb)
 *  instanced at every fixture. Per Elliot the LED is buried in the tube — we do
 *  NOT see the source — so the body renders as a dim bamboo HOUSING; the light
 *  itself stays the beam + petal-gobo + glow (TreeLights). Aimed per the
 *  fixture's real `aim` (schema 0.2). */
const dummy = new Object3D();
const DOWN = new Vector3(0, -1, 0);
const aimV = new Vector3();
const aimQ = new Quaternion();

export function LanternBodies() {
  const fixtures = useTwin((s) => s.fixtures);
  const treeSize = useTwin((s) => s.size);
  const { scene } = useGLTF("/downlight_lantern.glb");
  const ref = useRef<InstancedMesh>(null);

  // merge the glb's meshes into one geometry so we can instance it cheaply
  const geom = useMemo<BufferGeometry | null>(() => {
    const geos: BufferGeometry[] = [];
    scene.updateMatrixWorld(true);
    scene.traverse((o) => {
      const m = o as Mesh;
      if (m.isMesh && m.geometry) {
        const g = m.geometry.clone();
        g.applyMatrix4(m.matrixWorld);
        // keep only position+normal so heterogeneous sub-meshes merge cleanly
        for (const k of Object.keys(g.attributes)) {
          if (k !== "position" && k !== "normal") g.deleteAttribute(k);
        }
        if (!g.getAttribute("normal")) g.computeVertexNormals();
        geos.push(g);
      }
    });
    if (!geos.length) return null;
    return geos.length === 1 ? geos[0] : mergeGeometries(geos, false);
  }, [scene]);

  // the glb IS modelled at real-world scale (metres, ~0.07×0.25 m) in the same
  // units as the fixture positions → scale 1 is geometrically accurate. The
  // bodies are intentionally small + dim (the LED source is hidden in the tube).
  const scale = useMemo(() => 1, []);

  useLayoutEffect(() => {
    const im = ref.current;
    if (!im || !geom) return;
    fixtures.forEach((f, i) => {
      dummy.position.set(f.pos[0], f.pos[1], f.pos[2]);
      if (f.aim) {
        aimV.set(f.aim[0], f.aim[1], f.aim[2]).normalize();
        aimQ.setFromUnitVectors(DOWN, aimV);
        dummy.quaternion.copy(aimQ);
      } else {
        dummy.quaternion.identity();
      }
      dummy.scale.setScalar(scale);
      dummy.updateMatrix();
      im.setMatrixAt(i, dummy.matrix);
    });
    im.instanceMatrix.needsUpdate = true;
  }, [fixtures, geom, scale, treeSize]);

  if (!fixtures.length || !geom) return null;
  return (
    <instancedMesh ref={ref} args={[geom, undefined as never, fixtures.length]} key={`body${fixtures.length}`}>
      {/* dim bamboo housing — the source stays hidden inside it */}
      <meshStandardMaterial color="#a9803c" roughness={0.85} metalness={0} emissive="#160c03" />
    </instancedMesh>
  );
}

useGLTF.preload("/downlight_lantern.glb");
