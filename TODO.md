# TODO

Active punch list. Status: `[ ]` open, `[~]` in progress, `[x]` done. Owner in parens.

## Immediate documentation / repo hygiene

- [ ] Add `LOG_APPEND_2026-05-10.md` entry to `LOG.md` (Ben).
- [ ] Add ADR 0015 — PowerFeather V2 as leading COTS/reference architecture (Ben).
- [ ] Add ADR 0016 — COTS prototype shortlist after purchases (Ben).
- [ ] Add ADR 0017 — Battery cell format and sourcing (Ben).
- [ ] Add/rewrite ADR 0018 — LED module/interface plan. **Content now settled by 2026-06-04 bench work:** IS31 13×9 OUT (shared-bus brownout); **SK6812 "HEX" driven direct-GPIO @ 3.3 V off the I2C bus = primary** (distributed dimmable glow, no boost, ~1.6× more efficient than WS2812C NeoHEX); 4 W RGBW = single ultra-bright beacon option but needs 5 V. See LOG 2026-06-04 entries + `ops/bench/data/ca/led-par-vs-draw.png` (Ben).
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
- [x] Build interim Atom + Atomic Battery Base + M5Stack Unit NeoHEX stack over Grove (Ben).
- [x] Flash USB smoke-test firmware to Adafruit Feather ESP32-C6, FeatherS2 Neo, and Atom Matrix; record MAC, reset reason, board type, firmware version, and LED/I2C status (Ben).
- [x] Install/check smoke-test Arduino libraries: Adafruit IS31FL3741, Adafruit GFX, Adafruit BusIO, and a WS2812-capable LED library for integrated 5x5 boards (Ben).
- [x] Decide first OTA maintenance-mode mechanism for COTS smoke firmware: local WiFi AP credentials vs board-hosted temporary AP/web updater (Ben).
- [x] Test home-WiFi web OTA upload end-to-end on all three COTS smoke boards (Ben).
- [ ] Test temporary AP / portable-router OTA upload path before field-style testing (Ben).
- [x] ~~Build Track A: PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 matrix~~ — **SUPERSEDED: IS31 ruled out** (shared-bus brownout, 2026-06-04). LED axis → SK6812 HEX direct-GPIO (Ben).
- [~] Build Track B: PowerFeather V2 + LiFePO4 + solar panel + WS2812/SK6812 LED — **this is now the path** (HEX direct-GPIO on a free pin @ 3V3, off the I2C bus). LED side validated; remaining: solar + enclosure (Ben).
- [ ] Build Track C: FeatherS2 Neo + DFRobot DFR0559; Feather battery JST left empty (Ben).
- [ ] Build Track D: Atom Matrix + DFRobot DFR0559 (Ben).
- [ ] Run incoming inspection and board-ID procedure from COTS test plan (Ben).
- [ ] Measure sleep current for each stack (Ben).
- [ ] Measure active/radio/ESP-NOW current for each stack (Ben).
- [~] Measure LED current for center-only, 3-pixel, 9-pixel/crop, and full-array capped modes (Ben).
- [ ] Add SEN0291 I2C wattmeters to power-test harness and bench worksheet when they arrive (Ben).
- [ ] Run iso-current LED brightness/gobo comparison from `docs/tests/ISO_CURRENT_LED_BRIGHTNESS_TEST_2026-05-18.md` (Ben + Steve).
- [ ] Measure solar charge behavior for each 1–5 W panel in sun/shade/heat (Ben).
- [ ] Test low-battery + solar recovery for PowerFeather V2 and fallback stacks (Ben).
- [ ] Test standard OTA maintenance mode on at least two COTS boards (Ben).
- [ ] Test ESP-NOW heartbeat/state packets with jitter/sequence numbers (Ben).
- [ ] Test LED fail-safe: stuck LEDs, MCU hang, watchdog reset, rail-off recovery (Ben).
- [ ] Test gobo projection with IS31FL3741, NeoHEX, FeatherS2 Neo, Atom Matrix (Ben + Steve).
- [ ] RF test each candidate inside a mock hat with panel/battery/wiring installed (Ben + Steve).
- [ ] Time-trial COTS stack assembly into mock hat (Ben + Steve).
- [~] Capture NeoHEX passive adapter Rev A in KiCad from `hardware/led-adapter/neohex-passive-rev-a/` design packet; PCBA-friendly starter PCB exists, schematic remains (Ben).
- [x] Replace NeoHEX adapter through-hole connector candidates with SMT PCBA-friendly candidates: local M5Stack A118 HY2.0-4P SMD footprint for J1 and stock SMT JST-PH for J2 (Ben).
- [x] Add J5 JST-SH/STEMMA-QT fallback output for Adafruit 4528-style Grove-to-STEMMA-QT cable on NeoHEX adapter Rev A (Ben).
- [x] Prepare PCBWay quick-turn PCBA upload packet for NeoHEX adapter Rev A (Ben).
- [~] Revise PCBWay NeoHEX adapter assembly quote to DNP J1/A118 and build through J5 fallback output (Ben + PCBWay).
- [ ] Physically verify NeoHEX adapter J1 A118 candidate footprint against M5Stack Grove/HY2.0 cable and confirm pin order before ordering (Ben).
- [ ] Physically verify NeoHEX adapter J5 fallback output with Adafruit 4528-style Grove-to-STEMMA-QT cable and confirm signal lands on J5.4 (Ben).
- [ ] Verify J2 SMT JST-PH power connector against selected pre-crimped power leads, or swap to SMT JST-GH if preferred (Ben).
- [ ] Capture NeoHEX passive adapter schematic in KiCad and back-annotate the PCB from it (Ben).
- [ ] Order NeoHEX passive adapter Rev A quick-turn boards and record fab/shipping turnaround (Ben).

