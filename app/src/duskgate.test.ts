/** ADR-0031 dusk gate — schedule primary, telemetry confirmation.
 *  Both answers proven: midday fleet edges BLOCKED, dusk-window edges PASS,
 *  and the sim path stays instant. Burn-week dates, no Date.now anywhere. */
import { beforeEach, describe, expect, it } from "vitest";
import { BRC, inDuskWindow, sunsetUtcMs } from "./duskgate";
import { useTwin } from "./store";

// Burn Saturday 2026-09-05. BRC is UTC-7 (PDT): early-Sept sunset ≈ 19:20 local
// ≈ 02:20 UTC on 09-06.
const BURN_DAY_NOON_UTC = Date.UTC(2026, 8, 5, 19, 0, 0); // 12:00 PDT

describe("sunsetUtcMs", () => {
  it("puts burn-week BRC sunset near 19:20 local (±15 min)", () => {
    const sunset = sunsetUtcMs(new Date(BURN_DAY_NOON_UTC), BRC.lat, BRC.lon);
    const localMin = ((sunset / 60000) % 1440 + 1440) % 1440 - 7 * 60; // UTC→PDT
    const norm = (localMin + 1440) % 1440;
    expect(norm).toBeGreaterThan(19 * 60 + 5);
    expect(norm).toBeLessThan(19 * 60 + 35);
  });
});

describe("inDuskWindow", () => {
  const sunset = sunsetUtcMs(new Date(BURN_DAY_NOON_UTC), BRC.lat, BRC.lon);
  it("rejects midday, accepts the window, rejects deep night", () => {
    expect(inDuskWindow(BURN_DAY_NOON_UTC)).toBe(false); // 12:00 PDT — the cloud case
    expect(inDuskWindow(sunset - 30 * 60000)).toBe(true); // 30 min before sunset
    expect(inDuskWindow(sunset + 60 * 60000)).toBe(true); // 60 min after
    expect(inDuskWindow(sunset + 4 * 3600000)).toBe(false); // 23:20 — way past
  });
});

describe("solarPanelsCharging × dusk gate", () => {
  beforeEach(() => {
    useTwin.setState({ solarChargingCount: 3, activeShow: null, guest: false });
    useTwin.setState((s) => ({ control: { ...s.control, blackout: false } }));
  });

  it("FLEET edge at midday is BLOCKED (the ADR-0031 cloud case)", () => {
    useTwin.getState().solarPanelsCharging(0, "fleet", BURN_DAY_NOON_UTC);
    expect(useTwin.getState().activeShow).toBeNull();
    expect(useTwin.getState().solarChargingCount).toBe(0); // census still tracked
  });

  it("FLEET edge inside the dusk window FIRES Solar Ray", () => {
    const sunset = sunsetUtcMs(new Date(BURN_DAY_NOON_UTC), BRC.lat, BRC.lon);
    useTwin.getState().solarPanelsCharging(0, "fleet", sunset - 10 * 60000);
    expect(useTwin.getState().activeShow).toBe("solarray-show");
  });

  it("SIM edge stays instant at any hour (previews are exempt)", () => {
    useTwin.getState().solarPanelsCharging(0, "sim", BURN_DAY_NOON_UTC);
    expect(useTwin.getState().activeShow).toBe("solarray-show");
  });
});
