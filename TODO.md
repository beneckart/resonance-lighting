# TODO

Active punch list. Status: `[ ]` open, `[~]` in progress, `[x]` done. Owner in parens.

## Immediate architecture corrections

- [ ] Add ADR 0010: standard OTA only; no mesh-gossiped firmware images (Ben).
- [ ] Add ADR 0011: MCU module selection based on pre-certified RF and firmware headroom (Ben).
- [ ] Add ADR 0012: dual-track COTS/custom production architecture (Ben + Steve).
- [ ] Add ADR 0013: LED rail must be switchable/default-off; exact voltage rail chosen by test (Ben).
- [ ] Add ADR 0014: bq25185-class LiFePO4 charger reference preferred; CN3058 fallback (Ben).
- [ ] Mark ADRs 0001, 0003, 0004, 0006, 0008 as superseded by the new ADRs (Ben).
- [ ] Update firmware architecture to remove ESP-NOW firmware gossip language (Ben).

## COTS candidate purchasing / bench track

- [ ] Buy/test Adafruit bq25185 USB/DC/Solar charger board or distributor equivalent (LiFePO4-capable) (Ben).
- [ ] Buy/test Adafruit bq25185 3.3 V buck variant if available (Ben).
- [ ] Buy/test Adafruit bq25185 5 V boost variant if useful for USB-style fallback (Ben).
- [ ] Buy/test DFRobot Solar Power Manager 5V DFR0559 as LiPo fallback power module (Ben).
- [ ] Buy/test FeatherS2 Neo or equivalent 5x5 LED board (Ben).
- [ ] Buy/test ESP32-S3 Feather / Unexpected Maker FeatherS3[D] or equivalent high-headroom ESP32 board (Ben).
- [ ] Buy/test FireBeetle C6/C5 solar board only as LiPo/COTS fallback unless chemistry can be changed safely (Ben).
- [ ] Buy/test Adafruit 5x5 NeoPixel BFF as LED layout reference; do not rely on it as no-solder production part unless header/assembly problem solved (Ben).
- [ ] Buy 1–3 W panels with connector/cable options suitable for the hat (Ben).
- [ ] Buy LiFePO4 18650 sample cells and LiPo fallback cells; label chemistry clearly (Ben).

## COTS prototype validation

- [ ] Build one LiFePO4 bq25185-based bench stack (Ben).
- [ ] Build one LiPo fallback bench stack using DFRobot/Feather/FireBeetle ecosystem (Ben).
- [ ] Measure sleep current, radio active current, center LED current, 3-LED mode current, and 25-LED burst current (Ben).
- [ ] Test standard OTA over local WiFi on a COTS board; validate rollback or recovery behavior (Ben).
- [ ] Test ESP-NOW heartbeat/state packets between two boards (Ben).
- [ ] Test LED rail fail-safe: stuck-on LEDs, watchdog reset, low-battery cutoff, cold boot from low battery (Ben).
- [ ] Test gobo projection with center LED, 3-LED RGB/fringing, and all-array animation modes (Ben + Steve).
- [ ] Time-trial assembly of one COTS electronics stack into a mock hat (Ben + Steve).

## Custom hardware track

- [ ] Select MCU module family after COTS tests and sourcing check; default bias toward WROOM-style module with headroom (Ben).
- [ ] Select charger reference after bq25185 bench tests (Ben).
- [ ] Define custom board v1 as carrier/controller/power board; keep LED daughterboard separate unless optics are frozen (Ben + Steve).
- [ ] Reserve antenna-safe board/enclosure placement before layout starts (Ben + Steve).
- [ ] Design `solar_input` module — panel connector, polarity/input protection (Ben).
- [ ] Design `battery_charger` module — bq25185-class preferred or selected reference (Ben).
- [ ] Design `power_path` module only if not provided by charger reference (Ben).
- [ ] Design `voltage_regulator` module — sized for selected ESP32 module and radio bursts (Ben).
- [ ] Design `esp32_module` module — selected pre-certified module, USB/reset/boot/strapping pins (Ben).
- [ ] Design `led_power` module — default-off switch/load switch/regulator enable (Ben).
- [ ] Design `led_output` module — data connector/daughterboard, series resistor, decoupling, optional level shifter footprint (Ben).
- [ ] Design `battery_monitor` module — ADC or fuel gauge plus charge/fault signals (Ben).
- [ ] Add pogo/test pads from v1 (Ben).
- [ ] External schematic/layout review before order (Ben).
- [ ] Order 5 custom v1 boards only after COTS proof and review (Ben).

