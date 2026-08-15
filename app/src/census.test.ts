import { describe, it, expect } from "vitest";
import {
  emptyRegistry, applyHeartbeat, applyEvent, fleetCensus, litState,
  HEARD_RECENT_MS, LFP_LEDS_OFF_MV, GAUGE_FAULT_MV,
  type Registry,
} from "./macregistry";
import type { HbFrame, EvtFrame } from "./bridge";

/** These fixtures are NOT invented. Every row is telemetry read off the real
 *  fleet through the cambium daemon on 2026-08-15 (bridge E39F1C, channel 11).
 *  A census validated only against numbers I made up would be a guess with a
 *  test around it — the failure modes below (null lifecycle on a RUNNING light,
 *  a 121 mV gauge, a fleet parked under the LEDs-off floor) are exactly the
 *  ones the invented version would have missed. */
const hb = (
  mac: string,
  battMv: number,
  battMa: number,
  lifeState: number | null,
  program: number | null,
  powerTier: number | null,
  seq = 1,
): HbFrame => ({
  kind: "hb", mac, seq, uptimeMs: 1000, battMv, battMa, soc: 1,
  resetReason: 0, caState: lifeState ?? 0, mode: 0, dlPdrX1000: 0, dlRssi: -60,
  lifeState, program, powerTier,
});

const evt = (mac: string, intensity: number): EvtFrame => ({
  kind: "evt", mac, seq: 2, event: "state", value: 3, intensity,
});

/** the observed fleet, as measured */
function liveFleet(now = 10_000): Registry {
  let reg = emptyRegistry();
  // running a program — note life_state/program/power_tier all arrived NULL
  reg = applyHeartbeat(reg, hb("9E5A84", 3072, -119, null, null, null), now);
  reg = applyEvent(reg, evt("9E5A84", 25), now); // choreo says intensity 25
  // dormant at the lowest power tier — the fleet's typical state today
  reg = applyHeartbeat(reg, hb("F2B7DC", 2897, -144, 1, 0, 3), now);
  reg = applyHeartbeat(reg, hb("9F275C", 3265, -131, 1, 0, 3), now);
  reg = applyHeartbeat(reg, hb("F2BF54", 2922, -67, 1, 0, 3), now);
  // reports nothing about its lifecycle at all
  reg = applyHeartbeat(reg, hb("9E5A94", 2953, 2, 0, 0, 0), now);
  // heartbeating fine, but the fuel gauge is reporting 121 mV
  reg = applyHeartbeat(reg, hb("9F26BC", 121, 0, 1, 0, 0), now);
  return reg;
}

describe("litState — the answer must be able to be 'I don't know'", () => {
  it("calls a fixture LIT from choreo intensity even when the lifecycle fields are NULL", () => {
    // THE regression this whole change exists for: the seam used to coerce
    // life_state null -> 0, filing a running light under "dormant".
    const reg = liveFleet();
    expect(litState(reg.records["9E5A84"])).toBe("lit");
  });

  it("calls a fixture DORMANT only on positive evidence (life 1 + program 0)", () => {
    const reg = liveFleet();
    expect(litState(reg.records["F2B7DC"])).toBe("dormant");
    expect(litState(reg.records["9F275C"])).toBe("dormant");
  });

  it("returns UNKNOWN rather than guessing 'off' when the fixture hasn't said", () => {
    const reg = liveFleet();
    expect(litState(reg.records["9E5A94"])).toBe("unknown");
  });

  it("treats life_state 3 as lit even with no choreo event", () => {
    let reg = emptyRegistry();
    reg = applyHeartbeat(reg, hb("AAAAAA", 3200, -100, 3, 1, 0), 1000);
    expect(litState(reg.records["AAAAAA"])).toBe("lit");
  });
});

