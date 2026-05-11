# PowerFeather V1/V2 schematic notes — 2026-05-10

Inputs reviewed:

- `esp32-s3-powerfeather-7620cc4fefa671436564aefb91d09158.pdf` — V1 schematic, Rev 5, dated 2023-08-14.
- `esp32-s3-powerfeather-v2-c8fb0b7f2b084b2013f65c973cfaf223.pdf` — V2 schematic, Rev 2, dated 2026-01-11.

Status: first-pass schematic comparison. This is not a layout review.

## High-level conclusion

PowerFeather V2 is a strong reference architecture for the Resonance Lighting custom board.

The important discovery is that V1 and V2 share the same basic charger/power-path architecture. Both use **BQ25628E**. V2 becomes LiFePO4-appropriate at the board level mainly by replacing the fuel gauge and 3.3 V regulator:

- V1: **LC709204F** fuel gauge + **XC6220B331ER-G** LDO.
- V2: **MAX17260** fuel gauge + **20 mΩ sense resistor** + **TPS631013** buck-boost regulator.

That is exactly the class of change needed for LiFePO4: the charger was already capable, but the fuel-gauge/regulator system had to change.

## Shared V1/V2 blocks

Both V1 and V2 include:

- ESP32-S3-WROOM-1 module.
- USB-C connector.
- `VDC` external/solar input.
- Schottky ORing from `VBUS` and `VDC` into `VS`.
- BQ25628E charger / power-path IC.
- `+VSW` switched/system rail.
- AP22916 load switches for controllable rails.
- Feather-style 1x16 headers.
- PowerFeather pinout header.
- STEMMA-QT connector (`J2`).
- Reset/user buttons and status LEDs.
- Battery JST connector.

## V1-specific notes

V1 uses:

- `U4` BQ25628E charger/power-path.
- `U5` LC709204F fuel gauge.
- `U1` XC6220B331ER-G LDO for `+3.3VP`.
- `U6` / `U9` AP22916 load switches.

Why V1 is not a LiFePO4 board-level solution:

- The BQ25628E can support LiFePO4, but V1's LC709204F fuel gauge is Li-ion/LiPo oriented.
- The XC6220 LDO needs headroom; LiFePO4 nominal voltage around 3.2 V is not suitable for a robust 3.3 V rail across discharge.
- V1 remains useful as a LiPo fallback and as a charger/power-path reference.

## V2-specific notes

V2 uses:

- `U4` BQ25628E charger/power-path.
- `U5` MAX17260 fuel gauge.
- `R22` 20 mΩ sense resistor in the battery/fuel-gauge path.
- `U1` TPS631013YBGR buck-boost regulator for `+3.3VP`.
- `L2` 2.2 µH inductor for the buck-boost stage.
- `Q1A/Q1B` BSS138 FETs around the STEMMA-QT I2C lines.
- `VSQT`, `SDA_SQT`, and `SCL_SQT` nets for the external STEMMA-QT power/I2C domain.

Why V2 is suitable for LiFePO4 testing:

- BQ25628E supports LiFePO4 charge settings and power-path behavior.
- MAX17260 supports LiFePO4 profiles and current-sense telemetry.
- TPS631013 buck-boost can regulate 3.3 V across the LiFePO4 voltage range.
- STEMMA-QT power-domain isolation helps prevent back-powering when external modules are switched off.

## Visual / arrival inspection checklist

When Elecrow boards arrive, identify V1 vs V2 before attaching LiFePO4.

Likely V2 identifiers:

- TPS631013 regulator package near the 3.3 V rail.
- 2.2 µH inductor associated with the buck-boost converter.
- MAX17260 fuel gauge near the battery connector.
- 20 mΩ current-sense resistor near battery/fuel-gauge path.
- BSS138 I2C level/power-domain components near STEMMA-QT connector.

Likely V1 identifiers:

- XC6220 LDO.
- LC709204F fuel gauge.
- No 20 mΩ current-sense resistor for fuel-gauge current measurement.
- Simpler STEMMA-QT I2C connection without the V2 BSS138 isolation scheme.

Firmware/I2C scan should also distinguish revisions:

- V2 should expose MAX17260 and TPS631013 devices in addition to BQ25628E.
- V1 should expose LC709204F and BQ25628E, but not TPS631013.

## What to copy conceptually into a custom board

Copy these architectural decisions:

- WROOM-class pre-certified ESP32 module with onboard PCB antenna.
- BQ25628E-class switch-mode charger with power path and VINDPM/IINDPM behavior.
- Fuel gauge with real current sensing and LiFePO4 support.
- Buck-boost 3.3 V rail for LiFePO4, not an LDO-only 3.3 V rail.
- Switched external module rail (`VSQT`-like) that defaults off.
- I2C power-domain isolation for switched external modules.
- Solar/DC input separate from USB-C.
- Battery thermistor / TS path.
- Charger/fuel-gauge telemetry as a first-class firmware subsystem.

## What not to copy blindly

Do not copy the schematic into a production custom board without layout expertise.

Potentially difficult parts:

- BQ25628E is a small WQFN charger with switching power layout requirements.
- TPS631013 is a tiny WCSP buck-boost with sensitive layout and inductor placement.
- MAX17260 current-sense routing must be done carefully for useful measurements.
- The ESP32-S3-WROOM antenna keep-out must be honored in the actual hat geometry.
- Schottky-OR solar/USB input and exposed `VDC` path need production-friendly connector, strain relief, and input protection.

If PowerFeather KiCad/Gerber files become available, they are likely the best starting point for a bespoke Resonance board. If only schematics are available, use them as an architectural reference and hire/seek a hardware review before manufacturing.

## Custom Resonance board direction implied by V2

The custom board should now be PowerFeather-derived, not CN3058-derived:

```
Solar panel / VDC connector
  → input protection / Schottky or ideal-diode input handling
  → BQ25628E-class charger + power path
  → LiFePO4 cell + thermistor
  → MAX17260-class fuel gauge / current sensing
  → TPS631013-class 3.3 V buck-boost
  → ESP32-S3-WROOM-class module
  → switched external LED/STEMMA rail
  → LED module connector(s)
```

The old target of ESP32-C3-MINI-1 + CN3058 + AP2112K + direct-Vbat WS2812B is now superseded by the headroom/RF/telemetry/fail-safe architecture.
