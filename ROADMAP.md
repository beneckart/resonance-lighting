# Roadmap

Phases of work for the Resonance Lighting workstream, working backwards from Burning Man 2026.

**Hard deadline:** Late August 2026 build week starts at BRC. All 100 fixtures must be in hand and operational by ~Aug 20 to truck from Grass Valley pre-build staging to playa.

## Current critical-path principle

Do not make the custom PCBA the only path to success.

The project now runs two hardware tracks in parallel:

1. **COTS deployment/fallback track:** PowerFeather V2 / FeatherS2 Neo / Atom Matrix / DFR0559 candidates tested in real hats.
2. **Custom PCBA track:** bespoke board derived only after COTS tests identify the actual power, LED, telemetry, RF, and mechanical requirements.

## Phase 0 — documentation and decision correction

**Status:** Mostly done, with 2026-05-10 update in progress.

Deliverables:

- [x] Supersede custom-only ESP32-C3/CN3058/direct-LED assumptions.
- [x] Establish standard OTA only; no mesh-gossiped firmware images.
- [x] Establish WROOM/headroom/RF-module-first MCU criteria.
- [x] Establish dual-track COTS/custom hardware plan.
- [x] Establish switchable/default-off LED rail policy.
- [x] Establish bq25185/BQ25628E-class charger references.
- [ ] Add 2026-05-10 PowerFeather/COTS update docs.

## Phase 1 — COTS bench prototypes

**Window:** Now → parts arrival + ~2 weeks.
**Owner:** Ben, with Steve on optics/mechanical tests.
**Goal:** Measure real behavior of plausible production/fallback stacks before committing to custom hardware.

### Test stacks

1. **PowerFeather V2 + LiFePO4 + solar + Adafruit IS31FL3741 13x9 matrix.** Primary design-aligned stack.
2. **PowerFeather V2 + LiFePO4 + solar + M5Stack NeoHEX.** Alternate LED geometry stack.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback with integrated 5x5 optics.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback.

### Deliverables

- [ ] Identify whether Elecrow PowerFeather boards are V2 or V1.
- [ ] Confirm LiFePO4 behavior on actual PowerFeather V2 hardware.
- [ ] Measure sleep/active/radio/LED current on all stacks.
- [ ] Measure solar charge behavior on 1–5 W panels.
- [ ] Test standard OTA maintenance mode.
- [ ] Test ESP-NOW heartbeat/state packets.
- [ ] Test LED rail fail-safe behavior.
- [ ] Compare LED/gobo optics across IS31FL3741, NeoHEX, FeatherS2 Neo, Atom Matrix.
- [ ] RF test inside mock hat with panel/battery/wiring installed.
- [ ] Mechanical assembly-time test for each COTS stack.
- [ ] Create measured-results table and production recommendation.

### Go/no-go outputs

- **PowerFeather V2 viable?** If yes, it becomes production candidate and custom-PCBA reference.
- **IS31FL3741 viable optically?** If yes, it becomes primary no-solder LED module candidate.
- **NeoHEX significantly better optically?** If yes, consider GPIO/5 V LED rail path.
- **LiPo fallback acceptable?** If yes, DFR0559 + FeatherS2 Neo / Atom Matrix remains schedule rescue.

## Phase 2 — mechanical hat prototype around real electronics

**Window:** parallel with Phase 1.
**Owner:** Steve, with Ben input.
**Goal:** Fit real COTS stacks and solar/battery/LED modules into a hat form before custom board dimensions are frozen.

Deliverables:

- [ ] Hat v1 around PowerFeather + LED module + LiFePO4 + panel.
- [ ] Alternative mount points for FeatherS2 Neo / Atom Matrix fallback.
- [ ] Strain relief for solar panel pigtail / VDC connector.
- [ ] Antenna keep-out region that is not under panel/battery/metal.
- [ ] Filter/gobo placement and LED-to-filter distance tested.
- [ ] Thermal test in sun/heat.
- [ ] RF test inside printed/mock hat.
- [ ] Rope attachment recommendation: current default is primary on hat + secondary bamboo safety tie.

## Phase 3 — architecture decision: COTS production vs custom PCBA

**Window:** after Phase 1/2 data.
**Owner:** Ben + Steve.
**Goal:** Decide whether 2026 production uses COTS boards, a custom PCBA, or hybrid.

Decision criteria:

- measured power budget closes,
- solar harvest adequate,
- LED optics acceptable,
- RF range acceptable in hat,
- assembly time acceptable,
- sourcing of 100–150 units realistic,
- field-recovery path clear,
- enclosure fit acceptable.

