import { test, expect, type Page } from "@playwright/test";
import * as fs from "fs";

/**
 * MOBILE DEEP AUDIT (network-tester, Elliot-directed 2026-08-15: "we also need
 * you to go through the mobile version").
 *
 * control-audit.spec.ts covers WHETHER each mobile button clicks and does
 * something. This covers the failure modes that clicking alone cannot see —
 * every one of them a bug that has ALREADY shipped in this repo:
 *
 *   0eb38c9  "the tree does not respond"          → luminance must actually move
 *   f8b3081  ✕-to-desktop crashed the console     → hooks below an early return
 *   10d01e9  mode pages unreachable               → sheet must follow the TAB
 *   57041f7  tree must RESIZE to fit above sheet  → canvas non-zero on every tab
 *   2e1928e  PWA icons were EMPTY                 → Android would not install
 *
 * Plus the two standing mobile gates: no horizontal overflow at 375, and touch
 * targets that meet the 44px minimum.
 *
 * Report-only. Delivered to lighting-architect to land; QA does not commit here.
 */

const PHONE = { width: 375, height: 812 };
const TABS = ["Light Show", "Interactive", "Sound", "Calibrate"] as const;

const findings: any[] = [];
const record = (area: string, check: string, pass: boolean, detail: any) => {
  findings.push({ area, check, verdict: pass ? "PASS" : "FAIL", detail });
};

/** mean luminance of the live GL canvas — the "did the tree actually change" probe */
const LUMA = `(() => {
  const c = document.querySelector("canvas");
  if (!c) return -1;
  const g = c.getContext("webgl2") || c.getContext("webgl");
  if (!g) return -1;
  const px = new Uint8Array(c.width * c.height * 4);
  g.readPixels(0, 0, c.width, c.height, g.RGBA, g.UNSIGNED_BYTE, px);
  let sum = 0, n = 0;
  for (let i = 0; i < px.length; i += 4 * 32) { sum += 0.2126*px[i] + 0.7152*px[i+1] + 0.0722*px[i+2]; n++; }
  return n ? sum / n : -1;
})()`;

async function bootPhone(page: Page, query = "?e2e=1") {
  await page.setViewportSize(PHONE);
  await page.goto(`/${query}`, { waitUntil: "domcontentloaded" });
  await expect(page.locator("canvas").first()).toBeVisible({ timeout: 20000 });
  await page.waitForFunction(() => (window as any).twin?.getState()?.fixtures?.length > 0, null, { timeout: 20000 });
  await page.waitForTimeout(600);
}

test("mobile — TouchConsole auto-opens and lands on the Tree tab", async ({ page }) => {
  test.setTimeout(120000);
  await bootPhone(page);
  const touchOpen = await page.evaluate(() => (window as any).twin.getState().touchOpen);
  record("shell", "TouchConsole auto-opens at 375px", touchOpen === true, { touchOpen });
  expect(touchOpen, "phone-like viewport must open the touch console").toBe(true);
});

test("mobile — no horizontal overflow on any tab (375px)", async ({ page }) => {
  test.setTimeout(180000);
  await bootPhone(page);
  const measure = async (label: string) => {
    const o = await page.evaluate(() => ({
      scrollW: document.documentElement.scrollWidth,
      clientW: document.documentElement.clientWidth,
      overflow: document.documentElement.scrollWidth - document.documentElement.clientWidth,
    }));
    record("layout", `no horizontal overflow — ${label}`, o.overflow <= 0, o);
    return o.overflow;
  };
  await measure("tree");
  for (const t of TABS) {
    await page.getByRole("button", { name: new RegExp(t, "i") }).last().click({ timeout: 5000 }).catch(() => {});
    await page.waitForTimeout(500);
    await measure(t);
  }
  const bad = findings.filter((f) => f.area === "layout" && f.verdict === "FAIL");
  expect(bad, `horizontal overflow on: ${bad.map((b) => b.check).join(", ")}`).toEqual([]);
});

test("mobile — the tree canvas stays visible and non-zero on every tab (57041f7)", async ({ page }) => {
  test.setTimeout(180000);
  await bootPhone(page);
  for (const t of ["tree", ...TABS]) {
    if (t !== "tree") {
      await page.getByRole("button", { name: new RegExp(t, "i") }).last().click({ timeout: 5000 }).catch(() => {});
      await page.waitForTimeout(600);
    }
    const box = await page.locator("canvas").first().boundingBox();
    const ok = !!box && box.width > 100 && box.height > 80;
    record("layout", `tree canvas has real size — ${t}`, ok, box);
  }
  const bad = findings.filter((f) => f.check.startsWith("tree canvas") && f.verdict === "FAIL");
  expect(bad, `tree collapsed on: ${JSON.stringify(bad)}`).toEqual([]);
});

test("mobile — sheet content FOLLOWS the tab, tabs are not interchangeable (10d01e9)", async ({ page }) => {
  test.setTimeout(180000);
  await bootPhone(page);
  const sigs: Record<string, string> = {};
  for (const t of TABS) {
    await page.getByRole("button", { name: new RegExp(t, "i") }).last().click({ timeout: 5000 }).catch(() => {});
    await page.waitForTimeout(600);
    // signature = the set of button labels on screen, minus the always-present tab bar
    sigs[t] = await page.evaluate(() =>
      Array.from(document.querySelectorAll("button")).map((b) => (b.textContent || "").trim()).sort().join("|")
    );
  }
  const uniq = new Set(Object.values(sigs));
  record("navigation", "each mode tab renders distinct content", uniq.size === TABS.length, {
    tabs: TABS.length, distinctSignatures: uniq.size,
  });
  expect(uniq.size, "mode tabs must not render identical content").toBe(TABS.length);
});

