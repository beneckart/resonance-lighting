# Roadmap

Phases of work for the Resonance Lighting workstream, working backwards from Burning Man 2026.

**Hard deadline:** Late August 2026 build week starts at BRC. All 100 fixtures must be in hand and operational by ~Aug 20 to truck from Grass Valley pre-build staging to playa.

## Updated strategy

The project now runs two hardware tracks in parallel:

- **Track A — COTS deployable prototype / production fallback.** This proves the full system quickly and remains viable for all 100 fixtures if custom PCBA slips.
- **Track B — custom PCBA optimization.** This may become production only if it passes bench, enclosure, RF, charger, OTA, and assembly gates early enough.

This roadmap intentionally de-risks around known hazards:

- No custom firmware-image gossip over ESP-NOW.
- No custom RF design.
- No first-time custom charger as the only path to production.
- No always-on LED rail that can drain the battery after MCU failure.
- No skilled per-fixture soldering or configuration.

## Logistics flow

1. Bamboo Pure air-ships a small batch of bamboo lanterns to Steve in Tennessee for early mechanical prototyping.
2. Bamboo Pure ships the tree structure + remaining lantern bodies by sea container to Grass Valley, CA.
3. Ben builds COTS electronics prototypes in CA and ships representative hardware to Steve as needed.
4. Steve iterates hat design against real bamboo and at least one COTS electronics stack.
5. Ben designs the custom PCBA only after COTS tests prove the electrical/optical/mechanical architecture.
6. Final architecture decision chooses either COTS stack, custom PCBA, or a hybrid.
7. Final fixture integration happens in CA / Grass Valley before trucking to BRC.

## Phase 0 — Architecture correction and research

**Window:** 2026-05-08 → 2026-05-15
**Owner:** Ben
**Goal:** Correct the specs before hardware work bakes in risky assumptions.

**Deliverables:**

- [ ] ADR 0010 accepted: standard OTA only, no mesh-gossiped firmware images.
- [ ] ADR 0011 accepted: MCU module selection based on pre-certified RF and headroom, not C3-MINI compactness.
- [ ] ADR 0012 accepted: dual-track COTS/custom production architecture.
- [ ] ADR 0013 accepted: LED rail switchable/default-off; exact rail chosen by test.
- [ ] ADR 0014 accepted: bq25185-class LiFePO4 charger reference preferred over CN3058-first design.
- [ ] COTS candidate matrix written and kept current.
- [ ] Root README, hardware README, firmware architecture, TODO, and roadmap updated.

## Phase 1A — COTS deployable prototype

**Window:** 2026-05-08 → 2026-05-31
**Owner:** Ben
**Goal:** Build real working lantern electronics without waiting for custom PCBA.

**Candidate hardware:**

- FeatherS2 Neo or equivalent 5x5 onboard LED board.
- ESP32-S3 Feather / Unexpected Maker FeatherS3[D] or equivalent high-headroom MCU board.
- DFRobot FireBeetle C6/C5 or equivalent solar-integrated ESP32 board.
- Adafruit bq25185 charger board(s) for LiFePO4 reference testing.
- DFRobot Solar Power Manager 5V for LiPo fallback testing.
- 1–3 W solar panels.
- LiFePO4 18650 cells and LiPo fallback cells.
- Pre-crimped JST/USB/STEMMA cabling.

**Deliverables:**

- [ ] At least two complete electronics stacks running on the bench.
- [ ] One LiFePO4 stack using a bq25185-class charger reference.
- [ ] One LiPo fallback stack using a proven COTS power board.
- [ ] 5x5 or 3x3 LED optical test at realistic LED-to-filter distance.
- [ ] Center-only projection mode tested for gobo crispness.
- [ ] 3-LED RGB/fringing mode tested.
- [ ] LED rail fail-safe behavior tested: stuck-on command, watchdog reset, low-battery cutoff.
- [ ] Standard OTA to one fixture tested via local WiFi; no ESP-NOW image transport.
- [ ] ESP-NOW state broadcast between two fixtures tested.
- [ ] Measured current for sleep, active radio, center LED, 3 LED mode, and all-on burst.

## Phase 1B — Firmware skeleton on COTS hardware

**Window:** 2026-05-10 → 2026-06-07
**Owner:** Ben
**Goal:** Firmware architecture works before custom hardware exists.

**Deliverables:**

- [ ] `firmware/core/` native tests for packet codec, CA tick, state transitions.
- [ ] Board definitions for at least two COTS targets.
- [ ] LED rail enable/disable support where hardware provides it.
- [ ] Battery/charger telemetry abstraction.
- [ ] ESP-NOW heartbeat and neighbor table.
- [ ] Standard OTA maintenance mode.
- [ ] Smoke-test host script: reports node ID, firmware version, battery, charge/fault, peer count, reset reason.

## Phase 2 — Mechanical prototyping

**Window:** 2026-05-15 → 2026-06-30
**Owner:** Steve, with Ben input
**Goal:** Hat geometry and filter material fit real bamboo and real electronics.

**Deliverables:**

- [ ] Bamboo Pure prototype lanterns air-shipped to Steve.
- [ ] Hat v1 designed around the COTS electronics stack envelope, not an imaginary final PCB only.
- [ ] Hat v1 printed on Bambu and fitted to real bamboo.
- [ ] Set-screw interface tested on multiple bamboo samples.
- [ ] Hybrid rope attachment evaluated: primary on hat, secondary safety tie around bamboo neck.
- [ ] Thermal test: sealed hat with charger + battery + LEDs in sun/heat.
- [ ] RF test: ESP-NOW/WiFi inside hat with solar panel, battery, screws, and wiring installed.
- [ ] Filter material test: matte paint on PLA, translucent PLA, frosted resin.
- [ ] Hat v2 sent out for one MJF nylon test print.

