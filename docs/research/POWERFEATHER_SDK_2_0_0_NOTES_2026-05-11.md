# PowerFeather SDK 2.0.0 notes

**Date:** 2026-05-11
**Status:** Research note
**Source:** PowerFeather-SDK GitHub release notes pasted by Ben from <https://github.com/PowerFeather/powerfeather-sdk/releases>

## Why this matters

PowerFeather V2 is the leading COTS candidate for the Resonance Lighting electronics module because it matches the preferred architecture unusually well:

- ESP32-S3-WROOM-1 with onboard PCB antenna.
- Solar/DC input through `VDC`.
- BQ25628E charger / power-path IC.
- LiFePO4 support on V2.
- MAX17260 fuel gauge with current sensing and LiFePO4 profiles.
- TPS631013 buck-boost 3.3 V regulator.
- Controllable `3V3` and `VSQT` rails.
- STEMMA-QT connector for plug-and-play I2C modules.
- Deep sleep / ship / shutdown modes suitable for solar lantern duty cycle.

The SDK 2.0.0 release is a strong positive signal because it adds explicit V2 software support rather than leaving V2 as merely a schematic and product-page claim.

## Release-note items with project impact

### V2 board support

`POWERFEATHER_BOARD_V2` and ESP-IDF Kconfig selection should let Resonance firmware build explicitly for V2. This should avoid silent V1/V2 behavior mismatches.

### MAX17260 support

The release adds support for:

- battery current,
- health,
- cycle count,
- time estimates,
- alarms,
- learned-state restore,
- LiFePO4 mode,
- custom MAX17260 battery profiles.

This is directly useful for BM 2026 telemetry collection and BM 2027 design decisions. Rather than merely estimating daily drain, the system can log real charge/discharge current, state-of-charge trajectory, temperature, and time-to-empty/full behavior under playa sun/shade/dust.

### Shared fuel-gauge abstraction

The shared LC709204F / MAX17260 abstraction may make it easier to support both:

- V1 / LiPo fallback, and
- V2 / LiFePO4 preferred path.

This should feed into the Resonance firmware `PowerTelemetry` abstraction rather than board-specific logic leaking into CA/render code.

### `BatteryType::Generic_LFP`

This directly matches the preferred battery chemistry. First prototype firmware should use Generic_LFP unless a specific battery profile is provided.

### No-battery and custom-profile initialization

`Board.init()` for no-battery operation is useful for bench testing from USB or a supply. `Board.init(const MAX17260::Model&)` matters if the selected LiFePO4 cell has a known custom model/profile.

### Thermistor integration

`updateBatteryFuelGaugeTemp()` can read board thermistor temperature and update the fuel gauge. For Resonance, battery temperature should be logged in all outdoor solar tests and in any sealed-hat thermal tests.

### VSQT behavior fixed for V2

The note that V2 keeps power-management I2C available while `VSQT` is disabled is highly relevant. The likely LED-module path uses `VSQT` to power an external STEMMA-QT LED matrix. Resonance wants to turn that rail off during sleep or failure states while still retaining battery/charger telemetry.

### Charger-setting retention across warm boots

Retaining charger settings across RTC-preserving warm boots reduces the chance that a watchdog reset or light-sleep wake accidentally reverts the charger to an unsafe or suboptimal default. This should be tested rather than assumed.

### Custom profile charge voltage and termination current applied to charger

This is important for LiFePO4. A custom MAX17260 profile is only useful if the charger policy follows the cell chemistry. The release note says custom profiles now apply charge voltage and termination current to the charger.

### Initialization safety and fault handling

The release mentions charger part validation, POR/watchdog recovery, profile-change detection, policy reapplication, MAX17260 LFP configuration fixes, voltage alarm fixes, battery-current reporting edge cases, thermistor sanity checks, and bounded I2C transfer timeouts. These are exactly the failure classes that would be painful in a 100-fixture deployment.

## Firmware implications

- Use ESP-IDF >=5.2 and <=5.5 for the PowerFeather V2 prototype path.
- Do not develop against PowerFeather-SDK 1.x APIs unless intentionally testing V1/LiPo fallback.
- Store battery voltage/current/SOC/temp/health/cycles/time-to-empty/time-to-full in the Resonance telemetry schema from the start.
- Treat `VSQT` as a controlled external-load rail. Default off during sleep/shipping/failure states; explicitly enable for LED module use; verify I2C recovery after power cycling the LED matrix.
- Implement low-battery policies using both voltage and gauge state, but log disagreements between voltage-derived and gauge-derived estimates.
- Test watchdog/POR recovery and confirm charger policy reapplication.

## Test implications

Add to first-arrival PowerFeather V2 validation:

1. Compile and flash a minimal PowerFeather-SDK 2.0.0 example with `POWERFEATHER_BOARD_V2`.
2. I2C scan: verify BQ25628E, MAX17260, TPS631013, and any attached STEMMA-QT matrix.
3. Initialize with `BatteryType::Generic_LFP`.
4. Read battery voltage/current/SOC/temp/health/cycles/time estimates.
5. Read board thermistor and update fuel-gauge temperature.
6. Disable `VSQT`; verify power-management I2C remains available.
7. Re-enable `VSQT`; verify attached LED matrix recovers.
8. Trigger/recover from a warm reset and verify charger settings persist or are re-applied correctly.
9. Test no-battery `Board.init()` from USB/VDC.
10. Simulate missing/open/shorted temperature sensor if practical and verify SDK reports sane errors.
