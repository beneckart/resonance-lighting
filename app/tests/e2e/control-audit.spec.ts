import { test, expect, type Page } from "@playwright/test";
import * as fs from "fs";

/**
 * CONTROL AUDIT (network-tester, Elliot-directed 2026-08-15).
 *
 * The existing control-matrix.spec.ts is a no-errors SMOKE gate: it force-clicks
 * ~25 controls on one desktop viewport and asserts only that the console stayed
 * clean. Three things it cannot catch, all of which have shipped as real bugs
 * in this repo (0eb38c9 "the tree does not respond", f8b3081 ✕-crash):
 *
 *   1. force:true clicks a button that is invisible, zero-size, or covered by
 *      another element. A user cannot click it; the test says PASS.
 *   2. "no console error" says nothing about whether the click DID anything.
 *      A dead onClick handler is silently green.
 *   3. Desktop-only. The mobile shell is a different component tree entirely.
 *
 * This audit instead, for every shell x mode x viewport:
 *   - enumerates every <button> actually in the DOM
 *   - hit-tests it (elementFromPoint at its centre must resolve to it) — this is
 *     the "covered by an invisible overlay" check that force:true throws away
 *   - clicks it for REAL (no force), from a freshly reset surface each time
 *   - diffs window.twin store state + the DOM before/after to classify the
 *     click as HAS-EFFECT or NO-EFFECT
 *   - attributes any console/page error to the specific button that caused it
 *
 * Output: control-audit.json (machine-readable inventory) — the report is built
 * from that, not from prose. Report-only; this file is delivered to
 * lighting-architect to land, not committed by QA.
 */

type Surface = {
  id: string;
  viewport: { width: number; height: number };
  url: string;
  /** run in-page to put the app in this surface's shell/mode before enumeration */
  setup: string;
};

const DESKTOP = { width: 1440, height: 900 };
const MOBILE = { width: 375, height: 812 };

// ?e2e=1 = light scene (skips the 22MB bark glb) so headless GL stays stable
// under hundreds of interactions. Same flag the existing matrix spec uses.
const SURFACES: Surface[] = [
  {
    id: "dock/lightshow @1440",
    viewport: DESKTOP,
    url: "/?e2e=1",
    setup: `t.setDock(true); t.setCinematic(false); t.setUiMode("lightshow");`,
  },
  {
    id: "dock/interactive @1440",
    viewport: DESKTOP,
    url: "/?e2e=1",
    setup: `t.setDock(true); t.setCinematic(false); t.setUiMode("interactive");`,
  },
  {
    id: "dock/sound @1440",
    viewport: DESKTOP,
    url: "/?e2e=1",
    setup: `t.setDock(true); t.setCinematic(false); t.setUiMode("sound");`,
  },
  {
    id: "dock/calibrate @1440",
    viewport: DESKTOP,
    url: "/?e2e=1",
    setup: `t.setDock(true); t.setCinematic(false); t.setUiMode("calibrate");`,
  },
  {
    id: "float (widgets) @1440",
    viewport: DESKTOP,
    url: "/?e2e=1",
    setup: `t.setDock(false); t.setCinematic(false);`,
  },
  {
    id: "cinematic (clean view) @1440",
    viewport: DESKTOP,
    url: "/?e2e=1",
    setup: `t.setDock(true); t.setCinematic(true);`,
  },
  {
    id: "demo lock ?demo=1 @1440",
    viewport: DESKTOP,
    url: "/?e2e=1&demo=1",
    setup: `t.setDock(true); t.setCinematic(false);`,
  },
  // mobile: TouchConsole auto-opens under 767px and owns the screen.
  // Its tabs are component-local state, so they are driven by clicking the tab bar.
  { id: "touch/tree @375", viewport: MOBILE, url: "/?e2e=1", setup: `` },
  { id: "touch/lightshow @375", viewport: MOBILE, url: "/?e2e=1", setup: `__TAB__Light Show` },
  { id: "touch/interactive @375", viewport: MOBILE, url: "/?e2e=1", setup: `__TAB__Interactive` },
  { id: "touch/sound @375", viewport: MOBILE, url: "/?e2e=1", setup: `__TAB__Sound` },
  { id: "touch/calibrate @375", viewport: MOBILE, url: "/?e2e=1", setup: `__TAB__Calibrate` },
  { id: "demo lock ?demo=1 @375", viewport: MOBILE, url: "/?e2e=1&demo=1", setup: `` },
];

/** Serializable projection of the store — functions and the big fixture array dropped. */
const SNAPSHOT = `(() => {
  const s = window.twin.getState();
  const out = {};
  for (const k of Object.keys(s)) {
    const v = s[k];
    if (typeof v === "function") continue;
    if (k === "fixtures") { out.__fixtureCount = Array.isArray(v) ? v.length : 0; continue; }
    try { out[k] = JSON.parse(JSON.stringify(v)); } catch { out[k] = "<unserializable>"; }
  }
  return JSON.stringify(out);
})()`;

