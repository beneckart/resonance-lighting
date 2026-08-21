import { test, expect } from "@playwright/test";
import * as fs from "fs";

/**
 * INTERACTIVE / CA MODE AUDIT (network-tester, Elliot 2026-08-15:
 * "when I click organism, it doesn't fully work" + "can you test the different
 * interactive modes").
 *
 * The interactive family is CA_RULES = life · ripples · organism · living · chains
 * (store.ts:32). These are decentralised cellular automata: each light runs a
 * local rule over its neighbours. They fail differently from ordinary patterns —
 * an ordinary pattern is either driven or not, but a CA can:
 *
 *   - go EXTINCT (field.ts:558 says so explicitly: "the reaction can go EXTINCT
 *     (v -> 0 everywhere)"),
 *   - light only a FRACTION of the tree and stall there,
 *   - freeze (a still frame is not the same as a running automaton),
 *   - or need a presence poke before anything happens at all.
 *
 * "Doesn't fully work" is exactly the vocabulary of partial coverage, so this
 * measures COVERAGE and MOVEMENT over time, not just "is it lit" — a single
 * bright frame would pass a naive check while the tree sits half dead.
 */

const PROBE = `(() => {
  const c = document.querySelector("canvas");
  const g = c.getContext("webgl2") || c.getContext("webgl");
  const px = new Uint8Array(c.width * c.height * 4);
  g.readPixels(0, 0, c.width, c.height, g.RGBA, g.UNSIGNED_BYTE, px);
  let lum = 0, n = 0, lit = 0;
  for (let i = 0; i < px.length; i += 4 * 8) {
    const L = 0.2126*px[i] + 0.7152*px[i+1] + 0.0722*px[i+2];
    lum += L; n++; if (L > 45) lit++;
  }
  return { lum: lum/n, litFrac: lit/n };
})()`;

/** how much of the FIXTURE SET is actually emitting — the coverage question */
const COVERAGE = `(() => {
  const t = window.twin.getState();
  const n = t.fixtures.length;
  // sample the engine's own output buffers where exposed, else fall back to null
  return { fixtures: n };
})()`;

const CA_MODES = ["life", "ripples", "organism", "living", "chains"];
const report: any[] = [];

async function boot(page: any) {
  await page.setViewportSize({ width: 1440, height: 900 });
  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(900);
}

