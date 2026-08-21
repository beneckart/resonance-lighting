import { test, expect } from "@playwright/test";

/**
 * TARGETED VERIFICATION (network-tester).
 *
 * The control audit reported that on touch/interactive @375, pattern pads from
 * index ~14 onward could not be clicked, several explicitly hit-tested as covered
 * by the fixed bottom tab bar ("🌳Tree" / "🎬Shows" / "🎵Sound").
 *
 * That is a serious claim, and this harness has already produced one round of
 * false positives (accessible-name mismatch). So before it goes in a report it
 * gets verified a second, independent way: scroll the sheet fully, then ask the
 * DOM directly whether the LAST pattern pad is reachable — no click-timeout
 * inference, no data-qa-idx, no reliance on Playwright actionability at all.
 */
test("VERIFY — can a user reach the last pattern pad on mobile Interactive?", async ({ page }) => {
  test.setTimeout(120000);
  await page.setViewportSize({ width: 375, height: 812 });
  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(500);

  await page.getByRole("button", { name: /Interactive/i }).last().click();
  await page.waitForTimeout(800);

  const probe = async (note: string) => await page.evaluate((n) => {
    const btns = Array.from(document.querySelectorAll("button"));
    const last = btns.find((b) => (b.textContent || "").trim() === "solarray");
    const nav = document.querySelector("nav");
    const navRect = nav?.getBoundingClientRect();
    if (!last) return { note: n, found: false } as any;
    const r = last.getBoundingClientRect();
    const cx = r.left + r.width / 2, cy = r.top + r.height / 2;
    const top = document.elementFromPoint(cx, cy);
    // find the nearest scrollable ancestor and report whether it is at its end
    let el: HTMLElement | null = last.parentElement;
    let scroller: any = null;
    while (el) {
      const cs = getComputedStyle(el);
      if (/(auto|scroll)/.test(cs.overflowY) && el.scrollHeight > el.clientHeight + 2) {
        scroller = { tag: el.tagName, scrollTop: Math.round(el.scrollTop), scrollHeight: el.scrollHeight, clientHeight: el.clientHeight,
                     atEnd: el.scrollTop + el.clientHeight >= el.scrollHeight - 2 };
        break;
      }
      el = el.parentElement;
    }
    return {
      note: n, found: true,
      rect: { top: Math.round(r.top), bottom: Math.round(r.bottom), h: Math.round(r.height) },
      viewportH: innerHeight,
      navTop: navRect ? Math.round(navRect.top) : null,
      belowNav: navRect ? r.top >= navRect.top : null,
      topElementAtCentre: top ? (top.tagName + ":" + (top.textContent || "").trim().slice(0, 20)) : null,
      reachable: !!top && (top === last || last.contains(top)),
      scroller,
    };
  }, note);

  const before = await probe("before scroll");
  console.log("[verify] " + JSON.stringify(before, null, 1));

  // scroll the sheet as far as it goes, the way a user would
  await page.evaluate(() => {
    const btns = Array.from(document.querySelectorAll("button"));
    const last = btns.find((b) => (b.textContent || "").trim() === "solarray");
    last?.scrollIntoView({ block: "center" });
  });
  await page.waitForTimeout(600);
  const after = await probe("after scrollIntoView");
  console.log("[verify] " + JSON.stringify(after, null, 1));

  await page.screenshot({ path: "screenshots/verify-pattern-reach.png", fullPage: false });

  // The verdict we actually care about: after an honest attempt to scroll to it,
  // is the last pad hit-testable at its own centre?
  console.log(`[verify] VERDICT reachable_after_scroll=${after.reachable} coveredBy=${after.topElementAtCentre}`);
});
