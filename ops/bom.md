# BOM -- Fleet Bill of Materials vs Real Orders

**Status:** Updated 2026-08-08 against the Nevada City production layout. Fixture counts MIRROR the
canonical fleet table in `docs/block-diagram/SYSTEM.md` -- when counts change, update
SYSTEM.md first, then this file. Order dates, costs, and statuses live in
`ops/PROCUREMENT.md`. Current allocation: ADR 0032.

## Shared core (every fixture class)

| Item | Per fixture | Source / status | Notes |
|---|---|---|---|
| PowerFeather V2 (ESP32-S3) | 1 | Elecrow, 68 at Steve's + 90 ordered 07-09 (in transit) | ADR 0024. The controller, charger (BQ25628E), gauge (MAX17260), buck-boost, telemetry, USB-C. |
| Battery cell -- LARGE-enclosure fixtures (downlights) | 1 | **33140 LiFePO4 15 Ah, batteryhookup.com, 130 bought 07-24** (10 TN + 120 CA) | ADR 0025 annotation: new fleet standard for the large hats; QUALIFICATION PENDING (capacity/IR run + ADR 0023 threshold re-map on the new cell). |
| Battery cell -- SMALL-enclosure classes + chandelier | 1 | 32700 LiFePO4 6 Ah, fullbattery.com, 175 bought | ADR 0025. The only cell that fits the small enclosure; qualified n=2. |
| Battery lead / retention | 1 | XH cabling BOUGHT ~07-12/13 (abundance, multiple lengths) | Keyed, vibration-tolerant; no per-unit crimping (ADR 0009). Final lengths chosen at integration from the on-hand variety. |
| LED harness | 1 | XH cabling BOUGHT ~07-12/13 (same abundance; incl. 160x 5-pin Y-splitters) | RGBW feed DECIDED rail-fed (ADR 0029 amendment 2026-07-11): one harness + one pinout for both LED roles. |
| Waterproof USB-C panel-mount rescue port | 1 | Adafruit, 150 bought 07-10 | Extension cable from the PowerFeather USB-C to a gasketed panel-mount port on EVERY hat -- USB rescue/charging without opening the enclosure; solar-free classes charge through it. |
| Hat enclosure + fasteners | 1 | **BOUGHT 07-13: 172x Polycase enclosures + screws** (111 large + 61 small, incl. 2 transparent-lid demo units) | Large = **Polycase ML-70F\*15** (10x7x4 in) -> 72 downlight hats; small = **Polycase HN-57-03** (NEMA 4x, 6.7x5x3 in) -> 24 perimeter hats and candidate trunk-light enclosures. Panel flush with the lid (raised a few mm for the DC-cable bump); light + ToF flush with the bottom. Steve owns mechanical integration. Chandelier lights get a carpenter-built box (team-side). |
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
| BMP581 temp + barometric pressure | 1 on outermost ring; 0 on other rings | Adafruit, 30 bought 07-16: 24 outer-ring installs + 6 spares (ADR 0034) |

### Perimeter x24 (5 ft shepherd hooks; all HEX)

