import { chromium } from "@playwright/test";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto("http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); s.set({ pattern: "breathe", brightness: 0.9 }); });
await page.getByRole("button", { name: "🔧 Calibrate" }).click();
await page.getByRole("button", { name: /Test grid 7×7/ }).click();
await page.waitForFunction(() => window.twin.getState().fixtures.length === 49, null, { timeout: 10000 });
const st = await page.evaluate(() => { const s = window.twin.getState(); return { n: s.fixtures.length, src: s.source, heights: [...new Set(s.fixtures.map((f) => Math.round(f.pos[1])))].length }; });
console.log("grid state:", JSON.stringify(st));
await page.waitForTimeout(2500);
await page.screenshot({ path: "screenshots/test-grid.png" });
await page.getByRole("button", { name: "🌳 Tree (real)" }).click();
await page.waitForFunction(() => window.twin.getState().fixtures.length === 118, null, { timeout: 10000 });
console.log("back to tree:", await page.evaluate(() => window.twin.getState().fixtures.length), "fixtures");
await browser.close();
if (st.n !== 49 || st.src !== "testgrid" || st.heights < 2) { console.error("FAIL"); process.exit(1); }
console.log("PASS — 49-light grid loads, varied heights, tree restores");
