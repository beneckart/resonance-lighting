# 0072 -- Final-burn fast rollout and autonomous show rotation

**Date:** 2026-08-30

**Status:** Accepted for the 2026 field image; source/native and embedded build
gates pass, immutable fixture artifact and field rollout pending.

**Owner:** Ben

**Supersedes:** ADR 0071 clauses 7, 10, 11, and its pre-burn perimeter
sentinel requirement. Extends ADR 0031/0049 time authority, ADR 0040 artifact
identity, ADR 0051 load safety, ADR 0065 strike policy, ADR 0069 local
interaction, and ADR 0070 canopy presence.

## Context

The gate is open and the installed MVP is working, but the event has begun and
one empirical canary loop now costs roughly one hour under playa conditions.
Serial canaries for each non-safety artistic tweak are preventing useful work.
The canopy fixtures are also effectively inaccessible: any change that can
latch PROTECT, break sleep/recovery, or require opening a lid still demands the
strictest treatment.

The installed cymbal strikes are pleasant rather than disruptive. Perimeter
close-range gobos disappear when a very close target stops producing a valid
VL53L5CX distance. Color Virus and Epidemic look good, but a few chattering ToF
origins can repeatedly re-seed the network. Finally, a T-Deck-only show is not
an installation experience when the operator is away from the bridge.

## Decision

### Rollout boundary

1. Bundle non-safety artistic work into one immutable field artifact and allow
   a fleet rollout with rollback, without serial per-feature canaries.
2. This shortcut does not weaken ADR 0040 artifact identity, A/B pending-verify
   survival, PROTECT ownership, OTA rescue, load-armed persistence, D7 pulse/
   rest/failsafe limits, or any physical recovery gate.

### Daytime cymbals

3. Every ordinary field/downlight daytime wake is eligible for two 40 ms
   cymbal attempts: one after the wake's battery sample is trustworthy and one
   immediately before sleep. No UTC or solar-surplus proof is required.
4. FULL and DIM are eligible. OFF and PROTECT are hard vetoes. Operator leases,
   maintenance/OTA verification, non-downlight classes, a disarmed mechanism,
   rest time, D7 collision, load-marker failure, and the solenoid failsafe
   remain vetoes.
5. A durable `chance_x256` setting controls whether both ordinary-wake hits
   happen. Version 1 defaults to exact 100 percent (`255`); `64`, `32`, and `0`
   provide 25 percent, 12.5 percent, and off without another fixture flash.
6. ADR 0071's UTC-hour ritual remains, but its solar-surplus gate becomes the
   same FULL/DIM battery-safety gate. Its time-quality, deadline, retained
   attempt ledger, class, schedule, authority, and mechanism gates remain.

### Perimeter close hold

7. Only a valid distance at or inside 380 mm may enter the crisp center-pixel
   state. After that proof, a raw target with an invalid/zero distance extends
   the center state by 1.5 seconds. An invalid/empty scene can never enter it,
   and any valid farther distance immediately restores ordinary range rings.
   This addresses too-close error returns without turning every zero into a
   false visitor or adding a dense high-power frame.

### Propagation re-arm

8. Local ToF rendering remains as responsive as before. Only program/wave
   origin edges pass through the new re-arm gate.
9. Defaults permit one accepted origin every 300 seconds and require 30 seconds
   continuously clear before another. Both values are durable bridge knobs.
   A persistent visitor produces one origin; a stuck or chattering fixture
   cannot keep recoloring the fleet.
10. Every ToF-bearing class participates as an autonomous program origin:
    canopy TMF8820 and perimeter VL53L5CX fixtures may seed; sensorless uplights
    and chandeliers relay and render through the existing neighbor protocol.

### Autonomous show schedule

11. With trustworthy UTC and no bridge lease, the field fleet rotates every
    10 minutes through four deterministic acts:
    - the existing Greenberg-Hastings CA;
    - visitor-seeded Color Virus;
    - visitor-seeded Epidemic;
    - Greenberg-Hastings with K=2, zero spontaneous sparks, and ToF seeding.
12. Invalid UTC or a disabled rotation fails back to the existing CA. Bridge
    program leases remain authoritative and return to the current autonomous
    act on release/expiry.
13. The fourth act is deliberately harder to ignite: visitors can start a
    local fire, while neighboring excitations need two votes and spontaneous
    sparks cannot start it.

### Durable field tuning

14. Canonical packet type 31 (`NB_FIELD_TUNING`) carries the full setting as
    one fleet/target command. A persistent receive writes one versioned NVS
    blob, with RF-copy deduplication. The T-Deck Field screen requires explicit
    confirmation and checkpoints its action audit before the fleet send.
15. The setting includes ordinary-wake chime chance, rotation enable,
    propagation minimum interval, and continuously-clear re-arm time. Compiled
    defaults already select the accepted field posture, so a missed tuning
    packet does not make the new artifact behave differently on first boot.

### Deferred work

16. Continuous daytime perimeter interactivity, the ADR 0071 sentinel power
    campaign, jackpot/every-perimeter easter eggs, and deeper false-positive
    diagnosis are deferred until after the 2026 burn. USB serial telemetry is
    the preferred immediate diagnostic if the perimeter close hold needs field
    investigation.

## Consequences

- A visitor who stays 20 minutes sees at least two autonomous modes and usually
  three, even when no bridge operator is present.
- The propagation band-aid intentionally suppresses repeat visitor seeds for
  minutes. It favors a viable show over perfect distinction between a lingering
  visitor and a malfunctioning fixture.
- Two daytime hits per 72 downlights per roughly five-minute wake is a much
  larger strike count than the hourly ritual. It is accepted artistically, but
  remains voltage-tier- and mechanism-bounded and can be attenuated from the
  bridge without another fixture image.
- The no-serial-canary exception is explicitly deadline-driven and limited to
  non-safety art behavior. It is a post-event process-review item, not the new
  default for power, sleep, OTA, or recovery work.

## Validation

1. Full fixture native suite, including packet layout, autonomous preset
   restore, chance boundaries, close hold, and propagation re-arm.
2. Fresh T-Deck ESP32-S3 compile including the Field UI and canonical packet.
3. One fresh immutable fixture artifact with field profile, channel 11, and
   declared WiFi profile label.
4. Fleet OTA must still require fresh revision heartbeat, pending-verify
   survival, and rollback availability. No lid-opening recovery is accepted as
   a normal rollout step.

## References

- `firmware/fixture/src/core/field_behavior.*`
- `firmware/fixture/src/core/daytime_ritual.*`
- `firmware/fixture/src/core/choreo/runtime.cpp`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/src/esp32/nvs_store.*`
- `firmware/fixture/src/core/packet.h`
- `firmware/tdeck_bridge/src/ui/app_field.*`
- `firmware/tdeck_bridge/src/net/mesh_tx.cpp`

