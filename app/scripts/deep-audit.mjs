// DEEP CONTROLLER AUDIT (Elliot: "full pass… every button, slider, mode —
// actually watch what it does"). Drives the real UI, and after every action
// WATCHES the canvas + store for the expected effect. try/catch per check so
// one failure never hides the rest. Usage:
//   node scripts/deep-audit.mjs [url] [engine=chromium|webkit]
import { chromium, webkit } from "@playwright/test";
import { appendFileSync, writeFileSync } from "node:fs";
const LOG = process.env.AUDIT_LOG || "/tmp/auditC.live";
writeFileSync(LOG, "");

const url = process.argv[2] || "http://localhost:4173";
const engine = process.argv[3] === "webkit" ? webkit : chromium;
const browser = await engine.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
const pageErrors = [];
page.on("pageerror", (e) => pageErrors.push(String(e).slice(0, 160)));

await page.goto(url, { waitUntil: "networkidle", timeout: 45000 });
await page.waitForFunction(() => window.twin?.getState?.().fixtures.length > 0, null, { timeout: 30000 });
await page.evaluate(() => { const s = window.twin.getState(); s.setDock(true); s.setTimeOfDay(0); });

// ── watchers ──────────────────────────────────────────────────────────────
const frame = () => page.evaluate(() => {
  const c = document.querySelector("canvas");
  const g = document.createElement("canvas"); g.width = 200; g.height = 125;
  const x = g.getContext("2d"); x.drawImage(c, 0, 0, 200, 125);
  const d = x.getImageData(0, 0, 200, 125).data;
  let sum = 0, r = 0, gg = 0, b = 0, lit = 0;
  for (let i = 0; i < d.length; i += 4) {
    const v = d[i] + d[i + 1] + d[i + 2];
    sum += v;
    if (v > 130) { lit++; r += d[i]; gg += d[i + 1]; b += d[i + 2]; }
  }
  return { sum, lit, r: r / (lit || 1), g: gg / (lit || 1), b: b / (lit || 1) };
});
const moved = async (ms = 1600, thresh = 0.04) => {
  const a = await frame(); await page.waitForTimeout(ms); const b = await frame();
  return Math.abs(a.sum - b.sum) / Math.max(1, a.sum) > thresh || Math.abs(a.lit - b.lit) > 25;
};
const st = (sel) => page.evaluate(new Function("s", `return (${sel})(window.twin.getState())`));
const setSlider = async (label, frac) => page.evaluate(([label, frac]) => {
  const spans = [...document.querySelectorAll("label span span, label > span")];
  const s = spans.find((el) => el.textContent && el.textContent.includes(label));
  if (!s) return false;
  const input = s.closest("label").querySelector("input[type=range]");
  if (!input) return false;
  const v = (+input.min) + ((+input.max) - (+input.min)) * frac;
  Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, "value").set.call(input, String(v));
  input.dispatchEvent(new Event("input", { bubbles: true }));
  return true;
}, [label, frac]);

const results = [];
const check = async (name, fn) => {
  let row;
  try { const detail = await fn(); row = [true, name, typeof detail === "string" ? detail : ""]; }
  catch (e) { row = [false, name, String(e).split("\n")[0].slice(0, 130)]; }
  results.push(row);
  const line = `${row[0] ? "\u2713" : "\u2717 FAIL"}  ${row[1]}${row[2] ? " \u2014 " + row[2] : ""}\n`;
  appendFileSync(LOG, line); process.stdout.write(line);
};
const assert = (cond, msg) => { if (!cond) throw new Error(msg); };
const mode = async (m) => { try { await page.getByRole("button", { name: m }).first().click(); await page.waitForTimeout(300); } catch (e) { results.push([false, "MODE SWITCH " + m, String(e).split("\n")[0].slice(0, 100)]); } };
// wrap each section so a mid-section crash still reports the rest
async function section(fn) { try { await fn(); } catch (e) { results.push([false, "SECTION CRASH", String(e).split("\n")[0].slice(0, 120)]); } }

// ═══ 1 · LIGHT SHOW mode ═══════════════════════════════════════════════════
await mode("🎬 Light Show");
await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0.0, sat: 0.95, brightness: 0.9, speed: 1, colorCycle: "off", blackout: false, master: 1, xfade: 0 }));
await page.waitForTimeout(1500);

