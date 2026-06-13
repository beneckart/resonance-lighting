import { OrbitControls, Bounds } from "@react-three/drei";
import { blenderToThree, type FixturesDoc } from "./fixtures";

function Fixtures({ doc, size }: { doc: FixturesDoc; size: number }) {
  return (
    <group>
      {doc.fixtures.map((f) => {
        const [x, y, z] = blenderToThree(f.position);
        const [r, g, b] = f.design_color;
        return (
          <mesh key={f.fixture_id} position={[x, y, z]}>
            <sphereGeometry args={[size, 14, 14]} />
            <meshStandardMaterial
              color={[r, g, b]}
              emissive={[r, g, b]}
              emissiveIntensity={2.4}
              toneMapped={false}
            />
          </mesh>
        );
      })}
    </group>
  );
}

export function Scene({ doc }: { doc: FixturesDoc }) {
  // Size spheres from the FIXTURE cloud's own extent (the full-mesh bbox is much
  // larger and made the points overlap into a solid mass).
  const pts = doc.fixtures.map((f) => blenderToThree(f.position));
  const axisSpread = (i: 0 | 1 | 2) => {
    const v = pts.map((p) => p[i]);
    return Math.max(...v) - Math.min(...v);
  };
  const maxDim = Math.max(axisSpread(0), axisSpread(1), axisSpread(2)) || 10;
  const fixtureSize = maxDim * 0.006;

  return (
    <>
      <color attach="background" args={["#05070a"]} />
      <ambientLight intensity={0.35} />
      {/* The 78 real canopy lights ARE the tree's lit form. Decimated bamboo
          geometry will be layered in as faint context in a later increment (A4). */}
      <Bounds fit clip observe margin={1.2}>
        <Fixtures doc={doc} size={fixtureSize} />
      </Bounds>
      <OrbitControls makeDefault enableDamping />
    </>
  );
}
