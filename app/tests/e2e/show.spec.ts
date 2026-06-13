import { test, expect } from "@playwright/test";

/** Show-building flow (F1/F2/D5): command script → save 2 cues → recall → play/stop
 *  the timeline, asserting zero console/page errors throughout. */
test("show flow — commands, cues, timeline (no errors)", async ({ page }) => {
  test.setTimeout(60000);
  const errors: string[] = [];
  page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
  page.on("pageerror", (e) => errors.push(String(e)));
  await page.addInitScript(() => localStorage.clear());

  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" }); // light scene (skip 22MB bark glb) for stable headless GL
  await expect(page.locator("canvas")).toBeVisible({ timeout: 15000 });

  // command script
  await page.locator("textarea").fill("all pattern spectrum\nzone high color #00aaff\nevery 4 color red\nclear");
  await page.getByText("▶ run script", { exact: false }).click();
  await page.waitForTimeout(150);

  // save two cues
  await page.getByRole("button", { name: "spectrum", exact: true }).first().click();
  await page.getByPlaceholder("cue name").fill("A");
  await page.getByText("💾 save", { exact: false }).click();
  await page.getByRole("button", { name: "solid", exact: true }).first().click();
  await page.getByPlaceholder("cue name").fill("B");
  await page.getByText("💾 save", { exact: false }).click();

  // recall A
  await page.getByText("▶ A", { exact: false }).first().click();
  await page.waitForTimeout(150);

  // play then stop the timeline
  await page.getByText("▶ play timeline", { exact: false }).click();
  await page.waitForTimeout(800);
  await page.getByText("⏹ stop timeline", { exact: false }).click();

  await page.screenshot({ path: "screenshots/show-flow.png" });
  expect(errors, errors.join("\n")).toEqual([]);
});
