# COTS / reference-design survey -- 2026-05-10 update

Status: second-pass survey after purchasing R&D boards and reviewing PowerFeather V1/V2 schematics. Re-check stock, rev, and distributor status before production decisions.

## Executive summary

The COTS path is now strong enough to treat as a serious production fallback, not merely a bench-prototype convenience.

The leading candidate is **ESP32-S3 PowerFeather V2** paired with a plug-in LED module. V2 appears to solve many project risks simultaneously: WROOM-class ESP32, no external antenna, solar/DC input, power-path charger, LiFePO4 support, buck-boost 3.3 V rail, fuel-gauge telemetry, switchable STEMMA-QT rail, and enough compute/RAM/flash headroom that firmware does not have to be artificially constrained.

The main caveat is that PowerFeather V2 documentation is still marked preliminary. Boards ordered from Elecrow must be verified as actual V2 hardware on arrival.

## Current prototype shortlist

### Track A -- primary LiFePO4 / telemetry candidate

```
Solar panel -> PowerFeather V2 VDC
LiFePO4 cell -> PowerFeather battery JST
PowerFeather STEMMA QT -> Adafruit IS31FL3741 13x9 RGB matrix
```

Why this matters:

- Closest COTS match to the desired production architecture.
- Uses LiFePO4 rather than LiPo, if V2 hardware behaves as documented.
- Provides fuel-gauge and charger telemetry that can inform the 2027 design.
- Uses no external antenna.
- Uses no custom RF.
- Uses a no-solder LED module via STEMMA-QT.

Open questions:

- Are the Elecrow boards truly V2?
- Is V2 LiFePO4 behavior fully tested by the designer?
- Does the IS31FL3741 matrix look good through the gobo/filter?
- Does the matrix draw acceptable current on `VSQT`?
- Does I2C/multiplexed PWM produce any visible projection artifacts?

### Track B -- PowerFeather with WS2812-style external LED geometry

```
Solar panel -> PowerFeather V2 VDC
LiFePO4 cell -> PowerFeather battery JST
PowerFeather GPIO + suitable LED rail -> M5Stack NeoHEX
```

Why this matters:

- NeoHEX gives a center-plus-rings geometry that may be better than a square matrix for some mandala/gobo effects.
- It is no-solder on the LED-module side and comes with a Grove/HY2.0 cable.
- It is a good optical experiment even if it is not the final architecture.

Important caveat:

- NeoHEX is **not** a STEMMA-QT/I2C device. It is a WS2812C single-wire LED board using M5Stack HY2.0/Grove-style connectors. It needs power, ground, and a GPIO data line. It may also need a 5 V or otherwise suitable LED rail because M5Stack specifies 3.7-5.3 V supply.

### Track C -- FeatherS2 Neo + DFRobot DFR0559 LiPo fallback

```
Solar panel -> DFRobot DFR0559
LiPo cell -> DFRobot DFR0559
DFRobot USB output -> FeatherS2 Neo USB-C
FeatherS2 Neo battery JST left empty
```

Why this matters:

- Very assembly-proof.
- FeatherS2 Neo has ESP32-S2 plus integrated 5x5 RGB matrix.
- DFR0559 owns solar/battery management; Feather sees USB power.
- Good fallback if LiFePO4 custom/PowerFeather path slips.

Important caveat:

- This is a LiPo/Li-ion path, not LiFePO4.
- Do not attach a second battery to the FeatherS2 Neo in this configuration.

### Track D -- Atom Matrix + DFRobot DFR0559 ultra-simple fallback

```
Solar panel -> DFRobot DFR0559
LiPo cell -> DFRobot DFR0559
DFRobot USB output -> M5Stack Atom Matrix USB-C
```

Why this matters:

- Tiny, cheap, mechanically simple, no external LED board.
- Atom Matrix gives ESP32 + 5x5 WS2812C + USB-C in a 24 mm square object.
- Useful as a quick range/optics/fallback system even if not final.

Caveat:

- Power is 5 V USB only; battery/solar management must be off-board.
- It is not WROOM-class and does not provide the PowerFeather telemetry story.

