import { test, expect, chromium, type Page } from "@playwright/test";
import * as fs from "fs";

/**
 * SOUND REACTIVITY AUDIT (network-tester, Elliot 2026-08-15: "Test the Sound
 * reactivity as well I don't see the full controller coming up").
 *
 * Two separate questions, and they need separate answers:
 *
 *   1. PARITY — is the full DJ controller reachable on mobile at all?
 *   2. REACTIVITY — with real audio playing, do the lights actually follow it?
 *
 * (2) cannot be tested on the default headless browser: there is no audio device,
 * so every previous run recorded `NotFoundError` and the audio controls read as
 * inert. This spec launches its own Chromium with a FAKE MEDIA DEVICE and
 * autoplay allowed, which is the only way to get a real signal through the
 * WebAudio graph and out to the render.
 */

const results: any[] = [];

const SIG = `(() => {
  const c = document.querySelector("canvas");
  const g = c.getContext("webgl2") || c.getContext("webgl");
  const px = new Uint8Array(c.width * c.height * 4);
  g.readPixels(0, 0, c.width, c.height, g.RGBA, g.UNSIGNED_BYTE, px);
  let lum = 0, n = 0;
  for (let i = 0; i < px.length; i += 4 * 16) { lum += 0.2126*px[i] + 0.7152*px[i+1] + 0.0722*px[i+2]; n++; }
  return lum / n;
})()`;

async function boot(page: Page, w: number, h: number, url = "/?e2e=1") {
  await page.setViewportSize({ width: w, height: h });
  await page.goto(`http://localhost:4173${url}`, { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(800);
}

/** every control label on screen — the parity comparison */
const LABELS = `Array.from(document.querySelectorAll("button,input")).map(function(e){
  return (e.textContent || "").trim() || (e.getAttribute("aria-label") || "") || ("<" + e.tagName.toLowerCase() + " " + ((e).type || "") + ">");
}).filter(Boolean)`;

test("sound — DJ controller parity between desktop and mobile", async ({ page }) => {
  test.setTimeout(300000);

  // DESKTOP, Sound mode
  await boot(page, 1440, 900);
  await page.evaluate(() => { const t = (window as any).twin.getState(); t.setDock(true); t.setCinematic(false); t.setUiMode("sound"); });
  await page.waitForTimeout(900);
  const desktop: string[] = await page.evaluate(LABELS);

  // MOBILE, Sound tab
  await boot(page, 375, 812);
  await page.getByRole("button", { name: /Sound/i }).last().click({ timeout: 6000 }).catch(() => {});
  await page.waitForTimeout(900);
  const mobile: string[] = await page.evaluate(LABELS);

  const dset = new Set(desktop.map((s) => s.toLowerCase()));
  const mset = new Set(mobile.map((s) => s.toLowerCase()));
  const missingOnMobile = [...dset].filter((l) => !mset.has(l));

  // is the DJ deck itself mounted on mobile?
  const djMarkers = ["🎤 mic", "▶ track", "⚡ strobe", "🤖 auto", "🎵 rx-spd", "xfade", "eq"];
  const djOnDesktop = djMarkers.filter((m) => [...dset].some((l) => l.includes(m)));
  const djOnMobile = djMarkers.filter((m) => [...mset].some((l) => l.includes(m)));

  results.push({
    check: "dj-parity",
    desktopControls: desktop.length, mobileControls: mobile.length,
    djMarkersDesktop: djOnDesktop, djMarkersMobile: djOnMobile,
    missingOnMobileCount: missingOnMobile.length,
    missingOnMobile: missingOnMobile.slice(0, 60),
  });

  console.log(`[sound] desktop Sound surface: ${desktop.length} controls`);
  console.log(`[sound] mobile  Sound surface: ${mobile.length} controls`);
  console.log(`[sound] DJ markers — desktop: ${djOnDesktop.join(", ") || "none"} | mobile: ${djOnMobile.join(", ") || "none"}`);
  console.log(`[sound] present on desktop, MISSING on mobile: ${missingOnMobile.length}`);
});

test("sound — with REAL audio playing, does the render follow it?", async () => {
  test.setTimeout(300000);

  // own browser: fake mic + autoplay, so WebAudio actually produces a signal
  const browser = await chromium.launch({
    args: [
      "--use-fake-device-for-media-stream",
      "--use-fake-ui-for-media-stream",
      "--autoplay-policy=no-user-gesture-required",
      "--allow-file-access-from-files",
    ],
  });
  const ctx = await browser.newContext({ permissions: ["microphone"] });
  const page = await ctx.newPage();
  const errs: string[] = [];
  page.on("pageerror", (e) => errs.push(String(e)));

  try {
    await boot(page, 1440, 900);
    await page.evaluate(() => {
      const t = (window as any).twin.getState();
      t.set({ blackout: false, beaconPreempt: false, brightness: 1, pattern: "solid", hue: 0.15, sat: 1 });
    });
    await page.waitForTimeout(600);

    // start the bundled test track (124 BPM) through the real audio graph
    const started = await page.evaluate(async () => {
      const w = window as any;
      try {
        const m = await import("/src/audio.ts").catch(() => null);
        if (m?.startTrack) { await m.startTrack("/audio/test-beat-124bpm.wav"); return "module"; }
      } catch { /* built bundle: module path not importable */ }
      // fall back to clicking the UI control
      return "ui";
    });

    if (started === "ui") {
      await page.getByRole("button", { name: /test track|▶ TRACK/i }).first().click({ timeout: 6000 }).catch(() => {});
    }
    await page.waitForTimeout(2500);

    // read the audio analyser the app itself uses
    const feat = await page.evaluate(() => {
      const w = window as any;
      const t = w.twin?.getState?.();
      return { bpm: t?.audioBpm ?? null, audioOn: t?.control?.audioSpeed ?? null };
    });

    // turn ON audio-reactivity and watch the render move
    await page.evaluate(() => (window as any).twin.getState().set({ syncToBeat: true, audioSpeed: true }));
    await page.waitForTimeout(1200);

    const series: number[] = [];
    for (let i = 0; i < 8; i++) { series.push(await page.evaluate(SIG)); await page.waitForTimeout(350); }
    let movement = 0;
    for (let i = 1; i < series.length; i++) movement += Math.abs(series[i] - series[i - 1]);

    results.push({
      check: "audio-reactivity",
      startedVia: started, features: feat,
      lums: series.map((n) => Number(n.toFixed(2))),
      movement: Number(movement.toFixed(2)),
      pageErrors: errs,
    });
    console.log(`[sound] audio start=${started} feat=${JSON.stringify(feat)}`);
    console.log(`[sound] render series ${series.map((n) => n.toFixed(1)).join(" ")} → movement ${movement.toFixed(2)}`);
    console.log(`[sound] pageErrors: ${errs.length}`);
  } finally {
    await browser.close();
  }
});

test.afterAll(() => {
  fs.writeFileSync("sound-audit.json", JSON.stringify(results, null, 2));
});
