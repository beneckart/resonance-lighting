# BOM — first pass

**Status:** Estimate, not yet validated against JLCPCB part library or current spot pricing. Verify each part before committing the order.

## Per-fixture electronics (the carrier board)

| Designator | Part | Function | Qty | Source | Unit @ qty 100 | Notes |
|------------|------|----------|-----|--------|----------------|-------|
| U1 | ESP32-C3-MINI-1 | MCU module (RF + USB + flash) | 1 | JLCPCB Basic | $1.50 | C2934560 confirm before order |
| U2 | CN3058 | LiFePO4 charger | 1 | JLCPCB Basic | $0.30 | Verify part number on JLCPCB |
| U3 | AP2112K-3.3 | 3.3 V LDO | 1 | JLCPCB Basic | $0.10 | SOT-25 |
| Q1 | P-MOSFET (SOT-23) | Reverse-polarity protection / power-path | 1 | JLCPCB Basic | $0.10 | E.g. AO3401 |
| D1 | Schottky (SOD-123) | Solar input protection | 1 | JLCPCB Basic | $0.05 | E.g. SS14 |
| D2 | TVS (SOD-123) | Solar input clamp | 1 | JLCPCB Basic | $0.10 | Optional, recommended |
| LED1 | 0603 LED (status) | Charge indicator | 1 | JLCPCB Basic | $0.02 | Green |
| LED2-10 | WS2812B-2020 | Lighting LEDs (1–9 per fixture, typically 3×3 grid) | 1–9 | JLCPCB Basic | $0.10 each | Final count TBD; budget $0.50–$0.90 per fixture |
| C bulk | 10 µF 0805 | Bulk on Vbat rail | 2 | JLCPCB Basic | $0.04 | |
| C decouple | 100 nF 0402 | Decoupling per IC + per LED | ~12 | JLCPCB Basic | $0.005 each = $0.06 | |
| R | Various 0402 | Pull-ups, pull-downs, ADC divider, Iset | ~10 | JLCPCB Basic | $0.005 each = $0.05 | |
| J1 | USB-C 16-pin | USB programming + charge | 1 | JLCPCB Basic | $0.40 | E.g. TYPE-C-31-M-12 |
| J2 | JST-PH 2-pin SMT | Solar panel connector | 1 | JLCPCB Basic | $0.15 | |
| J3 | JST-PH 2-pin SMT | Battery connector | 1 | JLCPCB Basic | $0.15 | |
| J4 | JST-PH 3-pin SMT | LED chain output (Vbat, GND, Data) | 1 | JLCPCB Basic | $0.20 | |
| F1 | Polyfuse 0805 | Battery overcurrent protection | 1 | JLCPCB Basic | $0.10 | E.g. 2 A hold, 4 A trip |
| SW1 | Tactile button SMT | Reset / mode | 1 | JLCPCB Basic | $0.10 | Optional, accessible through hat hole |
| **PCB + assembly** | | 2-layer 4-layer | 1 | JLCPCB | ~$3.00 | At qty 100, SMT both sides |
| **Carrier board subtotal** | | | | | **~$7.00** | |

## Per-fixture non-PCB electronics

| Part | Function | Source | Unit @ qty 100 |
|------|----------|--------|----------------|
| LiFePO4 18650 cell | Battery | Generic AliExpress / Battery Junction | $3.00 |
| 18650 holder (PCB-mount or wired) | Battery holder | Keystone / generic | $0.50 |
| 2 W solar panel (~5 V) | Solar input | Voltaic Systems / generic | $5.00 |
| WS2812B chain on flex strip with JST-PH (alternate to PCB-mount LEDs) | LED option | Adafruit / AliExpress | $1.00 |
| **Non-PCB subtotal** | | | **~$9.50** |

## Per-fixture mechanical

| Part | Function | Source | Unit @ qty 100 |
|------|----------|--------|----------------|
| Hat enclosure (MJF nylon, ~50 g) | Sealed solar-electronics housing | JLC3DP / PCBWay / Xometry | $5.00 |
| Filter / gobo (FDM PLA, ~10 g) | Patterned aperture | Bambu print on Steve's machine | $0.30 |
| Set screws × 3 (M3 × 8 mm) | Bamboo clamping | McMaster | $0.30 |
| Wire harness (panel + battery + LED) | Internal connections | JST-PH pre-crimped | $1.00 |
| **Mechanical subtotal** | | | **~$6.60** |

## Per-fixture summary

| Category | Cost |
|----------|------|
| Carrier PCB + assembly | $7.00 |
| Non-PCB electronics (battery, panel, LEDs) | $9.50 |
| Mechanical (enclosure, filter, harness) | $6.60 |
| **Per-fixture total target** | **~$23.10** |

## 100-fixture total

**~$2,310.** Plus shipping (overland from Asian fab partners), customs (negligible at this scale), and a 10–20% margin for prototype iterations and spares (target 110–120 units fab to have spares).

**Comparison target:** decompose `INV_2026_00401` (Elliot, 04-16, "rough cost of the light") to see where that quote lands. Differences likely driven by labor (handmade vs SMT-assembled), per-unit panel cost (Voltaic premium vs generic), and battery chemistry (LiPo vs LiFePO4).

## Open BOM questions

- Confirm CN3058 is actually in JLCPCB Basic (not Extended). If Extended, fee adds ~$3 setup + small per-unit. Worth checking MCP73123 too.
- Decide between WS2812B-2020 (PCB-mount) and a separate flex-strip WS2812B (off-board, plugged in). Off-board is more flexible mechanically and lets the LEDs be positioned wherever the optics work best inside the bamboo lantern, but adds a JST connector and harness.
- 18650 holder choice: PCB-mount (clean, but takes a lot of board area) or wired-to-board with a JST-PH (more flexible, fewer constraints on hat layout). Likely wired.
- Polyfuse value: bench-test the worst-case load current under the wand-stress scenario to size correctly. 2 A hold / 4 A trip is a starting estimate.

## Sourcing logistics for 100 units

- **JLCPCB**: PCBs and SMT assembly. ~3 weeks turnaround. Order one prototype run of 5 boards (~$30–60) before committing 100.
- **JLC3DP**: MJF hat enclosures. ~10 days turnaround. Same caveat — order 5 enclosures first to verify mechanical fit on a real bamboo lantern.
- **Voltaic Systems**: solar panels with the right form factor and reliability for desert. Check their 2 W ETFE panels.
- **Battery Junction / 18650.com**: LiFePO4 18650 cells, US-based.
- **AliExpress**: cheaper LiFePO4 cells if c