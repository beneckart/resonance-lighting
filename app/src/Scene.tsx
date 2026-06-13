import { useEffect, useMemo } from "react";
import { useThree } from "@react-three/fiber";
import { OrbitControls, useGLTF } from "@react-three/drei";
import { EffectComposer, Bloom } from "@react-three/postprocessing";
import { Mesh, MeshStandardMaterial } from "three";
import { useTwin } from "./store";
import { TreeLights } from "./TreeLights";

/** Visible structural bamboo so the lights read as the Resonance Tree. */
function TreeContext() {
  const { scene } = useGLTF("/tree-context.glb");
  const styled = useMemo(() => {
    const s = scene.clone(true);
    const mat = new MeshStandardMaterial({
      color: "#9c7a44",
      roughness: 0.82,
      metalness: 0.0,
      transparent: true,
      opacity: 0.85,
    });
    s.traverse((o) => {
      if ((o as Mesh).isMesh) (o as Mesh).material = mat;
    });
    return s;
  }, [scene]);
  return <primitive object={styled} />;
}

/** Frame the whole tree at a hero 3/4 angle. */
function CameraRig() {
  const center = useTwin((s) => s.center);
  const size = useTwin((s) => s.size);
  const camera = useThree((s) => s.camera);
  const controls = useThree((s) => s.controls) as
    | { target?: { set: (x: number, y: number, z: number) => void }; update?: () => void }
    | null;

  useEffect(() => {
    const d = size * 0.92;
    camera.position.set(center[0] + d * 0.75, center[1] + d * 0.3, center[2] + d);
    camera.near = Math.max(0.1, size * 0.01);
    camera.far = size * 30;
    camera.updateProjectionMatrix();
    camera.lookAt(center[0], center[1], center[2]);
    if (controls?.target) {
      controls.target.set(center[0], center[1], center[2]);
      controls.update?.();
    }
  }, [center, size, camera, controls]);
  return null;
}

export function Scene() {
  return (
    <>
      <color attach="background" args={["#04060a"]} />
      <ambientLight intensity={0.35} />
      <directionalLight position={[1, 1.6, 1]} intensity={1.2} color="#fff1d8" />
      <directionalLight position={[-1.2, 0.4, -1]} intensity={0.55} color="#4a63b0" />
      <CameraRig />
      <TreeContext />
      <TreeLights />
      <OrbitControls makeDefault enableDamping />
      <EffectComposer>
        <Bloom intensity={1.15} luminanceThreshold={0.35} luminanceSmoothing={0.5} mipmapBlur radius={0.8} />
      </EffectComposer>
    </>
  );
}

useGLTF.preload("/tree-context.glb");
