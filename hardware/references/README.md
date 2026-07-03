# References

Reference schematics, datasheets, and reference designs for the current hardware path.
Download/cite exact PDFs in design files when a custom board or adapter actually uses them.

## Current Reference Architecture

### PowerFeather V2

Primary COTS/reference design for 2026:

- ESP32-S3-WROOM-1 module with PCB antenna.
- BQ25628E charger / power path.
- MAX17260 fuel gauge.
- TPS631013 buck-boost 3.3 V regulator.
- Switchable `3V3` and `VSQT` rails.
- USB-C and VDC solar input.

Use the official PowerFeather docs, SDK, and any licensed KiCad/Gerber files if the creator
shares them. Do not blindly copy switching-regulator or RF layout without review.

### BQ25628E - Charger / Power Path

Current charger reference because it is validated on PowerFeather V2.

Design notes:

- Set `VBUS_OVP=1` for standard 6 V-class panels whose open-circuit voltage can exceed the
  default low input-OVP threshold.
- Add a supply-present-but-not-good HIZ requalification kick so bright-sun connect does not
  leave a fixture silently not charging.
- Verify LiFePO4 charge voltage/current and battery temperature behavior in firmware.

### MAX17260 - Fuel Gauge

Current gauge reference because it is validated on PowerFeather V2.

Design notes:

- Set battery capacity/profile deliberately and avoid changing it in the field.
- Treat LiFePO4 percentage SOC as advisory until the gauge learns a real cycle.
- Use voltage/current and corrected coulomb counting for production guardrails.

### TPS631013 - 3.3 V Buck-Boost

Current 3.3 V rail reference because it is validated on PowerFeather V2.

Design notes:

- LFP terminal voltage can sit near buck/boost crossover at light loads.
- Measure real efficiency at production loads; do not rely on nominal curves.

### ESP32-S3-WROOM-Class Module

Current MCU/RF reference. Use a pre-certified module with PCB antenna and generous
keep-out. Avoid custom RF and avoid u.FL unless mock-hat RF tests require it.

## LED References

### SK6812 / WS2812-Protocol LEDs

Current direct-GPIO LED family for both live roles:

- HEX SK6812 array for close-range animation / glow.
- 4 W RGBW point source for crisp gobo projection.

Design notes:

- LED rail must be switchable/default-off.
- Send explicit all-off before sleep/rail shutdown.
- Add current caps for lit-count x brightness.
- Use 4.2 V, not 5 V, for the HEX boost experiment unless a data level shifter is added.

### IS31FL3741 - Historical / Ruled Out For V2 Battery Build

The IS31FL3741 matrix was useful early bench hardware but is ruled out for the PowerFeather
V2 battery architecture because it browns out the board on the shared charger/gauge I2C bus
under WiFi. Keep references only for historical tests or a future isolated-bus experiment.

## Historical / Superseded References

### CN3058 - Historical LiFePO4 Charger Candidate

CN3058 was the early custom-board charger pick. It is now superseded by the
PowerFeather/BQ25628E reference architecture and should be treated as fallback history.

### AP2112K-3.3 - Historical LDO Candidate

AP2112K was part of the original ESP32-C3 first pass. The current reference uses a
buck-boost 3.3 V rail.

### ESP32-C3-MINI-1 - Historical MCU Candidate

ESP32-C3-MINI-1 was the original production MCU pick. Later ADRs moved the default bias to
ESP32-S3-WROOM-class modules for RF/headroom margin.

## Mechanical Reference

- Bamboo lantern: `enclosure/references/DOWN LIGHTS DRAWINGS.pdf` - Vishnu's shop drawing,
  2026-04-22.
- Hat dimensional constraints: see `BACKGROUND.md` and `enclosure/README.md`.

## Existing Dev Boards We Lifted Ideas From

- **TTGO T-Beam** - Ben's prior Talisman v2 platform; useful historical reference.
- **TTGO T-Ice** - discontinued ESP32 + WS2812B driver board from the Marquee workstream.
- **DFRobot DFR0559** - LiPo fallback solar manager, not the leading LFP architecture.
- **Adafruit Feather ESP32-S3** - modular ESP32-S3 reference.
