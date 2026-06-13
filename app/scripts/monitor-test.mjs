import { chromium } from "@playwright/test";
const b = await chromium.launch();
const p = await b.newPage({ viewport: { width: 1280, height: 800 } });
const errs = [];
p.on("pageerror", (e) => errs.push(String(e)));
await p.goto("http://localhost:5173", { waitUntil: "networkidle" });
await p.waitForSelector("canvas");
await p.getByText("solid", { exact: true }).click();          // steady pattern so dead fixtures are obvious
await p.getByText("mock heartbeat", { exact: true }).click();
await p.getByText("monitor", { exact: true }).click();
await p.waitForTimeout(1800);
await p.screenshot({ path: "screenshots/cycle8-monitor.png" });
await b.close();
console.log(errs.length ? "ERRORS:\n" + errs.join("\n") : "monitor + mock enabled, no errors");
