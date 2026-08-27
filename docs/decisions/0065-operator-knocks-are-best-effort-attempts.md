# 0065 -- Operator knocks are best-effort mechanism attempts

**Date:** 2026-08-27

**Status:** Accepted in source; native and embedded builds pass, hardware
validation and artifact promotion pending

**Owner:** Ben

**Supersedes:** the deliberate radio/operator strike gating portions of ADR
0030, ADR 0038, ADR 0041 (universal solenoid defaults), and ADR 0060. Autonomous
program-generated knocks retain ADR 0060's energy gate.

## Context

A field Knocker roll reported 92 live fixtures but produced sparse audible
impacts. The T-Deck was already sending one targeted request to every fresh
peer. Receivers then rejected deliberate operator requests unless lifecycle
was DAY_ACTIVE, solar input was good and at least 150 mA, and the battery tier
was FULL. That policy treated telemetry-derived energy readiness as actuator
authority even though the solarnoid's capacitor is the immediate strike-energy
store and has no direct state-of-charge measurement.

The desired operator semantic is simpler: request a pulse from every receiver
that hears the command. A discharged or absent capacitor may produce a weak or
inaudible result; that is acceptable and is not itself a reason for firmware to
refuse the attempt. A fixture without physical mallet hardware has no connected
load and remains electrically inert.

This is different from autonomous CA. An unattended program may create many
repeated strike requests and therefore still needs the renewable-energy policy.

## Decision

1. A received exact-target `NB_TARGET_SOLENOID` command or deduplicated
   `NB_EVENT_SOLENOID_STRIKE` event is a deliberate operator/control-plane
   request. It attempts `solenoidStrike()` regardless of lifecycle, solar
   current, supply-good state, or power tier.
2. This does not wake a sleeping fixture or make a missed packet actionable.
   Targeted roll still addresses the fresh snapshot; broadcast modes reach only
   updated fixtures that are listening when the event is sent.
3. Operator bypass ends at the mechanism boundary. The persisted solenoid arm,
   minimum rest interval, pulse clamp, D7 collision check, durable load-armed
   marker, one-shot timer, and loop failsafe remain authoritative. Maintenance
   mode does not process mesh strike traffic. Scheduled events retain dedupe,
   one-pending-event, and late-drop rules.
4. Autonomous program output, including native CA/choreography strike edges,
   still requires `behaviorStrikePermitted()`: the existing field DAY_ACTIVE,
   renewable surplus, and power-tier policy remains its energy budget.
5. Local deliberate USER/SW1 behavior remains as before; it already reached the
   hard mechanism gate without lifecycle/solar qualification.
6. UI and agent wording must say `attempted`, not promise a mechanical impact.
   It must distinguish the retained hard mechanism gates from removed energy
   qualification.

## Consequences

- A deliberate operator can request a weak, night, or low-tier strike. It may
  be inaudible and consumes whatever energy the connected hardware can supply.
- A low-energy attempt may reset a fixture. ADR 0051's pre-load durable marker,
  reset escalation, and PROTECT policy remain the recovery boundary; this ADR
  deliberately does not treat those power outcomes as pre-command refusal.
- Fleet `live` remains radio freshness, not physical hardware inventory or a
  guarantee of sound. A 92-target 80 ms roll still spans about 7.36 seconds and
  can miss peers that sleep before their sorted turn. Broadcast-now removes
  that stagger for updated awake fixtures but is a different artistic action.
- Bare and small-hat fixtures may receive the same control packet and do
  nothing physically. This preserves one universal firmware image.

## Validation required

1. On one explicitly named, armed solarnoid, prove an exact-target operator
   request reaches the mechanism in DAY_CHARGE below 150 mA and in a non-FULL
   tier while the hard pulse/rest/failsafe behavior remains bounded.
2. Repeat with the capacitor deliberately discharged and confirm weak/no motion
   is accepted without an unbounded reset loop.
3. Prove disarm, rest interval, already-high D7, maintenance, failed load-marker
   persistence, and scheduled-event late-drop still refuse safely.
4. Prove native CA remains energy-gated while direct Knocker targeted,
   broadcast-now, and sync-+1.0 s use the operator policy.
5. Use explicit named canaries before any fleet physical-strike validation.

## References

- `firmware/fixture/src/core/strike_policy.h`
- `firmware/fixture/src/esp32/net_peer.cpp`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/src/esp32/solenoid.cpp`
- `firmware/tdeck_bridge/src/ui/app_knocker.cpp`
- ADR 0030, ADR 0038, ADR 0041 (universal solenoid defaults), ADR 0051,
  ADR 0060
