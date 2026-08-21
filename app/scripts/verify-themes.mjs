// Headless proof of the 2026-07-08 batch: piano follows colour themes, themes
// constrain light-show patterns, sustained bright white is capped, sound
// components live on the Sound page, shows sit on top of Light Show.
import { chromium } from "@playwright/test";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto(process.argv[2] || "http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); });
const sample = (thr = 120) => page.evaluate((thr) => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 320; g.height = 200;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 320, 200);
  const d = x.getImageData(0, 0, 320, 200).data;
  let r = 0, gg = 0, b = 0, lit = 0, mx = 0;
  for (let i = 0; i < d.length; i += 4) {
    const v = d[i] + d[i+1] + d[i+2];
    if (v > thr) { lit++; r += d[i]; gg += d[i+1]; b += d[i+2]; mx = Math.max(mx, d[i], d[i+1], d[i+2]); }
  }
  return { lit, r: r / (lit || 1), g: gg / (lit || 1), b: b / (lit || 1), mx };
}, thr);
const fails = [];
const check = (name, ok, detail) => { console.log(`${ok ? "✓" : "✗"} ${name} ${detail}`); if (!ok) fails.push(name); };

// ── SOUND PAGE: piano panel present w/ themes; DJ/EQ present ──
await page.getByRole("button", { name: "🎵 Sound" }).click();
check("Sound page has 🎹 Piano panel", await page.getByText("72 keys · one light per key").isVisible(), "");
check("Sound page has EQ (sound components home)", await page.getByText("EQ low→bass").first().isVisible().catch(() => false), "");
const elise = page.getByRole("button", { name: "Für Elise ★", exact: true });
await elise.waitFor({ timeout: 40000 }); // SW precache install can delay the manifest ~8s on first load
// Ocean theme → Für Elise in blues
await page.getByRole("button", { name: "🌊 Ocean" }).first().click();
await elise.click();
await page.waitForTimeout(6000);
let ocean = await sample(330); // bright note pixels only — the warm bark pollutes low thresholds
for (let k = 0; k < 6 && ocean.lit < 40; k++) { await page.waitForTimeout(1200); ocean = await sample(330); } // notes are transient — sample until some sound
// Love theme → pinks/reds
await page.getByRole("button", { name: "💗 Love" }).first().click();
await page.waitForTimeout(6000);
const love = await sample(330);
check("Für Elise + Ocean theme = blue/cyan notes", ocean.lit > 40 && ocean.b > ocean.r * 1.1,
  `rgb ${ocean.r.toFixed(0)}/${ocean.g.toFixed(0)}/${ocean.b.toFixed(0)} (lit ${ocean.lit})`);
// love anchors are pink/purple/red — magenta family: strong red+blue, LOW green
check("Für Elise + Love theme = warm pink notes (not blue/green)", love.lit > 40 && love.r > love.g * 1.5 && love.r > love.b,
  `rgb ${love.r.toFixed(0)}/${love.g.toFixed(0)}/${love.b.toFixed(0)} (lit ${love.lit})`);
await page.evaluate(() => window.twin.getState().set({ pattern: "solid" }));

// ── LIGHT SHOW PAGE: shows on top, themes in COLOR box, DJ/EQ gone ──
await page.getByRole("button", { name: "🎬 Light Show" }).click();
await page.waitForTimeout(800);
const firstSection = await page.evaluate(() => {
  const panel = document.querySelectorAll("[style*='overflow-y']");
  for (const p of panel) { const t = p.textContent || ""; const iShows = t.indexOf("Light Shows"); const iGroups = t.indexOf("Groups"); if (iShows >= 0 && iGroups >= 0) return iShows < iGroups; }
  return false;
});
check("Light Show: shows section on TOP", firstSection, "");
check("Light Show: DJ/EQ hidden (moved to Sound)", !(await page.getByText("EQ low→bass").first().isVisible().catch(() => false)), "");
check("Light Show: THEME picker in COLOR box", await page.getByText("🎭 THEME").isVisible(), "");
// theme constrains a light-show pattern: solid red + Ocean → blue-shifted
await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0.0, sat: 0.95, brightness: 0.9, colorCycle: "off" }));
await page.getByRole("button", { name: "🌊 Ocean" }).first().click();
await page.waitForTimeout(1500);
const themed = await sample();
check("Ocean theme pulls a red solid into blues", themed.b > themed.r, `rgb ${themed.r.toFixed(0)}/${themed.g.toFixed(0)}/${themed.b.toFixed(0)}`);
// Wild = unconstrained again
await page.getByRole("button", { name: "🎲 Wild" }).first().click();
// picking a theme also moves the BASE hue (by design) — restore red, then Wild
// must let it render RED (constraint released)
await page.evaluate(() => window.twin.getState().set({ hue: 0.0, sat: 0.95 }));
await page.waitForTimeout(2000);
const wild = await sample();
check("Wild releases the constraint (red renders red)", wild.r > wild.b * 1.3, `rgb ${wild.r.toFixed(0)}/${wild.g.toFixed(0)}/${wild.b.toFixed(0)}`);

// ── WHITE CAP: capped white must be clearly dimmer than the beacon's REAL full white ──
await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0, sat: 0, brightness: 1, master: 1 }));
await page.waitForTimeout(2000);
const white = await sample(400); // count of really-bright pixels
await page.evaluate(() => window.twin.getState().set({ beaconPreempt: true }));
await page.waitForTimeout(2000);
const beacon = await sample(400);
await page.evaluate(() => window.twin.getState().set({ beaconPreempt: false }));
check("sustained white is capped well below the beacon's full white", white.lit < beacon.lit * 0.55, `bright-px white ${white.lit} vs beacon ${beacon.lit}`);

await page.screenshot({ path: "screenshots/themes-audit.png" });
await browser.close();
if (fails.length) { console.error("FAILED:", fails.join(" · ")); process.exit(1); }
console.log("ALL THEME/LAYOUT/WHITE CHECKS PASS");