| Item | Per fixture | Source / status |
|---|---|---|
| SK6812 HEX | 1 | M5Stack, 90 bought (70+20; plus 20 NeoHEX fallback units) |
| Gobo / patterned filter ("dancing gobo") | 1 | Steve print program -- stepping the single lit HEX pixel around the board shifts the apparent pattern on the ground (corrected 07-27: perimeter DOES carry a gobo) |
| Grove/HY2.0 breakout (HEX connector adaptation) | 1 | 125 bought: 70x RobotShop 06-18 (at Steve's) + 55x Electromaker 07-10 |
| Voltaic P126-class 2 W panel + pigtail | 1 + 1 | Voltaic, 50 panels bought |
| VL53L5CX ToF, facing outward + protective cover | 1 + 1 | Mouser 48 + Gilisymo 60 covers bought |
| MSA311 accel + STEMMA cable (likely) | 1 + 1 | from the 150-accel pool |
| Shepherd hook | 1 | project-side sourcing, outside this electronics BOM |

### Trunk light x about 16 (target 16; no gobo)

| Item | Per fixture | Source / status |
|---|---|---|
| LED | 1 | Production direction is 4 W RGBW from the 150-unit pool; a smaller lensed 3 W RGB variant is under test for extra throw (ADR 0032). |
| Power source | 1 | Final trunk arrangement is being integrated in Nevada City. The 32700 6 Ah cell, small enclosures, P105 panels, and USB-C ports are available; lock the combination after the LED/throw test. |
| Gasketed panel-mount USB-C port | 1 | from the 150-port pool (bought 07-10; now a shared-core item) |
| Enclosure + mount | 1 | Candidate small Polycase from the 61-unit pool; final trunk mounting and optic protection are open. |

### Chandelier x18 (central cluster; scope still loose)

| Item | Per fixture | Source / status |
|---|---|---|
| HEX or RGBW (mix TBD) | 1 | from the HEX/RGBW pools (RGBW spares cover any mix) |
| Power source | 1 | likely 6 Ah + USB-C top-ups, low-brightness budget (20 Ah option closed 07-15) |
| Housing | shared | carpenter-built box for the 18-light cluster (team carpenter; not this BOM) |
| Sensors | none (tentative) | |

## Fleet totals + spares math (needed at nominally 130 vs bought)

| Part | Needed | Bought | Margin | Flag |
|---|---|---|---|---|
| PowerFeather V2 | 130 | 158 (68 + 90 ordered 07-09) (+~8 bench: 5 Ben, 3 Steve) | +28 production | healthy -- contingency and field spares |
| 32700 6 Ah | about 58 (perimeter + trunk + chandelier, if all use it) | 175 | about +117 | huge margin -- downlights moved to the 33140 |
| 33140 15 Ah | 72 downlights | 130 | +58 | qualification pending: capacity/IR + ADR 0023 threshold re-map |
| 4 W RGBW | 88-106 (72 downlights + up to 16 trunk + 0-18 chandelier) | 150 (100 + 50 ordered 07-10) | +44..+62 | healthy; the 3 W trunk trial may reduce need further |
| SK6812 HEX | 24-42 (24 perimeter + 0-18 chandelier) | 90 (+20 NeoHEX fallback) | +48..+66 | healthy |
| P105 5 W panel | 72-88 (72 downlights + up to 16 trunk, pending integration) | 110 | +22..+38 | healthy; trunk power choice open |
| P126 2 W panel | 24 | 50 | +26 | healthy |
| DC pigtails | 96-112 deployed panels (depending on trunk power) | 160 | +48..+64 | healthy |
| MSA311 | 96-112 (downlight + perimeter; trunk allocation TBD) | 150 | +38..+54 | healthy |
| TMF8820-mini | 72 | 100 | +28 | healthy |
| VL53L5CX | 24 | 48 | +24 | healthy |
| ToF protective covers | 24 | 60 | +36 | healthy |
| STEMMA cables | ~150-250 uses | 250 | ok | |
| USB-C panel-mount rescue ports | 130 (one per fixture) | 150 ordered 07-10 | +20 | universal rescue/charge port |
| Grove/HY2.0 breakouts | 24-42 (HEX fixtures incl. chandelier share) | 125 (70 + 55) | +83..+101 | healthy |
| Pre-crimped XH cables | ~2-4 per fixture | ~2,100+ pieces + 160 Y-splitters across lengths/colors | abundant | deliberate lead-time hedge; lengths chosen at integration |
| Enclosure, LARGE | 72 downlights | 111 (incl. 1 transparent-lid demo) | +39 | healthy |
| Enclosure, SMALL | about 40 (24 perimeter + about 16 trunk, if all trunk lights use it) | 61 (incl. 1 transparent-lid demo) | about +21 | healthy; final trunk enclosure choice open |
| ~~20 Ah LFP~~ | 0 | 2 samples (verified honest: 19,412 mAh) | -- | CANCELLED 07-15 on sourcing/timeline; Alibaba ~$4.50/cell = 2027 lead |
| MOSFET drivers (solarnoid) | downlights only, <=110 (large hats -- the solarnoid needs the space; scope settled ~07-24) | 160 (110 + 50) | +50 and likely more at 72 planned | surplus acknowledged |
| Solenoids (push-pull) | downlights only, <=110 | 150 in transit (75x 3 V + 75x 5 V) -- MAY BE RETURNED | -- | bake-off trending STRONGER (0730B 6 V/1 A primary); "solarnoid" design finalized ~07-24, mallet and all |
| Solarnoid mallets | <=110 (downlights) | craft store, bulk, very cheap (order details TBC) | -- | part of the finalized solarnoid design |
| GPS (SAM-M8Q) / RTC (DS3231) timing modules | experiment | 4 + 4 bought 07-20 | -- | dusk/dawn + sleep scheduling experiments; GPS doubles as a position anchor candidate |
| Strike caps (22,000 uF 16 V) | 1 per noisemaker fixture | 210 ordered 07-16 | abundant | VDC-tap strike storage; 22k uF = headroom for stronger solenoids; transients benign (VDC droop, reads like a passing cloud) |
| ~~Other noisemaker parts~~ | -- | 1x #3885 (damaged pot) + bench relays | -- | DECIDED 2026-07-15 (ADR 0030): solenoid bamboo-strike wins; speaker path abandoned (spares cancelled); relays/beeps not pursued |
| BMP581 env sensor | 24 outer-ring downlights | 30 ordered 07-16 | +6 | complete outer hanging ring + field spares (ADR 0034) |

Depth-sensor bookkeeping: the current allocation is 24x VL53L5CX + 72x
TMF8820-mini = 96 deployed sensors against production orders of 48 + 100. Bench
and sample units are additional.

## To-buy (summary -- live queue in ops/PROCUREMENT.md)

Remaining: trunk-light LED/optic, power, enclosure, and mounting hardware after
the lensed 3 W RGB comparison; solenoid driver control cables + mallet mounting
(strike caps ORDERED 07-16: 210x
22,000 uF 16 V; 5-pin XH Y-splitters already on hand for the VDC tap). CANCELLED 07-15:
the 20 Ah cells + end-caps (sourcing/timeline) and the spare #3885 speakers
(speaker path abandoned -- ADR 0030). DONE
since 07-08: 90 PowerFeathers (07-09), 100 MOSFET drivers + 150 solenoids
(07-10), 150 USB-C rescue ports + 50 RGBW (07-10), 125 Grove breakouts (06-18 +
07-10), XH cabling abundance + 172 Polycase enclosures (07-13).

## Open BOM inputs

- Trunk-light integration: compare 4 W RGBW with the smaller lensed 3 W RGB module,
  then lock power, mounting, enclosure, brightness, and sensor allocation.
- Small-enclosure allocation: 24 perimeter + about 16 trunk lights fits comfortably
  within the 61-unit pool if all trunk lights use the small box.
- Chandelier HEX/RGBW mix (RGBW spares now cover any split).
- Sensor allocation confirmation per class (ADR 0027 marks it tentative).
- USB-C port gasket/cutout approach per hat variant (part selected + bought 07-10;
  mechanical integration is Steve's).
- ~~RGBW feed decision~~ -- DECIDED rail-fed 2026-07-11 (ADR 0029 amendment); one
  harness + one pinout for both LED roles; the rail is the fail-safe kill.
- Harness/connector part numbers (JST-XH family).
- Solenoid strike-power source (VDC-tap + storage cap vs battery/VS pin) and
  noisemaker (solenoid, ADR 0030) scope per class -- drives the residual wiring buy.
- Spares policy for inventory beyond the nominal 130 deployed fixtures: build
  recovery, field replacement, and optional off-tree/camp use.
- Enclosure vendor/part details to record in the ledger (TBC); mechanical
  integration design (Steve): panel mount, bamboo clamp, USB-C gasket, ToF windows.
- Chandelier carpenter box: specs to coordinate (venting, access, USB charging
  reach for 18 fixtures).
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

The winning BOM is the one that closes energy and reliability while keeping fleet
assembly boring.
