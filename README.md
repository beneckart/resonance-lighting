# Resonance Lighting

Power and lighting workstream for the **Resonance Tree** -- a bamboo art installation for Burning Man 2026 + 2027. This repo covers the ~150 solar/battery-powered, mesh-networked lighting fixtures in and around the tree -- four classes: hanging downlights, perimeter lights, uplights, and the central chandelier (canonical counts: the fleet table in `docs/block-diagram/SYSTEM.md`; decision record: ADR 0024).

Sister tracks (not in this repo): bamboo structure (Bamboo Pure, Bali), structural engineering (Ed), parametric lighting design (Vishnu), project management (Elliot + Co-Work agent).

## Who's working here

- **Ben Eckart** -- power systems, firmware, mesh networking. Owns `/firmware/` and `/hardware/`.
- **Steve Eckart** -- enclosure design, 3D printing, mechanical fit. Owns `/enclosure/`.

Both work with AI pair-programmers. Coordinate via `LOG.md`, `TODO.md`, and ADRs.

## What is the deliverable

~150 modular lighting fixtures (150-152, tentative until installation) in four classes
-- 72 hanging downlights, 38-40 perimeter lights on shepherd hooks, 24 uplights, and
16 chandelier lights. The archetype (hanging downlight) is:

- A bamboo lantern body, fabricated by Bamboo Pure in Bali.
- A solar "hat" enclosure that sits partially inside, partially over the bamboo top.
- A solar panel, rechargeable battery, controller board, LED module, sensors
  (accelerometer + downward ToF), and a 3D-printed patterned aperture / gobo.
- Firmware that supports autonomous ambient lighting, ESP-NOW state exchange, standard OTA maintenance updates, telemetry, and graceful low-power behavior.

The other classes are variants on the same electronics: perimeter lights swap the LED
role and face the ToF outward; uplights and chandelier lights drop the gobo and may go
solar-free (big cell + USB-C charging -- open decision). All share one firmware image.

## Current architecture direction

**PowerFeather V2 (ESP32-S3) is the confirmed reference** for the controller / solar-and-battery manager / telemetry, after 5-board feasibility testing (ADR 0021): ESP-NOW mesh at scale, battery-only no-touch OTA + A/B rollback, and the solar charge path are all validated on hardware. Chemistry is **LiFePO4** (ADR 0002); the production cell is the fullbattery 32700 6 Ah, qualified n=2 (ADR 0025).

**The production path is decided: COTS PowerFeather V2 at ~150 units (ADR 0024).**
68 boards are bought and 82 more invoice 2026-07-10 (ledger: `ops/PROCUREMENT.md`).
The custom PowerFeather-derived PCBA (ESP32-S3-WROOM module, BQ25628E-class charger,
MAX17260-class gauge, buck-boost 3.3 V rail, switchable rails, keyed connectors) is
the 2027 option, carrying the ADR 0028 bus-integrity rules.

The **LED axis is a mixed fleet by optical role** (ADR 0022): SK6812 "HEX"
direct-GPIO for close-range animation / ambient glow, and a 4 W RGBW point source for
long-throw crisp gobo projection. Both are driven **direct-GPIO off a free pin**, currently
fed from the switchable 3V3 rail; the 4.2 V boost is measured and shelved, and a
measured-better VBAT-direct option for the RGBW (+33 % fringed white) is open with
its conversion plan recorded (ADR 0029). The type mix by class is in the
SYSTEM.md fleet table (tentative until installation). The earlier Adafruit
IS31FL3741 13x9 STEMMA-QT matrix was **ruled out** (ADR 0018) -- it browns out the
board on battery under WiFi (shared charger/gauge I2C bus). The earlier COTS bake-off
candidates (FeatherS2 Neo, Atom Matrix, NeoHEX, DFR0559) served their purpose;
PowerFeather V2 won.