## PowerFeather V2

PowerFeather V2 is currently the most aligned COTS candidate.

Relevant details from the official docs:

- ESP32-S3-WROOM-1 module.
- Board dimensions 65 mm x 23 mm x 7 mm.
- USB-C, battery JST, VDC input, STEMMA-QT, onboard PCB antenna.
- 240 MHz dual-core ESP32-S3 with 8 MB flash and 2 MB PSRAM.
- Power path can allow battery to supplement `VUSB` or `VDC` during load spikes.
- V2 supports LiFePO4 using MAX17260 fuel gauge profiles and TPS631013 buck-boost regulation.
- V1 does not support LiFePO4 at the board level, despite the charger IC supporting it, because V1 uses a LiPo-oriented fuel gauge and an LDO 3.3 V rail.
- Solar behavior is pseudo-MPPT: firmware can set the panel MPP voltage, and the charger dynamically regulates charge current to avoid collapsing the panel below that voltage.
- Hardware design files currently appear to include schematics and 3D models, not public KiCad board files.

Use:

- Primary COTS prototype.
- Leading reference architecture for a custom board.
- If V2 is stable and available in quantity, possible 2026 production board.

Procurement caveat:

- Elecrow product listing is useful but may not be explicit enough to distinguish V1 vs V2. Confirm hardware revision on arrival by chip markings and I2C scan.

## PowerFeather V1 vs V2 schematic notes

See `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md` for the detailed notes.

Short version:

- Both V1 and V2 use BQ25628E charger/power-path.
- V1 uses LC709204F fuel gauge and XC6220 LDO.
- V2 uses MAX17260 fuel gauge, 20 mohm sense resistor, and TPS631013 buck-boost.
- V2 adds/clarifies STEMMA-QT I2C power-domain isolation around `VSQT`.
- These changes are exactly what make V2 plausible for LiFePO4.

## LED module candidates

### Adafruit IS31FL3741 13x9 RGB matrix -- leading plug-and-play LED module

Facts:

- 13x9 RGB LED matrix, 117 RGB LEDs.
- 3 mm pitch.
- I2C control via IS31FL3741/IS32FL3741 driver.
- STEMMA-QT/Qwiic connectors.
- Runs from 3.3-5 V power and logic.
- Adafruit recommends 5 V for best green/blue headroom, but 3.3 V operation is supported.
- Four mounting holes.
- Adafruit publishes learn-guide resources and PCB files.

Why it is strong:

- It is the only currently identified no-solder, PowerFeather-STEMMA-QT-compatible RGB LED matrix.
- PowerFeather can switch the `VSQT` rail, giving a clean LED-module off state.
- A 9x9 center crop gives a true center LED and ample animation headroom.

Risks:

- It is multiplexed PWM, not NeoPixel. Test for projection artifacts through the gobo.
- I2C is slower than WS2812-style direct LED data. This is fine for ambient/simple animations, but not for high-frame-rate video.
- Current draw must be measured in actual center-pixel and show modes.

### M5Stack NeoHEX -- best no-solder WS2812 geometry test

Facts:

- 37 WS2812C-2020 LEDs.
- 36 mm x 36 mm x 9.6 mm.
- 3.7-5.3 V supply.
- HY2.0-4P Grove-style connectors.
- Includes milky diffuser and Grove cable.
- M5Stack lists full-white current around 568 mA at 5 V and 10% white around 207 mA for all 37 LEDs.

Why it is interesting:

- Center-plus-rings geometry may be optically better than a square grid.
- It can test the question: does a ring/hex layout give better chromatic fringing and animation without washing out the gobo?

Risks:

- It is not STEMMA-QT/I2C.
- It likely needs a 5 V or otherwise suitable LED rail; do not assume it will run correctly from PowerFeather `VSQT`.
- The diffuser may help or hurt gobo projection; test diffuser on/off if mechanically possible.

### FeatherS2 Neo -- best all-in-one optical board

Facts:

- ESP32-S2 board with integrated 5x5 RGB matrix.
- Built-in LiPo charging.
- The LED matrix has user-controlled power behavior.
- No external LED daughterboard needed.

