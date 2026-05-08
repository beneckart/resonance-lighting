# Roadmap

Phases of work for the Resonance Lighting workstream, working backwards from Burning Man 2026.

**Hard deadline:** Late August 2026 build week starts at BRC. All 100 fixtures must be in hand and operational by ~Aug 20 to truck from Grass Valley pre-build staging to playa.

**Logistics flow (confirmed):**

1. Bamboo Pure (Bali) air-ships a small batch of bamboo lanterns to **Steve in Tennessee** for early mechanical prototyping. Steve iterates on hat design with real bamboo in hand.
2. Bamboo Pure ships the rest of the project (the tree structure + remaining 100 lantern bodies) by sea container Bali → **Grass Valley, CA** (the project's pre-build staging area).
3. **Ben (CA)** designs and verifies the carrier PCB. Ships boards to Steve in Tennessee.
4. **Steve (TN)** finalizes the hat enclosure design with both bamboo lantern and PCB in hand. Prints/orders 100 hats.
5. Steve ships finalized hats to Ben (CA).
6. Ben drives hats + assembled electronics to **Grass Valley** to meet the rest of the bamboo arriving via container.
7. Final fixture integration (hat + bamboo lantern + LED + battery + filter) happens at Grass Valley.
8. Trucked from Grass Valley → BRC for build week.

The air-ship of prototype lanterns to Steve **decouples the lighting workstream from the May 10 Bali container schedule.** Electronics are not on the container at all. We work to our own schedule.

## Phase 0 — Decisions and documentation locked

**Status:** ✅ Done (2026-05-06).

Repo bootstrapped, BACKGROUND, AGENTS, ADRs 0001–0008, system block diagram + power budget, first-pass BOM, firmware architecture, atopile pattern, glossary. Roadmap (this file).

## Phase 1 — Bench prototype on existing TTGO modules

**Window:** 2026-05-07 → 2026-05-31 (~3 weeks).
**Owner:** Ben (firmware), Steve (mechanical, parallel).
**Goal:** Validate the entire system architecture on hardware that already exists. No custom PCB required.

**Hardware:**
- TTGO T-Beam (Steve's workshop) as the firmware-prototype platform. Has built-in TP4056 LiPo charger + USB.
- Small solar panel (1–2 W, ~$10) + LiPo 18650 (existing or ~$10) wired to T-Beam's LiPo pads.
- 1–4 WS2812B on a flex strip with JST connector.

**Deliverables:**
- [ ] Solar panel charges battery via T-Beam's onboard charger. Measured current under sun and shade.
- [ ] WS2812B drive via NeoPixelBus + I2S DMA. Animation runs cleanly while ESP-NOW is also active.
- [ ] Two T-Beams exchange ESP-NOW packets. Measured RSSI vs distance, packet loss vs distance, latency.
- [ ] OTA flash from a host (laptop) to one T-Beam. A/B partition flow validated.
- [ ] Firmware repo populated with `core/` + `esp32/` split. Native unit tests for CA tick and packet codec passing.
- [ ] Two-node CA running on bench: nodes broadcast state, render local LEDs. Visual validation that the CA logic produces the desired aesthetic.
- [ ] Power budget validated against measurements.

**Dependencies:** None — fully Ben-led on existing hardware.

## Phase 2 — Mechanical prototyping

**Window:** 2026-05-15 → 2026-06-30 (parallel with Phase 1, slightly trailing).
**Owner:** Steve, with Ben input on electronics envelope.
**Goal:** Final hat geometry and filter material that look right and fit a real bamboo lantern.

**Deliverables:**
- [ ] Bamboo Pure prototype lanterns air-shipped to Steve (TN). **Resolves prior dependency on Elliot's Bali trip.**
- [ ] Hat v1 designed in Fusion. Set-screw mechanical interface to bamboo neck, sealed cavity for electronics, mounting hole for solar panel.
- [ ] Hat v1 printed on Bambu, fitted to real bamboo. Iterate to v2/v3 as needed.
- [ ] Filter material test: matte paint on PLA, translucent PLA, frosted resin. Comparison shoot with WS2812B at different LED-to-filter distances. Choose material.
- [ ] Hat v2 (final-ish) STL sent out for one MJF nylon test print at JLC3DP. Verify mechanical fit, UV/heat tolerance.
- [ ] Rope attachment decision (with team).

**Dependencies:**
- Air-shipped bamboo lanterns arrive at Steve's workshop in TN.
- Team alignment on rope attachment (Vishnu, Ed, Elliot). Less time-pressured than before but still needed before final hat fab.

## Phase 3 — Custom carrier board v1

**Window:** 2026-06-15 → 2026-07-15 (~1 month, including JLCPCB turnaround).
**Owner:** Ben, with Claude pair-programming the schematic.
**Goal:** First physical custom PCB that reproduces what the bench prototype validated, but with LiFePO4 and the ESP32-C3-MINI-1 module.

**Deliverables:**
- [ ] atopile module library complete (`solar_input`, `lifepo4_charger`, `power_path`, `voltage_regulator`, `esp32_module`, `led_output`, `battery_monitor`).
- [ ] Top-level `resonance_carrier.ato` composed from modules.
- [ ] KiCad layout from atopile output. Hand-laid for mounting hole locations, RF antenna keep-out, thermal pad on CN3058.
- [ ] DRC clean. Manual review against gotcha checklist (ADR-pending: PCB review checklist).
- [ ] JLCPCB order: 5 boards SMT-assembled. Cost ~$30–60.
- [ ] Boards arrive. Power up, verify each rail. Flash firmware. Verify LED, mesh, OTA still work.
- [ ] Bench measurements: actual sleep current, actual charge current under sun, actual LED current at typical brightness.

**Dependencies:**
- Phase 1 firmware running on T-Beam (validates the architecture before committing to silicon).
- Phase 2 hat dimensions (so v1 board fits the enclosure).
- BOM lock (CN3058 confirmed in JLCPCB Basic, all other parts confirmed).

## Phase 4 — Carrier board v2 (only if v1 has issues)

**Window:** 2026-07-15 → 2026-08-01 (only if needed).
**Owner:** Ben.
**Goal:** Apply v1 fixes. Most likely class of issue: a missing decoupling cap or a swapped pin on the CN3058 (we have prior art for both).

**Deliverables:**
- [ ] v2 schematic delta documented as ADR.
- [ ] JLCPCB order: 5–10 v2 boards.
- [ ] Validate v2 fully on bench.
- [ ] **GO/NO-GO decision** for production fab.

**Dependencies:** Phase 3 v1 completed and tested.

## Phase 5 — Production fab

**Window:** 2026-08-01 → 2026-08-15 (~2 weeks for fab + ship + assembly).
**Owner:** Ben.
**Goal:** 100 working fixtures.

**Deliverables:**
- [ ] BOM final lock. CN3058 (or MCP73123 fallback), all parts confirmed.
- [ ] Final firmware rev tagged in git.
- [ ] **JLCPCB order: 110 boards SMT-assembled** (10 spares).
- [ ] **JLC3DP order: 110 hat enclosures in MJF nylon** (10 spares).
- [ ] **100 LiFePO4 18650 cells** (Battery Junction or vetted AliExpress).
- [ ] **100 solar panels** (Voltaic 2W ETFE preferred, alternates identified).
- [ ] **100 WS2812B chains** with JST connectors (PCB-mount or off-board flex strip per Phase 1 decision).
- [ ] Flashing jig built (USB-C breakout + pogo pin fixture, scripted auto-flash on insert).

**Dependencies:**
- Phase 4 GO decision.
- Phase 2 hat enclosure final geometry.

## Phase 6 — Cross-country logistics + Grass Valley integration

**Window:** 2026-08-10 → 2026-08-22.
**Owner:** Ben + Steve.
**Goal:** All 100 fixtures fully assembled at Grass Valley pre-build staging area, ready to truck to playa.

**Sub-phase 6a (early Aug):** Steve ships 100 finalized hat enclosures from TN → Ben in CA.

**Sub-phase 6b (~Aug 15):** Ben pre-assembles the electronics-and-hat half (PCB into hat, panel and battery wired, USB-flashed, sealed) at home in CA. Target ≤3 min/fixture × 100 = 5 hours.

**Sub-phase 6c (~Aug 20):** Ben drives 100 assembled solar-hats + spares + tools + flashing jig to **Grass Valley** to meet the rest of the bamboo arriving via container from Bali.

**Sub-phase 6d (Grass Valley, days):** Final fixture integration: mate hat to bamboo lantern, install filter at node notch, smoke-test (boot, charge, mesh, LED). Spares set aside. Then truck from Grass Valley → BRC.

**Deliverables:**
- [ ] 100 hats arrive at Ben's place in CA (from Steve, TN).
- [ ] 100 hats pre-assembled with PCB + panel + battery + USB-flashed in CA.
- [ ] 100 hats + 10 spares + tools + flashing jig packed for transport.
- [ ] All inventory arrives at Grass Valley.
- [ ] All 100 fixtures pass integration smoke test at Grass Valley.

**Dependencies:** Phase 5 fab complete. Bamboo container arrives at Grass Valley (handled by Elliot / Mainfreight, not us).

## Phase 7 — Build week deployment

**Window:** ~2026-08-23 → 2026-08-27.
**Owner:** Ben on-site at BRC.
**Goal:** All 100 fixtures hung in tree, charged, in mesh, running show.

**Deliverables:**
- [ ] Hang fixtures using rope per Phase 2 attachment decision.
- [ ] Charge day: leave panels in sun, top off all batteries.
- [ ] Mesh formation validation. All 100 fixtures discovered each other.
- [ ] OTA push: send the "playa show" firmware version to all 100 nodes.
- [ ] Tune CA parameters live (OTA pushes) until visual is right.

**Dependencies:** Phase 6 complete. Tree erected per Bamboo Pure / Bali container schedule (handled by other workstreams).

## Phase 8 — Burn week operations

**Window:** ~2026-08-30 → 2026-09-06.
**Owner:** Ben on-site at BRC, possibly Steve too.
**Goal:** Operate, observe, repair.

**Deliverables:**
- [ ] Daily walk-around: count working fixtures, log failures.
- [ ] Swap broken fixtures with spares (5-min swap target validated).
- [ ] Document failure modes (camera + notes) for 2027 iteration.
- [ ] Wand-lantern interactions tested with live audience.

## Phase 9 — Recovery and post-mortem

**Window:** Sept 2026.
**Owner:** Ben.

**Deliverables:**
- [ ] Recover all fixtures from tree at strike.
- [ ] Inventory damage. Categorize failure modes.
- [ ] Update `LOG.md` with field notes.
- [ ] Open new issues / ADRs for 2027 design iteration.

## Phase 10 — Off-season + 2027 R&D

**Window:** Oct 2026 → June 2027.
**Owner:** Ben + Steve + (Vishnu / Elliot for the conch shell expansion).

**Deliverables:**
- [ ] 2027 BM grant pitch (Elliot lead, supported by 2026 data).
- [ ] 2027 conch-shell lighting expansion design (additional fixtures, larger chandelier solar test bed for the shaded-panel question).
- [ ] Refurbish 2026 fixtures for reuse (battery rotation, hat repaint, firmware update).

## Critical path summary

```
      May    June   July   Aug    Sep
       │      │      │      │      │
Phase 0│✓     │      │      │      │
Phase 1│■■■■■ │      │      │      │   T-Beam bench prototype
Phase 2│  ■■■■│■■■■  │      │      │   Mechanical iteration
Phase 3│      │  ■■■■│■     │      │   Custom PCB v1
Phase 4│      │      │ ■■   │      │   v2 (if needed)
Phase 5│      │      │   ■■■│      │   Production fab
Phase 6│      │      │      │■     │   Assembly + ship
Phase 7│      │      │      │ ■■   │   Build week
Phase 8│      │      │      │  ■■■■│   Burn week
Phase 9│      │      │      │      │■■  Recovery
```

## Risk register

| Risk | Severity | Mitigation |
|------|----------|------------|
| CN3058 charger fails to behave per datasheet | Medium | Fallback to MCP73123 (designed-in pin compatibility) |
| Hat enclosure doesn't fit production bamboo lanterns (variability) | Medium | Set-screw mechanical interface; print 5 hats at MJF and try them on 5 bamboo lanterns before committing 100 |
| ESP-NOW range insufficient at full tree | Low | Bench validate Phase 1; mesh is multi-hop, so even 5 m range is fine |
| LiFePO4 cells from cheap source have poor real capacity | Medium | Spot-check 5 cells with bench discharge before ordering 100; have Battery Junction as US-side fallback |
| Solar panel mechanical mount fails in heat / vibration | Medium | Phase 4 stress test in summer parking lot; design margin in mount |
| 100-unit USB flashing takes longer than 1 hour | Low | Investigate JLCPCB pre-flash service; design pogo-pin jig + auto-flash-on-insert script as fallback (per ADR 0009) |
| One of the 100 fixtures has firmware bug discovered at burn | High | OTA architecture means we can push fixes during burn week; field-replaceable spares (10) buffer hardware failures |
| Steve or Ben unavailable due to life event | Medium | Document everything (this repo); cross-train where possible |

## Open dependencies on team

These need confirmation from Elliot / Vishnu / Dipta before timeline locks:

1. **Hat dimensions** to Vishnu so he can finalize renders. Doesn't block any internal phase but blocks Vishnu's deliverables.
2. **Rope attachment.** Hat / bamboo / hybrid. Affects Phase 2 hat design — needed by end of June at the latest, before Steve commits to the production hat geometry.
3. **2026 downlight scope** (full mesh/CA/OTA vs simple). Currently assumed full per the durable-infrastructure framing in `BACKGROUND.md`; worth a final confirmation from Elliot.
4. **Air-ship timing for prototype bamboo lanterns.** When does Bamboo Pure ship to Steve in TN? Affects when Phase 2 mechanical work can begin.
