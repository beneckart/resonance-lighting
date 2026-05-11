## 2026-05-11 — Ben + GPT — PowerFeather SDK 2.0.0 release confirms V2 support path

PowerFeather-SDK 2.0.0 was released shortly after the PowerFeather V2 hardware/schematic review. This is a strong positive signal that the PowerFeather developer is active and that V2 is far enough along to have first-class software support.

Key release-note items relevant to Resonance Lighting:

- Adds PowerFeather V2 board support selectable through ESP-IDF Kconfig or `POWERFEATHER_BOARD_V2`.
- Adds MAX17260 fuel-gauge support, including battery current, health, cycles, time estimates, alarms, learned-state restore, LiFePO4 mode, and custom MAX17260 battery profiles.
- Adds a shared fuel-gauge abstraction for LC709204F and MAX17260, which should let Resonance firmware support V1/LiPo fallback and V2/LiFePO4 paths behind one interface.
- Adds `BatteryType::Generic_LFP`, directly matching the project's preferred LiFePO4 chemistry.
- Adds `Board.init()` for no-battery operation and `Board.init(const MAX17260::Model&)` for custom battery profiles.
- Adds `updateBatteryFuelGaugeTemp()` overload that reads the board thermistor and updates the fuel gauge.
- V2 keeps the power-management I2C bus available while `VSQT` is disabled. This matters because Resonance wants to turn off external LED modules / STEMMA-QT loads while preserving housekeeping telemetry.
- Charger settings can be retained across RTC-preserving warm boots when battery/profile configuration still matches.
- Custom profiles now apply profile charge voltage and termination current to the charger.
- Initialization safety was improved: charger part validation, POR/watchdog recovery, profile-change detection, and full policy reapplication.
- MAX17260 LFP configuration, profile loading, learned-parameter handling, voltage alarms, and fuel-gauge reinitialization were fixed.
- Missing/open/shorted battery temperature sensors now get sanity checks.
- I2C fault latency was reduced with bounded transfer timeouts and the newer ESP-IDF I2C master driver.
- ESP-IDF requirement is now >=5.2, <=5.5.

Interpretation:

PowerFeather V2 is no longer just an attractive schematic. It now has explicit SDK support for the exact features Resonance cares about: LiFePO4 fuel-gauge mode, MAX17260 telemetry, thermistor integration, custom profiles, power-domain behavior with `VSQT` off, and improved recovery from charger/gauge initialization edge cases.

Action:

- Treat PowerFeather V2 + PowerFeather-SDK 2.x as the primary COTS LiFePO4 prototype path.
- On first hardware arrival, verify the boards are truly V2 by visual chip ID and I2C scan.
- Build first firmware with ESP-IDF >=5.2 and PowerFeather-SDK 2.x, not the older 1.x docs/examples.
- Add a small compatibility layer in Resonance firmware so PowerFeather telemetry can be consumed by the normal battery/power telemetry interface.
- Capture telemetry from BM 2026 fixtures if this platform or a PowerFeather-derived custom board is used; this data should inform BM 2027 solar/battery sizing.

Open questions:

- Does the Elecrow stock currently shipping as "ESP32-S3 PowerFeather V2" contain V2 hardware, or could it be V1 stock/listing ambiguity?
- Will the developer share V2 KiCad layout files, or only schematic/3D model?
- How well has V2 been tested with actual LiFePO4 cells under solar/VDC input?
- Does the SDK expose enough raw charger/fuel-gauge telemetry for long-term logging without significant custom driver work?
