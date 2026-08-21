import { THEMES } from "./themes";

/** The colour-theme tile grid — SAME look everywhere (Elliot: Light Show must
 *  show the themes exactly like Interactive mode does): name + a strip of the
 *  theme's actual hues. One source of truth for all four panels. */
export function ThemePicker({ value, onPick }: { value: string; onPick: (id: string) => void }) {
  return (
    <div style={{ display: "flex", flexWrap: "wrap", gap: 4 }}>
      {THEMES.map((t) => {
        const on = value === t.id;
        return (
          <button key={t.id} onClick={() => onPick(t.id)} title={t.blurb}
            style={{ flex: "1 0 30%", padding: "5px 4px", borderRadius: 7, cursor: "pointer", fontSize: 10.5, fontWeight: 700,
              border: on ? "1.5px solid #cdd6e4" : "1px solid #2a3a52", background: on ? "#1a2434" : "#121a26", color: on ? "#eef3fb" : "#9fb0c7" }}>
            <div>{t.emoji} {t.name}</div>
            <div style={{ display: "flex", gap: 1, marginTop: 3, height: 5, borderRadius: 2, overflow: "hidden" }}>
              {(t.hues.length ? t.hues : [0, 0.17, 0.33, 0.5, 0.67, 0.83]).map((h, k) => (
                <div key={k} style={{ flex: 1, background: `hsl(${h * 360},85%,55%)` }} />
              ))}
            </div>
          </button>
        );
      })}
    </div>
  );
}
