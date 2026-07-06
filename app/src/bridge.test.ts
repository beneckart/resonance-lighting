import { describe, expect, it } from "vitest";
import { MockBridge, type UpFrame } from "./bridge";
import {
  applyEvent, applyHeartbeat, emptyRegistry, exportCsv, onlineCount,
  sweepOffline, uplinkPdr,
} from "./macregistry";

const SPECS = Array.from({ length: 10 }, (_, i) => ({ mac: `MAC${String(i).padStart(3, "0")}`, role: "downlight" }));

function collect(bridge: MockBridge): UpFrame[] {
  const frames: UpFrame[] = [];
  bridge.onUp((f) => frames.push(f));
  return frames;
}

describe("MockBridge (the fleet sim behind the real seam)", () => {
  it("emits heartbeats at ~2 Hz per node with jitter", async () => {
    const b = new MockBridge(SPECS, 7);
    await b.connect();
    const frames = collect(b);
    for (let t = 0; t < 100; t++) b.tick(100); // 10 s
    const hbs = frames.filter((f) => f.kind === "hb");
    // 10 nodes × 2 Hz × 10 s ≈ 200 (jitter makes it approximate)
    expect(hbs.length).toBeGreaterThan(150);
    expect(hbs.length).toBeLessThan(250);
    // every node reports
    expect(new Set(hbs.map((f) => f.mac)).size).toBe(10);
  });

  it("local rule edges emit INSTANT events without polling", async () => {
    const b = new MockBridge(SPECS, 7);
    await b.connect();
    const frames = collect(b);
    for (let t = 0; t < 50; t++) b.tick(100); // 5 s
    const evts = frames.filter((f) => f.kind === "evt" && f.event === "state");
    expect(evts.length).toBeGreaterThan(5); // rules ticking on-fixture
  });

  it("tap → event arrives immediately (the two-way instant path)", async () => {
    const b = new MockBridge(SPECS, 7);
    await b.connect();
    const frames = collect(b);
    b.tap("MAC003");
    const evt = frames.find((f) => f.kind === "evt" && f.event === "tap");
    expect(evt).toBeTruthy();
    expect(evt!.mac).toBe("MAC003");
  });

  it("identify targets one node (or all) and acks", async () => {
    const b = new MockBridge(SPECS, 7);
    await b.connect();
    const frames = collect(b);
    b.send({ kind: "identify", mac: "MAC005", seconds: 5 });
    const acks = frames.filter((f) => f.kind === "evt" && f.event === "identify_ack");
    expect(acks).toHaveLength(1);
    expect(acks[0].mac).toBe("MAC005");
    b.send({ kind: "identify", mac: null, seconds: 2 });
    expect(frames.filter((f) => f.kind === "evt" && f.event === "identify_ack")).toHaveLength(11);
  });

  it("set_rate slows the heartbeat (battery conservation mode)", async () => {
    const b = new MockBridge(SPECS, 7);
    await b.connect();
    b.send({ kind: "set_rate", hbHz: 0.2, frameHz: 0 });
    const frames = collect(b);
    for (let t = 0; t < 100; t++) b.tick(100); // 10 s
    const hbs = frames.filter((f) => f.kind === "hb");
    // 10 nodes × 0.2 Hz × 10 s ≈ 20
    expect(hbs.length).toBeLessThan(40);
  });

  it("emits nothing when disconnected", () => {
    const b = new MockBridge(SPECS, 7);
    const frames = collect(b);
    b.tick(5000);
    b.tap("MAC001");
    expect(frames).toHaveLength(0);
  });
});

describe("MAC registry (map + log every light)", () => {
  const hb = (mac: string, seq: number, uptimeMs = seq * 500): Parameters<typeof applyHeartbeat>[1] => ({
    kind: "hb", mac, seq, uptimeMs, battMv: 3300, battMa: -60, soc: 80,
    resetReason: 1, caState: 3, mode: 1, dlPdrX1000: 990, dlRssi: -48,
  });

  it("first heartbeat registers the MAC with a first_heard event", () => {
    const reg = emptyRegistry();
    applyHeartbeat(reg, hb("AABBCC", 1), 1000);
    expect(reg.records["AABBCC"]).toBeTruthy();
    expect(reg.events.some((e) => e.kind === "first_heard" && e.mac === "AABBCC")).toBe(true);
  });

  it("detects reboots from uptime regression", () => {
    const reg = emptyRegistry();
    applyHeartbeat(reg, hb("AABBCC", 1, 60_000), 1000);
    applyHeartbeat(reg, hb("AABBCC", 2, 61_000), 2000);
    applyHeartbeat(reg, hb("AABBCC", 3, 1_000), 3000); // rebooted
    expect(reg.records["AABBCC"].reboots).toBe(1);
    expect(reg.events.some((e) => e.kind === "boot")).toBe(true);
  });

  it("counts sequence gaps as lost frames (uplink PDR)", () => {
    const reg = emptyRegistry();
    applyHeartbeat(reg, hb("AABBCC", 1), 1000);
    applyHeartbeat(reg, hb("AABBCC", 5), 3000); // 2,3,4 lost
    expect(reg.records["AABBCC"].lost).toBe(3);
    expect(uplinkPdr(reg.records["AABBCC"])).toBeCloseTo(2 / 5, 2);
  });

  it("events update state instantly and are logged", () => {
    const reg = emptyRegistry();
    applyHeartbeat(reg, hb("AABBCC", 1), 1000);
    applyEvent(reg, { kind: "evt", mac: "AABBCC", seq: 2, event: "tap", value: 6 }, 1100);
    expect(reg.records["AABBCC"].caState).toBe(6);
    expect(reg.events[reg.events.length - 1].kind).toBe("tap");
  });

  it("offline sweep flags quiet nodes, heartbeat brings them back online", () => {
    const reg = emptyRegistry();
    applyHeartbeat(reg, hb("AABBCC", 1), 1000);
    applyHeartbeat(reg, hb("DDEEFF", 1), 1000);
    applyHeartbeat(reg, hb("AABBCC", 2), 4000);
    const off = sweepOffline(reg, 4100);
    expect(off).toEqual(["DDEEFF"]);
    expect(onlineCount(reg)).toEqual({ online: 1, total: 2 });
    applyHeartbeat(reg, hb("DDEEFF", 2), 4200);
    expect(onlineCount(reg)).toEqual({ online: 2, total: 2 });
    expect(reg.events.some((e) => e.kind === "online" && e.mac === "DDEEFF")).toBe(true);
  });

  it("exports a CSV log", () => {
    const reg = emptyRegistry();
    applyHeartbeat(reg, hb("AABBCC", 1), 1000);
    const csv = exportCsv(reg);
    expect(csv.split("\n")).toHaveLength(2);
    expect(csv).toContain("AABBCC");
    expect(csv.split("\n")[0]).toContain("uplink_pdr");
  });

  it("bridge + registry end-to-end: fleet state lands in the ledger", async () => {
    const b = new MockBridge(SPECS, 7);
    await b.connect();
    const reg = emptyRegistry();
    let clock = 0;
    b.onUp((f) => {
      if (f.kind === "hb") applyHeartbeat(reg, f, clock);
      else applyEvent(reg, f, clock);
    });
    for (let t = 0; t < 30; t++) { clock += 100; b.tick(100); }
    expect(onlineCount(reg).total).toBe(10);
    expect(Object.values(reg.records).every((r) => r.battMv > 2900)).toBe(true);
  });
});
