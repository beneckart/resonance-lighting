# Firmware

ESP32 firmware for the Resonance fixtures. Current reality: a set of standalone
Arduino-ESP32 bench sketches (below), each proving a production subsystem on the
PowerFeather V2. The layered production codebase in `ARCHITECTURE.md` is the target,
not yet built; `net_bench` is the closest thing to production firmware today
(ESP-NOW + OTA + watchdog + field-cycle low-battery lifecycle).

> **Building a new app on the PowerFeather V2 bench boards?** Read
> [`POWERFEATHER_NOTES.md`](POWERFEATHER_NOTES.md) first -- the switchable 3V3 rail
> (GPIO4), the V2 SDK board flag, native-USB reset/IP recovery, and other gotchas
> that have each cost real bench time. Working sketches: `power_bench/`,
> `led_studio/` (merged HEX + RGBW + RGB aesthetic tool),
> `net_bench/` (ESP-NOW networking feasibility bench), `smoke_test/`,
> `powerfeather_demo_port/`, `presence_bench/` (I2C multi-sensor bench),
> `sway_demo/` (MSA311 tilt/sway -> RGBW color, with a web verifier),
> `speaker_demo/` (STEMMA speaker #3885 percussion synth, noisemaker candidate A).

Solar/charging baseline: any Resonance sketch that enables PowerFeather charging must
use `powerfeather_solar_guard.h` to force the BQ25628E wide input-OVP bit and to kick
input re-qualification if the panel is present but the charger is latched not-good.

## Planned production structure (NOT yet built -- see ARCHITECTURE.md)

```
firmware/
|-- core/             Platform-independent C++. Compiles native, has unit tests.
|   |-- ca/           Cellular automata engine.
|   |-- packet/       Mesh packet codec.
|   |-- pattern/      LED animation generators (port of TalismanPatterns).
|   `-- state/        Per-fixture state representation.
|
|-- esp32/            ESP32-specific glue. Builds against ESP-IDF or Arduino-ESP32.
|   |-- tasks/        FreeRTOS task definitions.
|   |-- drivers/      NeoPixelBus, ESP-NOW, charger/gauge, rail control.
|   `-- boards/       Pin mappings per board (powerfeather_v2, resonance_custom).
|
`-- tests/            Native unit tests. Run on host, not target.
```

## Architecture (intent -- see `BACKGROUND.md` for full reasoning)

FreeRTOS tasks, not bag-of-timers in `loop()`:

- **`led_render_task`** -- 60 Hz, high priority. Renders local state to NeoPixelBus.
- **`ca_tick_task`** -- 10 Hz. Reads neighbor states from queue, computes next local state, publishes to mesh.
- **`mesh_rx`** -- ESP-NOW callback enqueues packets.
- **`mesh_tx_task`** -- periodic state broadcasts and wand-presence handling.
- **`housekeeping_task`** -- battery check, OTA poll, sleep manager. Low priority.

## Reusable from `beneckart/future-robotics`

- `Talisman/arduino-sketches/talisman_v2rev2/TalismanPatterns.cpp` -- templated NeoPixelBus animation engine. Drop into `core/pattern/`.
- The 11-byte structured-payload packet pattern from `sendPkt()`. ESP-NOW handles framing so we drop the sentinels.
- Marquee Python OPC clients -- useful as host-side test harness once we want to stream test patterns to a single bench fixture.
- Marquee C++ effect engine (`particle_trail.cpp`, `rings.cpp`, etc.) -- reference for spatial-aware effects.

## Validated on hardware (superseding the old TTGO bench plan)

The COTS campaign ran on PowerFeather V2, not the TTGO modules. Record of record:

1. Solar charge path end-to-end incl. the bright-sun OVP/HIZ guard (ADR 0021/0026;
   `powerfeather_solar_guard.h`).
2. Direct-GPIO LED drive measured per role; boost shelved (ADR 0029; `led_studio`).
3. ESP-NOW at 5 nodes with a 100-node projection, range through house+yard+oak
   (ADR 0021; `net_bench`).
4. Battery-only standard OTA + A/B rollback + watchdog recovery; low-VBAT OTA
   brackets (ADR 0021; `net_bench`, `docs/tests/OTA_FLASH_BENCHMARKS_2026-05-15.md`).
5. Low-battery day/night lifecycle (field-cycle) with measured thresholds
   (ADR 0023); bus-integrity rules (ADR 0028; `POWERFEATHER_NOTES.md`).
6. Sensor chain: MSA311 + multizone ToF fusion on real geometry (ADR 0027;
   `sway_demo`, `presence_bench`).
