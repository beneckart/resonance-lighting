# BACKGROUND -- Resonance Tree, Lighting / Power Workstream

Project context for the Resonance Lighting workstream. Read alongside `README.md`, `AGENTS.md`, `docs/ROADMAP.md`, and `docs/decisions/`.

## What this project is

The **Resonance Tree** is a two-year bamboo art installation for Burning Man 2026 + 2027, fabricated by Bamboo Pure in Bali. The 2026 build is a ~7.5 m bamboo tree with two laminated rings (1.5 m and 1.7 m), 7.5 m doubled bamboo poles, 30 limbs, held together with steel wire and capped with a wind chime cluster. In 2027 the trunk becomes the inside of a multi-level conch shell built around it; the bamboo trunk and the 100 lighting fixtures are reused both years.

Hanging from the tree: **100 bamboo "downlights"** plus a chandelier-and-wind-chime assembly at the top.

This repo's scope is the 100 downlight fixtures: solar-powered, mesh-networked, autonomous, fungible.

**Two-year framing matters for the electronics spec.** The 100 downlights are durable infrastructure, not a 2026-only throwaway build. The chandelier at the top of the tree is a separate solar-shading testbed for the 2027 expansion's larger lighting loads -- it does not gate the downlight feature set.

## 2026 architecture correction -- risk-reduction policy

The electronics architecture now explicitly separates project goals from implementation accidents:

- **OTA:** standard ESP32 WiFi OTA only. Firmware images are not distributed through an ESP-NOW gossip protocol. ESP-NOW can advertise small maintenance/version metadata, but USB/pogo flashing remains the guaranteed recovery path.
- **MCU/RF:** use a pre-certified Espressif module or COTS board with integrated RF/antenna. Do not design custom RF. The exact module is selected for RF robustness, sourceability, and firmware headroom, not minimum size or minimum cost.
- **Hardware production:** run a COTS deployable prototype / fallback track in parallel with custom PCBA. A stack of factory-assembled COTS boards, pre-crimped cables, screws, standoffs, and keyed connectors is acceptable if it avoids skilled per-unit work.
- **Battery/charger:** LiFePO4 remains the preferred production chemistry, but the charger path should start from a proven LiFePO4-capable solar reference such as bq25185-class designs. CN3058 is fallback, not default.
- **LED power:** the LED rail must be switchable and default-off so a hung MCU cannot leave addressable LEDs on until the battery is depleted.

These corrections supersede earlier spec language that over-committed to ESP32-C3-MINI-1, CN3058-first custom charger design, no dev-board architectures, direct-always-on LED power, and mesh-gossiped OTA.

## Team

The wider Resonance project team:

- **Elliot Fabri** -- project lead, all final calls. Oakland-based, traveling to Bali for build season.
- **Ed Wilkes** -- structural engineer, owns the Rhino/Grasshopper master model. Bristol.
- **Vishnu V** -- lighting designer, owns parametric downlight model and wind chime cluster. India.
- **Ben Eckart** -- power systems, firmware, primary donor for Resonance Lighting. NVIDIA / US.
- **Steve Eckart** -- Ben's dad. Retired, longtime CAD professional, owns enclosure design and 3D printing for this workstream.
- **Luis Echeverria** -- Bamboo U GM, Bamboo Pure liaison. Bali.
- **Dipta Priyatna** -- Bamboo Pure production lead. Team includes Galang (production), Iwel (model maker), Zaki (content). Bali.
- **Josie New Numbee** -- renderings; experience with bamboo architecture; may join build at the Grass Valley pre-build staging area.
- **Michelle Satkin** -- international shipping / Mainfreight rep.
- Marketing: Gizem (social), Ibeya (design). Specialty: Uriah (woven net cradles for platforms). Institutional: Jan Williamson + Stephen Sacks at 18th Street Arts Center (fiscal sponsor); Katie Hazard + Peter at Burning Man HQ.

This repo is owned by Ben and Steve. Other team members coordinate through Elliot via WhatsApp.

## Lighting workstream -- fixture architecture

### Bamboo lantern body (Vishnu's design -- final, in fab at Bamboo Pure)

