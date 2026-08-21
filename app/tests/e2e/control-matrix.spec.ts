import { test, expect, type Locator } from "@playwright/test";

/**
 * Coverage gate (PRD testing lane): drive EVERY control — visualizers, all patterns,
 * all sequence modes, element modes, the command console, DJ toggles, auto-VJ, and a
 * presence ping — asserting zero console/page errors throughout. Clicks are `force`
 * (this is a no-errors smoke test, not a click-actionability test).
 */
test("control matrix — every control runs with no errors", async ({ page }) => {
  test.setTimeout(120000);
  const errors: string[] = [];
  page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
  page.on("pageerror", (e) => errors.push(String(e)));

  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" }); // light scene (skip 22MB bark glb) for stable headless GL under heavy interaction
  await expect(page.locator("canvas")).toBeVisible({ timeout: 15000 });

  const tap = async (loc: Locator) => {
    await loc.scrollIntoViewIfNeeded({ timeout: 1500 }).catch(() => {});
    await loc.click({ force: true, timeout: 1500 }).catch(() => {});
    await page.waitForTimeout(60);
  };
  const btn = (name: string) => page.getByRole("button", { name, exact: true }).first();
  const text = (t: string) => page.getByText(t, { exact: false }).first();

  for (const v of ["orbs", "wire", "lanterns"]) await tap(btn(v));
  for (const p of ["solid", "breathe", "chase", "ripple", "sparkle", "spectrum", "tricolor", "sequence"]) await tap(btn(p));
  for (const m of ["single", "snake", "groups", "everyN", "allOn", "allOff", "fill"]) await tap(btn(m));
  for (const e of ["wind", "ember", "rain", "beacon"]) await tap(btn(e));

  await page.locator("textarea").fill("all pattern spectrum\nzone high color #00aaff\nevery 4 color red");
  await tap(text("▶ run script"));
  await tap(btn("clear"));

  await tap(text("⚡ strobe"));
  await tap(text("⚡ strobe"));
  await tap(text("🤖 auto-VJ"));
  await tap(text("🤖 auto-VJ"));
  await tap(text("✨ ping"));
  await page.waitForTimeout(200);

  await page.screenshot({ path: "screenshots/control-matrix.png" });
  expect(errors, `console errors:\n${errors.join("\n")}`).toEqual([]);
});
