import { Suspense, useEffect, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Scene } from "./Scene";
import { Controls } from "./Controls";
import { DjController } from "./DjController";
import { AiPilot } from "./AiPilot";
import { AutoVj } from "./AutoVjDriver";
import { TimelineDriver } from "./TimelineDriver";
import { loadFixtures, validateFixturesDoc } from "./fixtures";
import { useTwin } from "./store";

export function App() {
  const [err, setErr] = useState<string | null>(null);
  const ready = useTwin((s) => s.fixtures.length > 0);

  useEffect(() => {
    loadFixtures()
      .then((doc) => {
        const v = validateFixturesDoc(doc);
        if (!v.ok) setErr(`fixtures.json invalid: ${v.errors.slice(0, 3).join("; ")}`);
        useTwin.getState().init(doc);
      })
      .catch((e) => setErr(String(e)));
  }, []);

  return (
    <div style={{ position: "fixed", inset: 0 }}>
      <Canvas
        shadows
        camera={{ position: [40, 30, 60], fov: 45, near: 0.1, far: 5000 }}
        gl={{ antialias: true, preserveDrawingBuffer: true }}
      >
        <Suspense fallback={null}>{ready && <Scene />}</Suspense>
      </Canvas>
      <Controls />
      <DjController />
      <AiPilot />
      <AutoVj />
      <TimelineDriver />
      {err && (
        <div style={{ position: "fixed", bottom: 12, left: 12, color: "#ff6b6b", font: "12px monospace" }}>
          ERROR: {err}
        </div>
      )}
    </div>
  );
}
