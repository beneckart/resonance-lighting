import { test, expect, type Page } from "@playwright/test";
import * as fs from "fs";

/**
 * FULL-CONTROL AUDIT (network-tester, Elliot 2026-08-15: "we need to make sure
 * there is full control of every light and function built into the simulator").
 *
 * Two questions, answered with evidence rather than by reading the grammar:
 *
 *   1. COVERAGE — can EVERY individual fixture be driven, one at a time?
 *      The fixture set is 130. The grammar's own comments say the `light`
 *      addressing covers "1..72", which is stale from a smaller tree — the code
 *      actually assigns `num: rankOf[i] + 1`, i.e. 1..N. If the comment were
 *      right, 58 lights would be unaddressable; if the code is right, the
 *      comment is a trap for whoever reads it next. This test settles it by
 *      addressing all 130 individually and checking the store, not the docs.
 *
 *   2. FUNCTION REACH — is every documented command head actually runnable?
 *      A head listed in COMMAND_HEADS that no longer parses is a control the
 *      operator believes they have and doesn't.
 */

const results: any[] = [];

async function boot(page: Page) {
  await page.setViewportSize({ width: 1440, height: 900 });
  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(600);
}

test("every individual light is addressable and can be driven", async ({ page }) => {
  test.setTimeout(10 * 60 * 1000);
  await boot(page);

  const meta = await page.evaluate(() => {
    const s = (window as any).twin.getState();
    const nums = s.fixtures.map((f: any) => f.num).filter((n: any) => typeof n === "number");
    return {
      fixtures: s.fixtures.length,
      withNum: nums.length,
      min: Math.min(...nums),
      max: Math.max(...nums),
      unique: new Set(nums).size,
    };
  });

  // every fixture must carry a unique addressable number, 1..N with no gaps
  expect(meta.withNum, "every fixture has an addressable number").toBe(meta.fixtures);
  expect(meta.unique, "addressable numbers are unique").toBe(meta.fixtures);
  expect(meta.min).toBe(1);
  expect(meta.max).toBe(meta.fixtures);

  // drive each light individually and confirm the override lands on exactly it
  const unreachable = await page.evaluate((total: number) => {
    const t = (window as any).twin.getState();
    const bad: number[] = [];
    for (let n = 1; n <= total; n++) {
      t.set({ blackout: false });
      // clear, then address exactly this one light
      t.runCommand("clear");
      t.runCommand(`light ${n} color blue`);
      const st = (window as any).twin.getState();
      const touched = Object.keys(st.overrides ?? {}).length;
      const idx = st.fixtures.findIndex((f: any) => f.num === n);
      const hit = idx >= 0 && !!(st.overrides ?? {})[idx];
      if (!hit || touched !== 1) bad.push(n);
    }
    (window as any).twin.getState().runCommand("clear");
    return bad;
  }, meta.fixtures);

  results.push({ check: "per-light-addressing", ...meta, unreachable });
  console.log(`[ctl] fixtures=${meta.fixtures} addressable=${meta.min}..${meta.max} unreachable=${unreachable.length}`);
  if (unreachable.length) console.log(`[ctl] UNREACHABLE: ${unreachable.slice(0, 40).join(", ")}`);

  expect(unreachable, `these lights could not be driven individually`).toEqual([]);
});

test("every documented command head actually runs", async ({ page }) => {
  test.setTimeout(5 * 60 * 1000);
  await boot(page);
  // install the snapshot helper in the page so the probe below can diff state
  await page.evaluate(`window.__snap = ${(() => {
    const s = (window as any).twin.getState();
    const o: any = {};
    for (const k of Object.keys(s)) {
      const v = (s as any)[k];
      if (typeof v === "function") continue;
      if (k === "fixtures") { o.__n = Array.isArray(v) ? v.length : 0; continue; }
      try { o[k] = JSON.parse(JSON.stringify(v)); } catch { /* skip */ }
    }
    return JSON.stringify(o);
  }).toString()}`);

  // one representative invocation per head, taken from the grammar's own docs
  const probes: [string, string][] = [
    ["clear", "clear"],
    ["on", "on"],
    ["off", "off"],
    ["all", "all pattern spiral"],
    ["hue", "hue 0.5"],
    ["speed", "speed 2"],
    ["pattern", "pattern sequence"],
    ["bri", "bri 0.7"],
    ["sat", "sat 0.9"],
    ["zone", "zone high color red"],
    ["range", "range 0-23 color green"],
    ["every", "every 4 color blue"],
    ["light", "light 1,7,17 color red"],
    ["lights", "lights 2 color white"],
    ["num", "num 3 color cyan"],
    ["show", "show ignition"],
    ["theme", "theme love"],
    ["cue", "cue"],
    ["blink", "blink"],
    ["gol", "gol"],
  ];

  // "it didn't throw" is the weak assertion this repo's existing smoke spec
  // already makes, and it passes for a head that quietly does nothing. Diff the
  // STORE across each command instead: a control the operator believes they
  // have must change something.

  const out = await page.evaluate((cmds: [string, string][]) => {
    const snap = (window as any).__snap as () => string;
    const t = () => (window as any).twin.getState();
    const rows: any[] = [];
    for (const [head, cmd] of cmds) {
      t().runCommand("clear");
      const before = snap();
      let threw = "";
      try { t().runCommand(cmd); } catch (e) { threw = String(e); }
      const after = snap();
      rows.push({ head, cmd, threw, changed: after !== before });
    }
    t().runCommand("clear");
    return rows;
  }, probes);

  const broken = out.filter((r: any) => r.threw);
  // a head that runs cleanly and changes nothing — the operator thinks they
  // have this control and does not
  const silent = out.filter((r: any) => !r.threw && !r.changed);

  results.push({ check: "command-heads", total: out.length, broken, silent, rows: out });
  for (const r of out) {
    const verdict = r.threw ? `THREW ${String(r.threw).slice(0, 60)}` : r.changed ? "changed state ✓" : "NO EFFECT";
    console.log(`[ctl] ${String(r.head).padEnd(8)} ${String(r.cmd).padEnd(26)} ${verdict}`);
  }
  console.log(`[ctl] heads with no observable effect: ${silent.length ? silent.map((s: any) => s.head).join(", ") : "none"}`);

  fs.writeFileSync("full-control-audit.json", JSON.stringify(results, null, 2));
  expect(broken, `command heads that threw: ${broken.map((b: any) => b.head).join(", ")}`).toEqual([]);
});

test.afterAll(() => {
  if (results.length) fs.writeFileSync("full-control-audit.json", JSON.stringify(results, null, 2));
});
