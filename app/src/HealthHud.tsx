import { useEffect, useRef, useState } from "react";
import { useTwin } from "./store";

/** Show-health HUD (Tier-2 ops): live fps + the mirror's fixture health —
 *  reporting / dead / stale counts (from monitorStats, populated by the
 *  mock-heartbeat truth loop) so an operator sees the rig's state at a glance.
 *  Diagnostic, so it hides in cinematic/clean view. */
export function HealthHud() {
  const stats = useTwin((s) => s.monitorStats);
  const total = useTwin((s) => s.fixtures.length);
  const mock = useTwin((s) => s.view.mock);
  const [fps, setFps] = useState(0);
  const [calls, setCalls] = useState(0);
  const frames = useRef(0);
  const t0 = useRef(performance.now());

  useEffect(() => {
    let raf = 0;
    const loop = () => {
      frames.current++;
      const now = performance.now();
      if (now - t0.current >= 500) {
        setFps(Math.round((frames.current * 1000) / (now - t0.current)));
        frames.current = 0;
        t0.current = now;
        // GPU draw-calls/frame from the renderer-info snapshot (Scene/ShadowFreeze)
        const perf = (window as unknown as { __perf?: { calls?: number } }).__perf;
        if (perf?.calls != null) setCalls(perf.calls);
      }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, []);

  const fpsColor = fps >= 50 ? "#3ddc97" : fps >= 30 ? "#ffd166" : "#ff5b6e";
  return (
    <div style={{
      position: "fixed", top: 12, left: "50%", transform: "translateX(-50%)", zIndex: 14,
      display: "flex", gap: 10, alignItems: "center", padding: "5px 12px", borderRadius: 999,
      background: "rgba(10,14,20,0.82)", border: "1px solid #1d2735", color: "#9fb0c7",
      font: "11px ui-monospace, SFMono-Regular, monospace", backdropFilter: "blur(6px)",
    }}>
      <span style={{ color: fpsColor }}>{fps} fps</span>
      {calls > 0 && <><span style={{ opacity: 0.4 }}>·</span><span style={{ color: "#9fb0c7" }}>{calls} calls</span></>}
      <span style={{ opacity: 0.4 }}>·</span>
      <span style={{ color: "#3ddc97" }}>{mock ? stats.reporting : total}/{total} lit</span>
      {mock && stats.dead > 0 && <span style={{ color: "#ff5b6e" }}>{stats.dead}✗</span>}
      {mock && stats.stale > 0 && <span style={{ color: "#ffd166" }}>{stats.stale}~ stale</span>}
    </div>
  );
}
