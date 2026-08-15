# Glossary

Terms and proper nouns used throughout this repo, in case an agent or human is dropping in cold.

## Project

- **Resonance Tree** -- The overall art project. ~7.5 m bamboo installation for Burning Man 2026. Reused and expanded in 2027 (conch shell built around the trunk).
- **Resonance** -- Short for the project. Also the parent art collective.
- **Resonance Lighting** -- This repo's scope. The nominal 130-fixture lighting fleet (four classes; canonical counts in `docs/block-diagram/SYSTEM.md`, ADR 0032).
- **Downlight** -- Hanging lantern fixture inside the tree (72 planned in three rings of 24, 7-10 ft). Bamboo lantern + electronics hat + filter + 4 W RGBW + downward ToF.
- **Perimeter light** -- One of 24 all-HEX fixtures on ~5 ft shepherd hooks around the piece, with an outward-facing ToF to catch passers-by.
- **Trunk light** -- One of about 16 no-gobo fixtures integrated with the trunk. Production is trending all 4 W RGBW; a smaller lensed 3 W RGB variant is under test for extra throw. Final power, mounting, enclosure, and sensor allocation are open (ADR 0032).
- **Uplight** -- Legacy planning/firmware name for the class that evolved into the current trunk lights. The former 24-uplight hinged-wing allocation is superseded by ADR 0032.
- **Chandelier light** -- One of 18 lights in the central chandelier cluster. HEX/RGBW mix TBD; likely 6 Ah + USB-C top-ups, housed in a carpenter-built box. Scope/ownership still loose (ADR 0032).
- **Hat** -- The sealed solar/electronics enclosure that mounts on top of each bamboo lantern. Since 2026-07-13 the bodies are bought Polycase boxes (111 large **ML-70F\*15** 10x7x4 in -> downlights; 61 small **HN-57-03** NEMA 4x 6.7x5x3 in -> perimeter + candidate trunk-light enclosures; 2 transparent-lid demo units); panel flush with the lid, light + ToF flush with the bottom; Steve owns the mechanical integration. Chandelier lights use a carpenter-built box instead.
- **Filter** / **gobo** -- Patterned-aperture insert that sits at the bamboo node notch. Casts mandala shadows on the ground. Two physical forms: flat disc (the likely production default -- simpler and less brittle) and projective cone (prototype; may be used for a few designs or none). Pattern program: in-house + generative bamboo-leaf designs (community submissions pulled 2026-07-08). Role assignment (corrected 2026-07-27): downlights AND perimeter carry gobos -- perimeter is the "dancing gobo" (stepping the single lit HEX pixel around the board shifts the apparent pattern on the ground); trunk lights (legacy `uplight` firmware class) and chandelier carry none. Both gobo roles use the exact same bamboo housing (only the enclosure size differs), so the source-to-gobo drop is 6 in on both (confirmed 2026-07-27).
- **Wand** -- A hand-carryable lantern variant (proposed) that participants can take through the piece. Tree fixtures react to its proximity.
- **Chandelier** -- The larger assembly at the top of the tree, sharing a wind chime cluster with a 0.8 m solar panel. Vishnu's design; structure built and in the shipping container. Its current layout has 18 lights in this repo's fleet class (see Chandelier light).

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
- **Beryl AX** -- GL.iNet GL-MT3000 travel router. The camp/art-site access point, fed by Starlink in bypass mode over Ethernet, USB-C powered at about 5 W. Its 2.4 GHz radio is pinned to channel 11 to match the mesh (ADR 0036). Ordered, not yet received.
- **Bypass mode** -- Starlink setting that disables the Starlink router's own WiFi and DHCP so a downstream router (the Beryl) is the only router on the link. Leaving bypass mode requires a Starlink factory reset, so the switch is rehearsed at home rather than improvised in the field.
- **Channel guard** -- Required firmware check on any Resonance device that associates to an AP while also using ESP-NOW: after STA association, read the actual operating channel and, if it does not match the compiled mesh channel, drop the WiFi association and keep the mesh. The mesh is the primary function; internet is the enhancement. Specified in ADR 0036; not yet implemented in any firmware.
- **Cricket console** -- Working name for the proposed Claude mesh bridge handheld (ADR 0037): a pocket device that is simultaneously a Claude chat client over WiFi/TLS, an ESP-NOW command transmitter, and a passive mesh observer. Direction only -- hardware is on hand but no firmware is written.
- **T-Deck Plus** -- LilyGO ESP32-S3 handheld, the variant this project owns (two on hand, LCD): 2.8 in ST7789 IPS 320x240 with GT911 capacitive touch, BlackBerry-style keyboard on an ESP32-C3 auxiliary MCU over I2C (commonly 0x55), trackball, 8 MB PSRAM / 16 MB flash, SX1262 LoRa, GPS, and a bundled 2000 mAh battery. The primary target for the cricket console. The Plus is what makes LoRa standard and adds GPS and the battery; the base **T-Deck** is a bring-your-own-cell devkit. LoRa is out of scope here -- the fleet link is ESP-NOW on channel 11.
- **T-Deck Pro** -- A *different* LilyGO device, not what this project owns: 3.1 in e-paper 320x240, CST328 touch, TCA8418 keypad controller, plus GPS and LoRa. Its display and input drivers do not transfer to the LCD T-Deck. Recorded only so nobody ports from the wrong spec page.
- **Cardputer ADV** -- M5Stack ESP32-S3 handheld with a 1.14 in ST7789 240x135 display and a 56-key matrix keyboard. One on hand; the secondary/pocket target for the cricket console, and the fallback if the T-Deck's IPS panel proves unreadable in direct sun.

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

