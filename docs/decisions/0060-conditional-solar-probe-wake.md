# 0060 -- Conditional solar-probe wake for daytime active mode

**Date:** 2026-08-26

**Status:** Accepted in source; native-tested and field-profile compiled, hardware validation pending

**Owner:** Ben

**Extends:** ADR 0030, ADR 0049, ADR 0053, ADR 0055

## Context

Field-profile fixtures use a 300-second deep sleep followed by a 15-second
listen window during `DAY_CHARGE`. Entering `DAY_ACTIVE` required 60 continuous
seconds of surplus evidence. Lifecycle state and its confirmation timer are
ordinary RAM, so every deep-sleep wake reset the timer. A fixture already in
the daytime cadence therefore slept at 15 seconds and could never reach the
60-second transition, even with strong panel input.

Simply increasing every wake to more than 60 seconds would multiply daytime
radio draw across the fleet. Carrying an apparent continuous timer through deep
sleep would claim evidence for periods when the fixture was not measuring the
input. Treating a high resting battery voltage as renewable surplus could also
keep a fixture awake while it was only discharging.

Daytime percussion needs a reachable `DAY_ACTIVE` state, but only when actual
input power and the ordinary battery/actuator gates support it.

## Decision

1. Keep the normal field cadence at 300 seconds asleep and 15 seconds awake.
2. During `DAY_CHARGE`, measured good input at or above 150 mA while the power
   tier is FULL starts a solar probe. While that evidence remains continuous,
   the lifecycle suppresses its ordinary 15-second sleep decision.
3. After 60 continuous seconds at the entry threshold, the fixture enters
   `DAY_ACTIVE` and remains awake. If the signal drops before confirmation, the
   probe timer resets and the fixture sleeps as soon as the ordinary grace has
   expired.
4. Battery voltage alone no longer enters or sustains `DAY_ACTIVE`. It remains
   part of the independent power-tier safety policy.
5. Once active, 100 mA is the remain-active threshold. Input below that level
   for 300 continuous seconds returns the fixture to `DAY_CHARGE`. The existing
   instantaneous strike gate still requires FULL tier, good input, and at least
   150 mA, so the 100-149 mA hysteresis band is awake but cannot strike.
6. Night/day transitions clear probe and fade timers. A live commission-to-field
   profile change reinitializes the lifecycle in conservative `DAY_CHARGE`
   instead of leaving the fixture stranded in `LIFE_COMMISSION` until reboot.
7. The probe uses RAM only. It adds no NVS writes, RTC-retained claim, packet
   field, or protocol change. Serial reports probe start, confirmation, or end.
8. Commission behavior and the independent PROTECT wake/sleep policy are
   unchanged. PROTECT may still park the fixture before lifecycle processing.

`supplyGood` does not distinguish panel from an authorized bench/USB source.
That is acceptable: in the installed field posture it represents panel input,
while on the bench a stable external source may deliberately provide the same
surplus condition.

## Consequences

- A sleeping daytime fixture can autonomously become reachable and knock-ready
  after strong input persists for one minute.
- Weak, absent, or transient input retains the low-energy 300/15 cadence.
- A charged battery without input can no longer keep the control plane awake.
- The 150 mA entry threshold is the existing strike-surplus threshold; the
  100 mA exit threshold adds hysteresis without weakening strike permission.
- Solar input near the threshold may produce occasional bounded probes. The
  field test must measure this behavior rather than assuming the current
  readings are noise-free.

## Validation required

1. On one named field-profile fixture with its production battery, begin from a
   true timer wake in scheduled day.
2. With input below 150 mA, verify about 15 seconds awake followed by the normal
   300-second sleep.
3. With stable input at or above 150 mA and FULL tier, verify the probe remains
   awake past 15 seconds, confirms after 60 seconds, and reports
   `LIFE_DAY_ACTIVE` without an operator command.
4. Drop input during confirmation and verify the candidate clears and sleeps.
5. From active, verify 100-149 mA remains awake but cannot strike, then verify
   less than 100 mA for 300 seconds returns to `DAY_CHARGE`.
6. Recheck commission no-sleep and PROTECT timing on the same source revision.
7. Measure awake/probe current and inspect cloudy input around both thresholds
   before promoting an immutable fleet artifact.

## References

- `firmware/fixture/src/core/lifecycle.h`
- `firmware/fixture/src/core/lifecycle.cpp`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/tests/test_behavior.cpp`
- `docs/decisions/0049-utc-consensus-civil-twilight-and-operator-overrides.md`
