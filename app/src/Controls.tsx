import { useEffect, useState } from "react";
import { PATTERN_IDS, SEQ_MODES, VIZ_MODES, useTwin, type PatternId, type SeqMode, type VizMode } from "./store";
import { startMic, startFile, startTrack, stopAudio, audioFeatures } from "./audio";

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
  maxHeight: "94vh",
  overflowY: "auto",
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
  const runCommand = useTwin((s) => s.runCommand);
  const runScript = useTwin((s) => s.runScript);
  const cmdLog = useTwin((s) => s.cmdLog);
  const [script, setScript] = useState("");
  const view = useTwin((s) => s.view);
  const setView = useTwin((s) => s.setView);
  const stats = useTwin((s) => s.monitorStats);
  const [cmd, setCmd] = useState("");
  const [bpm, setBpm] = useState(0);
  useEffect(() => {
    const id = setInterval(() => setBpm(audioFeatures.bpm), 250);
    return () => clearInterval(id);
  }, []);

  return (
    <div style={panel}>
      <div style={{ fontWeight: 700, fontSize: 13, color: "#eef3fb" }}>
        Resonance Tree · {count} lights
      </div>
      <div style={{ fontSize: 10, opacity: 0.55, marginTop: 2 }}>{source || "loading…"}</div>

      <div style={row}>
        <button style={btn(ctrl.autoVj)} onClick={() => setCtrl({ autoVj: !ctrl.autoVj })}>
          🤖 auto-VJ
        </button>
        {ctrl.autoVj && (
          <button style={btn(false)} onClick={() => setCtrl({ autoBars: ctrl.autoBars === 8 ? 4 : ctrl.autoBars === 4 ? 16 : 8 })}>
            {ctrl.autoBars} bars
          </button>
        )}
      </div>

      <div style={{ marginTop: 10, opacity: 0.7 }}>visualizer</div>
      <div style={row}>
        {VIZ_MODES.map((v: VizMode) => (
          <button key={v} style={btn(ctrl.visualizer === v)} onClick={() => setCtrl({ visualizer: v })}>
            {v}
          </button>
        ))}
      </div>

      <div style={{ marginTop: 10, opacity: 0.7 }}>pattern</div>
      <div style={row}>
        {PATTERN_IDS.map((p: PatternId) => (
          <button key={p} style={btn(ctrl.pattern === p)} onClick={() => setCtrl({ pattern: p })}>
            {p}
          </button>
        ))}
      </div>

      {ctrl.pattern === "sequence" && (
        <div style={{ margin: "6px 0", padding: 8, background: "#0d141e", borderRadius: 6 }}>
          <div style={{ opacity: 0.7, marginBottom: 4 }}>sequence mode</div>
          <div style={row}>
            {SEQ_MODES.map((m: SeqMode) => (
              <button key={m} style={btn(ctrl.seqMode === m)} onClick={() => setCtrl({ seqMode: m })}>
                {m}
              </button>
            ))}
          </div>
          <Slider label="step (ms)" v={ctrl.stepMs} min={40} max={1000} step={10} on={(v) => setCtrl({ stepMs: v })} />
          <Slider label="group size" v={ctrl.groupSize} min={1} max={78} step={1} on={(v) => setCtrl({ groupSize: Math.round(v) })} />
          <Slider label="every N" v={ctrl.everyN} min={2} max={8} step={1} on={(v) => setCtrl({ everyN: Math.round(v) })} />
          <div style={row}>
            <button style={btn(ctrl.syncToBeat)} onClick={() => setCtrl({ syncToBeat: !ctrl.syncToBeat })}>
              🥁 sync to beat
            </button>
            <button style={btn(ctrl.beatDiv === 2)} onClick={() => setCtrl({ beatDiv: ctrl.beatDiv === 2 ? 1 : 2 })}>
              ½ {ctrl.beatDiv === 2 ? "eighths" : "quarters"}
            </button>
          </div>
        </div>
      )}

      <Slider label="brightness" v={ctrl.brightness} min={0} max={1} on={(v) => setCtrl({ brightness: v })} />
      <Slider label="hue" v={ctrl.hue} min={0} max={1} on={(v) => setCtrl({ hue: v })} />
      <Slider label="saturation" v={ctrl.sat} min={0} max={1} on={(v) => setCtrl({ sat: v })} />
      <Slider label="speed" v={ctrl.speed} min={0} max={3} on={(v) => setCtrl({ speed: v })} />

      <div style={{ marginTop: 10, opacity: 0.7 }}>
        sound → reactive {bpm > 0 && <b style={{ color: "#7fe0a0" }}>· {bpm} BPM</b>}
      </div>
      <div style={row}>
        <button style={btn(false)} onClick={() => startTrack("/audio/test-beat-124bpm.wav").catch(console.error)}>
          🎶 test track
        </button>
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

      <div style={{ marginTop: 10, opacity: 0.7 }}>DJ</div>
      <Slider label="crossfade A↔B" v={ctrl.xfade} min={0} max={1} on={(v) => setCtrl({ xfade: v })} />
      <div style={{ display: "flex", alignItems: "center", gap: 6, margin: "4px 0" }}>
        <span style={{ fontSize: 10, opacity: 0.6 }}>look B</span>
        <select
          value={ctrl.djPatternB}
          onChange={(e) => setCtrl({ djPatternB: e.target.value as PatternId })}
          style={{ flex: 1, background: "#0b1119", color: "#dce6ff", border: "1px solid #2a3a52", borderRadius: 4, padding: "3px", font: "11px ui-monospace, monospace" }}
        >
          {PATTERN_IDS.map((p) => (
            <option key={p} value={p}>{p}</option>
          ))}
        </select>
      </div>
      <Slider label="EQ low→bass" v={ctrl.eqLow} min={0} max={1} on={(v) => setCtrl({ eqLow: v })} />
      <Slider label="EQ mid" v={ctrl.eqMid} min={0} max={1} on={(v) => setCtrl({ eqMid: v })} />
      <Slider label="EQ high→treble" v={ctrl.eqHigh} min={0} max={1} on={(v) => setCtrl({ eqHigh: v })} />
      <Slider label="master" v={ctrl.master} min={0} max={1} on={(v) => setCtrl({ master: v })} />
      <div style={row}>
        <button style={btn(ctrl.strobe)} onClick={() => setCtrl({ strobe: !ctrl.strobe })}>⚡ strobe</button>
      </div>

      <div style={{ marginTop: 10, opacity: 0.7 }}>command console — any light command</div>
      <input
        value={cmd}
        placeholder="range 0-23 color #00aaff"
        onChange={(e) => setCmd(e.target.value)}
        onKeyDown={(e) => {
          if (e.key === "Enter" && cmd.trim()) {
            runCommand(cmd);
            setCmd("");
          }
        }}
        style={{
          width: "100%", marginTop: 4, padding: "6px 8px", borderRadius: 6,
          border: "1px solid #2a3a52", background: "#0b1119", color: "#dce6ff",
          font: "11px ui-monospace, monospace",
        }}
      />
      <div style={row}>
        {["zone high off", "every 4 color red", "all pattern sequence", "clear"].map((ex) => (
          <button key={ex} style={{ ...btn(false), fontSize: 10 }} onClick={() => runCommand(ex)}>
            {ex}
          </button>
        ))}
      </div>
      <div style={{ marginTop: 8, opacity: 0.6, fontSize: 10 }}>LLM script (one command/line)</div>
      <textarea
        value={script}
        placeholder={"all pattern sequence\nzone high color #00aaff\nevery 4 color red"}
        onChange={(e) => setScript(e.target.value)}
        rows={3}
        style={{
          width: "100%", marginTop: 2, padding: "6px 8px", borderRadius: 6, resize: "vertical",
          border: "1px solid #2a3a52", background: "#0b1119", color: "#dce6ff",
          font: "11px ui-monospace, monospace",
        }}
      />
      <div style={row}>
        <button style={btn(false)} onClick={() => script.trim() && runScript(script)}>▶ run script</button>
      </div>

      {cmdLog.length > 0 && (
        <div style={{ marginTop: 4, fontSize: 10, opacity: 0.6, lineHeight: 1.5 }}>
          {cmdLog.map((l, i) => (
            <div key={i}>› {l}</div>
          ))}
        </div>
      )}

      <div style={{ marginTop: 10, opacity: 0.7 }}>truth loop / monitor</div>
      <div style={row}>
        <button style={btn(view.mock)} onClick={() => setView({ mock: !view.mock })}>
          mock heartbeat
        </button>
        <button style={btn(view.monitor)} onClick={() => setView({ monitor: !view.monitor })}>
          monitor
        </button>
      </div>
      {view.mock && (
        <Slider
          label="dead fixtures"
          v={view.deadCount}
          min={0}
          max={30}
          step={1}
          on={(v) => setView({ deadCount: Math.round(v) })}
        />
      )}
      {(view.mock || view.monitor) && (
        <div style={{ fontSize: 10, opacity: 0.7, marginTop: 4 }}>
          reporting <b style={{ color: "#7fe0a0" }}>{stats.reporting}</b> · dead{" "}
          <b style={{ color: "#ff6b6b" }}>{stats.dead}</b> · stale{" "}
          <b style={{ color: "#ffd27f" }}>{stats.stale}</b>
        </div>
      )}
    </div>
  );
}
