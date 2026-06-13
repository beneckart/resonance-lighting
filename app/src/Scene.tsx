import { useEffect, useMemo } from "react";
import { useThree } from "@react-three/fiber";
import { OrbitControls, useGLTF } from "@react-three/drei";
import { Mesh, MeshStandardMaterial } from "three";
import { useTwin } from "./store";
import { TreeLights } from "./TreeLights";

/** Faint structural backdrop (decimated bamboo) so the lights read as a tree.
 *  Dim + depthWrite off so it never occludes the fixtures. */
function TreeContext() {
  const { scene } = useGLTF("/tree-context.glb");
  const styled = useMemo(() => {
    const s = scene.clone(true);
    const mat = new MeshStandardMaterial({
      color: "#26303f",
      roughness: 1,
      metalness: 0,
      transparent: true,
      opacity: 0.22,
      depthWrite: false,
    });
    s.traverse((o) => {
      if ((o as Mesh).isMesh) (o as Mesh).material = mat;
    });
    return s;
  }, [scene]);
  return <primitive object={styled} />;
}

/** Frame the camera + orbit target on the fixture cloud once it loads. */
function CameraRig() {
  const center = useTwin((s) => s.center);
  const size = useTwin((s) => s.size);
  const camera = useThree((s) => s.camera);
  const controls = useThree((s) => s.controls) as { target?: { set: (x: number, y: number, z: number) => void }; update?: () => void } | null;

  useEffect(() => {
    camera.position.set(center[0] + size * 0.9, center[1] + size * 0.45, center[2] + size * 1.4);
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
      <color attach="background" args={["#05070a"]} />
      <ambientLight intensity={0.6} />
      <CameraRig />
      <TreeContext />
      <TreeLights />
      <OrbitControls makeDefault enableDamping />
    </>
  );
}
