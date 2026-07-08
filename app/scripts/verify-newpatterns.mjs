import { chromium } from "@playwright/test";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto("http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); });
const grab = () => page.evaluate(() => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 240; g.height = 150;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 240, 150);
  const d = x.getImageData(0, 0, 240, 150).data;
  let sum = 0; const arr = [];
  for (let i = 0; i < d.length; i += 4) { sum += d[i] + d[i+1] + d[i+2]; arr.push(d[i]); }
  return { sum, arr };
});
let fail = 0;
for (const pat of ["shockwave", "hurricane"]) {
  await page.evaluate((p) => window.twin.getState().set({ pattern: p, brightness: 0.9, sat: 0.9, speed: 1, blackout: false }), pat);
  await page.waitForTimeout(2500);
  const a = await grab(); await page.waitForTimeout(2000); const b = await grab();
  const moved = a.arr.filter((v, i) => Math.abs(v - b.arr[i]) > 30).length;
  const ok = a.sum > 400000 && moved > 40;
  console.log(`${ok ? "✓" : "✗"} ${pat}: lit sum ${a.sum} · macro-moved px ${moved}`);
  if (!ok) fail = 1;
}
await page.screenshot({ path: "screenshots/new-patterns.png" });
await browser.close();
process.exit(fail);
