# BOM -- Fleet Bill of Materials vs Real Orders

**Status:** Rewritten 2026-07-08 against actual procurement. Fixture counts MIRROR the
canonical fleet table in `docs/block-diagram/SYSTEM.md` -- when counts change, update
SYSTEM.md first, then this file. Order dates, costs, and statuses live in
`ops/PROCUREMENT.md`. Counts are tentative until installation (ADR 0024).

## Shared core (every fixture class)

| Item | Per fixture | Source / status | Notes |
|---|---|---|---|
| PowerFeather V2 (ESP32-S3) | 1 | Elecrow, 68 received-class + 82 invoicing 07-10 | ADR 0024. The controller, charger (BQ25628E), gauge (MAX17260), buck-boost, telemetry, USB-C. |
| 32700 LiFePO4 6 Ah cell | 1 | fullbattery.com, 175 bought | ADR 0025. Solar-free classes may swap to 20 Ah (OPEN). |
| Battery lead / retention | 1 | to-buy (JST-XH pre-crimped) | Keyed, vibration-tolerant; no per-unit crimping (ADR 0009). |
| LED harness | 1 | to-buy (JST-XH right-angle set) | Forks on the RGBW feed decision (3V3 rail as wired vs VBAT-direct + Y-cable GND tap) -- decide before ordering (ADR 0029). |
| Hat enclosure + fasteners | 1 | Steve (print/MJF); design in progress | Four variants: downlight hat, perimeter hat, uplight "boot", chandelier hat. |
| Firmware | one image | this repo | Runtime/NVS config only; no per-unit builds (ADR 0009). |

## Per-class additions

### Hanging downlight x72 (7-10 ft, gobo projection)

| Item | Per fixture | Source / status |
|---|---|---|
| 4 W RGBW warm white (point source) | 1 | Adafruit, 100 bought |
| Gobo / patterned filter | 1 | Steve print program (in-house + generative bamboo-leaf patterns) |
| Voltaic P105-class 5 W panel + 3.5x11 mm pigtail | 1 + 1 | Voltaic, 110 panels + 160 pigtails bought |
| TMF8820-mini ToF, facing downward | 1 | SparkFun, 100 bought (bench-validated on same-family TMF8821) |
| MSA311 accel + STEMMA cable | 1 + 1-2 | Adafruit, 150 accels + 250 cables bought |

### Perimeter x38-40 (5 ft shepherd hooks)

| Item | Per fixture | Source / status |
|---|---|---|
| SK6812 HEX | 1 | M5Stack, 90 bought (70+20; plus 20 NeoHEX fallback units) |
| Voltaic P126-class 2 W panel + pigtail | 1 + 1 | Voltaic, 50 panels bought |
| VL53L5CX ToF, facing outward + protective cover | 1 + 1 | Mouser 48 + Gilisymo 60 covers bought |
| MSA311 accel + STEMMA cable (likely) | 1 + 1 | from the 150-accel pool |
| Shepherd hook | 1 | project-side sourcing, outside this electronics BOM |

### Uplight x24 (simple bamboo cylinder, no gobo)