test("mobile — the tree RESPONDS: a pad tap moves real pixels (0eb38c9)", async ({ page }) => {
  test.setTimeout(180000);
  await bootPhone(page);
  await page.getByRole("button", { name: /Light Show/i }).last().click({ timeout: 5000 }).catch(() => {});
  await page.waitForTimeout(700);

  const before = await page.evaluate(LUMA);
  // drive through the store so the probe is about RENDERING, not about which pad
  // happens to be on screen — a pad that changes state but not pixels is the bug.
  await page.evaluate(() => {
    const t = (window as any).twin.getState();
    t.set({ blackout: false, pattern: "solid", hue: 0.55, sat: 1, brightness: 1 });
  });
  await page.waitForTimeout(900);
  const after = await page.evaluate(LUMA);

  const delta = Math.abs((after as number) - (before as number));
  // 0eb38c9's signature was a delta of ~2 (125 → 123) being called "responding"
  record("responsiveness", "tree luminance moves on a real state change", delta > 5, { before, after, delta });
  expect(delta, `tree did not visibly respond (before=${before} after=${after})`).toBeGreaterThan(5);
});

test("mobile — ✕ drops to desktop without crashing (f8b3081)", async ({ page }) => {
  test.setTimeout(180000);
  const errors: string[] = [];
  page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
  page.on("pageerror", (e) => errors.push("PAGEERROR " + String(e)));

  await bootPhone(page);
  const x = page.getByRole("button", { name: /full console/i }).first();
  const present = await x.count();
  record("navigation", "✕ (full console) is reachable on the Tree tab", present > 0, { count: present });

  if (present > 0) {
    await x.click({ timeout: 5000 });
    await page.waitForTimeout(900);
    const state = await page.evaluate(() => ({
      touchOpen: (window as any).twin.getState().touchOpen,
      buttons: document.querySelectorAll("button").length,
      canvas: !!document.querySelector("canvas"),
    }));
    record("navigation", "✕ → desktop console, no crash", errors.length === 0 && state.canvas && state.buttons > 0, {
      ...state, errors,
    });
    expect(errors, `✕-to-desktop produced errors:\n${errors.join("\n")}`).toEqual([]);
    expect(state.canvas, "canvas survived the shell switch").toBe(true);
  }
});

test("mobile — touch targets meet the 44px minimum", async ({ page }) => {
  test.setTimeout(240000);
  await bootPhone(page);
  const small: any[] = [];
  const scan = async (label: string) => {
    const items = await page.evaluate(() =>
      Array.from(document.querySelectorAll("button"))
        .map((b) => { const r = b.getBoundingClientRect(); return { label: (b.textContent || "").trim().slice(0, 32), w: Math.round(r.width), h: Math.round(r.height) }; })
        .filter((b) => b.w > 0 && b.h > 0 && (b.w < 44 || b.h < 44))
    );
    for (const i of items) small.push({ tab: label, ...i });
  };
  await scan("tree");
  for (const t of TABS) {
    await page.getByRole("button", { name: new RegExp(t, "i") }).last().click({ timeout: 5000 }).catch(() => {});
    await page.waitForTimeout(500);
    await scan(t);
  }
  record("a11y", "touch targets >= 44x44", small.length === 0, { violations: small.length, sample: small.slice(0, 25) });
  // reported, not gated — this is a polish finding, not a broken control
  console.log(`[mobile] sub-44px touch targets: ${small.length}`);
});

test("mobile — ?demo=1 sandbox has no escape hatch", async ({ page }) => {
  test.setTimeout(120000);
  await bootPhone(page, "?e2e=1&demo=1");
  const state = await page.evaluate(() => {
    const s = (window as any).twin.getState();
    return {
      demoLock: s.demoLock, guest: s.guest,
      escapeButtons: Array.from(document.querySelectorAll("button"))
        .map((b) => (b.getAttribute("aria-label") || b.textContent || "").trim())
        .filter((t) => /full console|✕/i.test(t)),
    };
  });
  record("demo-lock", "demo lock armed", state.demoLock === true, state);
  record("demo-lock", "no ✕ / full-console escape in demo", state.escapeButtons.length === 0, state);
  expect(state.demoLock, "?demo=1 must arm the lock").toBe(true);
  expect(state.escapeButtons, "demo sandbox must not expose a path to the operator console").toEqual([]);
});

test("mobile — PWA is installable: manifest + non-empty icons (2e1928e)", async ({ page }) => {
  test.setTimeout(120000);
  await bootPhone(page);
  const href = await page.getAttribute('link[rel="manifest"]', "href");
  record("pwa", "manifest link present", !!href, { href });
  expect(href, "no manifest link").toBeTruthy();

  const manifest = await page.evaluate(async (h) => {
    const r = await fetch(h!);
    return r.ok ? await r.json() : null;
  }, href);
  record("pwa", "manifest fetches + parses", !!manifest, { icons: manifest?.icons?.length ?? 0 });
  expect(manifest, "manifest did not parse").toBeTruthy();

  const icons = manifest.icons || [];
  const sizes: any[] = [];
  for (const ic of icons) {
    const r = await page.evaluate(async (src) => {
      const res = await fetch(src);
      const b = await res.arrayBuffer();
      return { src, ok: res.ok, bytes: b.byteLength };
    }, new URL(ic.src, page.url()).toString());
    sizes.push({ ...r, declared: ic.sizes });
  }
  const empty = sizes.filter((s) => !s.ok || s.bytes < 100);
  record("pwa", "every declared icon has real bytes", empty.length === 0, { icons: sizes, empty });
  expect(empty, `EMPTY PWA icons (Android will refuse to install): ${JSON.stringify(empty)}`).toEqual([]);
});

test.afterAll(() => {
  fs.writeFileSync("mobile-audit.json", JSON.stringify(findings, null, 2));
});
