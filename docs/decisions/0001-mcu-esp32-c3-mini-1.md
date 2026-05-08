# 0001 — Use ESP32-C3-MINI-1 for production

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben

## Context

Need to pick the MCU for the production carrier board. Constraints:

- Must support WiFi-class peer-to-peer mesh at BRC with no infrastructure (no APs).
- Must support OTA over WiFi after a single USB flash.
- Must drive WS2812B reliably (I2S+DMA preferred).
- Must run on a single LiFePO4 cell (Vbat 2.5–3.6 V).
- Must be SMT-assemblable in the JLCPCB Basic library.
- 100 units, hobby-art-project budget.
- ~5 µA deep sleep target for solar duty cycle.

## Options considered

- **ESP32-C3-MINI-1** (RISC-V, single-core, 160 MHz, WiFi+BLE, 4 MB flash, FCC-pre-certified module). $1.50 in JLCPCB Basic.
- **ESP32-S3-MINI-1** (Xtensa dual-core, 240 MHz, USB OTG). $2.50, slightly more capable but more current.
- **ESP32-D0WD-V3 + external flash + crystal + RF circuit** (the original ESP32). Cheaper chip but more BOM and design risk.
- **Nordic nRF52840** (BLE Mesh / Thread). Lower-power ecosystem but no WiFi means no easy OTA; mesh stack is heavier to build.

## Decision

**ESP32-C3-MINI-1.** Single-core is sufficient for this workload (4 LEDs + CA on ~10 mesh neighbors is microseconds of work). Lower idle current than dual-core variants. Cheapest of the FCC-pre-certified Espressif modules. Same firmware ecosystem (Arduino-ESP32, ESP-IDF, NeoPixelBus, ESP-NOW) as the gen-1 ESP32 used in the 2018 Talisman v2 — minimal porting cost.

## Consequences

- Locks the firmware to the ESP-NOW mesh path. No fallback to LoRa without adding hardware (acceptable; ESP-NOW is sufficient for the tree's spatial scale of ~20 ft).
- Forces the firmware architecture to be RTOS-task-based for clean concurrency under single-core preemption (see ADR 0004). Single-core does not bottleneck this workload.
- Fewer GPIO than gen-1 ESP32 (~22 vs ~34). Sufficient for our needs (data line, button, status LED, battery sense gate, LED enable, USB).
- USB-Serial built into the chip — no separate USB-Serial converter (CP2104 / CH9102 / etc.) on BOM.
- ESP32-C3 has BLE 5.0 only (no Bluetooth Classic). Not relevant; we don't use Classic.
