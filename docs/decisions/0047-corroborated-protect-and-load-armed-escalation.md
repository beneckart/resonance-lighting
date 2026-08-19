# 0047 - Corroborated PROTECT persistence and load-armed reset escalation

**Date:** 2026-08-18
**Status:** Adopted (bench ordering-matrix qualification still owed; see TODO)
**Owners:** Ben + Claude
**Amends:** ADR 0023/0046 (PROTECT persistence mechanics), boot-guard Phase-4
matrix (escalation precondition). The threshold values themselves are ADR 0046.

## Context

The August bring-up surfaced three ways a fixture could latch durable PROTECT
without a genuinely collapsing battery, each costing a bench visit:

1. **Floating-BAT window.** `batteryPresent()` accepted any 2.5-4.4 V reading.
   A cell-less BAT node held up by the powered charger can float in the
   2.5-3.05 V band (widened by ADR 0046's raised protect), producing a "valid"
   sample at ~0 mA that tripped the immediate protect floor and wrote the
   durable latch on a bare board.
2. **Power-ordering escalation.** POWERON counts as an unexpected reset, so
   two benign power interruptions (panel connected before battery, bench
   supply sequencing) from a stored FULL stage walked FULL -> DIM -> PROTECT
   with no load ever energized (`9F26F8`: 31 poweron resets in ~19 minutes).
3. **Recovery-lane self-latch.** The ADR 0042 low-VBAT lane sets
   `batteryPresent()` true at ~2.3 V for the charger's benefit, which made the
   ladder see a "valid" 2.3 V sample and persist PROTECT during the very
   rescue that was fixing the cell. Worse, a field-profile wake has only 8 s
   of grace while the charging guard needs >=6 s plus a 5 s retry cadence: one
   `WAITING` verdict and the fixture deep-sleeps 900 s with charging disabled,
   indefinitely.

## Decision

1. **PROTECT posture is immediate; the durable latch requires a corroborated
   battery.** The tier still drops to PROTECT (rails off, park) on any
   sub-protect sample, but the NVS stage write is deferred until at least one
   of: recent >=30 mA charge/discharge current, a passed TI SLUAB31A BQ
   presence test (requested on demand, rate-limited to one per minute, only
   with proven external power, restoring the charging-enable state on a REAL
   verdict and never on EMPTY), a recovery-lane BQ detection, or battery-only
   operation (the running system proves the cell). An uncorroborated PROTECT
   stays awake whenever a verified external supply is present -- the floating
   false positive requires a powered charger by construction, so the fixture
   is externally powered and serviceable. A fresh EMPTY verdict additionally
   vetoes `batteryPresent()` for 60 s (freezing the ladder); real current
   clears the veto instantly so a cell installed mid-session is never locked
   out.
2. **Boot-guard escalation requires the durable load-armed marker.** A new
   `load_arm` NVS key is written (on-change only) immediately before the LED
   rail or solenoid gate can energize -- an unpersistable marker refuses the
   load, mirroring the stage-persist doctrine -- and cleared by `allLoadsOff`,
   by a 60 s all-loads-quiet debounce, and at every boot after the decision is
   taken. `bootGuardDecide` escalates (FULL -> DIM retry, DIM/LEDS_OFF ->
   PROTECT) only when the marker was set; a disarmed unexpected reset
   preserves the stored stage and burns no retry. Stored PROTECT and the
   NVS-unreadable fail-safe remain authoritative regardless of the marker.
   An unreadable marker is treated as armed (conservative).
3. **An active low-VBAT recovery lane freezes the ladder.** `batt_valid`
   excludes `lowVbatRecoveryActive()`, so a rescue in progress cannot walk
   tiers or persist PROTECT; the freeze plus supply-good keeps the fixture
   awake for the duration (the recovery gate guarantees external power), and
   the BQ stays charging autonomously. Graduation restores normal sampling
   with the recovery detection already counting as corroboration, so a real
   low cell persists PROTECT correctly the moment the lane releases it.

## Consequences

- A bare board (or floating BAT node) can no longer acquire a durable PROTECT
  latch: worst case is a RAM-only park that a power cycle fully clears. The
  guarded serial `X` clear remains for latches persisted before this ADR.
- Panel-first / battery-last / bench-USB power ordering costs zero ladder
  progress. A genuine collapse loop under load escalates exactly as before,
  because the armed marker persists through the resets it causes.
- NVS wear is bounded: the marker writes on change only -- one write per
  arm-edge, one per quiet-minute disarm, at most one clear per boot. Solenoid
  strike series stay armed throughout (quiet debounce covers the gaps).
- The ~55 ms presence test briefly gates a 30 mA BAT discharge; it runs only
  when PROTECT wants to persist without corroboration, never during recovery,
  never battery-only, and at most once per minute. Wire1 stays at 100 kHz
  (ADR 0028) via the existing solar-guard register helpers.
- Escalation telemetry still reports `interrupted` on disarmed unexpected
  resets, so the dashboard keeps seeing power-ordering events without the
  fleet paying ladder progress for them.

## REVISIT

- Run the full battery/panel/USB ordering matrix (TODO) plus one deliberate
  load-collapse loop before calling the marker qualified.
- The EMPTY-verdict veto window (60 s) and current-evidence freshness (60 s)
  are first guesses; tune against bench observations of gauge settling.
- If the fleet ever runs loads on VSQT beyond the solenoid, those enable
  paths must also arm the marker.
