# TODO

Active punch list. Status: `[ ]` open, `[~]` in progress, `[x]` done. Owner in parens.

## Coordination with project team

- [ ] Confirm timing for Bamboo Pure air-ship of prototype lanterns to Steve in TN (Ben → Elliot / Dipta).
- [ ] Align with Elliot on rope-attachment decision (needed by end of June before Steve commits production hat geometry) (Ben).
- [ ] Confirm hat OD / height / bamboo-overlap to Vishnu so he can finalize renders (Ben).
- [ ] Pull `INV_2026_00401`. Decompose cost. Build target BOM and compare (Ben).
- [ ] Get Steve on the project's official "core build team" wiki (Ben → Elliot).
- [ ] Get shared access to Co-Work's wiki folder once Elliot has it cloud-hosted (Ben → Elliot).
- [ ] Drop a "lighting workstream digest" into the WhatsApp thread to inform Vishnu / Elliot / Dipta of decisions (Ben).

## Hardware track

- [ ] atopile module: `solar_input` — panel connector, Schottky, input filter.
- [ ] atopile module: `lifepo4_charger` — CN3058 + supporting passives, status LED.
- [ ] atopile module: `power_path` — load-sharing logic (panel-direct when sunny, battery fallback).
- [ ] atopile module: `voltage_regulator` — AP2112K-3.3 + decoupling.
- [ ] atopile module: `esp32_module` — ESP32-C3-MINI-1 + USB-C + reset/boot pins + decoupling.
- [ ] atopile module: `led_output` — JST-PH connector for WS2812B chain, decoupling, optional level shifter footprint.
- [ ] atopile module: `battery_monitor` — ADC voltage divider with gate MOSFET.
- [ ] atopile top — composition of modules into the carrier board.
- [ ] KiCad layout from atopile output.
- [ ] **Investigate JLCPCB / PCBWay firmware pre-flash service** at qty 100. If supported, production fixtures arrive flashed and the per-fixture ops time drops further (see ADR 0009).
- [ ] Design flashing jig fallback (USB-C breakout + pogo pins + auto-flash-on-insert script) for the case pre-flash isn't viable.
- [ ] First JLCPCB order — 5 boards SMT-assembled. Expect ~$30–60. Plan 2–3 spins.

## Enclosure track

- [ ] Design hat v1 in Fusion. Set-screw mechanical interface to bamboo neck (Steve).
- [ ] Print v1 on Bambu. Iterate fit on the bamboo prototype (Steve).
- [ ] Decide rope attachment point with team (Ben + Steve, blocked on team input).
- [ ] Material test for filter / gobo: matte paint test on PLA, comparison with translucent PLA, comparison with resin (Steve).
- [ ] Send hat v1 STL out for MJF print at JLC3DP / PCBWay for evaluation (Steve).

## Firmware track (parallel with hardware, validates architecture on existing TTGO modules)

- [ ] Bench validation on TTGO T-Beam: solar charging path. Connect small solar panel to LiPo input, validate end-to-end (Ben).
- [ ] Bench validation on TTGO T-Ice: WS2812B drive path. Confirm NeoPixelBus + I2S DMA works (Ben).
- [ ] Test ESP-NOW between two TTGO modules. Validate range, latency, packet loss (Ben).
- [ ] OTA prototype. Flash firmware over WiFi to a single TTGO. Validate full A/B partition flow (Ben).
- [ ] Port `TalismanPatterns.cpp` from `beneckart/future-robotics` into `firmware/core/pattern/` (Ben).
- [ ] Implement minimum-viable CA tick + render loop on bench (Ben).
- [ ] Smoke-test rig: host-side script that listens for fixture boot announcements and reports a checklist (per ADR 0009 — removes per-fixture visual inspection).

## Community Mandala Program (parked until Elliot signs off)

- [ ] Validate concept with Elliot + Vishnu (community-sourced apertures, art-gallery angle) (Ben).
- [ ] Pipeline: photo → vectorize (vtracer) → constraint check → cone projection (OpenSCAD) → STL.
- [ ] Brightness normalization in firmware (per-fixture flash calibration).
- [ ] Cataloging schema (designer, title, photo, easter eggs, inscribed ID).
- [ ] Contributor brief / submission form.
- [ ] Submission window opens; closes ~mid-July to leave time for printing 100.
