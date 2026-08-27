# 0064 -- Trustworthy short-wake power telemetry

**Date:** 2026-08-27

**Status:** Accepted in source; native and embedded builds pass, fixture canary pending

**Owner:** Ben

**Extends:** ADR 0023, ADR 0037, ADR 0060, ADR 0062

**Supersedes:** the effective 3-second awake portion of the 120 s / 3 s field
cadence; the timer-sleep interval remains 120 seconds

## Context

The new Fleet current column exposed a fleet-wide cluster of `0`, `+1`, and
`-3` mA readings on otherwise healthy daytime fixtures. Fixtures held awake by
an operator command soon reported substantial positive charge current. This
was not evidence that every resting fixture happened to be at electrical
balance.

The full boot heartbeat is sent before lifecycle initialization, before the
6-second battery/charger guard enables charging, and before a hibernating
MAX17260 is guaranteed to refresh its Current register. In hibernate the gauge
can take 5.625 seconds between updates. A successful I2C register read proves
communication, not sample freshness.

The deployed field recipe compounded the problem: a 3-second listen grace let
a timer-woken fixture return to deep sleep before the 6-second charge-enable
guard. Because boot initialization explicitly disables charging for bare-board
safety, a fixture could repeatedly wake, disable charging, publish a cached
zero, and sleep without re-enabling its solar charge path.

At the same time, the operator UI mixed two unrelated meanings of dark:
electrical blackout with the radio awake, and the repeated day-baseline command
used to catch sleeping fixtures. OTA also had no integrated check for a
persisted commission profile, which keeps a fixture awake and materially
increases drain.

## Decision

1. Append `power_sample_flags` as tail 17 of the canonical `NbHeartbeat` wire
   structure. Its append-only bits independently certify IBAT, VBAT, SOC, and
   charger fields. Old firmware has no flags, so receivers display its IBAT as
   unverified instead of interpreting a number.
2. IBAT becomes valid only after 12 seconds from boot. Charging is held off
   until the 6-second battery guard; 12 seconds is beyond one worst-case 5.625
   second MAX17260 hibernate conversion after that transition.
3. A field fixture may not take ordinary day deep sleep until IBAT is valid.
   If the gauge is absent or faulted, the hold fails open at 15 seconds so a
   telemetry fault cannot strand the fixture awake.
4. When the validity flags change, the fixture immediately sends one full
   heartbeat before sleeping. The normal 60-second rich-heartbeat cadence does
   not have to elapse.
5. A live program lease also blocks ordinary day sleep for the full lease.
   This prevents a long blackout lease from being lost when a timer sleep
   resets its RAM state.
6. The T-Deck latches BQ state across short heartbeats and derives operator
   charge phases from BQ25628E `CHG_STAT`: `CHARGING_CC`, `CHARGING_CV`,
   `TOP_OFF`, and `NOT_CHARGING/DONE`, plus explicit disabled, fault, unknown,
   and off-air states. It never infers charge phase from a near-zero IBAT.
7. Fleet shows signed IBAT only when its validity bit is present. Health can
   switch its fixed swatches between raw VBAT bands and charger phase. Detail
   keeps VBAT, validated IBAT, input power, advisory SOC, and human-readable
   class/lifecycle/tier/program/charge names.
8. Operator wording is separated:
   - `Blackout` means LED rail off with the radio awake and releasable.
   - `Deep sleep` means radio off until timer wake.
   - `Wake Fleet` repeatedly applies the dark day baseline for six minutes to
     catch timer wakes, after which each captured fixture has the normal
     ten-minute control hold for follow-up commands.
9. The fleet OTA host always audits the verified cohort's reported runtime
   profile. With explicit `--fix-commission-profile`, each verified commission
   fixture receives one exact-target persisted field command and must return a
   fresh expected-revision `profile=field` confirmation. There is no broadcast
   profile mutation.

## Consequences

- A timer wake now lasts about 12 seconds when the gauge is healthy, even if an
  older artifact recipe says 3 seconds. This costs more awake energy than the
  attempted 3-second cadence but restores charging, produces useful solar
  evidence, and remains shorter than the prior 15-second production default.
- Charging remains enabled during the following deep-sleep interval after the
  battery guard accepts the installed cell.
- Old fixture firmware remains visible, but its current is shown as `-` or
  unverified until it is upgraded. New full heartbeats are 193 bytes; short
  heartbeats and all command packets are unchanged.
- `NOT_CHARGING/DONE` is intentionally not called full. It can also mean no
  usable input or charger termination; VBAT, IBAT, and input telemetry provide
  the surrounding evidence.
- A commission-profile correction is a separate, logged NVS write after OTA
  verification. Artifact defaults still do not silently overwrite operator
  configuration.

## Validation required

1. OTA one named, battery-backed field canary with a clean immutable artifact.
2. Observe a true daytime timer wake: early full heartbeat has unverified IBAT,
   charging enables after the guard, a fresh corrected full heartbeat arrives
   at about 12 seconds, and then the fixture sleeps for the configured 120 s.
3. In sun, confirm BQ charge phase plus positive validated IBAT. Remove input
   and confirm a negative validated IBAT on a later wake.
4. Repeat with a full/terminating battery so `NOT_CHARGING/DONE` is distinguishable
   from unknown and from exact electrical balance.
5. On a no-battery or fault-injected canary, confirm the 15-second fail-open
   prevents a permanent awake drain.
6. Run Wake Fleet and Blackout separately; confirm the former catches timer
   wakes and the latter keeps already captured radios awake for its lease.
7. Include one deliberately persisted commission canary in an exact-target OTA
   job with `--fix-commission-profile`; preserve ledger evidence of detection,
   mutation, and fresh field-profile confirmation.

## References

- `firmware/fixture/src/core/packet.h`
- `firmware/fixture/src/esp32/board_power.cpp`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/tdeck_bridge/src/core/health_model.cpp`
- `ops/bench/fleet_dashboard_ota.py`
- Analog Devices MAX17260 data sheet:
  `https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17260.pdf`
- TI BQ25628E data sheet:
  `https://www.ti.com/lit/ds/symlink/bq25628e.pdf`
