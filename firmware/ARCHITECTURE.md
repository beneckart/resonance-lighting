# Firmware Architecture

Firmware runs across COTS prototypes and the eventual production target. The architecture must support multiple board definitions, standard OTA maintenance updates, ESP-NOW state exchange, LED rail fail-safes, and telemetry logging.

## Layered structure

```
firmware/
├── core/             Platform-independent C++. Compiles native. Unit-tested.
│   ├── ca/           Cellular automata engine.
│   ├── packet/       ESP-NOW packet codec.
│   ├── pattern/      LED animation generators.
│   ├── state/        Fixture state representation.
│   └── tests/        Native tests.
│
├── esp32/            ESP32-specific glue.
│   ├── boards/       Board definitions: powerfeather_v2, feathers2_neo, atom_matrix, resonance_custom.
│   ├── drivers/      ESP-NOW, OTA, LED drivers, charger/fuel gauge, rail control.
│   ├── tasks/        FreeRTOS tasks.
│   └── main.cpp
│
└── tools/            Host-side smoke test, log parser, telemetry tools.
```

## Board abstraction

Each board definition should specify:

- MCU type / build flags.
- LED driver type: IS31FL3741 I2C, WS2812/NeoPixelBus, integrated matrix, or no LED.
- LED/module rail control: `VSQT`, onboard LDO enable, external load switch, or always-on fallback.
- Charger/fuel-gauge devices available.
- Battery chemistry support.
- Standard OTA support.
- ESP-NOW radio settings.
- Sleep capabilities.

Initial board targets:

- `powerfeather_v2` — ESP32-S3-WROOM, BQ25628E, MAX17260, TPS631013, switchable VSQT.
- `feathers2_neo` — ESP32-S2 with integrated 5x5 matrix and LiPo charging.
- `atom_matrix` — ESP32-PICO-D4 with integrated 5x5 WS2812C.
- `resonance_custom` — future PowerFeather-derived custom board.

## OTA policy

OTA must remain boring and standard.

- Use normal ESP32 OTA partition/rollback mechanisms.
- Enter OTA in a deliberate maintenance mode.
- A laptop/Pi/local AP may host firmware.
- USB/pogo flashing remains the recovery path.
- ESP-NOW may advertise firmware version or maintenance availability.
- ESP-NOW must **not** carry firmware image chunks.
- No mesh-gossiped OTA image distribution.

## ESP-NOW policy

ESP-NOW is for lightweight fixture state and control only:

- heartbeat / boot announcement,
- fixture state,
- battery summary,
- neighbor RSSI / last-heard,
- global mode hints,
- wand/proximity events,
- optional maintenance-mode announcement.

Implementation notes:

- Add jitter to periodic sends.
- Use sequence numbers.
- Keep packets small and fixed-format where practical.
- Treat RSSI as approximate topology signal, not exact distance.
- Keep lighting functional even with partial packet loss.

## Task decomposition

```
led_task
  - render current local state to selected LED driver
  - enforce brightness/current caps
  - never block on network or file IO

ca_tick_task
  - drain mesh_rx_queue
  - update neighbor table
  - compute local state
  - enqueue state updates

mesh_rx_callback
  - enqueue received packets only

mesh_tx_task
  - heartbeat/state broadcasts
  - wand/control events
  - low-rate telemetry summary if enabled

power_task
  - read battery/charger/fuel gauge
  - manage LED/module rails
  - low-battery state machine
  - sleep/shipping mode transitions

telemetry_task
  - log power/solar/battery/RF/reset data
  - expose smoke-test status
  - throttle flash writes

ota_task
  - only runs in maintenance mode
  - standard OTA update / rollback
```

## LED drivers

### IS31FL3741 matrix

- I2C LED matrix over STEMMA-QT.
- Used by Adafruit 13x9 matrix.
- Must support power cycling `VSQT`, reinitialization, and low-current modes.
- Test for PWM/multiplex artifacts in gobo projection.

### WS2812 / NeoPixelBus

- Used by NeoHEX, Atom Matrix, FeatherS2 Neo, and possible custom LED boards.
- Use DMA-capable driver where supported.
- Include brightness/current caps.
- Include data-line safe state before powering rails off.

## Power / telemetry subsystem

Power telemetry is now a core firmware feature.

For PowerFeather V2/custom PowerFeather-like boards, log:

- battery voltage,
- battery current,
- SOC / remaining capacity,
- battery temperature,
- charger state,
- solar/VDC input behavior,
- charge current,
- fault flags,
- estimated time-to-empty/full,
- LED rail state,
- reset reason / brownout count.

For simpler fallback boards, log whatever is available and measure externally during bench tests.

## LED rail fail-safe

The firmware must assume LEDs can be left on by a bad state if not actively managed.

Required behavior:

- External LED/module rails default off at boot if hardware allows.
- Firmware enables LEDs only after boot sanity checks.
- Low battery turns LED rails off before brownout loop.
- Watchdog reset reinitializes LED rails safely.
- Shipping mode turns external LED rails off.
- LED rail off-state leakage is measured for each COTS stack.

## Sleep policy

Active-show mode may use light sleep or low-duty loops. Daytime/idle/shipping modes should use deeper sleep when possible.

For each board, measure actual current rather than trusting nominal specs:

- MCU-only sleep,
- sleep with LED module attached and rail off,
- sleep with charger/fuel gauge active,
- sleep with solar input present,
- wake latency and state restoration.

## Smoke-test boot announcement

Every fixture should report at boot:

- MAC-derived fixture ID,
- board type,
- firmware version,
- reset reason,
- battery voltage/SOC if available,
- charger/fault state if available,
- LED module detected,
- LED rail state,
- mesh peer count after a short delay.

This supports production QA and field debugging.
