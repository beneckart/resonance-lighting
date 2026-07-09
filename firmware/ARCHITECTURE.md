# Firmware Architecture

**Status (2026-07-08):** this is the TARGET production architecture. Current code is
standalone bench sketches under `firmware/<app>/` (see `firmware/README.md`);
`net_bench` is the closest ancestor of production firmware. Constraints learned on
the bench that bind this design: ADR 0028 (power-management bus integrity), ADR 0029
(LED drive per role), ADR 0023 (low-battery thresholds), ADR 0027 (sensors).

Firmware runs across COTS prototypes and the eventual production target. The architecture must support multiple board definitions, standard OTA maintenance updates, ESP-NOW state exchange, LED rail fail-safes, and telemetry logging.

## Layered structure

```
firmware/
|-- core/             Platform-independent C++. Compiles native. Unit-tested.
|   |-- ca/           Cellular automata engine.
|   |-- packet/       ESP-NOW packet codec.
|   |-- pattern/      LED animation generators.
|   |-- state/        Fixture state representation.
|   `-- tests/        Native tests.
|
|-- esp32/            ESP32-specific glue.
|   |-- boards/       Board definitions: powerfeather_v2, resonance_custom.
|   |-- drivers/      ESP-NOW, OTA, LED drivers, charger/fuel gauge, rail control.
|   |-- tasks/        FreeRTOS tasks.
|   `-- main.cpp
|
`-- tools/            Host-side smoke test, log parser, telemetry tools.
```

## Board abstraction

Each board definition should specify:

- MCU type / build flags.
- LED driver type: WS2812/SK6812 via NeoPixelBus, 4 W RGBW point source, or no LED
  (direct-GPIO only -- I2C LED controllers are ruled out, ADR 0018/0028).
- LED/module rail control: `VSQT`, onboard LDO enable, external load switch, or always-on fallback.
- Charger/fuel-gauge devices available.
- Battery chemistry support.
- Standard OTA support.
- ESP-NOW radio settings.
- Sleep capabilities.

Board targets:

- `powerfeather_v2` -- ESP32-S3-WROOM, BQ25628E, MAX17260, TPS631013, switchable
  VSQT. The 2026 production board (ADR 0024).
- `resonance_custom` -- possible 2027 PowerFeather-derived custom board (dedicated
  power-management I2C bus per ADR 0028).

(The COTS bake-off boards `feathers2_neo` / `atom_matrix` are retired -- ADR 0016
annotation.)

## OTA policy

OTA must remain boring and standard.

- Use normal ESP32 OTA partition/rollback mechanisms.
- Enter OTA in a deliberate maintenance mode: an ESP-NOW metadata packet sends the
  fixture(s) onto shared WiFi (portable router), where each serves `/update`; the
  host uploads in parallel (`ops/bench/net_bench_ota.py`). This is the fleet
  default; the self-hosted-AP fallback is deprecated (AGENTS.md bench gotchas).
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
  - low-battery state machine (ADR 0023 tiers: dim/off/sleep, hysteresis,
    coulomb-primary voltage-backstop)
  - sleep/shipping mode transitions
  - NEVER pinned to core 0 while WiFi is active, and the shared charger/gauge
    bus stays at 100 kHz (ADR 0028)

sensor_task
  - MSA311 sway/tilt + multizone ToF presence (per class -- ADR 0027)
  - short per-frame reads sized to the 100 kHz bus budget
  - feeds choreography state into ca_tick_task

telemetry_task
  - log power/solar/battery/RF/reset data
  - expose smoke-test status
  - throttle flash writes

ota_task
  - only runs in maintenance mode
  - standard OTA update / rollback
```

## LED drivers

### IS31FL3741 matrix -- RULED OUT

Rejected for the battery build (ADR 0018): it disturbs the shared charger/gauge I2C
bus and browns out the board under WiFi. Nothing optional rides the power-management
bus (ADR 0028). Kept here only so nobody re-adds it.

### WS2812 / SK6812 via NeoPixelBus (HEX role)

- SK6812 HEX on the switchable 3V3 rail (the rail is its kill switch -- ADR 0029).
- Use DMA-capable driver where supported.
- Include brightness/current caps.
- Include data-line safe state + explicit all-off before powering rails off.

### 4 W RGBW point source (gobo role)

- Currently fed from the switchable 3V3 rail (V+/GND/A0 via right-angle JST-XH);
  the VBAT-direct option (+33 % fringed white) is OPEN -- taking it means a pin
  move (A0 -> D13), a fail-safe redesign (rail shutoff no longer kills the LED),
  and tapping downstream of the gauge shunt (ADR 0029).
- Single-emitter PWM drive; supports the swept/single-pixel gobo modes.

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
