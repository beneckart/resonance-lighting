# Glossary

Terms and proper nouns used throughout this repo, in case an agent or human is dropping in cold.

## Project

- **Resonance Tree** -- The overall art project. ~7.5 m bamboo installation for Burning Man 2026. Reused and expanded in 2027 (conch shell built around the trunk).
- **Resonance** -- Short for the project. Also the parent art collective.
- **Resonance Lighting** -- This repo's scope. The ~150-fixture lighting fleet (four classes; canonical counts in `docs/block-diagram/SYSTEM.md`, ADR 0024).
- **Downlight** -- Hanging lantern fixture inside the tree (72 planned, 7-10 ft). Bamboo lantern + electronics hat + filter + 4 W RGBW + downward ToF.
- **Perimeter light** -- HEX fixture on a ~5 ft shepherd hook around the piece (38-40 planned), with an outward-facing ToF to catch passers-by.
- **Uplight** -- Ground-pointing-up fixture on a simple bamboo cylinder (24 planned). RGBW, no gobo; small Polycase "boot" at the base with a hinged solar "wing" (likely P105 5 W; decided 2026-07-15), 6 Ah cell, gasketed USB-C port; runs a low-brightness budget tuned at the NC prebuild.
- **Chandelier light** -- One of 16 lights in the central chandelier cluster's bamboo shafts. HEX/RGBW mix TBD; likely 6 Ah + USB-C top-ups, housed in a carpenter-built box. Scope/ownership still loose (ADR 0024).
- **Hat** -- The sealed solar/electronics enclosure that mounts on top of each bamboo lantern. Since 2026-07-13 the bodies are bought Polycase boxes (111 large -> downlights; 61 small -> perimeter + uplight boots; 2 transparent-lid demo units); Steve owns the mechanical integration. Chandelier lights use a carpenter-built box instead.
- **Filter** / **gobo** -- Patterned-aperture insert that sits at the bamboo node notch. Casts mandala shadows on the ground. Two physical forms: flat disc (the likely production default -- simpler and less brittle) and projective cone (prototype; may be used for a few designs or none). Pattern program: in-house + generative bamboo-leaf designs (community submissions pulled 2026-07-08).
- **Wand** -- A hand-carryable lantern variant (proposed) that participants can take through the piece. Tree fixtures react to its proximity.
- **Chandelier** -- The larger assembly at the top of the tree, sharing a wind chime cluster with a 0.8 m solar panel. Vishnu's design; structure built and in the shipping container. Its 16 light shafts are tentatively this repo's fleet class (see Chandelier light).

## People

- **Elliot Fabri** -- Resonance project lead. Oakland -> Bali. All final calls.
- **Ed Wilkes** -- Structural engineer. Bristol. Owns the Rhino/Grasshopper master model.
- **Vishnu V** -- Lighting designer. India. Owns the bamboo lantern shop drawing, wind chime, chandelier.
- **Ben Eckart** -- Power systems, firmware, primary donor for Resonance Lighting. NVIDIA / US.
- **Steve Eckart** -- Ben's dad. CAD wizard, retired. Owns enclosure design.
- **Luis Echeverria** -- Bamboo U GM, Bamboo Pure liaison. Bali.
- **Dipta Priyatna** -- Bamboo Pure production lead. Bali.
- **Galang, Iwel, Zaki** -- Bamboo Pure team (production, model maker, content).
- **Josie New Numbee** -- Renderings.
- **Michelle Satkin** -- Mainfreight shipping rep.

## Organizations

- **Bamboo Pure** -- Bali fabricator building the tree and the bamboo lantern bodies (100 in BOQ R6, with ample extras).
- **Bamboo U** -- Bali institution / school where Luis is GM.
- **18th Street Arts Center** -- Fiscal sponsor (Jan Williamson + Stephen Sacks).
- **Burning Man org / BMHQ** -- Burning Man corporate. Katie Hazard + Peter are the project's contacts.

## Agents and tools

- **Co-Work** -- Elliot's PM agent. Maintains the Resonance project wiki from WhatsApp threads + Fireflies meeting transcripts. Refresh cycle every 3 hours. Currently self-hosted on Elliot's laptop; planned to move to cloud.
- **Resonance Agentic Wiki** -- The WhatsApp group where Co-Work answers questions about the project. Also a name for the wiki Co-Work maintains.
- **Cowork** -- The Anthropic product Ben is using for project management and review (this side of things). Distinct from Co-Work, the project's PM agent. Confusing namespace.
- **Claude Code** -- The Anthropic product Ben (and probably Steve) will use for daily code/CAD iteration.

## Technical terms specific to this project

