# 0022 - Use a mixed LED fleet by optical role

**Date:** 2026-06-17
**Status:** Accepted. Supersedes the "LED module undecided" portion of ADR 0018; ADR
0018 still stands for the IS31FL3741 rejection and direct-GPIO interface constraint.
**Owners:** Ben + Steve

## Context

ADR 0018 left two direct-GPIO LED candidates live after the PowerFeather V2 battery
bench ruled out the IS31FL3741 matrix:

- SK6812 "HEX" array: distributed source, good for close-range animation and glow.
- 4 W RGBW point source: small emitter, good for crisp gobo projection.

The 2026-06-11 inverted-lantern gobo session showed that these are not redundant
candidates. They make different kinds of light and both are useful to the piece.

## Decision

Use both LED types in the 2026 fleet, assigned by optical role:

- **HEX lanterns:** close-range / intimate fixtures, animated mandala motion, color split
  effects, soft ambient glow, and looks that read best within roughly 6 ft.
- **4 W RGBW point-source lanterns:** longer-throw crisp gobo fixtures, including
  mandala shadows that still read around 10-15 ft and color-fringe overlap effects.

Both stay on the direct-GPIO LED interface path. Do not return to the IS31FL3741 shared
I2C matrix for the PowerFeather V2 battery build.

## Consequences

- Production planning needs a **type mix and placement plan**, not a single LED winner.
  Open question: which tree heights / positions get HEX vs point-source modules.
- The hat and internal mounting should support both optical roles as long as possible.
- The BOM may split by role: the smaller Voltaic P126-class panel may fit HEX fixtures if
  the final HEX duty cycle closes; point-source RGBW fixtures may deserve the larger
  P105-class panel for margin.
- Firmware should keep one shared direct-GPIO LED abstraction with per-fixture role/config,
  current caps, all-off-before-sleep behavior, and different pattern sets per role.
- HEX 4.2 V boost remains an optimization test, not a prerequisite for this decision.

## Open follow-ups

- Decide the HEX / point-source count ratio and placement by tree height / sightline.
- Close the bottom-up power budget for each role and map panel size to role.
- Finish the HEX 4.2 V boost bench test and boosted-build current cap.
- Record keeper `led_studio` looks and gobo/filter photos for both roles.
