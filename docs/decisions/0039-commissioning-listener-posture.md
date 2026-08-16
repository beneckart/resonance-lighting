# 0039 -- Commissioning listener posture

**Date:** 2026-08-15

**Status:** Accepted; source integration and named-artifact qualification pending

**Owners:** Ben + Elliot

**Supersedes:** ADR 0038 decision 2 and its no-command-dark consequence only.
ADR 0038's control-plane liveness, safety, PROTECT recovery, and deliberate
field-promotion decisions remain in force.

## Context

ADR 0038 made commission mode deliberately dark without a bridge lease. That
removed autonomous ambiguity, but a dark lantern is also an ambiguous physical
object during assembly: it may be healthy and listening, power-vetoed, failed,
or simply not receiving commands.

Elliot's `Lighting-Controller` branch added a quiet listener posture on nine
battery-backed lanterns: low red while ready, a subdued MAC-derived identity
pulse, and a local ToF response that changes to the fixture's signature color
for a nearby confident target. Bridge commands continue to override local idle
behavior. Ben accepts the low-red and basic ToF behavior as useful build-week
feedback: the light visibly says that it is alive and listening.

## Decision

1. The normal **commission listener** posture stays awake and bridge-authoritative,
   but no-command fallback is a low-red ready beacon rather than rail-off dark.
2. A confident, fresh ToF target inside the commissioned threshold may show the
   fixture's signature color while present. This is a local diagnostic interaction,
   not autonomous CA or a production show.
3. A low-duty MAC-derived identity pulse is allowed if it remains visually calm.
   It is not the USB boot salute and must use a distinguishable pattern.
4. Any active bridge/direct lease overrides the listener and presence frames.
   Command expiry returns to listener posture.
5. Local power tiers, boot guard, OTA guard, and actuator safety retain final
   authority. OFF/PROTECT still cut the LED rail even in listener mode.
6. Strict rail-off commission-dark remains available as a diagnostic posture for
   power and rail-cycle tests, but is not the default assembly experience.
7. Listener, strict commission, and field become explicit runtime settings in
   one normal fleet image. While listener is still a compile flag, its artifact
   is canary-only and may not be promoted as the final one-image fleet build.
   Materially different temporary builds may not share a firmware revision.
8. ToF presence must have freshness/debounce semantics. A stale last distance is
   not continuing presence.

## Consequences

- A builder can distinguish a healthy listening fixture from a dark/unpowered
  one without first driving a scene.
- Commission mode consumes more energy than ADR 0038's dark fallback because the
  LED rail and ready pixel remain active. It is still a temporary build-week and
  service posture, not unattended field behavior.
- The listener deliberately relaxes "no command means electrically dark" while
  retaining "no command means no autonomous show."
- Rail-off/rail-on regression testing must explicitly select strict commission
  or another test that physically cycles the rail; a continuously lit listener
  can mask that path.
- The accepted behavior requires a new uniquely identified artifact. The reused
  `fixture-2026-08-15.4` string is not an acceptable release identity.

## Validation required

1. Measure listener idle current and confirm it is acceptable for build-week use.
2. Prove bridge commands override immediately and expiry returns to low red.
3. Prove a close ToF target enters the signature color and a departed/no-target
   sample clears it without latching stale range data.
4. Exercise OFF and PROTECT and verify the physical rail still cuts.
5. Exercise maintenance enter/resume on a non-updated fixture and verify the LED
   returns after the same-boot rail cycle.
6. Keep a strict commission artifact/runtime setting available for the explicit
   `rail on -> rail off -> rail on` hardware regression.