- **Brain v2.0** -- The microcontroller used in the 2018 Talisman v2 build. TTGO T-Beam (ESP32 + LoRa + GPS + 18650 + LiPo charger).
- **Talisman** -- Ben's 2017-2018 Burning Man wearable pendant project. ESP32 + LoRa mesh + LED display showing friend locations.
- **Marquee** -- Ben's 2018 Burning Man piece using ESP32 (TTGO T-Ice) driving 240 WS2812B LEDs over WiFi via Open Pixel Control.
- **Marauder's Map** -- Ben's 2018 Burning Man piece. 3 ft x 3 ft solar-powered laser-etched map of BRC backlit by 450 LEDs at road junctions.
- **future-robotics** -- The github.com/beneckart/future-robotics repo containing prior project code.
- **Mystery white-enclosure board** -- Initially unknown, identified as **TTGO T-Ice** (LilyGO, discontinued ESP32+WS2812B-driver board with white snap-on case).

## Physical / temporal

- **Playa** -- The dry lakebed at Black Rock Desert where Burning Man happens.
- **BRC** -- Black Rock City, the temporary city erected on the playa.
- **Grass Valley / Nevada City** -- The project's pre-build staging area in Northern California (twin towns), where bamboo from the Bali sea container meets the electronics before trucking to BRC. The 2026 prebuild site is **Bodhi Hive, Nevada City** (Jul 31 - Aug 19; container unload Aug 1-2; lights team build Aug 8-9; container load Aug 21 -- per resonancenetwork.org/camp). Early repo docs say "Grass Valley" for this.
- **Bodhi Hive** -- The Nevada City venue hosting the 2026 NC prebuild.
- **The Man** -- The central wooden effigy at Burning Man, used as the cardinal landmark for orientation. The Talisman's GPS navigation was relative to "the Man."
- **Burn week** / **Build week** -- Late August through early September 2026 (and 2027). Build week starts roughly 1 week before the burn proper.
- **Default world** -- Burning Man slang for the regular world outside the playa.

## Acronyms

- **ADR** -- Architectural Decision Record. Files in `docs/decisions/`.
- **BLE** -- Bluetooth Low Energy.
- **BOM** -- Bill of Materials.
- **BOQ** -- Bill of Quantities (used by Bamboo Pure for the tree fab).
- **CA** -- Cellular Automaton / Automata.
- **DMA** -- Direct Memory Access.
- **EDA** -- Electronic Design Automation.
- **FCC** -- US Federal Communications Commission. Use pre-certified Espressif modules
  such as ESP32-S3-WROOM-class parts; avoid custom RF.
