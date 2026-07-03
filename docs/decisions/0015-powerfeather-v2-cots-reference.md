# 0015 -- Treat PowerFeather V2 as the leading COTS/reference architecture

**Date:** 2026-05-10
**Status:** Accepted
**Owners:** Ben

## Context

After the first COTS survey, the project identified a stronger off-the-shelf architecture than the original custom ESP32-C3-MINI-1 + CN3058 plan.

ESP32-S3 PowerFeather V2 appears to provide, in one COTS Feather-format board:

- ESP32-S3-WROOM-1 module with onboard PCB antenna.
- BQ25628E charger / power-path IC.
- LiFePO4 support in V2.
- MAX17260 fuel gauge with LiFePO4 profile support.
- TPS631013 buck-boost 3.3 V regulator.
- Solar/DC input via `VDC`.
- USB-C.
- STEMMA-QT connector with switchable `VSQT` rail.
- Rich power telemetry useful for BM 2026 field logging and BM 2027 design decisions.

The official docs mark V2 details as preliminary, and Elecrow product listings may not make the V1/V2 distinction obvious. Hardware revision must be verified when boards arrive.

## Decision

Use **PowerFeather V2** as the leading COTS prototype and reference architecture.

This does not yet make PowerFeather V2 the production board. It means:

- Prioritize bench testing PowerFeather V2 as soon as boards arrive.
- Verify V2 hardware revision by chip markings and I2C scan before using LiFePO4.
- Treat PowerFeather V2 as the reference architecture for any bespoke PCB.
- Keep COTS deployment as a credible fallback if PowerFeather V2 performs well and can be sourced.

## Consequences

- The custom-board architecture should move away from CN3058/AP2112K-first thinking and toward PowerFeather-like power management: switch-mode charger with power path, buck-boost 3.3 V, fuel gauge, switchable external rails, and WROOM-class MCU.
- Power telemetry becomes a core feature, not an optional diagnostic. Battery voltage/current/temp/SOC, charge state, fault flags, and solar input behavior should be logged for BM 2026 field learning.
- If PowerFeather V2 proves stable, the 2026 production path could be COTS PowerFeather + LED module + custom hat rather than a full custom PCBA.
- If the creator shares KiCad/Gerber files, those become the best starting point for a bespoke Resonance board.
- If only schematics are available, the project should still use PowerFeather V2 as a block-level reference but must not copy the switching/fuel-gauge design blindly without layout review.

## Validation required

Before production reliance:

- Confirm boards are V2, not V1.
- Confirm LiFePO4 configuration and charging behavior.
- Confirm sleep current with external LED module attached and `VSQT` off.
- Confirm solar input behavior with the actual 1-5 W panels.
- Confirm telemetry accuracy enough for operational logging.
- Confirm RF performance inside a mock hat.
- Confirm thermal behavior inside a sealed or semi-sealed hat.
- Confirm supply availability for 100-150 units if using COTS production.
