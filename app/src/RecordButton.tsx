import { useRef, useState } from "react";
import { useTwin } from "./store";
import { getPianoAudioStream, setPianoSound } from "./pianoAudio";

// pick the best container the browser can record. The codecs MUST be fully
// specified: H.264 + AAC ("avc1…,mp4a.40.2") plays everywhere (QuickTime, iPhone,
// iMessage, Instagram). Bare "video/mp4" makes Chrome mux OPUS audio into the mp4,
// which QuickTime refuses to open — that was the "download doesn't work" bug.
function pickMime(): string {
  const opts = [
    'video/mp4;codecs="avc1.640028,mp4a.40.2"', // H.264 High + AAC-LC — best quality
    'video/mp4;codecs="avc1.42E01E,mp4a.40.2"', // H.264 Baseline + AAC-LC
    "video/webm;codecs=vp9,opus",
    "video/webm;codecs=vp8,opus",
    "video/webm",
  ];
  for (const m of opts) if (typeof MediaRecorder !== "undefined" && MediaRecorder.isTypeSupported(m)) return m;
  return "video/webm";
}

/** One-click screen+audio recorder. Captures the 3-D canvas and the piano audio
 *  (same Web Audio graph that plays the music) into a single downloadable video —
 *  so a social clip has the music baked in, no external screen-recorder needed. */
export function RecordButton() {
  const dockOn = useTwin((s) => s.dock && !s.cinematic);
  const [recording, setRecording] = useState(false);
  const [secs, setSecs] = useState(0);
  const [saved, setSaved] = useState("");
  const recRef = useRef<MediaRecorder | null>(null);
  const timer = useRef<number | null>(null);
  // DRAGGABLE (Elliot: "we should be able to move it") — drag to reposition
  // (persisted), tap to record. >6px of movement = a drag, not a click.
  const [pos, setPos] = useState<{ x: number; y: number } | null>(() => {
    try { const r = localStorage.getItem("recbtn.pos"); if (r) { const p = JSON.parse(r); return { x: Math.max(0, Math.min(p.x, window.innerWidth - 120)), y: Math.max(0, Math.min(p.y, window.innerHeight - 48)) }; } } catch { /* default */ }
    return null; // null = the default top-center spot
  });
  const dragSt = useRef<{ px: number; py: number; x: number; y: number; moved: boolean } | null>(null);
  const onBtnDown = (e: React.PointerEvent) => {
    const el = e.currentTarget as HTMLElement;
    const r = el.getBoundingClientRect();
    dragSt.current = { px: e.clientX, py: e.clientY, x: r.left, y: r.top, moved: false };
    try { el.setPointerCapture(e.pointerId); } catch { /* fine */ }
  };
  const posRef = useRef(pos); // synchronous latest position (state lags pointer events)
  const onBtnMove = (e: React.PointerEvent) => {
    const d = dragSt.current; if (!d) return;
    if (!d.moved && Math.hypot(e.clientX - d.px, e.clientY - d.py) < 6) return;
    d.moved = true;
    const x = Math.max(0, Math.min(d.x + e.clientX - d.px, window.innerWidth - 140));
    const y = Math.max(0, Math.min(d.y + e.clientY - d.py, window.innerHeight - 44));
    posRef.current = { x, y };
    setPos({ x, y });
  };
  const lastWasDrag = useRef(false);
  const onBtnUp = () => {
    const d = dragSt.current; dragSt.current = null;
    lastWasDrag.current = !!d?.moved;
    if (d?.moved && posRef.current) { try { localStorage.setItem("recbtn.pos", JSON.stringify(posRef.current)); } catch { /* fine */ } }
  };

  const start = () => {
    const canvas = document.querySelector("canvas") as HTMLCanvasElement | null;
    if (!canvas) return;
    setPianoSound(true); // ensure the audio bus exists (this click is the user gesture)
    const v = canvas.captureStream(30);
    const a = getPianoAudioStream();
    const stream = new MediaStream([...v.getVideoTracks(), ...(a ? a.getAudioTracks() : [])]);
    const mime = pickMime();
    let rec: MediaRecorder;
    try { rec = new MediaRecorder(stream, { mimeType: mime, videoBitsPerSecond: 8_000_000 }); }
    catch { rec = new MediaRecorder(stream); }
    const chunks: BlobPart[] = [];
    rec.ondataavailable = (e) => { if (e.data.size) chunks.push(e.data); };
    rec.onstop = () => {
      const blob = new Blob(chunks, { type: rec.mimeType || mime });
      if (blob.size < 1000) { setSaved("⚠ empty — record longer"); setTimeout(() => setSaved(""), 4000); return; }
      const url = URL.createObjectURL(blob);
      const name = `resonance-tree.${(rec.mimeType || mime).includes("mp4") ? "mp4" : "webm"}`;
      const link = document.createElement("a");
      link.href = url;
      link.download = name;
      link.style.display = "none";
      document.body.appendChild(link); // MUST be in the DOM or some browsers block the download
      link.click();
      setTimeout(() => { link.remove(); URL.revokeObjectURL(url); }, 8000);
      setSaved(`✓ saved ${name} (${(blob.size / 1e6).toFixed(1)} MB) → Downloads`);
      setTimeout(() => setSaved(""), 8000);
    };
    rec.start(250);
    recRef.current = rec;
    setRecording(true); setSecs(0);
    timer.current = window.setInterval(() => setSecs((s) => s + 1), 1000);
  };

  const stop = () => {
    recRef.current?.stop();
    recRef.current = null;
    if (timer.current) { clearInterval(timer.current); timer.current = null; }
    setRecording(false);
  };

  return (
    <button
      onClick={() => { if (lastWasDrag.current) { lastWasDrag.current = false; return; } recording ? stop() : start(); }}
      onPointerDown={onBtnDown} onPointerMove={onBtnMove} onPointerUp={onBtnUp}
      title={recording ? "stop & save the video" : "record video + audio (for a social post) — drag to move"}
      style={{
        ...(pos
          ? { position: "fixed" as const, top: pos.y, left: pos.x, transform: "none" }
          : { position: "fixed" as const, top: 46, left: dockOn ? "25%" : "50%", transform: "translateX(-50%)" }),
        zIndex: 60, touchAction: "none",
        padding: "7px 13px", borderRadius: 10, cursor: "pointer", fontWeight: 700,
        border: recording ? "1.5px solid #ff5b6e" : "1px solid #2a3a52",
        background: recording ? "#2a1016" : "rgba(12,16,24,0.85)",
        color: recording ? "#ff8fa0" : "#cdd6e4", font: "12px ui-monospace, monospace", backdropFilter: "blur(6px)",
        boxShadow: recording ? "0 0 16px #ff5b6e66" : "none",
      }}>
      {recording ? `⏺ recording ${secs}s · stop & save` : (saved || "🎥 record video + audio")}
    </button>
  );
}
