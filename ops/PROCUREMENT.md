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
| grove-robotshop | RobotShop | 2026-06-18 | 70x Grove/HY2.0 breakouts (HEX connector adaptation) -- shipped to Steve (TN) | $64.86 | received (recovered from memory 2026-07-12 -- the one forgotten order found so far) |
| solar-pigtails | Voltaic Systems | 2026-06-24 | 160x 3.5x11 mm DC pigtails | $364.79 | received (assumed -- confirm) |
| batteries-75 | fullbattery.com | 2026-06-11 (same day the first sample qualified) | 75x 32700 LiFePO4 6 Ah | $441.70 | received |
| sensors-cables ("order A") | Adafruit | 2026-07-07 | 150x MSA311 accel + 150x tiny/small STEMMA + 100x long/xlong STEMMA cables | $1,057.54 | placed, in transit |
| tof-l5cx | Mouser | 2026-07-07 | 48x VL53L5CX multizone ToF | $1,279.03 | placed, in transit |
| tof-covers | Gilisymo (EUR) | 2026-07-07 | 60x protective optical covers for ToF apertures | ~$384.00 | placed, in transit |
| tof-tmf | SparkFun | 2026-07-07 | 100x TMF8820-mini multizone ToF | $1,422.05 | placed, in transit |
| batteries-100 | fullbattery.com | 2026-07-07 | 100x 32700 LiFePO4 6 Ah | $565.20 | placed, in transit |
| pf-batch-2 | Elecrow | 2026-07-09 | 90x PowerFeather V2 ($30/board + s&h + bank fee + tariff) | $3,494.24 | placed, in transit (CN; grew from the planned 82) |
| mosfet-drivers | Adafruit | 2026-07-10 | 100x MOSFET driver (solenoid noisemaker, candidate B) | $345.00 | placed, in transit |
| solenoids | AliExpress | 2026-07-10 | 75x 3 V + 75x 5 V push-pull solenoids (voltage variants for A/B) | $319.12 | placed, in transit |
| usbc-rgbw | Adafruit | 2026-07-10 | 150x waterproof USB-C panel-mount extension cables ($540 -- a rescue/charge port on EVERY fixture, wired to the PowerFeather USB-C) + 50x 4 W RGBW top-up ($247.50); remainder ~$73 s&h/tax | $860.34 | placed, in transit |
| grove-electromaker | Electromaker | 2026-07-10 | 55x Grove/HY2.0 breakouts (125 total with the 06-18 RobotShop 70) | $85.26 | placed, in transit |
| xh-keszoox | Keszoox (Amazon) | ~2026-07-12/13 | 150x 10 cm red + 150x 10 cm black pre-crimped XH + 60x PH pigtails 10 cm (just-in-case) | $220.26 | placed, in transit |
| xh-aliexpress | AliExpress | ~2026-07-12/13 | 1,800x double-ended pre-crimped XH: 150 each of yellow/blue/black/red in 30/20/10 cm | $139.22 | placed, in transit -- watch lead time |
| xh-ysplit-1 | (TBC) | ~2026-07-12/13 | 70x JST XH 5-pin Y-splitter cables | $94.96 | placed, in transit (split TN/CA) |
| xh-ysplit-2 | (TBC) | ~2026-07-12/13 | 90x JST XH 5-pin Y-splitter cables | $120.81 | placed, in transit (split TN/CA) |
| enclosures-tn | Polycase | 2026-07-13 | 22x enclosures (11 large + 11 small) + screws, shipped to Steve (TN) | $822.67 | placed, in transit |
| enclosures-ca | Polycase | 2026-07-13 | 150x enclosures (100 large + 50 small) + screws, shipped to CA | $4,483.83 | placed, in transit |

Committed so far: **~$23,709** across the rows above (production boards 158; RGBW
150; Grove breakouts 125; enclosures 172 = 111 large + 61 small; XH cabling in
deliberate lead-time-hedged abundance). The $440 / $170 / $159 rows are the
fixture-relevant portions of larger orders; the remainders are not itemized here.

Cabling strategy note (2026-07-13): final harness lengths are unknown until the
hats + fixtures come together, so Ben ordered pre-crimped XH across MANY lengths/
colors from multiple vendors ("they are cheap") betting that a few orders land
early -- an abundance is the plan, not an accident. Additional small
receptacle/header orders (Amazon/AliExpress) exist beyond the itemized rows.

