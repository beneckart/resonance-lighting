# 0034 -- BMP581 outer hanging-ring allocation

**Date:** 2026-08-08

**Status:** Accepted -- production allocation

**Owners:** Ben

## Context

Thirty BMP581 temperature/barometric-pressure STEMMA boards were bought as
environmental loggers. Their original uplight allocation became an open
trunk-light allocation when the fixture layout changed in ADR 0032. The final
hanging-downlight layout has three physically clear rings of 24. The first five
production downlights arrived with BMP581s already installed after the MSA311 in
the TMF8820 -> MSA311 -> BMP581 chain, and all five passed live production
firmware sampling.

A scattered 24-of-72 downlight allocation would be difficult to build, identify,
and service. One complete ring is a simple physical rule and gives evenly
distributed environmental sampling around the canopy.

## Decision

1. Install BMP581s on all 24 downlights in the **outermost hanging ring**.
2. The remaining six of the 30 purchased BMP581s are production/field spares.
3. The first five commissioned BMP-equipped downlights (`F40364`, `F2B7DC`,
   `9E5A94`, `9F275C`, and `F2BE48`) are assigned to that outer ring; exact ring
   positions remain unassigned.
4. Middle- and inner-ring downlights do not require a BMP581. Trunk lights no
   longer have a planned BMP581 allocation.
5. All downlights continue to share one firmware image. Commissioning uses the
   physical build role to require BMP581 acceptance for outer-ring units and to
   omit that requirement for the other two rings.

## Consequences

- The production build rule is visible and countable: one complete 24-light ring
  has pressure/temperature sensing, and two complete rings do not.
- Six identical spares cover build damage and field replacement.
- Registry/install labels must preserve the outer-ring designation so a
  BMP-equipped unit is not consumed in an inner or middle position.
- Environmental telemetry comes from many nearby sensors. Firmware may aggregate
  or subsample them; deploying 24 does not require transmitting every reading at
  a high rate.

## Validation required

- Physically count the received BMP581 inventory before completing the ring; the
  procurement record establishes 30 bought, not a receiving audit.
- Continue the same sustained MSA311/TMF8820/BMP581 acceptance used on the first
  five for the remaining 19 outer-ring units.
