# Hardware

Electronics for the Resonance Lighting downlight fixture. Current strategy is **COTS-first with custom-PCBA fallback/optimization**, not custom-PCBA-only.

## Current hardware tracks

### Track A — COTS production/fallback

Use off-the-shelf boards to validate and possibly deploy the system without waiting on a custom PCB.

Current leading stack:

```
Solar panel → PowerFeather V2 VDC
LiFePO4 cell → PowerFeather battery JST
PowerFeather STEMMA QT / VSQT → Adafruit IS31FL3741 13x9 LED matrix
```

Alternate stacks:

```
PowerFeather V2 GPIO + suitable LED rail → M5Stack NeoHEX
DFRobot DFR0559 USB output → FeatherS2 Neo
DFRobot DFR0559 USB output → M5Stack Atom Matrix
```

### Track B — custom PCBA

If the COTS tests justify a bespoke board, the custom board should be derived from the successful COTS/reference architecture, currently PowerFeather V2.

Current target reference architecture:

```
[Solar panel / VDC connector]
        ↓
[Input protection / diode or ideal-diode input handling]
        ↓
[BQ25628E-class charger + power path]
        ↓
[LiFePO4 cell + thermistor]
        ↓
[MAX17260-class fuel gauge / current sense]
        ↓
[TPS631013-class 3.3 V buck-boost]
        ↓
[ESP32-S3-WROOM-class module, PCB antenna]
        ↓
[Switched external LED/STEMMA rail]
        ↓
[LED module connector(s)]
```

## Superseded earlier target

The older target architecture was:

```
CN3058 charger → AP2112K LDO → ESP32-C3-MINI-1 → WS2812B direct from Vbat
```

That approach is now superseded by later ADRs and the PowerFeather V2 findings. CN3058 may remain a backup charger concept, but it is not the leading production direction.

## Why PowerFeather V2 matters

PowerFeather V2 combines several features that the project otherwise would have to design and validate separately:

- ESP32-S3-WROOM-1 module with onboard PCB antenna.
- BQ25628E charger/power-path with LiFePO4 support.
- MAX17260 fuel gauge with LiFePO4 profile support and current sensing.
- TPS631013 buck-boost 3.3 V regulator.
- Switchable `VSQT` rail for STEMMA-QT modules.
- Solar/DC input.
- Power telemetry useful for BM 2026 → BM 2027 learning.

On arrival, Elecrow boards must be identified as V1 vs V2 before LiFePO4 testing.

## Interfaces

### STEMMA-QT / Qwiic

- 4-pin JST-SH connector.
- I2C: GND, V+, SDA, SCL.
- PowerFeather exposes a switchable `VSQT` rail.
- Adafruit IS31FL3741 13x9 matrix is the primary LED module on this interface.

### Grove / HY2.0

- Physical connector used by M5Stack modules.
- Can carry I2C, UART, GPIO, analog, or custom signals depending on device.
- M5Stack NeoHEX uses HY2.0 physically but is a WS2812/GPIO LED device, not an I2C/STEMMA-QT device.

### USB power fallback

DFRobot DFR0559 can own LiPo battery/solar management and power FeatherS2 Neo or Atom Matrix over USB. In that configuration, the downstream board's battery connector should stay empty.

## Custom PCB constraints

If a custom board proceeds:

- Use a pre-certified WROOM-class module with onboard PCB antenna by default.
- Do not use u.FL/external antenna unless final hat RF testing fails.
- Include USB/pogo flashing/recovery pads regardless of factory flashing.
- Use a switchable/default-off LED/module rail.
- Include charger/fuel-gauge telemetry.
- Include battery thermistor or temperature strategy.
- Add keyed solar and battery connectors or production-safe pigtails.
- Avoid direct unstrained panel wires soldered to production board pads.
- Keep LED module/daughterboard separate until optics are frozen.
- Use 4-layer PCB and external schematic/layout review for switching charger, buck-boost, current sensing, and RF keep-out.

## Immediate hardware tests

See `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`.

Top priorities:

- Confirm PowerFeather V2 identity.
- Verify LiFePO4 configuration and charging.
- Measure sleep current with LED modules attached and powered off.
- Test IS31FL3741 and NeoHEX optics through the gobo.
- Test RF in a mock hat.
- Test low-battery/stuck-on-LED fail-safes.