Enclosure mapping (corrected 2026-07-15): **LARGE (111) -> hanging downlights
only** (<=110 deployed); **SMALL (61) -> perimeter lights AND uplight boots**
(perimeter + boots <= 60 combined). The 16 chandelier lights get a carpenter-built
box (team-side, not this ledger), so 150 fixtures fit inside the enclosure pools
with spares back at camp; Elliot is flexible on the final light allocation. Two of
the enclosures (1 large + 1 small) have TRANSPARENT LIDS -- "show and tell" demo
models for explaining the piece to visitors at the art piece.

Uplight power RESOLVED 2026-07-15: the 20 Ah cell is OUT -- batteryspace could not
supply enough in time, and the Alibaba counterpart (a bargain at ~$4.50/cell bulk)
needs ocean freight that misses 2026 (worth remembering for 2027). Uplights instead
get a **hinged solar "wing" on the boot** drawing partial sun -- likely wanting the
5 W P105 panel -- and run mostly at low brightness on the standard 6 Ah cell,
tuned by experiments at the Nevada City prebuild.

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
  piezo, 8002A amp, Adafruit STEMMA speaker #3885 at $4.76); 10x Adafruit MOSFET
  drivers ($46, pre-fleet order -- so 110 drivers total with the 07-10 buy).

## To-buy queue