Per `enclosure/references/DOWN LIGHTS DRAWINGS.pdf`:

- **Form:** Bamboo pole with node, cut 15 cm above + 25 cm below the node, then split into 15 vertical strips along the lower 25 cm. Strips are steam-bent outward into a flared "skirt" (12-14 cm at base). Top remains a 7 cm cylinder.
- **Total height:** 40 cm.
- **Bamboo split ring** (3 mm thick) wraps the join at the node, 15 cm down from the top.
- **Tooling:** jig saw cuts, expansion via heated metal ring, 1-day setting.
- **Interior diameter (top cylinder):** 5.5 cm minimum -- confirmed by Bamboo Pure.
- **Quantity:** 100 lanterns in BOQ R6.

### Solar "hat" / electronics enclosure (this repo's deliverable)

**Architecture:** Standard solar-garden-lantern modular pattern -- the bamboo lantern is the passive decorative shade with cutouts on the bottom half; the hat is a self-contained sealed enclosure (PV + battery + charge controller + MCU + LEDs + antenna) that sits *partially nested into* the top of the bamboo and *partially mushrooming over* it. The hat is larger than the 76 mm bamboo OD because the PV needs more area than the bamboo top affords. The 5.5 cm bamboo interior diameter is therefore *not* a constraint on the electronics envelope -- it's just the diameter of the neck the hat plugs into. The hat itself can be whatever size makes structural and visual sense within the 1 kg/fixture weight budget set by the structural engineer.

**Mechanical connection:** Set screws (or equivalent adjustable clamping) absorb bamboo dimensional variability -- bamboo is a natural material and every lantern body is slightly different. The hat clamps to the bamboo neck.

**Reference:** Common metal solar garden lanterns with separable solar tops (Moroccan-style cutout examples on Amazon, e.g. ASIN B0DKNLGCDM). Same architecture, bamboo skin instead of metal.

**Modularity goal:** Fixtures are fully fungible. If one breaks, swap it in 30 seconds. The mechanical and firmware design must not require pairing or per-unit re-flashing on swap -- any unit drops in. (See ADR 0009 -- minimize per-fixture operations at scale.)

**Open mechanical decision: where does the hanging rope attach?**

- *On the hat* (preferred for fungibility): the hat is the persistent structural fixture; the bamboo lantern body is the consumable. Swap = pull set screws, slide off old bamboo, slide on new, retighten. Electronics never disconnect from rope or mesh. Cost: hat top must be designed as a load-bearing point for repeated wind/load cycles.
- *On the bamboo*: aesthetically more "of a piece," and Bali artisans are practiced at bamboo rope ties. But every bamboo swap requires disturbing the rope, and a hidden bamboo failure drops the electronics.
- *Hybrid (recommended):* primary load on the hat; soft secondary tie around the bamboo neck as a safety so a backed-out set screw doesn't drop the lantern body 20 ft.
- This is also an aesthetic call -- pending team input from Vishnu and Ed.

### Patterned aperture / "filter"

Steve has 3D-printed the bamboo lantern body to Vishnu's spec and built a series of patterned-aperture inserts ("filters" / "lenses" in the family vocabulary; *gobos* in stage-lighting vocabulary). Latest version extrudes the 2D pattern into a translucent cone via projective geometry. Friction-fits at the bamboo node notch (15 cm from the top), where Vishnu's 3 mm split ring lives. Dual visual purpose:

- *Looking up at the lit fixture:* the filter reads as a glowing cone-shaped bulb with a distorted version of the pattern wrapped around its surface.
- *Looking down at the cast light:* clean undistorted mandala shadows on the ground.

**Empirical findings from Steve's pulley rig** (1 ft to 8 ft tested in workshop; >16 ft tested off the back deck):

