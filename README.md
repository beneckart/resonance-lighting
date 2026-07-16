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
role and face the ToF outward; uplights drop the gobo and carry a hinged solar "wing"
on their base boot (decided 2026-07-15); chandelier lights live in a carpenter-built
box, likely on 6 Ah cells with USB-C top-ups. All share one firmware image.

## Current architecture direction

**PowerFeather V2 (ESP32-S3) is the confirmed reference** for the controller / solar-and-battery manager / telemetry, after 5-board feasibility testing (ADR 0021): ESP-NOW mesh at scale, battery-only no-touch OTA + A/B rollback, and the solar charge path are all validated on hardware. Chemistry is **LiFePO4** (ADR 0002); the production cell is the fullbattery 32700 6 Ah, qualified n=2 (ADR 0025).

**The production path is decided: COTS PowerFeather V2 at ~150 units (ADR 0024).**
158 boards are bought (68 received + 90 ordered 2026-07-09; ledger: `ops/PROCUREMENT.md`).
The custom PowerFeather-derived PCBA (ESP32-S3-WROOM module, BQ25628E-class charger,
MAX17260-class gauge, buck-boost 3.3 V rail, switchable rails, keyed connectors) is
the 2027 option, carrying the ADR 0028 bus-integrity rules.

The **LED axis is a mixed fleet by optical role** (ADR 0022): SK6812 "HEX"
direct-GPIO for close-range animation / ambient glow, and a 4 W RGBW point source for
long-throw crisp gobo projection. Both are driven **direct-GPIO off a free pin**, both
fed from the switchable 3V3 rail -- decided by instrumented A/B through
production-realistic cabling (ADR 0029 + 2026-07-11 amendment); the 4.2 V boost is
measured and shelved. The type mix by class is in the
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

As of 2026-07-15: **production is locked on COTS PowerFeather V2 at ~150 fixtures in
four classes (ADR 0024) and the buy is essentially complete** -- boards, batteries,
panels, LEDs, sensors, cabling, USB-C rescue ports, noisemaker hardware, and 172
Polycase enclosures are ordered or received (~$23.7k committed; ledger in
`ops/PROCUREMENT.md`). The battery vendor is qualified n=2 with measured dim/off/sleep
thresholds (ADRs 0025/0023); panels are selected and outdoor-measured (ADR 0026);
sensors are chosen and allocated by class (ADR 0027); the LED electrical drive
matrix is measured with the boost shelved and both LED roles decided onto the 3V3
rail by instrumented A/B (ADR 0029 + 2026-07-11 amendment); and the two-month reboot mystery closed as a bus-integrity rule, sealed by a
46-hour soak (ADR 0028).

Remaining gates before Nevada City assembly (~Aug 1): the bottom-up nightly energy
budget by role, the uplight wing design (power decision resolved 2026-07-15: hinged
solar wing + 6 Ah; the 20 Ah option died on sourcing), hat thermal/RF proof on the
Polycase boxes, the ADR 0023 state machine into production firmware, and the
noisemaker verdict. Treat LFP SOC as advisory until the gauge learns; use
coulomb counting and voltage/current guardrails. See `LOG.md` and `TODO.md` for the
live state.
