# 0041 -- Fixture class from STEMMA sensor signature

**Date:** 2026-08-16

**Status:** Accepted

**Owner:** Ben

## Context

The fleet dashboard needs a trustworthy fixture class to distinguish canopy,
perimeter, trunk/uplight, and chandelier devices. The assembled fixture classes
already have distinct STEMMA sensor complements, but the first implementation of
the automatic class probe incorrectly used BMP581 as the uplight discriminator.
ADR 0034 assigns BMP581 to the outer 24 canopy/downlight fixtures, so that rule
would misclassify known downlights.

A missing sensor also cannot be treated as an intentional hardware conversion.
For example, an assembled canopy fixture still has its MSA311 if its TMF8820-mini
fails or becomes disconnected. That MSA311-only result is physically identical to
the intended trunk/uplight signature unless the fixture remembers its prior class.

## Decision

Automatic fixture identity uses this ordered STEMMA signature:

1. An ID-verified TMF8820/TMF8821-family sensor means canopy/downlight.
2. Otherwise, a VL53L5CX means perimeter.
3. Otherwise, an MSA311 means trunk/uplight.
4. Otherwise, no sensors means chandelier.

TMF takes precedence over VL53 if both answer, but the impossible combination is
reported as a mismatch. BMP581 is environmental telemetry and never determines
class. A lone BMP581 is also a mismatch because a production chandelier has no
sensors at all.

The persisted `class_last` value is a sensor-death guard:

- If a previous downlight or perimeter re-probes as MSA311-only, retain the
  previous class and report a mismatch.
- If any previously sensored class re-probes with no class sensor, retain the
  previous class and report a mismatch.
- A new MSA311-only device, or one already remembered as uplight, is an uplight.
- A new no-sensor device, or one already remembered as chandelier, is a
  chandelier.
- A newly detected strong ToF discriminator is accepted and learned so deliberate
  hardware repurposing remains possible.

An explicit `class_ovr` still wins, but disagreement with the observed signature
is reported and the override does not rewrite `class_last`.

## Consequences

- Dashboard glyphs can represent physical fixture roles once the deployed
  firmware publishes its class tail: circle for canopy/downlight, hexagon for
  perimeter, triangle for trunk/uplight, and diamond for chandelier.
- BMP-equipped outer-ring downlights remain downlights, consistent with ADR 0034.
- A failed or disconnected ToF sensor raises a diagnostic instead of silently
  changing the LED electrical profile.
- The currently powered fleet contains no expected chandelier fixtures; the
  chandelier is unpowered. A live no-sensor classification therefore deserves
  physical verification even though it is a valid production signature.
- Existing deployed fixture artifacts that omit class telemetry continue to show
  as unknown until a separately named, validated artifact is intentionally
  deployed. This decision does not authorize an OTA operation.
