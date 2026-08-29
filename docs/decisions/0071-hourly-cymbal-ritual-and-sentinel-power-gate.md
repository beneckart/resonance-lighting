# 0071 -- Hourly daytime cymbal ritual; persistent sentinel requires power evidence

**Date:** 2026-08-29

**Status:** Accepted in source; native/embedded and named hardware validation
pending. Persistent sentinel behavior is explicitly not accepted yet.

**Owner:** Ben

**Supersedes:** ADR 0030 only where it names bamboo as the installed strike
surface, and ADR 0060 only where DAY_ACTIVE implied an all-day radio lease.
Extends ADR 0031/0049 time authority, ADR 0051 load safety, ADR 0064 short-wake
telemetry, and ADR 0065 autonomous strike policy.

## Context

All 72 canopy/downlight mallets now strike small installed finger cymbals, not
the bamboo. A 72-fixture synchronized chime is loud, powerful, and iconic. A
roll call or organic spread is spatially more interesting, and the hardware
does not impose one bad artistic ordering.

Continuous radio reception is not an acceptable daytime default. Measured dark
awake draw was roughly 126-144 mA before adding useful light or sensor work;
that consumes much of the panel harvest merely to remain reachable. Deep sleep
is sub-mA and should own most of the day.

Shared GPS/RTC-derived UTC already lets fixtures make rootless deterministic
choices. The fleet therefore does not need an awake bridge or new mesh packet
to perform one bounded hourly ritual. The perimeter VL53L5CX also detects a
very close palm in sunlight, but its continuous radio-off energy cost has not
been measured. A popular interactive sculpture must not be able to hold the
fleet awake indefinitely.

## Decision

### Installed instrument

1. The installed canopy noisemaker is a solenoid mallet striking a finger
   cymbal mounted on each of the 72 bamboo/downlight assemblies. Bamboo remains
   the structural and acoustic setting, not the current impact surface.
2. ADR 0030's speaker rejection, bounded solenoid pulse, capacitor-backed
   strike path, and mechanism safety remain valid.

### Hourly ritual

3. A field-profile, sensor-verified downlight may participate once per UTC
   hour during scheduled day. UTC hour boundaries are also local hour
   boundaries at the event because the site offset is a whole number of hours.
4. Version 1 has a fixed 67-second receiver window, from T-20 seconds through
   T+47 seconds, and three bounded acts:
   - T+5.0 seconds: one fleet unison attempt;
   - T+12.0 through T+35.5 seconds: one deterministic 500 ms hash slot per
     fixture, producing an organic roll without registration;
   - T+42.0 through T+45.5 seconds: a deterministic one-quarter-fleet sparse
     after-ring.
5. The unison requires reported UTC uncertainty <=500 ms. Time uncertainty
   through 3,000 ms may still produce the intentionally organic acts. Invalid
   or weaker time causes abstention, never a late or smeared imitation of
   synchronization. An event more than 350 ms late is dropped.
6. Each event is marked attempted in RTC-retained memory before the actuator
   call. A mechanism refusal or reset cannot retry the same hour/event and
   become a strike/brownout loop.
7. Operator/program leases veto the autonomous ritual immediately. Scheduled
   night, non-downlight classes, non-FULL power, insufficient measured solar,
   disabled mechanism, maintenance, rest time, D7 collision, load-marker
   failure, and every existing hard mechanism gate remain vetoes.
8. The deterministic hash is a deployment-independent starting point. A later
   registered ring/azimuth map may replace only the artistic ordering; it must
   not weaken the time, energy, authority, deadline, or actuator gates.

### Energy readiness is not radio wakefulness

9. DAY_ACTIVE means measured energy permission, not permission to receive all
   day. A field fixture in DAY_ACTIVE resumes the ordinary bounded wake/listen
   duty cycle after the boot/control grace.
10. A confirmed DAY_ACTIVE fixture may carry readiness across one timer sleep.
    The carry is consumed on wake and re-armed only if the fresh wake still
    satisfies the real strike gate. Vanished solar therefore cannot leave a
    stale permission behind.