## Phase 3 — Custom PCBA v1, only after COTS proof

**Window:** 2026-06-10 → 2026-07-10
**Owner:** Ben
**Goal:** First custom board reproduces a proven COTS architecture.

**Preconditions:**

- COTS LiFePO4 stack works on bench.
- LED rail fail-safe design is known.
- MCU module family is selected by sourcing + headroom + RF constraints.
- Enclosure reserves safe antenna placement.
- Charger reference is selected.

**Deliverables:**

- [ ] Schematic module set: `solar_input`, `battery_charger`, `power_path`, `voltage_regulator`, `esp32_module`, `led_power`, `led_output`, `battery_monitor`, `test_pads`.
- [ ] 4-layer PCB unless reviewer explicitly says 2-layer is safe.
- [ ] Antenna keep-out reviewed against chosen module guidelines.
- [ ] Charger thermal reviewed.
- [ ] USB-C / pogo flashing reviewed.
- [ ] DRC clean.
- [ ] External human hardware review completed.
- [ ] 5 assembled boards ordered.
- [ ] Power-up tests: Vbat, system rail, 3V3, charger, LED rail switch, USB/pogo flash, ESP-NOW, standard OTA.
- [ ] Multi-day outdoor/enclosure test.

## Phase 4 — Production architecture gate

**Window:** 2026-07-10 → 2026-07-20
**Owner:** Ben + Steve
**Goal:** Pick the production path while there is still time to recover.

**Decision:** choose one:

1. **COTS production** — if COTS works and custom is late/risky.
2. **Custom PCBA production** — only if v1/v2 custom has passed real tests.
3. **Hybrid production** — custom controller/power board plus COTS/custom LED daughterboard.

**Hard pass criteria for custom production:**

- Standard OTA works on the board.
- USB/pogo recovery works.
- Charger behaves with real panel/cell/load.
- LED rail fail-safe passes stuck-on/reset/low-battery tests.
- RF works inside the final-ish hat.
- Sleep/active current closes the budget.
- Assembly can be done by an unskilled person in minutes.
- BOM/CPL/parts sourcing reviewed.

If any of these are not true by the gate date, production defaults to COTS/hybrid.

## Phase 5 — Production procurement

**Window:** 2026-07-20 → 2026-08-10
**Owner:** Ben
**Goal:** All electronics and enclosures ordered with spares.

**Deliverables:**

- [ ] 110–120 electronics sets ordered (100 + spares).
- [ ] 110–120 hats ordered/printed.
- [ ] 110–120 batteries ordered from vetted source.
- [ ] 110–120 solar panels ordered.
- [ ] All harnesses/cables pre-crimped / premade.
- [ ] Flashing jig and smoke-test rig built.
- [ ] Firmware release tagged.
- [ ] Production test script ready.

## Phase 6 — Pre-assembly and Grass Valley integration

**Window:** 2026-08-10 → 2026-08-22
**Owner:** Ben + Steve
**Goal:** All 100 fixtures assembled, flashed, tested, and packed for BRC.

**Deliverables:**

- [ ] Electronics installed in hats.
- [ ] Batteries and panels connected with strain relief.
- [ ] Firmware flashed or verified.
- [ ] Smoke test: all nodes report firmware version, battery, charge/fault status, reset reason, peer count.
- [ ] Hats mated to bamboo lanterns at Grass Valley.
- [ ] Filters installed.
- [ ] Spares packed separately.

## Phase 7 — Build week deployment

**Window:** ~2026-08-23 → 2026-08-27
**Owner:** Ben on-site at BRC
**Goal:** All fixtures hung, charged, and running the show.

**Deliverables:**

- [ ] Hang fixtures using final rope attachment method.
- [ ] Charge day in sun.
- [ ] Mesh/control-plane validation.
- [ ] Standard OTA maintenance update only if needed.
- [ ] Tune CA parameters live through normal control/config mechanisms.

## Phase 8 — Burn week operations

**Window:** ~2026-08-30 → 2026-09-06
**Owner:** Ben on-site, possibly Steve
**Goal:** Operate, observe, repair.

**Deliverables:**

- [ ] Daily health report from smoke-test/telemetry tool.
- [ ] Swap failed fixtures with spares.
- [ ] Document failure modes.
- [ ] Log battery/solar behavior for 2027.
- [ ] Test wand-lantern interaction with audience if stable.

## Phase 9 — Recovery and post-mortem

**Window:** Sept 2026
**Owner:** Ben

**Deliverables:**

- [ ] Recover fixtures.
- [ ] Inventory damage and battery health.
- [ ] Update `LOG.md` with field notes.
- [ ] Open ADRs/issues for 2027 iteration.

## Risk register

| Risk | Severity | Mitigation |
|------|----------|------------|
| Custom PCBA slips | High | COTS production fallback is real, not a toy |
| Custom OTA protocol bricks nodes | High | No custom OTA transport; standard OTA only + pogo recovery |
| RF under solar/battery/hat is poor | Medium | WROOM-style module, antenna keep-out, real enclosure RF tests |
| Charger behaves badly with weak solar | Medium | bq25185 COTS reference first, conservative charge current, thermal tests |
| LED rail stuck on drains battery | High | Switchable/default-off LED rail, watchdog/low-battery tests |
| LiFePO4 sourcing poor | Medium | Vet cells, discharge-test samples, keep US-side fallback |
| COTS stock insufficient | Medium | Buy candidates early; maintain custom/hybrid path |
| Hat thermal issues | Medium | Sealed-hat heat test before production |
| Assembly too slow | Medium | Time trial on 10 units; no hand solder/crimp/pairing |
