import { Suspense, useEffect, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Scene } from "./Scene";
import { loadFixtures, type FixturesDoc } from "./fixtures";

export function App() {
  const [doc, setDoc] = useState<FixturesDoc | null>(null);
  const [err, setErr] = useState<string | null>(null);

  useEffect(() => {
    loadFixtures()
      .then(setDoc)
      .catch((e) => setErr(String(e)));
  }, []);

  return (
    <div style={{ position: "fixed", inset: 0 }}>
      <Canvas
        camera={{ position: [40, 30, 60], fov: 45, near: 0.1, far: 5000 }}
        gl={{ antialias: true, preserveDrawingBuffer: true }}
      >
        <Suspense fallback={null}>{doc && <Scene doc={doc} />}</Suspense>
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
        Resonance Tree — mirror twin
        {doc && ` · ${doc.meta.count} fixtures · ${doc.meta.source.split(":")[1] ?? ""}`}
        {err && ` · ERROR: ${err}`}
      </div>
    </div>
  );
}
