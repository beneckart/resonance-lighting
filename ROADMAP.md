# Roadmap

Phases of work for the Resonance Lighting workstream, working backwards from Burning Man
2026.

**Hard deadline:** Late August 2026 build week starts at BRC. All 100 fixtures must be in
hand and operational by about Aug 20 to truck from Grass Valley pre-build staging to playa.

## Current Critical Path

Do not make a custom PCBA the only path to success. PowerFeather V2 is now the validated
COTS/reference architecture (ADR 0021), and the project should continue with the smallest
production path that closes energy, thermal, RF, optics, sourcing, and assembly.

Current gates:

1. **Energy sizing:** close role-specific nightly load vs measured harvest-at-MPP.
2. **Panel choice:** Voltaic P105/P126 outdoor tests, after BQ25628E OVP/HIZ guard.
3. **LED role placement:** ADR 0022 says both HEX and RGBW point-source are used; decide
   type mix and where each goes in the tree.
4. **Hat proof:** fit, antenna keep-out, panel retention, thermal behavior, rope/hybrid tie.
5. **Production path:** COTS PowerFeather V2, custom PowerFeather-derived assembly, or hybrid.
6. **Firmware hardening:** production OTA health/rollback, watchdog, low-battery modes,
   LED all-off/switchable rail behavior, and telemetry schema.

## Phase 0 - Documentation And Decision Correction

**Status:** Done through ADR 0022, with living TODOs for implementation.

Delivered:

- Superseded custom-only ESP32-C3/CN3058/direct-LED assumptions.
- Established standard OTA only; no mesh-gossiped firmware images.
- Established WROOM/headroom/RF-module-first MCU criteria.
- Established dual-track COTS/custom hardware plan.
- Established switchable/default-off LED rail policy.
- Adopted PowerFeather V2 as COTS/reference architecture.
- Validated PowerFeather V2 feasibility: ESP-NOW, solar path, no-touch OTA/rollback.
- Ruled out IS31FL3741 on the V2 shared I2C bus.
- Accepted mixed LED fleet by optical role: HEX + RGBW point source.

## Phase 1 - COTS Bench And Sizing Campaign

**Window:** In progress.
**Owner:** Ben, with Steve on optics/mechanical tests.
**Goal:** Finish measured inputs for the 100-unit buy.

Current test stack:

```
PowerFeather V2 + LiFePO4 + solar panel + direct-GPIO LED role
```

Validated:

- ESP-NOW heartbeat/state packets with jitter and sequence numbers.
- 5-node networking/range/rate feasibility, with 100-node projection.
- Battery-only standard OTA and A/B rollback.
- Watchdog/autosleep recovery.
- Solar telemetry over ESP-NOW.
- 2000 mAh LFP and 32700 capacity runs.
- Gobo verdict: HEX and RGBW point source both useful by role.

Open deliverables:

- [ ] Implement/test BQ25628E `VBUS_OVP=1` plus HIZ requalification kick.
- [ ] Run Voltaic P105/P126 outdoor MPP tests on a hungry LFP cell.
- [ ] Choose MPPT policy: fixed, temperature-compensated, or software P&O.
- [ ] Re-derive nightly power budget bottom-up by LED role and show duty cycle.
- [ ] Finish HEX 4.2 V boost bench test and boosted-build count/current cap.
- [ ] Capture keeper `led_studio` settings and gobo photos for both LED roles.
- [ ] Decide HEX/RGBW type mix and placement by tree height/sightline.
- [ ] Validate mock-hat RF with panel and battery installed.
- [ ] Run sealed-hat thermal test with charger and LEDs operating.

## Phase 2 - Mechanical Hat Prototype Around Real Electronics

**Window:** Parallel with Phase 1.
**Owner:** Steve, with Ben input.
**Goal:** Fit the production-real electronics, panel, battery, and optical module into a
hat that works on actual bamboo lanterns.

Deliverables:

- [ ] Hat v1 around PowerFeather-class board, LiFePO4 cell, panel, and both LED roles.
- [ ] Panel pocket/backup retention for P105/P126-class rectangular ETFE panels.
- [ ] Strain relief for solar panel pigtail / VDC connector.
- [ ] Battery retention and service path for one larger LFP cell.
- [ ] Antenna keep-out region away from panel/battery/metal/screws/wiring.
- [ ] Filter/gobo placement and LED-to-filter distance confirmed.
- [ ] Thermal test in sun/heat.
- [ ] Rope attachment recommendation: current default is primary on hat plus secondary
  bamboo safety tie.

## Phase 3 - Architecture Decision: COTS, Custom, Or Hybrid

**Window:** After Phase 1/2 data, before procurement lead time becomes risky.
**Owner:** Ben + Steve.
**Goal:** Decide the 2026 production architecture.

Decision criteria:

- energy budget closes by role;
- solar harvest adequate after MPP/OVP guard;
- LED optics and placement acceptable;
- RF range acceptable in the hat;
- thermal behavior acceptable in sun/heat;
- assembly time and connector plan acceptable;
- sourcing of 100-150 units realistic;
- field recovery path clear.

