# Resonance Lighting

Power and lighting workstream for the **Resonance Tree** — a bamboo art installation for Burning Man 2026 + 2027. This repo covers the 100 solar-powered, mesh-networked downlight fixtures that hang inside the tree.

Sister tracks (not in this repo): bamboo structure (Bamboo Pure, Bali), structural engineering (Ed), parametric lighting design (Vishnu), project management (Elliot + Co-Work agent).

## Who's working here

- **Ben Eckart** — power systems, firmware, mesh networking. Owns `/firmware/` and `/hardware/`.
- **Steve Eckart** — enclosure design, 3D printing, mechanical fit. Owns `/enclosure/`.

Both work with AI pair-programmers (Claude). Coordinate via `LOG.md` and `TODO.md` at repo root.

## What is the deliverable

100 modular "downlight" fixtures. Each one is:

- A bamboo lantern body (cylindrical bamboo pole with a steam-bent flared skirt, made by Bamboo Pure in Bali; not designed here — this repo's deliverable is the *electronics module* that mounts on top).
- A solar "hat" enclosure that sits partially inside, partially over the bamboo top (3D printed, MJF nylon for production).
- A custom carrier PCB containing: ESP32-C3-MINI-1, LiFePO4 charger, voltage regulator, USB-C, JST connectors for solar panel / battery / LED chain.
- A small solar panel (~1–2 W), LiFePO4 battery, 1–9 WS2812B LEDs, and an optional 3D-printed patterned aperture ("filter" / gobo) inside the bamboo body.

Goals:

- **Fungible** — any unit replaces any other with no per-device configuration. Swap a broken fixture in under a minute.
- **Fully wireless** — no data lines, no power lines, no fixed topology. Mesh via ESP-NOW. OTA updates after fab.
- **Durable infrastructure** — fixtures are reused in 2026 and 2027 (year-2 expansion: conch shell built around the same trunk).
- **Operations cost O(1) per fixture, not O(N).** Boards arrive fully SMT-assembled by the fab — no soldering on receipt. Same firmware image for every fixture; per-unit identity derived at runtime from MAC. Pre-flash firmware at the fab if available; otherwise flash via a pogo-pin jig in seconds. See ADR 0009.
- **Beautiful** — not just on; ambient cellular-automata light dynamics, hand-carried "wand" lantern interactions, optional community-designed mandala apertures projecting shadows.

## Repo layout

```
.
├── README.md          You are here.
├── LOG.md             Append-only session journal. Read first.
├── TODO.md            Current punch list. Owners and priorities.
├── BACKGROUND.md      Project context. Read by humans and agents at session start.
│
├── hardware/          Carrier PCB design.
│   ├── atopile/       atopile source (.ato) — the schematic as code.
│   ├── kicad/         KiCad project for layout (generated from atopile).
│   └── references/    Reference schematics and datasheets we lifted from.
│
├── enclosure/         Solar-hat enclosure CAD (Steve's territory).
│   ├── stl/           Print-ready STLs.
│   ├── source/        Fusion / FreeCAD / OpenSCAD source.
│   └── references/    Bamboo lantern drawings from Vishnu.
│
├── firmware/          ESP32 firmware. Stub for now; populated post-board.
│   └── README.md      Firmware roadmap and build notes.
│
├── docs/              Design docs.
│   ├── block-diagram/ System block diagram + power budget.
│   ├── decisions/     ADR-style decision records.
│   └── tests/         Test plans, measured results.
│
└── ops/               Logistics, vendor contacts, BOM, shipping.
```

## How to use this with agents

Before any session, agents (and humans) should read:

1. `README.md` (this file) — orientation
2. `LOG.md` — what changed recently
3. `TODO.md` — what's open
4. `BACKGROUND.md` — full project context, team, decisions

That's the contract. New decisions go into `LOG.md` with a date stamp. New work goes into `TODO.md` with an owner. Architectural decisions go into `docs/decisions/` as short ADR files.

## Status

Pre-prototype. Bench validation on TTGO modules (T-Beam and T-Ice from prior projects) is in progress. Custom carrier PCB design hasn't started yet — see `TODO.md` for current state.
