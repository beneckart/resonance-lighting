import { useMemo, useState } from "react";
import { Widget } from "./Widget";
import { compileRules, MAX_BYTES, parseRules, RULE_PRESETS } from "./rules";

/** FLEET RULES editor — write the behavior program, flash it to the fleet.
 *
 *  The program is TEXT (one rule per line, first match wins, last line is the
 *  default). It compiles to a table that fits one ESP-NOW broadcast — pushing
 *  new behavior to 118 lights costs one frame. The Fleet panel's env sliders
 *  then prove the fleet actually switches (watch st/pattern flip in the
 *  ledger, announced by instant events).
 *
 *  Publishing goes through whatever bridge is connected: the Fleet panel owns
 *  the link; this panel hands it the compiled bytes via a custom event so the
 *  two stay decoupled. */

const ACCENT = "#b06cd8";

export function RulesPanel() {
  const [text, setText] = useState<string>(() => {
    try { return localStorage.getItem("resonance.rules.draft") ?? RULE_PRESETS["night-saver"]; }
    catch { return RULE_PRESETS["night-saver"]; }
  });
  const [epoch, setEpoch] = useState(1);
  const [flashed, setFlashed] = useState<string | null>(null);

  const parsed = useMemo(() => parseRules(text, epoch), [text, epoch]);
  const bytes = parsed.ok ? compileRules(parsed.ruleset!).length : 0;

  const save = (t: string) => {
    setText(t);
    setFlashed(null);
    try { localStorage.setItem("resonance.rules.draft", t); } catch { /* non-fatal */ }
  };

  const flash = () => {
    if (!parsed.ok) return;
    const payload = [...compileRules(parsed.ruleset!)];
    // hand to whoever holds the bridge (FleetPanel listens)
    window.dispatchEvent(new CustomEvent("resonance:flash-rules", { detail: { epoch, bytes: payload } }));
    setFlashed(`epoch ${epoch} · ${payload.length} B · one broadcast`);
    setEpoch((e) => e + 1);
  };

  const btn = (label: string, onClick: () => void, disabled = false, primary = false) => (
    <button onClick={onClick} disabled={disabled}
      style={{ padding: "5px 9px", borderRadius: 8, border: `1px solid ${primary ? ACCENT : "#2a3648"}`,
        background: primary ? "rgba(176,108,216,0.16)" : "#121a26", color: disabled ? "#5a677a" : "#e8eefb",
        font: "11.5px ui-monospace, monospace", cursor: disabled ? "default" : "pointer" }}>
      {label}
    </button>
  );

  return (
    <Widget id="fleetrules" title="📜 Fleet Rules · behavior program" x={340} y={64} w={330} h={430} accent={ACCENT}>
      <div style={{ display: "flex", flexDirection: "column", gap: 8, font: "11.5px ui-monospace, monospace" }}>
        <div style={{ color: "#9fb0c7", lineHeight: 1.45 }}>
          How the tree behaves in each condition — the fixtures run this
          <b> themselves</b>, radio or no radio. One rule per line, first match
          wins, last line is the default. Sensors: hour · soc · presence ·
          sound · supply · mode.
        </div>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
          {Object.keys(RULE_PRESETS).map((p) => (
            <span key={p}>{btn(p, () => save(RULE_PRESETS[p]))}</span>
          ))}
        </div>
        <textarea value={text} onChange={(e) => save(e.target.value)} spellCheck={false} rows={9}
          style={{ width: "100%", boxSizing: "border-box", resize: "vertical", background: "#0a0f16",
            color: "#d7e3f4", border: `1px solid ${parsed.ok ? "#1d2735" : "#8a2f44"}`, borderRadius: 8,
            padding: 8, font: "11.5px ui-monospace, monospace", lineHeight: 1.5 }} />
        {/* live validation */}
        {parsed.ok ? (
          <div style={{ color: "#9fb0c7" }}>
            ✓ {parsed.ruleset!.rules.length} rules · compiles to{" "}
            <b style={{ color: bytes > MAX_BYTES * 0.9 ? "#ffb454" : "#3ddc97" }}>{bytes} B</b>
            <span style={{ color: "#5a677a" }}> / {MAX_BYTES} B (one ESP-NOW frame)</span>
          </div>
        ) : (
          <div style={{ color: "#ff5470" }}>
            {parsed.errors.slice(0, 4).map((e, i) => <div key={i}>✗ {e}</div>)}
          </div>
        )}
        <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
          {btn("⚡ flash rules to fleet", flash, !parsed.ok, true)}
          {flashed && <span style={{ color: "#3ddc97" }}>sent · {flashed}</span>}
        </div>
        <div style={{ color: "#5a677a", lineHeight: 1.4 }}>
          Connect the fleet in the 📡 Fleet panel first, then use its env
          sliders (hour / presence / sound / daylight) to watch the whole
          fleet switch behavior — instantly, via events.
        </div>
      </div>
    </Widget>
  );
}
