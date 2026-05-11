# 0018 — LED module/interface plan for COTS testing

**Date:** 2026-05-10
**Status:** Accepted
**Owners:** Ben + Steve

## Context

The project wants future-proof LED geometry — ideally 3x3, 5x5, or larger — while keeping default optical behavior crisp through the gobo/filter. The first COTS survey identified several LED boards, but not all are no-solder or compatible with PowerFeather's STEMMA-QT connector.

Important interface distinction:

- **STEMMA-QT/Qwiic** is an I2C connector family using JST-SH. It carries power, ground, SDA, and SCL.
- **Grove/HY2.0** is a physical connector family. It can carry I2C, UART, GPIO, analog, or custom signals depending on device.
- **M5Stack NeoHEX** uses Grove/HY2.0 physically but is a WS2812C single-wire LED board, not an I2C device.

## Decision

Use two LED paths in parallel for testing:

1. **Adafruit IS31FL3741 13x9 matrix** as the primary no-solder PowerFeather/STEMMA-QT LED module.
2. **M5Stack NeoHEX** as the primary no-solder WS2812/GPIO geometry experiment.

FeatherS2 Neo and Atom Matrix remain integrated 5x5 fallback/optics boards.

## Consequences

- PowerFeather + IS31FL3741 is the cleanest no-solder COTS stack: STEMMA-QT cable, I2C control, switchable `VSQT` rail.
- NeoHEX must be wired as GPIO data + power + ground. It may require a 5 V or otherwise suitable LED rail and is not plug-and-play on STEMMA-QT.
- The IS31FL3741 matrix is multiplexed PWM over I2C, not NeoPixel. It must be tested for gobo projection artifacts, brightness, and current.
- NeoHEX has many LEDs and significant current in full-white modes; firmware must cap brightness/current from the start.
- The final custom board should keep LED module choice flexible until optics tests are complete.

## Tests required

For each LED module:

- Center LED projection.
- 3-pixel RGB/chromatic fringing.
- 9x9 or center-crop modes.
- Full-array low-brightness animation.
- Current draw at representative brightness.
- Rail-off leakage / back-powering.
- Sleep current with module connected.
- Mechanical placement on optical axis.
- Diffuser on/off behavior where applicable.
