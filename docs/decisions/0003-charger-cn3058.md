# 0003 -- CN3058 LiFePO4 charger IC

**Date:** 2026-05-06
**Status:** Superseded by ADR 0014 -- LiFePO4-capable solar charger reference first, bq25185 preferred candidate
**Owners:** Ben

## Context

Need a charger IC compatible with single-cell LiFePO4, input from a small solar panel, and a load that includes an ESP32-class MCU plus 1-25 addressable RGB LEDs.

## Original decision

Use CN3058 as the preferred charger IC.

## Why this is superseded

CN3058 is still a plausible fallback, but the project should avoid making a first custom solar power-path design the only route to production. TI bq25185-class designs provide a stronger reference point: Li-ion/LiPo/LiFePO4 configurability, power-path behavior, weak-source/solar input support, battery temperature protection, low quiescent current, and published Adafruit breakout designs. See ADR 0014.
