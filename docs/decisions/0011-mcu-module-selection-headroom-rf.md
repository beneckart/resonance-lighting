# 0011 -- MCU module selection: pre-certified module with RF/headroom margin

**Date:** 2026-05-08
**Status:** Accepted
**Owners:** Ben
**Supersedes:** ADR 0001

## Context

The production MCU decision should minimize RF/layout risk and avoid painting the firmware into a corner. The hat enclosure has enough volume that the MCU module does not need to be the smallest or cheapest option. The project cost target is flexible; schedule, robustness, and development headroom matter more.

The original ADR chose ESP32-C3-MINI-1. That is a legitimate pre-certified module, but the rationale over-weighted compactness, low current, and low cost. The production design should instead select from larger, more forgiving Espressif module families when appropriate.

## Decision criteria

The selected MCU/module must:

- Be an Espressif module with integrated RF front-end, shield can, crystal, flash, and antenna or antenna connector.
- Avoid custom RF matching, chip antennas, hand-tuned PCB antennas, or bare ESP32 chip designs.
- Support ESP-NOW for local lighting/control messages.
- Support standard WiFi OTA and A/B partitioning.
- Provide enough flash/RAM for OTA, telemetry, logging, animation code, and future 2027 expansion.
- Provide enough GPIO for LED data, LED rail enable, battery sense, solar/charge sense, status LED, reset/boot, optional thermistor, and test pads.
- Have a conservative antenna placement path in the hat: module antenna at a board edge, full keep-out, no solar panel/battery/metal/screws/copper/wiring in the antenna zone.
- Be readily sourceable through the assembler or mainstream distributors.

## Options to evaluate

- **ESP32-S3-WROOM-1 / WROOM-1U variants:** preferred custom-PCBA family if available. Dual-core 240 MHz, larger GPIO/RAM/flash options, mature Espressif ecosystem, pre-certified module options, and more firmware headroom.
- **ESP32-C6-WROOM-1 variants:** good option if WiFi 6 / 802.15.4 future-proofing is attractive. Single-core but modern, ESP-NOW supported, WROOM module form factor available.
- **ESP32-C3-WROOM-02 variants:** simpler/lower-power option with a WROOM-style module and PCB antenna; less headroom than S3.
- **ESP32-C3-MINI-1:** still acceptable only if layout, sourcing, and firmware margin look better than alternatives. It is not the default target anymore.
- **Bare ESP32 chip + external RF:** rejected. Too much avoidable RF/layout/manufacturing risk.

## Decision

Do not lock production to ESP32-C3-MINI-1 yet. Select the exact production module only after the COTS prototype path and JLC/LCSC sourcing check are complete. The default bias is toward an ESP32-S3 WROOM-class module or another WROOM-style Espressif module with comfortable headroom and a forgiving antenna implementation.

## Consequences

- The hardware module should be renamed from `esp32_c3_mini_1` to a generic `esp32_module` until the exact part is locked.
- Firmware board definitions must support multiple dev/COTS targets during prototyping.
- Power budget should measure actual radio current rather than assuming the smallest MCU wins.
- The board outline and enclosure should reserve RF-safe placement for a WROOM-size module.
- Cost increases are acceptable if they reduce schedule or field risk.
