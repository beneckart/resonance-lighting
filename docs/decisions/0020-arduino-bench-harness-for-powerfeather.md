# 0020 — Use Arduino-ESP32 for the PowerFeather power-bench harness

**Date:** 2026-06-02
**Status:** Accepted for the bench/feasibility track (amends 0019)
**Owners:** Ben

## Context

ADR 0019 adopted PowerFeather-SDK 2.x and noted firmware "for this track should be
built against ESP-IDF >=5.2." With the PowerFeather V2.R2 in hand, the immediate
need is a **measurement bench** to answer procurement-blocking questions fast
(which board, battery, LED module, and panel to buy ~100 of) before the August 2026
deadline. Speed-to-data is the guiding constraint.

The existing `firmware/smoke_test` scaffolding (WiFi, web OTA, `/mode` endpoints,
LED drivers for IS31FL3741 and NeoPixel) is Arduino-ESP32 and already proven on the
COTS boards. The PowerFeather-SDK ships as **both** an Arduino library and an
ESP-IDF component, exposing the **identical `PowerFeather::Board` Mainboard API**.
So the power telemetry we collect is framework-independent.

## Options considered

- **ESP-IDF now (per 0019):** best long-term alignment, but re-implements the
  OTA/WiFi/LED scaffolding the smoke test already has; slower to first data.
- **Arduino-ESP32, reuse smoke_test:** fastest path to a working telemetry bench;
  the SDK telemetry calls are identical to ESP-IDF, so numbers port 1:1 later.

## Decision

Build the **power-bench harness on Arduino-ESP32**, reusing the smoke_test
scaffolding (`firmware/power_bench/`). PowerFeather-SDK 2.1.0 is installed via the
Arduino library manager. FQBN `esp32:esp32:esp32s3_powerfeather`, board macro
`ARDUINO_ESP32S3_POWERFEATHER`, ESP32 core 3.3.7.

This is a **bench/feasibility decision only**. The production framework (Arduino vs
ESP-IDF) stays open and will be decided with the Phase 3 COTS-vs-custom call. The
telemetry collected here is valid either way because it comes through the same SDK
`Mainboard` API.

## Consequences

- Data collection can start immediately on Arduino without an ESP-IDF toolchain.
- Battery/supply voltage and current are read through the SDK and are portable to
  an ESP-IDF production build; only the WiFi/OTA shell would change.
- The harness serves telemetry as JSON over WiFi (`/telemetry`) and is logged by
  `ops/bench/power_logger.py` into site-partitioned JSONL for cross-site analysis.
- Chemistry stays a one-line firmware change (`BatteryType::Generic_3V7` ->
  `Generic_LFP`); no hardware jumpers exist.
- If production lands on ESP-IDF, the `PowerTelemetry` abstraction from 0019 still
  applies; this harness informs its field list.

## Notes

- **Mandatory build flag.** The SDK selects the fuel-gauge IC at compile time:
  MAX17260 (V2) only if `POWERFEATHER_BOARD_V2` or `CONFIG_ESP32S3_POWERFEATHER_V2`
  is defined, otherwise the V1 `LC709204F`. Arduino builds set neither by default,
  so the bench build MUST pass `-DPOWERFEATHER_BOARD_V2=1` (handled by
  `firmware/power_bench/build.sh`; enforced by a `#error` guard in the sketch).
  Without it, SOC/health/cycles silently fail (gauge probes the wrong IC). This was
  the cause of the initial "SOC unavailable" symptom; resolved.

## Open questions

- Whether any SDK call behaves differently between the Arduino library and the
  ESP-IDF component (not expected; same Mainboard implementation).
