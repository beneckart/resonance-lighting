# BOM -- Fleet Bill of Materials vs Real Orders

**Status:** Rewritten 2026-07-08 against actual procurement. Fixture counts MIRROR the
canonical fleet table in `docs/block-diagram/SYSTEM.md` -- when counts change, update
SYSTEM.md first, then this file. Order dates, costs, and statuses live in
`ops/PROCUREMENT.md`. Counts are tentative until installation (ADR 0024).

## Shared core (every fixture class)

| Item | Per fixture | Source / status | Notes |
|---|---|---|---|
| PowerFeather V2 (ESP32-S3) | 1 | Elecrow, 68 at Steve's + 90 ordered 07-09 (in transit) | ADR 0024. The controller, charger (BQ25628E), gauge (MAX17260), buck-boost, telemetry, USB-C. |
| 32700 LiFePO4 6 Ah cell | 1 | fullbattery.com, 175 bought | ADR 0025. One cell per fixture, every class (the 20 Ah alternative was cancelled 07-15). |
| Battery lead / retention | 1 | XH cabling BOUGHT ~07-12/13 (abundance, multiple lengths) | Keyed, vibration-tolerant; no per-unit crimping (ADR 0009). Final lengths chosen at integration from the on-hand variety. |
| LED harness | 1 | XH cabling BOUGHT ~07-12/13 (same abundance; incl. 160x 5-pin Y-splitters) | RGBW feed DECIDED rail-fed (ADR 0029 amendment 2026-07-11): one harness + one pinout for both LED roles. |
| Waterproof USB-C panel-mount rescue port | 1 | Adafruit, 150 bought 07-10 | Extension cable from the PowerFeather USB-C to a gasketed panel-mount port on EVERY hat -- USB rescue/charging without opening the enclosure; solar-free classes charge through it. |
| Hat enclosure + fasteners | 1 | **BOUGHT 07-13: 172x Polycase enclosures + screws** (111 large + 61 small, incl. 2 transparent-lid demo units) | Large -> downlight hats (<=110); small -> perimeter hats + uplight "boots" (<=60 combined). Steve owns mechanical integration (panel mount, USB-C gasket cutout, ToF windows, bamboo clamp, uplight wing hinge). Chandelier lights get a carpenter-built box (team-side). |
| Firmware | one image | this repo | Runtime/NVS config only; no per-unit builds (ADR 0009). |

## Per-class additions

### Hanging downlight x72 (7-10 ft, gobo projection)

| Item | Per fixture | Source / status |
|---|---|---|
| 4 W RGBW warm white (point source) | 1 | from the 150-RGBW pool |
| Gobo / patterned filter | 1 | Steve print program (in-house + generative bamboo-leaf patterns) |
| Voltaic P105-class 5 W panel + 3.5x11 mm pigtail | 1 + 1 | Voltaic, 110 panels + 160 pigtails bought |
| TMF8820-mini ToF, facing downward | 1 | SparkFun, 100 bought (bench-validated on same-family TMF8821) |
| MSA311 accel + STEMMA cable | 1 + 1-2 | Adafruit, 150 accels + 250 cables bought |

### Perimeter x38-40 (5 ft shepherd hooks)

