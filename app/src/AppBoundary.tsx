import { Component, type ReactNode } from "react";

/**
 * TOP-LEVEL CRASH BOUNDARY (production readiness, 2026-08-15).
 *
 * `ErrorBoundary` isolates ASSET subtrees inside the 3D scene and falls back to
 * `null` — correct there, because a missing gobo should silently drop. But
 * main.tsx rendered <App /> bare, so a render error in ANY ui panel took the
 * whole app to a white screen with no way back.
 *
 * That is not hypothetical in this repo: f8b3081 ("✕-to-desktop crashed the
 * console — hooks lived below the early return") was exactly this, and a white
 * screen at night in the desert with no recovery is the worst possible failure.
 *
 * The second half matters as much as the first: this app persists a LOT to
 * localStorage — ui.mode, dock.order.v2.*, touch.collapsed, saved cues,
 * calibration, the operator model. A single corrupt value crashes on every
 * boot, and a plain "reload" loops forever. SAFE MODE clears the UI state that
 * can rot while deliberately preserving the things that are expensive or
 * impossible to re-enter: the operator API key and the calibration map.
 */

const UI_STATE_PREFIXES = ["ui.", "dock.", "touch.", "twin.", "cues"];
/** never cleared: re-entering these costs the operator real time in the field */
const PRESERVE = ["resonance.openrouter.key", "resonance.calibration"];

function clearUiState(): number {
  let cleared = 0;
  try {
    const doomed: string[] = [];
    for (let i = 0; i < localStorage.length; i++) {
      const k = localStorage.key(i);
      if (!k || PRESERVE.some((p) => k.startsWith(p))) continue;
      if (UI_STATE_PREFIXES.some((p) => k.startsWith(p))) doomed.push(k);
    }
    for (const k of doomed) { localStorage.removeItem(k); cleared++; }
  } catch { /* storage unavailable — nothing to clear */ }
  return cleared;
}

const btn: React.CSSProperties = {
  minHeight: 48, padding: "0 20px", borderRadius: 12, cursor: "pointer",
  fontSize: 15, fontWeight: 700, border: "1.5px solid #2b3a52",
  background: "#141a26", color: "#e7ecf6", touchAction: "manipulation",
};

export class AppBoundary extends Component<{ children: ReactNode }, { failed: boolean; msg: string }> {
  state = { failed: false, msg: "" };

  static getDerivedStateFromError(err: unknown) {
    return { failed: true, msg: err instanceof Error ? err.message : String(err) };
  }

  componentDidCatch(err: unknown) {
    // console.error, not a silent swallow — the operator can hand this to a dev
    console.error("[AppBoundary] the app crashed:", err);
  }

  render() {
    if (!this.state.failed) return this.props.children;
    return (
      <div style={{
        position: "fixed", inset: 0, zIndex: 9999, background: "#0b0f17", color: "#e7ecf6",
        display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center",
        gap: 18, padding: 24, textAlign: "center",
        font: "15px -apple-system, ui-sans-serif, system-ui, sans-serif",
      }}>
        <div style={{ fontSize: 40 }}>🌳</div>
        <div style={{ fontSize: 20, fontWeight: 700 }}>The controller hit an error.</div>
        <div style={{ color: "#9fb0c7", maxWidth: 460, lineHeight: 1.6 }}>
          The lights themselves are unaffected — they keep running their last program.
          Reload to get the console back.
        </div>
        <div style={{ display: "flex", gap: 12, flexWrap: "wrap", justifyContent: "center" }}>
          <button style={{ ...btn, borderColor: "#ffb454", color: "#ffb454" }}
            onClick={() => window.location.reload()}>↻ Reload</button>
          <button style={btn} onClick={() => {
            // for a crash that reproduces on every boot: drop the saved UI state
            // (layout, collapsed sections, saved looks) and reload clean.
            const n = clearUiState();
            console.warn(`[AppBoundary] safe mode: cleared ${n} saved UI key(s)`);
            window.location.reload();
          }}>⚠ Safe mode — reset saved layout</button>
        </div>
        <details style={{ marginTop: 8, color: "#7e8ea6", maxWidth: 560 }}>
          <summary style={{ cursor: "pointer" }}>error detail</summary>
          <pre style={{
            textAlign: "left", whiteSpace: "pre-wrap", wordBreak: "break-word",
            fontSize: 12, background: "#0d1420", border: "1px solid #1c2740",
            borderRadius: 8, padding: 10, marginTop: 8,
          }}>{this.state.msg}</pre>
        </details>
        <div style={{ color: "#5b6b80", fontSize: 12 }}>
          Safe mode keeps your operator key and calibration.
        </div>
      </div>
    );
  }
}
