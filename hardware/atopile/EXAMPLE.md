# atopile module example: voltage_regulator

This is a sketch showing what one of our atopile modules will look like. It's not yet buildable (no atopile project file) -- it exists to make the pattern concrete for whoever picks up the schematic-as-code workstream next.

## What atopile is

[atopile](https://atopile.io) lets you express PCB schematics as code. You define modules (like classes) with typed power/signal interfaces, instantiate them in a top-level project, and the toolchain generates KiCad schematics + DRC checks.

For our purposes: it's the schematic equivalent of writing C++ instead of laying out circuits visually in KiCad.

## Module: `voltage_regulator.ato`

The 3.3 V LDO module wraps an AP2112K-3.3 with required input/output decoupling. Used by the carrier-board top-level project.

```ato
import Power from "generics/interfaces.ato"
import Capacitor from "generics/capacitors.ato"

# AP2112K-3.3V LDO regulator. ~600 mA, 450 mV dropout.
# Input range: 2.5-6V. Output: 3.3V regulated.
component AP2112K_3v3:
    package = "SOT-25-5"
    mpn = "AP2112K-3.3TRG1"  # Diodes Inc.
    # Pinout per datasheet
    signal Vin ~ pin 1
    signal GND ~ pin 2
    signal EN ~ pin 3
    # pin 4 NC
    signal Vout ~ pin 5

# Module: 3V3 regulator block with decoupling
module VoltageRegulator3v3:
    # Public interfaces
    power_in = new Power
    power_out = new Power

    # Internals
    u1 = new AP2112K_3v3
    cin = new Capacitor
    cout = new Capacitor

    cin.value = 1uF
    cin.package = "0402"
    cout.value = 1uF
    cout.package = "0402"

    # Wiring
    u1.Vin ~ power_in.vcc
    u1.GND ~ power_in.gnd
    u1.GND ~ power_out.gnd
    u1.Vout ~ power_out.vcc
    u1.EN ~ power_in.vcc  # always-on; tie EN to Vin

    # Decoupling
    cin.p1 ~ power_in.vcc
    cin.p2 ~ power_in.gnd
    cout.p1 ~ power_out.vcc
    cout.p2 ~ power_out.gnd

    # Constraints (atopile checks these)
    assert power_in.voltage within 2.5V to 6V
    assert power_out.voltage is 3.3V +/- 1%
```

## What this looks like in the top project

```ato
import VoltageRegulator3v3 from "modules/voltage_regulator.ato"
import LiFePO4Charger from "modules/lifepo4_charger.ato"
import ESP32C3Mini from "modules/esp32_module.ato"

module ResonanceCarrier:
    # Top-level instantiation
    charger = new LiFePO4Charger
    regulator = new VoltageRegulator3v3
    mcu = new ESP32C3Mini

    # Connect: charger output -> regulator input -> MCU power
    charger.power_out ~ regulator.power_in
    regulator.power_out ~ mcu.power
```

## Why this pattern

- **Reviewable.** Diffs in PRs show what changed at the schematic level, not as gerber pixels.
- **AI-pair-programmable.** Claude can read and propose changes to atopile code the same way it does Python or C++.
- **Reusable.** `VoltageRegulator3v3` is a building block. Use it again in 2027 or in a different project. Standard library of "things we know work."
- **Constraint-checked.** atopile catches "you connected a 5 V output to a 3.3 V input" at compile time.
- **Exports to KiCad.** Layout still happens in KiCad, but with auto-generated schematics that are clean by construction.

## Modules to build for Resonance (planned, see `TODO.md`)

| Module | Purpose | Wraps |
|--------|---------|-------|
| `solar_input.ato` | Panel connector + protection | JST-PH, Schottky, TVS, ferrite |
| `lifepo4_charger.ato` | LiFePO4 charging | CN3058 + Rprog + status LED |
| `power_path.ato` | Solar/battery load sharing | P-MOSFET ideal-diode |
| `voltage_regulator.ato` | 3V3 rail | AP2112K-3.3 + caps (this file) |
| `esp32_module.ato` | MCU + USB-C + boot/reset | ESP32-C3-MINI-1 + strapping pin pull-ups + USB-C connector |
| `led_output.ato` | WS2812B chain output | JST-PH 3-pin + decoupling cap |
| `battery_monitor.ato` | Voltage divider for ADC | 2 resistors + gate MOSFET |

Once each module is built and tested in isolation, the `ResonanceCarrier` top is a small composition file.

## Resources

- atopile docs: https://docs.atopile.io
- atopile GitHub: https://github.com/atopile/atopile
- The kicad-happy AI-agent skills for Claude Code working in KiCad: https://github.com/aklofas/kicad-happy
