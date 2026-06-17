# Glossary

Terms and proper nouns used throughout this repo, in case an agent or human is dropping in cold.

## Project

- **Resonance Tree** — The overall art project. ~7.5 m bamboo installation for Burning Man 2026. Reused and expanded in 2027 (conch shell built around the trunk).
- **Resonance** — Short for the project. Also the parent art collective.
- **Resonance Lighting** — This repo's scope. The 100 downlight fixtures.
- **Downlight** — Each of the 100 hanging lantern fixtures inside the tree. Bamboo lantern + electronics hat + filter.
- **Hat** — The 3D-printed solar/electronics enclosure that mounts on top of each bamboo lantern. Our workstream.
- **Filter** / **gobo** — Patterned-aperture insert that sits at the bamboo node notch. Casts mandala shadows on the ground. Two physical forms: flat disc (early) and projective cone (Steve's recent prototype).
- **Wand** — A hand-carryable lantern variant (proposed) that participants can take through the piece. Tree fixtures react to its proximity.
- **Chandelier** — A separate larger fixture at the top of the tree, sharing a wind chime cluster with a 0.8 m solar panel. Vishnu's design. Tests bamboo-shaded solar performance for the 2027 expansion.

## People

- **Elliot Fabri** — Resonance project lead. Oakland → Bali. All final calls.
- **Ed Wilkes** — Structural engineer. Bristol. Owns the Rhino/Grasshopper master model.
- **Vishnu V** — Lighting designer. India. Owns the bamboo lantern shop drawing, wind chime, chandelier.
- **Ben Eckart** — Power systems, firmware, primary donor for Resonance Lighting. NVIDIA / US.
- **Steve Eckart** — Ben's dad. CAD wizard, retired. Owns enclosure design.
- **Luis Echeverría** — Bamboo U GM, Bamboo Pure liaison. Bali.
- **Dipta Priyatna** — Bamboo Pure production lead. Bali.
- **Galang, Iwel, Zaki** — Bamboo Pure team (production, model maker, content).
- **Josie New Numbee** — Renderings.
- **Michelle Satkin** — Mainfreight shipping rep.

## Organizations

- **Bamboo Pure** — Bali fabricator building the tree and the 100 bamboo lanterns.
- **Bamboo U** — Bali institution / school where Luis is GM.
- **18th Street Arts Center** — Fiscal sponsor (Jan Williamson + Stephen Sacks).
- **Burning Man org / BMHQ** — Burning Man corporate. Katie Hazard + Peter are the project's contacts.

## Agents and tools

- **Co-Work** — Elliot's PM agent. Maintains the Resonance project wiki from WhatsApp threads + Fireflies meeting transcripts. Refresh cycle every 3 hours. Currently self-hosted on Elliot's laptop; planned to move to cloud.
- **Resonance Agentic Wiki** — The WhatsApp group where Co-Work answers questions about the project. Also a name for the wiki Co-Work maintains.
- **Cowork** — The Anthropic product Ben is using for project management and review (this side of things). Distinct from Co-Work, the project's PM agent. Confusing namespace.
- **Claude Code** — The Anthropic product Ben (and probably Steve) will use for daily code/CAD iteration.

## Technical terms specific to this project

- **Brain v2.0** — The microcontroller used in the 2018 Talisman v2 build. TTGO T-Beam (ESP32 + LoRa + GPS + 18650 + LiPo charger).
- **Talisman** — Ben's 2017–2018 Burning Man wearable pendant project. ESP32 + LoRa mesh + LED display showing friend locations.
- **Marquee** — Ben's 2018 Burning Man piece using ESP32 (TTGO T-Ice) driving 240 WS2812B LEDs over WiFi via Open Pixel Control.
- **Marauder's Map** — Ben's 2018 Burning Man piece. 3 ft × 3 ft solar-powered laser-etched map of BRC backlit by 450 LEDs at road junctions.
- **future-robotics** — The github.com/beneckart/future-robotics repo containing prior project code.
- **Mystery white-enclosure board** — Initially unknown, identified as **TTGO T-Ice** (LilyGO, discontinued ESP32+WS2812B-driver board with white snap-on case).

## Physical / temporal

- **Playa** — The dry lakebed at Black Rock Desert where Burning Man happens.
- **BRC** — Black Rock City, the temporary city erected on the playa.
- **Grass Valley** — The project's pre-build staging area in Northern California, where bamboo from the Bali sea container meets the electronics and final integration happens before trucking to BRC.
- **The Man** — The central wooden effigy at Burning Man, used as the cardinal landmark for orientation. The Talisman's GPS navigation was relative to "the Man."
- **Burn week** / **Build week** — Late August through early September 2026 (and 2027). Build week starts roughly 1 week before the burn proper.
- **Default world** — Burning Man slang for the regular world outside the playa.

## Acronyms

- **ADR** — Architectural Decision Record. Files in `docs/decisions/`.
- **BLE** — Bluetooth Low Energy.
- **BOM** — Bill of Materials.
- **BOQ** — Bill of Quantities (used by Bamboo Pure for the tree fab).
- **CA** — Cellular Automaton / Automata.
- **DMA** — Direct Memory Access.
- **EDA** — Electronic Design Automation.
- **FCC** — US Federal Communications Commission. Use pre-certified Espressif modules
  such as ESP32-S3-WROOM-class parts; avoid custom RF.
- **GPIO** — General-Purpose Input/Output (pin).
- **IDE** — Integrated Development Environment.
- **I2S** — Inter-IC Sound bus. ESP32 uses it for parallel WS2812B output via DMA.
- **JLCPCB** — Chinese PCB fab + assembly service. Our assembler.
- **LDO** — Low-Dropout regulator.
- **LiPo** — Lithium Polymer (Li-ion variant, 4.2 V max charge).
- **LiFePO4** — Lithium Iron Phosphate (3.6 V max charge, our chemistry).
- **MCU** — Microcontroller Unit.
- **MJF** — Multi Jet Fusion (HP's powder-bed-fusion 3D printing process). Our planned production enclosure technology.
- **MPP** / **MPPT** — Maximum Power Point / MPP Tracking (solar panel optimization).
- **OPC** — Open Pixel Control (LED streaming protocol used in Marquee).
- **OTA** — Over-the-Air firmware update.
- **PCB** / **PCBA** — Printed Circuit Board / PCB Assem