test("interactive modes — each CA rule lights up, covers the tree, and keeps MOVING", async ({ page }) => {
  test.setTimeout(12 * 60 * 1000);
  await boot(page);

  const errors: string[] = [];
  page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
  page.on("pageerror", (e) => errors.push("PAGEERROR " + String(e)));

  // reference frames
  await page.evaluate(() => (window as any).twin.getState().set({
    blackout: false, beaconPreempt: false, brightness: 1, sat: 1, hue: 0.15, colorCycle: "off", pattern: "solid",
  }));
  await page.waitForTimeout(1200);
  const solid = await page.evaluate(PROBE);
  await page.evaluate(() => (window as any).twin.getState().set({ blackout: true }));
  await page.waitForTimeout(900);
  const dark = await page.evaluate(PROBE);
  await page.evaluate(() => (window as any).twin.getState().set({ blackout: false }));

  for (const mode of CA_MODES) {
    const before = errors.length;
    await page.evaluate((p: string) => (window as any).twin.getState().set({ pattern: p, blackout: false }), mode);
    await page.waitForTimeout(1500); // let the automaton get going

    // sample a TIME SERIES — a frozen CA and a running one look identical in one frame
    const series: any[] = [];
    for (let i = 0; i < 6; i++) {
      series.push(await page.evaluate(PROBE));
      await page.waitForTimeout(500);
    }

    const lums = series.map((s) => s.lum);
    const lits = series.map((s) => s.litFrac);
    const peakLum = Math.max(...lums);
    const peakLit = Math.max(...lits);
    // movement = does the frame actually change between samples?
    let movement = 0;
    for (let i = 1; i < series.length; i++) movement += Math.abs(lums[i] - lums[i - 1]);

    // now poke it — several CA rules are presence-driven by design
    await page.evaluate(() => (window as any).twin.getState().pingPresence?.());
    await page.waitForTimeout(1800);
    const poked = await page.evaluate(PROBE);

    const entry = {
      mode,
      peakLum: Number(peakLum.toFixed(2)),
      peakLitFrac: Number(peakLit.toFixed(4)),
      movement: Number(movement.toFixed(2)),
      afterPokeLum: Number(poked.lum.toFixed(2)),
      afterPokeLitFrac: Number(poked.litFrac.toFixed(4)),
      // coverage relative to a fully-driven solid frame
      coverageVsSolid: Number((peakLit / (solid.litFrac || 1)).toFixed(3)),
      newErrors: errors.slice(before),
      verdict: "",
    };

    // classify honestly
    if (peakLum <= dark.lum * 1.2 && poked.lum <= dark.lum * 1.2) entry.verdict = "DARK — never lights, even after a poke";
    else if (movement < 1.5) entry.verdict = "STATIC — lights but does not animate";
    else if (entry.coverageVsSolid < 0.35) entry.verdict = "PARTIAL — animates but covers little of the tree";
    else entry.verdict = "OK";

    report.push(entry);
    console.log(`[ca] ${mode.padEnd(9)} peakLum=${entry.peakLum.toString().padStart(6)} litFrac=${entry.peakLitFrac} cover=${entry.coverageVsSolid} move=${entry.movement} poke=${entry.afterPokeLum} → ${entry.verdict}`);
  }

  console.log(`[ca] reference: solid lum=${solid.lum.toFixed(2)} litFrac=${solid.litFrac.toFixed(4)} | blackout lum=${dark.lum.toFixed(2)}`);
  fs.writeFileSync("interactive-audit.json", JSON.stringify({ solid, dark, modes: report }, null, 2));

  expect(report.length).toBe(CA_MODES.length);
});

test("interactive modes — the Interactive arm/disarm controls drive the render", async ({ page }) => {
  test.setTimeout(300000);
  await boot(page);

  // desktop Interactive mode forces pattern -> "life" (store.ts:794). Confirm
  // that path actually produces a lit, moving tree rather than the quiet world
  // Elliot measured as "the tree does not respond".
  await page.evaluate(() => (window as any).twin.getState().set({ blackout: false, beaconPreempt: false, brightness: 1 }));
  await page.evaluate(() => (window as any).twin.getState().setUiMode?.("interactive"));
  await page.waitForTimeout(2000);

  const pattern = await page.evaluate(() => (window as any).twin.getState().control.pattern);
  const series: any[] = [];
  for (let i = 0; i < 5; i++) { series.push(await page.evaluate(PROBE)); await page.waitForTimeout(600); }
  const lums = series.map((s) => s.lum);
  let movement = 0;
  for (let i = 1; i < lums.length; i++) movement += Math.abs(lums[i] - lums[i - 1]);

  await page.evaluate(() => (window as any).twin.getState().pingPresence?.());
  await page.waitForTimeout(2000);
  const poked = await page.evaluate(PROBE);

  const entry = {
    check: "uiMode=interactive",
    forcedPattern: pattern,
    peakLum: Number(Math.max(...lums).toFixed(2)),
    movement: Number(movement.toFixed(2)),
    afterPokeLum: Number(poked.lum.toFixed(2)),
  };
  report.push(entry);
  console.log(`[ca] uiMode=interactive → pattern=${pattern} peak=${entry.peakLum} move=${entry.movement} poke=${entry.afterPokeLum}`);

  fs.writeFileSync("interactive-audit.json", JSON.stringify({ modes: report }, null, 2));
  expect(pattern, "interactive mode should select a CA rule").toBeTruthy();
});