## Production test / flashing

- [ ] Investigate JLCPCB programming/partial programming test details for ESP32 modules (Ben).
- [ ] Investigate PCBWay IC programming and post-assembly programming for ESP32 modules (Ben).
- [ ] Design USB/pogo flashing jig regardless of factory pre-flash availability (Ben).
- [ ] Write smoke-test host script: node ID, firmware version, battery, charge/fault, reset reason, peer count (Ben).
- [ ] Define production acceptance checklist for each fixture (Ben).

## Enclosure track

- [ ] Design hat v1 around COTS stack envelope, not just custom PCB placeholder (Steve).
- [ ] Print v1 on Bambu and iterate fit on real bamboo (Steve).
- [ ] Decide rope attachment point with team; hybrid primary-hat + secondary-bamboo safety tie is current recommendation (Ben + Steve).
- [ ] Add mounting bosses/standoffs for both COTS and custom board options (Steve).
- [ ] Material test for filter/gobo: matte paint on PLA, translucent PLA, frosted resin (Steve).
- [ ] RF test inside real/printed hat with panel, battery, screws, and wiring installed (Ben + Steve).
- [ ] Thermal test sealed hat in sun/heat with charger and LEDs operating (Ben + Steve).
- [ ] Send hat v1/v2 STL out for MJF print at JLC3DP / PCBWay / Xometry for evaluation (Steve).

## Firmware track

- [ ] Board definitions for COTS candidates and custom target (Ben).
- [ ] Port `TalismanPatterns.cpp` into `firmware/core/pattern/` (Ben).
- [ ] Implement minimum-viable CA tick + render loop on bench (Ben).
- [ ] Implement LED rail enable/disable abstraction and fail-safe tests (Ben).
- [ ] Implement battery/charger telemetry abstraction (Ben).
- [ ] Implement ESP-NOW heartbeat/state packets with jitter and sequence numbers (Ben).
- [ ] Implement standard OTA maintenance mode; no ESP-NOW firmware chunks (Ben).
- [ ] Implement low-battery modes: dim, LED hard-off, shipping mode (Ben).
- [ ] Implement watchdog/reset-reason/brownout logging (Ben).

## Coordination with project team

- [ ] Confirm timing for Bamboo Pure air-ship of prototype lanterns to Steve in TN (Ben → Elliot / Dipta).
- [ ] Align with Elliot on rope-attachment decision (Ben).
- [ ] Confirm hat OD / height / bamboo-overlap to Vishnu so he can finalize renders (Ben).
- [ ] Pull `INV_2026_00401`, decompose cost, compare to COTS/custom BOMs (Ben).
- [ ] Get Steve on the project's official core build team wiki (Ben → Elliot).
- [ ] Get shared access to Co-Work's wiki folder once cloud-hosted (Ben → Elliot).
- [ ] Drop lighting workstream digest into WhatsApp after ADR correction pass (Ben).

## Community Mandala Program (parked until Elliot signs off)

- [ ] Validate concept with Elliot + Vishnu (Ben).
- [ ] Pipeline: photo → vectorize → constraint check → cone projection → STL.
- [ ] Brightness normalization in firmware or per-filter metadata.
- [ ] Cataloging schema.
- [ ] Contributor brief / submission form.
- [ ] Submission window opens/closes early enough for printing and reprints.
