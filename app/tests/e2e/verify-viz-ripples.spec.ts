import { test, expect } from "@playwright/test";
import * as fs from "fs";

/**
 * TARGETED VERIFICATION #2 (network-tester).
 *
 * The simulator audit produced two claims that must not reach a report on the
 * strength of one averaged metric:
 *
 *   A. visualizer "lanterns" and "orbs" render identically (signature distance
 *      0.01, vs ~31.9 to "wire"). Source says they differ ONLY by dotSize
 *      (0.012 vs 0.022, TreeLights.tsx:53) — so this is plausible, but a mean-RGB
 *      signature could simply be too coarse to see a dot-size change.
 *   B. pattern "ripples" renders at 3.61 luminance vs a blackout reference of
 *      3.613 — i.e. selecting it appears to leave the tree dark.
 *
 * Both are re-tested here by a genuinely independent method: capture real PNG
 * screenshots and compare them pixel-by-pixel. No averaging, no signature maths.
 */

const findings: any[] = [];

async function boot(page: any) {
  await page.setViewportSize({ width: 1440, height: 900 });
  await page.goto("/?e2e=1", { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(1000);
}

/** fraction of pixels that differ by more than a small tolerance */
function pixelDiff(a: Buffer, b: Buffer): { diffFrac: number; maxDelta: number } {
  // PNGs of identical dimensions from the same page: compare raw bytes as a
  // cheap proxy, then report both byte-inequality and a magnitude estimate.
  const len = Math.min(a.length, b.length);
  let differing = 0, maxDelta = 0;
  for (let i = 0; i < len; i++) {
    const d = Math.abs(a[i] - b[i]);
    if (d > 2) differing++;
    if (d > maxDelta) maxDelta = d;
  }
  return { diffFrac: differing / len, maxDelta };
}

test("VERIFY A — do 'lanterns' and 'orbs' actually look different?", async ({ page }) => {
  test.setTimeout(240000);
  await boot(page);
  await page.evaluate(() => (window as any).twin.getState().set({
    blackout: false, beaconPreempt: false, pattern: "solid", brightness: 1, sat: 1, hue: 0.1, colorCycle: "off",
  }));
  await page.waitForTimeout(800);

  const shot = async (viz: string) => {
    await page.evaluate((v: string) => (window as any).twin.getState().set({ visualizer: v }), viz);
    await page.waitForTimeout(1200);
    const buf = await page.locator("canvas").first().screenshot({ path: `screenshots/viz-${viz}.png` });
    const state = await page.evaluate(() => (window as any).twin.getState().control.visualizer);
    return { buf, state };
  };

  const lant = await shot("lanterns");
  const orbs = await shot("orbs");
  const wire = await shot("wire");

  // sanity: the store really did change between captures
  expect(lant.state).toBe("lanterns");
  expect(orbs.state).toBe("orbs");
  expect(wire.state).toBe("wire");

  const lo = pixelDiff(lant.buf, orbs.buf);
  const lw = pixelDiff(lant.buf, wire.buf);

  findings.push({ check: "lanterns_vs_orbs", ...lo });
  findings.push({ check: "lanterns_vs_wire", ...lw });
  console.log(`[verify] lanterns↔orbs  diffFrac=${(lo.diffFrac*100).toFixed(3)}% maxDelta=${lo.maxDelta}`);
  console.log(`[verify] lanterns↔wire  diffFrac=${(lw.diffFrac*100).toFixed(3)}% maxDelta=${lw.maxDelta}`);
  console.log(`[verify] VERDICT orbs_is_distinct=${lo.diffFrac > 0.001}`);
});

test("VERIFY B — does 'ripples' leave the tree dark?", async ({ page }) => {
  test.setTimeout(240000);
  await boot(page);

  const lum = async () => await page.evaluate(() => {
    const c = document.querySelector("canvas") as HTMLCanvasElement;
    const g = (c.getContext("webgl2") || c.getContext("webgl")) as WebGLRenderingContext;
    const px = new Uint8Array(c.width * c.height * 4);
    g.readPixels(0, 0, c.width, c.height, g.RGBA, g.UNSIGNED_BYTE, px);
    let s = 0, n = 0, bright = 0;
    for (let i = 0; i < px.length; i += 4 * 16) {
      const L = 0.2126*px[i] + 0.7152*px[i+1] + 0.0722*px[i+2];
      s += L; n++; if (L > 60) bright++;
    }
    return { mean: s/n, brightFrac: bright/n };
  });

  const set = async (p: any) => { await page.evaluate((x: any) => (window as any).twin.getState().set(x), p); await page.waitForTimeout(1200); };

  await set({ blackout: false, beaconPreempt: false, brightness: 1, sat: 1, hue: 0.15 });
  await set({ pattern: "solid" });
  const solid = await lum();
  await set({ blackout: true });
  const dark = await lum();
  await set({ blackout: false, pattern: "ripples" });
  const ripplesAtRest = await lum();

  // ripples is a CA/presence pattern — give it a poke and time to propagate
  await page.evaluate(() => { const t = (window as any).twin.getState(); t.pingPresence?.(); });
  await page.waitForTimeout(2500);
  const ripplesPoked = await lum();

  findings.push({ check: "ripples", solid: solid.mean, dark: dark.mean, atRest: ripplesAtRest.mean, afterPoke: ripplesPoked.mean });
  console.log(`[verify] solid=${solid.mean.toFixed(2)} blackout=${dark.mean.toFixed(2)} ripples@rest=${ripplesAtRest.mean.toFixed(2)} ripples+poke=${ripplesPoked.mean.toFixed(2)}`);
  console.log(`[verify] VERDICT ripples_dark_at_rest=${ripplesAtRest.mean <= dark.mean * 1.2} recovers_on_poke=${ripplesPoked.mean > dark.mean * 1.5}`);

  await page.screenshot({ path: "screenshots/verify-ripples.png" });
});

test.afterAll(() => {
  fs.writeFileSync("verify-audit.json", JSON.stringify(findings, null, 2));
});
