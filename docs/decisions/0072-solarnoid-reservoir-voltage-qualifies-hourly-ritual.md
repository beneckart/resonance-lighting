# 0072 -- Solarnoid reservoir voltage qualifies the hourly ritual

**Date:** 2026-08-29

**Status:** Accepted in source; exact-target hardware canary pending. Fleet
promotion is not authorized by this decision alone.

**Owner:** Ben

**Supersedes:** ADR 0060 and ADR 0071 only where they require instantaneous
150 mA supply current for autonomous solarnoid permission.

## Context

The first field observation of ADR 0071's production schedule sampled two
sunlit downlights shortly after the 12:00 local ritual. Hawkeye `9F2664` and
Navi `9F0E7C` both reported a valid roughly 6.65 V VDC input and FULL power
tier, but 0 mA supply current. Their ritual audit ledgers remained entirely
zero: they never entered the window or attempted a mechanism action.

Fresh Hawkeye charger telemetry resolved the ambiguity. The BQ25628E was
enabled, not in high-impedance mode, fault-free, and in charge-done state. A
fleet snapshot found the same valid-input/zero-current/done condition on many
downlights. Later Navi data moved back into high charge current without a
configuration change. This is ordinary panel/charger variation, including
charge taper and termination, not evidence that the solarnoid rail is absent.

The old policy tested BQ input current because `DAY_ACTIVE` originally meant
"enough surplus to stay awake." The installed noisemaker has a different
immediate energy source: its VDC-fed storage capacitor. At charge taper, the
capacitor can remain charged while the charger accepts no battery current.
Requiring 150 mA at the actuation instant therefore rejects the most
energy-rich part of the day.

## Decision

1. A field fixture may enter solar-energy-ready `DAY_ACTIVE` after the existing
   60-second confirmation when all ordinary schedule and battery conditions
   pass and either:
   - qualified supply current is at least 150 mA; or
   - `supply_good` is true and the measured VDC input/reservoir is at least
     5.8 V.
2. An active fixture remains energy-ready while qualified current is at least
   100 mA or qualified VDC remains at least 5.4 V. The existing 300-second exit
   confirmation is unchanged.
3. An autonomous strike requires fresh entry-grade evidence: FULL tier,
   `supply_good`, and either at least 150 mA or at least 5.8 V VDC. The lower
   remain-active thresholds do not themselves authorize an actuation.
4. Battery voltage or gauge SOC alone never grants readiness. Scheduled day,
   valid time, operator-lease veto, exact canary target/hour, power policy, D7
   collision protection, durable load marker, pulse/rest bounds, and all other
   mechanism gates remain unchanged.
5. The 5.8 V entry floor matches the previously qualified P126 operating
   region and is comfortably below the roughly 6.65 V charged-rail field
   observation. The 5.4 V exit floor adds hysteresis. These values require
   exact-target field validation before fleet promotion.

## Consequences

- A charged solarnoid reservoir can authorize the hourly ritual even when the
  battery charger has tapered or terminated at 0 mA.
- Strong live harvest continues to qualify when BQ input voltage is pulled
  down near VINDPM, so a voltage-only rule does not reject the opposite end of
  the operating range.
- A high LFP with absent/invalid VDC remains silent. A weak high-voltage panel
  must hold the reservoir threshold continuously for one minute before earning
  readiness, and every act rechecks entry-grade evidence.
- This is deliberately a canary correction. The 84 fixtures on
  `fx-260829-af1d4ec-p` keep the earlier safe-abstaining gate until a named
  target proves wake alignment, audit masks, sound, and clean return to sleep.

## Validation required

1. Build one immutable field variant locked to Hawkeye `9F2664` and one future
   UTC hour. Retain the exact prior production binary and SHA-256.
2. Before the window, prove FULL tier, valid scheduled day/time, valid VDC, no
   charger fault/recovery, correct downlight class/sensors, and armed solenoid.
3. Require window seen/complete, expected = attempted = fired, no policy or
   mechanism refusal, no duplicate, and ordinary sleep after T+47 seconds.
4. Record VDC/current around each act. A zero-current success is the positive
   regression; falling below both entry paths must still abstain.
5. Restore the exact prior production image and pass fresh-rejoin plus pending-
   verify survival. Only then consider a new production artifact.

## References

- `firmware/fixture/src/core/lifecycle.*`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `docs/howto/DAYTIME_RITUAL_CANARY.md`
- `ops/bench/data/Black Rock City/20260829-hawkeye-9F2664-hourly-ritual-1B9E29F8-observe-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-navi-9F0E7C-hourly-ritual-7F584EC1-observe-job.jsonl`
- ADR 0030, ADR 0060, ADR 0065, ADR 0071