Possible outcomes:

1. **COTS production:** use PowerFeather V2 or fallback stack directly.
2. **Hybrid production:** COTS controller/power board + custom LED/mechanical daughterboard.
3. **Custom production:** PowerFeather-derived custom PCBA.
4. **Fallback LiPo production:** DFR0559 + FeatherS2 Neo / Atom Matrix if LiFePO4 path slips.

## Phase 4 — custom board only if needed

**Window:** after COTS proof and architecture decision.
**Owner:** Ben, with external hardware review recommended.
**Goal:** Build only the board that the measured COTS tests justify.

Current reference architecture:

```
Solar panel / VDC connector
  → input protection
  → BQ25628E-class charger + power path
  → LiFePO4 cell + thermistor
  → MAX17260-class fuel gauge / current sense
  → TPS631013-class buck-boost 3.3 V
  → ESP32-S3-WROOM-class module
  → switched external LED/STEMMA rail
  → LED module connector(s)
```

Deliverables:

- [ ] Confirm whether PowerFeather V2 KiCad/Gerbers are available.
- [ ] Define custom schematic only after COTS data.
- [ ] Keep LED module separate unless optics are frozen.
- [ ] External schematic/layout review.
- [ ] Order 5 custom v1 boards only after review.
- [ ] Validate rails, charger, fuel gauge, RF, LED, OTA, and sleep.

## Phase 5 — production lock

**Window:** deadline determined by fab/procurement lead times.
**Owner:** Ben + Steve.
**Goal:** Lock the 100-unit architecture with enough time for procurement, assembly, and testing.

Deliverables:

- [ ] Board/module procurement plan for 110–120 units.
- [ ] Battery procurement plan.
- [ ] Solar panel procurement plan.
- [ ] Hat production plan.
- [ ] LED module procurement/custom plan.
- [ ] Flashing/recovery plan.
- [ ] Smoke-test rig.
- [ ] Assembly checklist.

## Phase 6 — integration and Grass Valley assembly

**Owner:** Ben + Steve.
**Goal:** All hats/electronics ready to mate with bamboo at Grass Valley.

Deliverables:

- [ ] Pre-assemble electronics into hats in CA/TN as practical.
- [ ] Pack spares, tools, chargers, cables, power meters, programmer/jig.
- [ ] Mate hats to bamboo lanterns.
- [ ] Install filters/gobos.
- [ ] Run smoke test.
- [ ] Log inventory and spare count.

## Phase 7 — BRC deployment

Deliverables:

- [ ] Hang fixtures.
- [ ] Charge day / solar verification.
- [ ] Mesh formation validation.
- [ ] Standard OTA maintenance update if needed.
- [ ] Tune lighting modes.
- [ ] Start field telemetry capture.

## Phase 8 — burn week operations

Deliverables:

- [ ] Daily walk-around.
- [ ] Swap failed fixtures with spares.
- [ ] Collect telemetry / failure data.
- [ ] Photograph / document failures.
- [ ] Test wand interactions if ready.

## Phase 9 — recovery and 2027 postmortem

Deliverables:

- [ ] Recover all fixtures.
- [ ] Categorize failures.
- [ ] Analyze battery/solar/thermal data.
- [ ] Update architecture for 2027.
- [ ] Refurbish or redesign as appropriate.

## Current risk register

| Risk | Severity | Mitigation |
|------|----------|------------|
| Elecrow boards are V1, not V2 | Medium | Identify by chip markings/I2C before LiFePO4; V1 remains LiPo fallback |
| PowerFeather V2 preliminary features not fully validated | Medium | Contact creator; bench-test all power modes before relying on it |
| LiFePO4 18650 sourcing weak | Medium | Call BatterySpace; identify alternate 18650/26650 suppliers; avoid multi-14430 packs unless forced |
| IS31FL3741 optics poor through gobo | Medium | Test NeoHEX, FeatherS2 Neo, Atom Matrix; keep LED module separate |
| NeoHEX not compatible with PowerFeather STEMMA-QT | Low | Treat NeoHEX as GPIO/WS2812 + separate rail experiment |
| Round solar panels slow to source | Low/Medium | Use rectangular panels for R&D; design hat top to allow later panel geometry swap |
| RF degraded by hat/panel/battery | Medium | RF test inside mock hat; maintain PCB antenna keep-out; avoid u.FL unless necessary |
| Custom PCBA takes too long | High | COTS production/fallback track remains active |
| Fancy OTA breaks fleet | High | Standard OTA only; no mesh-gossiped firmware images; USB/pogo recovery |
