# Firmware Architecture

## Philosophy

Firmware should be modular, testable, and boring where failures are catastrophic. Lighting behavior can be creative; firmware update transport should not be.

Key rules:

- ESP-NOW is for small lighting/control/status packets, not firmware image transport.
- OTA uses standard ESP32 WiFi OTA with A/B partitions and rollback.
- USB-C / pogo flashing is the guaranteed recovery path.
- Firmware must run on multiple board targets: COTS prototypes and custom PCBA.
- LED power rail control is a first-class safety feature.

## Layered structure

```
firmware/
├── core/             ← Platform-independent C++. Compiles native. Unit-tested.
│   ├── ca/           Cellular automata engine. Pure logic.
│   ├── packet/       ESP-NOW packet codec. Pure logic.
│   ├── pattern/      LED animation generators.
│   ├── state/        Per-fixture local-state representation.
│   └── tests/        Native unit tests.
│
├── esp32/            ← ESP32-specific glue. Builds against ESP-IDF or Arduino-ESP32.
│   ├── tasks/        FreeRTOS task definitions.
│   ├── drivers/      LED driver, ESP-NOW, ADC/fuel gauge, charger status, LED rail power.
│   ├── boards/       Pin mappings for COTS and custom boards.
│   └── main.cpp      board init, task creation, watchdog setup.
│
└── tools/            ← Host-side utilities: smoke test, log parser, OTA host scripts.
```

## Board targets

At minimum, support board-specific pin/config headers for:

- TTGO T-Beam / T-Ice bench fixtures where useful.
- FeatherS2 Neo or equivalent 5x5 LED COTS prototype.
- ESP32-S3 Feather / Unexpected Maker FeatherS3[D] or equivalent headroom prototype.
- DFRobot FireBeetle C6/C5 or equivalent solar COTS prototype.
- `resonance_v1` custom PCBA.

Each board definition must describe:

- MCU family and OTA partition assumptions.
- LED data pin.
- LED power-enable pin, if present.
- Battery voltage / fuel-gauge path.
- Solar / charge / fault sense pins, if present.
- Boot/reset/flash behavior.

## Task decomposition (FreeRTOS)

```
ESP-NOW receive callback
        ↓ enqueue only; no heavy work
mesh_rx_queue
        ↓
ca_tick_task / state_task      housekeeping_task
        ↓                      ↓
local_state                    battery, charger, watchdog, OTA, sleep
        ↓
led_render_task → LED rail guard → LED driver

mesh_tx_task → heartbeat / neighbor state / wand events
```

### `led_render_task`

- Highest priority.
- Renders local state to the LED driver.
- Never assumes LED rail is always powered.
- Requests LED rail enable only after battery and fault checks pass.
- Forces black/off frame before disabling LED rail where hardware allows.

### `ca_tick_task`

- Drains ESP-NOW packets from the queue.
- Maintains neighbor table from RSSI / last-heard timestamps.
- Computes local CA / animation state.
- Does not block on I/O.

### `mesh_tx_task`

- Sends low-rate jittered state broadcasts.
- Handles wand-presence events and small TTL control messages.
- Does not send firmware chunks.
- Uses sequence numbers and small fixed-size packets where possible.

### `housekeeping_task`

- Reads battery voltage / fuel gauge.
- Reads charger status, solar-present, and faults where available.
- Manages low-battery LED cutoff and shipping mode.
- Feeds watchdog and records reset reason / brownout count.
- Handles standard OTA maintenance mode.

## ESP-NOW policy

ESP-NOW packet types are allowed for:

- fixture heartbeat and firmware version metadata;
- local lighting state;
- neighbor discovery / RSSI;
- wand presence and TTL-limited interaction events;
- simple maintenance-mode announcements.

ESP-NOW packet types are not allowed for:

- firmware image chunks;
- distributed OTA transport;
- custom reliable file transfer;
- anything whose failure can brick the swarm.

## OTA strategy

- Use standard ESP32 OTA mechanisms only.
- Use A/B partitions with validation and rollback.
- Enter OTA through explicit maintenance mode.
- A local laptop or Pi hosts the firmware image over ordinary WiFi.
- Fixtures connect to a known local AP or controlled WiFi mode for update.
- ESP-NOW may advertise “update available” or “maintenance mode,” but it never carries the image.
- USB-C / pogo flashing remains the recovery path.

## LED rail safety

The LED rail must be treated as a power-controlled peripheral:

- Hardware default is LED power OFF during reset/boot/unprogrammed states.
- Firmware enables LED power only after boot, watchdog setup, and battery sanity check.
- Firmware disables LED power on low battery, fault, shipping mode, or watchdog-recovery conditions.
- A hung MCU with LEDs previously commanded on must not be able to drain the battery indefinitely if hardware watchdog/reset recovery is functioning.
- Test cases must simulate stuck-on LEDs, watchdog reset, low-battery cutoff, and cold boot from depleted battery.

## Boot sequence

```
setup():
  1. Board definition selected at compile time.
  2. Configure LED rail enable to safe OFF state immediately.
  3. Initialize watchdog and reset-reason logging.
  4. Initialize battery/charger telemetry.
  5. If battery is below safe threshold, stay in low-power recovery mode.
  6. Initialize LED driver with LEDs off.
  7. Initialize WiFi STA mode and ESP-NOW for control packets.
  8. Load NVS config: brightness limits, calibration, last mode.
  9. Create FreeRTOS tasks.
 10. Optionally enter OTA maintenance mode if requested.
```

## Reusable from prior projects

- `TalismanPatterns.cpp` can be ported into `core/pattern/`.
- Prior compact packet patterns remain useful, but ESP-NOW framing means old sentinels are unnecessary.
- Marquee OPC tools are useful as development harnesses for visual effects, not production protocol.
- Prior datalog infrastructure is useful for battery/solar telemetry during burn week.

## Build system

- ESP32 target: PlatformIO or ESP-IDF with pinned toolchain versions.
- Native unit tests: CMake + doctest/Catch2 for `firmware/core/`.
- CI: build COTS board targets and custom target; run packet/CA tests.
- Optional Wokwi simulation for ESP-NOW packet behavior, but not a substitute for real RF/enclosure testing.
