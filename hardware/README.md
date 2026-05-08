# Hardware

Carrier PCB for the Resonance Lighting downlight fixture.

## Workflow

Schematic-as-code via [atopile](https://atopile.io). Layout in [KiCad](https://www.kicad.org). AI pair-programming (Claude) for module review and gotcha-checking.

```
hardware/
├── atopile/        atopile sources (.ato). Schematic lives here.
├── kicad/          KiCad project for layout. Generated from atopile, then hand-laid.
└── references/     Reference schematics and datasheets we lifted from.
```

## Architecture (target)

```
[Solar panel ~1-5W] → [Reverse-polarity protection]
                          ↓
                 [CN3058 LiFePO4 charger] ←→ [LiFePO4 cell]
                          ↓
                  [Power-path / load-sharing]
                          ↓
                 [AP2112K-3.3 LDO] → 3.3V rail → [ESP32-C3-MINI-1]
                          ↓
                    Battery rail → [WS2812B chain] (1-9 LEDs)
                          ↓
                    USB-C → [Programming + USB-charge fallback]
```

Detail in `docs/block-diagram/` (next deliverable).

## Module library (planned)

Each is a reusable atopile module with typed power/signal interfaces:

| Module | Purpose | Key parts |
|--------|---------|-----------|
| `solar_input` | Panel connector, reverse-polarity protection, input filter | JST-PH connector, Schottky diode, ferrite bead |
| `lifepo4_charger` | Charge a single LiFePO4 cell from solar input | CN3058, status LED, Rprog resistor |
| `power_path` | Cleanly switch between solar-direct and battery | P-MOSFET ideal-diode or charger-internal |
| `voltage_regulator` | 3.3V rail for ESP32 | AP2112K-3.3 + decoupling |
| `esp32_module` | The MCU and its required passives + USB | ESP32-C3-MINI-1, USB-C, reset/boot, strapping pins |
| `led_output` | WS2812B chain output | JST-PH connector, optional level-shifter footprint |
| `battery_monitor` | ADC voltage divider for battery sense | 2 resistors, optional MOSFET to gate the divider |

## Constraints (baked in to all design decisions)

- **O(1) per-fixture operations.** This is a top-tier constraint — see ADR 0009. Every step in the per-fixture pipeline must be O(1) human time. No soldering on receipt. No per-unit configuration. Same firmware image for every fixture; per-unit identity from the chip's MAC. Pre-flash at the fab if available; otherwise pogo-pin jig.
- **All SMT, all in JLCPCB Basic or Extended parts library.** No through-hole. No hand-soldered parts. Verify each part's JLCPCB stock before committing.
- **No headers anywhere except optional debug pads on a single dev board.** No socketed dev modules. The ESP32-C3-MINI-1 is reflow-soldered to the carrier.
- **LiFePO4-tuned charge profile.** 3.6 V max charge voltage, ~3.0 V discharge cutoff. Do not use LiPo-tuned chargers (TP4056, bq24074, CN3791).
- **Direct-from-battery LED power.** WS2812B's data threshold is 0.7 × Vcc. With LiFePO4 max 3.6 V battery and 3.3 V GPIO data, 3.3 ≥ 0.7 × 3.6 = 2.52 → comfortable margin. No level shifter needed (verified on Talisman v2 with similar math).
- **Designed for 100-unit JLCPCB SMT assembly.** Every BOM line goes to the assembler.