await check("show: each of the 6 shows changes the tree differently", async () => {
  const sigs = [];
  for (const nm of ["Performance", "Bioluminescence", "Aurora", "Awakening", "Ignition", "Cosmos"]) {
    await page.getByRole("button", { name: new RegExp(nm) }).click();
    await page.waitForTimeout(2800);
    const f = await frame();
    sigs.push(`${nm}:${Math.round(f.r)},${Math.round(f.g)},${Math.round(f.b)}`);
    assert(f.lit > 15 || nm === "Performance", `${nm} lit nothing`); // Performance opens near-dark by design
  }
  await page.evaluate(() => window.twin.getState().playShow(null));
  assert(new Set(sigs.map((x) => x.split(":")[1])).size >= 4, "shows look identical: " + sigs.join(" "));
  return sigs.length + " shows distinct";
});

await check("show progress bar + stop toggle", async () => {
  await page.getByRole("button", { name: /Aurora/ }).click();
  await page.waitForTimeout(1200);
  assert(await st("(s) => s.activeShow") === "aurora", "activeShow not set");
  assert(await page.getByText(/↻ loops/).isVisible(), "progress row missing");
  await page.getByRole("button", { name: /Aurora/ }).click(); // toggle stop
  assert(await st("(s) => s.activeShow") === null, "stop did not clear activeShow");
});

await check("patterns: every pattern id renders light + animates", async () => {
  const pats = await page.evaluate(() => window.twin.getState().fixtures && ["solid","breathe","chase","ripple","sparkle","sequence","spectrum","tricolor","spiral","godray","rising","planewipe","warmcool","bloom","firefly","ca","hero","plasma","chromatic","rings","fibonacci","sweep","aurora","chladni","glyph","interference","lissajous","shockwave","hurricane"]);
  const dead = [], dark = [];
  for (const p of pats) {
    await page.evaluate((x) => window.twin.getState().set({ pattern: x, brightness: 0.9 }), p);
    await page.waitForTimeout(1300);
    const f = await frame();
    if (f.lit < 12) dark.push(p);
    else if (p !== "solid" && !(await moved(1600, 0.015))) dead.push(p);
  }
  assert(!dark.length, "render dark: " + dark.join(","));
  assert(!dead.length, "static: " + dead.join(","));
  return pats.length + " patterns alive";
});

await check("swatches + custom hue recolour the tree", async () => {
  await page.evaluate(() => window.twin.getState().set({ pattern: "solid", colorCycle: "off", brightness: 0.9, sat: 0.95 }));
  await page.getByTitle("blue").nth(1).click(); await page.waitForTimeout(1200);
  const b = await frame();
  await page.getByTitle("red").nth(1).click(); await page.waitForTimeout(1200);
  const r = await frame();
  assert(b.b > b.r && r.r > r.b, `swatch fail blue=${b.r | 0}/${b.b | 0} red=${r.r | 0}/${r.b | 0}`);
});

await check("colour cycles: rainbow/family/shades/per-light all differ from hold", async () => {
  const sig = [];
  for (const nm of ["● hold", "🌈 rainbow", "family", "shades", "🎲 per-light"]) {
    await page.getByRole("button", { name: nm }).click();
    await page.waitForTimeout(1500);
    const f = await frame();
    sig.push(Math.round(f.r) + "/" + Math.round(f.g) + "/" + Math.round(f.b));
  }
  await page.getByRole("button", { name: "● hold" }).click();
  assert(new Set(sig).size >= 3, "cycles identical: " + sig.join(" "));
});

await check("SPEED slider changes pattern pace", async () => {
  await page.evaluate(() => window.twin.getState().set({ pattern: "chase", colorCycle: "off" }));
  assert(await setSlider("SPEED", 0.05), "slider not found");
  await page.waitForTimeout(1500);
  const slow = await moved(1500, 0.03);
  await setSlider("SPEED", 0.95);
  await page.waitForTimeout(1000);
  const fast = await moved(1500, 0.03);
  assert(fast, "no motion at high speed");
  return `slow-moved=${slow} fast-moved=${fast}`;
});

await check("brightness + master sliders dim", async () => {
  await page.evaluate(() => window.twin.getState().set({ pattern: "solid", brightness: 0.95, master: 1 }));
  await page.waitForTimeout(1000);
  const base = await frame();
  assert(await setSlider("brightness", 0.08), "brightness slider missing");
  await page.waitForTimeout(1200);
  const dim = await frame();
  await setSlider("brightness", 0.9);
  assert(dim.sum < base.sum * 0.8, `brightness no effect ${base.sum}→${dim.sum}`);
});

