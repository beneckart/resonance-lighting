import { useTwin, CA_RULES, type PatternId } from "./store";
import { Widget } from "./Widget";

/** INTERACTIVITY MODE — the tree lives on its own DECENTRALISED rules (Ben's
 *  BACKGROUND.md mesh spec: each light runs a simple local rule over its pre-baked
 *  neighbour list) and reacts to presence. Sibling to Light-Show mode: shows are
 *  authored/central, this is emergent/local + interactive.
 *
 *  The four rules are true cellular automata already living in field.ts — this
 *  panel just frames them as one coherent mode with a "poke the tree" control that
 *  injects a disturbance the mesh propagates outward (the carried "wand", in sim). */
const RULE_META: Record<string, { name: string; blurb: string; emoji: string; hue: number }> = {
  life: { name: "Game of Life", blurb: "cells born & die by neighbour count", emoji: "🌱", hue: 0.05 },
  ripples: { name: "Excitable", blurb: "waves ripple out & fade · Greenberg-Hastings", emoji: "💫", hue: 0.55 },
  organism: { name: "Reaction-Diffusion", blurb: "blobs drift, split & merge · Gray-Scott", emoji: "🫧", hue: 0.5 },
  living: { name: "Firefly Sync", blurb: "fireflies fall into travelling waves · Kuramoto", emoji: "✨", hue: 0.12 },
};

export function InteractivityPanel() {
  const control = useTwin((s) => s.control);
  const set = useTwin((s) => s.set);
  const ping = useTwin((s) => s.pingPresence);
  const setTod = useTwin((s) => s.setTimeOfDay);
  const active = control.pattern;
  const isCA = (CA_RULES as PatternId[]).includes(active);

  const pickRule = (r: PatternId) => {
    // enter the rule with safe, non-strobe defaults; keep it dark so the CA reads
    set({
      pattern: r, colorCycle: "off", order: "linear", reverse: false,
      strobe: false, blackout: false, beaconPreempt: false, master: 1,
      brightness: 0.95, sat: 0.85, hue: RULE_META[r]?.hue ?? control.hue,
    });
    setTod(0); // night — the living field reads best against black
  };

  return (
    <Widget id="interactivity" title="🌱 Interactivity" x={568} y={12} w={232} h={318} accent="#3ddc97">
      <div style={{ fontSize: 10.5, color: "#8fb9a6", lineHeight: 1.35, marginBottom: 8 }}>
        The tree lives on its own <b>local rules</b> — each light decides from its
        neighbours, no central pattern. Then you disturb it.
      </div>

      {(CA_RULES as PatternId[]).map((r) => {
        const m = RULE_META[r];
        const on = active === r;
        return (
          <button key={r} onClick={() => pickRule(r)}
            style={{ display: "block", width: "100%", textAlign: "left", margin: "4px 0", padding: "6px 8px", borderRadius: 7, cursor: "pointer",
              border: on ? "1px solid #3ddc97" : "1px solid #2a3a52", background: on ? "#10362a" : "#121a26", color: on ? "#a9f0d4" : "#9fb0c7" }}>
            <div style={{ fontWeight: 700 }}>{on ? "● " : "○ "}{m.emoji} {m.name}</div>
            <div style={{ fontSize: 9.5, opacity: 0.7 }}>{m.blurb}</div>
          </button>
        );
      })}

      {/* the "wand" poke — inject a disturbance the mesh propagates outward */}
      <button onClick={() => { if (!isCA) pickRule("life"); ping(); }}
        title="poke the tree — seed a disturbance that rolls outward across the mesh (the carried wand, in sim)"
        style={{ width: "100%", marginTop: 10, padding: "10px 8px", borderRadius: 9, cursor: "pointer", fontWeight: 700, fontSize: 13,
          border: "1.5px solid #3ddc97", background: "#12402f", color: "#b7f5db", boxShadow: "0 0 14px #3ddc9744" }}>
        👋 Poke the tree
      </button>
      <div style={{ fontSize: 9.5, color: "#6f8a7e", marginTop: 4, textAlign: "center" }}>
        seeds a wavefront at the nearest fixture → hops outward
      </div>

      {/* live shaping — speed / palette / brightness ride the running field */}
      <div style={{ marginTop: 12, opacity: isCA ? 1 : 0.4, pointerEvents: isCA ? "auto" : "none" }}>
        <Row label={`Speed · ${control.speed.toFixed(2)}`}>
          <input type="range" min={0.15} max={3} step={0.05} value={control.speed}
            onChange={(e) => set({ speed: +e.target.value })} style={{ width: "100%" }} />
        </Row>
        <Row label={`Palette · hue ${control.hue.toFixed(2)}`}>
          <input type="range" min={0} max={1} step={0.01} value={control.hue}
            onChange={(e) => set({ hue: +e.target.value })}
            style={{ width: "100%", accentColor: `hsl(${control.hue * 360},80%,55%)` }} />
        </Row>
        <Row label={`Brightness · ${Math.round(control.brightness * 100)}%`}>
          <input type="range" min={0.1} max={1} step={0.02} value={control.brightness}
            onChange={(e) => set({ brightness: +e.target.value })} style={{ width: "100%" }} />
        </Row>
      </div>
    </Widget>
  );
}

function Row({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <label style={{ display: "block", marginBottom: 8 }}>
      <div style={{ fontSize: 10, color: "#8aa0bb", marginBottom: 2 }}>{label}</div>
      {children}
    </label>
  );
}
