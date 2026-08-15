/** CambiumBridge — wire-contract tests against a scripted fake WebSocket.
 *
 *  The fake speaks EXACTLY the shapes in cambium/api/ws_protocol.py and
 *  uplink/fleetstate.py (read 2026-08-13, commit bea2a2d). If cambium's
 *  vocabulary moves, update the fixtures here FROM THE SOURCE — these tests
 *  are the contract mirror, not an aspiration. */
import { describe, expect, it, vi } from "vitest";
import { CambiumBridge, startFramePump, type WsLike } from "./cambium";
import type { UpFrame } from "./bridge";

class FakeWs implements WsLike {
  readyState = 0;
  sent: string[] = [];
  closed = false;
  onopen: ((ev?: unknown) => void) | null = null;
  onclose: ((ev?: unknown) => void) | null = null;
  onerror: ((ev?: unknown) => void) | null = null;
  onmessage: ((ev: { data: unknown }) => void) | null = null;
  send(data: string): void { this.sent.push(data); }
  close(): void { this.closed = true; this.readyState = 3; this.onclose?.(); }
  // test drivers
  open(): void { this.readyState = 1; this.onopen?.(); }
  push(obj: unknown): void { this.onmessage?.({ data: JSON.stringify(obj) }); }
  pushRaw(data: unknown): void { this.onmessage?.({ data }); }
}

function bridgeWithFake(now?: () => number) {
  const sockets: FakeWs[] = [];
  const b = new CambiumBridge({
    url: "ws://test:8600/ws",
    wsFactory: () => { const ws = new FakeWs(); sockets.push(ws); return ws; },
    now,
  });
  return { b, sockets };
}

/** the exact hb payload fleetstate._apply_heartbeat emits (flattened by WsHub) */
const HB_WIRE = {
  kind: "hb", rssi: -48, mac: "f2be80", batt_mv: 3291, batt_ma: -63,
  soc_pct: 71, dl_rssi: -52, mode: 1, last_seen: 12.5,
  life_state: 3, program: 2, power_tier: 1,
};

