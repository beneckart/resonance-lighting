# Firmware Architecture

## Layered structure

```
firmware/
├── core/             ← Platform-independent C++. Compiles native. Unit-tested.
│   ├── ca/           Cellular automata engine. Pure logic.
│   ├── packet/       Mesh packet codec. Pure logic.
│   ├── pattern/      LED animation generators (port of TalismanPatterns).
│   ├── state/        Per-fixture local-state representation.
│   └── tests/        Native unit tests using doctest or Catch2.
│
├── esp32/            ← ESP32-specific glue. Builds against ESP-IDF or Arduino-ESP32.
│   ├── tasks/        FreeRTOS task definitions.
│   ├── drivers/      NeoPixelBus, ESP-NOW, ADC, button.
│   ├── boards/       Pin mappings: ttgo_t_beam.h, ttgo_t_ice.h, resonance_v1.h.
│   └── main.cpp      setup(), task creation, board init.
│
└── tools/            ← Host-side utilities (OPC client for bench-streaming, log parser, etc.)
```

The split exists to make the algorithmic parts (CA, packet codec, animations) test-driveable on a laptop in milliseconds, while the platform-tied parts (drivers, RTOS tasks, sleep modes) only need to run on hardware or in Wokwi.

## Task decomposition (FreeRTOS)

```
                    ┌──────────────────┐
                    │   ESP-NOW radio  │
                    │   callback (ISR) │
                    └─────────┬────────┘
                              │ enqueue raw packet
                              ▼
                    ┌──────────────────┐
                    │  mesh_rx_queue   │ (FreeRTOS Queue)
                    └─────────┬────────┘
                              │
              ┌───────────────┴────────────────┐
              ▼                                ▼
   ┌─────────────────────┐         ┌─────────────────────┐
   │   ca_tick_task      │         │  housekeeping_task  │
   │   priority 3        │         │   priority 1        │
   │   period 100 ms     │         │   period 5000 ms    │
   │                     │         │                     │
   │ - drain rx queue    │         │ - read battery ADC  │
   │ - update neighbors  │         │ - check OTA         │
   │ - compute CA next   │         │ - manage sleep      │
   │ - update local_state│         │ - log events        │
   │ - trigger tx        │         │                     │
   └─────────┬───────────┘         └─────────────────────┘
             │ atomic writes
             ▼
   ┌─────────────────────┐
   │  local_state struct │ (shared, atomic-write-safe)
   │  - device_id        │
   │  - color, brightness│
   │  - mode             │
   │  - timestamp        │
   └─────────┬───────────┘
             │ read
             ▼
   ┌─────────────────────┐
   │  led_render_task    │
   │  priority 5 (high)  │
   │  period 16 ms (60Hz)│
   │                     │
   │ - render(local_state│
   │   → frame buffer)   │
   │ - NeoPixelBus.Show()│
   │   (DMA, ~120 µs CPU)│
   └─────────────────────┘

   ┌─────────────────────┐
   │  mesh_tx_task       │
   │  priority 3         │
   │  period 1000 ms +   │
   │  event-driven       │
   │                     │
   │ - heartbeat         │
   │ - wand presence     │
   │ - state broadcasts  │
   └─────────────────────┘
```

## Task priority rationale

- **`led_render_task` (priority 5):** highest. LED frame timing is what users see; nothing should preempt it. With FreeRTOS preemption + I2S DMA hardware actually transmitting the LED signal, the CPU work per frame is ~120 µs of setup. Even at 60 Hz that's <1% CPU.
- **`ca_tick_task` (priority 3):** medium. Computes next state at 10 Hz. Reads neighbor packets from the queue (drains everything pending), updates local state. CPU work per tick is microseconds for 4 LEDs and ~10 neighbors.
- **`mesh_tx_task` (priority 3):** medium. Periodic broadcasts every 1 s plus event-driven sends (e.g. wand-presence detected). ESP-NOW send completes in ~5 ms.
- **`housekeeping_task` (priority 1):** lowest. Battery checks, OTA polls, sleep manager, logging. None of these are time-critical.

## Inter-task communication

- **`mesh_rx_queue`:** FreeRTOS queue. ESP-NOW callback enqueues raw packets (using ISR-safe `xQueueSendFromISR`). `ca_tick_task` drains.
- **`local_state` struct:** small (under 32 bytes), accessed atomically by `ca_tick_task` (writer) and `led_render_task` (reader). Use `std::atomic` or manual memory barriers — atomic write of an aligned 32-bit field is naturally consistent on RISC-V.
- **`neighbor_table` struct:** larger, owned by `ca_tick_task`. `housekeeping_task` reads for diagnostics. Lock with a mutex if needed; usually contention is rare.

## Sleep behavior

- Between LED frames (16 ms period, ~120 µs work): housekeeping task can call `esp_light_sleep_start()` if no broadcasts are pending. Wakes ~50 µs before the next frame for `led_render_task` to fire.
- Between mesh ticks (100 ms period): same pattern.
- Long sleep (deep sleep) is not used in active mode — would lose mesh sync. Only used in "shipping mode" between BM events.

## Boot sequence

```
setup() (Arduino-ESP32 entry point):
  1. Read board.h pin definitions (which TTGO / production board am I?)
  2. Init NeoPixelBus (clear LEDs)
  3. Init NVS (load device_id, calibration, last-mode)
  4. Init WiFi STA mode (no AP), then init ESP-NOW
  5. Register ESP-NOW receive callback
  6. Create FreeRTOS tasks: led_render, ca_tick, mesh_tx, housekeeping
  7. Delete the default loop() task (xTaskDelete(NULL))
```

## Reusable from `beneckart/future-robotics`

- `Talisman/arduino-sketches/talisman_v2rev2/TalismanPatterns.cpp` — drop into `core/pattern/`. Already templated on NeoPixelBus color feature and method.
- `talisman_v2rev2/Switch.cpp` and `Switch.h` — debounced button class. Drop in if we add a button to the production hat.
- The 11-byte sentinel-bracketed packet pattern. ESP-NOW gives us framing for free, so drop the 0xDEAD/0xBEEF sentinels but keep the "tiny structured payload" discipline.
- Marquee Python OPC clients (in `Marquee/python/`) — useful as a host-side animation development harness once we want to stream test patterns to a single bench fixture over WiFi (development only, not part of production firmware).

## Build system

- ESP32 target: PlatformIO (handles ESP-IDF + Arduino-ESP32 dual mode cleanly). `platformio.ini` pinning toolchain version.
- Native unit tests: CMake + doctest. Compiles `firmware/core/` only, links with stub headers for any platform types it touches.
- CI: GitHub Actions running both `pio run` and `cmake --build` + tests on every PR. Wokwi CI integration added later for full-firmware integration tests.

## OTA strategy

- A/B partition scheme using ESP32-C3's OTA partitions (default 4 MB flash splits cleanly).
- "OTA available" announcements piggyback on the mesh broadcast: any node can carry a version flag and a small "I have firmware v.X" indicator.
- Source of truth at deploy time: a laptop or Pi at the base of the tree on the same WiFi as one designated "bridge" fixture. Bridge serves the OTA image to peers; peers gossip it forward through the mesh.
- Validate on bench with 2–3 ESP32 nodes before trusting the protocol on 100.
