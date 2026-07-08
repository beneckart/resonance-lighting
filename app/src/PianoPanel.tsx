import { useEffect, useState } from "react";
import { useTwin } from "./store";
import { resetPiano, setPiece, PIECE_LIST, currentPiece, loadMidiPiece } from "./piano";
import { setPianoSound } from "./pianoAudio";
import { THEMES } from "./themes";
import { Widget } from "./Widget";
import { asset } from "./fixtures";

/** 🎹 PIANO — the tree plays a score, one light per key (SOUND page; Elliot:
 *  "all the sound related components need to go to the sound page"). Pieces are
 *  the built-ins plus any full .mid scores listed in /midi/manifest.json. The
 *  colour THEME row reuses interactive mode's moods — every note's colour is
 *  pulled into the picked world ("play Für Elise to different colored themes"). */
export function PianoPanel() {
  const pianoOn = useTwin((s) => s.control.pattern === "piano");
  const caTheme = useTwin((s) => s.caTheme);
  const setCaTheme = useTwin((s) => s.setCaTheme);
  const set = useTwin((s) => s.set);
  const playShow = useTwin((s) => s.playShow);
  const setTod = useTwin((s) => s.setTimeOfDay);
  const [soundOn, setSoundOn] = useState(false);
  const [pieces, setPieces] = useState(() => [...PIECE_LIST]);
  useEffect(() => {
    // push the persisted theme into the field engine — on the Sound page this
    // panel may be the only one mounted, and the piano's colours read from it
    setCaTheme(useTwin.getState().caTheme);
    fetch(asset("/midi/manifest.json")).then((r) => (r.ok ? r.json() : [])).then(async (list: { id: string; name: string; file: string }[]) => {
      let added = false;
      for (const m of list || []) if (await loadMidiPiece(m.id, m.name, asset("/midi/" + m.file))) added = true;
      console.info(`[piano] manifest: ${(list || []).length} listed · pieces now ${PIECE_LIST.length}`);
      if (added) setPieces([...PIECE_LIST]);
    }).catch((e) => { console.info("[piano] manifest load failed:", e); });
  }, [setCaTheme]);

  return (
    <Widget id="piano" title="🎹 Piano" x={344} y={12} w={216} h={280}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <span style={{ fontWeight: 700, color: "#eef3fb", fontSize: 12 }}>72 keys · one light per key</span>
        <button onClick={() => { const v = !soundOn; setSoundOn(v); setPianoSound(v); }} title="play sound (to your speaker / Bluetooth)"
          style={{ padding: "2px 8px", borderRadius: 6, cursor: "pointer", fontSize: 11, border: soundOn ? "1px solid #3ddc97" : "1px solid #2a3a52", background: soundOn ? "#10241c" : "#141a26", color: soundOn ? "#7af0c0" : "#9fb0c7" }}>
          {soundOn ? "🔊 sound on" : "🔇 sound"}
        </button>
      </div>
      <div style={{ display: "flex", flexWrap: "wrap", gap: 4, marginTop: 6 }}>
        {pieces.map((p) => {
          const on = pianoOn && currentPiece() === p.id;
          return (
            <button key={p.id} onClick={() => { playShow(null); setPiece(p.id); resetPiano(); setPianoSound(true); setSoundOn(true); set({ pattern: "piano", brightness: 0.8, sat: 0.85, colorCycle: "off", reverse: false }); setTod(0); }}
              style={{ flex: "1 0 auto", padding: "5px 8px", borderRadius: 6, cursor: "pointer", fontSize: 11,
                border: on ? "1px solid #5b8cff" : "1px solid #2a3a52", background: on ? "#21345e" : "#121a26", color: on ? "#dce6ff" : "#9fb0c7" }}>
              {on ? "▶ " : ""}{p.name}
            </button>
          );
        })}
        {pianoOn && (
          <button onClick={() => set({ pattern: "solid" })}
            style={{ flex: "1 0 auto", padding: "5px 8px", borderRadius: 6, cursor: "pointer", fontSize: 11, border: "1px solid #5a3a3a", background: "#1a1016", color: "#ff8fa0" }}>⏹ stop</button>
        )}
      </div>
      {/* colour THEME — the same moods as interactive mode; Wild 🎲 = the piano's
          native warm arc */}
      <div style={{ marginTop: 8, fontSize: 10, color: "#8aa0bb" }}>🎨 theme</div>
      <div style={{ display: "flex", flexWrap: "wrap", gap: 3, marginTop: 3 }}>
        {THEMES.map((t) => {
          const on = caTheme === t.id;
          return (
            <button key={t.id} onClick={() => setCaTheme(t.id)} title={t.blurb}
              style={{ flex: "1 0 22%", padding: "4px 3px", borderRadius: 6, cursor: "pointer", fontSize: 9.5, fontWeight: 700,
                border: on ? "1.5px solid #cdd6e4" : "1px solid #2a3a52", background: on ? "#1a2434" : "#121a26", color: on ? "#eef3fb" : "#9fb0c7" }}>
              {t.emoji} {t.name}
            </button>
          );
        })}
      </div>
    </Widget>
  );
}