describe("fleetCensus", () => {
  it("reproduces the shape of the real 2026-08-15 reading", () => {
    const c = fleetCensus(liveFleet(), 10_000);
    expect(c.total).toBe(6);
    expect(c.lit).toBe(1); // exactly one light was on
    // 4, not 3: 9F26BC's fuel gauge is broken but its LIFECYCLE is healthy and
    // says life 1 / program 0, so it is genuinely dormant. A bad battery reading
    // does not make a fixture's other telemetry untrustworthy — I expected 3
    // here and the code was right.
    expect(c.dormant).toBe(4);
    expect(c.unknown).toBe(1);
    expect(c.lit + c.dormant + c.unknown).toBe(c.total); // no fixture unclassified
  });

  it("EXCLUDES a broken gauge from the battery stats instead of poisoning them", () => {
    const c = fleetCensus(liveFleet(), 10_000);
    // 121 mV would drag a mean/min into nonsense and invent a dead fixture
    expect(c.gaugeFaults).toEqual(["9F26BC"]);
    expect(c.minMv).toBeGreaterThanOrEqual(GAUGE_FAULT_MV);
    expect(c.minMv).toBe(2897);
    expect(c.maxMv).toBe(3265);
  });

  it("counts fixtures under Ben's ADR-0023 LEDs-off floor", () => {
    const c = fleetCensus(liveFleet(), 10_000);
    // 2897, 2922, 2953(no—above)… only those at/under 2950
    expect(c.belowLedsOff).toBe(2);
    expect(LFP_LEDS_OFF_MV).toBe(2950);
  });

  it("counts CHARGING by the sign of battMa, which is the actionable number", () => {
    const c = fleetCensus(liveFleet(), 10_000);
    expect(c.charging).toBe(1); // only 9E5A94 at +2 mA
  });

  it("reports null (not zero) battery when nothing has been heard", () => {
    const c = fleetCensus(emptyRegistry(), 1000);
    expect(c.medianMv).toBeNull(); // 0 would render as "0 mV" and read as dead
    expect(c.total).toBe(0);
  });
});

describe("heard-windows — the count is a function of listen time", () => {
  it("drops a fixture out of 'recent' without ever deleting it", () => {
    const reg = liveFleet(0);
    const later = HEARD_RECENT_MS + 5_000;
    const c = fleetCensus(reg, later);
    expect(c.heardRecent).toBe(0); // nobody spoke in the last 60 s
    expect(c.heardWindow).toBe(6); // but all 6 are inside the 5-minute window
    expect(c.total).toBe(6); // and the roster never shrinks
  });

  it("counts a slow tier-3 fixture as HEARD where the 2.5 s offline sweep would not", () => {
    // this is the real-fleet failure: ~0.21 Hz staggered beats, minutes apart
    let reg = emptyRegistry();
    reg = applyHeartbeat(reg, hb("F2BF8C", 2920, -145, 1, 0, 3), 0);
    const c = fleetCensus(reg, 45_000); // 45 s of silence — normal for tier 3
    expect(c.heardRecent).toBe(1);
    expect(c.heardWindow).toBe(1);
  });

  it("reports how long it has been listening, so no count is quoted bare", () => {
    const c = fleetCensus(liveFleet(0), 300_000, 0);
    expect(c.listeningMs).toBe(300_000);
  });
});

describe("sparse reporting must not erase what we already know", () => {
  it("keeps the last known lifecycle when a later heartbeat omits it", () => {
    let reg = emptyRegistry();
    reg = applyHeartbeat(reg, hb("BBBBBB", 3100, -100, 1, 0, 3, 1), 1000);
    expect(reg.records["BBBBBB"].powerTier).toBe(3);
    // next beat carries nulls — a flickering "??" is worse than a stale number
    reg = applyHeartbeat(reg, hb("BBBBBB", 3090, -100, null, null, null, 2), 2000);
    expect(reg.records["BBBBBB"].powerTier).toBe(3);
    expect(reg.records["BBBBBB"].lifeState).toBe(1);
    expect(reg.records["BBBBBB"].battMv).toBe(3090); // but fresh values DO land
  });
});
