# 0001 — Use ESP32-C3-MINI-1 for production

**Date:** 2026-05-06
**Status:** Superseded by ADR 0011 — MCU module selection: pre-certified module with RF/headroom margin
**Owners:** Ben

## Context

Need to pick the MCU for the production carrier board. Constraints:

- Must support WiFi-class peer-to-peer coordination at BRC with no infrastructure (no APs).
- Must support standard OTA over WiFi after a single USB/pogo flash.
- Must drive WS2812/SK6805 LEDs reliably.
- Must run on a single-cell battery power system.
- Must avoid custom RF design.
- 100 units, art-project budget, but schedule/reliability matter more than BOM cost.

## Original decision

Use ESP32-C3-MINI-1 for production.

## Why this is superseded

The original decision over-weighted compactness, low unit cost, and low power. The hat enclosure is not especially space-constrained, the budget can absorb a larger module, and the project should not trade compute/RAM/flash/RF margin for a smaller module package. The updated rule is to choose a pre-certified Espressif module with integrated RF/antenna and comfortable firmware headroom. See ADR 0011.
