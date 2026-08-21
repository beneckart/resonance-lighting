/** LIVE end-to-end: CambiumBridge ⇄ a REAL cambium daemon (fakefleet).
 *
 *  Skipped unless a daemon is listening on :8600 — start one with:
 *    cambium fakefleet run --fixtures app/public/fixtures.json --start-night
 *
 *  What this proves that the unit tests cannot: the actual ws_protocol
 *  validator accepts our frames (no {"kind":"err"} back), the roster join
 *  resolves OUR fixture ids (the exact failure Justin hit — "synthetic ids
 *  can't match the sim's"), and real hb payloads translate into the seam. */
import { afterAll, beforeAll, describe, expect, it } from "vitest";
import { CambiumBridge } from "./cambium";
import type { UpFrame } from "./bridge";
import type { CambiumMeta } from "./cambium";

const URL_ = "ws://localhost:8600/ws";

async function daemonUp(): Promise<boolean> {
  try {
    const r = await fetch("http://localhost:8600/fakefleet/", { signal: AbortSignal.timeout(1500) });
    return r.ok;
  } catch { return false; }
}

const up = await daemonUp();
const d = describe.skipIf(!up);

d("CambiumBridge ⇄ live fakefleet daemon", () => {
  let bridge: CambiumBridge;
  const ups: UpFrame[] = [];
  const errs: CambiumMeta[] = [];

  beforeAll(async () => {
    bridge = new CambiumBridge({ url: URL_ });
    bridge.onUp((f) => ups.push(f));
    bridge.onMeta((m) => { if (m.kind === "err") errs.push(m); });
    await bridge.connect();
  });
  afterAll(() => bridge.disconnect());

  it("receives real heartbeats translated into the seam", async () => {
    await new Promise((r) => setTimeout(r, 3000)); // fleet hb cadence
    const hbs = ups.filter((f) => f.kind === "hb");
    expect(hbs.length).toBeGreaterThan(0);
    const hb = hbs[0];
    if (hb.kind !== "hb") throw new Error("unreachable");
    expect(hb.mac).toMatch(/^[0-9A-F]{6}$/);
    expect(hb.battMv).toBeGreaterThan(2500); // LFP range, not undefined/0
    expect(hb.soc).toBeGreaterThan(0);
  }, 15000);

  it("drive + frame with OUR fixture ids draws NO protocol error (roster join works)", async () => {
    bridge.send({ kind: "drive", on: true });
    errs.length = 0;
    // F000 is the first fixture_id in app/public/fixtures.json (schema 0.3)
    bridge.sendFrame([{ id: "F000", rgb: [1, 0, 0] }]);
    bridge.sendFrame([{ id: "F000", rgb: [0, 1, 0] }]);
    await new Promise((r) => setTimeout(r, 1200));
    expect(errs.map((e) => e.payload.msg)).toEqual([]);
  }, 10000);

  it("a deliberately bad frame DOES draw err — the error channel itself is proven live", async () => {
    errs.length = 0;
    bridge.send({ kind: "frame", seq: 1, fixtures: "nope" } as never);
    await new Promise((r) => setTimeout(r, 1200));
    expect(errs.length).toBeGreaterThan(0);
  }, 10000);

  it("identify whole-fleet (null mac) passes the validator", async () => {
    errs.length = 0;
    bridge.send({ kind: "identify", mac: null, seconds: 1 });
    await new Promise((r) => setTimeout(r, 800));
    expect(errs.map((e) => e.payload.msg)).toEqual([]);
  }, 10000);
});

if (!up) {
  it("e2e skipped — no cambium daemon on :8600 (start fakefleet to enable)", () => {
    expect(up).toBe(false);
  });
}
