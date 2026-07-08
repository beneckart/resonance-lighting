// Headless proof (Elliot: "you were supposed to test all these sliders"):
// the SPEED dial must visibly change the PACE of every CA engine — measured as
// pixel-change rate between frames, slow dial vs fast dial. Plus: blackout
// releases on mode click; group theme chips persist; chandelier is warm in shows.
import { chromium } from "@playwright/test";
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
await page.goto(process.argv[2] || "http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); });
await page.waitForTimeout(2000);

const grab = () => page.evaluate(() => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 240; g.height = 150;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 240, 150);
  return Array.from(x.getImageData(0, 0, 240, 150).data);
});
// count MACRO changes (>30 per channel) — bloom shimmer redraws everything
// slightly every frame regardless of the dial; the dial moves the PATTERN
const diff = (a, b) => { let d = 0; for (let i = 0; i < a.length; i += 4) { if (Math.abs(a[i] - b[i]) > 30) d++; if (Math.abs(a[i+1] - b[i+1]) > 30) d++; if (Math.abs(a[i+2] - b[i+2]) > 30) d++; } return d; };
const paceAt = async (speed) => {
  await page.evaluate((sp) => window.twin.getState().set({ speed: sp }), speed);
  await page.waitForTimeout(2500); // let the engine settle at the new pace
  // excite RIGHT before sampling — at speed 3 a wave crosses the whole graph in
  // ~4s and burns out, so exciting earlier leaves nothing to measure
  await page.evaluate(() => { const s = window.twin.getState(); if (s.control.pattern === "ripples") for (let k = 0; k < 4; k++) s.triggerAt((Math.random() * s.fixtures.length) | 0); });
  // the tap ALSO fires a 3s visual ripple overlay that animates at any dial —
  // wait it out so we measure the GH medium itself. At glacial the excited
  // cells then sit frozen-lit (no tick for ~5.6s); at speed 3 the wave +
  // refractory tail are still evolving.
  const isRipples = await page.evaluate(() => window.twin.getState().control.pattern === "ripples");
  await page.waitForTimeout(isRipples ? 3400 : 600);
  const f1 = await grab(); await page.waitForTimeout(2000); const f2 = await grab();
  return diff(f1, f2);
};
const fails = [];
const check = (name, ok, detail) => { console.log(`${ok ? "✓" : "✗"} ${name} ${detail}`); if (!ok) fails.push(name); };

for (const rule of ["living", "organism", "ripples"]) {
  await page.evaluate((r) => window.twin.getState().set({ pattern: r, brightness: 0.9, blackout: false }), rule);
  if (rule === "ripples") await page.evaluate(() => { const s = window.twin.getState(); for (let k = 0; k < 4; k++) s.triggerAt((Math.random() * s.fixtures.length) | 0); });
  await page.waitForTimeout(2000);
  const slow = await paceAt(0.05);
  const fast = await paceAt(3);
  check(`${rule}: speed dial changes the pace`, fast > slow * 2, `Δslow ${slow} vs Δfast ${fast} (${(fast / Math.max(1, slow)).toFixed(1)}×)`);
}

// ── blackout releases when a mode is picked ──
await page.evaluate(() => window.twin.getState().set({ blackout: true }));
await page.waitForTimeout(1200);
const dark = await grab();
const darkSum = dark.reduce((a, v, i) => (i % 4 < 3 ? a + v : a), 0) / (dark.length * 0.75);
await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0.3, sat: 0.9, brightness: 0.9 }));
await page.waitForTimeout(1500);
const bo = await page.evaluate(() => window.twin.getState().control.blackout);
const back = await grab();
const backSum = back.reduce((a, v, i) => (i % 4 < 3 ? a + v : a), 0) / (back.length * 0.75);
check("blackout releases on mode click (store)", bo === false, `blackout=${bo}`);
check("lights visibly return after mode click", backSum > darkSum * 1.5, `dark ${darkSum.toFixed(1)} → back ${backSum.toFixed(1)}`);

// ── group theme chip persists per group ──
await page.getByRole("button", { name: "🎬 Light Show" }).click();
await page.getByRole("button", { name: /ring1/ }).last().click().catch(() => {});
await page.getByRole("button", { name: "💗 Love" }).first().click(); // group editor chip (GroupPanel section)
const gt = await page.evaluate(() => window.twin.getState().groupThemes);
check("group theme chip sets the group's theme", Object.values(gt).includes("love"), JSON.stringify(gt));

// ── chandelier is WARM during Performance phase 0 (no white core) ──
await page.evaluate(() => window.twin.getState().set({ blackout: false }));
await page.getByRole("button", { name: /Performance/ }).click();
await page.waitForTimeout(9000); // phase 0: chandelier breathe layer active
const bright = await page.evaluate(() => {
  // sample ONLY the trunk interior (where the chandelier hangs) — the sky
  // sparkle cue would otherwise dominate the bright pixels
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 240; g.height = 150;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 240, 150);
  const d = x.getImageData(0, 0, 240, 150).data;
  let r = 0, b = 0, lit = 0, whiteish = 0;
  for (let py = 50; py < 115; py++) for (let px = 85; px < 155; px++) {
    const i = (py * 240 + px) * 4;
    const mx = Math.max(d[i], d[i+1], d[i+2]);
    if (mx > 140) { lit++; r += d[i]; b += d[i+2]; const mn = Math.min(d[i], d[i+1], d[i+2]); if (mn / mx > 0.82) whiteish++; }
  }
  return { lit, r: r / (lit || 1), b: b / (lit || 1), whiteish };
});
await page.screenshot({ path: "screenshots/performance-phase0.png" });
// a SOLID WHITE MASS (the old bug) makes most bright pixels neutral (~0.8+);
// colourful lights with bloom cores sit ~0.3-0.5. Threshold splits the two.
check("no solid white mass in the trunk interior (see performance-phase0.png)", bright.lit > 0 && bright.whiteish < bright.lit * 0.6,
  `lit ${bright.lit} · r ${bright.r.toFixed(0)} vs b ${bright.b.toFixed(0)} · whiteish ${bright.whiteish}`);
await page.evaluate(() => window.twin.getState().playShow(null));

await page.screenshot({ path: "screenshots/ca-speed-audit.png" });
await browser.close();
if (fails.length) { console.error("FAILED:", fails.join(" · ")); process.exit(1); }
console.log("ALL CA-SPEED / BLACKOUT / GROUP-THEME / CHANDELIER CHECKS PASS");