## PowerFeather power-bench (2026-06-02, board 9E5AB8 — see docs/tests/POWER_BENCH_HARNESS_2026-06-02.md)

- [x] Stand up Arduino power-bench firmware `firmware/power_bench/` with SDK 2.1.0 telemetry + `/telemetry` JSON (Ben).
- [x] Confirm V2 hardware via Wire1 scan: MAX17260 (0x36), BQ25628E (0x6A), IS31 (0x30) (Ben).
- [x] `Board.init(4400, Generic_3V7)` Ok; battery/supply voltage + current read correctly over WiFi (Ben).
- [x] Wire up IS31FL3741 13x9 on the STEMMA-QT bus (Wire1, GPIO47/48) shared with the SDK (Ben).
- [x] Host logger `ops/bench/power_logger.py` + `power_summary.py` + site-partitioned JSONL data layout (Ben).
- [x] Resolve MAX17260 SOC/health/cycles `InvalidState` — root cause was the missing `-DPOWERFEATHER_BOARD_V2=1` compile flag (SDK fell back to V1 LC709204F gauge); now in build.sh + #error guard. SOC/health/cycles/time_left populate (Ben).
- [ ] Verify BQ25628E charger telemetry: state, faults, input regulation, charge current (Ben).
- [x] Add NeoHEX + single-RGBW LED build variants on bench — DONE: `--led neohex/rgbw1/neodriver`, `--pixel-pin` to drive WS2812/SK6812 direct on any free GPIO (used A0/GPIO10) (Ben).
- [ ] Solar harvest sweep across panels/conditions; set `RES_PF_MAINTAIN_V` to panel MPP (Ben).
- [x] Clean LED-current runs — DONE via `--bright-sweep` on battery + `--wifi-lowpower` (steadies the WiFi baseline so small LED currents resolve); the gauge `ima` is the metric, charging masks it so runs are on battery (Ben).
- [ ] Steve mirrors the bench in TN; merge JSONL via the repo (Steve).
- [ ] Add live telemetry readout to `ops/bench/cots-mode-dashboard.html` (Ben).
- [x] Configure LiFePO4 profile on V2 — DONE: `--chem lfp` (`RES_PF_BATTERY_TYPE=Generic_LFP`) used throughout; LFP favored for safety/heat/cycles (counterpoint = buck-boost crossover tax, see Field reliability) (Ben).

## Battery-brownout investigation (see docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md — LARGELY RESOLVED 2026-06-04: cause = IS31FL3741 chip on the V2's shared charger/gauge I2C bus + WiFi; fix = don't put LEDs on that bus → direct-GPIO WS2812/SK6812. Remaining items are follow-ups, not blockers.)

