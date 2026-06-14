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
    const g = geos.length === 1 ? geos[0] : mergeGeometries(geos, false);
    g.computeBoundingBox();
    return g;
  }, [scene]);

  // SCALE FIX: the glb is modelled in metres (~0.25 m tall) but the fixture
  // positions live in a ~100-unit tree space — so scale 1 = sub-pixel invisible.
  // Normalize the body to a visible fraction of the tree (≈3.5% of treeSize ≈ a
  // housing a bit larger than the LED dot) using the glb's own measured height.
  const scale = useMemo(() => {
    const bb = geom?.boundingBox;
    const h = bb ? Math.max(1e-4, bb.max.y - bb.min.y) : 0.25;
    return (treeSize * 0.035) / h;
  }, [geom, treeSize]);

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
