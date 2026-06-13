import { useEffect, useMemo, useRef } from "react";
import { useThree } from "@react-three/fiber";
import { OrbitControls, useGLTF, useTexture } from "@react-three/drei";
import { EffectComposer, Bloom, ToneMapping } from "@react-three/postprocessing";
import { ToneMappingMode } from "postprocessing";
import { Mesh, MeshStandardMaterial, Object3D, SRGBColorSpace, type SpotLight as ThreeSpotLight } from "three";
import { useTwin } from "./store";
import { TreeLights } from "./TreeLights";
import { ErrorBoundary } from "./ErrorBoundary";

/** Ground plane + a downward spotlight projecting the mandala gobo onto it (A5). */
function GoboFloor() {
  const center = useTwin((s) => s.center);
  const size = useTwin((s) => s.size);
  const gobo = useTexture("/gobo-mandala.png");
  gobo.colorSpace = SRGBColorSpace;
  const light = useRef<ThreeSpotLight>(null);
  const target = useMemo(() => new Object3D(), []);
  const groundY = center[1] - size * 0.5;

  useEffect(() => {
    target.position.set(center[0], groundY, center[2]);
    target.updateMatrixWorld();
    if (light.current) light.current.target = target;
  }, [center, size, groundY, target]);

  return (
    <>
      <mesh rotation-x={-Math.PI / 2} position={[center[0], groundY, center[2]]} receiveShadow>
        <planeGeometry args={[size * 2.8, size * 2.8]} />
        <meshStandardMaterial color="#0b0e14" roughness={1} />
      </mesh>
      <primitive object={target} />
      <spotLight
        ref={light}
        position={[center[0], groundY + size * 1.15, center[2]]}
        angle={0.62}
        penumbra={0.5}
        intensity={1.7}
        decay={0}
        distance={0}
        castShadow
        map={gobo}
        color="#ffe1b0"
        shadow-mapSize={[1024, 1024]}
      />
    </>
  );
}

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
      {/* Darker stage = each fixture's ray reads distinctly, less cross-bleed
          (light pollution) from neighbour to neighbour. */}
      <ambientLight intensity={0.16} />
      <directionalLight position={[1, 1.6, 1]} intensity={0.85} color="#fff1d8" />
      <directionalLight position={[-1.2, 0.4, -1]} intensity={0.32} color="#4a63b0" />
      <CameraRig />
      <ErrorBoundary>
        <GoboFloor />
      </ErrorBoundary>
      <ErrorBoundary>
        <TreeContext />
      </ErrorBoundary>
      <TreeLights />
      <OrbitControls makeDefault enableDamping />
      <EffectComposer>
        <Bloom intensity={0.85} luminanceThreshold={0.42} luminanceSmoothing={0.5} mipmapBlur radius={0.8} />
        <ToneMapping mode={ToneMappingMode.ACES_FILMIC} />
      </EffectComposer>
    </>
  );
}

useGLTF.preload("/tree-context.glb");