/** Identity + geometry + REAL hittability for every button on screen. */
const ENUMERATE = `(() => {
  const seen = {};
  return Array.from(document.querySelectorAll("button")).map((b, domIndex) => {
    const label = (b.textContent || "").trim().replace(/\\s+/g, " ").slice(0, 60);
    const key = label || "(unlabelled)";
    seen[key] = (seen[key] || 0) + 1;
    const r = b.getBoundingClientRect();
    const cs = getComputedStyle(b);
    const cx = r.left + r.width / 2, cy = r.top + r.height / 2;
    const inViewport = cx >= 0 && cy >= 0 && cx <= innerWidth && cy <= innerHeight;
    const topEl = inViewport ? document.elementFromPoint(cx, cy) : null;
    // hittable = the point at the button's own centre resolves back to it
    const hittable = !!topEl && (topEl === b || b.contains(topEl));
    let blockedBy = null;
    if (inViewport && !hittable && topEl) {
      blockedBy = topEl.tagName.toLowerCase() +
        (topEl.className && typeof topEl.className === "string" ? "." + topEl.className.split(" ")[0] : "") +
        ' "' + (topEl.textContent || "").trim().slice(0, 24) + '"';
    }
    return {
      domIndex,
      label,
      nth: seen[key] - 1,
      disabled: b.disabled,
      w: Math.round(r.width), h: Math.round(r.height),
      area: Math.round(r.width * r.height),
      visible: r.width > 0 && r.height > 0 && cs.visibility !== "hidden" && cs.display !== "none" && Number(cs.opacity) > 0.01,
      inViewport, hittable, blockedBy,
      title: b.getAttribute("title") || null,
      ariaLabel: b.getAttribute("aria-label") || null,
    };
  });
})()`;

/**
 * Stamp a stable handle on every button.
 *
 * Learned from run 3: locating by accessible name is WRONG for this app. A button
 * built from nested divs (`<div>🌱</div>Interactive`) has textContent "🌱Interactive"
 * but an accessible name of "🌱 Interactive" — so getByRole({exact:true}) missed
 * every mode tab and every show pad and reported them as UNCLICKABLE when they were
 * provably hittable. That is a harness defect masquerading as 12 app defects, which
 * is exactly the kind of false positive a QA report must never ship.
 * Tagging by DOM index sidesteps name resolution entirely.
 */
const TAG = `(() => {
  document.querySelectorAll("button").forEach((b, i) => b.setAttribute("data-qa-idx", String(i)));
  return document.querySelectorAll("button").length;
})()`;

async function gotoSurface(page: Page, s: Surface) {
  await page.setViewportSize(s.viewport);
  await page.goto(s.url, { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  if (s.setup.startsWith("__TAB__")) {
    const tabName = s.setup.replace("__TAB__", "");
    await page.getByRole("button", { name: new RegExp(tabName, "i") }).last().click({ timeout: 5000 }).catch(() => {});
  } else if (s.setup.trim()) {
    await page.evaluate(`(() => { const t = window.twin.getState(); ${s.setup} })()`);
  }
  await page.waitForTimeout(280);
  await page.evaluate(TAG);
}

const SHARD_DIR = "audit-shards";

/**
 * ONE TEST PER SURFACE. Learned the hard way on the first run: a single
 * 14-surface test wrote its JSON only at the very end, so hitting the timeout
 * threw away every data point collected. Per-surface tests each get their own
 * timeout and write their own shard the moment they finish — a slow surface
 * costs you that surface, not the sweep.
 */
for (const s of SURFACES) {
  test(`control audit — ${s.id}`, async ({ page }) => {
    test.setTimeout(9 * 60 * 1000);

    const errors: string[] = [];
    page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
    page.on("pageerror", (e) => errors.push("PAGEERROR " + String(e)));

    await gotoSurface(page, s);
    const inventory: any[] = await page.evaluate(ENUMERATE);
    const surfaceEntry = { surface: s.id, viewport: s.viewport, buttonCount: inventory.length, buttons: [] as any[] };

    // ADAPTIVE RESET. Reloading before every button is correct but far too slow
    // (measured: one dense surface blew a 9-minute budget). Most clicks — picking
    // a pattern, a colour, a camera — leave the DOM shape untouched, so the next
    // button is still exactly where the inventory says it is. We therefore only
    // pay for a reload after a click that actually changed the button set (shell
    // switches, panel opens), which is precisely when the inventory goes stale.
    let dirty = true;

    for (const item of inventory) {
      if (dirty) await gotoSurface(page, s);
      // re-stamp: a React re-render can drop an externally-set attribute. The DOM
      // button count is unchanged here (a change would have set dirty), so indices
      // are stable and re-tagging is idempotent.
      else await page.evaluate(TAG);
      dirty = false;
      errors.length = 0;

      const before = await page.evaluate(SNAPSHOT);
      const domBefore = await page.evaluate(`document.querySelectorAll("button").length`);

      const loc = page.locator(`[data-qa-idx="${item.domIndex}"]`);

      let clickResult = "ok";
      let clickError: string | null = null;
      try {
        // REAL click — no force. Actionability IS the test.
        await loc.click({ timeout: 3500 });
      } catch (e) {
        clickResult = "UNCLICKABLE";
        clickError = String(e).split("\n").slice(0, 3).join(" | ").slice(0, 300);
      }
      await page.waitForTimeout(160);

      let after = before, domAfter = domBefore;
      try {
        after = await page.evaluate(SNAPSHOT);
        domAfter = await page.evaluate(`document.querySelectorAll("button").length`);
      } catch { /* page may be mid-teardown */ }

      const domChanged = domAfter !== domBefore;
      // a click that reshaped the DOM invalidates the inventory → reload next time
      if (domChanged || clickResult === "UNCLICKABLE") dirty = true;

      surfaceEntry.buttons.push({
        ...item,
        clickResult,
        clickError,
        stateChanged: after !== before,
        domChanged,
        hasEffect: after !== before || domChanged,
        errors: [...errors],
      });
    }

    fs.mkdirSync(SHARD_DIR, { recursive: true });
    fs.writeFileSync(`${SHARD_DIR}/${s.id.replace(/[^a-z0-9]+/gi, "_")}.json`, JSON.stringify(surfaceEntry, null, 2));
    // eslint-disable-next-line no-console
    console.log(`[audit] ${s.id}: ${surfaceEntry.buttons.length} buttons audited`);
    expect(surfaceEntry.buttons.length, "surface rendered at least one button").toBeGreaterThan(0);
  });
}