| Item | Est qty | Serves | Decision gate | Order-by | Risk |
|---|---|---|---|---|---|
| ~~PowerFeather V2 batch 2~~ | ~~82~~ | fleet boards | ORDERED 2026-07-09 as **90 boards** ($3,494.24) -- see ledger | -- | residual risk = CN transit only; track receipt |
| ~~JST-XH cabling + receptacles + headers~~ | ~~fleet-wide~~ | DONE ~2026-07-12/13 across multiple redundant orders (lead-time hedge): 300x 10 cm single-color + 1,800x multi-length/color pre-crimped + 160x 5-pin Y-splitters + 60x PH pigtails + receptacle/header smalls | -- | residual = lead times only; abundance intended |
| ~~Grove breakout(s)~~ | ~~small~~ -> 125 | DONE: 70x RobotShop 06-18 (at Steve's) + 55x Electromaker 07-10 -- covers every HEX fixture at any chandelier mix | -- | -- |
| ~~USB cabling + panel-mount USB-C ports~~ | ~~40~~ -> 150 | ORDERED 2026-07-10 ($540 portion of usbc-rgbw): waterproof panel-mount USB-C on EVERY fixture for rescue/charging, not just solar-free classes | -- | -- |
| ~~20 Ah LFP cells (batteryspace #6832)~~ | ~~40~~ | ~~solar-free uplights/chandelier~~ | **CANCELLED 2026-07-15**: batteryspace quantity short; Alibaba alternative (~$4.50/cell bulk!) needs ocean freight = misses 2026. Uplights go hinged-solar-wing + 6 Ah instead. Revisit for 2027. | -- | -- |
| ~~20 Ah end-cap connection hardware~~ | ~~40~~ | -- | CANCELLED with the cells | -- | -- |
| Uplight wing hardware (hinges, panel mount on the boot) | ~24 | hinged solar wing on the uplight boot | wing mechanical design (Steve) | ~late July | Low/Medium |
| Noisemaker strike-power + wiring residuals | subset TBD | solenoid strike supply (VDC-tap Y-cables + storage caps vs battery/VS pin), driver control cables, mallet mounting | strike-power decision (VDC-tap sweep in progress); drivers + solenoids ALREADY ORDERED 07-10 | ~late July | Low/Medium |
| ~~Spare STEMMA speakers #3885~~ | -- | -- | CANCELLED 2026-07-15: speaker path ABANDONED (ADR 0030) -- solenoid bamboo-strike wins | -- | -- |
| ~~RGBW top-up~~ | ~~20+~~ -> 50 | ORDERED 2026-07-10 ($247.50 portion of usbc-rgbw) -- 150 RGBW total, spares healthy at any chandelier mix | -- | -- |
| ~~JST 2-pin Y-cables (~$0.50 ea)~~ | ~~100~~ | GND tap for the VBAT LED-feed option | DROPPED 2026-07-11: ADR 0029 amended -- RGBW stays rail-fed (A/B lux campaign, rail +2.5 % mean). NOTE: XH 2-pin Y-cables may return as the solenoid VDC strike tap -- see the strike-power row | -- | -- |

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
  (solenoid strike-power parts; uplight wing hardware -- the harness and enclosure
  buys landed 07-12/13; the 20 Ah and #3885-spare buys are cancelled).
- **2026-07-09:** 90-board Elecrow batch ORDERED ($3,494.24). CN transit 1-2 weeks
  puts boards ~mid-to-late July; residual risk is transit, not commitment.
- July-07 domestic orders (sensors, cables, batteries) and the July-10 noisemaker
  orders should land ~mid-July (AliExpress solenoids possibly later -- watch).

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
- 2026-06-18 -- 70x Grove breakouts (RobotShop, to Steve/TN).
- 2026-06-24 -- **panel buy**: 110x P105 + 50x P126 + 160 pigtails (ADR 0026).
- 2026-06-29 -- Voltaic outdoor MPP measurements; solar guard ships in firmware.
- 2026-07-02/03/05 -- boost campaign (ADR 0029); presence bench (5 sensors);
  bus-integrity fix + 46 h soak (ADR 0028).
- 2026-07-06/07 -- battery shootout; vendor locked (ADR 0025); thresholds
  (ADR 0023); sway/tilt bench (accel + ToF fusion); noisemaker bench (speaker synth).
- 2026-07-07 -- **sensor + cable + 100-cell battery buys** (ADR 0027).
- 2026-07-09 -- first big camp-wide meeting (noisemaker opinions, team sync);
  **90-board Elecrow batch-2 ordered** ($3,494.24 -- grew from the planned 82).
- 2026-07-10 -- **noisemaker fleet buys**: 100x MOSFET drivers (Adafruit, $345) +
  150x solenoids (75x 3 V + 75x 5 V, AliExpress, $319.12); solenoid candidate-B
  first bench same day (815 strikes, no resets). Also **150x waterproof USB-C
  panel-mount rescue ports + 50x RGBW top-up** (Adafruit, $860.34) -- the USB
  rescue/charge port goes universal, one per fixture -- and 55x Grove breakouts
  (Electromaker, $85.26).
- 2026-07-11 -- RGBW feed DECIDED rail-fed (ADR 0029 amendment, instrumented A/B);
  harness buy unblocked.
- 2026-07-12 -- container lands, Port of Oakland (resonancenetwork.org/camp);
  **20 Ah sample 1 verified** (19,412 mAh = 97.1 % of label) -- sample 2 gates the
  ~40-cell buy.
- ~2026-07-12/13 -- **harness + enclosure wave**: pre-crimped XH cabling in
  lead-time-hedged abundance (~$575 across 4+ orders) and **172 Polycase
  enclosures** (111 large + 61 small + screws, $5,306.50, both orders placed
  07-13, split TN/CA; incl. 2 transparent-lid demo units). Chandelier housing =
  carpenter-built box (team-side).
- 2026-07-15 -- **uplight power RESOLVED: hinged solar wing + 6 Ah** (20 Ah
  cancelled on sourcing/timeline; Alibaba ~$4.50/cell noted for 2027). Enclosure
  mapping corrected: large -> downlights only; small -> perimeter + boots.
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
| batteryspace.com | 20 Ah LFP cylindrical (#6832) samples; 18650 bench LFP | 20 Ah bulk buy CANCELLED 07-15 (quantity short); Alibaba ~$4.50/cell is the 2027 lead |
| Polycase | 172x enclosures + screws (111 large + 61 small; 2 transparent-lid demo units) | Both orders 2026-07-13 |
| Amazon | Palowextra 32700 (REJECTED -- ADR 0025); misc bench | |
| AliExpress | Push-pull solenoids (75x 3 V + 75x 5 V) | Noisemaker candidate B; watch transit time |
| DFRobot | SEN0291 wattmeters, DFR0559 | Bench instrumentation |
| RobotShop | Grove/HY2.0 breakouts (70, at Steve's) | |
| Electromaker | Grove/HY2.0 breakouts (55) | |
| Keszoox (Amazon) | Pre-crimped XH cables, PH pigtails | Multiple lengths/colors |
| PCBWay | NeoHEX adapter Rev A assembly quote (~$32.82/5) | Quoted 2026-05-20, never ordered |

Shipping: electronics ship domestic to Ben (CA) and Steve (TN); the staging point is
the NC prebuild at Bodhi Hive, Nevada City (earlier docs said "Grass Valley"). The
Bali bamboo container (incl. the finished chandelier structure) lands at the Port of
Oakland 2026-07-12 and is Elliot's logistics track with Michelle Satkin / Mainfreight
-- outside this repo. Project timeline gold standard: https://resonancenetwork.org/camp.
