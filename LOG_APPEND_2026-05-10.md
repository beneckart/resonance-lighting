## 2026-05-10 — Ben + ChatGPT — PowerFeather V2 / COTS R&D update

Second-pass architecture update after COTS search, purchases, and schematic review.

### What changed

- **PowerFeather V2 is now the leading COTS/reference architecture.** It appears to match the project unusually well: ESP32-S3-WROOM-1, onboard PCB antenna, BQ25628E charger/power-path, LiFePO4 support in V2, MAX17260 fuel gauge, TPS631013 buck-boost 3.3 V rail, switchable VSQT/STEMMA-QT rail, solar/DC input, and rich power telemetry. V2 status is still preliminary until hardware arrives and is verified.
- **PowerFeather V1 remains LiPo-only as a board-level system.** V1 uses BQ25628E, but the board-level fuel gauge and regulator choices make it unsuitable for LiFePO4 production use. It may still be a strong LiPo fallback.
- **PowerFeather V1/V2 schematic diff completed.** V1 and V2 both use BQ25628E. V2 swaps the 3.3 V regulator from XC6220 LDO to TPS631013 buck-boost, swaps the fuel gauge from LC709204F to MAX17260, adds a 20 mΩ current-sense resistor, and adds I2C power-domain isolation around the STEMMA-QT rail.
- **COTS purchases made.** Ben bought the R&D candidates discussed in the COTS survey except USB power meters, which are already on hand. Elecrow PowerFeather boards were ordered despite possible ambiguity about whether the listing is V2 or V1. Ben also contacted the PowerFeather creator about V2 availability and KiCad files.
- **LED module plan narrowed.** The Adafruit IS31FL3741 13x9 RGB matrix is the leading plug-and-play STEMMA-QT LED module for PowerFeather. M5Stack NeoHEX is promising optically but is WS2812/Grove, not STEMMA-QT/I2C, and likely needs a GPIO data line plus a 5 V or otherwise suitable LED rail. M5Stack Atom Matrix is a compelling all-in-one fallback with ESP32 + 5x5 LEDs + USB-C.
- **Battery sourcing narrowed.** Prefer one larger LiFePO4 cell per fixture, ideally 18650 1500–2000 mAh, instead of multiple 14430 cells in parallel. 14430 cells are easy to find and cheap, but packs of many small cells add contacts, matching, wiring, assembly, and QA risk.
- **Solar-panel plan clarified.** Square/rectangular 1–5 W panels are fine for R&D. Round panels remain aesthetically attractive for production but are harder to source quickly and should not block testing.

### Current COTS prototype tracks

1. **PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 13x9 matrix.** Primary design-aligned candidate.
2. **PowerFeather V2 + LiFePO4 + solar panel + M5Stack NeoHEX.** Alternative LED geometry test; not STEMMA-QT plug-and-play.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback: DFR0559 owns battery/solar, FeatherS2 Neo battery JST stays empty, Feather is powered over USB.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback: small ESP32 + 5x5 LEDs powered by USB from the solar manager.

### Immediate tests once parts arrive

- Confirm whether Elecrow PowerFeather boards are V2 or V1 by chip markings and I2C scan.
- Verify LiFePO4 configuration and charging behavior on actual V2 hardware before trusting it.
- Measure sleep current with VSQT off and LED modules attached.
- Measure solar harvest and charge behavior for each 1–5 W panel under sun, shade, and heat.
- Compare IS31FL3741, NeoHEX, FeatherS2 Neo, and Atom Matrix for gobo projection, brightness, color fringing, PWM artifacts, current draw, and mechanical fit.
- RF-test each candidate inside a mock hat with panel, battery, screws, and wiring in realistic locations.
- Validate fail-safe behavior: LEDs stuck on, MCU hang, watchdog reset, low-battery cutoff, and recovery from depleted battery when solar input returns.

### Follow-up docs added

- `docs/research/COTS_SURVEY_2026-05-10.md`
- `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md`
- `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`
- ADR 0015 — PowerFeather V2 as leading COTS/reference architecture
- ADR 0016 — Purchased COTS prototype shortlist
- ADR 0017 — Battery cell format and sourcing
- ADR 0018 — LED module/interface plan
