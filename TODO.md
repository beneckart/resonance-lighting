# TODO

Active punch list. Status: `[ ]` open, `[~]` in progress, `[x]` done. Owner in parens.

## Immediate documentation / repo hygiene

- [ ] Add `LOG_APPEND_2026-05-10.md` entry to `LOG.md` (Ben).
- [ ] Add ADR 0015 — PowerFeather V2 as leading COTS/reference architecture (Ben).
- [ ] Add ADR 0016 — COTS prototype shortlist after purchases (Ben).
- [ ] Add ADR 0017 — Battery cell format and sourcing (Ben).
- [ ] Add ADR 0018 — LED module/interface plan (Ben).
- [ ] Add `docs/research/COTS_SURVEY_2026-05-10.md` (Ben).
- [ ] Add `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md` (Ben).
- [ ] Add `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md` (Ben).

## COTS purchasing / arrival

- [x] Buy R&D candidate set: PowerFeather, FeatherS2 Neo, Atom Matrix, NeoHEX, Adafruit IS31FL3741 matrix, DFR0559, panels, battery samples (Ben).
- [x] Contact PowerFeather creator re: V2 availability and KiCad files (Ben).
- [ ] Follow up on PowerFeather forum thread if no reply within a few days (Ben).
- [ ] Confirm whether Elecrow boards are V2 or V1 on arrival (Ben).
- [ ] Confirm whether PowerFeather V2 KiCad/Gerbers can be shared or licensed (Ben).
- [ ] Call/email BatterySpace if order confirmation is missing; verify 18650 LiFePO4 availability (Ben).
- [ ] Buy alternate LiFePO4 18650/26650 sources if BatterySpace order fails (Ben).

## COTS bench testing

- [x] Build interim Track A0: Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 matrix via STEMMA-QT until PowerFeather boards arrive (Ben).
- [x] Flash USB smoke-test firmware to Adafruit Feather ESP32-C6, FeatherS2 Neo, and Atom Matrix; record MAC, reset reason, board type, firmware version, and LED/I2C status (Ben).
- [x] Install/check smoke-test Arduino libraries: Adafruit IS31FL3741, Adafruit GFX, Adafruit BusIO, and a WS2812-capable LED library for integrated 5x5 boards (Ben).
- [x] Decide first OTA maintenance-mode mechanism for COTS smoke firmware: local WiFi AP credentials vs board-hosted temporary AP/web updater (Ben).
- [x] Test home-WiFi web OTA upload end-to-end on all three COTS smoke boards (Ben).
- [ ] Test temporary AP / portable-router OTA upload path before field-style testing (Ben).
- [ ] Build Track A: PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 matrix (Ben).
- [ ] Build Track B: PowerFeather V2 + LiFePO4 + solar panel + M5Stack NeoHEX with GPIO/suitable rail (Ben).
- [ ] Build Track C: FeatherS2 Neo + DFRobot DFR0559; Feather battery JST left empty (Ben).
- [ ] Build Track D: Atom Matrix + DFRobot DFR0559 (Ben).
- [ ] Run incoming inspection and board-ID procedure from COTS test plan (Ben).
- [ ] Measure sleep current for each stack (Ben).
- [ ] Measure active/radio/ESP-NOW current for each stack (Ben).
- [~] Measure LED current for center-only, 3-pixel, 9-pixel/crop, and full-array capped modes (Ben).
- [ ] Add SEN0291 I2C wattmeters to power-test harness and bench worksheet when they arrive (Ben).
- [ ] Measure solar charge behavior for each 1–5 W panel in sun/shade/heat (Ben).
- [ ] Test low-battery + solar recovery for PowerFeather V2 and fallback stacks (Ben).
- [ ] Test standard OTA maintenance mode on at least two COTS boards (Ben).
- [ ] Test ESP-NOW heartbeat/state packets with jitter/sequence numbers (Ben).
- [ ] Test LED fail-safe: stuck LEDs, MCU hang, watchdog reset, rail-off recovery (Ben).
- [ ] Test gobo projection with IS31FL3741, NeoHEX, FeatherS2 Neo, Atom Matrix (Ben + Steve).
- [ ] RF test each candidate inside a mock hat with panel/battery/wiring installed (Ben + Steve).
- [ ] Time-trial COTS stack assembly into mock hat (Ben + Steve).

## PowerFeather-specific tests

- [ ] I2C-scan PowerFeather boards and identify BQ25628E / MAX17260 / TPS631013 vs V1 parts (Ben).
- [ ] Configure LiFePO4 profile on confirmed V2 hardware only (Ben).
- [ ] Verify BQ25628E charger telemetry: state, faults, input regulation, charge current (Ben).
- [ ] Verify MAX17260 telemetry: voltage, current, temperature, SOC, time-to-empty/full (Ben).
- [ ] Test `VSQT` off-state leakage with IS31FL3741 attached (Ben).
- [ ] Test `VSQT` sleep/wake/reinitialize cycle (Ben).
- [ ] Test panel MPP/VINDPM settings for each panel (Ben).
- [ ] Test thermistor / battery-temperature path if accessible (Ben).

