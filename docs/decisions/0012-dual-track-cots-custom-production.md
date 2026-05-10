# 0012 — Dual-track production architecture: COTS fallback plus custom PCBA optimization

**Date:** 2026-05-08
**Status:** Accepted
**Owners:** Ben + Steve
**Supersedes:** ADR 0006 where it bans all dev-board / daughterboard architectures

## Context

The top production constraint is not “one custom PCB.” The top production constraint is that 100 fixtures must be assembled with little skilled labor, no repetitive soldering, no per-unit pairing, no per-unit firmware configuration, and no fragile hand-built wiring. ADR 0009 captures this correctly; ADR 0006 over-constrained the implementation by forbidding dev boards and headers outright.

Cost is not the primary risk. Schedule and field reliability are.

## Decision

Build two production-credible tracks in parallel:

### Track A — COTS deployable prototype / fallback

Use off-the-shelf boards, charger modules, LED daughterboards, pre-crimped cables, USB/JST connections, standoffs, screws, and factory-soldered headers where needed. This is not a toy prototype; it must be good enough to deploy if the custom PCBA slips.

Allowed in Track A:

- Factory-soldered headers.
- Stacked boards held by screws/standoffs.
- USB-C / USB-A short internal power cables with strain relief.
- JST-PH / JST-SH / STEMMA/Qwiic cables.
- Separate solar charger board, MCU board, and LED board.
- COTS LiPo fallback if LiFePO4 custom power is not ready.

Not allowed in Track A:

- Hand-soldering 100 header sets.
- Hand-crimping 100 harnesses.
- Friction-only board stacks.
- Per-unit firmware setup, pairing, calibration ritual, or hidden assembly steps.

### Track B — custom PCBA optimization

Design the custom board after the COTS track proves the power, optics, firmware, assembly, and enclosure. The custom board may integrate charger, MCU, connectors, test pads, and LED rail switching. The LED array may remain a separate daughterboard if optics are still evolving.

## Consequences

- A COTS bill of materials must be maintained alongside the custom BOM.
- The roadmap must contain a hard go/no-go date for “custom PCBA production vs COTS fallback production.”
- The custom board is an optimization, not the only route to a working installation.
- The enclosure must be designed with enough volume and mounting flexibility to accept both architectures.
- The smoke-test firmware must run on both COTS and custom boards via board-specific pin definitions.
- The project can spend money to buy schedule confidence: duplicate boards, spare modules, and multiple COTS candidates are expected.
