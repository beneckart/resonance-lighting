# 0009 -- Minimize per-fixture operations at scale (O(1), not O(N))

**Date:** 2026-05-06
**Status:** Accepted, clarified by ADR 0012
**Owners:** Ben, Steve

## Context

Producing 100 functional, deployable fixtures imposes operations cost on whoever does the assembly. Anything done per-fixture is multiplied by 100 -- a 10-minute task becomes a 17-hour day, a 30-second annoyance becomes 50 minutes of unpaid grunt work. The 2018 LoRa-pendant project burned ~40 hours of one weekend just hand-soldering headers onto carrier boards for 16 devices. That experience should not be repeated at 100 units.

This is a top-tier design constraint. The original implementation interpreted it as "all custom SMT, no dev boards, no headers." ADR 0012 clarifies the more important rule: **no skilled, slow, error-prone per-fixture work.** A COTS/daughterboard architecture is allowed if it meets that rule.

## Decision

Every step in the per-fixture pipeline must be O(1) human time, or close to it.

### Required

- **No skilled soldering on receipt.** Production parts arrive fully assembled, or headers are factory-soldered, or boards are connected by keyed cables/connectors.
- **No hand-crimping at scale.** Harnesses are bought pre-crimped or made by a vendor.
- **No per-unit firmware configuration.** Same firmware image for every fixture; identity derived from MAC/eFuse or assigned through an automated process.
- **No per-unit pairing.** Fixtures discover each other automatically.
- **No per-unit calibration ritual.** Physical tolerances are absorbed mechanically; brightness/filter calibration is computed or automated if used.
- **No mandatory per-unit visual inspection.** Smoke-test rig reports node health.
- **USB/pogo recovery path.** Even if factory pre-flashing is available, fixtures have a reliable local flashing/recovery method.

### Allowed when they reduce risk

- Factory-soldered headers.
- Screw-mounted stacked boards.
- COTS MCU boards.
- COTS charger boards.
- LED daughterboards.
- Short internal USB cables with strain relief.
- JST/STEMMA/Qwiic/Grove-style keyed cables.
- COTS LiPo fallback architecture, if chemistry and thermal behavior are correct.

### Not allowed

- Hand-soldering 100 header sets.
- Hand-crimping 100 harnesses.
- Fragile friction-only board stacks.
- Ambiguous/unkeyed power connectors that can be reversed.
- Per-unit setup wizards.
- Manually pairing nodes in the field.
- Relying on OTA as the only recovery path.

## Operations that can legitimately scale O(N)

These are unavoidable but must be timed and budgeted:

- Mating the assembled hat onto the bamboo lantern.
- Installing the patterned filter.
- Hanging fixtures from the tree.
- Observing final automated smoke-test output.

Target total per-fixture human operations time for Grass Valley + BRC integration: <= 7 minutes / unit, or <= 12 hours total for 100 units with two people.

## Consequences

- ADR 0012 creates a COTS production fallback in parallel with custom PCBA.
- The enclosure must support both COTS and custom board mounting until production architecture is locked.
- The smoke-test rig is mandatory, not optional.
- Factory pre-flashing is useful but does not replace local USB/pogo recovery.
- BOM decisions are judged by assembly risk and schedule risk, not just elegance or unit cost.
