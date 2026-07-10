// LEAN bug pass — the sections NOT already covered by verify-*.mjs, with tight
// bounded timeouts so it can't hang. Writes each result immediately.
// (shows/patterns/controls/themes/CA-speed/grid/GoL are proven by their own
//  verify-*.mjs; this covers interactive lifecycle, sound gating, calibrate,
//  view modes, and safety preempts.)
import { chromium } from "@playwright/test";
import { appendFileSync, writeFileSync } from "node:fs";
const LOG = "/tmp/lean.live"; writeFileSync(LOG, "");
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
page.setDefaultTimeout(8000); // bounded — a stuck click fails fast, never hangs
const errs = [];
page.on("pageerror", (e) => errs.push(String(e).slice(0, 140)));
await page.goto("http://localhost:4173", { waitUntil: "networkidle", timeout: 30000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 20000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); });

const st = (f) => page.evaluate(new Function("s", `return (${f})(window.twin.getState())`));
const frame = () => page.evaluate(() => {
  const c = document.querySelector("canvas"); const g = document.createElement("canvas"); g.width = 200; g.height = 125;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 200, 125); const d = x.getImageData(0, 0, 200, 125).data;
  let sum = 0, lit = 0; for (let i = 0; i < d.length; i += 4) { const v = d[i] + d[i+1] + d[i+2]; sum += v; if (v > 130) lit++; }
  return { sum, lit };
});
const assert = (c, m) => { if (!c) throw new Error(m); };
const mode = async (m) => { await page.getByRole("button", { name: m }).first().click(); await page.waitForTimeout(400); };
const check = async (name, fn) => {
  let row; try { const d = await fn(); row = `✓  ${name}${typeof d === "string" ? " — " + d : ""}`; }
  catch (e) { row = `✗ FAIL  ${name} — ${String(e).split("\n")[0].slice(0, 120)}`; }
  appendFileSync(LOG, row + "\n"); process.stdout.write(row + "\n");
};

// ── INTERACTIVE lifecycle ──
await mode("🌱 Interactive");
await check("BLACKOUT auto-releases when a CA rule is picked", async () => {
  await page.evaluate(() => window.twin.getState().set({ blackout: true }));
  await page.getByRole("button", { name: /Game of Life/ }).first().click();
  await page.waitForTimeout(500);
  assert(await st("(s) => s.control.blackout") === false, "blackout stayed latched");
});
await check("each CA answers a tap (GoL/Excitable/RD/Firefly)", async () => {
  for (const [rule, wait] of [["Game of Life", 11500], ["Excitable", 2200], ["Reaction-Diffusion", 2200], ["Firefly Sync", 2200]]) {
    await page.getByRole("button", { name: new RegExp(rule) }).first().click();
    await page.waitForTimeout(wait);
    const b = await frame();
    await page.evaluate(() => { const s = window.twin.getState(); for (let k = 0; k < 4; k++) s.triggerAt(25 + k * 20); });
    await page.waitForTimeout(2600);
    const a = await frame();
    assert(Math.abs(a.sum - b.sum) / Math.max(1, b.sum) > 0.02 || Math.abs(a.lit - b.lit) > 12, rule + " ignored the tap");
  }
  return "all four respond";
});
await check("Game of Light: arm→visitor→LIVE→disarm no latch", async () => {
  await page.getByRole("button", { name: /Arm \(standby\)/ }).click();
  assert(await st("(s) => s.gol.phase") === "standby", "not standby");
  await page.getByRole("button", { name: /Sim first visitor/ }).click();
  await page.waitForFunction(() => window.twin.getState().gol.phase === "live", null, { timeout: 25000 });
  await page.getByRole("button", { name: /Disarm/ }).click();
  assert(await st("(s) => s.gol.phase") === "off" && !(await st("(s) => s.control.blackout")), "disarm latched");
});
await check("trigger-rule sliders reach the store", async () => {
  await page.getByRole("button", { name: "one colour" }).click();
  assert(await st("(s) => s.triggerRule.colorMode") === "fixed", "colorMode not set");
  await page.getByRole("button", { name: "cycle" }).click();
});