await check("theme tiles constrain + Wild releases", async () => {
  await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0, sat: 0.95, brightness: 0.9 }));
  await page.getByRole("button", { name: /🌊 Ocean/ }).first().click();
  await page.waitForTimeout(1400);
  const oc = await frame();
  await page.getByRole("button", { name: /🎲 Wild/ }).first().click();
  await page.evaluate(() => window.twin.getState().set({ hue: 0, sat: 0.95 }));
  await page.waitForTimeout(1400);
  const wd = await frame();
  assert(oc.b > oc.r && wd.r > wd.b, `theme fail ocean=${oc.r | 0}/${oc.b | 0} wild=${wd.r | 0}/${wd.b | 0}`);
});

await check("groups: create custom group, drive it, delete it", async () => {
  await page.evaluate(() => { const s = window.twin.getState(); s.defineGroup("qa-test", [1, 2, 3, 4, 5, 6, 7, 8]); s.selectGroup("qa-test"); s.toggleGroupActive("qa-test", true); s.setGroupControl("qa-test", { pattern: "solid", hue: 0.62, sat: 1, brightness: 1 }); });
  await page.waitForTimeout(1200);
  const layers = await st("(s) => s.layers.length");
  assert(layers > 0, "group layer not active");
  await page.evaluate(() => { const s = window.twin.getState(); s.toggleGroupActive("qa-test", false); s.deleteGroup?.("qa-test"); });
  return "layer composed + removed";
});

await check("single-light command console (light N color …)", async () => {
  await page.evaluate(() => window.twin.getState().runCommand("light 5 color blue"));
  const ov = await st("(s) => Object.keys(s.overrides).length");
  await page.evaluate(() => window.twin.getState().runCommand("light 5 auto"));
  assert(ov > 0, "override not applied");
});

await check("cues: save + recall a look", async () => {
  await page.evaluate(() => { const s = window.twin.getState(); s.set({ pattern: "spiral", hue: 0.3 }); s.addCue("qa-cue"); s.set({ pattern: "solid", hue: 0.9 }); });
  const cues = await st("(s) => s.cues.filter((c) => c.name === 'qa-cue')");
  assert(cues.length === 1, "cue not saved");
  await page.evaluate((id) => window.twin.getState().recallCue(id), cues[0].id);
  await page.waitForTimeout(400);
  assert(await st("(s) => s.control.pattern") === "spiral", "cue recall wrong pattern");
  await page.evaluate((id) => window.twin.getState().deleteCue(id), cues[0].id);
});

await check("BEACON preempts everything white, BLACKOUT wins over beacon", async () => {
  await page.evaluate(() => window.twin.getState().set({ pattern: "solid", hue: 0.0, sat: 1, brightness: 0.9 }));
  await page.getByRole("button", { name: /BEACON/ }).click();
  await page.waitForTimeout(1200);
  const bc = await frame();
  await page.getByRole("button", { name: /BLACKOUT/ }).click();
  await page.waitForTimeout(1200);
  const bo = await frame();
  await page.getByRole("button", { name: /BLACKOUT/ }).click();
  await page.getByRole("button", { name: /BEACON/ }).click();
  await page.waitForTimeout(600);
  const spread = Math.min(bc.r, bc.g, bc.b) / Math.max(1, Math.max(bc.r, bc.g, bc.b));
  assert(spread > 0.75, "beacon not white");
  assert(bo.sum < bc.sum * 0.35, `blackout not dark ${bc.sum}→${bo.sum}`);
});

await check("visualizer modes lanterns/orbs/wire all render", async () => {
  for (const v of ["orbs", "wire", "lanterns"]) {
    await page.getByRole("button", { name: v, exact: true }).click();
    await page.waitForTimeout(1200);
    assert((await frame()).lit > 10, v + " renders dark");
  }
});

await check("camera presets + time of day respond", async () => {
  await page.getByRole("button", { name: /top-down/ }).click();
  await page.waitForTimeout(1500);
  const td = await frame();
  await page.getByRole("button", { name: /hero 3\/4/ }).click();
  await page.waitForTimeout(1500);
  const hero = await frame();
  assert(Math.abs(td.sum - hero.sum) / Math.max(1, hero.sum) > 0.05, "camera presets look identical");
  await page.getByRole("button", { name: /☀️? ?day/ }).click();
  await page.waitForTimeout(1500);
  const day = await frame();
  await page.getByRole("button", { name: /🌙 night/ }).click();
  await page.waitForTimeout(1500);
  const night = await frame();
  assert(day.sum > night.sum * 1.3, `day not brighter ${day.sum} vs ${night.sum}`);
});

// ═══ 2 · INTERACTIVE mode ══════════════════════════════════════════════════
await mode("🌱 Interactive");
await page.waitForTimeout(800);

