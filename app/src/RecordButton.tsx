import { useRef, useState } from "react";
import { getPianoAudioStream, setPianoSound } from "./pianoAudio";

// pick the best container the browser can record — mp4 (best for social/iMessage) else webm
function pickMime(): string {
  const opts = ["video/mp4;codecs=avc1,mp4a", "video/mp4", "video/webm;codecs=vp9,opus", "video/webm;codecs=vp8,opus", "video/webm"];
  for (const m of opts) if (typeof MediaRecorder !== "undefined" && MediaRecorder.isTypeSupported(m)) return m;
  return "video/webm";
}

/** One-click screen+audio recorder. Captures the 3-D canvas and the piano audio
 *  (same Web Audio graph that plays the music) into a single downloadable video —
 *  so a social clip has the music baked in, no external screen-recorder needed. */
export function RecordButton() {
  const [recording, setRecording] = useState(false);
  const [secs, setSecs] = useState(0);
  const [saved, setSaved] = useState("");
  const recRef = useRef<MediaRecorder | null>(null);
  const timer = useRef<number | null>(null);

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
    <button onClick={() => (recording ? stop() : start())}
      title={recording ? "stop & save the video" : "record video + audio (for a social post)"}
      style={{
        position: "fixed", top: 46, left: "50%", transform: "translateX(-50%)", zIndex: 60,
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