**Sensors** (ADR 0027): every downlight carries an MSA311 accelerometer + downward
TMF8820-mini multizone ToF; perimeter lights carry an outward VL53L5CX. Fused IMUs
were rejected (per-device calibration doesn't scale to 150 units). A **noisemaker**
axis is in exploration (speaker synth vs solenoid bamboo-strike -- open).

The old custom-board target of ESP32-C3-MINI-1 + CN3058 + AP2112K + direct-from-battery WS2812B has been superseded by later ADRs.

## Goals

- **Fungible:** any unit replaces any other with no per-device configuration.
- **Fully wireless:** no data lines, no power lines, no fixed topology. ESP-NOW is for lightweight state/control packets, not firmware-image transfer.
- **Standard OTA only:** OTA updates use normal ESP32 OTA mechanisms in a deliberate maintenance mode. No custom mesh-gossiped firmware images.
- **Durable infrastructure:** fixtures are reused in 2026 and 2027.
- **Low per-fixture operations:** no skilled repetitive work at 150-unit scale. Small, deliberate soldering such as a solar pigtail can be acceptable; hand-soldering rows of headers or hand-crimping harnesses is not.
- **Telemetry:** power, solar, battery, temperature, and failure data should inform BM 2027 design decisions.
- **Beautiful:** default center-source gobo projection plus optional multi-LED chromatic/animation modes.

## Repo layout

```
.
|-- README.md
|-- LOG.md
|-- TODO.md
|-- BACKGROUND.md
|-- ROADMAP.md
|-- AGENTS.md
|-- hardware/
|   |-- atopile/
|   |-- led-adapter/       NeoHEX passive adapter Rev A (KiCad + PCBWay packet)
|   `-- references/
|-- enclosure/             README only so far; CAD lives with Steve
|-- firmware/
|   |-- ARCHITECTURE.md    target production architecture
|   |-- README.md          index of the working bench sketches
|   |-- POWERFEATHER_NOTES.md
|   |-- powerfeather_solar_guard.h
|   `-- <app>/             bench sketches (net_bench, power_bench, led_studio, ...)
|-- docs/
|   |-- block-diagram/     SYSTEM.md -- canonical architecture + fleet table
|   |-- decisions/         ADRs 0001-0029
|   |-- research/
|   `-- tests/
`-- ops/
    |-- bom.md             fleet BOM + spares math
    |-- PROCUREMENT.md     orders ledger + timeline
    `-- bench/             bench tooling + JSONL data
```

## Read order for agents and humans

1. `README.md`
2. `LOG.md`
3. `TODO.md`
4. `BACKGROUND.md`
5. `docs/decisions/` -- especially ADRs 0010 onward
6. `docs/research/COTS_SURVEY_2026-05-10.md`
7. `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md`
8. `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`

## Status

As of 2026-07-08: **production is locked on COTS PowerFeather V2 at ~150 fixtures in
four classes (ADR 0024) and the bulk buy has largely happened** -- boards, batteries,
panels, LEDs, and sensors are ordered or received (~$12.7k committed; ledger in
`ops/PROCUREMENT.md`). The battery vendor is qualified n=2 with measured dim/off/sleep
thresholds (ADRs 0025/0023); panels are selected and outdoor-measured (ADR 0026);
sensors are chosen and allocated by class (ADR 0027); the LED electrical drive
matrix is measured with the boost shelved and the RGBW feed decision open (ADR
0029); and the two-month reboot mystery closed as a bus-integrity rule, sealed by a
46-hour soak (ADR 0028).

Remaining gates before Grass Valley assembly (~Aug 1): the bottom-up nightly energy
budget by role, the uplight/chandelier power decision (solar vs 20 Ah vs 6 Ah), hat
thermal/RF proof, cabling buys, the ADR 0023 state machine into production firmware,
and the noisemaker verdict. Treat LFP SOC as advisory until the gauge learns; use
coulomb counting and voltage/current guardrails. See `LOG.md` and `TODO.md` for the
live state.
