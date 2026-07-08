// Headless proof: Interactive → Game of Life = ceremony → 4-9-light seed →
// Conway-mesh evolution that never goes permanently dark or static.
import { chromium } from "@playwright/test";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto(process.argv[2] || "http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.set({ speed: 1 }); });
await page.getByRole("button", { name: "🌱 Interactive" }).click();
await page.getByRole("button", { name: /Game of Life/ }).first().click();
console.log("pattern:", await page.evaluate(() => window.twin.getState().control.pattern));
// let the ceremony land + a few generations run
await page.waitForTimeout(14000);
const lit = () => page.evaluate(() => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 320; g.height = 200;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 320, 200);
  const d = x.getImageData(0, 0, 320, 200).data;
  let n = 0; for (let i = 0; i < d.length; i += 4) if (d[i] + d[i+1] + d[i+2] > 90) n++;
  return n;
});
const a = await lit();
await page.waitForTimeout(8000); // ~4 generations
const b = await lit();
await page.waitForTimeout(8000);
const c = await lit();
console.log("lit-pixel samples over ~16s:", a, b, c);
const alive = a > 50 && b > 50 && c > 50;
const changing = Math.abs(a - b) + Math.abs(b - c) > 20;
await page.screenshot({ path: "screenshots/gol-conway.png" });
await browser.close();
if (!alive || !changing) { console.error("FAIL — alive:", alive, "changing:", changing); process.exit(1); }
console.log("PASS — Game of Life alive and evolving (never static, never dark)");