Use:

- Fastest optical/firmware prototype.
- LiPo fallback architecture when paired with onboard battery or DFR0559 USB output.
- Not the preferred LiFePO4 architecture.

### M5Stack Atom Matrix -- best tiny fallback system

Facts:

- ESP32-PICO-D4, dual-core 240 MHz.
- 25 WS2812C-2020 LEDs.
- USB-C.
- Grove connector.
- 5 V @ 500 mA input.
- 24 mm square footprint.

Use:

- Ultra-compact fallback lantern brain when powered by DFR0559.
- Excellent for quick range/firmware/optical tests.
- Not the reference for the custom LiFePO4 board.

## Solar manager fallback

### DFRobot DFR0559

Facts:

- CN3165-based solar manager for 5 V panels.
- Charges 3.7 V lithium batteries up to 900 mA.
- Provides 5 V 1 A output.
- Includes protection functions for battery, solar panel, and output.

Use:

- Strong LiPo fallback.
- Very simple assembly: solar input, battery, USB output.

Caveat:

- Not LiFePO4. Do not connect LiFePO4 cells.

## Battery sourcing and format

Current direction:

- Prefer one 18650 LiFePO4 cell per fixture if sourcing is reliable.
- 1500-2000 mAh is plenty for the expected power budget and gives useful autonomy.
- 14430 cells are common and cheap, but 400-450 mAh each. Avoid parallel packs of many 14430 cells unless geometry forces it.
- Consider 26650 only if 18650 sourcing is unreliable and the hat geometry accepts the larger cell.

Why avoid multi-cell 14430 packs:

- More holders or welded tabs.
- More contacts/wires.
- More matching and balancing concerns.
- More assembly operations.
- More failure points.

## Round solar panels

R&D uses square/rectangular 1-5 W panels because they are easy to buy quickly.

Production may prefer round panels aesthetically, but fast ready-to-ship round panels are harder to source. Many round/ETFE panel options appear to be manufacturer/direct-from-China products with longer lead time. Do not block electrical/firmware/enclosure testing on final round-panel sourcing.

Hat design should allow a replaceable top plate or panel recess so R&D panels and production panels can differ.

## Purchased R&D set as of 2026-05-10

Ben bought the current R&D candidates discussed in the survey, except USB power meters, which are already on hand. Purchased/ordered candidates include:

- PowerFeather boards from Elecrow; verify V2 vs V1 on arrival.
- FeatherS2 Neo.
- M5Stack Atom Matrix.
- M5Stack NeoHEX.
- Adafruit IS31FL3741 13x9 matrix.
- DFRobot DFR0559 Solar Power Manager 5V.
- 1-5 W solar panels in square/rectangular form factors.
- LiFePO4 18650 sample cells, if BatterySpace order succeeds.
- LiPo cells/modules for fallback tests.

## Source links

- PowerFeather docs: https://docs.powerfeather.dev/
- Elecrow PowerFeather listing: https://www.elecrow.com/esp32-s3-powerfeather.html
- TI BQ25628E: https://www.ti.com/product/BQ25628E
- Analog MAX17260: https://www.analog.com/en/products/max17260.html
- TI TPS631013: https://www.ti.com/product/TPS631013
- Adafruit IS31FL3741 13x9 matrix: https://www.adafruit.com/product/5201
- Adafruit IS31FL3741 guide: https://learn.adafruit.com/adafruit-is31fl3741
- M5Stack NeoHEX: https://shop.m5stack.com/products/neo-hex-37-rgb-led-board-ws2812
- M5Stack Atom Matrix: https://docs.m5stack.com/en/core/Atom-Matrix_v1.1
- DFRobot DFR0559 wiki: https://wiki.dfrobot.com/Solar_Power_Manager_5V_SKU__DFR0559
- BatterySpace LiFePO4 18650 2000 mAh: https://www.batteryspace.com/LiFePO4-18650-Rechargeable-Cell-3.2V-2000-mAh-6.4Wh-6A-Rate.aspx
