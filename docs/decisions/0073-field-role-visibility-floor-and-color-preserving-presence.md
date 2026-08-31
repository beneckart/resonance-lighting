# 0073 -- Field role visibility floor and color-preserving presence

**Date:** 2026-08-31

**Status:** Accepted for the 2026 field image; source, native fixture tests, and
fixture/T-Deck development builds pass. Immutable artifact and rollout pending.

**Owner:** Ben

**Supersedes:** ADR 0069/0070 field-render clauses and ADR 0072 clauses 7, 8,
and 10 for the 2026 perimeter posture. Extends ADR 0023 power-policy authority
and ADR 0072's deadline-driven rollout boundary.

## Context

The installed piece is viable and intentionally quieter than surrounding playa
art, but three roles need a simpler final-burn posture. Interior uplights are a
climbing-safety aid and cannot disappear during an artistically dark show
state. Perimeter interaction is not producing a dependable gobo and the small
6 Ah/P126 fixtures are the fleet's most concerning energy cohort. Canopy
presence works, but replacing every program/direct color with dedicated warm
white erases Color Virus's infection hue and the operator's LED-app choice.

The brightness path was audited before changing it. Program frame channels are
linear 8-bit values, the NeoPixel driver's global brightness is 255, and no
gamma transform exists. The installation's dimness is partly optical/3V3-rail
reality, but also comes from deliberately low autonomous frame values, the
battery DIM cap, and the 765-RGB-channel-unit perimeter current budget.

## Decision

1. In production profile during scheduled night, every uplight renders one RGB
   point. An underlying nonzero/non-suppressed mode becomes `255,255,255`; an
   underlying dark or suppressed mode becomes `128,128,128`. The physical R/G
   observation is not corrected in this deadline build because equal RGB is
   invariant to that swap.
2. In production profile during scheduled night, every perimeter renders only
   the physical center HEX pixel at RGB `255,255,255`. The other 36 pixels and
   the W channel remain zero regardless of the selected artistic program or
   direct LED stream.
3. The one-pixel perimeter frame totals exactly 765 linear RGB channel units,
   so the existing HEX current limiter passes it at full scale. It can never
   become a dense all-HEX frame under this field policy.
4. Perimeter VL53L5CX readings remain sampled and telemetried, but they do not
   modify local output and cannot originate program/presence traffic. The pure
   range renderer and gates remain available for post-burn testing.
5. An active canopy presence latch blinks the already-rendered program/direct
   color at 1 Hz (500 ms visible, 500 ms dark). It neither invents a hue nor
   converts RGB to dedicated W. A black/suppressed frame remains unable to
   awaken through interaction.
6. The T-Deck LED app adds an explicit `RGB white` swatch carrying
   `R=G=B=255,W=0`; the existing `white` swatch remains dedicated-W for canopy
   point sources.
7. The role floor sits after art/local interaction but before the LED driver's
   power cap. FULL passes the values unchanged, DIM scales them through the
   existing cap, and OFF/PROTECT cut the rail. Startup sag limiting, boot/OTA
   safety, transport wake darkness, Identify, and Smoke arbitration also remain
   authoritative.
8. "Always" means the scheduled-night field show, not daytime charging or
   shipping. This supplies the requested night/climbing visibility without
   adding a 24-hour load to the small battery cohorts.

## Consequences

- Uplights retain a visible two-level rhythm but give up program color while
  this rule is active.
- Perimeters give up ToF and multi-pixel choreography for one dependable,
  crisp, maximum-output gobo with a bounded sparse load.
- Canopy presence becomes legible inside every colored mode and direct stream.
- The tree may look materially brighter because these role values now reach
  full linear output in FULL tier. That does not remove the 3V3 rail or optical
  limits and does not bypass battery safety.
- The previous deployed artifact remains the rollback if this minimum-viability
  art direction is not acceptable in the field.

## Validation

1. Native role-policy tests pin dark/lit uplight values, a single physical
   perimeter center pixel, and FULL/DIM current-budget composition.
2. Native interaction tests pin same-color canopy frames on the visible half
   and an all-zero frame on the dark half.
3. Full fixture native suite and ESP32-S3 development compile.
4. Focused T-Deck direct-frame native test and ESP32-S3 development compile.
   The full T-Deck native wrapper remains blocked by the existing callsign
   omissions for `9F268C` and `F402A8`; no identity was invented for this work.
5. Before any fleet rollout, create one immutable ADR 0040 field artifact from
   clean source and retain fresh exact-revision/pending-verify evidence.

## References

- `firmware/fixture/src/core/field_role_policy.*`
- `firmware/fixture/src/core/interaction_modulator.*`
- `firmware/fixture/src/core/frame_budget.*`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/src/esp32/led_driver.cpp`
- `firmware/tdeck_bridge/src/ui/app_zones.cpp`
