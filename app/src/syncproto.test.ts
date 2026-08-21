import { describe, expect, it } from "vitest";
import {
  assignCmd, foldSession, isCalFrame, lockCmd, mapHash, surveyCmd,
  type RssiReport, type TofReport,
} from "./syncproto";

describe("frame builders", () => {
  it("surveyCmd builds a valid frame", () => {
    const f = surveyCmd(7);
    expect(f.kind).toBe("cal_survey");
    expect(isCalFrame(f)).toBe(true);
  });
  it("assignCmd builds a valid frame", () => {
    const f = assignCmd("A1B2C3", "F012", "hypothesis", [1, 2, 3], 0.8);
    expect(isCalFrame(f)).toBe(true);
    expect(f.stage).toBe("hypothesis");
  });
  it("lockCmd embeds the map hash", () => {
    const entries = [{ mac: "AA", fixtureId: "F001" }, { mac: "BB", fixtureId: "F002" }];
    const f = lockCmd(7, 3, entries);
    expect(f.mapHash).toBe(mapHash(entries));
    expect(isCalFrame(f)).toBe(true);
  });
});

describe("mapHash", () => {
  it("is order-independent and drift-sensitive", () => {
    const a = [{ mac: "AA", fixtureId: "F001" }, { mac: "BB", fixtureId: "F002" }];
    const b = [a[1], a[0]];
    expect(mapHash(a)).toBe(mapHash(b));
    expect(mapHash(a)).not.toBe(mapHash([{ mac: "AA", fixtureId: "F002" }, { mac: "BB", fixtureId: "F001" }]));
  });
});

describe("isCalFrame", () => {
  it("rejects junk from the untrusted wire", () => {
    expect(isCalFrame(null)).toBe(false);
    expect(isCalFrame({})).toBe(false);
    expect(isCalFrame({ proto: 1, kind: "cal_rssi", session: 1, mac: "AA", role: "x", rows: [{ mac: 3 }] })).toBe(false);
    expect(isCalFrame({ proto: 2, kind: "cal_survey", session: 1, durationS: 10, minPings: 5 })).toBe(false);
    expect(isCalFrame({ proto: 1, kind: "cal_assign", mac: "AA", fixtureId: "F1", stage: "nope", pos: [0, 0, 0], confidence: 1 })).toBe(false);
  });
  it("accepts every well-formed frame kind", () => {
    const rssi: RssiReport = { proto: 1, kind: "cal_rssi", session: 1, mac: "AA", role: "downlight", rows: [{ mac: "BB", med: -55, n: 20 }] };
    const tof: TofReport = { proto: 1, kind: "cal_tof", session: 1, mac: "AA", heightM: 2.5, sigmaM: 0.05, clear: true };
    expect(isCalFrame(rssi)).toBe(true);
    expect(isCalFrame(tof)).toBe(true);
    expect(isCalFrame({ proto: 1, kind: "cal_ack", mac: "AA", fixtureId: "F1", stage: "confirmed" })).toBe(true);
  });
});

describe("foldSession", () => {
  const rssi = (mac: string, rows: { mac: string; med: number; n: number }[]): RssiReport =>
    ({ proto: 1, kind: "cal_rssi", session: 1, mac, role: "downlight", rows });
  const tof = (mac: string, heightM: number | null, clear: boolean): TofReport =>
    ({ proto: 1, kind: "cal_tof", session: 1, mac, heightM, sigmaM: 0.05, clear });

  it("later reports replace earlier ones", () => {
    const nodes = foldSession(
      [rssi("AA", [{ mac: "BB", med: -50, n: 5 }]), rssi("AA", [{ mac: "BB", med: -52, n: 30 }])],
      [],
    );
    expect(nodes).toHaveLength(1);
    expect(nodes[0].rows[0].med).toBe(-52);
  });
  it("gates ToF on the clear-ground flag", () => {
    const nodes = foldSession(
      [rssi("AA", []), rssi("BB", [])],
      [tof("AA", 2.5, true), tof("BB", 1.1, false)], // BB stares into foliage
    );
    expect(nodes.find((n) => n.mac === "AA")!.tofHeightM).toBe(2.5);
    expect(nodes.find((n) => n.mac === "BB")!.tofHeightM).toBeNull();
  });
  it("ignores ToF for macs that never sent an RSSI report", () => {
    const nodes = foldSession([rssi("AA", [])], [tof("ZZ", 2, true)]);
    expect(nodes).toHaveLength(1);
  });
});
