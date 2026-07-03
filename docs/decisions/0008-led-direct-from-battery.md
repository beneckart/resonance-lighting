# 0008 -- WS2812B powered direct from battery rail

**Date:** 2026-05-06
**Status:** Superseded by ADR 0013 -- LED rail must be switchable and fail-safe; exact voltage rail chosen by test
**Owners:** Ben

## Context

WS2812B/SK6805-style LEDs can often be powered directly from a single-cell battery, especially LiFePO4, while accepting 3.3 V data from an ESP32-class MCU. The original design used this to avoid a boost converter or level shifter.

## Original decision

Power LEDs from Vbat directly. No level shifter.

## Why this is superseded

Direct battery power is still allowed, but the stronger requirement is fail-safe LED power control. If the MCU hangs after the last LED command was nonzero, addressable LEDs can remain on and drain the battery into brownout/reboot-loop territory. Production hardware must make the LED rail switchable, default-off at reset, and controllable under low-battery/watchdog/shipping-mode conditions. The exact LED voltage rail -- Vbat, regulated 3.3 V, 4.5 V load rail, or 5 V boost rail -- is chosen by bench testing and by the COTS/custom architecture. See ADR 0013.
