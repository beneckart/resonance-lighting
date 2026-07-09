# Procurement Ledger + Timeline

**Status:** Created 2026-07-08 from Ben's order records and LOG evidence. This is the
ledger of what was ordered when, for how much, and what is still to buy. What a
fixture is made of lives in `ops/bom.md`; the canonical fleet counts live in
`docs/block-diagram/SYSTEM.md`. Update convention: append order rows; edit Status /
Received in place as things land. Dates marked `~` are approximate (from memory or
LOG inference) -- correct them from receipts when convenient.

## Placed orders (production-scale)

| Label | Vendor | Placed | Items | Cost | Status |
|---|---|---|---|---|---|
| m5stack-samples | M5Stack | 2026-05-10 | 20x HEX + 20x NeoHEX (part of a larger R&D order) | $170 + $159 (portions) | received |
| pf-batch-1 | Elecrow | 2026-06-11 | 68x PowerFeather V2 (~$25/board) | $2,219.75 | received -- at Steve's (TN); ~70 boards there incl. his bench units |
| led-rgbw | Adafruit | 2026-06-17 | 100x 4 W RGBW warm white ($440 portion of a $690.13 order; remainder = other items, unitemized) | $440.00 | received (assumed -- confirm) |
| led-hex | M5Stack | 2026-06-17 | 70x SK6812 HEX | $633.50 | received (assumed -- confirm) |
| solar-panels | Voltaic Systems | 2026-06-24 | 110x P105-class 5 W + 50x P126-class 2 W ETFE | $3,521.99 | received (assumed -- confirm) |
| solar-pigtails | Voltaic Systems | 2026-06-24 | 160x 3.5x11 mm DC pigtails | $364.79 | received (assumed -- confirm) |
| batteries-75 | fullbattery.com | 2026-06-11 (same day the first sample qualified) | 75x 32700 LiFePO4 6 Ah | $441.70 | received |
| sensors-cables ("order A") | Adafruit | 2026-07-07 | 150x MSA311 accel + 150x tiny/small STEMMA + 100x long/xlong STEMMA cables | $1,057.54 | placed, in transit |
| tof-l5cx | Mouser | 2026-07-07 | 48x VL53L5CX multizone ToF | $1,279.03 | placed, in transit |
| tof-covers | Gilisymo (EUR) | 2026-07-07 | 60x protective optical covers for ToF apertures | ~$384.00 | placed, in transit |
| tof-tmf | SparkFun | 2026-07-07 | 100x TMF8820-mini multizone ToF | $1,422.05 | placed, in transit |
| batteries-100 | fullbattery.com | 2026-07-07 | 100x 32700 LiFePO4 6 Ah | $565.20 | placed, in transit |
| pf-batch-2 | Elecrow | pending -- invoice expected 2026-07-10, ships same day (per Elecrow rep, direct comms) | 82x PowerFeather V2 (~$30/board) | ~$2,460 (TBC) | NOT YET INVOICED |

Committed so far: **~$12,659** across the rows above; **~$15.1k** once pf-batch-2
invoices. The $440 / $170 / $159 rows are the fixture-relevant portions of larger
orders; the remainders are not itemized here.

## Small / sample orders (bench-scale, dollars mostly unrecorded)

- ~2026-05-10 -- COTS bake-off set: Elecrow PowerFeather bench boards (~5, Ben; ~3
  more with Steve; the PowerFeather portion was customs-delayed ~3 weeks and landed
  in early June, ~Jun 2-5), Adafruit Feather C6 + IS31FL3741, UnexpectedMaker
  FeatherS2 Neo, M5Stack Atom Matrix + Atomic Battery Base, DFRobot DFR0559
  (LOG 2026-05-10/15/18, 2026-06-02).
