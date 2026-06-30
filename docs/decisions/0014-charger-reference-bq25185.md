# 0014 -- LiFePO4-capable solar charger reference first; bq25185 preferred candidate

**Date:** 2026-05-08
**Status:** Accepted
**Owners:** Ben
**Supersedes:** ADR 0003

## Context

The project needs solar charging, battery protection, low quiescent current, and enough load current for an ESP32-class MCU plus addressable LEDs. The preferred chemistry remains LiFePO4 for heat tolerance and two-year reuse, but LiFePO4 narrows COTS charger choices compared with LiPo.

The earlier CN3058 decision was attractive for cost and simplicity, but it put custom charger behavior and custom power-path details on the critical path. That is not the lowest-risk route for a 100-unit art deployment.

## Options considered

- **bq25185-based design:** Li-ion/LiPo/LiFePO4 support, power path, solar/weak-source input behavior, configurable charge voltage/current, low quiescent current, battery temperature/fault protection. Strong candidate for preferred custom design and COTS reference testing.
- **Adafruit bq25185 charger boards:** useful COTS/reference designs. The basic board, 3.3 V buck board, and 5 V boost board cover several prototype architectures. Availability must be checked before production.
- **CN3058:** still a plausible fallback for a simple LiFePO4 charger, but no longer the preferred first custom charger path.
- **DFRobot CN3165-based solar boards:** good LiPo COTS fallback, not a LiFePO4 preferred path.
- **LiPo-only Feather/FireBeetle integrated chargers:** acceptable only for COTS fallback or firmware/optics prototypes unless paired with the correct battery chemistry.

## Decision

Make bq25185-class LiFePO4-capable solar charging the preferred charger direction. Use an off-the-shelf bq25185 board as the first LiFePO4 solar power reference. Do not lock a custom CN3058 design until bq25185 availability, layout difficulty, and bench behavior have been evaluated.

## Specific implementation notes

- For LiFePO4, configure the battery regulation voltage appropriately; do not leave charger boards in default LiPo mode.
- Charge current should be conservative. The solar/power budget does not require aggressive fast charging, and lower current reduces thermal stress inside the sealed hat.
- Use battery temperature monitoring if available; hot-charge behavior matters in a sealed solar enclosure.
- The bq25185 six-hour safety timer behavior must be understood. If the chosen board ties CE permanently, test whether long solar days cause charge interruptions that matter.
- Prefer designs with accessible charge/fault/power-good signals for the smoke-test rig.

## Consequences

- `lifepo4_charger` should become `battery_charger` until the IC is locked.
- The custom board should copy a proven reference design only after bench testing.
- CN3058 remains a fallback ADR candidate, not the default.
- LiPo COTS fallback remains acceptable when paired with LiPo batteries and heat testing.
