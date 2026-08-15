# 0032 -- Nevada City production fleet: 130 fixtures

**Date:** 2026-08-06
**Status:** Accepted. Supersedes only the fleet-count and role-allocation snapshot in
ADR 0024; its COTS PowerFeather V2 architecture decision remains accepted.
**Owners:** Ben

## Context

The July plan in ADR 0024 deliberately kept placement flexible at 150-152 fixtures.
Physical assembly in Nevada City has now converged far enough to replace that loose
allocation with the expected production installation. The purchased inventory remains
larger than the deployed fleet, so reducing the target improves spares and does not
reverse any procurement decision.

## Decision

Plan a 130-fixture production deployment:

| Physical role | Count | LED direction |
|---|---:|---|
| Hanging downlights | 72 | 4 W RGBW point source + gobo; three rings of 24 |
| Perimeter | 24 | All SK6812 HEX |
| Chandelier | 18 | Mixed HEX/RGBW |
| Trunk/uplight | 16 | Moving toward all RGBW; smaller-die 3 W RGB + lens remains an optical test |

The total is `72 + 24 + 18 + 16 = 130`. These are the current production counts,
barring an unforeseen installation issue that forces fewer lights.

`Trunk/uplight` is the physical-role name. Keep the existing wire/NVS
`FixtureClass::UPLIGHT` value stable; renaming a deployed enum is not justified by a
placement-label change. Host tools may accept `trunk` as an alias for that class.

## Consequences

- The canonical living table in `docs/block-diagram/SYSTEM.md` and the mirrored
  `ops/bom.md` counts move to 130.
- The 158 purchased production PowerFeathers leave 28 boards beyond the deployment
  target, before separately identified bench stock. Other bought component pools also
  gain margin; procurement quantities remain historical facts and do not change.
- The small-enclosure allocation is now 24 perimeter + 16 trunk = 40, comfortably
  below the 60-unit deployment cap.
- Network airtime and collision projections should be re-run at 130 nodes. The prior
  100-node measurement/extrapolation remains the validated claim until that calculation
  is repeated.
- The smaller-die 3 W RGB lens experiment is not a new fleet class or protocol change.
  Promote it only if the optical and electrical bench results justify replacing the
  current RGBW direction for some trunk fixtures.

## References

- ADR 0024 (COTS production lock and superseded July fleet snapshot).
- `docs/block-diagram/SYSTEM.md` (canonical living counts).
- `ops/bom.md` (per-role parts and spares math).
- LOG 2026-08-06 (Nevada City count update and Cambium three-fixture acceptance).
