# 0005 -- Firmware as FreeRTOS tasks, not Arduino loop()

**Date:** 2026-05-06
**Status:** Accepted, updated 2026-05-08 to remove ESP32-C3-specific rationale
**Owners:** Ben

## Context

Prior project firmware used Arduino's single-`loop()` "bag of timers" pattern. That worked, but LED animations can stutter when blocking I/O shares one loop with radio, storage, serial, or sensor operations.

Resonance has concurrent concerns: LED rendering, ESP-NOW receive callbacks, periodic state broadcasts, cellular automata tick, battery/charger monitoring, standard OTA maintenance mode, sleep/shipping mode, watchdog logging, and smoke-test telemetry.

## Options considered

- **Arduino loop() with timer gates:** simple but fragile under blocking I/O.
- **Ad hoc async callbacks:** easy to start, hard to reason about at field scale.
- **FreeRTOS tasks with priorities and queues:** clean concurrent decomposition; already the underlying runtime for ESP-IDF and Arduino-ESP32.

## Decision

Use FreeRTOS tasks with priorities and queues.

| Task | Priority | Period | Job |
|------|----------|--------|-----|
| `led_render_task` | High | 30-60 Hz fixed | Render local state, manage LED rail, call LED driver |
| `ca_tick_task` / `state_task` | Medium | 5-10 Hz | Drain neighbor-state queue, compute next local state |
| `mesh_tx_task` | Medium | Low-rate + event-driven | Heartbeat, state broadcasts, wand events |
| `mesh_rx_callback` | WiFi task callback | event-driven | Enqueue packet only; no heavy work |
| `housekeeping_task` | Low | 0.2-1 Hz | Battery/charger telemetry, watchdog, reset logging, sleep, OTA maintenance |

## Consequences

- Platform-independent code lives in `firmware/core/` and is tested natively.
- Board-specific code lives in `firmware/esp32/boards/` so COTS and custom targets can share core firmware.
- LED rail enable/disable is a first-class firmware responsibility, not an afterthought.
- OTA maintenance mode is part of housekeeping, but firmware images are never sent through ESP-NOW.
- The architecture works on single-core and dual-core ESP32 variants. If a dual-core ESP32-S3 is selected, tasks may later be pinned, but that is not required for the initial design.