- [x] Characterize exact conditions for VSYS power-on reset on battery — ~~load-stacking~~ ~~not-reproducible~~ **UPDATED 2026-06-04: brownout CAME BACK** — board 1 did a 794-reboot loop overnight on battery (poweron, healthy bv 3.24–3.46, all SOC, lightest load, dying at WiFi association). Real + intermittent on board 1 ⇒ H2 (marginal connection) strengthened. See doc Status (Ben).
- [x] Repeat brownout characterization on a second board + known-good cell — DONE (n=3 all stable in short runs); **NOW extending: pristine board 2 multi-hour with fixed guard to see if it loops like board 1 overnight (board-specificity)** (Ben).
- [x] Fix the overnight auto-sleep guard — RAM coulomb/timer state reset every reboot, so a brownout loop defeated it. Added **NVS-persisted reboot-loop breaker** (`--autosleep`, ≥25 sub-survival boots ⇒ deep sleep before WiFi); fw power-bench-2026-06-04.1 (Ben).
- [ ] **Inspect/reflow board 1's battery + VDC solder joints** under magnification (cold joint / flux / hairline bridge); re-run overnight to confirm the loop is gone — the direct H2 test now that it reproduces (Ben).
- [x] **Verify the reboot-loop breaker actually fires** — DONE: board 2 looped and the breaker deep-slept it (validated in the wild) (Ben).
- [x] **FIX loop-breaker brick-risk** — DONE (fw 2026-06-04.2): (1) **never deep-sleep while external supply is present** (USB/VDC → stay flashable/recoverable — the root cause of the stranding); (2) sleep with a **15-min timer wake** (not indefinite); (3) on a timer wake still on battery → re-sleep, on supply → run/charge. So plugging USB self-recovers within one wake interval; never bricks. **VALIDATED LIVE 2026-06-04** (3 mAh budget / 60 s wake test): ran on USB w/o sleeping → coulomb-budget sleep on battery → 124 s of timer-wake/re-sleep → recovered (charging, fresh boot) on USB plug, no BOOT+RESET needed. Tuning flags added: `--budget-mah`, `--wake-s` (Ben/Claude).
- [ ] Keep a **VSYS bulk cap as cheap insurance** and bench-characterize it opportunistically — demoted from "key fix" to "nice to have" now that 3 boards are stable without it (Ben).
- [ ] Watch for brownout **recurrence in the field**; if it returns, capture which connection/board and whether re-seating clears it (Ben).
- [x] Distinguish IS31-specific vs any-I2C-device: **NeoDriver (5766, SeeSaw I2C) on the same bus = STABLE** (371 s+, through heavy WiFi) where the IS31 loops in ~1 min ⇒ **brownout is IS31-SPECIFIC**, not the bus. NeoDriver+WS2812 is a viable no-solder LED path (Ben).
- [ ] **Confirm NeoDriver robustness with an hours/overnight run** (the IS31 was intermittent — stable for minutes then failed; n=1/6min not enough). Do AFTER the wake-source fix (Ben).
- [x] **Direct-GPIO WS2812/SK6812 validated** (board 2, HEX on A0/GPIO10, off the I2C bus) — works, brownout-safe by construction, and ~10% MORE efficient than via NeoDriver (no passthrough drop). **BOM front-runner: SK6812 HEX + direct-GPIO.** 3-way plot `led-eff-3way.png` (Ben).
- [ ] **LED bring-up sequencing for production:** WS2812 latch last frame (send explicit all-off to blank); avoid full-white inrush on hot-connect (ramp gently); direct-GPIO's full VCC browns a marginal cell sooner → cap brightness / healthy pack (Ben).
- [ ] **Decide pixel-power architecture (NeoDriver only level-shifts the DATA signal, does NOT boost pixel power — corrected; "3-5V vin/vlogic" = accepted *input* range):** pixels run at whatever Vin is. Options + a key power-mgmt axis (software-cuttability):
  - **(a) 3V3 header** → dim (3.3 V under-volt) but **software-cuttable via `enable3V3(false)`** (free LED kill-switch; can't accidentally drain the pack), zero extra parts. Strong budget default (Ben's pick-direction).
  - **(b) VBAT** → brighter (≤4.2 V Li-ion) but **always live** → needs a load-switch/MOSFET + GPIO to be safe.
  - **(c) 5 V boost fed FROM the switchable 3V3** → **full brightness AND still cuttable** (cut 3V3 → boost+LEDs die), +1 boost part.
  - Bench-check: does cutting Vin-3V3 kill pixels while the SeeSaw stays alive on STEMMA 3.3 V (ideal: LEDs off, I2C up)? (Ben).
