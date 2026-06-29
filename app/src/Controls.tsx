import { useEffect, useState } from "react";
import { PATTERN_IDS, ELEMENT_MODES, SEQ_MODES, VIZ_MODES, COLOR_CYCLES, useTwin, type PatternId, type SeqMode, type VizMode, type ColorCycle } from "./store";
import { startMic, startFile, startTrack, stopAudio, audioFeatures } from "./audio";
import { compileShow, showToJson } from "./showcompiler";
import { interpret } from "./llm";
import { encodeFixture } from "./protocol";
import { autoBalanceGain } from "./sensors";
import { startMidi, ccToControl } from "./midi";

const panel: React.CSSProperties = {
  position: "fixed",
  top: 12,
  left: 12,
  width: 320,
  padding: "14px 16px",
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

// base-colour swatches — set the show's hue/saturation, available with ANY mode
const SWATCHES: { name: string; hue: number; sat: number }[] = [
  { name: "red", hue: 0.0, sat: 1 },
  { name: "orange", hue: 0.07, sat: 1 },
  { name: "yellow", hue: 0.15, sat: 1 },
  { name: "green", hue: 0.33, sat: 1 },
  { name: "cyan", hue: 0.5, sat: 1 },
  { name: "blue", hue: 0.62, sat: 1 },
  { name: "purple", hue: 0.78, sat: 1 },
  { name: "pink", hue: 0.9, sat: 0.85 },
  { name: "white", hue: 0, sat: 0 },
];

// native colour-picker hex → the store's hue (0..1) + saturation (0..1)
function hexToHueSat(hex: string): { hue: number; sat: number } {
  const r = parseInt(hex.slice(1, 3), 16) / 255;
  const g = parseInt(hex.slice(3, 5), 16) / 255;
  const b = parseInt(hex.slice(5, 7), 16) / 255;
  const max = Math.max(r, g, b), min = Math.min(r, g, b), d = max - min;
  let h = 0;
  if (d !== 0) {
    if (max === r) h = ((g - b) / d) % 6;
    else if (max === g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h /= 6;
    if (h < 0) h += 1;
  }
  return { hue: h, sat: max === 0 ? 0 : d / max };
}

export function Controls() {
  const ctrl = useTwin((s) => s.control);
  const setCtrl = useTwin((s) => s.set);
  const count = useTwin((s) => s.fixtures.length);
  const fixtures = useTwin((s) => s.fixtures);
  const source = useTwin((s) => s.source);
  const runCommand = useTwin((s) => s.runCommand);
  const runScript = useTwin((s) => s.runScript);
  const cmdLog = useTwin((s) => s.cmdLog);
  const [script, setScript] = useState("");
  const view = useTwin((s) => s.view);
  const setView = useTwin((s) => s.setView);
  const stats = useTwin((s) => s.monitorStats);
  const net = useTwin((s) => s.net);
  const setNet = useTwin((s) => s.setNet);
  const cues = useTwin((s) => s.cues);
  const addCue = useTwin((s) => s.addCue);
  const recallCue = useTwin((s) => s.recallCue);
  const deleteCue = useTwin((s) => s.deleteCue);
  const timeline = useTwin((s) => s.timeline);
  const setTimeline = useTwin((s) => s.setTimeline);
  const pingPresence = useTwin((s) => s.pingPresence);
  const guest = useTwin((s) => s.guest);
  const setGuest = useTwin((s) => s.setGuest);
  const sensors = useTwin((s) => s.sensors);
  const setSensors = useTwin((s) => s.setSensors);
  const cameraPreset = useTwin((s) => s.cameraPreset);
  const setCameraPreset = useTwin((s) => s.setCameraPreset);
  const timeOfDay = useTwin((s) => s.timeOfDay);
  const setTimeOfDay = useTwin((s) => s.setTimeOfDay);
  const [cueName, setCueName] = useState("");
  const [midi, setMidi] = useState("");
  const [nl, setNl] = useState("");
  const [nlNote, setNlNote] = useState("");

  const connectMidi = () =>
    startMidi(
      (cc, value) => {
        const p = ccToControl(cc, value);
        if (p) useTwin.getState().set(p);
      },
      (note) => {
        const cs = useTwin.getState().cues;
        if (cs.length) useTwin.getState().recallCue(cs[note % cs.length].id);
      }
    ).then(setMidi);
  const f0 = useTwin((s) => s.fixtures[0]);
  const ov0 = useTwin((s) => s.overrides[0]);
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

      {/* PROMINENT speed dial — the one control people reach for most: how fast
          the whole show moves. Lives at the top so it's always one tap away. */}
      <div style={{ margin: "10px 0 4px", padding: "8px 10px", background: "#101a28", borderRadius: 8, border: "1px solid #2a3a52" }}>
        <Slider label="⚡ SPEED — how fast the lights move" v={ctrl.speed} min={0} max={3} on={(v) => setCtrl({ speed: v })} />
      </div>

      {/* COLOR — a base colour available with ANY mode/pattern. Most patterns
          build their palette off this hue (solid uses it directly; rainbow modes
          like spectrum override it). Swatches + a custom picker. */}
      <div style={{ margin: "8px 0 4px", padding: "8px 10px", background: "#101a28", borderRadius: 8, border: "1px solid #2a3a52" }}>
        <div style={{ opacity: 0.8, marginBottom: 6 }}>🎨 COLOR — works with any mode</div>
        <div style={{ display: "flex", gap: 5, flexWrap: "wrap", alignItems: "center" }}>
          {SWATCHES.map((s) => {
            const sel = Math.abs(ctrl.hue - s.hue) < 0.02 && Math.abs(ctrl.sat - s.sat) < 0.06;
            return (
              <button
                key={s.name}
                title={s.name}
                onClick={() => setCtrl({ hue: s.hue, sat: s.sat })}
                style={{
                  width: 26, height: 26, borderRadius: "50%", cursor: "pointer", padding: 0,
                  background: s.sat === 0 ? "#f4f6fb" : `hsl(${s.hue * 360},85%,55%)`,
                  border: sel ? "2px solid #fff" : "1px solid #2a3a52",
                  boxShadow: sel ? "0 0 7px rgba(255,255,255,0.5)" : "none",
                }}
              />
            );
          })}
          <input
            type="color"
            title="custom colour"
            onChange={(e) => { const { hue, sat } = hexToHueSat(e.target.value); setCtrl({ hue, sat }); }}
            style={{ width: 32, height: 26, padding: 0, border: "1px solid #2a3a52", borderRadius: 6, background: "#0b1119", cursor: "pointer" }}
          />
        </div>
        {/* colour MOTION on top of the picked colour — works with any pattern */}
        <div style={{ ...row, marginBottom: 0 }}>
          {COLOR_CYCLES.map((m: ColorCycle) => (
            <button key={m} style={{ ...btn(ctrl.colorCycle === m), fontSize: 10 }} onClick={() => setCtrl({ colorCycle: m })}
              title={m === "off" ? "hold the picked colour" : m === "rainbow" ? "sweep through ALL colours" : m === "group" ? "drift through the adjacent family (warm/cool)" : "drift through the shades of the picked colour"}>
              {m === "off" ? "● hold" : m === "rainbow" ? "🌈 rainbow" : m === "group" ? "family" : "shades"}
            </button>
          ))}
        </div>
        <Slider label="hue" v={ctrl.hue} min={0} max={1} on={(v) => setCtrl({ hue: v })} />
        <Slider label="saturation" v={ctrl.sat} min={0} max={1} on={(v) => setCtrl({ sat: v })} />
      </div>

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

      <div style={{ marginTop: 10, opacity: 0.7 }}>camera</div>
      <div style={row}>
        <button style={btn(cameraPreset === "hero")} onClick={() => setCameraPreset("hero")}>hero 3/4</button>
        <button style={btn(cameraPreset === "top")} onClick={() => setCameraPreset("top")}>⬇ top-down (petals)</button>
      </div>
      <div style={{ marginTop: 8, opacity: 0.7 }}>time of day</div>
      <div style={row}>
        <button style={btn(timeOfDay < 0.25)} onClick={() => setTimeOfDay(0)}>🌙 night</button>
        <button style={btn(timeOfDay >= 0.25 && timeOfDay < 0.75)} onClick={() => setTimeOfDay(0.5)}>🌆 dusk</button>
        <button style={btn(timeOfDay >= 0.75)} onClick={() => setTimeOfDay(1)}>☀️ day</button>
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
      <div style={{ marginTop: 6, opacity: 0.7 }}>element modes</div>
      <div style={row}>
        {ELEMENT_MODES.map((p: PatternId) => (
          <button key={p} style={btn(ctrl.pattern === p)} onClick={() => setCtrl({ pattern: p })}>
            {p}
          </button>
        ))}
      </div>
      <div style={row}>
        <button style={btn(false)} onClick={() => pingPresence()}>✨ ping (presence ripple)</button>
      </div>

      <div style={{ marginTop: 10, opacity: 0.7 }}>environment sensors (sim)</div>
      <div style={{ margin: "4px 0", padding: 8, background: "#0d141e", borderRadius: 6 }}>
        <Slider label={`crowd ${Math.round(sensors.crowd * 100)}%`} v={sensors.crowd} min={0} max={1} step={0.01} on={(v) => setSensors({ crowd: v })} />
        <Slider label={`motion ${Math.round(sensors.motion * 100)}% → ripples`} v={sensors.motion} min={0} max={1} step={0.01} on={(v) => setSensors({ motion: v })} />
        <Slider label={`temp ${sensors.tempC.toFixed(0)}°C`} v={sensors.tempC} min={-5} max={45} step={1} on={(v) => setSensors({ tempC: v })} />
        <Slider label={`wind ${sensors.windKph.toFixed(0)} km/h`} v={sensors.windKph} min={0} max={80} step={1} on={(v) => setSensors({ windKph: v })} />
        <Slider label={`daylight ${Math.round(sensors.ambient * 100)}%`} v={sensors.ambient} min={0} max={1} step={0.01} on={(v) => setSensors({ ambient: v })} />
        <button
          onClick={() => setCtrl({ autoBalance: !ctrl.autoBalance })}
          style={{
            width: "100%", marginTop: 6, padding: "5px 8px", borderRadius: 5, cursor: "pointer", fontSize: 10,
            border: ctrl.autoBalance ? "1px solid #3ddc97" : "1px solid #243044",
            background: ctrl.autoBalance ? "#10241c" : "#0d141e", color: ctrl.autoBalance ? "#7af0c0" : "#7a8aa0",
          }}
        >
          ⚖ auto-balance {ctrl.autoBalance ? `ON · ${Math.round(autoBalanceGain(sensors.ambient) * 100)}% drive` : "OFF"}
        </button>
        <div style={{ fontSize: 9.5, opacity: 0.5, marginTop: 2 }}>cold→cool hue · wind→speed · crowd→energy · auto-balance: daylight→boost drive to stay readable</div>
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
        <button style={btn(!!midi)} onClick={connectMidi}>🎹 MIDI</button>
        <button style={btn(guest)} onClick={() => setGuest(!guest)}>🔒 guest</button>
      </div>
      {guest && <div style={{ fontSize: 10, color: "#ffd27f" }}>guest scope: brightness/master capped, strobe locked</div>}
      {midi && <div style={{ fontSize: 10, opacity: 0.6 }}>{midi}</div>}

      <div style={{ marginTop: 10, opacity: 0.7 }}>cues</div>
      <div style={{ display: "flex", gap: 4, margin: "4px 0" }}>
        <input
          value={cueName}
          placeholder="cue name"
          onChange={(e) => setCueName(e.target.value)}
          onKeyDown={(e) => { if (e.key === "Enter") { addCue(cueName); setCueName(""); } }}
          style={{ flex: 1, padding: "5px 8px", borderRadius: 6, border: "1px solid #2a3a52", background: "#0b1119", color: "#dce6ff", font: "11px ui-monospace, monospace" }}
        />
        <button style={btn(false)} onClick={() => { addCue(cueName); setCueName(""); }}>💾 save</button>
      </div>
      {cues.map((c) => (
        <div key={c.id} style={row}>
          <button style={{ ...btn(false), flex: 1, textAlign: "left" }} onClick={() => recallCue(c.id)}>▶ {c.name}</button>
          <button style={btn(false)} onClick={() => deleteCue(c.id)}>×</button>
        </div>
      ))}
      {cues.length > 0 && (
        <>
          <div style={row}>
            <button style={btn(timeline.playing)} onClick={() => setTimeline({ playing: !timeline.playing })}>
              {timeline.playing ? "⏹ stop timeline" : "▶ play timeline"}
            </button>
          </div>
          <Slider label="step (s)" v={timeline.stepSecs} min={1} max={30} step={1} on={(v) => setTimeline({ stepSecs: Math.round(v) })} />
          <div style={row}>
            <button style={btn(false)} onClick={() => {
              const doc = compileShow(cues, fixtures, ctrl, net.channel, timeline.stepSecs * 1000);
              const blob = new Blob([showToJson(doc)], { type: "application/json" });
              const a = document.createElement("a");
              a.href = URL.createObjectURL(blob);
              a.download = "resonance-show.json";
              a.click();
              URL.revokeObjectURL(a.href);
            }}>⬇ compile show → JSON</button>
          </div>
        </>
      )}

      <div style={{ marginTop: 10, opacity: 0.7 }}>🧠 say it (LLM operator → commands)</div>
      <input
        value={nl}
        placeholder='e.g. "make the canopy pulse blue and slow"'
        onChange={(e) => setNl(e.target.value)}
        onKeyDown={(e) => {
          if (e.key === "Enter" && nl.trim()) {
            const r = interpret(nl);
            if (r.commands.length) runScript(r.commands.join("\n"));
            setNlNote(r.note);
            setNl("");
          }
        }}
        style={{
          width: "100%", marginTop: 4, padding: "6px 8px", borderRadius: 6,
          border: "1px solid #3a4a6a", background: "#0b1119", color: "#dce6ff",
          font: "11px ui-monospace, monospace", boxSizing: "border-box",
        }}
      />
      {nlNote && <div style={{ fontSize: 9.5, opacity: 0.6, marginTop: 2 }}>{nlNote}</div>}

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

      <div style={{ marginTop: 10, opacity: 0.7 }}>control plane (ESP-NOW · params, not pixels)</div>
      <div style={{ display: "flex", alignItems: "center", gap: 6, margin: "4px 0" }}>
        <span style={{ fontSize: 10, opacity: 0.6 }}>ch</span>
        <select
          value={net.channel}
          onChange={(e) => setNet({ channel: +e.target.value })}
          style={{ background: "#0b1119", color: "#dce6ff", border: "1px solid #2a3a52", borderRadius: 4, padding: "3px", font: "11px ui-monospace, monospace" }}
        >
          {Array.from({ length: 13 }, (_, i) => i + 1).map((c) => (
            <option key={c} value={c}>{c}</option>
          ))}
        </select>
        <button style={{ ...btn(net.driveReal), flex: 1 }} onClick={() => setNet({ driveReal: !net.driveReal })}>
          📡 drive real {net.driveReal ? "(armed)" : "(off)"}
        </button>
      </div>
      {f0 && (
        <div style={{ fontSize: 9, opacity: 0.55, wordBreak: "break-all", lineHeight: 1.4 }}>
          pkt → {JSON.stringify(encodeFixture(ctrl, f0, ov0))}
        </div>
      )}
    </div>
  );
}
