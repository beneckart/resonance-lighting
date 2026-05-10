# Resonance Lighting

Power and lighting workstream for the **Resonance Tree** — a bamboo art installation for Burning Man 2026 + 2027. This repo covers the 100 solar-powered, mesh-networked downlight fixtures that hang inside the tree.

Sister tracks (not in this repo): bamboo structure (Bamboo Pure, Bali), structural engineering (Ed), parametric lighting design (Vishnu), project management (Elliot + Co-Work agent).

## Who's working here

- **Ben Eckart** — power systems, firmware, mesh networking. Owns `/firmware/` and `/hardware/`.
- **Steve Eckart** — enclosure design, 3D printing, mechanical fit. Owns `/enclosure/`.

Both work with AI pair-programmers. Coordinate via `LOG.md` and `TODO.md` at repo root.

## What is the deliverable

100 modular "downlight" fixtures. Each one is:

- A bamboo lantern body (cylindrical bamboo pole with a steam-bent flared skirt, made by Bamboo Pure in Bali; not designed here — this repo's deliverable is the electronics module that mounts on top).
- A solar "hat" enclosure that sits partially inside, partially over the bamboo top (3D printed, MJF nylon or equivalent production process).
- A solar power system, battery, MCU, addressable LED source, and optional 3D-printed patterned aperture ("filter" / gobo).

## Current architecture policy

The project now has two parallel hardware tracks:

1. **Track A — COTS deployable prototype / fallback.** Off-the-shelf boards, charger modules, LED boards, pre-crimped cables, USB/JST/STEMMA connections, screws, standoffs, and factory-soldered headers are allowed if they avoid skilled per-unit assembly. This track must be good enough to deploy all 100 fixtures if the custom PCBA slips.
2. **Track B — custom PCBA optimization.** A custom board may integrate charger, MCU module, LED rail switching, connectors, and test pads after the COTS path proves power, firmware, optics, and enclosure constraints.

This is a deliberate risk-control decision. The real constraint is not "single board." The real constraint is no skilled, slow, error-prone per-fixture work.

## Goals

- **Fungible** — any unit replaces any other with no per-device configuration. Swap a broken fixture quickly.
- **Fully wireless** — no data lines, no power lines, no fixed topology. Local coordination via ESP-NOW or equivalent ESP32 peer-to-peer packets.
- **Standard OTA only** — firmware updates use the most vanilla ESP32 WiFi OTA path available, with A/B partitions and rollback. Firmware images are never gossiped through the ESP-NOW mesh. USB-C / pogo flashing remains the guaranteed recovery path.
- **Durable infrastructure** — fixtures are reused in 2026 and 2027.
- **Operations cost O(1) per fixture, not O(N).** No soldering on receipt; same firmware image for every fixture; per-unit identity derived at runtime from MAC; no per-unit pairing. Pre-flash at the fab if available; otherwise flash via a pogo/USB jig.
- **Beautiful** — ambient cellular-automata light dynamics, hand-carried "wand" lantern interactions, and optional mandala apertures projecting shadows.

## Production design principles

- Prefer a pre-certified Espressif module with integrated RF/antenna and comfortable compute/RAM/flash headroom. Do not design custom RF.
- Prefer LiFePO4 for the final chemistry because the installation is hot, outdoor, stored, and reused. LiPo COTS prototypes/fallbacks are allowed only when paired with the correct LiPo charger and battery.
- Use a proven LiFePO4-capable solar charger reference before committing to a custom charger. bq25185-class designs are the current preferred direction; CN3058 is now fallback, not default.
- LED rail must be switchable and default-off so a hung MCU cannot leave LEDs on indefinitely and drain the battery.
- The LED array may be a daughterboard. The optical geometry can evolve separately from the power/controller board.
- Use 3x3 or 5x5 LED geometry for future creative modes, but default optical projection modes should use the center LED for crisp gobos.

## Repo layout

```
.
├── README.md          You are here.
├── LOG.md             Append-only session journal.
├── TODO.md            Current punch list.
├── BACKGROUND.md      Project context.
│
├── hardware/          Carrier PCB and/or COTS electronics integration.
├── enclosure/         Solar-hat enclosure CAD.
├── firmware/          ESP32 firmware.
├── docs/              Design docs, ADRs, research, tests.
└── ops/               Logistics, vendor contacts, BOM, shipping.
```

## How to use this with agents

Before any session, agents and humans should read:

1. `README.md`
2. `TODO.md`
3. `BACKGROUND.md`
4. `docs/ROADMAP.md`
5. `docs/decisions/`
6. `docs/research/`

Architectural decisions go into `docs/decisions/` as ADR files. New ADRs supersede old ADRs; old ADRs should not be silently rewritten except to mark supersession.

## Status

Pre-prototype / architecture correction pass. The current priority is building a COTS deployable prototype while revising the custom-PCBA plan around standard OTA, WROOM-style module selection, bq25185-class power management, and fail-safe LED rail switching.
