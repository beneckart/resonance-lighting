# 0009 — Minimize per-fixture operations at scale (O(1), not O(N))

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben, Steve

## Context

Producing 100 functional, deployable fixtures imposes operations cost on whoever does the assembly. Anything done per-fixture is multiplied by 100 — a 10-minute task becomes a 17-hour day, a 30-second annoyance becomes 50 minutes of unpaid grunt work. The 2018 LoRa-pendant project burned ~40 hours of one weekend just hand-soldering headers onto carrier boards for *16* devices. That experience should not be repeated at 100×.

This is a top-tier design constraint — equal in importance to the chemistry and MCU decisions — and easy to violate accidentally if the constraint isn't explicit.

## Decision

**Every step in the per-fixture pipeline must be O(1) human time, or close to it.** Specifically:

- **No soldering on receipt.** Boards arrive from JLCPCB fully SMT-assembled. Every part on the BOM is in JLCPCB's Basic or Extended parts library. No through-hole parts that require hand-soldering. No headers anywhere except optional debug pads on a single dev board.
- **No per-unit configuration in firmware.** The same firmware image flashes onto every fixture identically. Per-unit identity (device ID, brightness calibration, neighbor list) is derived at runtime from the chip's eFuse MAC, or set once via OTA broadcast at install time.
- **No per-unit pairing.** Fixtures discover each other automatically over ESP-NOW at boot. Swap a broken fixture in 30 seconds without touching firmware on the rest of the swarm.
- **No per-unit firmware loading by hand if avoidable.** Investigate whether JLCPCB / PCBWay can flash custom firmware as part of their assembly process (this is offered by some fabs at small extra cost). If yes, **production fixtures arrive flashed and OTA-ready out of the box** — total per-fixture human ops time during production assembly is "plug it in, close the case." If no, build a flashing jig (USB-C breakout + pogo pin fixture + auto-flash-on-insert script) so each board flashes in ~10 seconds with zero attention. Target: 100 boards flashed in under one hour total.
- **No per-unit calibration ritual.** All physical tolerances absorbed mechanically (set screws on the bamboo neck, JST connectors that key one way, screws not glued). The bamboo's natural variability is handled by the hat's clamping mechanism, not by per-fixture adjustment.
- **No mandatory per-unit visual inspection.** Smoke test on receipt is automated: plug in, fixture boots, joins mesh, flashes a sequence indicating health (battery percentage as LED count, mesh peer count as color), reports back to a host. Visual confirmation is "did all 100 LEDs flash green at the right moment," not "let me check each one individually."

## Operations that *can* legitimately scale O(N) and which we'll budget for explicitly

Not everything can be O(1); these are unavoidable per-fixture but should be ≤ a few minutes each:

- Mating the assembled hat onto the bamboo lantern at Grass Valley (mechanical, requires a real pair of hands per fixture). Target ≤2 min/unit.
- Installing the patterned filter at the bamboo node (drop in, friction fit). Target ≤30 sec/unit.
- Hanging fixtures from the tree at BRC build week (rope work). Target ≤3 min/unit.
- Final smoke-test verification at Grass Valley. Target ≤30 sec/unit (automated, just observe).

**Total per-fixture human ops time at the Grass Valley + BRC integration steps: target ≤7 min/unit × 100 = ≤12 hours.** That's one long workday for two people, not a multi-day grind.

## Consequences

- **BOM constraint:** Every part SMT, every part in JLCPCB's library. No exceptions. If a needed part is only available through-hole or as a non-JLCPCB part, the design must change.
- **Connector choice:** All inter-PCB connectors (panel, battery, LED chain) are SMT-mounted JST-PH; the wire side is pre-crimped from the supplier with mating connectors. No hand-crimping at scale.
- **Firmware design:** OTA-first. Same image for every fixture. Per-unit identity from MAC. Mesh discovery automatic. No per-unit "first-boot setup wizard" that requires hand-holding.
- **Flashing investigation:** Add to TODO — investigate JLCPCB/PCBWay firmware pre-flash service. If supported, the production process becomes truly O(1) human time per board.
- **Flashing jig fallback:** If pre-flash is not supported or too expensive at qty 100, design and build a USB-C pogo-pin jig with auto-flash-on-insert script. Target 10 seconds per board, 100 in under an hour.
- **Smoke-test rig:** Build a simple host-side script that listens for fixture boot announcements over WiFi or ESP-NOW and reports a checklist of "all 100 nodes seen, all batteries above threshold, all mesh peer counts ≥ N." Removes per-fixture visual inspection.
- **Mechanical tolerance budget:** Hat-to-bamboo connection must accommodate the natural variability of bamboo without per-fixture custom fitting. Set screws are the canonical answer; verify on prototype before committing.

This ADR is in tension with no other ADR. It reinforces ADR 0006 (custom PCB with reflowed module, not dev-board-on-carrier) and ADR 0007 (modular hat) — both of those choices already exist partly to satisfy this constraint.
