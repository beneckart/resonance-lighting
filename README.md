# Resonance Lighting

Power and lighting workstream for the **Resonance Tree** — a bamboo art installation for Burning Man 2026 + 2027. This repo covers the 100 solar-powered, mesh-networked downlight fixtures that hang inside the tree.

Sister tracks (not in this repo): bamboo structure (Bamboo Pure, Bali), structural engineering (Ed), parametric lighting design (Vishnu), project management (Elliot + Co-Work agent).

## Who's working here

- **Ben Eckart** — power systems, firmware, mesh networking. Owns `/firmware/` and `/hardware/`.
- **Steve Eckart** — enclosure design, 3D printing, mechanical fit. Owns `/enclosure/`.

Both work with AI pair-programmers. Coordinate via `LOG.md`, `TODO.md`, and ADRs.

## What is the deliverable

100 modular "downlight" fixtures. Each one is:

- A bamboo lantern body, fabricated by Bamboo Pure in Bali.
- A solar "hat" enclosure that sits partially inside, partially over the bamboo top.
- A solar panel, rechargeable battery, controller board, LED module, and optional 3D-printed patterned aperture / gobo.
- Firmware that supports autonomous ambient lighting, ESP-NOW state exchange, standard OTA maintenance updates, telemetry, and graceful low-power behavior.

## Current architecture direction

**PowerFeather V2 (ESP32-S3) is the confirmed reference** for the controller / solar-and-battery manager / telemetry, after 5-board feasibility testing (ADR 0021): ESP-NOW mesh at scale, battery-only no-touch OTA + A/B rollback, and the solar charge path are all validated on hardware. Chemistry is **LiFePO4** (ADR 0002).

Two production paths remain open, pending procurement and the sizing/thermal de-risks:

1. **COTS track.** Buy PowerFeather V2 boards (ideally with factory-soldered VDC + LED connectors via a custom assembly, to avoid per-unit hand-soldering — ADR 0009). Removes custom-hardware risk.
2. **Custom PCBA track.** A PowerFeather-derived board (ESP32-S3-WROOM module, BQ25628E-class charger, MAX17260-class gauge, buck-boost 3.3 V rail, switchable LED/STEMMA rail, keyed connectors, boring USB/pogo flashing) if COTS supply/cost/assembly at 100+ units doesn't pencil out.

The **LED module is still being decided** (ADR 0018): SK6812 "HEX" direct-GPIO @ 3.3 V (distributed area/wash) vs a 4 W RGBW point source (crisp gobo), both driven **direct-GPIO off a free pin**. The earlier Adafruit IS31FL3741 13×9 STEMMA-QT matrix was **ruled out** — it browns out the board on battery under WiFi (shared charger/gauge I2C bus). The earlier COTS bake-off candidates (FeatherS2 Neo, Atom Matrix, NeoHEX, DFR0559) served their purpose; PowerFeather V2 won.

The old custom-board target of ESP32-C3-MINI-1 + CN3058 + AP2112K + direct-from-battery WS2812B has been superseded by later ADRs.

## Goals

- **Fungible:** any unit replaces any other with no per-device configuration.
- **Fully wireless:** no data lines, no power lines, no fixed topology. ESP-NOW is for lightweight state/control packets, not firmware-image transfer.
- **Standard OTA only:** OTA updates use normal ESP32 OTA mechanisms in a deliberate maintenance mode. No custom mesh-gossiped firmware images.
- **Durable infrastructure:** fixtures are reused in 2026 and 2027.
- **Low per-fixture operations:** no skilled repetitive work at 100-unit scale. Small, deliberate soldering such as a solar pigtail can be acceptable; hand-soldering rows of headers or hand-crimping harnesses is not.
- **Telemetry:** power, solar, battery, temperature, and failure data should inform BM 2027 design decisions.
- **Beautiful:** default center-source gobo projection plus optional multi-LED chromatic/animation modes.

## Repo layout

```
.
├── README.md
├── LOG.md
├── TODO.md
├── BACKGROUND.md
├── hardware/
│   ├── atopile/
│   ├── kicad/
│   └── references/
├── enclosure/
│   ├── stl/
│   ├── source/
│   └── references/
├── firmware/
│   ├── ARCHITECTURE.md
│   └── README.md
├── docs/
│   ├── block-diagram/
│   ├── decisions/
│   ├── research/
│   └── tests/
└── ops/
```

## Read order for agents and humans

1. `README.md`
2. `LOG.md`
3. `TODO.md`
4. `BACKGROUND.md`
5. `docs/decisions/` — especially ADRs 0010 onward
6. `docs/research/COTS_SURVEY_2026-05-10.md`
7. `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md`
8. `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`

## Status

The COTS bake-off concluded: **PowerFeather V2 is confirmed as the controller / solar / telemetry brain** (ADR 0021). Current work is the set of de-risks gating a bulk parts order: closing the **battery/panel sizing** (harvest-at-MPP vs LED-show + idle load; the "2000 mAh" LFP cell capacity is now VERIFIED at ~2077 mAh (2026-06-10 full charge→empty INA-coulomb run) — at/above rating; the gauge's own SOC stays unreliable on LFP's flat plateau until it learns a cycle, so coulomb-count. Production targets a larger LFP 32700 (~6000 mAh) anyway), **thermal** validation in a sealed hat (LFP charge-temperature limits), the **LED module** decision, and **procurement** at 100+ units (COTS vs custom assembly). See `LOG.md` and `TODO.md` for the live state.
