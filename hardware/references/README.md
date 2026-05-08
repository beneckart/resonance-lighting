# References

Reference schematics, datasheets, and reference designs we're lifting from. Cite the file or URL each module's design draws from in its atopile source.

## Status

This directory will be populated as we work through each module. Initial pulls below — not yet downloaded; URLs and intent only.

## Power management

### CN3058 — LiFePO4 charger (our pick)

- Datasheet: search "CN3058 datasheet" — Consonance / Shanghai Belling. Multiple Chinese-vendor mirrors.
- JLCPCB part: confirm at jlcpcb.com → Parts Library → search "CN3058". Typically a Basic part.
- Reference circuit: from datasheet figure "Typical Application." Single LiFePO4 cell, programmable charge current via Rprog resistor, status output.

### bq24074 — LiPo charger with power-path (rejected — wrong chemistry, but useful reference)

The bq24074's *power-path topology* is what we want to mimic for LiFePO4. The IC does load sharing internally. CN3058 doesn't have built-in power-path, so we'll add a simple ideal-diode P-MOSFET ahead of the regulator.

- Datasheet: ti.com/lit/ds/symlink/bq24074.pdf
- Adafruit "Universal USB / DC / Solar LiPo Charger": adafruit.com/product/4755 — schematic at learn.adafruit.com.

### AP2112K-3.3 — 3.3 V LDO

- Diodes Inc. datasheet: diodes.com/assets/Datasheets/AP2112.pdf
- JLCPCB Basic part. Used in Adafruit Feathers, Sparkfun ESP32, and many others — verified to handle ESP32 transient peaks (~500 mA momentary).

## MCU

### ESP32-C3-MINI-1 — Espressif

- Datasheet: espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf
- Hardware design guidelines: espressif.com/sites/default/files/documentation/esp32-c3-mini-1_hardware_design_guidelines_en.pdf
- Reference circuit: from datasheet "Application Schematics" section. Includes USB-C wiring, programming pins, strapping pin requirements at boot.
- JLCPCB Basic part.

## LEDs

### WS2812B — Worldsemi

- Datasheet: cdn-shop.adafruit.com/datasheets/WS2812B.pdf
- Direct-from-battery wiring confirmed in Talisman v2 (`beneckart/future-robotics`). One 100 nF decoupling cap per LED, plus a bulk 10 µF on the rail.

## Mechanical reference

- Bamboo lantern: `enclosure/references/DOWN LIGHTS DRAWINGS.pdf` — Vishnu's shop drawing, 04-22.
- Hat dimensional constraints: see `BACKGROUND.md` Lighting section. ~165 mm OD placeholder; final dimension TBD.

## Existing dev boards we lifted ideas from

- **TTGO T-Beam** (LilyGO) — schematics on github.com/Xinyuan-LilyGO/LilyGo-T-Beam-Series. Ben's prior platform on Talisman v2; Steve has multiple in workshop.
- **TTGO T-Ice** (LilyGO, discontinued) — purpose-built ESP32 + WS2812B board. The white-enclosure modules in Steve's workshop.
- **DFRobot FireBeetle ESP32-E** — well-documented low-power IoT reference. Schematic at wiki.dfrobot.com.
- **Adafruit Feather ESP32-S3** — modular, schematic on Adafruit.
