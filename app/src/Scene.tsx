import { useEffect, useMemo, useRef, useState } from "react";
import { useThree, useFrame } from "@react-three/fiber";
import { OrbitControls, useGLTF, useTexture } from "@react-three/drei";
import { EffectComposer, Bloom, ToneMapping } from "@react-three/postprocessing";
import { ToneMappingMode } from "postprocessing";
import { Mesh, MeshStandardMaterial, Object3D, SRGBColorSpace, type SpotLight as ThreeSpotLight } from "three";
import { useTwin } from "./store";
import { TreeLights } from "./TreeLights";
import { LanternBodies } from "./LanternBodies";
import { ErrorBoundary } from "./ErrorBoundary";
import { groundTint } from "./groundtint";
import { parseIES } from "./ies";

/** Ground plane + a downward spotlight projecting the mandala gobo onto it (A5). */
function GoboFloor() {
  const center = useTwin((s) => s.center);
  const size = useTwin((s) => s.size);
  const gobo = useTexture("/gobo.png"); // real skirt-petal projection (blender-architect bake)
  gobo.colorSpace = SRGBColorSpace;
  const light = useRef<ThreeSpotLight>(null);
  const target = useMemo(() => new Object3D(), []);
  const groundY = center[1] - size * 0.5;
  // cone geometry from the real baked IES photometric profile
  const [cone, setCone] = useState({ angle: 0.62, penumbra: 0.5 });

  useEffect(() => {
    let alive = true;
    fetch("/downlight.ies")
      .then((r) => r.text())
      .then((t) => {
        const p = parseIES(t);
        const DEG = Math.PI / 180;
        const fieldHalf = (p.fieldDeg / 2) * DEG;
        const beamHalf = (p.beamDeg / 2) * DEG;
        // crisp cookie so the ~15 petal shapes resolve (penumbra down)
        const penumbra = Math.min(0.4, Math.max(0.04, ((fieldHalf - beamHalf) / Math.max(fieldHalf, 1e-3)) * 0.5));
        if (alive) setCone({ angle: Math.min(Math.PI / 2 - 0.01, fieldHalf), penumbra });
      })
      .catch(() => {});
    return () => { alive = false; };
  }, []);

  useEffect(() => {
    target.position.set(center[0], groundY, center[2]);
    target.updateMatrixWorld();
    if (light.current) light.current.target = target;
  }, [center, size, groundY, target]);

  // tint + brighten the floor projection from the tree's live aggregate colour
  useFrame(() => {
    const l = light.current;
    if (!l) return;
    l.color.setRGB(groundTint.r, groundTint.g, groundTint.b);
    l.intensity = 0.9 + 2.4 * groundTint.level;
  });

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
        angle={cone.angle}
        penumbra={cone.penumbra}
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

/** Visible structural bamboo + Plu Plu bark so the lights read as the Resonance
 *  Tree. The bark ("treev4 Plupu") is the light-shaping SHELL — render it darker
 *  and near-opaque so the lantern glow leaks through it; the bamboo structure is
 *  lighter + more translucent. */
function TreeContext() {
  const { scene } = useGLTF("/tree-context.glb");
  const styled = useMemo(() => {
    const s = scene.clone(true);
    const bamboo = new MeshStandardMaterial({
      color: "#9c7a44", roughness: 0.82, metalness: 0, transparent: true, opacity: 0.7,
    });
    const bark = new MeshStandardMaterial({
      color: "#6b5230", roughness: 0.95, metalness: 0, transparent: true, opacity: 0.96,
    });
    s.traverse((o) => {
      const m = o as Mesh;
      if (!m.isMesh) return;
      const nm = (m.name || m.parent?.name || "").toLowerCase();
      m.material = nm.includes("plupu") ? bark : bamboo;
    });
    return s;
  }, [scene]);
  return <primitive object={styled} />;
}

/** Frame the whole tree at a hero 3/4 angle. */
function CameraRig() {
  const center = useTwin((s) => s.center);
  const size = useTwin((s) => s.size);
  const preset = useTwin((s) => s.cameraPreset);
  const camera = useThree((s) => s.camera);
  const controls = useThree((s) => s.controls) as
    | { target?: { set: (x: number, y: number, z: number) => void }; update?: () => void }
    | null;

  useEffect(() => {
    const d = size * 0.92;
    camera.near = Math.max(0.1, size * 0.01);
    camera.far = size * 30;
    if (preset === "top") {
      // straight-down projection view — see the petal gobo pattern on the floor
      camera.position.set(center[0], center[1] + d * 1.5, center[2] + 0.001);
    } else {
      camera.position.set(center[0] + d * 0.75, center[1] + d * 0.3, center[2] + d);
    }
    camera.updateProjectionMatrix();
    camera.lookAt(center[0], center[1], center[2]);
    if (controls?.target) {
      controls.target.set(center[0], center[1], center[2]);
      controls.update?.();
    }
  }, [center, size, camera, controls, preset]);
  return null;
}

/** e2e/perf flag: ?e2e skips the heavy 22MB bark context glb so the
 *  interaction tests run on a light scene (headless GL can't hold the full mesh
 *  under intensive clicking). The real app always renders the full tree. */
const LIGHT_SCENE = typeof location !== "undefined" && new URLSearchParams(location.search).has("e2e");

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
      {!LIGHT_SCENE && (
        <ErrorBoundary>
          <TreeContext />
        </ErrorBoundary>
      )}
      <ErrorBoundary>
        <LanternBodies />
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
