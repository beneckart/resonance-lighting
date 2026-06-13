import { PATTERN_IDS, useTwin, type PatternId } from "./store";
import { startMic, startFile, stopAudio } from "./audio";

const panel: React.CSSProperties = {
  position: "fixed",
  top: 12,
  left: 12,
  width: 260,
  padding: "12px 14px",
  background: "rgba(10,14,20,0.82)",
  border: "1px solid #1d2735",
  borderRadius: 10,
  color: "#cdd6e4",
  font: "12px ui-monospace, SFMono-Regular, monospace",
  backdropFilter: "blur(6px)",
};
const row: React.CSSProperties = { display: "flex", flexWrap: "wrap", gap: 4, margin: "8px 0" };

function btn(active: boolean): React.CSSProperties {
  return {
    flex: "1 0 auto",
    padding: "5px 8px",
    borderRadius: 6,
    cursor: "pointer",
    border: active ? "1px solid #5b8cff" : "1px solid #2a3a52",
    background: active ? "#21345e" : "#121a26",
    color: active ? "#dce6ff" : "#9fb0c7",
    textAlign: "center",
  };
}

function Slider({ label, v, min, max, step, on }: {
  label: string; v: number; min: number; max: number; step?: number; on: (v: number) => void;
}) {
  return (
    <label style={{ display: "block", margin: "6px 0" }}>
      <span style={{ display: "flex", justifyContent: "space-between", opacity: 0.8 }}>
        <span>{label}</span>
        <span>{v.toFixed(2)}</span>
      </span>
      <input
        type="range"
        min={min}
        max={max}
        step={step ?? 0.01}
        value={v}
        onChange={(e) => on(parseFloat(e.target.value))}
        style={{ width: "100%" }}
      />
    </label>
  );
}

export function Controls() {
  const ctrl = useTwin((s) => s.control);
  const setCtrl = useTwin((s) => s.set);
  const count = useTwin((s) => s.fixtures.length);
  const source = useTwin((s) => s.source);

  return (
    <div style={panel}>
      <div style={{ fontWeight: 700, fontSize: 13, color: "#eef3fb" }}>
        Resonance Tree · {count} lights
      </div>
      <div style={{ fontSize: 10, opacity: 0.55, marginTop: 2 }}>{source || "loading…"}</div>

      <div style={{ marginTop: 10, opacity: 0.7 }}>pattern</div>
      <div style={row}>
        {PATTERN_IDS.map((p: PatternId) => (
          <button key={p} style={btn(ctrl.pattern === p)} onClick={() => setCtrl({ pattern: p })}>
            {p}
          </button>
        ))}
      </div>

      <Slider label="brightness" v={ctrl.brightness} min={0} max={1} on={(v) => setCtrl({ brightness: v })} />
      <Slider label="hue" v={ctrl.hue} min={0} max={1} on={(v) => setCtrl({ hue: v })} />
      <Slider label="saturation" v={ctrl.sat} min={0} max={1} on={(v) => setCtrl({ sat: v })} />
      <Slider label="speed" v={ctrl.speed} min={0} max={3} on={(v) => setCtrl({ speed: v })} />

      <div style={{ marginTop: 10, opacity: 0.7 }}>sound → reactive</div>
      <div style={row}>
        <button style={btn(false)} onClick={() => startMic().catch(console.error)}>🎤 mic</button>
        <label style={btn(false)}>
          🎵 song
          <input
            type="file"
            accept="audio/*"
            style={{ display: "none" }}
            onChange={(e) => {
              const f = e.target.files?.[0];
              if (f) startFile(f).catch(console.error);
            }}
          />
        </label>
        <button style={btn(false)} onClick={() => stopAudio()}>stop</button>
      </div>
    </div>
  );
}