- **GPIO** -- General-Purpose Input/Output (pin).
- **IDE** -- Integrated Development Environment.
- **I2S** -- Inter-IC Sound bus. ESP32 uses it for parallel WS2812B output via DMA.
- **JLCPCB** -- Chinese PCB fab + assembly service. The old custom-PCBA plan's assembler; moot for 2026 now that production is COTS (ADR 0024).
- **LDO** -- Low-Dropout regulator.
- **LiPo** -- Lithium Polymer (Li-ion variant, 4.2 V max charge).
- **LiFePO4** -- Lithium Iron Phosphate (3.6 V max charge, our chemistry).
- **MCU** -- Microcontroller Unit.
- **MJF** -- Multi Jet Fusion (HP's powder-bed-fusion 3D printing process). Was the planned hat production technology; superseded by the bought Polycase boxes (2026-07-13) -- still an option for gobo/fitting batches.
- **MPP** / **MPPT** -- Maximum Power Point / MPP Tracking (solar panel optimization).
- **OPC** -- Open Pixel Control (LED streaming protocol used in Marquee).
- **OTA** -- Over-the-Air firmware update.
- **PCB** / **PCBA** -- Printed Circuit Board / PCB Assembly.
- **PDR** -- Packet Delivery Ratio (per-source, from ESP-NOW sequence numbers).
- **PWM** -- Pulse-Width Modulation.
- **RSSI** -- Received Signal Strength Indicator.
- **RTOS** -- Real-Time Operating System (FreeRTOS on ESP32).
- **SOC** -- State Of Charge. On the LFP plateau treat gauge SOC as advisory only.
- **ToF** -- Time of Flight (optical distance sensor; "multizone" = a small depth grid per frame).
- **VINDPM** -- charger input-voltage regulation setpoint (the BQ25628E knob MPP sweeps adjust, `m46` = 4.6 V).
- **OVP** / **HIZ** -- Input Over-Voltage Protection / high-impedance input state on the BQ25628E. The bright-sun latch fix (solar guard) toggles these.
- **BATFET** -- The charger's battery switch. Corrupted power-path registers can open it: instant battery-only `poweron` reset (ADR 0028).
- **WROOM** -- Espressif's pre-certified ESP32 module family (ESP32-S3-WROOM-1 on the PowerFeather).

## Current hardware stack (2026 production -- ADRs 0024-0029)

- **PowerFeather V2** -- ESP32-S3 controller board with solar charger, fuel gauge, and switchable rails; the production COTS board (Elecrow, 150 bought/committed).
- **BQ25628E** -- TI solar charger / power-path IC on the PowerFeather. Buck-only: panel hot Vmp must be >= 4.6 V.
- **MAX17260** -- Fuel gauge IC. Known traits: +8 % current bias (/1.08 correction), no cold-POR off a deeply discharged cell, LFP-plateau-blind SOC.
- **TPS631013** -- The PowerFeather's 3.3 V buck-boost rail regulator.
- **32700** -- Large cylindrical cell format (32 mm dia x 70 mm). Production cell: fullbattery.com LiFePO4 6 Ah, qualified n=2 at ~5.75 Ah (ADR 0025).
- **fullbattery.com** -- Production battery vendor. The Amazon "Palowextra 7.2 Ah" alternative measured 78 % of label with 2.3x IR and was rejected.
- **Voltaic P105 / P126** -- ETFE-laminated solar panels, 5 W / 2 W: P105 for downlights, P126 for perimeter fixtures (ADR 0026). ETFE = the tough fluoropolymer front layer.
- **SK6812 "HEX"** -- M5Stack 37-LED hexagonal addressable board; the close-range/ambient LED role, fed from the switchable 3V3 rail.
- **NeoHEX** -- M5Stack WS2812C-2020 hex board; least-efficient fallback (20 on hand).
- **4 W RGBW** -- Adafruit warm-white 4 W RGBW point-source emitter; the crisp-gobo long-throw role. Fed from the switchable 3V3 rail, same as the HEX -- decided by instrumented A/B 2026-07-11 (ADR 0029 amendment).
- **MSA311** -- Adafruit STEMMA 3-axis accelerometer; per-fixture sway/tilt sensing, no per-unit calibration (ADR 0027).
- **TMF8820-mini** -- AMS 3x3 multizone ToF (SparkFun mini breakout); downward presence sensor on downlights (bench-validated on the same-family TMF8821).
- **VL53L5CX** -- ST multizone ToF (up to 8x8); outward presence sensor on perimeter fixtures; 60 protective optical covers bought (Gilisymo).
- **STEMMA-QT / Qwiic** -- JST-SH 4-pin I2C connector standard used by the sensor boards.
- **Grove / HY2.0** -- M5Stack's physical connector family (carries GPIO data for the HEX, not I2C).
- **JST-XH** -- Keyed wire-to-board connector family planned for battery/LED harnesses (right-angle headers + pre-crimped cables in the to-buy queue).
- **TCA9548A** -- I2C mux used on the presence bench to host same-address sensors.
- **#6832** -- batteryspace.com product ID for the 20 Ah LFP cylindrical cell. Verified honest (19,412 mAh, 2026-07-12) but the bulk buy was CANCELLED 2026-07-15 on sourcing/timeline; uplights use a hinged solar wing + 6 Ah instead. The ~$4.50/cell Alibaba equivalent is a 2027 idea.

## Firmware / bench terms

- **solar guard** -- `firmware/powerfeather_solar_guard.h`: forces wide VBUS_OVP and kicks a HIZ requalification when a bright-sun connect latches the charger input off. Baseline in every charging sketch.
- **field-cycle** -- net_bench's day/night lifecycle mode: charge -> wait-dark -> draw -> protect, with ADR 0023 low-battery thresholds.
- **maintenance mode** -- OTA path: an ESP-NOW metadata packet (`U` fleet / `U<id>` targeted) sends a fixture onto shared WiFi where it serves `/update` and `/telemetry`; `ops/bench/net_bench_ota.py` uploads in parallel. The self-hosted `--maint-ap` fallback is deprecated.
- **A/B rollback** -- Standard ESP32 dual-partition OTA: a new image must pass `verifyOta()` (C linkage!) or the bootloader reverts to the last-good image.
- **heartbeat** -- The ~1-2 Hz ESP-NOW broadcast state packet (id/seq/battery/PDR/RSSI + telemetry tails), kept <= 128 bytes for bridge compat.
- **Bench apps** -- `net_bench` (mesh/OTA/field-cycle; closest to production firmware), `power_bench` (charger/gauge matrix), `led_studio` (LED looks), `presence_bench` (5-sensor rig), `sway_demo` (accel+ToF fusion), `speaker_demo` / `clacker_demo` (noisemakers), `smoke_test` (acceptance), `wifi_diag` (RF probe), `ina_monitor` (external coulomb/lux ground truth).
