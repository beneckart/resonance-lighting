# NeoHEX Passive Adapter Rev A

Small adapter PCB for connecting M5Stack Unit HEX / Unit NeoHEX LED modules to
PowerFeather V2 or Feather-class controller boards without per-unit soldering.

Status: design packet plus KiCad 10 starter PCB layout.

See `kicad/` for a routed starter board that passes `kicad-cli pcb drc` with
zero violations. The starter layout is PCBA-friendly but not order-ready: J1
uses a local candidate footprint for the M5Stack A118 HY2.0-4P SMD connector,
J5 provides a JST-SH/STEMMA-QT fallback output for a Grove-to-STEMMA cable, J2
uses a stock SMT JST-PH footprint, and connector/cable orientation plus the
KiCad schematic still need verification before fab.

## Design Intent

This board treats HEX/NeoHEX as two logical interfaces:

- LED power: `VLED` + `GND`, supplied by an appropriate battery/load/boost rail.
- LED data: one MCU GPIO and shared `GND`.

The board intentionally does not include a regulator or boost converter. Power
conversion should stay on the power board. This adapter only handles keyed
connectors, power injection, data selection, and a data-line series resistor.

## Non-Goals

- No constant-current output. NeoHEX/HEX modules are addressable LED modules
  that expect a voltage rail, not a constant-current LED driver.
- No 3 V to 5 V boost conversion. A STEMMA/QT booster board class is useful for
  low-current 5 V I2C peripherals, but NeoHEX can draw hundreds of milliamps.
- No attempt to make STEMMA/QT power carry NeoHEX current.
- No USB-C repurposing as an internal LED harness.

## Schematic

```text
Power input                         NeoHEX / HEX outputs
-----------                         --------------------
J2.1 VLED ------------------------> J1.2 VLED
                                  -> J5.2 VLED
J2.2 GND  ------------------------> J1.1 GND
                                  -> J5.1 GND

Data input candidates
---------------------
J3.3 STEMMA_SDA -- SJ1 --+
J3.4 STEMMA_SCL -- SJ2 --+-- DATA_RAW -- R1 330R -- DATA_OUT --> J1.3 DATA
J4.3 GPIO_DIN  -- SJ3 --+                                      -> J5.4 DATA

Ground reference
----------------
J3.1 GND -------------------------- GND
J4.1 GND -------------------------- GND

Optional / normally open
------------------------
J3.2 STEMMA_V+ -- SJ4 -- VLED

J1.4 NC and J5.3 NC are routed to a labeled test pad only.
```

Only one of `SJ1`, `SJ2`, or `SJ3` should be closed. Leave `SJ4` open for
NeoHEX/HEX unless deliberately testing a tiny low-current load. Do not power a
37-LED module from a STEMMA/QT accessory rail.

J1 and J5 are parallel LED outputs. For Rev A bring-up, plug in only one LED
module/output cable at a time. J5 is intended for Adafruit 4528-style
Grove-to-STEMMA-QT cables; the NeoHEX signal is on the Grove yellow/SCL-position
wire, so J5 pin 4 is `DATA_OUT` and J5 pin 3 is `NC`.

## Connectors

| Ref | Function | Suggested connector | Pinout |
| --- | --- | --- | --- |
| J1 | HEX/NeoHEX output | M5Stack A118 HY2.0-4P SMD candidate | `1 GND`, `2 VLED`, `3 DATA`, `4 NC` |
| J2 | LED power input | JST-PH S2B-PH-SM4-TB SMT candidate | `1 VLED`, `2 GND` |
| J3 | STEMMA/QT data input | JST-SH BM04B-SRSS-TB SMT | `1 GND`, `2 V+`, `3 SDA`, `4 SCL` |
| J4 | Generic data input | JST-SH BM03B-SRSS-TB SMT | `1 GND`, `2 3V3_REF/NC`, `3 GPIO_DIN` |
| J5 | HEX/NeoHEX fallback output | JST-SH BM04B-SRSS-TB SMT | `1 GND`, `2 VLED`, `3 NC`, `4 DATA` |

Before layout, verify actual footprint pin numbering against the connector
datasheet and cable orientation. Grove-style cables are easy to adapt
mechanically while silently swapping the electrical order.

## Parts

