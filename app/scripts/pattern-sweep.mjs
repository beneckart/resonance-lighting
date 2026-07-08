// Pattern-library audit sweep: screenshot + lit/motion metrics for EVERY pattern
// (Elliot: patterns must be unique, visually interesting, no repetitive loops).
// Usage: node scripts/pattern-sweep.mjs   → screenshots/pattern-sweep/*.png + metrics
import { chromium } from "@playwright/test";
import { mkdirSync } from "node:fs";
mkdirSync("screenshots/pattern-sweep", { recursive: true });
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto("http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(false); s.setCinematic(true); s.setTimeOfDay(0); });
await page.waitForTimeout(1500);
const grab = () => page.evaluate(() => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 240; g.height = 150;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 240, 150);
  const d = x.getImageData(0, 0, 240, 150).data;
  let sum = 0; const arr = new Array(d.length / 4);
  for (let i = 0; i < d.length; i += 4) { sum += d[i] + d[i+1] + d[i+2]; arr[i / 4] = d[i] + d[i+1] + d[i+2]; }
  return { sum, arr };
});
const pats = await page.evaluate(() => window.twin.getState().fixtures && [
  "solid","breathe","chase","ripple","sparkle","sequence","spectrum","tricolor","spiral","godray","rising",
  "planewipe","warmcool","bloom","firefly","ca","hero","plasma","chromatic","rings","fibonacci","sweep",
  "aurora","chladni","glyph","interference","lissajous","shockwave","hurricane","wind","ember","rain",
]);
console.log("pattern            lit-sum   motion(2s)  motion(4s)");
for (const pat of pats) {
  await page.evaluate((p) => window.twin.getState().set({ pattern: p, brightness: 0.9, sat: 0.9, hue: 0.08, speed: 1, colorCycle: "off", blackout: false }), pat);
  await page.waitForTimeout(2500);
  const a = await grab();
  await page.waitForTimeout(2000);
  const b = await grab();
  await page.waitForTimeout(2000);
  const c2 = await grab();
  const mv = (x, y) => x.arr.filter((v, i) => Math.abs(v - y.arr[i]) > 80).length;
  await page.screenshot({ path: `screenshots/pattern-sweep/${pat}.png` });
  console.log(`${pat.padEnd(18)} ${String(b.sum).padStart(8)} ${String(mv(a, b)).padStart(10)} ${String(mv(a, c2)).padStart(10)}`);
}
await browser.close();
