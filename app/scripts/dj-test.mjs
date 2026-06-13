import { chromium } from "@playwright/test";
const b = await chromium.launch();
const p = await b.newPage({ viewport: { width: 1280, height: 800 } });
const errs=[]; p.on("pageerror",e=>errs.push(String(e)));
await p.goto("http://localhost:5173",{waitUntil:"networkidle"});
await p.waitForSelector("canvas");
await p.getByText("spectrum",{exact:true}).click(); // look A = spectrum
// drag crossfade to ~0.7 toward look B (ripple)
const slider = p.locator('input[type=range]').nth(7); // crossfade is after pattern/seq/sliders
await p.waitForTimeout(800);
await p.screenshot({ path:"screenshots/cycle18-dj.png" });
await b.close();
console.log(errs.length?"ERRORS:\n"+errs.join("\n"):"dj panel ok");
