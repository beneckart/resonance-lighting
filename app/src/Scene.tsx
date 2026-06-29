import { useEffect, useMemo, useRef, useState } from "react";
import { useThree, useFrame } from "@react-three/fiber";
import { OrbitControls, useGLTF, useTexture } from "@react-three/drei";
import { EffectComposer, Bloom, ToneMapping } from "@react-three/postprocessing";
import { ToneMappingMode } from "postprocessing";
import { Mesh, MeshStandardMaterial, Object3D, SRGBColorSpace, type SpotLight as ThreeSpotLight } from "three";
import { useTwin } from "./store";
import { TreeLights } from "./TreeLights";
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
        {/* visible ground surface at the tree base — was near-black (#0b0e14) and
            blended into the night background; lift it to a dark slate so it reads
            as a floor and catches the gobo projection. */}
        <meshStandardMaterial color="#222a39" roughness={0.95} />
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

/** The central chandelier (ring + wind-chimes) hung at the crown — blender-
 *  architect's chandelier.glb, world-space so it lands at the crown fixtures. */
function Chandelier() {
  const { scene } = useGLTF("/chandelier.glb");
  const fixtures = useTwin((s) => s.fixtures);
  const styled = useMemo(() => {
    const s = scene.clone(true);
    const mat = new MeshStandardMaterial({ color: "#c08a3e", roughness: 0.65, metalness: 0.35, transparent: true, opacity: 0.9 });
    s.traverse((o) => { if ((o as Mesh).isMesh) (o as Mesh).material = mat; });
    return s;
  }, [scene]);
  // The glb is authored centred at its own origin; anchor it on the chandelier
  // fixtures' centroid (three-space) so the mesh lands on the crown lights —
  // robust to the real Blender export swapping in different crown coords.
  const at = useMemo<[number, number, number]>(() => {
    const ch = fixtures.filter((f) => f.role === "chandelier");
    if (ch.length === 0) return [0, 0, 0];
    const sum = ch.reduce((a, f) => [a[0] + f.pos[0], a[1] + f.pos[1], a[2] + f.pos[2]] as [number, number, number], [0, 0, 0] as [number, number, number]);
    return [sum[0] / ch.length, sum[1] / ch.length, sum[2] / ch.length];
  }, [fixtures]);
  return <primitive object={styled} position={at} />;
}

/** PERF: the gobo spotlight's shadow map covers the STATIC scene (tree + ground
 *  never move; the instanced lanterns aren't shadow-casters and their colour
 *  changes don't affect shadows). So render the shadow for the first ~45 frames
 *  (let the async glb + gobo finish loading), then FREEZE it — autoUpdate=false
 *  drops a full re-draw of the 2364-mesh bark scene every single frame. */
function ShadowFreeze() {
  const gl = useThree((s) => s.gl);
  const frame = useRef(0);
  useFrame(() => {
    frame.current += 1;
    if (frame.current === 45) {
      gl.shadowMap.needsUpdate = true; // capture one final, fully-loaded shadow
      gl.shadowMap.autoUpdate = false; // then stop re-rendering it every frame
    }
    // publish a lightweight render-stats snapshot for the HUD + perf verification
    (window as unknown as { __perf?: object }).__perf = {
      calls: gl.info.render.calls,
      triangles: gl.info.render.triangles,
      shadowAutoUpdate: gl.shadowMap.autoUpdate,
      frame: frame.current,
    };
  });
  return null;
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
  const tod = useTwin((s) => s.timeOfDay); // 0 night → 1 day
  // background + ambient ramp with time-of-day so we can preview the install
  // at night (lights pop), dusk, and day (washed — real daytime visibility)
  const bg = ["#04060a", "#0f1422", "#243044", "#5a6e88"][Math.min(3, Math.floor(tod * 3.001))];
  const ambient = 0.16 + tod * 1.0;
  return (
    <>
      <color attach="background" args={[bg]} />
      {/* Darker stage = each fixture's ray reads distinctly, less cross-bleed
          (light pollution); ambient ramps up toward day. */}
      <ambientLight intensity={ambient} />
      <directionalLight position={[1, 1.6, 1]} intensity={0.85 + tod * 1.2} color="#fff1d8" />
      <directionalLight position={[-1.2, 0.4, -1]} intensity={0.32 + tod * 0.4} color="#4a63b0" />
      <CameraRig />
      <ShadowFreeze />
      <ErrorBoundary>
        <GoboFloor />
      </ErrorBoundary>
      {!LIGHT_SCENE && (
        <ErrorBoundary>
          <TreeContext />
        </ErrorBoundary>
      )}
      {!LIGHT_SCENE && (
        <ErrorBoundary>
          <Chandelier />
        </ErrorBoundary>
      )}
      {/* lantern bodies now render inside TreeLights (the lit fixture itself) */}
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
