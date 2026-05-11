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

The project now has two active hardware tracks:

1. **COTS production/fallback track.** Use off-the-shelf boards where possible to remove custom-hardware risk. The leading candidate is **PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 13x9 STEMMA-QT matrix**. Other COTS candidates include FeatherS2 Neo, M5Stack Atom Matrix, M5Stack NeoHEX, and DFRobot DFR0559 Solar Power Manager.
2. **Custom PCBA track.** If needed, design a bespoke board derived from the successful COTS/reference architecture. Current bias is toward a PowerFeather-like design: ESP32-S3-WROOM-class module, BQ25628E-class charger/power path, MAX17260-class fuel gauge, buck-boost 3.3 V rail, switchable LED/STEMMA rail, keyed connectors, and boring USB/pogo flashing.

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

R&D parts have been ordered for a COTS architecture bake-off. Immediate work is bench testing PowerFeather V2 vs FeatherS2 Neo / Atom Matrix fallback paths, validating LiFePO4/solar behavior, testing LED modules through the gobo/filter, and deciding whether 2026 production should use COTS boards, a custom PCBA, or a hybrid.