## Current hardware stack (2026 production -- ADRs 0024-0032)

- **PowerFeather V2** -- ESP32-S3 controller board with solar charger, fuel gauge, and switchable rails; the production COTS board (Elecrow, 158 production boards bought/committed against a nominal 130-light deployment).
- **BQ25628E** -- TI solar charger / power-path IC on the PowerFeather. Buck-only: panel hot Vmp must be >= 4.6 V.
- **MAX17260** -- Fuel gauge IC. Known traits: +8 % current bias (/1.08 correction), no cold-POR off a deeply discharged cell, LFP-plateau-blind SOC.
- **TPS631013** -- The PowerFeather's 3.3 V buck-boost rail regulator.
- **32700** -- Cylindrical cell format (32 mm dia x 70 mm). fullbattery.com LiFePO4 6 Ah, qualified n=2 at ~5.75 Ah (ADR 0025); since 07-24 the cell for SMALL-enclosure classes (perimeter + candidate trunk-light enclosures -- the only cell that fits) + chandelier.
- **33140** -- Larger cylindrical cell format (33 mm dia x 140 mm). batteryhookup.com LiFePO4 15 Ah at an absurd ~$4.50/cell (130 bought 2026-07-24); the fleet-standard cell for LARGE-enclosure fixtures (downlights). Qualification pending -- capacity/IR run + ADR 0023 threshold re-map.
- **fullbattery.com** -- Production battery vendor. The Amazon "Palowextra 7.2 Ah" alternative measured 78 % of label with 2.3x IR and was rejected.
- **Voltaic P105 / P126** -- ETFE-laminated solar panels, 5 W / 2 W: P105 for downlights, P126 for perimeter fixtures (ADR 0026). ETFE = the tough fluoropolymer front layer.
- **SK6812 "HEX"** -- M5Stack 37-LED hexagonal addressable board; the close-range/ambient LED role, fed from the switchable 3V3 rail.
- **NeoHEX** -- M5Stack WS2812C-2020 hex board; least-efficient fallback (20 on hand).
- **4 W RGBW** -- Adafruit warm-white 4 W RGBW point-source emitter; the crisp-gobo long-throw role. Fed from the switchable 3V3 rail, same as the HEX -- decided by instrumented A/B 2026-07-11 (ADR 0029 amendment).
- **MSA311** -- Adafruit STEMMA 3-axis accelerometer; per-fixture sway/tilt sensing, no per-unit calibration (ADR 0027).
- **BMP581** -- Bosch temp + barometric pressure sensor (Adafruit STEMMA); 30 bought 2026-07-16 as generic environmental loggers for playa weather/2027 design. ADR 0034 assigns 24 to the outermost hanging-downlight ring and keeps 6 as spares.
- **TMF8820-mini** -- AMS 3x3 multizone ToF (SparkFun mini breakout); downward presence sensor on downlights (bench-validated on the same-family TMF8821). 940 nm VCSEL, **Class 1 laser** (IEC 60825-1 eye-safe cert + hardware VCSEL-fault shutoff) -- same class as phone face-unlock; BM's laser-registration policy targets display lasers, not Class 1 embedded sensors (one-line disclosure via lasers@burningman.org recommended, 2026-07-27).
- **VL53L5CX** -- ST multizone ToF (up to 8x8); outward presence sensor on perimeter fixtures; 60 protective optical covers bought (Gilisymo). 940 nm VCSEL, **Class 1 laser** per IEC 60825-1:2014 incl. single-fault conditions -- eye-safe at any viewing distance; keep flat windows only in front of the emitter (no lenses) to preserve the classification.
- **Solarnoid** -- The fleet noisemaker, design finalized ~2026-07-24 (ADR 0030): a solar-fed striker -- VDC-tap + 22,000 uF storage cap + MOSFET-driven push-pull solenoid + craft-store mallet -- physically knocks the bamboo. Daytime solar-surplus percussion; night belongs to the light show. Paired with LARGE-enclosure fixtures (downlights) only. The #3885 speaker-synth path was abandoned; a stronger-solenoid bake-off picked the part (0730B-class primary).
- **PUCA / PUCA DSP** -- Ohmic Limited's original-ESP32 + WM8978 audio/DSP
  board. Resonance owns an Original Edition in the 6 HP Eurorack expansion as the
  primary optional performance-audio publisher (ADR 0035). It ingests the RODE
  mic, a DJ line output, or its onboard microphones and will publish directed
  show data over ESP-NOW. Hardware is received; custom firmware is pending. The
  manufacturer's spelling uses an accented `u`; this repo uses ASCII `PUCA`.
