# PowerFeather V2 + SDK 2.0.0 validation plan

**Date:** 2026-05-11
**Status:** Draft test plan
**Purpose:** Validate PowerFeather V2 as the leading COTS LiFePO4 + solar + telemetry platform for Resonance Lighting.

## Hardware under test

- ESP32-S3 PowerFeather, ideally V2.
- LiFePO4 cell, preferably single 18650 1500-2000 mAh for production-like tests.
- Solar panels: 1 W, 2 W, 3 W, 5 W rectangular/square prototypes.
- Adafruit IS31FL3741 13x9 matrix over STEMMA-QT.
- Optional: M5Stack NeoHEX via GPIO/WS2812 data and suitable power rail.
- Optional: thermistor at/near battery if not integrated into cell/holder.

## Software under test

- ESP-IDF >=5.2, <=5.5.
- PowerFeather-SDK >=2.0.0.
- Build option: `POWERFEATHER_BOARD_V2` or ESP-IDF Kconfig V2 selection.
- Initial battery type: `BatteryType::Generic_LFP`, unless a custom MAX17260 battery model is available.

## Phase 1 — identify hardware version

- [ ] Photograph front/back of each PowerFeather under macro.
- [ ] Visually identify V2 components if possible: MAX17260, TPS631013, BQ25628E, 20 mΩ sense resistor.
- [ ] I2C scan internal bus.
- [ ] Confirm fuel gauge is MAX17260, not LC709204F.
- [ ] Confirm regulator/control device expected for V2.
- [ ] Label physical boards `PFV2-001`, `PFV2-002`, etc.

## Phase 2 — minimal SDK bring-up

- [ ] Build minimal SDK 2.0.0 firmware for V2.
- [ ] Run `Board.init()` with no battery, powered from USB/VDC.
- [ ] Run LiFePO4 initialization with `BatteryType::Generic_LFP`.
- [ ] Confirm charger part validation succeeds.
- [ ] Confirm no unexpected SDK errors on boot.
- [ ] Log SDK version, ESP-IDF version, board config, and detected devices.

## Phase 3 — battery and fuel-gauge telemetry

Log once per second for at least 10 minutes, then once per minute for long tests:

- battery voltage,
- battery current,
- state of charge,
- health,
- cycle count,
- time to empty/full,
- board/battery temperature,
- charger state,
- supply voltage/current,
- charge current limit,
- maintain-supply / pseudo-MPPT voltage setting,
- faults/alarms.

Tests:

- [ ] Battery-only idle.
- [ ] Battery-only with LED matrix off.
- [ ] Battery-only with LED center pixel.
- [ ] Battery-only with LED 3-pixel chromatic mode.
- [ ] Battery-only with LED matrix stress mode.
- [ ] USB/VDC input with battery attached.
- [ ] VDC input with no battery.
- [ ] VDC input with depleted or partially depleted battery.

## Phase 4 — VSQT / external LED-module power behavior

For the IS31FL3741 STEMMA-QT matrix:

- [ ] Attach matrix over STEMMA-QT.
- [ ] Confirm matrix appears on I2C when `VSQT` is enabled.
- [ ] Turn matrix center pixel on.
- [ ] Turn matrix off via driver.
- [ ] Disable `VSQT`.
- [ ] Verify matrix no longer consumes meaningful current.
- [ ] Verify PowerFeather power-management I2C still works with `VSQT` disabled.
- [ ] Re-enable `VSQT`.
- [ ] Reinitialize matrix and display center pixel.
- [ ] Repeat 100 cycles and log any I2C or matrix failures.

## Phase 5 — sleep current

Measure with a USB power meter only for rough tests; use a power profiler or DMM/uCurrent-style setup for serious sleep numbers.

Cases:

- [ ] Deep sleep, fuel gauge enabled, no external module.
- [ ] Deep sleep, fuel gauge disabled, no external module.
- [ ] Deep sleep, IS31FL3741 attached, `VSQT` disabled.
- [ ] Deep sleep, IS31FL3741 attached, `VSQT` enabled but display off.
- [ ] Ship mode.
- [ ] Shutdown mode.

Record whether results match the documented order-of-magnitude: tens of µA in deep sleep and ~1 µA in ship/shutdown.

## Phase 6 — solar/VDC input

For each panel:

- [ ] Measure open-circuit voltage.
- [ ] Measure loaded voltage into PowerFeather VDC.
- [ ] Set maintain-supply voltage appropriate for panel MPP.
- [ ] Record charger/supply telemetry in full sun.
- [ ] Record telemetry in partial shade.
- [ ] Record telemetry through the actual hat top / mounting geometry if available.
- [ ] Confirm panel does not collapse below maintain voltage under normal conditions.
- [ ] Confirm battery charge current stays within selected cell limits.

## Phase 7 — failure and recovery

- [ ] Watchdog reset while LEDs are on.
- [ ] Brownout-like low-battery condition if safely reproducible.
- [ ] Warm reboot with RTC preserved; confirm charger settings retained or safely re-applied.
- [ ] Full power loss and restart; confirm policy re-applied.
- [ ] Missing/open/shorted thermistor behavior if practical.
- [ ] Low-battery cutoff: external LED rail off, telemetry preserved, no reboot loop.
- [ ] Dead/depleted battery + sun recovery test.

## Phase 8 — RF and enclosure

- [ ] Run ESP-NOW packet tests bare board.
- [ ] Run ESP-NOW packet tests inside mock hat with battery and panel in realistic positions.
- [ ] Test antenna orientation relative to solar panel, battery, and screws.
- [ ] Log RSSI, packet loss, and latency for 2-node and 5-node cases.

## Pass criteria for COTS production candidacy

PowerFeather V2 remains a production candidate if:

- It is confirmed to be V2 hardware.
- LiFePO4 initialization and charging work reliably through SDK 2.x.
- Sleep current with LED module attached/off is acceptable.
- Solar input recovers daily drain with realistic panel/housing conditions.
- `VSQT` can reliably power-cycle the LED module without breaking telemetry.
- No stuck-on LED or low-battery reboot-loop failure is observed.
- RF works in the final or near-final hat geometry.
- Assembly requires only simple, repeatable steps: panel pigtail, battery JST, STEMMA-QT cable, screws/standoffs, USB/pogo flashing.
