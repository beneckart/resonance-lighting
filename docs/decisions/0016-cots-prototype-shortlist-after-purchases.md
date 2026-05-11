# 0016 — COTS prototype shortlist after R&D purchases

**Date:** 2026-05-10
**Status:** Accepted
**Owners:** Ben

## Context

Ben purchased the current R&D candidate set after the COTS survey. The purpose is to test several realistic paths in parallel rather than betting the project on one custom board.

The purchased candidates include PowerFeather boards from Elecrow, FeatherS2 Neo, M5Stack Atom Matrix, M5Stack NeoHEX, Adafruit IS31FL3741 13x9 RGB matrix, DFRobot DFR0559 Solar Power Manager 5V, several 1–5 W panels, and battery samples / fallback LiPo hardware.

## Decision

Focus bench testing on four COTS stacks:

1. **PowerFeather V2 + LiFePO4 + solar + Adafruit IS31FL3741 13x9 matrix.** Primary design-aligned path.
2. **PowerFeather V2 + LiFePO4 + solar + M5Stack NeoHEX.** Alternate LED geometry path; requires GPIO data and suitable LED rail, not STEMMA-QT.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback with integrated 5x5 optics. DFR0559 owns battery/solar, Feather battery JST stays empty.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback with ESP32 + 5x5 LEDs + USB-C in a tiny module.

## Consequences

- The old TTGO-only Phase 1 plan is no longer the main bench path. Existing TTGO modules remain useful, but COTS production candidates should be tested first.
- FeatherS2 Neo and Atom Matrix are not ideal production reference architectures, but they are valuable optical and fallback candidates because they remove LED daughterboard soldering.
- PowerFeather V2 + IS31FL3741 is the first COTS path that satisfies most desired constraints at once.
- The production decision should be based on measured current, solar behavior, optics, RF, assembly time, and sourcing — not on board elegance.

## Explicit constraints

- Do not attach LiFePO4 to boards that are only proven for LiPo/Li-ion.
- Do not attach a second battery to FeatherS2 Neo when powered by DFR0559 USB output.
- Do not assume NeoHEX is compatible with STEMMA-QT just because adapters exist; it is WS2812/GPIO, not I2C.
- Do not rely on external antennas/u.FL unless PCB-antenna range tests fail.