Possible outcomes:

1. **COTS production:** PowerFeather V2 plus role-specific LED/panel/harness.
2. **Hybrid production:** COTS controller/power board plus custom LED/power adapter.
3. **Custom production:** PowerFeather-derived custom PCBA or assembly.
4. **Fallback simplification:** reduce show duty cycle, role count, or panel split if
   procurement/timeline forces it.

## Phase 4 - Custom Board / Adapter Only If Needed

**Window:** After COTS proof and architecture decision.
**Owner:** Ben, with external hardware review recommended.
**Goal:** Build only the board that measured COTS tests justify.

Current reference architecture:

```
Solar panel / VDC connector
  -> input protection + bright-sun qualification guard
  -> BQ25628E-class charger + power path
  -> LiFePO4 cell + thermistor
  -> MAX17260-class fuel gauge / current sense
  -> TPS631013-class buck-boost 3.3 V
  -> ESP32-S3-WROOM-class module
  -> switchable/default-off LED rail
  -> direct-GPIO LED module connector(s)
```

Deliverables:

- [ ] Confirm whether PowerFeather V2 KiCad/Gerbers are available or licensable.
- [ ] Define custom schematic only after COTS data.
- [ ] Keep LED module separate unless optics are frozen.
- [ ] External schematic/layout review.
- [ ] Order prototype boards/adapters only after review.
- [ ] Validate rails, charger, fuel gauge, RF, LED, OTA, and sleep.

## Phase 5 - Production Lock

**Window:** Deadline determined by fab/procurement lead times.
**Owner:** Ben + Steve.
**Goal:** Lock the 100-unit architecture with enough time for procurement, assembly, and
testing.

Deliverables:

- [ ] Board/module procurement plan for 110-120 units.
- [ ] Battery procurement plan.
- [ ] Solar panel procurement plan, possibly role-specific.
- [ ] Hat production plan.
- [ ] LED module procurement/custom plan.
- [ ] Flashing/recovery plan.
- [ ] Smoke-test rig and acceptance checklist.
- [ ] Assembly checklist optimized for low per-fixture operations.

## Phase 6 - Integration And Grass Valley Assembly

**Owner:** Ben + Steve.
**Goal:** All hats/electronics ready to mate with bamboo at Grass Valley.

Deliverables:

- [ ] Pre-assemble electronics into hats in CA/TN as practical.
- [ ] Pack spares, tools, chargers, cables, power meters, programmer/jig.
- [ ] Mate hats to bamboo lanterns.
- [ ] Install filters/gobos.
- [ ] Run smoke test.
- [ ] Log inventory and spare count.

## Phase 7 - BRC Deployment

Deliverables:

- [ ] Hang fixtures.
- [ ] Charge day / solar verification.
- [ ] Mesh formation validation.
- [ ] Standard OTA maintenance update if needed.
- [ ] Tune lighting modes.
- [ ] Start field telemetry capture.

## Phase 8 - Burn Week Operations

Deliverables:

- [ ] Daily walk-around.
- [ ] Swap failed fixtures with spares.
- [ ] Collect telemetry / failure data.
- [ ] Photograph / document failures.
- [ ] Test presence/wand interactions if ready.

## Phase 9 - Recovery And 2027 Postmortem

Deliverables:

- [ ] Recover all fixtures.
- [ ] Categorize failures.
- [ ] Analyze battery/solar/thermal data.
- [ ] Update architecture for 2027.
- [ ] Refurbish or redesign as appropriate.

## Current Risk Register

| Risk | Severity | Mitigation |
|---|---|---|
| Energy budget does not close for desired show duty cycle | High | Role-specific budgets; larger P105 panel on RGBW fixtures; cap duty cycle gracefully. |
| BQ25628E rejects high-Voc panels in bright sun | High | Firmware `VBUS_OVP=1` plus HIZ requalification; shade panels during bench connect until fixed. |
| RF degraded by solar panel / battery / hat geometry | Medium | Mock-hat RF test; antenna keep-out; avoid u.FL unless necessary. |
| Sealed hat gets too hot for LFP charging | High | Thermal test; battery thermistor / charge-temp policy; venting/material changes if needed. |
| HEX boost enables destructive all-pixel current | Medium | Firmware count/current cap; rail enable control; bench before production. |
| COTS PowerFeather supply or connector labor fails at 100+ | High | Hybrid/custom assembly path; factory-soldered connectors; adapter PCB. |
| Firmware bug discovered after hanging | High | Standard OTA, A/B rollback, watchdog, USB/pogo recovery, spares. |
| Hat/rope attachment unresolved | Medium | Hybrid primary-hat plus secondary-bamboo safety tie is current recommendation; align with team. |

## Open Dependencies On Wider Team

1. Hat dimensions / visual envelope for Vishnu's renders.
2. Rope attachment decision: hat / bamboo / hybrid.
3. Air-ship timing for prototype bamboo lanterns to Steve.
4. Whether the Community Mandala Program proceeds.
5. `INV_2026_00401` decomposition for BOM comparison.
