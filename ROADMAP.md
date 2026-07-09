# Roadmap

Phases of work for the Resonance Lighting workstream, working backwards from Burning Man
2026.

**Hard deadline:** all ~150 fixtures (four classes -- see the fleet table in
`docs/block-diagram/SYSTEM.md`) must be in hand and operational by about Aug 20 --
**the container loads in Nevada City on Aug 21**.

Project-wide anchor dates (gold standard: https://resonancenetwork.org/camp,
corroborated 2026-07-08): container lands Port of Oakland **Jul 12**; NC prebuild at
Bodhi Hive, Nevada City **Jul 31 - Aug 19**; all-hands container unload **Aug 1-2**;
**lights + camp systems team build Aug 8-9**; container load **Aug 21**; early crews
roll Aug 22-24; gates open Aug 30; burn night Sep 5. (Earlier repo docs said "Grass
Valley" for the staging area; the site pins it as Bodhi Hive, Nevada City.)

## Current Critical Path

The production path is decided: **COTS PowerFeather V2 at ~150 units (ADR 0024)**, with
procurement largely executed (see `ops/PROCUREMENT.md`). The remaining gates are
integration and hardening, not architecture:

1. **Energy sizing:** close role-specific nightly load vs measured harvest-at-MPP
   (panels measured 2026-06-29; budget-by-role still open).
2. **Uplight/chandelier power:** off-light panel vs solar-free 20 Ah vs budgeted 6 Ah
   -- bench test on the 20 Ah samples gates the batteryspace buy (ADRs 0025/0026).
3. **Hat proof:** fit, antenna keep-out, panel retention, thermal behavior, rope/hybrid
   tie; new uplight/chandelier enclosure variants (battery-in-cylinder, USB-C port).
4. **Firmware productization:** ADR 0023 low-battery state machine, production OTA
   health/rollback, watchdog, telemetry schema, sensor + choreography integration,
   and the RGBW feed decision (rail vs VBAT -- forks harness + pinout, ADR 0029).
5. **Procurement completion:** 82-board Elecrow batch (invoice 2026-07-10), cabling
   (JST-XH + crimped), USB-C ports, conditional 20 Ah cells, noisemaker parts.
6. **Assembly at 150-unit scale:** smoke-test rig, acceptance checklist, low
   per-fixture operations (ADR 0009).

## Phase 0 - Documentation And Decision Correction

**Status:** Done through ADR 0029, with living TODOs for implementation.

Delivered:

- Superseded custom-only ESP32-C3/CN3058/direct-LED assumptions.
- Established standard OTA only; no mesh-gossiped firmware images.
- Established WROOM/headroom/RF-module-first MCU criteria.
- Established dual-track COTS/custom hardware plan.
- Established switchable/default-off LED rail policy.
- Adopted PowerFeather V2 as COTS/reference architecture.
- Validated PowerFeather V2 feasibility: ESP-NOW, solar path, no-touch OTA/rollback.
- Ruled out IS31FL3741 on the V2 shared I2C bus.
- Accepted mixed LED fleet by optical role: HEX + RGBW point source (ADR 0022).
- Derived measured LFP dim/off/sleep power-policy thresholds (ADR 0023).
- Locked COTS production at ~150 units in four fixture classes (ADR 0024).
- Qualified the production battery vendor; rejected the impostor (ADR 0025).
- Selected Voltaic ETFE panels with role mix (ADR 0026).
- Selected the sensor architecture: MSA311 + multizone ToF by class (ADR 0027).
- Recorded the power-management bus-integrity rules from the reboot hunt (ADR 0028).
- Recorded the measured LED electrical drive per role; shelved the boost (ADR 0029).

## Phase 1 - COTS Bench And Sizing Campaign

**Window:** In progress (largely complete).
**Owner:** Ben, with Steve on optics/mechanical tests.
**Goal:** Finish measured inputs for the production fleet (buy now largely executed --
see `ops/PROCUREMENT.md`).

Current test stack:

```
PowerFeather V2 + LiFePO4 + solar panel + direct-GPIO LED role
```

Validated:

- ESP-NOW heartbeat/state packets with jitter and sequence numbers.
- 5-node networking/range/rate feasibility, with 100-node projection (150-node
  extrapolation re-check queued -- ADR 0024).
- Battery-only standard OTA and A/B rollback; low-VBAT OTA brackets (~3.10 V
  battery-only / 2.901 V solar-assisted / 2.496 V USB-assisted).
- Watchdog/autosleep recovery.
- Solar telemetry over ESP-NOW; multi-day outdoor field-cycle solar lifecycle runs.
- 2000 mAh LFP and 32700 capacity runs; production cell qualified n=2 and vendor
  locked (ADR 0025); power-policy thresholds derived (ADR 0023).
- Gobo verdict: HEX and RGBW point source both useful by role (ADR 0022).
- Bus-integrity root cause of the reboot epidemics; sealed by the 46 h soak (ADR 0028).
- LED electrical drive matrix measured; boost shelved (ADR 0029).

Open deliverables:

- [x] Implement/test BQ25628E `VBUS_OVP=1` plus HIZ requalification kick.
  (Implemented 2026-06-29 as `firmware/powerfeather_solar_guard.h`, baseline in all
  charging sketches; bright-sun hardware validation still pending.)
- [x] Run Voltaic P105/P126 outdoor MPP tests on a hungry LFP cell. (2026-06-29:
  P105 ~3.8-3.9 W panel-side, P126 ~1.89 W; hungrier P105 re-run still queued.)
- [ ] Choose MPPT policy: fixed, temperature-compensated, or software P&O
  (`--field-mppt` perturb firmware built, not yet deployed).
- [ ] Re-derive nightly power budget bottom-up by LED role and show duty cycle.
- [x] Finish HEX 4.2 V boost bench test. (2026-07-02 campaign: boost NOT worth it for
  HEX at healthy SOC; boost shelved; RGBW rail-vs-VBAT feed measured but the
  production decision is open -- ADR 0029. The boosted-build count/current cap is
  moot unless the boost is revived.)
- [ ] Capture keeper `led_studio` settings and gobo photos for both LED roles.
- [~] Decide HEX/RGBW type mix and placement. (Fleet plan recorded 2026-07-08 --
  SYSTEM.md fleet table; counts tentative until installation.)
- [ ] Validate mock-hat RF with panel and battery installed.
- [ ] Run sealed-hat thermal test with charger and LEDs operating.
- [ ] Bench-test the 20 Ah solar-free option on the two samples (gates the
  batteryspace #6832 buy for uplights/chandelier).

## Phase 1b - Presence Sensing / Interactivity (active workstream)

**Owner:** Ben. **Status:** sensor architecture decided (ADR 0027); choreography open.

- Five-sensor presence bench live behind a TCA9548A mux (2026-07-02).
- sway_demo validated the MSA311 + VL53L5CX accel-vs-ToF chain on real geometry
  (2026-07-06/07); spin-invariant mount-zero; accel near spin axis rule.
- Open: walk-under datasets, lantern-rig splay-occlusion session, outdoor lantern
  test, winning-sensor heartbeat integration, mesh choreography firmware
  (ripple/wand/CA modes), findings report to Elliot. See TODO.md presence section.

## Phase 1c - Noisemaker (active workstream, decision OPEN)

**Owner:** Ben. **Status:** everything is still technically on the table; Ben leans
physical. Wider crowd input expected at the first big camp-wide meeting 2026-07-09.

- Candidate A (STEMMA speaker #3885 + percussion synth) proven clean on hardware
  (fw .9, 2026-07-07); loudness and current draw still unmeasured; exposed trim pot
  is a fleet liability (bridge/epoxy or evaluate MAX98357A I2S).
- Candidate B (MOSFET + solenoid mallet striking the bamboo) untested -- first bench
  test is the priority.
- Relay clicks (and even simple beeps) remain live options: early small-n crowd
  testing disliked square waves and was mixed on clicks, but the sample was small
  and listeners may not have imagined 150 rippling through the tree. The $18/unit
  Omron relay is out on cost; cheaper relays are not. See LOG 2026-07-07 + TODO.md.

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

**RESOLVED 2026-07-08: COTS PowerFeather V2 at ~150 units (ADR 0024; 68 + 82 board
Elecrow buy).** The criteria below are kept as the historical decision framework.

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

**Status 2026-07-08: not on the 2026 critical path (ADR 0024) -- this is the 2027
option.** Any future board carries ADR 0028's dedicated power-management-bus rule and
ADR 0029's LED wiring rules.

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
**Goal:** Lock the ~150-unit architecture with enough time for procurement, assembly, and
testing. **Status 2026-07-08: procurement largely executed -- the ledger is
`ops/PROCUREMENT.md`, the per-class BOM is `ops/bom.md`.**

Deliverables:

- [x] Board/module procurement: 68 boards received-class + 82 invoicing 2026-07-10
  (Elecrow). Spares thin -- see risk register.
- [x] Battery procurement: 175x 32700 6 Ah bought (ADR 0025); 20 Ah decision open.
- [x] Solar panel procurement, role-specific: 110x P105 + 50x P126 + 160 pigtails
  (ADR 0026).
- [x] LED module procurement: 100x RGBW + 110x HEX-class bought (~60 spares).
- [ ] Cabling/connector buy: JST-XH right-angle headers + pre-crimped harness,
  USB cabling + panel-mount USB-C ports (solar-free classes).
- [ ] Hat production plan (Steve; now four enclosure variants incl. uplight "boot").
- [ ] Flashing/recovery plan.
- [ ] Smoke-test rig and acceptance checklist.
- [ ] Assembly checklist optimized for low per-fixture operations.

## Phase 6 - Integration And Nevada City Assembly

**Owner:** Ben + Steve.
**Goal:** All hats/electronics ready to mate with bamboo at the Nevada City prebuild
(Bodhi Hive, Jul 31 - Aug 19; lights team build Aug 8-9; container loads Aug 21).

Deliverables:

- [ ] TENTATIVE: Ben TN trip ~3rd-4th week of July to test the ~70 boards at
  Steve's at fleet scale -- basic production-firmware mesh lighting effects +
  presence detection, indoors if enclosures aren't ready. Back for the Aug 1-2
  container unload. Decide by mid-July.
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
| BQ25628E rejects high-Voc panels in bright sun | Medium (guard shipped, bright-sun hardware validation pending) | `firmware/powerfeather_solar_guard.h` baseline in all charging sketches; shade panels during bench connect until validated. |
| RF degraded by solar panel / battery / hat geometry | Medium | Mock-hat RF test; antenna keep-out; avoid u.FL unless necessary. |
| Sealed hat gets too hot for LFP charging | High | Thermal test; battery thermistor / charge-temp policy; venting/material changes if needed. |
| PowerFeather spares thin (150 bought vs 150-152 plan; ~8 bench boards as buffer) | High | 82-board batch lands ~mid-July (invoice 2026-07-10); further Elecrow top-up if allowed; deploy count can trim to boards on hand. |
| Second Elecrow batch (82 boards) slips past assembly window | High | Invoice/ship 2026-07-10 per rep; track receipt in `ops/PROCUREMENT.md`; escalate immediately if not shipped. |
| 20 Ah solar-free decision starves uplight/chandelier build time | Medium | Bench test on samples ASAP; fallback is budgeted 6 Ah cells already on hand (175 bought). |
| Cabling/connector buy (JST-XH, USB-C ports) not yet placed | Medium | Small-dollar, short-lead items -- order once counts firm; tracked in the to-buy queue. |
| Firmware bug discovered after hanging | High | Standard OTA, A/B rollback, watchdog, USB/pogo recovery, spares. |
| Hat/rope attachment unresolved | Medium | Hybrid primary-hat plus secondary-bamboo safety tie is current recommendation; align with team. |

Retired risks: HEX-boost destructive all-pixel current (boost shelved -- ADR 0029);
COTS supply at 100+ (150 boards bought/committed -- ADR 0024); battery vendor
uncertainty (qualified n=2 -- ADR 0025).

## Open Dependencies On Wider Team

1. Hat dimensions / visual envelope for Vishnu's renders.
2. Rope attachment decision: hat / bamboo / hybrid.
3. Air-ship timing for prototype bamboo lanterns to Steve.
4. Gobo pattern program: community submissions PULLED (2026-07-08, time). Current
   plan is in-house designs plus generative-AI-modulated bamboo-leaf patterns per
   bamboo species used in the tree (see BACKGROUND.md).
5. Chandelier electronics scope/ownership (16 shafts; internals fungible with the
   fleet -- ADR 0024).

Retired: `INV_2026_00401` decomposition -- the invoice's identity is unclear
(probably the Bamboo Pure lantern invoice, possibly the early custom-PCBA quote);
with real COTS procurement recorded in `ops/PROCUREMENT.md` it is no longer a useful
comparison baseline (2026-07-08).
