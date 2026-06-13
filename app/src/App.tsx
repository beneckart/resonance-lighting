import { Suspense, useEffect, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Scene } from "./Scene";
import { Controls } from "./Controls";
import { DjController } from "./DjController";
import { AiPilot } from "./AiPilot";
import { TouchConsole } from "./TouchConsole";
import { PresenceDriver } from "./PresenceDriver";
import { AutoVj } from "./AutoVjDriver";
import { TimelineDriver } from "./TimelineDriver";
import { loadFixtures, validateFixturesDoc } from "./fixtures";
import { useTwin } from "./store";

export function App() {
  const [err, setErr] = useState<string | null>(null);
  const ready = useTwin((s) => s.fixtures.length > 0);
  const cinematic = useTwin((s) => s.cinematic);
  const setCinematic = useTwin((s) => s.setCinematic);

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
      {!cinematic && (
        <>
          <Controls />
          <DjController />
          <AiPilot />
          <TouchConsole />
        </>
      )}
      {/* always-on cinematic toggle — hide all panels to see just the tree */}
      <button
        onClick={() => setCinematic(!cinematic)}
        title={cinematic ? "show controls" : "hide controls — clean view"}
        style={{
          position: "fixed", top: 12, left: cinematic ? 12 : "auto", right: cinematic ? "auto" : 280,
          zIndex: 60, padding: "7px 11px", borderRadius: 10, cursor: "pointer",
          border: "1px solid #2a3a52", background: "rgba(12,16,24,0.85)", color: "#cdd6e4",
          font: "12px ui-monospace, monospace", backdropFilter: "blur(6px)",
        }}
      >
        {cinematic ? "🎛 controls" : "✨ clean view"}
      </button>
      <PresenceDriver />
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
