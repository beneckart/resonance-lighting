// Verify: Sound mode dock exposes the piano piece picker, and clicking
// "Moonlight ★" (full MIDI) actually starts the piano pattern.
import { chromium } from "@playwright/test";

const url = process.argv[2] || "http://localhost:4173";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
const errors = [];
page.on("pageerror", (e) => errors.push(String(e)));

await page.goto(url, { waitUntil: "networkidle", timeout: 30000 });
await page.waitForSelector("canvas", { state: "attached", timeout: 15000 });
await page.waitForTimeout(2500);

// go to Sound mode (whole-tree scope is the default)
await page.getByRole("button", { name: /🎵/ }).first().click();
await page.waitForTimeout(800);

// the Shows·Piano section must be present in Sound mode
const pianoHeader = page.getByText("🎹 Piano · 72 keys");
if (!(await pianoHeader.isVisible())) {
  // maybe section is below the fold — scroll the dock
  await pianoHeader.scrollIntoViewIfNeeded().catch(() => {});
}
const headerVisible = await pianoHeader.isVisible();
console.log("piano section visible in Sound mode:", headerVisible);

// wait for MIDI manifest pieces to load, then click Moonlight ★ (exact match —
// careful: "Moonlight" built-in also exists)
const star = page.getByRole("button", { name: "Moonlight ★", exact: true });
await star.waitFor({ state: "visible", timeout: 10000 });
await star.click();
await page.waitForTimeout(2500);

// truth check via the store, not the UI
const state = await page.evaluate(() => {
  const s = window.twin?.getState?.() ?? window.twin;
  return { pattern: s?.control?.pattern, brightness: s?.control?.brightness };
});
console.log("store after click:", JSON.stringify(state));

await page.screenshot({ path: process.env.SHOT || "screenshots/piano-sound-mode.png" });
await browser.close();

if (errors.length) { console.error("PAGE ERRORS:\n" + errors.join("\n")); process.exit(1); }
if (!headerVisible || state.pattern !== "piano") { console.error("FAIL"); process.exit(1); }
console.log("PASS — Moonlight ★ playing from Sound mode");
