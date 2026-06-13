import { useEffect } from "react";
import { useThree } from "@react-three/fiber";
import { OrbitControls } from "@react-three/drei";
import { useTwin } from "./store";
import { TreeLights } from "./TreeLights";

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
      <CameraRig />
      <TreeLights />
      <OrbitControls makeDefault enableDamping />
    </>
  );
}