describe("CambiumBridge", () => {
  it("connects when the socket opens and reports connected()", async () => {
    const { b, sockets } = bridgeWithFake();
    const p = b.connect();
    sockets[0].open();
    await p;
    expect(b.connected()).toBe(true);
  });

  it("translates cambium hb (snake_case) into the seam's HbFrame", async () => {
    let t = 1000;
    const { b, sockets } = bridgeWithFake(() => t);
    const up: UpFrame[] = [];
    b.onUp((f) => up.push(f));
    const p = b.connect(); sockets[0].open(); await p;

    t = 1000; sockets[0].push(HB_WIRE);
    t = 3500; sockets[0].push(HB_WIRE);

    expect(up).toHaveLength(2);
    const f = up[1];
    expect(f).toMatchObject({
      kind: "hb", mac: "F2BE80", seq: 2, uptimeMs: 2500,
      battMv: 3291, battMa: -63, soc: 71,
      caState: 3, mode: 1, dlRssi: -52, dlPdrX1000: 0,
    });
  });

  it("translates cambium evt (choreo state edge) into an EvtFrame", async () => {
    const { b, sockets } = bridgeWithFake();
    const up: UpFrame[] = [];
    b.onUp((f) => up.push(f));
    const p = b.connect(); sockets[0].open(); await p;

    sockets[0].push({ kind: "evt", mac: "a1b2c3", state: 5, prev_state: 2, program: 1, generation: 7, intensity: 0.4 });
    expect(up).toEqual([
      { kind: "evt", mac: "A1B2C3", seq: 1, event: "state", value: 5 },
    ]);
  });

  it("routes charging / bridge_status / err to onMeta, NOT through the seam", async () => {
    const { b, sockets } = bridgeWithFake();
    const up: UpFrame[] = []; const meta: string[] = [];
    b.onUp((f) => up.push(f));
    b.onMeta((m) => meta.push(m.kind));
    const p = b.connect(); sockets[0].open(); await p;

    sockets[0].push({ kind: "charging", count: 4, macs: ["F2BE80"] });
    sockets[0].push({ kind: "bridge_status", serial: "up" });
    sockets[0].push({ kind: "err", msg: "'frame'.seq must be an integer" });
    expect(up).toHaveLength(0);
    expect(meta).toEqual(["open", "charging", "bridge_status", "err"]);
  });

  it("skips malformed wire data without dying (untrusted-wire doctrine)", async () => {
    const { b, sockets } = bridgeWithFake();
    const up: UpFrame[] = [];
    b.onUp((f) => up.push(f));
    const p = b.connect(); sockets[0].open(); await p;

    sockets[0].pushRaw("not json{");
    sockets[0].pushRaw(12345); // binary/odd frame
    sockets[0].push({ kind: "hb" }); // hb with no mac
    sockets[0].push({ kind: "wormhole", mac: "F2BE80" }); // future vocabulary
    sockets[0].push(HB_WIRE); // then a good one still lands
    expect(up).toHaveLength(1);
    expect(up[0].kind).toBe("hb");
  });

  it("identify: null mac (whole fleet) OMITS the key — cambium's mac is optional, not nullable", async () => {
    const { b, sockets } = bridgeWithFake();
    const p = b.connect(); sockets[0].open(); await p;

    b.send({ kind: "identify", mac: null, seconds: 5 });
    b.send({ kind: "identify", mac: "F2BE80", seconds: 3 });
    expect(JSON.parse(sockets[0].sent[0])).toEqual({ kind: "identify", seconds: 5 });
    expect(JSON.parse(sockets[0].sent[1])).toEqual({ kind: "identify", mac: "F2BE80", seconds: 3 });
  });

  it("show / set_rate / drive pass through verbatim; nothing sends before open", async () => {
    const { b, sockets } = bridgeWithFake();
    b.send({ kind: "show", phase: 0.25, hue: 0.6, flags: 0 }); // pre-connect: dropped
    const p = b.connect(); sockets[0].open(); await p;

    b.send({ kind: "show", phase: 0.25, hue: 0.6, flags: 0 });
    b.send({ kind: "set_rate", hbHz: 4, frameHz: 0 });
    b.send({ kind: "drive", on: true });
    const kinds = sockets[0].sent.map((s) => JSON.parse(s).kind);
    expect(kinds).toEqual(["show", "set_rate", "drive"]);
  });

  it("sendFrame owns seq: monotonic across calls, fixtures verbatim", async () => {
    const { b, sockets } = bridgeWithFake();
    const p = b.connect(); sockets[0].open(); await p;

    b.sendFrame([{ id: "F000", rgb: [1.2, 0.5, 0] }]); // >1.0 legal: linear floats
    b.sendFrame([{ id: "F001", rgb: [0, 0, 0.3] }]);
    const frames = sockets[0].sent.map((s) => JSON.parse(s));
    expect(frames[0]).toEqual({ kind: "frame", seq: 1, fixtures: [{ id: "F000", rgb: [1.2, 0.5, 0] }] });
    expect(frames[1].seq).toBe(2);
  });

  it("reconnects with backoff after an established connection drops", async () => {
    vi.useFakeTimers();
    try {
      const { b, sockets } = bridgeWithFake();
      const meta: string[] = [];
      b.onMeta((m) => meta.push(m.kind));
      const p = b.connect(); sockets[0].open(); await p;
      expect(sockets).toHaveLength(1);

      sockets[0].close(); // drop → close meta + retry armed
      expect(b.connected()).toBe(false);
      await vi.advanceTimersByTimeAsync(500);
      expect(sockets).toHaveLength(2); // redial happened
      sockets[1].open();
      expect(b.connected()).toBe(true);
      expect(meta).toEqual(["open", "close", "open"]);
    } finally {
      vi.useRealTimers();
    }
  });

  it("startup case B (app before daemon): failed FIRST connect never zombie-redials", async () => {
    vi.useFakeTimers();
    try {
      const { b, sockets } = bridgeWithFake();
      const p = b.connect();
      sockets[0].onerror?.(); // daemon not there
      sockets[0].close();
      await expect(p).rejects.toThrow(/unreachable/);
      await vi.advanceTimersByTimeAsync(60000);
      expect(sockets).toHaveLength(1); // no background retry loop left behind
      expect(b.connected()).toBe(false);
    } finally {
      vi.useRealTimers();
    }
  });

  it("night gate: NightDown passes through verbatim (the #1 bench trap has a wire path)", async () => {
    const { b, sockets } = bridgeWithFake();
    const p = b.connect(); sockets[0].open(); await p;
    b.send({ kind: "night", mode: 1, mac: null });
    expect(JSON.parse(sockets[0].sent[0])).toEqual({ kind: "night", mode: 1, mac: null });
  });

  it("disconnect() is final: no redial after user intent to stop", async () => {
    vi.useFakeTimers();
    try {
      const { b, sockets } = bridgeWithFake();
      const p = b.connect(); sockets[0].open(); await p;
      b.disconnect();
      await vi.advanceTimersByTimeAsync(20000);
      expect(sockets).toHaveLength(1); // never redialed
      expect(b.connected()).toBe(false);
    } finally {
      vi.useRealTimers();
    }
  });
});