| Item | Per fixture | Source / status |
|---|---|---|
| SK6812 HEX | 1 | M5Stack, 90 bought (70+20; plus 20 NeoHEX fallback units) |
| Grove/HY2.0 breakout (HEX connector adaptation) | 1 | 125 bought: 70x RobotShop 06-18 (at Steve's) + 55x Electromaker 07-10 |
| Voltaic P126-class 2 W panel + pigtail | 1 + 1 | Voltaic, 50 panels bought |
| VL53L5CX ToF, facing outward + protective cover | 1 + 1 | Mouser 48 + Gilisymo 60 covers bought |
| MSA311 accel + STEMMA cable (likely) | 1 + 1 | from the 150-accel pool |
| Shepherd hook | 1 | project-side sourcing, outside this electronics BOM |

### Uplight x24 (simple bamboo cylinder, no gobo)

| Item | Per fixture | Source / status |
|---|---|---|
| 4 W RGBW warm white | 1 | from the 100-RGBW pool |
| Power source | 1 | **RESOLVED 2026-07-15: hinged solar "wing" on the boot** (partial/shady sun; likely wants the 5 W P105) + standard 6 Ah cell; run mostly low-brightness, tuned by Nevada City prebuild experiments. (20 Ah CANCELLED on sourcing/timeline -- ADR 0025 annotation.) |
| Gasketed panel-mount USB-C port | 1 | from the 150-port pool (bought 07-10; now a shared-core item) |
| Base "boot" enclosure + wing | 1 | small Polycase from the 61-unit pool (bought 07-13) + hinged wing hardware (to-buy; Steve designs) |
| Sensors | none (tentative) | |

### Chandelier x16 (central shafts; scope still loose)

| Item | Per fixture | Source / status |
|---|---|---|
| HEX or RGBW (mix TBD) | 1 | from the HEX/RGBW pools (RGBW spares cover any mix) |
| Power source | 1 | likely 6 Ah + USB-C top-ups, low-brightness budget (20 Ah option closed 07-15) |
| Housing | shared | carpenter-built box for the 16-light cluster (team carpenter; not this BOM) |
| Sensors | none (tentative) | |

## Fleet totals + spares math (needed at 150-152 vs bought)

| Part | Needed | Bought | Margin | Flag |
|---|---|---|---|---|
| PowerFeather V2 | 150-152 | 158 (68 + 90 ordered 07-09) (+~8 bench: 5 Ben, 3 Steve) | +6..+8 production | healthy -- spares risk RESOLVED by the 90-board order |
| 32700 6 Ah | 150-152 (every class) | 175 | +23..+25 | healthy |
| 4 W RGBW | 96 + chandelier share (up to ~104) | 150 (100 + 50 ordered 07-10) | +46..+54 | healthy -- top-up DONE |
| SK6812 HEX | 38-40 + chandelier share (~46-48) | 90 (+20 NeoHEX fallback) | ~+42 | healthy |
| P105 5 W panel | ~96 (72 downlights + ~24 uplight wings, likely) | 110 | +14 | ok -- wing panel choice to confirm |
| P126 2 W panel | 38-40 | 50 | +10..+12 | ok |
| DC pigtails | = deployed panels (110-136) | 160 | +24..+50 | ok |
| MSA311 | ~110-112 | 150 | +38..+40 | healthy |
| TMF8820-mini | 72 | 100 | +28 | healthy |
| VL53L5CX | 38-40 | 48 | +8..+10 | ok |
| ToF protective covers | 38-40 | 60 | +20 | ok |
| STEMMA cables | ~150-250 uses | 250 | ok | |
| USB-C panel-mount rescue ports | 150-152 (one per fixture) | 150 ordered 07-10 | ~0 | universal rescue/charge port; margin thin but ports are only needed on deployed units |
| Grove/HY2.0 breakouts | ~46-48 (HEX fixtures incl. chandelier share) | 125 (70 + 55) | +77 | healthy |
| Pre-crimped XH cables | ~2-4 per fixture | ~2,100+ pieces + 160 Y-splitters across lengths/colors | abundant | deliberate lead-time hedge; lengths chosen at integration |
| Enclosure, LARGE | downlights, <=110 deployed (72 planned) | 111 (incl. 1 transparent-lid demo) | +38 at plan | healthy (mapping corrected 07-15: perimeter is SMALL, not large) |
| Enclosure, SMALL | perimeter + uplight boots, <=60 combined (38-40 + 24 = 62-64 planned!) | 61 (incl. 1 transparent-lid demo) | -3..-1 vs the loose plan | allocation flexes under the <=60 cap (Elliot flexible); watch at installation |
| ~~20 Ah LFP~~ | 0 | 2 samples (verified honest: 19,412 mAh) | -- | CANCELLED 07-15: sourcing/timeline; uplights go solar-wing + 6 Ah; Alibaba ~$4.50/cell = 2027 lead |
| MOSFET drivers (solenoid) | subset TBD (up to fleet) | 110 (100 ordered 07-10 + 10 prior) | -- | THE noisemaker (ADR 0030) |
| Solenoids (push-pull) | subset TBD (up to fleet) | 150 in transit (75x 3 V + 75x 5 V) -- MAY BE RETURNED | -- | bake-off trending STRONGER (0730B 6 V/1 A primary, 07-16); mounting remains |
| Strike caps (22,000 uF 16 V) | 1 per noisemaker fixture | 210 ordered 07-16 | abundant | VDC-tap strike storage; 22k uF = headroom for stronger solenoids; transients benign (VDC droop, reads like a passing cloud) |
| ~~Other noisemaker parts~~ | -- | 1x #3885 (damaged pot) + bench relays | -- | DECIDED 2026-07-15 (ADR 0030): solenoid bamboo-strike wins; speaker path abandoned (spares cancelled); relays/beeps not pursued |

Depth-sensor bookkeeping: production orders are 48x VL53L5CX + 100x TMF8820-mini;
with the bench/sample units already on hand the total is **150 depth sensors** --
parity with the 150 accelerometers.

## To-buy (summary -- live queue in ops/PROCUREMENT.md)

Remaining: uplight wing hardware (hinges + panel mount; Steve designs); solenoid
driver control cables + mallet mounting (strike caps ORDERED 07-16: 210x
22,000 uF 16 V; 5-pin XH Y-splitters already on hand for the VDC tap). CANCELLED 07-15:
the 20 Ah cells + end-caps (sourcing/timeline) and the spare #3885 speakers
(speaker path abandoned -- ADR 0030). DONE
since 07-08: 90 PowerFeathers (07-09), 100 MOSFET drivers + 150 solenoids
(07-10), 150 USB-C rescue ports + 50 RGBW (07-10), 125 Grove breakouts (06-18 +
07-10), XH cabling abundance + 172 Polycase enclosures (07-13).

## Open BOM inputs

- ~~20 Ah vs 6 Ah vs off-light-panel for uplights/chandelier~~ -- RESOLVED
  2026-07-15: hinged solar wing + 6 Ah for uplights (20 Ah cancelled on
  sourcing/timeline); chandelier likely 6 Ah + USB-C. Remaining: wing mechanical
  design, wing panel choice (likely P105 5 W), and the low-brightness budget
  (Nevada City experiments).
- Small-enclosure allocation: perimeter + boots capped at <=60 by the 61-unit
  pool, vs a loose 62-64 plan -- flexes at installation (Elliot flexible).
- Chandelier HEX/RGBW mix (RGBW spares now cover any split).
- Sensor allocation confirmation per class (ADR 0027 marks it tentative).
- USB-C port gasket/cutout approach per hat variant (part selected + bought 07-10;
  mechanical integration is Steve's).
- ~~RGBW feed decision~~ -- DECIDED rail-fed 2026-07-11 (ADR 0029 amendment); one
  harness + one pinout for both LED roles; the rail is the fail-safe kill.
- Harness/connector part numbers (JST-XH family).
- Solenoid strike-power source (VDC-tap + storage cap vs battery/VS pin) and
  noisemaker (solenoid, ADR 0030) scope per class -- drives the residual wiring buy.
- Spares policy per part once deploy counts firm up at installation -- note the
  LARGE enclosure line has effectively zero spares (111 vs 110-112 needed).
- Enclosure vendor/part details to record in the ledger (TBC); mechanical
  integration design (Steve): panel mount, bamboo clamp, USB-C gasket, ToF windows.
- Chandelier carpenter box: specs to coordinate (venting, access, USB charging
  reach for 16 fixtures).
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
