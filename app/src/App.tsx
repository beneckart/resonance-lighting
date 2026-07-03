import { Suspense, useEffect, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Scene } from "./Scene";
import { Controls } from "./Controls";
import { DjController } from "./DjController";
import { AiPilot } from "./AiPilot";
import { TouchConsole } from "./TouchConsole";
import { CommissioningPanel } from "./CommissioningPanel";
import { GroupPanel } from "./GroupPanel";
import { DataLog } from "./DataLog";
import { ShowsPanel } from "./ShowsPanel";
import { InteractivityPanel } from "./InteractivityPanel";
import { ShowPlayer } from "./ShowPlayer";
import { RecordButton } from "./RecordButton";
import { HealthHud } from "./HealthHud";
import { PresenceDriver } from "./PresenceDriver";
import { IgnitionDriver } from "./IgnitionDriver";
import { AudioReactiveDriver } from "./AudioReactiveDriver";
import { AutoVj } from "./AutoVjDriver";
import { TimelineDriver } from "./TimelineDriver";
import { loadFixtures, validateFixturesDoc, auditFixtures } from "./fixtures";
import { useTwin } from "./store";

export function App() {
  const [err, setErr] = useState<string | null>(null);
  const ready = useTwin((s) => s.fixtures.length > 0);
  const cinematic = useTwin((s) => s.cinematic);
  const setCinematic = useTwin((s) => s.setCinematic);
  const beacon = useTwin((s) => s.control.beaconPreempt);
  const blackout = useTwin((s) => s.control.blackout);
  const setCtrl = useTwin((s) => s.set);

  useEffect(() => {
    loadFixtures()
      .then((doc) => {
        const v = validateFixturesDoc(doc);
        if (!v.ok) setErr(`fixtures.json invalid: ${v.errors.slice(0, 3).join("; ")}`);
        // data-quality audit (role/zone counts + aim sanity) — surface anomalies
        const a = auditFixtures(doc);
        console.info(`[fixtures] ${doc.fixtures.length} loaded · roles`, a.byRole, "· zones", a.byZone, `· ${a.withAim} with aim`);
        if (a.warnings.length) console.warn(`[fixtures] ${a.warnings.length} aim anomalies:`, a.warnings.slice(0, 8));
        useTwin.getState().init(doc);
      })
      .catch((e) => setErr(String(e)));
  }, []);

  return (
    <div style={{ position: "fixed", inset: 0 }}>
      <Canvas
        shadows
        // PERF: cap devicePixelRatio (a 2-3x HiDPI panel would otherwise fill
        // 4-9x the pixels) + antialias off — Bloom hides the edges, and the
        // pixel-fill saving dwarfs the AA cost on this fragment-heavy scene.
        dpr={[1, 1.5]}
        camera={{ position: [40, 30, 60], fov: 45, near: 0.1, far: 5000 }}
        gl={{ antialias: false, preserveDrawingBuffer: true, powerPreference: "high-performance" }}
      >
        <Suspense fallback={null}>{ready && <Scene />}</Suspense>
      </Canvas>
      {!cinematic && (
        <>
          <Controls />
          <GroupPanel />
          <ShowsPanel />
          <InteractivityPanel />
          <DataLog />
          <DjController />
          <AiPilot />
          <TouchConsole />
          <CommissioningPanel />
          <HealthHud />
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
      {/* always-available BEACON safety preempt (reachable even in clean view) */}
      <button
        onClick={() => setCtrl({ beaconPreempt: !beacon })}
        title="BEACON — force full-white safety beam over everything"
        style={{
          position: "fixed", bottom: 14, left: "50%", transform: "translateX(-50%)", zIndex: 60,
          padding: "8px 16px", borderRadius: 12, cursor: "pointer", fontWeight: 700, letterSpacing: 0.5,
          border: beacon ? "1.5px solid #fff" : "1.5px solid #5a3a3a",
          background: beacon ? "#ffffff" : "rgba(40,16,16,0.85)", color: beacon ? "#111" : "#ffb4b4",
          boxShadow: beacon ? "0 0 22px #ffffffcc" : "none", font: "13px ui-monospace, monospace", backdropFilter: "blur(6px)",
        }}
      >
        🔦 BEACON{beacon ? " ON" : ""}
      </button>
      {/* always-available BLACKOUT (instant all-off, pairs with BEACON) */}
      <button
        onClick={() => setCtrl({ blackout: !blackout })}
        title="BLACKOUT — force all fixtures off instantly"
        style={{
          position: "fixed", bottom: 14, left: "calc(50% + 110px)", zIndex: 60,
          padding: "8px 14px", borderRadius: 12, cursor: "pointer", fontWeight: 700, letterSpacing: 0.5,
          border: blackout ? "1.5px solid #ff5b6e" : "1.5px solid #3a3a4a",
          background: blackout ? "#1a1020" : "rgba(16,16,24,0.85)", color: blackout ? "#ff8fa0" : "#8a8aa0",
          boxShadow: blackout ? "0 0 18px #ff5b6e88" : "none", font: "13px ui-monospace, monospace", backdropFilter: "blur(6px)",
        }}
      >
        🌑 BLACKOUT{blackout ? " ON" : ""}
      </button>
      <ShowPlayer />
      <RecordButton />
      <PresenceDriver />
      <IgnitionDriver />
      <AudioReactiveDriver />
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
