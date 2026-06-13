import { chromium } from "@playwright/test";
const b = await chromium.launch({ args: ["--autoplay-policy=no-user-gesture-required"] });
const p = await b.newPage({ viewport: { width: 1280, height: 800 } });
const errs = []; p.on("pageerror", (e) => errs.push(String(e)));
await p.goto("http://localhost:5173", { waitUntil: "networkidle" });
await p.waitForSelector("canvas");
await p.getByText("🎶 test track", { exact: false }).click();
await p.getByText("breathe", { exact: true }).click(); // steady pattern so beat-flash is obvious
await p.waitForTimeout(9000);
const body = await p.evaluate(() => document.body.innerText);
const m = body.match(/(\d+)\s*BPM/);
await p.screenshot({ path: "screenshots/cycle12-audio.png" });
await b.close();
console.log("detected:", m ? m[1] + " BPM" : "no BPM shown", errs.length ? "ERRORS:\n" + errs.join("\n") : "· no errors");