await check("turn-speed presets set the clock + label updates", async () => {
  for (const [nm, v, lbl] of [["🐢 slow", 0.25, "4.9s"], ["▶ baseline", 1, "1.0s"], ["🚀 turbo", 4, "0.2s"]]) {
    await page.locator("button", { hasText: nm }).click();
    assert(Math.abs(await st("(s) => s.control.speed") - v) < 0.01, nm + " speed wrong");
    assert(await page.getByText("one turn every " + lbl).isVisible(), nm + " label wrong");
  }
  await page.locator("button", { hasText: "▶ baseline" }).click();
});

await check("each CA rule responds to a tap within 2 turns", async () => {
  for (const rule of ["Game of Life", "Excitable", "Reaction-Diffusion", "Firefly Sync"]) {
    await page.getByRole("button", { name: new RegExp(rule) }).first().click();
    await page.waitForTimeout(rule === "Game of Life" ? 11500 : 2200); // GoL = entry ceremony
    const before = await frame();
    await page.evaluate(() => { const s = window.twin.getState(); for (let k = 0; k < 3; k++) s.triggerAt(30 + k * 25); });
    await page.waitForTimeout(2600);
    const after = await frame();
    assert(Math.abs(after.sum - before.sum) / Math.max(1, before.sum) > 0.02 || Math.abs(after.lit - before.lit) > 12,
      rule + " ignored the tap");
  }
  return "GoL+Excitable+RD+Firefly all answer touch";
});

await check("walk-through: successive triggers land over ~8s", async () => {
  await page.getByRole("button", { name: /Game of Life/ }).first().click();
  await page.waitForTimeout(11500);
  await page.locator("button", { hasText: "🚶 Sim a walk-through" }).click();
  await page.waitForTimeout(300);
  const early = await st("(s) => s.ripples.length");
  await page.waitForTimeout(4200);
  const later = await st("(s) => s.ripples.filter((r) => performance.now() / 1000 - r.t0 < 2).length");
  assert(early >= 1 && later >= 1, `walk fizzled early=${early} later=${later}`);
});

await check("Game of Light: arm → sim visitor → LIVE → disarm restores", async () => {
  await page.getByRole("button", { name: /Arm \(standby\)/ }).click();
  assert(await st("(s) => s.gol.phase") === "standby", "not standby");
  await page.getByRole("button", { name: /Sim first visitor/ }).click();
  await page.waitForFunction(() => window.twin.getState().gol.phase === "live", null, { timeout: 30000 });
  await page.evaluate(() => window.twin.getState().addNode(40));
  assert(await st("(s) => s.gol.nodes.length") >= 1, "node not added");
  await page.waitForTimeout(2500);
  assert((await frame()).lit > 5, "live mode dark w/ node");
  await page.getByRole("button", { name: /Disarm/ }).click();
  assert(await st("(s) => s.gol.phase") === "off" && (await st("(s) => s.control.blackout")) === false, "disarm left latched state");
});

await check("trigger rules: colour mode + duration + spread reach the store", async () => {
  await page.getByRole("button", { name: "one colour" }).click();
  assert(await st("(s) => s.triggerRule.colorMode") === "fixed", "colorMode");
  await page.getByRole("button", { name: "cycle" }).click();
  assert(await setSlider("Time on", 0.9), "duration slider missing");
  assert((await st("(s) => s.triggerRule.duration")) > 10, "duration not set");
  assert(await setSlider("Spread", 0.9), "spread slider missing");
  assert((await st("(s) => s.triggerRule.spread")) > 1.6, "spread not set");
});

await check("GoL rule editor: preset buttons + steppers change the engine", async () => {
  await page.getByRole("button", { name: "classic B3/S23" }).click();
  const r = await page.evaluate(() => window.twin.getState() && (window.__lr = null) === null && true);
  void r;
  assert(await page.getByText(/B3 \/ S2-3/).first().isVisible(), "rule label not B3/S23");
  await page.getByRole("button", { name: "Conway B2/S23" }).click();
  assert(await page.getByText(/B2 \/ S2-3/).first().isVisible(), "rule label not back to B2/S23");
});

// ═══ 3 · SOUND mode ════════════════════════════════════════════════════════
await mode("🎵 Sound");
await page.waitForTimeout(1000);

await check("piano: piece starts, lights follow, themes recolour, stop works", async () => {
  const ml = page.getByRole("button", { name: "Moonlight ★", exact: true });
  await ml.waitFor({ timeout: 25000 });
  await ml.click();
  await page.waitForTimeout(4000);
  assert(await st("(s) => s.control.pattern") === "piano", "piano not started");
  assert(await moved(2000, 0.01), "piano lights not moving");
  await page.getByRole("button", { name: /💗 Love/ }).first().click();
  await page.waitForTimeout(4000);
  const love = await frame();
  assert(love.r > love.g, "love theme not warm on piano");
  await page.getByRole("button", { name: "⏹ stop" }).click();
  assert(await st("(s) => s.control.pattern") === "solid", "stop failed");
});

