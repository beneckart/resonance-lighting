// Demo-clip recorder — drives the twin in real Chrome and captures slide-deck
// clips via the in-app 🎥 RecordButton, which now tab-captures the WHOLE page:
// tree on the left, organized dock/console on the right (Elliot's demo layout),
// with the piano audio baked in. The --auto-select-tab-capture-source-by-title
// flag answers the getDisplayMedia picker so the run is unattended.
// Raw MediaRecorder mp4s are FRAGMENTED (QuickTime chokes) — remux after with:
//   ffmpeg -i raw.mp4 -c copy -movflags +faststart out.mp4
// Usage (preview server must be running):  node scripts/record-demo.mjs [url] [outDir]
import { chromium } from "@playwright/test";
import { mkdirSync, existsSync } from "node:fs";
import { resolve } from "node:path";

const url = process.argv[2] || "http://localhost:4173";
const outDir = resolve(process.argv[3] || "../demo-videos/raw");
mkdirSync(outDir, { recursive: true });

const browser = await chromium.launch({
  channel: "chrome", // system Chrome: MediaRecorder supports avc1+mp4a
  headless: false, // real GPU — SwiftShader would turn the god-rays into a slideshow
  args: [
    "--window-size=1920,1200",
    '--auto-select-tab-capture-source-by-title=Resonance Tree — Mirror Twin',
    "--autoplay-policy=no-user-gesture-required",
    "--disable-background-timer-throttling",
    "--disable-backgrounding-occluded-windows",
    "--disable-renderer-backgrounding",
  ],
});
const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
page.on("pageerror", (e) => console.error("PAGE ERROR:", e));

await page.goto(url, { waitUntil: "networkidle", timeout: 30000 });
await page.waitForSelector("canvas", { state: "attached", timeout: 15000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setCinematic?.(false); s.setTimeOfDay(0); });
await page.waitForTimeout(2500); // scene settle

const startBtn = () => page.getByRole("button", { name: /record video \+ audio/ });
const stopBtn = () => page.getByRole("button", { name: /stop & save/ });

async function record(file, seconds, action) {
  if (existsSync(resolve(outDir, file + ".mp4")) || existsSync(resolve(outDir, file + ".webm"))) {
    console.log(`↷ ${file} already recorded — skipping`);
    return;
  }
  console.log(`▶ ${file} — ${seconds}s`);
  await startBtn().click(); // getDisplayMedia picker is auto-answered by the launch flag
  await page.waitForTimeout(4000); // picker auto-select + share-bar settle + encoder start
  await action();
  await page.waitForTimeout(seconds * 1000);
  const dl = page.waitForEvent("download", { timeout: 30000 });
  await stopBtn().click();
  const download = await dl;
  const ext = download.suggestedFilename().endsWith(".mp4") ? ".mp4" : ".webm";
  const dest = resolve(outDir, file + ext);
  await download.saveAs(dest);
  console.log(`  ✓ raw saved ${dest}`);
  await startBtn().waitFor({ timeout: 20000 }); // button resets after its "saved" toast
}

// ── 1 · Moonlight Sonata — Sound mode, full MIDI score, audio baked in ──────
await page.getByRole("button", { name: "🎵 Sound" }).click();
const moonlight = page.getByRole("button", { name: "Moonlight ★", exact: true });
await moonlight.waitFor({ timeout: 15000 }); // manifest pieces loaded
await record("moonlight-sonata", 72, async () => {
  await moonlight.click();
  const pat = await page.evaluate(() => window.twin.getState().control.pattern);
  if (pat !== "piano") { await moonlight.click(); } // one retry — a late picker overlay can swallow the first click
});

// ── 2 · Interactive — Game of Life (entry ceremony + visitor taps) ──────────
await page.evaluate(() => window.twin.getState().set({ pattern: "solid", brightness: 0.4 })); // silence the piano
await page.getByRole("button", { name: "🌱 Interactive" }).click();
await record("interactive-game-of-life", 52, async () => {
  await page.getByRole("button", { name: /Game of Life/ }).first().click(); // ceremony: dark → flourish → blank
  (async () => {
    await page.waitForTimeout(14000); // let the ceremony land
    for (let i = 0; i < 7; i++) {
      await page.evaluate(() => {
        const s = window.twin.getState();
        const outer = s.fixtures.map((f, i) => ({ f, i })).filter((x) => x.f.role === "downlight" && x.f.radialT >= 0.35);
        const pick = outer[Math.floor(Math.random() * outer.length)] ?? { i: Math.floor(Math.random() * s.fixtures.length) };
        s.triggerAt(pick.i); // a visitor touches the tree
      });
      await page.waitForTimeout(4500);
    }
  })().catch((e) => console.error("tap loop:", e));
});

// ── 3 · Light show — Aurora (noise curtains · standing waves) ───────────────
await page.evaluate(() => window.twin.getState().set({ pattern: "solid" }));
await page.getByRole("button", { name: "🎬 Light Show" }).click();
await record("aurora-show", 40, async () => {
  await page.getByRole("button", { name: /Aurora/ }).click();
});

// ── 4 · Light show — Ignition (energetic · strobe + rainbow) ────────────────
await record("ignition-show", 40, async () => {
  await page.getByRole("button", { name: /Ignition/ }).click();
});

await page.evaluate(() => window.twin.getState().playShow(null));
await browser.close();
console.log("ALL RAW CLIPS DONE →", outDir);