## Battery / solar sourcing

- [ ] Compare 18650 LiFePO4 sample capacity against rated capacity (Ben).
- [ ] Evaluate 26650 LiFePO4 only if 18650 sourcing or autonomy becomes a problem (Ben).
- [ ] Avoid multi-14430 production pack unless mechanical constraints force it (Ben + Steve).
- [ ] Record panel dimensions, weight, output, connector type, and shipping lead time (Ben).
- [ ] Search for round/circular panels for production aesthetics, but do not block R&D on them (Ben).
- [ ] Design hat top so R&D rectangular panels and production round panels can both be accommodated if needed (Steve).

## Custom hardware track

- [ ] Decide whether custom board is needed after COTS tests (Ben + Steve).
- [ ] If custom board proceeds, use PowerFeather V2 as reference architecture (Ben).
- [ ] Select charger/fuel-gauge/regulator architecture: BQ25628E + MAX17260 + buck-boost is current leading reference (Ben).
- [ ] Keep LED module/daughterboard separate until optics are frozen (Ben + Steve).
- [ ] Add keyed solar connector/pigtail plan; do not rely on direct panel wires to board pads for production (Ben + Steve).
- [ ] Add input protection review for outdoor solar cable (Ben).
- [ ] Add hardware reviewer before any custom board order (Ben).
- [ ] Use PCB-antenna WROOM module by default; do not use u.FL unless RF tests fail (Ben).

## Enclosure track

- [ ] Design hat v1 around COTS stack envelope as well as possible custom board envelope (Steve).
- [ ] Add mounting/standoff options for PowerFeather, FeatherS2 Neo, Atom Matrix, DFR0559, and LED modules (Steve).
- [ ] Add strain-relief plan for panel pigtail / VDC connector (Steve + Ben).
- [ ] Keep antenna region away from solar panel, battery, screws, and metal (Steve + Ben).
- [ ] Decide rope attachment point with team; hybrid primary-hat + secondary-bamboo safety tie remains current recommendation (Ben + Steve).
- [ ] Material test for filter/gobo: matte paint on PLA, translucent PLA, frosted resin (Steve).
- [ ] Thermal test sealed hat in sun/heat with charger and LEDs operating (Ben + Steve).
- [ ] Send hat v1/v2 STL out for MJF evaluation after COTS fit is known (Steve).

## Firmware track

- [ ] Create board definitions for PowerFeather V2, FeatherS2 Neo, Atom Matrix, and custom target (Ben).
- [ ] Implement telemetry abstraction for charger/fuel gauge / battery monitor (Ben).
- [ ] Implement LED driver abstraction: IS31FL3741 I2C matrix, WS2812/NeoPixelBus, integrated board LEDs (Ben).
- [ ] Implement LED rail power abstraction (`VSQT`, onboard LED LDO, external rail enable) (Ben).
- [ ] Implement standard OTA maintenance mode; no ESP-NOW firmware chunks (Ben).
- [ ] Implement ESP-NOW heartbeat/state packets with jitter and sequence numbers (Ben).
- [ ] Implement low-battery modes: dim, LED hard-off, shipping mode (Ben).
- [ ] Implement watchdog/reset-reason/brownout logging (Ben).
- [ ] Implement field telemetry logging schema for BM 2026 → 2027 design data (Ben).
- [ ] Port `TalismanPatterns.cpp` into `firmware/core/pattern/` (Ben).
- [ ] Implement minimum-viable CA tick + render loop on bench (Ben).

## Production test / flashing

- [ ] Keep USB/pogo flashing as mandatory recovery path even if COTS boards support USB-C (Ben).
- [ ] Investigate JLCPCB / PCBWay firmware pre-flash only for custom-PCBA path (Ben).
- [ ] Write smoke-test host script: node ID, firmware version, battery, charge/fault, reset reason, peer count (Ben).
- [ ] Define production acceptance checklist for each fixture (Ben + Steve).

## Coordination with project team

- [ ] Confirm timing for Bamboo Pure air-ship of prototype lanterns to Steve in TN (Ben → Elliot / Dipta).
- [ ] Align with Elliot on rope-attachment decision (Ben).
- [ ] Confirm hat OD / height / bamboo-overlap to Vishnu so he can finalize renders (Ben).
- [ ] Pull `INV_2026_00401`, decompose cost, compare to COTS/custom BOMs (Ben).
- [ ] Get Steve on project's official core build team wiki (Ben → Elliot).
- [ ] Get shared access to Co-Work's wiki folder once cloud-hosted (Ben → Elliot).
- [ ] Drop lighting workstream digest into WhatsApp after COTS bench results (Ben).

## Community Mandala Program (parked until Elliot signs off)

- [ ] Validate concept with Elliot + Vishnu (Ben).
- [ ] Pipeline: photo → vectorize → constraint check → cone projection → STL.
- [ ] Brightness normalization in firmware or per-filter metadata.
- [ ] Cataloging schema.
- [ ] Contributor brief / submission form.
- [ ] Submission window opens/closes early enough for printing and reprints.