describe("startFramePump", () => {
  it("pumps provider frames at the clamped rate only while connected", async () => {
    const { b, sockets } = bridgeWithFake();
    const p = b.connect(); sockets[0].open(); await p;

    const timers: (() => void)[] = [];
    const schedule = (fn: () => void) => { timers.push(fn); return 0 as unknown as ReturnType<typeof setTimeout>; };
    const stop = startFramePump(b, () => [{ id: "F000", rgb: [0.5, 0.5, 0.5] as [number, number, number] }], 8, schedule);

    timers.shift()!(); // tick 1
    timers.shift()!(); // tick 2
    expect(sockets[0].sent.filter((s) => JSON.parse(s).kind === "frame")).toHaveLength(2);

    stop();
    timers.shift()!(); // dead tick: no send, no reschedule
    expect(sockets[0].sent).toHaveLength(2);
    expect(timers).toHaveLength(0);
  });

  it("sends nothing while the socket is down (no queue growth, no throw)", () => {
    const { b } = bridgeWithFake(); // never connected
    const timers: (() => void)[] = [];
    const schedule = (fn: () => void) => { timers.push(fn); return 0 as unknown as ReturnType<typeof setTimeout>; };
    startFramePump(b, () => [{ id: "F000", rgb: [1, 1, 1] as [number, number, number] }], 8, schedule);
    timers.shift()!();
    timers.shift()!();
    // nothing to assert on a socket that doesn't exist — surviving is the test
    expect(timers).toHaveLength(1);
  });
});

/** urlFromLocation — the ?cambium= contract. The same-origin form exists because
 *  the absolute form has two field failure modes we hit for real (2026-08-15):
 *  the operator must hand-edit the host for every device, and ws:// from an https
 *  page is blocked as mixed content with no visible error. */
describe("CambiumBridge.urlFromLocation", () => {
  const withLocation = <T,>(href: string, fn: () => T): T => {
    const u = new URL(href);
    const g = globalThis as { window?: unknown };
    const prev = g.window;
    g.window = { location: { search: u.search, protocol: u.protocol, host: u.host } };
    try { return fn(); } finally { g.window = prev; }
  };

  it("no ?cambium= → the localhost default", () => {
    expect(withLocation("http://192.168.1.216:5173/", () => CambiumBridge.urlFromLocation()))
      .toBe("ws://localhost:8600/ws");
  });

  it("an absolute ws:// URL passes through untouched", () => {
    expect(withLocation("http://x/?cambium=ws://10.0.0.9:8600/ws", () => CambiumBridge.urlFromLocation()))
      .toBe("ws://10.0.0.9:8600/ws");
  });

  it("?cambium=1 resolves to THIS page's origin — one link works on every device", () => {
    expect(withLocation("http://192.168.1.216:5173/?cambium=1", () => CambiumBridge.urlFromLocation()))
      .toBe("ws://192.168.1.216:5173/cambium/ws");
    expect(withLocation("http://localhost:5173/?cambium=proxy", () => CambiumBridge.urlFromLocation()))
      .toBe("ws://localhost:5173/cambium/ws");
  });

  it("upgrades to wss:// on an https page — the mixed-content trap, closed", () => {
    expect(withLocation("https://tree.example.com/?cambium=1", () => CambiumBridge.urlFromLocation()))
      .toBe("wss://tree.example.com/cambium/ws");
  });

  it("a bare path is taken as a same-origin path", () => {
    expect(withLocation("http://h:5173/?cambium=/alt/ws", () => CambiumBridge.urlFromLocation()))
      .toBe("ws://h:5173/alt/ws");
    expect(withLocation("http://h:5173/?cambium=alt/ws", () => CambiumBridge.urlFromLocation()))
      .toBe("ws://h:5173/alt/ws");
  });
});
