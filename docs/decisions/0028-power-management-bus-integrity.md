# 0028 -- Power-management bus integrity: 100 kHz rule, dedicated bus on custom PCBA

**Date:** 2026-07-08 (records the 2026-07-03 conviction and the 2026-07-05 46-hour
soak seal)
**Status:** Accepted. Constrains ADR 0005's task architecture and any ADR 0012
Track-B custom board; generalizes the ADR 0018 IS31 rejection.
**Owners:** Ben + Claude

## Context

Two months of intermittent battery-only reboots -- the June "brownout" episode
(IS31FL3741 on the shared bus, ADR 0018) and the July presence-bench "reboot
epidemic" -- turned out to be one mechanism. The PowerFeather's Wire1 I2C bus is
shared by the BQ25628E charger/power-path IC (the chip the battery current flows
through), the MAX17260 gauge, and any user peripherals. Degraded signal integrity on
that bus under WiFi TX noise can corrupt transactions near the power path's control
registers (BATFET/ship/EN_HIZ class) and open the battery switch outright: no sag,
no brownout detector -- straight to `reset_reason=poweron`. USB is immune because
VBUS bypasses the BATFET, which is why the failure only ever appeared on battery.

Evidence (LOG 2026-07-02/03/05):

- Controlled A/B, identical firmware, worst board (~60 deaths of history), battery
  at -318 mA full load: **400 kHz Wire1 dies in 10-160 s; 100 kHz runs 900+ s.**
  Only the clock differed.
- **46.2-hour continuous soak** (full five-sensor bench at 100 kHz, mean 209 mA)
  ended by honest cell exhaustion, not failure -- fix considered sealed.
- The June IS31 brownout is the same disturbance class: a chip dragging SDA/SCL
  instead of an elevated clock. The NeoDriver stability result proved that device
  benign, not the bus robust (ADR 0018 addendum 2026-07-03).
- Honest corrections retained: the earlier "core-0 SDK round-robin" attribution was
  inference -- the bisect varied what ran, never where; core placement is at most an
  aggravator. The XM125's ~5 % read errors were retracted as evidence (they persist
  at 100 kHz; separate protocol quirk).
- Cost of the fix: sensor cadence 0.8 -> 0.6 Hz. Negligible.

## Decision

Production rules, firmware and hardware:

1. **Any I2C bus shared with the charger/gauge runs at 100 kHz. Never raise the
   clock.** (100 kHz is now the compiled default in bench firmware.)
2. **A custom PCBA (if the 2027 Track-B board happens) gets a dedicated
   power-management I2C bus** -- charger + gauge only; user peripherals go on a
   separate bus.
3. **No power-management I2C traffic from core-0-pinned tasks while WiFi is
   active** (defense in depth given the un-measured core interaction; direct
   constraint on ADR 0005's task placement).
4. **Treat battery-only `poweron` resets as possible power-path register upsets**
   -- suspect the bus first; port the boot-counter / reset-reason / breadcrumb
   telemetry idiom into production firmware.

## Consequences

- Sensor and SDK polling rates are designed within the 100 kHz budget (keep
  per-frame reads short -- e.g. sway_demo disables per-SPAD ToF data).
- ADR 0018's "LEDs stay off the power-management bus" is strengthened and
  generalized: nothing optional rides that bus at fleet scale.
- The rule is cheap insurance with measured cost ~nothing; violating it produced
  the single most expensive debugging campaign of the project.
- `firmware/POWERFEATHER_NOTES.md` carries the full mechanism write-up and remains
  the reference of record; SYSTEM.md and `hardware/README.md` production-design
  rules mirror these four rules.

## References

- LOG 2026-07-03 (CASE CLOSED + housekeeping re-grade), 2026-07-02 cont. 10
  (bisect), 2026-07-05 (46 h soak)
- `docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md` (retro-analysis header)
- `firmware/POWERFEATHER_NOTES.md` ("Wire1 at >100 kHz" section)
- ADR 0005 (task architecture, constrained), 0018 (IS31, generalized), 0012/0024
  (custom-board implications)
