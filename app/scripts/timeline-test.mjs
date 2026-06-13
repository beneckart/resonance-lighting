import { chromium } from "@playwright/test";
const b = await chromium.launch();
const p = await b.newPage({ viewport: { width: 1280, height: 950 } });
const errs=[]; p.on("pageerror",e=>errs.push(String(e)));
await p.addInitScript(()=>localStorage.clear());
await p.goto("http://localhost:5173",{waitUntil:"networkidle"});
await p.waitForSelector("canvas");
// save two cues then play the timeline
await p.getByRole("button",{name:"spectrum",exact:true}).click();
await p.getByPlaceholder("cue name").fill("A"); await p.getByText("💾 save",{exact:false}).click();
await p.getByRole("button",{name:"solid",exact:true}).click();
await p.getByPlaceholder("cue name").fill("B"); await p.getByText("💾 save",{exact:false}).click();
await p.getByText("▶ play timeline",{exact:false}).click();
await p.waitForTimeout(1500);
await p.screenshot({ path:"screenshots/cycle26-timeline.png" });
await b.close();
console.log(errs.length?"ERRORS:\n"+errs.join("\n"):"timeline ok");