| Ref | Value | Package | Notes |
| --- | --- | --- | --- |
| J1 | HY2.0-4P | SMD, local footprint | Candidate for M5Stack A118; verify against cable before order |
| J2 | JST-PH-2 | SMD right-angle | External LED power input |
| J3 | JST-SH-4 | SMD vertical | STEMMA/QT data input |
| J4 | JST-SH-3 | SMD vertical | Generic GPIO data input |
| J5 | JST-SH-4 | SMD vertical | Fallback output for Grove-to-STEMMA-QT cable |
| R1 | 330 ohm | 0805 | Series resistor on LED data line |
| C1 | 0.1 uF | 0805 | Local high-frequency decoupling, VLED to GND |
| C2 | 100-470 uF, >=6.3 V | CP_Elec_6.3x5.4 SMD | Optional bulk cap, VLED to GND |
| SJ1 | solder jumper | 2-pad | Select STEMMA SDA as data |
| SJ2 | solder jumper | 2-pad | Select STEMMA SCL as data |
| SJ3 | solder jumper | 2-pad | Select generic GPIO input as data |
| SJ4 | solder jumper | 2-pad, normally open | Optional STEMMA V+ to VLED, mark "NO NEOHEX" |
| TP1 | test pad | 1.5 mm | VLED |
| TP2 | test pad | 1.5 mm | GND |
| TP3 | test pad | 1.5 mm | DATA_RAW |
| TP4 | test pad | 1.5 mm | DATA_OUT |
| TP5 | test pad | 1.5 mm | J1.4 NC |

## Layout Guidance

- 2-layer board, 1 oz copper is enough for first article.
- Current starter board size: 72 mm x 35 mm, deliberately roomy for inspection.
- Use a solid ground pour on both layers with stitching vias.
- Route `VLED` and `GND` as short, wide traces or pours.
- Design current target: 1 A continuous margin for NeoHEX, even though the
  stickered 100% white current is 568 mA.
- Put R1 close to J1 DATA if possible.
- Put C1 and C2 close to J1 VLED/GND.
- Keep populated parts SMT for PCBA quoting; only unpopulated M2 mounting holes
  and connector locating holes are through-board features.
- Keep J1 and J5 close enough to inspect but far enough apart to avoid plugging
  both output cables during normal bring-up.
- Silkscreen every connector with both signal names and cable colors:
  `GND black`, `VLED red`, `DATA yellow`, `NC white`.
- Silkscreen `SJ1/SJ2/SJ3: close one only`.
- Silkscreen `SJ4: leave open for NeoHEX`.

## Assembly Variants

### A1 - STEMMA Data, External LED Power

Default target for PowerFeather/Feather testing.

- Populate J1, J2, J3, J5, R1, C1.
- Optional C2.
- Close either SJ1 or SJ2 depending on chosen GPIO pin.
- Leave SJ3 open.
- Leave SJ4 open.
- Feed LED power through J2.
- Use either J1 direct-to-Grove/HY2.0 output or J5 through the Grove-to-STEMMA-QT
  cable, not both at once.

### A2 - Generic GPIO Data, External LED Power

Useful if the controller exposes a non-STEMMA GPIO connector.

- Populate J1, J2, J4, J5, R1, C1.
- Optional C2.
- Close SJ3.
- Leave SJ1, SJ2, and SJ4 open.
- Feed LED power through J2.
- Use either J1 direct-to-Grove/HY2.0 output or J5 through the Grove-to-STEMMA-QT
  cable, not both at once.

### A3 - STEMMA Power Test, Tiny Loads Only

Not for NeoHEX/HEX production use.

- Populate J1 or J5, J3, R1, C1.
- Close one data jumper.
- Close SJ4 only for a low-current validation load.

## Bring-Up Checklist

1. With no LED module connected, check continuity:
   - J2 VLED to J1 VLED.
   - J2 VLED to J5 VLED.
   - J2 GND to J1 GND.
   - J2 GND to J5 GND.
   - Selected data input through R1 to J1 DATA.
   - Selected data input through R1 to J5 DATA.
   - No short from VLED to GND.
2. Confirm only one data-source solder jumper is closed.
3. Confirm SJ4 is open before connecting NeoHEX/HEX.
4. Confirm only one output cable/module is connected.
5. Power from current-limited bench supply first.
6. Send smoke-test mode `1`, then `2`, then `4`.
7. Check VLED sag and data waveform if LEDs flicker.
8. Record module current with SEN0291 before full-white stress tests.

## Open Questions Before Ordering Rev A

- Verify the local M5Stack A118 HY2.0-4P SMD candidate footprint physically
  matches the M5Stack Grove/HY2.0 cable and preserves the intended J1 pin order.
- Verify J5 with an actual Adafruit 4528-style Grove-to-STEMMA-QT cable and
  confirm the NeoHEX data signal lands on J5.4.
- Verify the J2 JST-PH SMD power connector matches available pre-crimped power
  leads, or swap to SMT JST-GH if the harness standard changes.
- Whether Rev A should include footprints for an optional level shifter.
- Whether the adapter should mechanically mount in the hat or float in the
  cable harness.
- Whether PowerFeather V2 exposes a preferred GPIO near a connector, making
  STEMMA data repurposing unnecessary.
