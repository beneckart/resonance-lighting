// Headless audit: do the SLIDERS actually change the lights? Do swatches/colour
// work? Do pre-designed shows drive the control plane over time?
import { chromium } from "@playwright/test";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto(process.argv[2] || "http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); s.set({ pattern: "solid", brightness: 0.9, sat: 0.9, hue: 0.0, colorCycle: "off", master: 1 }); });
await page.getByRole("button", { name: "🎬 Light Show" }).click();
await page.waitForTimeout(2500);

const sample = () => page.evaluate(() => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 320; g.height = 200;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 320, 200);
  const d = x.getImageData(0, 0, 320, 200).data;
  let sum = 0, r = 0, gg = 0, b = 0, lit = 0;
  for (let i = 0; i < d.length; i += 4) {
    const v = d[i] + d[i+1] + d[i+2];
    sum += v;
    if (v > 120) { lit++; r += d[i]; gg += d[i+1]; b += d[i+2]; }
  }
  return { mean: sum / (d.length / 4), lit, r: r / (lit || 1), g: gg / (lit || 1), b: b / (lit || 1) };
});
const setRange = (label, val) => page.evaluate(([label, val]) => {
  const spans = [...document.querySelectorAll("label span span")];
  const s = spans.find((el) => el.textContent.includes(label));
  if (!s) return "NO SLIDER: " + label;
  const input = s.closest("label").querySelector("input[type=range]");
  Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, "value").set.call(input, String(val));
  input.dispatchEvent(new Event("input", { bubbles: true }));
  return "ok";
}, [label, val]);

const fails = [];
const check = (name, ok, detail) => { console.log(`${ok ? "✓" : "✗"} ${name} ${detail}`); if (!ok) fails.push(name); };

// 1 · brightness slider → pixels actually dim
const base = await sample();
console.log("baseline:", JSON.stringify(base));
console.log(await setRange("brightness", 0.12));
await page.waitForTimeout(1500);
const dim = await sample();
check("brightness slider dims the tree", dim.mean < base.mean * 0.75, `mean ${base.mean.toFixed(1)}→${dim.mean.toFixed(1)}`);
await setRange("brightness", 0.95);
await page.waitForTimeout(1200);

// 2 · master slider → dims
await setRange("master", 0.15);
await page.waitForTimeout(1500);
const dimM = await sample();
check("master slider dims the tree", dimM.mean < base.mean * 0.8, `mean ${base.mean.toFixed(1)}→${dimM.mean.toFixed(1)}`);
await setRange("master", 1);
await page.waitForTimeout(1200);

// 3 · colour swatches → lit-pixel colour actually changes
await page.getByTitle("blue").nth(1).click(); // nth(1) = the GLOBAL Controls swatch (nth(0) is the group-look editor)
await page.waitForTimeout(1500);
const blue = await sample();
check("blue swatch turns lit pixels blue", blue.b > blue.r * 1.3, `rgb ${blue.r.toFixed(0)}/${blue.g.toFixed(0)}/${blue.b.toFixed(0)}`);
await page.getByTitle("red").nth(1).click();
await page.waitForTimeout(1500);
const red = await sample();
check("red swatch turns lit pixels red", red.r > red.b * 1.3, `rgb ${red.r.toFixed(0)}/${red.g.toFixed(0)}/${red.b.toFixed(0)}`);

// 4 · hue slider (store + pixels)
await setRange("hue", 0.33);
await page.waitForTimeout(1500);
const green = await sample();
const hueStore = await page.evaluate(() => window.twin.getState().control.hue);
check("hue slider → store", Math.abs(hueStore - 0.33) < 0.01, `hue=${hueStore}`);
check("hue slider → green pixels", green.g > green.r * 1.15 && green.g > green.b * 1.15, `rgb ${green.r.toFixed(0)}/${green.g.toFixed(0)}/${green.b.toFixed(0)}`);

// 5 · saturation slider → white-ish
await setRange("saturation", 0.02);
await page.waitForTimeout(1500);
const white = await sample();
const spread = Math.max(white.r, white.g, white.b) / Math.max(1, Math.min(white.r, white.g, white.b));
check("saturation→0 gives white light", spread < 1.35, `rgb ${white.r.toFixed(0)}/${white.g.toFixed(0)}/${white.b.toFixed(0)}`);

// 6 · SPEED slider reaches the store
await setRange("SPEED", 2.4);
const spd = await page.evaluate(() => window.twin.getState().control.speed);
check("speed slider → store", Math.abs(spd - 2.4) < 0.01, `speed=${spd}`);

// 7 · shows drive the control plane over time, and differ from each other
await page.evaluate(() => window.twin.getState().set({ brightness: 0.9, sat: 0.9 }));
const showTrace = async (name) => {
  await page.getByRole("button", { name: new RegExp(name) }).click();
  const pats = [];
  for (let i = 0; i < 5; i++) { pats.push(await page.evaluate(() => window.twin.getState().control.pattern)); await page.waitForTimeout(2500); }
  return pats;
};
const aurora = await showTrace("Aurora");
const ignition = await showTrace("Ignition");
await page.evaluate(() => window.twin.getState().playShow(null));
check("Aurora show sets its own patterns", aurora.some((p) => p !== "solid"), aurora.join(","));
check("Ignition differs from Aurora", ignition.join() !== aurora.join(), ignition.join(","));

// 8 · GROUP look layer: activate a group, give it a colour — its lights change
await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0.0, sat: 0.9, brightness: 0.9, colorCycle: "off" }));
await page.waitForTimeout(1200);
const preGroup = await sample();
await page.getByRole("button", { name: /ring1/ }).nth(1).click().catch(() => page.getByRole("button", { name: /ring1/ }).first().click()); // group selector inside GroupPanel
await page.getByRole("button", { name: "○ off" }).click(); // toggle the group LIVE
await page.getByTitle("blue").first().click(); // the group-look swatch
await page.waitForTimeout(1500);
const grp = await sample();
check("group look layer recolours part of the tree", grp.b > preGroup.b * 1.15 || Math.abs(grp.r - preGroup.r) > 8, `rgb ${preGroup.r.toFixed(0)}/${preGroup.g.toFixed(0)}/${preGroup.b.toFixed(0)} → ${grp.r.toFixed(0)}/${grp.g.toFixed(0)}/${grp.b.toFixed(0)}`);
await page.getByRole("button", { name: "● live" }).click(); // back off

await page.screenshot({ path: "screenshots/controls-audit.png" });
await browser.close();
if (fails.length) { console.error("FAILED:", fails.join(" · ")); process.exit(1); }
console.log("ALL CONTROLS PASS");