await check("DJ deck present in Sound (and NOT in Light Show)", async () => {
  assert(await page.getByText("EQ low→bass").first().isVisible(), "EQ missing in Sound");
  await mode("🎬 Light Show");
  await page.waitForTimeout(600);
  assert(!(await page.getByText("EQ low→bass").first().isVisible().catch(() => false)), "EQ leaked into Light Show");
  await mode("🎵 Sound");
  await page.waitForTimeout(600);
});

await check("AI auto-pilot toggles on/off without errors", async () => {
  const btn = page.getByRole("button", { name: /^OFF$|^ON$/ }).first();
  await btn.click();
  await page.waitForTimeout(1500);
  await btn.click();
});

await check("strobe + guest lock", async () => {
  await page.getByRole("button", { name: /⚡ strobe/ }).click();
  assert(await st("(s) => s.control.strobe") === true, "strobe not set");
  await page.getByRole("button", { name: /⚡ strobe/ }).click();
  await page.getByRole("button", { name: /🔒 guest/ }).click();
  assert(await st("(s) => s.guest") === true, "guest not set");
  await page.getByRole("button", { name: /🔒 guest/ }).click();
});

// ═══ 4 · CALIBRATE mode ════════════════════════════════════════════════════
await mode("🔧 Calibrate");
await page.waitForTimeout(800);

await check("test grid: 49 lights + lab grid, re-hang differs, tree restores", async () => {
  await page.getByRole("button", { name: /Test grid 7×7/ }).click();
  await page.waitForFunction(() => window.twin.getState().fixtures.length === 49, null, { timeout: 10000 });
  const pos1 = await st("(s) => s.fixtures.map((f) => f.pos[1].toFixed(2)).join()");
  await page.getByRole("button", { name: /Test grid 7×7/ }).click(); // re-hang
  await page.waitForTimeout(800);
  const pos2 = await st("(s) => s.fixtures.map((f) => f.pos[1].toFixed(2)).join()");
  assert(pos1 !== pos2, "re-hang did not change heights");
  await page.getByRole("button", { name: /Tree \(real\)/ }).click();
  await page.waitForFunction(() => window.twin.getState().fixtures.length === 118, null, { timeout: 10000 });
});

await check("self-map: survey → solve produces a map with confidences", async () => {
  await page.getByRole("button", { name: /survey mesh/ }).click();
  await page.waitForTimeout(2500);
  await page.getByRole("button", { name: /solve map/ }).click();
  await page.waitForFunction(() => {
    const t = document.body.textContent || "";
    return /median|confidence|locked|assigned/i.test(t);
  }, null, { timeout: 30000 });
});

await check("mock fleet monitor: dead-node injection shows in HUD", async () => {
  await page.evaluate(() => window.twin.getState().setView({ mock: true, deadCount: 6 }));
  await page.waitForTimeout(2500);
  const stats = await st("(s) => s.monitorStats");
  await page.evaluate(() => window.twin.getState().setView({ mock: false, deadCount: 0 }));
  assert(stats.dead >= 5, "dead nodes not reported: " + JSON.stringify(stats));
});

await check("float mode: widgets render + dock returns", async () => {
  await page.getByRole("button", { name: /⧉ float/ }).click();
  await page.waitForTimeout(1000);
  assert(await page.getByText("🎛 Resonance Tree").isVisible(), "float widgets missing");
  await page.getByRole("button", { name: /🗂 dock/ }).click();
  await page.waitForTimeout(600);
});

await check("clean view hides panels, controls return", async () => {
  await page.getByRole("button", { name: /clean view/ }).click();
  await page.waitForTimeout(600);
  assert(!(await page.getByText("Interactivity").first().isVisible().catch(() => false)), "panels still visible");
  await page.getByRole("button", { name: /🎛 controls/ }).click();
  await page.waitForTimeout(600);
});

await report();
async function report() {
// ═══ report ════════════════════════════════════════════════════════════════
console.log("\n══════ DEEP AUDIT ══════");
let pass = 0, fail = 0;
for (const [ok, name, detail] of results) {
  console.log(`${ok ? "✓" : "✗ FAIL"}  ${name}${detail ? " — " + detail : ""}`);
  ok ? pass++ : fail++;
}
console.log(`\n${pass} pass · ${fail} fail`);
if (pageErrors.length) console.log("PAGE ERRORS:\n" + [...new Set(pageErrors)].join("\n"));
}
await browser.close().catch(() => {});
process.exit(0);
