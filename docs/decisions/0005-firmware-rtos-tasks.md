# 0005 — Firmware as FreeRTOS tasks, not Arduino loop()

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben

## Context

Prior project (Talisman v2, 2018) used Arduino's single-`loop()` "bag of timers" pattern: every periodic task gated by `if (millis() - last_X >= DELAY_X) { do_thing(); }`. Worked, but LED animations stuttered when blocking I/O (SPIFFS writes, GPS Serial reads, LoRa transmits) ran in the same loop.

Resonance has even more concurrent concerns: LED render at 60 Hz, ESP-NOW mesh receive callbacks, periodic state broadcasts, cellular automata tick, battery monitoring, OTA polling, sleep manager. Bag-of-timers will not scale to this cleanly.

## Options considered

- **Arduino loop() with timer gates** (status quo): simple but fragile under blocking I/O.
- **Pin LED render to one core, everything else to the other** (works on dual-core ESP32 only): solves stutter but locks us to dual-core (more power, more $).
- **FreeRTOS tasks with priorities and queues** (what ESP-IDF and Arduino-ESP32 actually run on, just usually hidden): clean concurrent decomposition; works on single-core via preemption.

## Decision

**FreeRTOS tasks with priorities and queues.** Single-core ESP32-C3 sufficient. Decomposition:

| Task | Priority | Period | Job |
|------|----------|--------|-----|
| `led_render_task` | High (5) | 60 Hz fixed | Render local state → NeoPixelBus, `Show()` |
| `ca_tick_task` | Med (3) | 10 Hz | Drain neighbor-state queue, compute next state, publish to mesh |
| `mesh_tx_task` | Med (3) | 1 Hz + event-driven | Heartbeat broadcasts, wand-presence broadcasts |
| `mesh_rx_callback` | (ISR) | event-driven | ESP-NOW callback enqueues received packet to `mesh_rx_queue` |
| `housekeeping_task` | Low (1) | 0.2 Hz | Battery ADC, OTA poll, sleep manager, log writer |

Inter-task communication via FreeRTOS queues (`mesh_rx_queue`, `state_update_queue`) and atomic writes to a small shared `local_state` struct.

## Consequences

- ESP32-C3 single-core is sufficient. LED frame work (~120 µs of CPU during I2S setup, then zero CPU during DMA transfer) leaves the core idle most of the time. Other tasks preempt cleanly.
- The architecture is *the firmware*. Don't write a v0 in bag-of-timers and port; build directly on FreeRTOS tasks from the bench-prototype phase.
- Native unit-testable code (CA logic, packet codec, animation generators) lives in `firmware/core/` and compiles on host. Platform glue (drivers, RTOS task definitions, ESP-NOW callbacks) lives in `firmware/esp32/`. See `firmware/ARCHITECTURE.md`.
- Sleep behavior is task-aware: the housekeeping task can issue `esp_light_sleep_start()` between LED frames if all queues are empty and no broadcast is pending. Saves significant power vs naive `vTaskDelay`.
- If we ever do hit a CPU-bound problem, escape hatch is ESP32-S3 (dual-core RISC-V) — same architecture, same FreeRTOS task code, just enables `xTaskCreatePinnedToCore`. Not expected to be needed.