- LED-to-filter distance behaves like a focal length: closer = larger, more divergent mandala on the ground; farther = smaller, sharper.
- Multi-LED clusters (e.g. headlamp) wash out the pattern -- multiple offset shadow-casters overlap into a halo. Single point sources give crisp mandalas.
- High-power sources reflect inside the PLA walls and degrade the cast pattern. Hypothesis to test: matte interior paint on the printed filter (bamboo's matte interior absorbs; PLA's gloss reflects). If matte paint closes the gap, problem solved cheap.
- The WS2812B's R/G/B dies are physically offset (~1-2 mm), producing slight chromatic fringing on the cast mandala. Reads as a feature, not a bug -- same physics as chromatic aberration in cheap lenses. Amplifies when the LED is close to the filter; suppresses when far.
- Actuating LED-to-filter distance was considered for "mandala zoom" but rejected -- moving parts are a maintenance liability. Static distance, set by hat geometry, is the right call.

**Geometric note:** Because the filter sits at the bamboo node notch and the LED sits at a known depth in the hat, LED-to-filter distance is geometrically determined and consistent across all 100 fixtures. No per-unit calibration needed.

**Aesthetic alignment:** The team has explicitly accepted plastic in the design (the hat enclosure itself sticks out the top of the bamboo). Position is "if 3D printing affords something we can't otherwise do artistically, it is worth doing." The filter program fits this.

### Chandelier / wind chime cluster (top of tree, Vishnu's workstream -- not in this repo)

A 6-inch bamboo *asper* in the center of the chime cluster becomes a chandelier providing center-tower lighting. The ring holding the chandelier hosts a 0.8 m diameter solar panel (partially shaded by the bamboo skin). Elliot frames this as a testbed for whether bamboo-shaded solar can sustain the larger lighting loads planned for the 2027 conch-shell expansion. The 100 downlight spec is *not* gated on this; downlights are durable infrastructure for both years.

## Community Mandala Program (proposed)

Concept: instead of one designer producing 100 patterns, source them from a wider artist community. The piece becomes a 100-pattern "art gallery": visitors find favorite patterns, hunt for easter eggs (designer initials hidden in the geometry, references to past Burning Man art cars, in-jokes for the team and contributors).

**Pipeline (proposed):**

