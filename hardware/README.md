# Hardware

Electronics for the Resonance Lighting downlight fixture. The hardware workstream now supports two parallel paths:

1. **COTS deployable prototype / production fallback** — off-the-shelf MCU boards, charger boards, LED daughterboards, USB/JST/STEMMA wiring, and mechanical mounting in the hat.
2. **Custom PCBA optimization** — a custom carrier board after the COTS path proves power, firmware, optics, and enclosure requirements.

See ADR 0012.

## Workflow

- COTS prototypes first: measure current, solar behavior, LED optics, RF range, OTA, and mechanical fit before locking the custom board.
- Custom schematic-as-code via atopile where useful; layout in KiCad.
- External review is recommended before ordering production PCBs: schematic, layout, RF keep-out, power routing, USB, charger thermal, JLC/PCBWay assembly, and test pads.

```
hardware/
├── atopile/        custom-PCBA sources, once architecture is proven
├── kicad/          custom-PCBA layout
└── references/     reference schematics, datasheets, COTS board notes
```

## Track A — COTS deployable prototype / fallback

Candidate building blocks:

- MCU board: FeatherS2 Neo, Adafruit ESP32-S3 Feather, Unexpected Maker FeatherS3[D], DFRobot FireBeetle variants, or similar ESP32 board.
- Power board: bq25185 LiFePO4-capable solar charger board for preferred chemistry; DFRobot CN3165 solar manager or board-integrated LiPo charger for LiPo fallback only.
- LED board: integrated 5x5 on FeatherS2 Neo, Adafruit 5x5 NeoPixel BFF as layout reference, or a custom LED daughterboard.
- Wiring: short internal USB cables, JST-PH, JST-SH/STEMMA, pre-crimped harnesses. No hand-crimping at scale.

Allowed:

- Factory-soldered headers.
- Screw-mounted daughterboards / stacked boards.
- USB/JST/STEMMA cables with strain relief.
- Separate power, MCU, and LED boards.

Not allowed:

- Hand-soldering 100 header sets.
- Hand-crimping 100 harnesses.
- Friction-only board stacks.
- Per-unit pairing or configuration.

## Track B — custom PCBA target architecture

The exact custom board is not locked. Current target blocks:

```
[Solar panel 1-3 W]
        ↓
[Battery charger / power-path]
        ↓
[Single-cell battery]
        ↓
[MCU 3V3 regulator] → [pre-certified Espressif module]
        ↓
[Switchable LED rail] → [3x3 or 5x5 LED daughterboard / LED output]
        ↓
[USB-C and/or pogo pads for flashing + recovery]
```

## Module library, after architecture lock

| Module | Purpose | Notes |
|--------|---------|-------|
| `solar_input` | Panel connector, reverse-polarity/input protection | Match actual panel and charger reference |
| `battery_charger` | Single-cell charger, preferred LiFePO4-capable | bq25185-class preferred; CN3058 fallback |
| `power_path` | Load sharing / system output | Prefer proven reference behavior; avoid custom cleverness |
| `voltage_regulator` | MCU rail | Size for selected module and RF bursts |
| `esp32_module` | Pre-certified Espressif module | WROOM/S3/C6/C3 module TBD; no custom RF |
| `led_power` | Switchable LED rail | Default-off hardware state required |
| `led_output` | Data + power connector or on-board LED driver | LED board may stay separate |
| `battery_monitor` | ADC/fuel-gauge/charge telemetry | Include battery, solar, and fault state where possible |
| `test_pads` | Pogo/USB recovery and production test | Required from v1 |

## Hard constraints

- No custom RF. Use pre-certified Espressif modules or proven COTS boards.
- Antenna placement must be mechanically and electrically reserved: board edge, keep-out intact, no solar panel/battery/metal/screws/copper/wiring in the antenna zone.
- LED rail must be switchable and default-off at reset/boot.
- Charger must match battery chemistry. LiPo chargers are never used with LiFePO4 cells.
- USB/pogo flashing is the guaranteed recovery path, even if factory pre-flashing is available.
- COTS and custom designs both need automated smoke-test telemetry.

## Custom PCBA review checklist

Before ordering any custom assembled boards:

- USB-C CC resistors and data path correct.
- Reset/boot/strapping pins correct and not polluted by external circuits.
- Pogo/USB flashing path tested on schematic.
- Battery, solar, and load connectors keyed and labeled.
- Charger configuration matches battery chemistry and conservative charge current.
- Charger thermal path adequate in a sealed hat.
- Battery temperature / hot-charge mitigation addressed.
- LED rail switch defaults off and can be hard-disabled by firmware.
- LED data line has series resistor and safe state during LED power-off.
- LED rail decoupling sized for worst credible all-on burst.
- ESP32 antenna keep-out exactly followed.
- Board fits both COTS fallback enclosure assumptions and custom enclosure assumptions.
- Test pads accessible after partial assembly.
- BOM/CPL orientation manually checked against assembler preview.
