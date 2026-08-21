// Playwright screenshot harness for the mandatory per-cycle visual verification.
// Usage: node scripts/screenshot.mjs [url] [outPath]
//   defaults: http://localhost:4173  ->  screenshots/latest.png
import { chromium } from "@playwright/test";
import { mkdirSync } from "node:fs";
import { dirname } from "node:path";

const url = process.argv[2] || "http://localhost:4173";
const out = process.argv[3] || "screenshots/latest.png";

mkdirSync(dirname(out), { recursive: true });

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
const errors = [];
page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
page.on("pageerror", (e) => errors.push(String(e)));

await page.goto(url, { waitUntil: "networkidle", timeout: 30000 });
await page.waitForSelector("canvas", { timeout: 15000 });
await page.waitForTimeout(2000); // let the scene settle / a few frames render
await page.screenshot({ path: out });
await browser.close();

if (errors.length) {
  console.error("PAGE ERRORS:\n" + errors.join("\n"));
  process.exit(1);
}
console.log("screenshot ->", out);
