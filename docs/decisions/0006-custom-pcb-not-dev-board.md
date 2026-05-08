# 0006 — Custom carrier PCB, not dev-board-on-carrier

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben

## Context

Two paths exist for getting an ESP32 onto a custom board:

1. Use a **dev module with headers** (e.g. Seeed XIAO, Wemos S2 Mini, ESP32-C3 SuperMini), socketed onto a carrier board.
2. Reflow-solder a **bare RF module** (e.g. ESP32-C3-MINI-1) directly onto the carrier board as a SMT part.

In 2018, the LoRa-pendant project used path 1 (Wemos D1 Mini boards on a carrier). Hand-soldering headers for 16 devices took ~40 hours. Path 1 does not scale to 100 units.

## Options considered

- Dev module with headers (path 1).
- ESP32-C3 bare chip + supporting RF circuit + crystal + flash (full custom).
- ESP32-C3-MINI-1 module reflow-soldered to carrier (path 2).

## Decision

**Path 2 — ESP32-C3-MINI-1 module reflowed to the carrier board.**

The MINI-1 is a complete RF module: chip, crystal, flash, antenna, RF shield, FCC pre-certification, all in a metal-can SMT package with castellated pads on the edge. JLCPCB stocks it as a Basic part. Carrier board adds USB-C, regulator, charger, connectors, LEDs.

Result: 100 fully-assembled boards from JLCPCB with no human soldering required.

## Consequences

- **No through-hole parts on the BOM ever.** Every part SMT and in JLCPCB's library. Verify each part's stock before committing. Cost of slipping (extended part fee + setup) is real.
- **No headers anywhere except optional debug pads** on a single development unit. Production runs are pure SMT.
- Prototyping uses dev boards on breadboards (existing TTGO T-Beam and T-Ice modules from Steve's workshop, plus ESP32-C3 SuperMini if needed) with jumper wires. Do not design any custom PCB for prototyping; iterate firmware on dev boards, then translate the design to a custom board with the MINI-1 once the architecture is validated.
- The pin mapping on the production board differs from the prototype dev boards. Handle with a `firmware/esp32/boards/` directory containing per-board pin headers.
- **Plan for 2–3 board spins before committing to 100.** First fab will have at least one mistake. JLCPCB does 5-board orders for ~$30–60 with assembly; budget that into the timeline. Allow ~4–6 weeks of round-trips total.