11. With valid time, an energy-ready downlight shortens only the final ordinary
    sleep needed to wake at T-20 seconds. Fresh time heard during that wake
    holds the receiver through the hard ritual window. Invalid time, weak
    energy, or an operator lease returns to the ordinary sleep cadence.

### Perimeter sentinel gate

12. Persistent radio-off perimeter sensing is not a production behavior until
    a named exact-target canary measures it. Source provides a test-only `-t`
    image compiled to one six-digit perimeter MAC.
13. The canary automatically records a one-second A/B/A campaign in local
    PSRAM:
    - 10 minutes radio off + sensor rail off (baseline A);
    - 30 seconds sensor warm-up;
    - 10 minutes radio off + perimeter MSA311/VL53L5CX active at the current
      production cadence;
    - 10 minutes radio off + sensor rail off (baseline B).
14. ESP-NOW remains off throughout all three measurement phases. Maintenance
    WiFi starts only after the campaign to drain the immutable history. The
    trace records corrected/raw battery current, battery/supply voltage and
    current, charger state, radio/rail truth, VL53 frame progress, near zones,
    closest range, and palm-cover edges.
15. The campaign follows ADR 0040: physically confirm one target, record its
    exact prior artifact/SHA and state, declare one writer, use one immutable
    target-locked `-t` artifact, exclusive-create the trace, restore the exact
    prior fleet binary, and prove fresh rejoin plus pending-verify survival.
16. A production sentinel needs measured energy acceptance against the 6 Ah
    perimeter budget, reliable bright-sun palm detection, bounded activation,
    a non-extendable session deadline, cooldown, and a path back to the normal
    cadence. Popularity may increase a session's artistic density but may not
    extend its deadline.

## Consequences

- The iconic full-fleet hit and a spatially organic roll coexist in one short
  ritual without a bridge or registration dependency.
- A time-quality failure is audible as a reduced/absent ritual rather than bad
  synchronization.
- Strong solar can authorize an actuation without paying continuous radio
  receive current for the rest of the hour.
- The first ritual after cold boot may be skipped while energy and UTC are
  established. This is preferable to inventing permission.
- The sentinel canary temporarily takes one perimeter fixture out of mesh for
  about 31 minutes. It is not a fleet artifact and does not implement visitor-
  triggered wake propagation.

## Validation required

1. Pass the complete native suite and one guarded ESP32-S3 field compile.
2. On one named cymbal downlight with a production battery, verify wake
   alignment, high-quality unison, deterministic roll, optional after-ring,
   fixed end, and no duplicate after a deliberate reset inside the hour.
3. Repeat with RTC-grade >500 ms uncertainty and prove the unison abstains while
   the organic roll remains bounded. Repeat with invalid time, weak solar,
   non-FULL tier, a bridge lease, scheduled night, and disarmed solenoid.
4. Confirm non-downlight fixtures never request the ritual and all ordinary
   daytime classes return to sleep from energy-ready state.
5. Run the exact-target sentinel campaign first battery-isolated or panel-
   shaded for clean load delta, then in full sun for net-energy and bright-sun
   detection. Exercise at least 20 deliberate palm approaches during the ToF
   phase and record misses/latency separately.
6. Reject sentinel promotion if the radio appears in any measurement sample,
   VL53 frames do not advance, resets/faults occur, palm reliability is not
   acceptable, or the measured daily cost does not leave the agreed night-show
   reserve.
7. Restore and verify the exact pre-test fleet artifact before the canary
   returns to installation service.

## References

- `firmware/fixture/src/core/daytime_ritual.*`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/src/core/sentinel_trace.*`
- `firmware/fixture/src/esp32/sentinel_trace.*`
- `docs/howto/PERIMETER_SENTINEL_POWER_TRACE.md`
- ADR 0030, ADR 0031, ADR 0040, ADR 0049, ADR 0051, ADR 0060, ADR 0064,
  ADR 0065
