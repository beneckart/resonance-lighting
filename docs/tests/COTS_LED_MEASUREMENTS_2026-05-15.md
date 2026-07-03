# COTS LED Measurements -- 2026-05-15

Purpose: first current and optics measurements for the three connected COTS
prototype stacks.

Firmware: `firmware/smoke_test`, version `smoke-2026-05-18.2`.

Boards under test:

- Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 RGB LED matrix over
  STEMMA-QT, fixture `E41B2C`.
- UnexpectedMaker FeatherS2 Neo built-in 5x5 RGB LED matrix, fixture `570D32`.
- M5Stack Atom Matrix built-in 5x5 RGB LED matrix, fixture `1B5108`.
- M5Stack Atom Matrix v1.1 + Atomic Battery Base + M5Stack Unit NeoHEX over
  Grove, fixture `55BA78`.

## Measurement Modes

Use serial commands, or use `/mode?m=<mode>` while OTA WiFi is active.

| Mode | Name | Intended reading |
| --- | --- | --- |
| `q` | Quiet baseline, WiFi off, LEDs off | Board idle current without radio |
| `0` | LEDs off, WiFi/OTA unchanged | Radio/OTA overhead baseline |
| `1` | Center dim warm white | Default gobo candidate |
| `2` | 3-pixel RGB fringe | Chromatic/fringing candidate |
| `3` | Center 3x3 dim warm white | Small-area wash candidate |
| `4` | Full-array very-low white | Full-aperture low wash |
| `5` | Full-array capped white, brief only | Capped worst-case spot check |

## Procedure

1. Put a USB power meter inline with one board at a time.
2. Let the board boot and settle for 10 seconds.
3. Record `0` first if WiFi OTA is active.
4. Record `1`, `2`, `3`, and `4`.
5. Use `5` only as a short-duration reading.
6. Use serial command `q` for the quiet baseline, because it intentionally
   stops the OTA server and turns WiFi off.
7. For optics/gobo notes, keep LED-to-gobo distance and projection distance
   fixed for all modes.

## Results

Record USB voltage/current from the meter. Add notes for visible flicker,
gobo washout, color fringing, and thermal concerns.

| Board | Power source | Mode | USB V | USB mA | Optical notes | Thermal / other notes |
| --- | --- | --- | ---: | ---: | --- | --- |
| C6 + IS31FL3741 | USB | `0` | | | | |
| C6 + IS31FL3741 | USB | `q` | | | | |
| C6 + IS31FL3741 | USB | `1` | | | | |
| C6 + IS31FL3741 | USB | `2` | | | | |
| C6 + IS31FL3741 | USB | `3` | | | | |
| C6 + IS31FL3741 | USB | `4` | | | | |
| C6 + IS31FL3741 | USB | `5` | | | | |
| FeatherS2 Neo | USB | `0` | | | | |
| FeatherS2 Neo | USB | `q` | | | | |
| FeatherS2 Neo | USB | `1` | | | | |
| FeatherS2 Neo | USB | `2` | | | | |
| FeatherS2 Neo | USB | `3` | | | | |
| FeatherS2 Neo | USB | `4` | | | | |
| FeatherS2 Neo | USB | `5` | | | | |
| Atom Matrix | USB | `0` | | | | |
| Atom Matrix | USB | `q` | | | | |
| Atom Matrix | USB | `1` | | | | |
| Atom Matrix | USB | `2` | | | | |
| Atom Matrix | USB | `3` | | | | |
| Atom Matrix | USB | `4` | | | | |
| Atom Matrix | USB | `5` | | | | |
| Atom + NeoHEX | USB / battery base | `0` | | | | |
| Atom + NeoHEX | USB / battery base | `q` | | | | |
| Atom + NeoHEX | USB / battery base | `1` | | | | |
| Atom + NeoHEX | USB / battery base | `2` | | | | |
| Atom + NeoHEX | USB / battery base | `3` | | | | |
| Atom + NeoHEX | USB / battery base | `4` | | | | |
| Atom + NeoHEX | USB / battery base | `5` | | | | |

## Notes

- `q` is the best baseline for sleep/current planning, but this smoke firmware
  is not yet a true deep-sleep test.
- `0` is useful for subtracting LED current when OTA WiFi is active.
- `smoke-2026-05-15.7` removes the earlier NeoPixel double-dimming path
  where low RGB values were also scaled by low `setBrightness()` values. It
  also keeps IS31FL3741 RGB565 test values above the low-end quantization
  threshold so full-array low modes are visible.
- The integrated 5x5 matrices are useful for early optics and firmware
  validation, but their board-level current is not a substitute for the final
  ESP32-C3-MINI custom hardware baseline.
- The Atom + NeoHEX variant uses Atom Grove GPIO26 for the NeoHEX data line,
  37 pixels, and fixture ID `55BA78`. The selected center pixel index is 18.
  `smoke-2026-05-18.2` changes mode `3` from a contiguous seven-index run to
  a first-pass center hex cluster: `11, 12, 17, 18, 19, 24, 25`.