| Item | Per fixture | Source / status |
|---|---|---|
| 4 W RGBW warm white | 1 | from the 100-RGBW pool |
| Power source | 1 | OPEN (ADRs 0025/0026): off-light P105 panel vs 20 Ah LFP in-cylinder (batteryspace #6832, bench-gated) vs budgeted 6 Ah |
| Gasketed panel-mount USB-C port + USB cabling | 1 | to-buy (if solar-free wins) |
| Base "boot" enclosure | 1 | Steve; battery may fill the bamboo cylinder, LED near the lit end |
| Sensors | none (tentative) | |

### Chandelier x16 (central shafts; scope still loose)

| Item | Per fixture | Source / status |
|---|---|---|
| HEX or RGBW (mix TBD) | 1 | from the HEX/RGBW pools; mix drives a possible RGBW top-up |
| Power source | 1 | likely solar-free + USB-C, like uplights (OPEN) |
| Hat (similar to uplight boot; shafts packed closely) | 1 | Steve; possibly a distinct enclosure |
| Sensors | none (tentative) | |

## Fleet totals + spares math (needed at 150-152 vs bought)

| Part | Needed | Bought | Margin | Flag |
|---|---|---|---|---|
| PowerFeather V2 | 150-152 | 150 (+~8 bench: 5 Ben, 3 Steve) | -2..0 production | **THIN** -- top-up order likely if Elecrow allows; risk-register item |
| 32700 6 Ah | 150-152 (drops to ~112 if 20 Ah takes uplights+chandelier) | 175 | +23..+63 | healthy |
| 4 W RGBW | 96 + chandelier share (up to ~104) | 100 | -4..+4 today | top-up PLANNED (cheap; 20+ units); chandelier mix sizes it |
| SK6812 HEX | 38-40 + chandelier share (~46-48) | 90 (+20 NeoHEX fallback) | ~+42 | healthy |
| P105 5 W panel | 72 (+24 if uplights go solar) | 110 | +14..+38 | healthy |
| P126 2 W panel | 38-40 | 50 | +10..+12 | ok |
| DC pigtails | = deployed panels (110-136) | 160 | +24..+50 | ok |
| MSA311 | ~110-112 | 150 | +38..+40 | healthy |
| TMF8820-mini | 72 | 100 | +28 | healthy |
| VL53L5CX | 38-40 | 48 | +8..+10 | ok |
| ToF protective covers | 38-40 | 60 | +20 | ok |
| STEMMA cables | ~150-250 uses | 250 | ok | |
| 20 Ah LFP | 0 or ~40 | 2 samples | conditional | bench test gates the buy |
| Noisemaker parts | subset TBD | 1x #3885 (damaged pot) + bench relays | -- | all options live incl. relays/beeps; camp-meeting input 07-09 |

Depth-sensor bookkeeping: production orders are 48x VL53L5CX + 100x TMF8820-mini;
with the bench/sample units already on hand the total is **150 depth sensors** --
parity with the 150 accelerometers.

## To-buy (summary -- live queue in ops/PROCUREMENT.md)

JST-XH right-angle headers + pre-crimped harness (feed-decision-gated); Grove
breakout(s); 82 PowerFeathers (invoicing 07-10); USB cabling + panel-mount USB-C
ports; conditional ~40x 20 Ah LFP; conditional ~100x JST 2-pin Y-cables (VBAT feed
option); noisemaker parts; spare #3885 speakers; planned RGBW top-up (20+).

## Open BOM inputs

- 20 Ah vs 6 Ah vs off-light-panel for uplights/chandelier (bench test on samples).
- Chandelier HEX/RGBW mix (drives the RGBW top-up question).
- Sensor allocation confirmation per class (ADR 0027 marks it tentative).
- USB-C panel-mount part selection + gasket approach.
- **RGBW feed decision (ADR 0029): 3V3 rail as wired vs VBAT-direct** -- forks the
  harness set, the firmware pinout (A0 vs D13), the fail-safe design, and the
  Y-cable buy. Decide before the harness order.
- Harness/connector part numbers (JST-XH family) and, if VBAT-fed, the default-off
  kill element (ADR 0029 open implementation detail).
- Spares policy per part once deploy counts firm up at installation.
- Shepherd-hook sourcing (project-side).

## Superseded

The 2026-06-17 "working procurement skeleton" version of this file (pre-purchase
Track A/B tables, costing guidance) is preserved in git history. Its two tracks
resolved to COTS production (ADR 0024); its open procurement inputs are either
executed (see `ops/PROCUREMENT.md`) or carried in the lists above. The still-useful
costing rule survives here:

```
total_cost = parts + shipping + spares + assembly labor + QA/rework allowance
ops_risk = solder joints + crimps + one-off configs + fragile connectors + field access
```

The winning BOM is the one that closes energy and reliability while keeping 150-unit
assembly boring.