1. **Collect** sketches -- black ink on white paper, contained within a circle, photographed with a phone. Specify size and contrast guidelines in the contributor brief.
2. **Image processing** -- deskew, threshold, denoise, crop to circle. Standard CV.
3. **Symmetry pass (optional, toggleable per submission)** -- detect rotational order and average rotated copies. Cleans wobble while preserving the artist's hand.
4. **Vectorize** with [vtracer](https://github.com/visioncortex/vtracer) (good fine-detail, open source) or potrace (the classic for B&W). AI is bad at outputting high-fidelity SVG; use deterministic tools for the geometry step, AI only for interpretation ("is this radially symmetric?", "what's the rotational order?").
5. **Constraint check** -- minimum feature width for printability (0.4 mm FDM, 0.2 mm resin); no floating islands; flag unprintable submissions back to the artist.
6. **Cone projection extrude** -- OpenSCAD script taking SVG path as input. Single `filter_cone.scad` + shell loop = batch geometry generation.
7. **Slice and print** on Bambu (or send batch to JLC3DP / PrintAVoid / Slant3D for resin if FDM detail is insufficient).

**Brightness normalization across patterns.** Different mandalas have wildly different open-area fractions (sparse vs dense). Without compensation, the tree will read as "broken lanterns" rather than "varied lanterns." Solve in firmware: per-fixture brightness calibration stored in flash, computed from open-area fraction at print time and burned in during the USB flashing step.

**Cataloging.** Each pattern has metadata: designer name, pattern title, source-sketch photo, lit-on-rig photo, easter egg notes, unique inscribed identifier (small text on rim, hidden in pattern, etc.). Becomes the basis for a printed program / map at the playa.

**Curation.** Expect ~50% submission yield. Collect ~150-200 sketches to pick 100 strong, printable, in-spirit ones.

**Timeline.** Filters in hand by ~Aug 10 to ship to Grass Valley. Print 100 x ~30 min on Bambu = ~50 hours / 4 days continuous. Vectorization + constraint repair + reprints: ~1-2 weeks. Submission window must close by mid-July at the latest.

## Wireless / mesh creative possibilities

Going fully wireless -- no data lines, no fixed wiring topology -- opens creative territory beyond "dimmer switch over the air." Concepts under consideration for the firmware to support natively, even if not used on day one:

- **Cellular automata light fields.** Each lantern reads its neighbors' state and updates per a rule. Reaction-diffusion rules (Belousov-Zhabotinsky-style) produce organic, flock-like, forest-fire-like wave dynamics -- much more compelling than randomness or pre-programmed sequences. Other CA candidates: Greenberg-Hastings (excitable medium), Game of Life variants, continuous Lenia (smooth blob dynamics).
- **Spatial / topological neighbor awareness.** ESP-NOW exposes RSSI per packet. With a one-time install-time calibration (each lantern learns its top-K nearest neighbors by signal strength, or is told its physical position), neighbor lists can be pre-baked. This solves the CA neighbor-identification problem cleanly without GPS or fancy hardware. Stored in flash, doesn't change at runtime.
- **Hand-carried "wand" lantern.** A battle-hardened lantern variant (or a custom unit) that participants pick up and carry through the piece. The wand broadcasts presence over ESP-NOW; tree fixtures measure RSSI to determine proximity; nearest fixtures react (brighten, color-shift, kick off a CA wave). Mesh hops propagate the disturbance outward -- the network topology becomes visible as a wavefront rolling away from the wand.
- **Synced "choir" moments.** Loose CA most of the time, with occasional global-sync events where all 100 lanterns coordinate (NTP-style time sync over mesh) for a unified moment, then break back into local rule.
- **Wind-chime coupling.** If the chime cluster ends up with a microphone or accelerometer, real wind events at the top of the tree could feed into the CA below -- wind literally drives the light waves through the tree.
- **Time/state programs.** Different rule sets per night (e.g. one CA Monday, another Tuesday), pushed OTA the day before.
- **Solar-aware grace.** Lanterns with low battery contribute less to the show, fade to lower brightness, drop out gracefully -- turning a power-budget reality into a visible "the tree is breathing" effect rather than a failure.

**Firmware architecture implication.** To leave all of this on the table without painting into a corner, the firmware: (a) stores position / neighbor-list / per-fixture brightness calibration in flash; (b) treats the rendering loop as a function of (local state + neighbor states + time + global mode), rather than hardcoding any one mode; (c) keeps the global "mode" pushable through a normal control/config path, while firmware updates use standard ESP32 OTA maintenance mode rather than a custom mesh firmware transport. Build for the framework first, then the modes.



## Current COTS / PowerFeather R&D state (2026-05-10)

The hardware strategy has shifted from custom-PCBA-first to a **dual COTS/custom track**. This is not a retreat from a custom board; it is a risk-control strategy. The project can now test real boards, real solar input, real batteries, real LED modules, and real hat geometry before deciding whether 2026 production needs a bespoke PCBA.

### Leading COTS candidate: PowerFeather V2

ESP32-S3 PowerFeather V2 is currently the strongest COTS candidate and the best reference architecture for a future bespoke board. Its documented V2 architecture is unusually aligned with Resonance Lighting:

- ESP32-S3-WROOM-1 module with onboard PCB antenna.
- BQ25628E charger / power-path IC.
- LiFePO4 support in V2.
- MAX17260 fuel gauge with LiFePO4 profile support.
- TPS631013 buck-boost 3.3 V rail.
- Switchable `VSQT` / STEMMA-QT rail.
- Solar/DC input via `VDC`.
- USB-C and Feather-compatible form factor.
- Rich telemetry: voltage, current, battery temperature, SOC, charger state, faults, and estimated time-to-empty/full.

Telemetry is not just a diagnostic convenience. It is valuable field data for BM 2027: actual playa sun exposure, solar-panel shading, dust effects, battery drain, thermal behavior, and realistic nightly load.

Caveat: V2 docs are still preliminary. Ben ordered PowerFeather boards from Elecrow and contacted the PowerFeather creator about V2 availability and KiCad files. On arrival, boards must be identified as V1 or V2 before attaching LiFePO4.

### PowerFeather V1 vs V2 schematic finding

The V1 and V2 schematics show that both revisions use BQ25628E. The important V2 changes are the regulator and fuel-gauge subsystems:

- V1: BQ25628E + LC709204F fuel gauge + XC6220 LDO. Good LiPo board; not a board-level LiFePO4 solution.
- V2: BQ25628E + MAX17260 fuel gauge + 20 mohm current sense + TPS631013 buck-boost. Suitable for LiFePO4 testing if hardware and firmware behave as documented.

This is now the main reference architecture for the custom board, superseding the older CN3058/AP2112K/ESP32-C3-MINI direction.

### LED module candidates

Current LED candidates are split by interface. **(Stale -- see ADR 0018 / README "Current
architecture direction" for the live state: the IS31 is ruled out and the choice is now
between SK6812 "HEX" direct-GPIO and a 4 W RGBW point source, both driven direct-GPIO.)**

- **Adafruit IS31FL3741 13x9 RGB matrix** -- ~~primary no-solder PowerFeather companion~~ **RULED OUT (ADR 0018):** on the V2's shared charger/gauge I2C bus it browns out the board on battery under WiFi. It uses STEMMA-QT/Qwiic I2C; multiplexed PWM, not NeoPixel.
- **M5Stack NeoHEX** -- promising center-plus-rings optical geometry with 37 WS2812C LEDs. It uses M5Stack HY2.0/Grove physically but is not an I2C/STEMMA-QT device; it needs GPIO data and a suitable LED power rail.
- **FeatherS2 Neo** -- integrated ESP32-S2 + 5x5 RGB matrix + LiPo charging. Fastest optical prototype and LiPo fallback.
- **M5Stack Atom Matrix** -- tiny ESP32 + 5x5 WS2812C + USB-C module. Strong ultra-simple fallback when powered by DFRobot DFR0559.

### Current prototype tracks

1. **PowerFeather V2 + LiFePO4 + solar panel + ~~IS31FL3741 13x9~~ direct-GPIO LED (SK6812 HEX or 4 W RGBW).** The confirmed reference (ADR 0021); IS31 dropped (ADR 0018), LED module still being decided.
2. **PowerFeather V2 + LiFePO4 + solar panel + M5Stack NeoHEX.** Alternate LED geometry test; not STEMMA-QT plug-and-play.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback. DFR0559 owns battery/solar; FeatherS2 Neo battery JST stays empty.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback.

### Battery sourcing update

LiFePO4 remains preferred, but cell format matters. 14430 LiFePO4 cells are common and cheap, usually around 400-450 mAh. Production should still prefer one larger cell per fixture -- ideally 18650 LiFePO4 around 1500-2000 mAh -- rather than parallel packs of many small 14430 cells. Multi-cell packs add contacts, matching, wiring, assembly, and QA risk.

### Solar panel sourcing update

Square/rectangular 1-5 W panels are now on order for R&D. Round/circular panels remain aesthetically appealing for production but are harder to source quickly. R&D should not wait for round panels. The hat should be designed so the panel mounting surface can adapt between rectangular R&D panels and possible circular production panels.

### Near-term bench questions

The next phase is measurement-driven:

- Is the Elecrow board actually PowerFeather V2?
- Does PowerFeather V2 charge and gauge LiFePO4 correctly?
- What are real sleep currents with LED modules connected and rails off?
- Which LED module gives the best gobo projection?
- What solar harvest do the 1-5 W panels produce in sun, shade, and heat?
- Does the PCB antenna still work inside the hat with panel, battery, wiring, and screws installed?
- Can the firmware prevent stuck-on LEDs from draining the battery into an unrecoverable brownout loop?

## Lessons from the 2018 Talisman v2 build (Ben + Steve's prior collaboration)

Ben and Steve have built ESP32-plus-WS2812B-plus-LoRa-mesh wearable pendants before -- the *Talisman v2* project for Burning Man 2018. That project's Drive folder contains real measured data, real PCB-attempt experience, and real library choices that carry over directly to the Resonance downlights. Anything below marked with * is reusable.

**Hardware platform used (*).** The "Brain v2.0" was a TTGO T-Beam -- ESP32 + SX1276 LoRa + NEO-6M GPS + 18650 holder + LiPo charge IC + USB. Steve has multiple in his workshop. **Excellent for prototyping the Resonance lighting** without any custom hardware: it already has battery + charge IC + USB integrated, so plug a small solar panel into the LiPo input pads and the entire solar charging path is validated end-to-end on existing hardware.

**WS2812B-direct-from-battery confirmed working (*).** The 2018 wiring notes worked through the math (WS2812B threshold = 0.7 x Vcc; with 3.3 V GPIO and battery up to 4.2 V, threshold = 2.94 V, margin 360 mV) and verified on bench. For LiFePO4 the math is even friendlier (threshold 2.52 V, margin 780 mV). No level shifter is expected to be required for the data line, but the production design still needs a switchable/default-off LED power rail so a hung MCU cannot leave pixels on indefinitely.

**3.3V regulator limit (*, important).** The 2018 build explicitly rejected powering the LEDs from the ESP32's 3.3 V regulator output -- the regulator caps at ~600 mA and the radio bursts can take 250 mA, leaving little margin for LEDs. For Resonance: same constraint applies. Power LEDs from the battery rail; only the MCU runs off the 3.3 V regulator.

**LED library choice (*).** The 2018 build used [NeoPixelBus](https://github.com/Makuna/NeoPixelBus) with ESP32's I2S+DMA driver, *not* Adafruit's NeoPixel library or FastLED bit-banging. I2S+DMA means zero CPU load for LED updates, freeing the CPU for mesh handling. Same call applies to Resonance.

**Real measured power numbers (*, gold).** From bench testing of a 16-LED Adafruit ring:

| Configuration | Current draw |
|---------------|--------------|
| 16 LEDs full bright, white | 500 mA |
| 16 LEDs full bright, monochromatic (R/G/B alone) | 200 mA |
| 16 LEDs half bright, white | 280 mA |
| 16 LEDs `setBrightness(26)` (~10%), white | 70 mA |
| 16 LEDs `setBrightness(1)`, white | 20 mA |
| 1 LED at `setBrightness(255)`, red | 27.7 mA |

Per-LED current scales linearly. For Resonance with 1-9 WS2812B per fixture and typically 1-3 lit at a time, see `docs/block-diagram/SYSTEM.md` for the budget that drops out.

**Custom PCB attempt -- Steve has tried this before (*).** Steve designed a flat 73-LED "LED-Disk" PCB in 2018. He noted afterward: *"I think I broke board design rules. Some components are too close together, and I didn't put caps between LEDs or put trace connection holes for power and signal."* The specific gotchas he hit (component spacing, decoupling caps between LEDs, vias for power/signal) are exactly the gotchas modern AI review will catch on the next attempt. He's not starting from zero -- he's starting from "I tried this once and I know what I broke."

**Decoupling rule of thumb.** Roughly one decoupling cap per 2 LEDs on the 2018 design (60 caps for 120 LEDs). For Resonance with 1-9 LEDs, plan for 2-4 decoupling caps (100 nF each) plus a bulk cap (10 uF) on the LED rail.

**What the 2018 team wanted to do but didn't get to.** The 2018 action list called for "design new data packet to transmit entire map for each time slice for multi-hop network functionality." They had single-hop LoRa but wanted multi-hop mesh and ran out of time. Resonance with ESP-NOW is the chance to do that properly.

**Files in the Drive folder, by usefulness:**

- `Talisman v2.0` -- main project doc, contains the power numbers above.
- `Talisman v2 Low-Level Notes (Wiring and Library)` -- the WS2812B-from-battery analysis and library choices.
- `Talisman Project Action List` -- historical scope; useful for understanding what shipped vs what didn't.
- `MMM BOM` -- Mini Marauder's Map BOM; Pi-based base station, not directly relevant.
- `LoRa Parameters` and `LoRa_spd_test.xlsx` -- spreading factor / bandwidth tuning. Not directly applicable since Resonance uses ESP-NOW, but worth knowing if LoRa ever comes back into scope (e.g. for long-range wand -> tree).
- `Talisman2_led_board_quote_seeed.pdf` -- historical Seeed quote, reference only.
- `LED-Disk-73.JPG` and `Talisman-Stuffed.jpg` -- photos of Steve's PCB attempt and the final hardware.

## Reusable code from `beneckart/future-robotics` repo

Ben's GitHub repo contains the firmware from Talisman v2 (and v2rev2), Marquee, MaraudersMap, and Winduino -- earlier Burning Man projects. Several pieces are directly reusable for Resonance.

**Talisman v2rev2 was a TTGO LoRa32 (not the T-Beam).** Pin mapping in `HardwareDefines.h`: SCK=5, MISO=19, MOSI=27, SS=18, RST=14, DI0=26, OLED I2C at 21/22, button at 39, battery sense at 35. That's the Lilygo TTGO LoRa32 v1 with the SX1276/78 module and built-in OLED. The Brain v2.0 from the 2018 design doc was the T-Beam -- so Steve's workshop probably has *both*: T-Beams (used as the brain/talisman pendant) and LoRa32s (used at some point as a tracker-only OLED variant).

**Marquee was a generic ESP32 driving 240 WS2812B LEDs over WiFi via OPC.** From `Marquee/sketches/ESP32_Marquee/ESP32_Marquee.ino`: WiFi-only, no LoRa, no GPS. Drives 2 x 120 LEDs on pins 2 and 14 using two NeoPixelBus I2S channels in parallel. Implements the [Open Pixel Control](http://openpixelcontrol.org) protocol on port 7890. **The board this firmware runs on is the TTGO T-Ice** (LilyGO, discontinued), a purpose-built ESP32 + WS2812B driver. Steve has multiple of these in workshop too -- they're the "white-enclosure" modules.

**Reusable pieces for Resonance:**

- **`TalismanPatterns.cpp`** -- clean templated extension of `NeoPixelBrightnessBus` with built-in animation patterns (RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE) driven by interval-based update logic. Drops directly into `firmware/core/pattern/`.
- **The 11-byte packet format from `sendPkt()`** in `talisman_v2rev2.ino` -- good reference for compact mesh messages: 0xDEAD start sentinel, device ID, payload bytes, 0xBEEF end sentinel. ESP-NOW gives framing for free, so the sentinels can be dropped, but the discipline of "tiny structured payload, fixed length" carries over.
- **OPC server on port 7890** -- a battle-tested pattern for "host streams frames to a single high-bandwidth lantern over WiFi." Useful as a host-side animation development harness for streaming test patterns to a single bench fixture (development only, not production firmware).
- **Python animation library** (`Marquee/python/`) -- Conway's Life, gif renderer, lava lamp, raver_plaid, sailor_moon, spatial_stripes, speed_test. All on the host side via OPC; useful for prototyping animations before porting to embedded.
- **C++ effect engine** (`Marquee/cpp/`) -- particle_trail, rings, looper, mixer, simple, spokes. Particle-based animation with a JSON layout file. Uses `nanoflann` (KDTree spatial neighbor lookup) and `svl` (vector math). Closest existing code to "spatially-aware lantern reactions," just running on a host instead of distributed across a mesh.
- **NeoPixelBrightnessBus + I2S DMA pattern** -- confirmed working on ESP32 with two parallel channels. Same call for Resonance.
- **Datalog infrastructure** (`Talisman/datalog_processing/`) -- SPIFFS-based on-device logging with a Python parser. Could be repurposed to log per-fixture battery/charge data over the Burning Man week to inform the 2027 design.

**Talisman v2rev2 production settings (Resonance baseline).** LED brightness: `LED_BRIGHTNESS = 4` (out of 255). **Very dim -- under 2%.** Consistent with art-piece ambient lighting. This is the realistic operational brightness target, not the demo-on-bench brightness.

**Earlier projects worth knowing about, lower-relevance for Resonance:**

- **MaraudersMap** -- 3 ft x 3 ft laser-etched solar-powered map of BRC backlit by 450 LEDs at road junctions, fadecandy-driven, dithered.
- **Winduino** -- 500 individually-addressable LEDs, wind-controlled. Real-world sensor (wind) modulating the lighting connects to the wind-chime-coupling idea above.
- **HugBot** -- pressure-sensing robot with EL wire. Unrelated.
- **HeartBot, MindMachine, dgs, VQVAE** -- unrelated experiments.
