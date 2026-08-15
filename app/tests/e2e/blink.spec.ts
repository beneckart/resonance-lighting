import { test, expect, type Page } from "@playwright/test";
import * as fs from "fs";

/**
 * BLINK AUDIT (network-tester, Elliot 2026-08-15: "Why does the Blink still not
 * work? I am trying this on the command software").
 *
 * Blink is the ONE Command control that does not behave like the others, and
 * that is deliberate (TouchConsole.tsx:688): every other Command button routes
 * through runScript(), which "hits the SIM unconditionally", while BLINK calls
 * fleetIdentify() and only ever emits a wire packet.
 *
 * The consequence, which is what this measures: with no live daemon socket,
 * BLINK is a total no-op — nothing renders, nothing errors, and (on the Command
 * pad) nothing is even said to the operator. It fails silently at three layers:
 *
 *   fleetlink.ts   bridge?.send(...)                       ← optional chain, no bridge = no-op
 *   cambium.ts:190 if (!ws || readyState !== OPEN) return;  ← socket closed = silent drop
 *   TouchConsole   Command pad has NO feedback at all;
 *                  the Locate sheet says "blink sent (3 s)" UNCONDITIONALLY
 *
 * So the operator cannot distinguish "sent and the lantern ignored it" from
 * "never left the browser". That is the actual defect.
 */

const results: any[] = [];

const LUMA = `(() => {
  const c = document.querySelector("canvas");
  const g = c.getContext("webgl2") || c.getContext("webgl");
  const px = new Uint8Array(c.width * c.height * 4);
  g.readPixels(0, 0, c.width, c.height, g.RGBA, g.UNSIGNED_BYTE, px);
  let s = 0, n = 0;
  for (let i = 0; i < px.length; i += 4 * 16) { s += 0.2126*px[i] + 0.7152*px[i+1] + 0.0722*px[i+2]; n++; }
  return s / n;
})()`;

async function bootPhone(page: Page) {
  await page.setViewportSize({ width: 375, height: 812 });
  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(800);
  await page.getByRole("button", { name: /Command/i }).last().click({ timeout: 6000 }).catch(() => {});
  await page.waitForTimeout(700);
}

test("BLINK — does it do anything at all without a live daemon?", async ({ page }) => {
  test.setTimeout(240000);
  const errors: string[] = [];
  page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
  page.on("pageerror", (e) => errors.push("PAGEERROR " + String(e)));

  await bootPhone(page);

  // is a bridge socket actually open? this is the precondition BLINK needs
  const connected = await page.evaluate(() => {
    const w = window as any;
    try { return w.twin?.getState?.().fleetConnected ?? null; } catch { return null; }
  });

  // baseline the render, then press BLINK, then watch
  await page.evaluate(() => (window as any).twin.getState().set({ blackout: false, pattern: "solid", brightness: 1, hue: 0.15, sat: 1 }));
  await page.waitForTimeout(900);
  const before = await page.evaluate(LUMA);
  const domBefore = await page.evaluate(() => document.body.innerText);

  const blink = page.getByRole("button", { name: /BLINK/i }).first();
  const found = await blink.count();
  expect(found, "BLINK pad is present on the Command page").toBeGreaterThan(0);
  await blink.click({ timeout: 6000 });

  // CAPTURE FEEDBACK IMMEDIATELY. The acknowledgement is a 3 s transient, and
  // the render sampling below takes ~3.6 s — reading the DOM after the loop
  // measured an empty note and reported "no feedback" when the note had simply
  // already expired. Sample the thing on its own timescale, not the loop's.
  await page.waitForTimeout(250);
  const domAfter = await page.evaluate(() => document.body.innerText);

  // identify is a 3 s bounded packet — sample across its whole window
  const series: number[] = [];
  for (let i = 0; i < 8; i++) { series.push(await page.evaluate(LUMA)); await page.waitForTimeout(450); }

  let movement = 0;
  for (let i = 1; i < series.length; i++) movement += Math.abs(series[i] - series[i - 1]);
  const domChanged = domAfter !== domBefore;

  // compare against a control that DOES have a sim path
  await page.evaluate(() => (window as any).twin.getState().set({ blackout: false, pattern: "solid", brightness: 1 }));
  await page.waitForTimeout(600);
  const ctlBefore = await page.evaluate(LUMA);
  await page.getByRole("button", { name: /⭘ OFF|^OFF$/i }).first().click({ timeout: 6000 }).catch(() => {});
  await page.waitForTimeout(1200);
  const ctlAfter = await page.evaluate(LUMA);

  const entry = {
    check: "blink-command-pad",
    bridgeConnected: connected,
    lumBefore: Number(before.toFixed(2)),
    lumSeries: series.map((n) => Number(n.toFixed(2))),
    renderMovement: Number(movement.toFixed(2)),
    domChanged,
    // match the NOTE, not the button label — /blink/i alone matches the "✨ BLINK"
    // pad itself and reports feedback that isn't there
    feedbackText: (domAfter.split("\n").find((l) => /nothing to blink|blink sent/i.test(l)) || "<none>").trim(),
    userFeedback: domChanged ? "the operator was told something" : "NONE — nothing told the operator anything",
    errors,
    controlOffButton: { before: Number(ctlBefore.toFixed(2)), after: Number(ctlAfter.toFixed(2)),
                        delta: Number(Math.abs(ctlAfter - ctlBefore).toFixed(2)) },
  };
  results.push(entry);

  console.log(`[blink] bridge connected: ${connected}`);
  console.log(`[blink] render movement after BLINK: ${entry.renderMovement}  (series ${entry.lumSeries.join(" ")})`);
  console.log(`[blink] feedback: ${entry.userFeedback} → "${entry.feedbackText}"`);
  console.log(`[blink] errors: ${errors.length}`);
  console.log(`[blink] CONTROL — the OFF pad (which has a sim path): ${entry.controlOffButton.before} → ${entry.controlOffButton.after} (delta ${entry.controlOffButton.delta})`);

  fs.writeFileSync("blink-audit.json", JSON.stringify(results, null, 2));
});
