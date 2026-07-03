# Hardware

Electronics for the Resonance Lighting downlight fixture. Current strategy is
**PowerFeather V2 first, COTS deployable, custom-PCBA optional**, not custom-PCBA-only.

## Current Hardware Tracks

### Track A - COTS Production / Fallback

Use off-the-shelf boards, factory-soldered connector options, pre-crimped cables, screws,
standoffs, and adapter boards to validate and possibly deploy the system without waiting
on a custom PCB.

Current leading stack:

```
Solar panel -> PowerFeather V2 VDC
LiFePO4 cell -> PowerFeather battery JST
PowerFeather GPIO10/A0 -> direct-GPIO LED module
PowerFeather switchable rail / adapter -> LED power
```

Live LED modules, per ADR 0022:

- **SK6812 HEX array:** close-range animation / ambient glow role.
- **4 W RGBW point source:** long-throw crisp gobo role.

Ruled out for the V2 battery build:

- **Adafruit IS31FL3741 13x9 matrix on STEMMA-QT:** shared-bus brownout under WiFi
  (ADR 0018). Keep it only as historical bench context unless a future isolated-bus
  experiment explicitly revives it.

Small adapter-board workstream:

```
PowerFeather GPIO + switchable/boosted LED rail
        |
        v
NeoHEX passive / future boosted adapter PCB
        |
        v
HEX / NeoHEX / RGBW LED module connector
```

See `hardware/led-adapter/neohex-passive-rev-a/`.

### Track B - Custom PCBA / Assembly Optimization

If COTS supply, connector labor, cost, or packaging does not pencil out at 100+ units,
derive a custom board or custom assembly from the validated PowerFeather V2 architecture.

Current reference architecture:

```
[Solar panel / VDC connector]
        |
[Input protection + bright-sun qualification guard]
        |
[BQ25628E-class charger + power path]
        |
[LiFePO4 cell + thermistor]
        |
[MAX17260-class fuel gauge / current sense]
        |
[TPS631013-class 3.3 V buck-boost]
        |
[ESP32-S3-WROOM-class module, PCB antenna]
        |
[Switchable/default-off LED rail]
        |
[Direct-GPIO LED module connector(s)]
```

## Superseded Earlier Target

The older target architecture was:

```
CN3058 charger -> AP2112K LDO -> ESP32-C3-MINI-1 -> WS2812B direct from Vbat
```

That approach is superseded by later ADRs and the PowerFeather V2 findings. CN3058 remains
historical/fallback context only; it is not the leading production direction.

## Why PowerFeather V2 Matters

PowerFeather V2 combines several features the project otherwise would have to design and
validate separately:

- ESP32-S3-WROOM-1 module with onboard PCB antenna.
- BQ25628E charger/power-path with LiFePO4 support.
- MAX17260 fuel gauge with LiFePO4 profile support and current sensing.
- TPS631013 buck-boost 3.3 V regulator.
- Switchable `3V3` header rail and `VSQT` rail.
- Solar/DC input.
- Power telemetry useful for BM 2026 -> BM 2027 learning.

ADR 0021 validates the risky axes: ESP-NOW scale/range, battery-only OTA + rollback, and
the solar charge path. The remaining hardware work is sizing, productionization, thermal,
RF-in-hat, and connector/assembly choices.

## Interfaces

### Direct-GPIO LED

- Current production-facing LED interface.
- Bench default: GPIO10 / A0.
- Requires a switchable/default-off LED rail and local decoupling.
- Supports HEX and RGBW point-source roles with one firmware abstraction.

### STEMMA-QT / Qwiic

- 4-pin JST-SH connector carrying I2C: GND, V+, SDA, SCL.
- Useful for env sensors, ToF, IMU, INA monitors, and other low-power peripherals.
- Do not put the IS31FL3741 matrix back on the PowerFeather V2 shared charger/gauge bus.

### Grove / HY2.0

- Physical connector used by M5Stack modules.
- Can carry I2C, UART, GPIO, analog, or custom signals depending on the device.
- M5Stack NeoHEX uses HY2.0 physically but is a WS2812/GPIO LED device, not I2C.

### USB / Pogo Recovery

USB-C is available on COTS PowerFeather. Any custom board or assembly must preserve a
boring wired recovery path via USB or pogo pads even though OTA is validated.

## Custom PCB Constraints

If a custom board proceeds:

- Use a pre-certified WROOM-class module with onboard PCB antenna by default.
- Do not use u.FL/external antenna unless final hat RF testing fails.
- Include USB/pogo flashing/recovery pads regardless of factory flashing.
- Use a switchable/default-off LED/module rail.
- Include charger/fuel-gauge telemetry.
- Include battery thermistor or another explicit temperature strategy.
- Add keyed solar and battery connectors or production-safe pigtails.
- Avoid direct unstrained panel wires soldered to production board pads.
- Keep LED module/daughterboard separate until optics and placement are frozen.
- Use 4-layer PCB and external schematic/layout review for switching charger,
  buck-boost, current sensing, and RF keep-out.

## Immediate Hardware Tests

Top priorities:

- Run Voltaic P105/P126 harvest tests with BQ25628E VBUS_OVP/HIZ guard in firmware.
- Close bottom-up power budget by LED role.
- Finish HEX 4.2 V boost test and boosted-build current cap.
- RF test inside a mock hat with panel/battery/wiring installed.
- Thermal test sealed hat in sun/heat with charger and LEDs operating.
- Time-trial a production-like COTS assembly with keyed connectors and strain relief.
