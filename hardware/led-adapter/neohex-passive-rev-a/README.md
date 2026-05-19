# NeoHEX Passive Adapter Rev A

Small adapter PCB for connecting M5Stack Unit HEX / Unit NeoHEX LED modules to
PowerFeather V2 or Feather-class controller boards without per-unit soldering.

Status: design packet, not yet captured in KiCad.

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
Power input                         NeoHEX / HEX output
-----------                         -------------------
J2.1 VLED ------------------------> J1.2 VLED
J2.2 GND  ------------------------> J1.1 GND

Data input candidates
---------------------
J3.3 STEMMA_SDA -- SJ1 --+
J3.4 STEMMA_SCL -- SJ2 --+-- DATA_RAW -- R1 330R -- DATA_OUT --> J1.3 DATA
J4.3 GPIO_DIN  -- SJ3 --+

Ground reference
----------------
J3.1 GND -------------------------- GND
J4.1 GND -------------------------- GND

Optional / normally open
------------------------
J3.2 STEMMA_V+ -- SJ4 -- VLED

J1.4 NC is routed to a labeled test pad only.
```

Only one of `SJ1`, `SJ2`, or `SJ3` should be closed. Leave `SJ4` open for
NeoHEX/HEX unless deliberately testing a tiny low-current load. Do not power a
37-LED module from a STEMMA/QT accessory rail.

## Connectors

| Ref | Function | Suggested connector | Pinout |
| --- | --- | --- | --- |
| J1 | HEX/NeoHEX output | HY2.0-4P / Grove-compatible socket | `1 GND`, `2 VLED`, `3 DATA`, `4 NC` |
| J2 | LED power input | JST-PH-2 or JST-GH-2 | `1 VLED`, `2 GND` |
| J3 | STEMMA/QT data input | JST-SH-4, 1.0 mm | `1 GND`, `2 V+`, `3 SDA`, `4 SCL` |
| J4 | Generic data input | JST-SH-3 or 0.1 in header | `1 GND`, `2 3V3_REF/NC`, `3 GPIO_DIN` |

Before layout, verify actual footprint pin numbering against the connector
datasheet and cable orientation. Grove-style cables are easy to adapt
mechanically while silently swapping the electrical order.

## Parts

| Ref | Value | Package | Notes |
| --- | --- | --- | --- |
| R1 | 330 ohm | 0603 or 0805 | Series resistor on LED data line |
| C1 | 0.1 uF | 0603 | Local high-frequency decoupling, VLED to GND |
| C2 | 100-470 uF, >=6.3 V | radial or low-profile SMD | Optional bulk cap, VLED to GND |
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
- Target board size: roughly 25 mm x 18 mm, adjust to connector footprints.
- Use a solid ground pour on both layers with stitching vias.
- Route `VLED` and `GND` as short, wide traces or pours.
- Design current target: 1 A continuous margin for NeoHEX, even though the
  stickered 100% white current is 568 mA.
- Put R1 close to J1 DATA if possible.
- Put C1 and C2 close to J1 VLED/GND.
- Add two M2 mounting holes or zip-tie slots for strain relief.
- Silkscreen every connector with both signal names and cable colors:
  `GND black`, `VLED red`, `DATA yellow`, `NC white`.
- Silkscreen `SJ1/SJ2/SJ3: close one only`.
- Silkscreen `SJ4: leave open for NeoHEX`.

## Assembly Variants

### A1 - STEMMA Data, External LED Power

Default target for PowerFeather/Feather testing.

- Populate J1, J2, J3, R1, C1.
- Optional C2.
- Close either SJ1 or SJ2 depending on chosen GPIO pin.
- Leave SJ3 open.
- Leave SJ4 open.
- Feed LED power through J2.

### A2 - Generic GPIO Data, External LED Power

Useful if the controller exposes a non-STEMMA GPIO connector.

- Populate J1, J2, J4, R1, C1.
- Optional C2.
- Close SJ3.
- Leave SJ1, SJ2, and SJ4 open.
- Feed LED power through J2.

### A3 - STEMMA Power Test, Tiny Loads Only

Not for NeoHEX/HEX production use.

- Populate J1, J3, R1, C1.
- Close one data jumper.
- Close SJ4 only for a low-current validation load.

## Bring-Up Checklist

1. With no LED module connected, check continuity:
   - J2 VLED to J1 VLED.
   - J2 GND to J1 GND.
   - Selected data input through R1 to J1 DATA.
   - No short from VLED to GND.
2. Confirm only one data-source solder jumper is closed.
3. Confirm SJ4 is open before connecting NeoHEX/HEX.
4. Power from current-limited bench supply first.
5. Send smoke-test mode `1`, then `2`, then `4`.
6. Check VLED sag and data waveform if LEDs flicker.
7. Record module current with SEN0291 before full-white stress tests.

## Open Questions Before KiCad Capture

- Exact connector families for J2/J4: JST-PH, JST-GH, JST-SH, or 0.1 in header.
- Whether Rev A should include footprints for an optional level shifter.
- Whether the adapter should mechanically mount in the hat or float in the
  cable harness.
- Whether PowerFeather V2 exposes a preferred GPIO near a connector, making
  STEMMA data repurposing unnecessary.
