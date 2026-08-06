# Resonance Lighting

Power and lighting workstream for the **Resonance Tree** -- a bamboo art installation for Burning Man 2026 + 2027. This repo covers the 130 planned solar/battery-powered, mesh-networked lighting fixtures in and around the tree -- four physical roles: hanging downlights, perimeter lights, trunk/uplights, and the central chandelier (canonical counts: the fleet table in `docs/block-diagram/SYSTEM.md`; decision record: ADR 0032).

Sister tracks (not in this repo): bamboo structure (Bamboo Pure, Bali), structural engineering (Ed), parametric lighting design (Vishnu), project management (Elliot + Co-Work agent).

## Who's working here

- **Ben Eckart** -- power systems, firmware, mesh networking. Owns `/firmware/` and `/hardware/`.
- **Steve Eckart** -- enclosure design, 3D printing, mechanical fit. Owns `/enclosure/`.

Both work with AI pair-programmers. Coordinate via `LOG.md`, `TODO.md`, and ADRs.

## What is the deliverable

130 modular lighting fixtures in the Nevada City production plan -- 72 hanging
downlights (three rings of 24), 24 all-HEX perimeter lights, 16 trunk/uplights moving
toward all RGBW, and 18 mixed HEX/RGBW chandelier lights. Counts may decrease only if
an unforeseen installation issue forces fewer lights. The archetype (hanging
downlight) is:

- A bamboo lantern body, fabricated by Bamboo Pure in Bali.
- A solar "hat" enclosure that sits partially inside, partially over the bamboo top.
- A solar panel, rechargeable battery, controller board, LED module, sensors
  (accelerometer + downward ToF), and a 3D-printed patterned aperture / gobo.
- Firmware that supports autonomous ambient lighting, ESP-NOW state exchange, standard OTA maintenance updates, telemetry, and graceful low-power behavior.

The other classes are variants on the same electronics: perimeter lights are all HEX
and face the ToF outward; trunk/uplights drop the gobo and carry a hinged solar "wing"
on their base boot (decided 2026-07-15), with a smaller-die 3 W RGB + lens still being
tested for extra throw; chandelier lights live in a carpenter-built box, likely on
6 Ah cells with USB-C top-ups. All share one firmware image. The firmware/wire class
name stays `uplight`; `trunk` is the physical-role alias.

## Current architecture direction

**PowerFeather V2 (ESP32-S3) is the confirmed reference** for the controller / solar-and-battery manager / telemetry, after 5-board feasibility testing (ADR 0021): ESP-NOW mesh at scale, battery-only no-touch OTA + A/B rollback, and the solar charge path are all validated on hardware. Chemistry is **LiFePO4** (ADR 0002); batteries are two-tier since 2026-07-24 (ADR 0025): 33140 15 Ah in the large hats (downlights; qualification pending) and the fullbattery 32700 6 Ah, qualified n=2, in the small hats.

**The production path is decided: COTS PowerFeather V2, with 130 deployed fixtures
(ADRs 0024 and 0032).**
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
were rejected (per-device calibration doesn't scale to a 130-fixture fleet). The **noisemaker**
is decided (ADR 0030): a solenoid mallet physically strikes the bamboo -- daytime
solar-surplus percussion; the speaker-synth path was abandoned once the strikes
proved out.

**Production show timing is scheduled** (ADR 0031): four purchased SAM-M8Q modules
are the initial GPS/GNSS soft anchors for absolute UTC, and four purchased Adafruit
DS3231 modules are the initial battery-backed RTC holdover anchors. ESP-NOW distributes
time quality to the rest of the fleet, so all 130 fixtures do not need RTCs.
Panel/lux dusk inference remains useful bench telemetry but is not the production show
clock.

The old custom-board target of ESP32-C3-MINI-1 + CN3058 + AP2112K + direct-from-battery WS2812B has been superseded by later ADRs.

## Goals

- **Fungible:** any unit replaces any other with no per-device configuration.
- **Fully wireless:** no data lines, no power lines, no fixed topology. ESP-NOW is for lightweight state/control packets, not firmware-image transfer.
- **Standard OTA only:** OTA updates use normal ESP32 OTA mechanisms in a deliberate maintenance mode. No custom mesh-gossiped firmware images.
- **Durable infrastructure:** fixtures are reused in 2026 and 2027.
- **Low per-fixture operations:** no skilled repetitive work at 130-fixture deployment scale. Small, deliberate soldering such as a solar pigtail can be acceptable; hand-soldering rows of headers or hand-crimping harnesses is not.
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
|   |-- decisions/         ADRs 0001-0032
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

As of 2026-08-06: **production is locked on COTS PowerFeather V2, and the Nevada City
build has converged on 130 fixtures in four physical roles (ADRs 0024 and 0032)** --
boards, two battery
tiers, panels, LEDs, sensors, cabling, USB-C rescue ports, solarnoid hardware, and 172
Polycase enclosures are ordered or received (about $25.2k committed; ledger in
`ops/PROCUREMENT.md`). The qualified 32700 6 Ah cell remains the small-hat/chandelier
battery while 130 newly bought 33140 15 Ah cells are pending qualification for large
hats (ADR 0025). Panels are selected and outdoor-measured (ADR 0026); sensors are
allocated by class (ADR 0027); both LED roles are on the switchable 3V3 rail after
instrumented A/B (ADR 0029); the solarnoid is the selected noisemaker for large-hat
downlights (ADR 0030); and deterministic scheduled shows from sparse GPS/RTC anchors
are the production timing direction (ADR 0031).

Remaining gates before Nevada City assembly (~Aug 1): the bottom-up nightly energy
budget by role, 33140 qualification and thresholds, uplight-wing mechanics and
brightness budget, hat thermal/RF proof on the Polycase boxes, the ADR 0023 state
machine in production firmware, solarnoid part/mounting details, and qualification of
the four SAM-M8Q GPS plus four DS3231 RTC timing anchors including invalid-time
behavior. Treat LFP SOC as advisory until the gauge learns; use coulomb counting and
voltage/current guardrails. See `LOG.md` and `TODO.md` for the live state.