// ── SOUND gating ──
await mode("🎵 Sound");
await check("piano piece starts + stop works", async () => {
  const ml = page.getByRole("button", { name: "Moonlight ★", exact: true });
  await ml.waitFor({ timeout: 25000 });
  await ml.click(); await page.waitForTimeout(2500);
  assert(await st("(s) => s.control.pattern") === "piano", "piano not started");
  await page.getByRole("button", { name: "⏹ stop" }).click();
  assert(await st("(s) => s.control.pattern") === "solid", "stop failed");
});
await check("DJ/EQ present in Sound, absent in Light Show", async () => {
  assert(await page.getByText("EQ low→bass").first().isVisible(), "EQ missing in Sound");
  await mode("🎬 Light Show");
  assert(!(await page.getByText("EQ low→bass").first().isVisible().catch(() => false)), "EQ leaked to Light Show");
  await mode("🎵 Sound");
});

// ── SAFETY preempts ──
await mode("🎬 Light Show");
await check("BEACON white + BLACKOUT beats it", async () => {
  await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0, sat: 1, brightness: 0.9, blackout: false, beaconPreempt: false }));
  await page.waitForTimeout(600);
  await page.getByRole("button", { name: /BEACON/ }).click(); await page.waitForTimeout(900);
  const bc = await frame();
  await page.getByRole("button", { name: /BLACKOUT/ }).click(); await page.waitForTimeout(900);
  const bo = await frame();
  await page.getByRole("button", { name: /BLACKOUT/ }).click();
  await page.getByRole("button", { name: /BEACON/ }).click();
  assert(bo.sum < bc.sum * 0.35, `blackout didn't beat beacon ${bc.sum}->${bo.sum}`);
});

// ── CALIBRATE ──
await mode("🔧 Calibrate");
await check("test grid: 49 lights, re-hang differs, tree restores", async () => {
  await page.getByRole("button", { name: /Test grid 7×7/ }).click();
  await page.waitForFunction(() => window.twin.getState().fixtures.length === 49, null, { timeout: 8000 });
  const a = await st("(s) => s.fixtures.map((f) => f.pos[1].toFixed(2)).join()");
  await page.getByRole("button", { name: /Test grid 7×7/ }).click(); await page.waitForTimeout(600);
  const b = await st("(s) => s.fixtures.map((f) => f.pos[1].toFixed(2)).join()");
  assert(a !== b, "re-hang identical");
  await page.getByRole("button", { name: /Tree \(real\)/ }).click();
  await page.waitForFunction(() => window.twin.getState().fixtures.length === 118, null, { timeout: 8000 });
});
await check("self-map survey→solve produces a result", async () => {
  await page.getByRole("button", { name: /survey mesh/ }).click(); await page.waitForTimeout(2000);
  await page.getByRole("button", { name: /solve map/ }).click();
  await page.waitForFunction(() => /median|confidence|locked|assigned|✓/i.test(document.body.textContent || ""), null, { timeout: 25000 });
});

// ── VIEW modes ──
await check("float widgets ⇄ dock", async () => {
  await page.getByRole("button", { name: /⧉ float/ }).click(); await page.waitForTimeout(800);
  assert(await page.getByText("🎛 Resonance Tree").first().isVisible(), "float widgets missing");
  await page.getByRole("button", { name: /🗂 dock/ }).click(); await page.waitForTimeout(500);
});
await check("clean view hides panels, controls return", async () => {
  await page.getByRole("button", { name: /clean view/ }).click(); await page.waitForTimeout(500);
  assert(!(await page.getByText("Light Shows").first().isVisible().catch(() => false)), "panels still shown");
  await page.getByRole("button", { name: /🎛 controls/ }).click(); await page.waitForTimeout(500);
});
await check("flight recorder captures inputs + keyframes", async () => {
  await mode("🌱 Interactive");
  await page.evaluate(() => { const s = window.twin.getState(); s.setCaTheme("ocean"); for (let k = 0; k < 4; k++) s.triggerAt(20 + k * 15); });
  await page.waitForTimeout(3000);
  const c = await page.getByText(/ev · .*kf/).first().innerText();
  const m = c.match(/(\d+)ev · (\d+)kf/);
  assert(m && +m[1] >= 4 && +m[2] >= 2, "recorder not capturing: " + c);
  return c.trim();
});

appendFileSync(LOG, "DONE\n");
if (errs.length) appendFileSync(LOG, "PAGE ERRORS: " + [...new Set(errs)].join(" | ") + "\n");
await browser.close();