- **RODE VideoMic NTG** -- Directional, battery-powered microphone bought for the
  PUCA/CoreS3 performance bridge. Its active variable 3.5 mm output simplifies
  gain staging for bowls, violin, singing, and ambient performance capture; the
  WS11 furry windshield is the outdoor configuration.
- **CoreS3 desk bridge** -- M5Stack ESP32-S3 screen-equipped fleet bridge in
  `firmware/cores3_bridge/`. It has already validated direct audio-reactive
  frames through Module Audio and remains the independent fallback/reference for
  the not-yet-written PUCA bridge.
- **STEMMA-QT / Qwiic** -- JST-SH 4-pin I2C connector standard used by the sensor boards.
- **Grove / HY2.0** -- M5Stack's physical connector family (carries GPIO data for the HEX, not I2C).
- **JST-XH** -- Keyed wire-to-board connector family planned for battery/LED harnesses (right-angle headers + pre-crimped cables in the to-buy queue).
- **TCA9548A** -- I2C mux used on the presence bench to host same-address sensors.
- **#6832** -- batteryspace.com product ID for the 20 Ah LFP cylindrical cell. Verified honest (19,412 mAh, 2026-07-12) but the bulk buy was CANCELLED 2026-07-15 on sourcing/timeline. The subsequent 24-uplight hinged-wing plan was itself superseded by the trunk-light allocation in ADR 0032. The ~$4.50/cell Alibaba equivalent is a 2027 idea.

## Firmware / bench terms

- **WonkyHouse** -- retired Tennessee bench WiFi SSID. Historical fleet records keep
  the profile that was actually flashed, but new builds reject it by default. Dad's
  personal-computer bench is the only opt-in legacy exception.
- **BubbyNet** -- a historical California/home bench AP (channel 11) and the
  original bench OTA profile. It is not a production-peer SSID.
- **Party In The Woods** -- the current production-peer maintenance SSID in
  Nevada City. The intended future field setup is one virtual SSID with this
  exact name across the BM camp and art-site Starlinks; that topology and channel
  behavior still require validation.
- **fixture (sketch)** -- `firmware/fixture/`, the production fleet firmware
  (one image, class probed at boot); `net_bench` remains the desk bridge build.

- **solar guard** -- `firmware/powerfeather_solar_guard.h`: forces wide VBUS_OVP and kicks a HIZ requalification when a bright-sun connect latches the charger input off. Baseline in every charging sketch.
- **field-cycle** -- net_bench's day/night lifecycle mode: charge -> wait-dark -> draw -> protect, with ADR 0023 low-battery thresholds.
- **maintenance mode** -- OTA path: an ESP-NOW metadata packet (`U` fleet / `U<id>` targeted) sends a fixture onto shared WiFi where it serves `/update` and `/telemetry`; `ops/bench/net_bench_ota.py` uploads in parallel. The self-hosted `--maint-ap` fallback is deprecated.
- **A/B rollback** -- Standard ESP32 dual-partition OTA: a new image must pass `verifyOta()` (C linkage!) or the bootloader reverts to the last-good image.
- **heartbeat** -- The ~1-2 Hz ESP-NOW broadcast state packet (id/seq/battery/PDR/RSSI + telemetry tails), kept <= 128 bytes for bridge compat.
- **Bench apps** -- `net_bench` (mesh/OTA/field-cycle; closest to production firmware), `power_bench` (charger/gauge matrix), `led_studio` (LED looks), `presence_bench` (5-sensor rig), `sway_demo` (accel+ToF fusion), `speaker_demo` / `clacker_demo` (noisemakers), `smoke_test` (acceptance), `wifi_diag` (RF probe), `ina_monitor` (external coulomb/lux ground truth).
