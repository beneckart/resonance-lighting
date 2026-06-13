import { Canvas } from "@react-three/fiber";
import { OrbitControls, Grid } from "@react-three/drei";

/**
 * Cycle 1 scaffold (A1): a live R3F canvas with OrbitControls + a ground grid and a
 * reference object. Proves the render pipeline works end to end. The real tree + the
 * mirror sim land in later increments.
 */
export function App() {
  return (
    <div style={{ position: "fixed", inset: 0 }}>
      <Canvas
        camera={{ position: [6, 5, 8], fov: 50 }}
        gl={{ antialias: true, preserveDrawingBuffer: true }}
      >
        <color attach="background" args={["#07090c"]} />
        <ambientLight intensity={0.3} />
        <directionalLight position={[5, 10, 5]} intensity={0.8} />
        <mesh position={[0, 1, 0]} castShadow>
          <icosahedronGeometry args={[1, 1]} />
          <meshStandardMaterial color="#ffb347" emissive="#ff7b00" emissiveIntensity={0.6} />
        </mesh>
        <Grid
          args={[20, 20]}
          cellColor="#1a2433"
          sectionColor="#2a3a52"
          fadeDistance={30}
          infiniteGrid
        />
        <OrbitControls makeDefault />
      </Canvas>
      <div
        style={{
          position: "fixed",
          top: 10,
          left: 12,
          color: "#7d8ca3",
          font: "12px ui-monospace, monospace",
          pointerEvents: "none",
        }}
      >
        Resonance Tree — mirror twin · scaffold
      </div>
    </div>
  );
}