- [ ] **Re-check NeoHEX-vs-HEX efficiency at the actual ship pixel-voltage** (the 1.6x edge was measured at 3.3 V under-volt; SK6812 handles low V better, so the gap may differ at 5 V) (Ben).
- [x] **DECISION: IS31FL3741 13×9 ruled out for the V2 battery build** (shared-bus brownout). Revisit only if the 13×9 grid form factor is a hard requirement (then try VSYS bulk cap or 2nd-I2C-bus GPIO35/36). **Flag ADR 0018 (IS31 primary module) for update** (Ben/Claude).
- [x] Confirm NeoDriver works powered from 3V3 (dim, under 1 A) on battery — YES, board 1 stable, no brownout; `--brightness` flag added (Ben).
- [x] **NeoHEX (WS2812C-2020) vs HEX (SK6812) efficiency** — DONE: **HEX ~1.6x more PAR/mA** (consistent across brightness), so HEX favored for the power budget. Tooling: `--bright-sweep` fw + `ops/bench/led_efficiency_sweep.py` + Apogee SQ-420 PAR sensor. Still TODO: visual **color/dimming** comparison (PAR can't capture it); optional higher-SNR re-run (sensor closer than 6") (Ben).
- [x] **Test single high-power RGBW LED** (Adafruit 5163, 4 W) — DONE: brightest + most efficient at high brightness, BUT voltage-starved at 3.3 V (needs 5 V boost), poor low-end dimming, single point source. `led-par-vs-draw.png` / `rgbw-undervolt.png` (Ben).
- [x] **LED axis resolved** → distributed dimmable glow = **SK6812 HEX direct-GPIO @3.3V** (no boost); single ultra-bright beacon = **4 W RGBW @5V boost**. IS31 out. Update ADR 0018 to reflect (Ben).
- [x] **Measure LED current vs brightness** — DONE across NeoHEX/HEX/RGBW/warm-white via `--bright-sweep` + Apogee PAR sensor; full efficiency map in `led-par-vs-draw.png`. (Caveat: confounded by buck-boost efficiency vs SOC — see Field reliability.) (Ben).
- [ ] Investigate disabling BQ25628E input source-detection to beat the 500 mA USB charge cap (bench convenience only; solar unaffected) (Ben).
- [x] ~~Build a SOLID LFP connection / re-run on solid connection~~ — **SUPERSEDED**: the brownout turned out IS31-specific (its chip on the shared I2C bus), not the battery connection. The H2-marginal-connection thread is closed (Ben).
- [x] Test LFP full-SOC vs low-SOC under identical load (boost-mode hypothesis H3) — evidence AGAINST H3: boards ran stable in **active boost** at 3.18–3.24 V (the harder regime), so low-LFP/boost is not the brownout cause (Ben).
- [ ] Run ported demo on battery (firmware/powerfeather_demo_port, AP + ~10 Hz) +/- LED; does the reference app reset? (Ben).
- [ ] If resets reproduce on a good connection, add a VSYS bulk cap and re-test (hypothesis H4) (Ben).
- [ ] Exercise ported demo web UI: connect phone to PowerFeather_Demo AP -> 192.168.1.1 (Ben).
- [ ] Test `VSQT` off-state leakage with IS31FL3741 attached (Ben).
- [ ] Test `VSQT` sleep/wake/reinitialize cycle (Ben).
- [ ] Test panel MPP/VINDPM settings for each panel (Ben).
- [ ] Test thermistor / battery-temperature path if accessible (Ben).

## Field reliability concerns (surfaced 2026-06-04 — important for the deployed lantern)

- [ ] **Auto/remote reset is unreliable on the bench USB-JTAG path — harden the FIELD reset paths so a deployed lantern NEVER needs a physical button press** (that would mean taking it down + disassembling = unacceptable). Observed: after a USB flash, the PowerFeather's "Hard reset via RTS pin" sometimes did NOT start the app (no liveness LED) until a *physical* reset or a serial-open nudge (chip verified healthy via esptool; worst on the heavily-abused board 2). Field paths to verify/build: (1) **OTA `/update` uses a software reset (`esp_restart`)** which shouldn't share the JTAG-RTS flakiness — **test end-to-end that it reliably boots the new image**; (2) **add a watchdog** so a hang auto-restarts with no human; (3) the `--autosleep` guard already wakes/recovers on USB-supply (validated) — keep it. Goal: zero field scenarios that require the reset button (Ben).
- [ ] **Buck-boost converter efficiency varies with VBAT — and LFP's plateau sits on the crossover (real budget + chemistry finding).** `battery_mA` ≠ LED current (TPS631013 buck-boost sits between them); efficiency dips in the buck↔boost crossover (~VBAT 3.25–3.35 V) where it 4-switch/mode-hunts. **LFP's flat plateau (~3.2–3.3 V) parks right there for most of the discharge** = a standing efficiency tax on everything; **Li-ion lives mostly in clean buck** (better converter efficiency, the counterpoint to LFP's safety/heat/cycle wins). **Test:** hold one fixed brightness, discharge full→empty, log `ima` vs `VBAT` → maps converter efficiency vs SOC (the real budget input) + confirms the crossover bump; run on LFP and Li-ion to quantify the chemistry tax. NOTE this **confounds the existing PAR/mA efficiency plots** (each LED run was at a different SOC/load → different converter point), so those slopes are *system* efficiency at as-measured conditions, not a clean LED-intrinsic ranking — re-rank at a fixed VBAT (bench supply) or correct with this curve (Ben).
- [ ] **WS2812/SK6812 latch their last frame** — firmware must send an explicit all-off on shutdown/sleep or the LEDs stay lit (and keep drawing) with no data; matters for low-power/shipping modes (Ben).

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
