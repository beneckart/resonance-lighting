# 0019 — Use PowerFeather-SDK 2.x for PowerFeather V2 prototypes

**Date:** 2026-05-11
**Status:** Accepted for COTS prototype track
**Owners:** Ben

## Context

PowerFeather V2 is the leading COTS candidate for Resonance Lighting because it combines:

- ESP32-S3-WROOM-1 with onboard PCB antenna,
- solar/DC input,
- BQ25628E charger/power-path,
- LiFePO4-capable V2 hardware,
- MAX17260 fuel gauge,
- TPS631013 buck-boost 3.3 V regulator,
- controllable `3V3` and `VSQT` rails,
- STEMMA-QT I2C connector,
- low-power states.

A new PowerFeather-SDK 2.0.0 release adds first-class V2 support, MAX17260 support, LiFePO4 mode, Generic_LFP battery type, custom battery profiles, thermistor/fuel-gauge temperature integration, and improved charger/fuel-gauge initialization safety.

## Options considered

- **Use PowerFeather-SDK 1.x:** stable for V1/LiPo, but lacks the V2/MAX17260/LiFePO4 support needed for the preferred track.
- **Write direct BQ25628E/MAX17260/TPS631013 drivers ourselves:** maximum control, but unnecessary risk for early prototypes and duplicates work now provided upstream.
- **Use PowerFeather-SDK 2.x:** aligns with V2 hardware and exposes the telemetry/power-management features the project needs.

## Decision

Use **PowerFeather-SDK 2.x** for the PowerFeather V2 COTS prototype track.

Firmware for this track should be built against ESP-IDF >=5.2 and <=5.5, using V2 board selection through ESP-IDF Kconfig or `POWERFEATHER_BOARD_V2`.

## Consequences

- The first PowerFeather V2 firmware should not be written against old 1.x APIs.
- Project firmware needs a `PowerTelemetry` abstraction so board-specific SDK calls do not leak into CA/render logic.
- The prototype telemetry schema should include voltage, current, state-of-charge, health, cycles, temperature, time-to-empty/full, charger state, supply voltage/current, and faults/alarms.
- V2 tests must verify `VSQT` off behavior, because SDK 2.0.0 claims V2 keeps power-management I2C available while `VSQT` is disabled.
- V2 tests must verify LiFePO4 charging policy and custom profile behavior, not just compile/run.
- If PowerFeather V2 is used in production, SDK 2.x becomes a production dependency and should be version-pinned.

## Open questions

- Whether Elecrow-shipped boards are definitely V2 hardware.
- Whether V2 KiCad layout/Gerbers can be obtained from the PowerFeather creator.
- Whether SDK 2.x telemetry APIs provide all raw fields needed for BM 2026/2027 solar and battery analysis.
- Whether SDK 2.x behavior under watchdog/POR/brownout recovery matches deployment needs.