- 2026-05-15 -- 12x DFRobot SEN0291 I2C wattmeters (LOG 2026-05-15).
- ~2026-06 -- TSL2591 lux + SHT31 temp/RH (arrived ~06-11); Voltaic P105/P126 sample
  panels (in hand by the 06-15 test prep, before the bulk buy); fullbattery 32700
  6 Ah samples (first measured 06-11); Amazon Palowextra "7.2 Ah" samples (measured,
  REJECTED -- ADR 0025); 2x 20 Ah LFP cylindrical samples (batteryspace #6832
  candidate); TPS63802 boost modules; Nitecore charger. On hand from before: Seeed
  3 W panel, Apogee SQ-420 PAR meter, Anker USB bank.
- ~2026-07 -- SparkFun presence-bench kit (MLX90640, VL53L5CX, TMF8821, XM125,
  TCA9548A mux, TOF400C/VL53L1X); Adafruit KB2040; noisemaker candidates (Songle
  relay, SparkFun Qwiic Omron relay + RedBot buzzer, Arduino Modulino Buzzer/Vibro,
  piezo, 8002A amp, Adafruit STEMMA speaker #3885 at $4.76).

## To-buy queue

| Item | Est qty | Serves | Decision gate | Order-by | Risk |
|---|---|---|---|---|---|
| PowerFeather V2 batch 2 | 82 | fleet boards | none -- invoice 2026-07-10 | immediate | HIGH: the schedule long pole; escalate if not shipped 07-10 |
| JST-XH right-angle headers + pre-crimped harness | fleet-wide | LED/battery wiring (ADR 0029 fat conductors) | harness design + counts | ~mid-July | Medium |
| Grove breakout(s) | small | HEX (HY2.0 physical connector) adaptation | harness design | ~mid-July | Low |
| USB cabling + panel-mount female USB-C ports | ~40 | uplight/chandelier charge+flash port | solar-free decision | ~late July | Medium |
| 20 Ah LFP cells (batteryspace #6832) | ~40 | solar-free uplights/chandelier | CONDITIONAL: uplight energy bench on the 2 samples | decide by ~late July | Medium: lead time unverified |
| Noisemaker parts (MOSFET drivers, solenoids, relays, wire) | subset TBD | noisemaker fleet option | candidate-B bench verdict | ~late July | Low/Medium |
| Spare STEMMA speakers #3885 | 2+ | candidate-A crowd test (bench unit's trim pot died) | none | soon | Low |
| RGBW top-up (PLANNED -- "definitely buy more, they're cheap") | 20+ | chandelier mix + healthy spares | sizing only (chandelier HEX/RGBW split) | ~mid-late July | Low |
| JST 2-pin Y-cables (~$0.50 ea) | ~100 | GND tap for the VBAT LED-feed option | CONDITIONAL: RGBW feed decision (ADR 0029); verify quantity availability | with harness buy | Low/Medium |

## Lead-time picture (backward from the Aug 21 container load)

Project anchor dates from https://resonancenetwork.org/camp (gold standard,
corroborated 2026-07-08):

- **Aug 21:** CONTAINER LOAD, Nevada City -- fixtures must be operational by ~Aug 20.
- **Jul 31 - Aug 19:** NC prebuild at Bodhi Hive, Nevada City; **Aug 1-2** all-hands
  container unload; **Aug 8-9 team build: lights + camp systems** (the natural
  fixture-assembly all-hands). All parts should be on hand by ~Aug 1.
- **~Jul 20-31 (TENTATIVE):** Ben TN trip to fleet-test the ~70 boards at Steve's
  (production-firmware mesh effects + presence), back for the container unload.
- **Jul 12:** container lands, Port of Oakland.
- **Mid-late July:** last safe order window for anything with 1-2 week lead
  (batteryspace 20 Ah, USB-C ports, cabling, RGBW top-up, Y-cables).
- **2026-07-10:** Elecrow batch-2 invoice/ship date per rep. CN transit 1-2 weeks
  puts boards ~mid-to-late July -- fine if it ships on time; the single most
  schedule-critical line item.
- July-07 domestic orders (sensors, cables, batteries) should land ~mid-July.

## Procurement timeline (dated milestones)

- 2026-05-07 -- repo bootstrap; architecture ADRs 0001-0009.
- 2026-05-10 -- COTS bake-off purchases (incl. first PowerFeather bench boards,
  20+20 HEX/NeoHEX samples).
- 2026-06-02/05 -- R&D PowerFeathers land after a ~3-week customs delay; V2.R2
  bench bring-up begins.
- 2026-06-07/08 -- networking, OTA/rollback, solar path validated (ADR 0021).
- 2026-06-11 -- 32700 6 Ah first qualification (5,726 mAh); **68-board Elecrow
  order AND 75-cell battery order placed same day**.
- 2026-06-15/20 -- travel-bench solar testing (Ben in TN); codified by the 06-29
  home runs.
- 2026-06-17 -- **LED production buy**: 100x RGBW + 70x HEX (ADR 0022 same day).
- 2026-06-24 -- **panel buy**: 110x P105 + 50x P126 + 160 pigtails (ADR 0026).
- 2026-06-29 -- Voltaic outdoor MPP measurements; solar guard ships in firmware.
- 2026-07-02/03/05 -- boost campaign (ADR 0029); presence bench (5 sensors);
  bus-integrity fix + 46 h soak (ADR 0028).
- 2026-07-06/07 -- battery shootout; vendor locked (ADR 0025); thresholds
  (ADR 0023); sway/tilt bench (accel + ToF fusion); noisemaker bench (speaker synth).
- 2026-07-07 -- **sensor + cable + 100-cell battery buys** (ADR 0027).
- 2026-07-09 -- first big camp-wide meeting (noisemaker opinions, team sync).
- 2026-07-10 (planned) -- Elecrow batch-2 invoice + ship (82 boards).
- 2026-07-12 -- container lands, Port of Oakland (resonancenetwork.org/camp).
- ~2026-07-20/31 (TENTATIVE) -- Ben TN trip: ~70-board fleet test at Steve's.
- 2026-07-31/08-19 -- NC prebuild, Bodhi Hive, Nevada City; **08-01/02 container
  unload (all hands)**; **08-08/09 lights + camp systems team build**.
- 2026-08-21 -- CONTAINER LOAD, Nevada City; early crews roll 08-22/24; gates open
  08-30; burn night 09-05.

## Vendor directory

| Vendor | Supplies | Notes |
|---|---|---|
| Elecrow | PowerFeather V2 boards | Direct rep contact for batch pricing ($25 -> $30/board) |
| Adafruit | MSA311, STEMMA cables, 4 W RGBW, speakers, misc | "order A" and LED order |
| M5Stack | SK6812 HEX, NeoHEX | HY2.0/Grove physical connector |
| Voltaic Systems | P105/P126 ETFE panels, 3.5x11 mm pigtails | Panel + pigtail ecosystem match |
| fullbattery.com | 32700 LiFePO4 6 Ah (production cell) | Qualified n=2 (ADR 0025); ~$0.89/delivered-Ah |
| Mouser | VL53L5CX | |
| SparkFun | TMF8820-mini; presence-bench sensor kit | |
| Gilisymo | ToF protective optical covers | EUR pricing |
| batteryspace.com | 20 Ah LFP cylindrical (#6832) candidate; 18650 bench LFP | Conditional buy pending bench test |
| Amazon | Palowextra 32700 (REJECTED -- ADR 0025); misc bench | |
| DFRobot | SEN0291 wattmeters, DFR0559 | Bench instrumentation |
| PCBWay | NeoHEX adapter Rev A assembly quote (~$32.82/5) | Quoted 2026-05-20, never ordered |

Shipping: electronics ship domestic to Ben (CA) and Steve (TN); the staging point is
the NC prebuild at Bodhi Hive, Nevada City (earlier docs said "Grass Valley"). The
Bali bamboo container (incl. the finished chandelier structure) lands at the Port of
Oakland 2026-07-12 and is Elliot's logistics track with Michelle Satkin / Mainfreight
-- outside this repo. Project timeline gold standard: https://resonancenetwork.org/camp.
