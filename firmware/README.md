# Firmware

ESP32 firmware for the Resonance downlight. **Stub for now** — populated once the basic hardware design is locked down and bench validation begins.

> **Building a new app on the PowerFeather V2 bench boards?** Read
> [`POWERFEATHER_NOTES.md`](POWERFEATHER_NOTES.md) first — the switchable 3V3 rail
> (GPIO4), the V2 SDK board flag, native-USB reset/IP recovery, and other gotchas
> that have each cost real bench time. Working sketches: `power_bench/`,
> `led_studio/` (merged HEX + RGBW + RGB aesthetic tool),
> `net_bench/` (ESP-NOW networking feasibility bench), `smoke_test/`,
> `powerfeather_demo_port/`.

## Planned structure

```
firmware/
├── core/             Platform-independent C++. Compiles native, has unit tests.
│   ├── ca/           Cellular automata engine.
│   ├── packet/       Mesh packet codec.
│   ├── pattern/      LED animation generators (port of TalismanPatterns).
│   └── state/        Per-fixture state representation.
│
├── esp32/            ESP32-specific glue. Builds against ESP-IDF or Arduino-ESP32.
│   ├── tasks/        FreeRTOS task definitions.
│   ├── drivers/      NeoPixelBus, ESP-NOW, charger ADC, button.
│   └── boards/       Pin mappings per board (TTGO T-Beam, T-Ice, custom).
│
└── tests/            Native unit tests. Run on host, not target.
```

## Architecture (intent — see `BACKGROUND.md` for full reasoning)

FreeRTOS tasks, not bag-of-timers in `loop()`:

- **`led_render_task`** — 60 Hz, high priority. Renders local state to NeoPixelBus.
- **`ca_tick_task`** — 10 Hz. Reads neighbor states from queue, computes next local state, publishes to mesh.
- **`mesh_rx`** — ESP-NOW callback enqueues packets.
- **`mesh_tx_task`** — periodic state broadcasts and wand-presence handling.
- **`housekeeping_task`** — battery check, OTA poll, sleep manager. Low priority.

## Reusable from `beneckart/future-robotics`

- `Talisman/arduino-sketches/talisman_v2rev2/TalismanPatterns.cpp` — templated NeoPixelBus animation engine. Drop into `core/pattern/`.
- The 11-byte structured-payload packet pattern from `sendPkt()`. ESP-NOW handles framing so we drop the sentinels.
- Marquee Python OPC clients — useful as host-side test harness once we want to stream test patterns to a single bench fixture.
- Marquee C++ effect engine (`particle_trail.cpp`, `rings.cpp`, etc.) — reference for spatial-aware effects.

## Bench validation targets (parallel work, not blocked on custom board)

Run on existing TTGO T-Beam and T-Ice modules in Steve's workshop:

1. Solar panel charges battery via the T-Beam's built-in charger. Verify charge profile, runtime.
2. WS2812B output via NeoPixelBus + I2S DMA. Animation runs cleanly.
3. ESP-NOW between two TTGO modules. Range, latency, packet loss measurements.
4. OTA over WiFi to one TTGO. Validate A/B partition flow.
5. RTOS task decomposition with scaffolded `core/` + `esp32/` split.
