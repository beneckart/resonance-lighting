# 0032 -- Nevada City production fleet allocation

**Date:** 2026-08-06

**Status:** Accepted -- current production target

**Owners:** Ben

## Context

ADR 0024 locked the 2026 production electronics architecture on COTS
PowerFeather V2 and recorded a tentative 150-152-fixture allocation: 72
downlights, 38-40 perimeter lights, 24 uplights, and 16 chandelier lights.
That allocation was intentionally expected to move when the physical tree and
lighting composition came together.

The Nevada City build has now converged on a substantially firmer layout. The
tree has three hanging rings with 24 downlights each. The perimeter has settled
at 24 fixtures, all using HEX LEDs. The chandelier has 18 light positions with a
mixed HEX/RGBW population. The former uplight class has evolved into about 16
trunk lights, with RGBW as the production direction. A smaller lensed 3 W RGB
module is also being tested there for additional throw.

The project has already bought hardware above this deployment count. That
inventory is useful contingency and does not require installing extra fixtures
just to consume it.

## Decision

1. The current production target is nominally **130 fixtures**:
   - 72 hanging downlights, arranged as three rings of 24;
   - 24 perimeter lights, all HEX;
   - 18 chandelier lights, with a mixed HEX/RGBW population; and
   - about 16 trunk lights, trending all RGBW.
2. The lensed 3 W RGB trunk-light variant remains an active qualification trial.
   It may replace some or all of the 4 W RGBW trunk allocation if its throw,
   appearance, power, thermal, and mechanical results are better.
3. The team intends to install this full nominal layout. A lower deployed count
   is a contingency for an unforeseen integration, schedule, or field issue,
   not the planning baseline.
4. Purchased boards, LEDs, batteries, panels, sensors, enclosures, and harnesses
   beyond the 130-fixture target are retained as build recovery stock, field
   spares, and optional off-tree/camp inventory.
5. The COTS PowerFeather V2 decision, shared/fungible electronics strategy, and
   other architectural decisions in ADR 0024 remain in force. This ADR
   supersedes only ADR 0024's fixture count and class allocation.
6. `docs/block-diagram/SYSTEM.md` remains the canonical living count. Current
   planning documents and BOM math use 130 as the deployment baseline; historical
   reports keep the count that was true when they were written.

## Consequences

- The production BOM now has materially healthier spares than the old 150-152
  allocation implied.
- Network, OTA, bridge, and simulator planning use 130 deployed fixtures as the
  current baseline. A 150-node run can still be useful as a conservative stress
  case because the hardware inventory supports it.
- Firmware and fleet manifests may temporarily map the existing `uplight` class
  identifier to the physical trunk-light role. That naming should be cleaned up
  without creating a separate firmware image.
- Final trunk-light LED selection, mounting, power arrangement, enclosure details,
  and sensor allocation remain open integration work.
- The 18-position chandelier housing, service access, power distribution, and
  exact HEX/RGBW mix need to match the updated count.

## Validation required

- Compare the 4 W RGBW and lensed 3 W RGB trunk-light variants for visible throw,
  color, power draw, heat, weather protection, and mounting simplicity.
- Confirm the physical 24-perimeter and 18-chandelier layouts against the build.
- Update commissioning manifests and dashboards to represent all four current
  classes and the 130-fixture target.
- Re-run network scale projections at 130 nodes; optionally retain 150 nodes as a
  stress case.
