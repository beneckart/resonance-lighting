# TODO

Active punch list. Status: `[ ]` open, `[~]` in progress, `[x]` done. Owner in parens.

## Immediate documentation / repo hygiene

- [x] Add `LOG_APPEND_2026-05-10.md` entry to `LOG.md` -- **DONE 2026-06-08**: merged 05-10 + 05-11 entries into `LOG.md`; staging files (`LOG_APPEND_*`, `DROP_IN_INSTRUCTIONS.md`) removed (Ben/Claude).
- [x] Add ADR 0015 -- PowerFeather V2 as COTS/reference architecture -- **DONE** (`docs/decisions/0015-powerfeather-v2-cots-reference.md`) (Ben).
- [x] Add ADR 0016 -- COTS prototype shortlist -- **DONE** (`docs/decisions/0016-...`) (Ben).
- [x] Add ADR 0017 -- Battery cell format and sourcing -- **DONE** (`docs/decisions/0017-...`) (Ben).
- [x] Add/rewrite ADR 0018 -- LED module/interface plan. **DONE 2026-06-04**: IS31 ruled out for the V2 battery build; direct-GPIO LED path established (Ben/Claude).
- [x] Add ADR 0022 -- mixed LED fleet by optical role. **DONE 2026-06-17**: HEX for close-range animation / glow; 4 W RGBW point source for long-throw crisp gobo. Placement/type mix remains open below (Ben/Codex).
- [x] Add `docs/research/COTS_SURVEY_2026-05-10.md` -- **DONE** (exists) (Ben).
- [x] Add `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md` -- **DONE** (exists) (Ben).
- [x] Add `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md` -- **DONE** (exists) (Ben).

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
- [x] Repeat low-battery OTA boundary test on the shared-WiFi parallel OTA path, not
  per-device maintenance AP. First maint-AP attempt 2026-06-29: at ~2.57 V the peer
  entered/attempted maint-AP, then brownout-reset and emitted only two heartbeats at
  ~2.33 V before going stale. Second attempt 2026-06-29: at ~2.95 V the peer left
  ESP-NOW after `U`, but no reachable AP/shared-WiFi IP was found and no OTA upload
  occurred; then `9E5AB8` was USB-flashed to known shared-WiFi `net-bench-2026-06-29.3`.
  `net-bench-2026-06-29.5` adds maintenance-entry power preflight telemetry, immediate
  comms resume on OTA-start failure, and watchdog feeds during upload; re-test should
  use shared-WiFi/parallel OTA to measure the real lower-voltage bound. `mt=2` is an
  advisory power warning by default, not a hard block. Follow-up attempt 2026-06-29:
  local `wifi_secrets.h` was stale (`Brandon Springs Activity Guest`) while the laptop
  was on `BubbyNet`, and peer `9E5AF0` exposed `ResonanceMaint-9E5AF0`, proving an old
  maint-AP image was still present. No AP OTA was used. `9E5AB8` rebooted by task
  watchdog during maintenance entry around 3.02-3.03 V loaded, then was USB-flashed on
  COM4 to known-clean `.5` with BubbyNet shared-WiFi secrets and no `ResonanceMaint`
  string. `9E5AF0` was then USB-flashed on COM6 to the same known-clean `.5` BubbyNet
  non-AP image; boot banner confirmed direct `COMMS (ESP-NOW)`, and a WiFi scan showed
  no `ResonanceMaint-*` SSID. Both live peers are now clean for the next true shared-WiFi
  OTA test. **DONE 2026-06-29 official test:** both peers entered shared-WiFi maintenance
  on BubbyNet and accepted parallel `net_bench_ota.py --reboot comms` uploads to
  `net-bench-2026-06-30.1`; `9E5AB8` succeeded from about 3.10 V loaded (INA about
  3.10 V, below advisory floor) and `9E5AF0` from about 3.27 V loaded. Both rejoined
  ESP-NOW with `reset_reason=software`, no button, no AP SSID. Treat ~3.10 V loaded as a
  proven-success lower bound, not a final production cutoff. Soften prior interpretation:
  the earlier 2.95-3.03 V failures were wrong-path/pre-upload failures with stale WiFi
  secrets, AP-contaminated firmware, and/or pre-`.5` watchdog behavior; they do **not**
  prove low VBAT was the root cause (Ben/Codex).
- [x] Add a targeted shared-WiFi maintenance command (`U<id>` or dashboard peer action)
  so a single-peer OTA does not pull every awake peer off ESP-NOW. **DONE 2026-06-30
  in `net-bench-2026-06-30.6`:** the bridge accepts `U9E5AB8`-style sustained targeted
  maintenance, peers handle `NB_TARGET_ENTER_MAINT`, and the dashboard `Peer maint`
  button sends `U<selected-id>`. Bare `U` remains available for deliberate fleet wake
  and for first-hop migration of older peers that cannot parse the targeted packet
  yet (Ben/Codex).
- [x] ~~Build Track A: PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 matrix~~ -- **SUPERSEDED: IS31 ruled out** (shared-bus brownout, 2026-06-04). LED axis -> SK6812 HEX direct-GPIO (Ben).
- [~] Build Track B: PowerFeather V2 + LiFePO4 + solar panel + direct-GPIO LED --
  **the leading path**. LED brownout-safety validated; ADR 0022 selects a mixed
  HEX + 4 W RGBW fleet by optical role. Remaining: solar sizing, type mix/placement,
  enclosure, and production connectorization (Ben).
- [ ] Build Track C: FeatherS2 Neo + DFRobot DFR0559; Feather battery JST left empty (Ben).
- [ ] Build Track D: Atom Matrix + DFRobot DFR0559 (Ben).
- [ ] Run incoming inspection and board-ID procedure from COTS test plan (Ben).
- [ ] Measure sleep current for each stack (Ben).
- [ ] Measure active/radio/ESP-NOW current for each stack (Ben).
- [~] Measure LED current for center-only, 3-pixel, 9-pixel/crop, and full-array capped modes -- LARGELY DONE 2026-06-11 via `/set?n=` + `ops/bench/hex_ramp.py` (INA ground truth, HEX): single px 41.8 mA full, count-ramp safe to n=10 @ full (288 mA) / n=37 @ val 64 (261 mA) on the bench LFP -- the ceiling is battery sag, see LOG 2026-06-11 (Ben).
- [ ] Add SEN0291 I2C wattmeters to power-test harness and bench worksheet when they arrive (Ben).
- [ ] Run iso-current LED brightness/gobo comparison from `docs/tests/ISO_CURRENT_LED_BRIGHTNESS_TEST_2026-05-18.md` (Ben + Steve).
- [ ] Measure solar charge behavior for each 1-5 W panel in sun/shade/heat (Ben). Next
  outdoor run: Voltaic P105/P126 ETFE prep in
  `docs/tests/VOLTAIC_ETFE_PANEL_TEST_PREP_2026-06-15.md`.
- [~] Test low-battery + solar recovery for PowerFeather V2 and fallback stacks.
  **PowerFeather V2 solar-only OTA path validated 2026-06-30:** `9E5AB8` recovered from
  low VBAT on panel-only charge, crossed the watcher threshold at 2.901 V, entered
  shared-WiFi maintenance via one last bare `U` from `.4`, accepted OTA to
  `net-bench-2026-06-30.6`, rebooted/rejoined without a button, and resumed field-cycle
  telemetry. Remaining scope: characterize full day/night cycle behavior and repeat/port
  any needed checks on fallback stacks if they stay in contention (Ben/Codex).
- [x] Flash outdoor solar peer `9E5AB8` to current targeted-control net_bench peer image
  -- **DONE 2026-06-29**: USB-flashed `net-bench-2026-06-29.3`, shared-WiFi maintenance
  path, LFP/6000 mAh/1500 mA, channel 11, 1 Hz heartbeat (Ben/Codex).
- [x] Flash the USB bridge/master to current net_bench after it is plugged back in
  -- **DONE 2026-06-29**: first reflashed COM7 as `.3` serial bridge
  (`NB_SERIAL_BRIDGE=1`, channel 11, 1 Hz default frame rate) and verified selected-peer
  `G9E5AB8:1500`; later reflashed COM7 to `.4` with the solar guard, watchdog-safe
  WiFi-join loops, 96-byte ESP-NOW receive buffer, and bridge/peer firmware-rev dashboard
  telemetry (Ben/Codex).
- [ ] Compare outdoor recovery at `R1` versus targeted `P9E5AB8:3600` solar naps now
  that the outdoor peer has targeted nap support (Ben).
- [ ] Test standard OTA maintenance mode on at least two COTS boards (Ben). Current `.3`
  peers exposed a maintenance-entry watchdog problem before any upload: the 20 s shared-WiFi
  join loop can trip the 8 s task watchdog. Fixed in `.4`; `.5` also feeds the watchdog
  during upload and reports low-power maintenance-entry warnings before dropping ESP-NOW.
  At least one peer still needs USB flash or a successful quick-join OTA before fleet OTA
  can be considered healthy.
- [~] Test ESP-NOW heartbeat/state packets with jitter/sequence numbers -- prototyped + bench-validated on 1 board in `firmware/net_bench/` (broadcast heartbeat w/ seq + jitter, per-source seq-gap PDR). Multi-node matrix pending (see Networking feasibility below) (Ben).
- [ ] Test LED fail-safe: stuck LEDs, MCU hang, watchdog reset, rail-off recovery (Ben).
- [ ] Test remaining production-relevant gobo/filter variants with HEX and 4 W RGBW
  point-source modules; keep older IS31/NeoHEX/FeatherS2/Atom data as historical
  fallback context only (Ben + Steve).
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

## PowerFeather power-bench (2026-06-02, board 9E5AB8 -- see docs/tests/POWER_BENCH_HARNESS_2026-06-02.md)

- [x] Stand up Arduino power-bench firmware `firmware/power_bench/` with SDK 2.1.0 telemetry + `/telemetry` JSON (Ben).
- [x] Confirm V2 hardware via Wire1 scan: MAX17260 (0x36), BQ25628E (0x6A), IS31 (0x30) (Ben).
- [x] `Board.init(4400, Generic_3V7)` Ok; battery/supply voltage + current read correctly over WiFi (Ben).
- [x] Wire up IS31FL3741 13x9 on the STEMMA-QT bus (Wire1, GPIO47/48) shared with the SDK (Ben).
- [x] Host logger `ops/bench/power_logger.py` + `power_summary.py` + site-partitioned JSONL data layout (Ben).
- [x] Resolve MAX17260 SOC/health/cycles `InvalidState` -- root cause was the missing `-DPOWERFEATHER_BOARD_V2=1` compile flag (SDK fell back to V1 LC709204F gauge); now in build.sh + #error guard. SOC/health/cycles/time_left populate (Ben).
- [ ] Verify BQ25628E charger telemetry: state, faults, input regulation, charge current (Ben).
- [x] Add NeoHEX + single-RGBW LED build variants on bench -- DONE: `--led neohex/rgbw1/neodriver`, `--pixel-pin` to drive WS2812/SK6812 direct on any free GPIO (used A0/GPIO10) (Ben).
- [~] Solar harvest sweep across panels/conditions; set `RES_PF_MAINTAIN_V` to panel MPP -- STARTED 2026-06-08: path validated on the Seeed 3W panel + LFP (net-positive ~10 mA charge even in partly-cloudy-through-glass @ 0.37W, VINDPM steady at 5.5V). Remaining (do on USB so reflash is safe): full-sun board-asleep harvest number + **`--maintain` sweep (5.5/5.0/4.6) for the shaded canopy** (lower VINDPM may harvest more when the panel sags) (Ben).
- [x] **Outdoor solar telemetry over ESP-NOW + WiFi range diagnostic -- BOTH DONE 2026-06-08** (LOG cont. 7/8/9): plan `docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md`. Next: a **sizing-oriented** solar run (below).
  - [x] **(b) WiFi 2.4GHz coverage diagnostic -- DONE + hypothesis SETTLED 2026-06-08** (LOG cont. 7+9). Wireless ESP-NOW bridge (no laptop tether), doubling as (a)'s infrastructure: net_bench `--serial-bridge` + `--scan-report` (`NB_SCANAP`), host `net_bench_serial_bridge.py` + `net_bench_log.py` `nb-scanap` rows; `firmware/wifi_diag/` = the complementary tethered probe (unused -- scan route sufficed). **Conclusion (high confidence):** the ESP32 latches one Eero BSSID and doesn't auto-roam; carried outdoors it clings to the weak indoor node while a -46 dBm nearer one sits available -> drop. **It's a moving-board artifact; deployed fixtures are stationary -> low field risk.** Logged as a gotcha (POWERFEATHER_NOTES) + firmware-guard TODO below. A formal yard-walk coverage-at-distance map is **optional/deferred** (the question is answered) (Ben).
  - [x] **(a) Solar telemetry over ESP-NOW -- BUILT + VALIDATED on hardware 2026-06-08** (LOG cont. 8). Heartbeat carries `supply_mv`/`supply_ma`/`supply_good` (append-only, `NB_PROTO_VER` unchanged) -> `nb-peer` `sv=/sma=/sgood=` -> `net_bench_log.py` derives `supply_w`/`battery_w`/`load_w` into JSONL. Validated: panel V/I logs over ESP-NOW with no WiFi-STA; revealed a dark-panel (`supply_v=0.123`, traced to a reseated connector) cause for the earlier net-discharge. **Note:** the bench `load_w` is the diagnostic firmware's draw, NOT a fixture budget -- still need the bottom-up nightly budget (see Field reliability TODO) before sizing the cell/panel (Ben).
  - [~] **Sizing-oriented solar campaign** -- in progress 2026-06-08 (LOG cont. 10). Spec cell (LFP mAh) + panel (W) from harvest (Wh/day at MPP) vs load (sleep + LED show). Progress + open items:
    - [x] **Supply telemetry + idle floor** -- always-on ESP-NOW peer = **~168 mA/0.55 W** (radio-RX-dominated; scanning negligible) -> unsustainable on battery, **deep-sleep mandatory**.
    - [x] **Sleep-cycle firmware** (`--sleep-cycle`) + **`U` fleet wake-for-maintenance** (no-touch OTA of a sleeping board) -- both validated on hw.
    - [x] **`SET_MAINTAIN`** runtime VINDPM (MPP-sweep + P&O-MPPT actuator).
    - [x] **LFP drawdown -- ABORTED as redundant** (LOG cont. 11): the 2026-06-03 reboot-loop drain already has the LFP curve (mean -145 mA, SOC 92->30 %, flat ~3.25 V). **Capacity: UNCONFIRMED** -- the 06-03 drain delivered >=617 mAh but stopped *mid-plateau* (not empty), on an un-learned gauge, so SOC-derived capacity is unreliable on LFP's flat curve. Likely a normal ~1-1.5 Ah 18650 LFP; the earlier "~1000 mAh / overrated 2x" was too strong (walked back). Needs a clean full->empty coulomb run + learned gauge / the INA meter (Ben).
    - [ ] **Clean full->empty capacity run** (USB top-up to full first, coulomb-count) to confirm the real LFP capacity (~1000 mAh?) -- gating for battery sizing (Ben).
    - [x] **Sleep-cycle idle drain -- rails were the culprit** (LOG 2026-06-09). Cutting both 3V3 rails (`enable3V3(false)+enableVSQT(false)`) in deep sleep dropped idle ~1.7->0.5 %/h (~3-4x, ~20->5 %/night); INA ground-truth in the battery lead shows the rails-OFF duty-cycled drain is **sub-mA** (the gauge over-read it on the flat LFP plateau). **Idle is negligible (ground-truth). Sizing is LED-show- + harvest-bound.** Gotcha captured in POWERFEATHER_NOTES (Ben).
    - [x] **External ammeter -- BUILT: 4-channel INA219 monitor** (`firmware/ina_monitor/`, Adafruit Metro S3 + 4x SEN0291 @ 0x40/41/44/45, separate-monitor topology). Reads a board-under-test's current through deep sleep, gauge-independent. **Next:** (a) **fast-sample capture** (raise rate) to nail per-wake energy -- 10 Hz may miss a <100 ms radio-init spike; (b) **calibrate R_shunt** (0.1 ohm provisional; raw shunt_mV logged so recoverable); (c) wire **panel-lead + LED-rail** channels for the full power-flow map; (d) sharpen sleep resolution (drop PGA range) (Ben).
    - [~] **Clean full-sun MPP sweep -- HOT SESSION DONE 2026-06-11** (LOG 06-11 cont. 2; data `2026-06-11-mpp-sweep-hot-pm-*.jsonl` + knee re-sweep): hot panel (~60 deg C back / 68 deg C front IR) optimum **4.6-4.7 V -> 1.73 W BQ-side / 1.91 W panel-INA ground truth = 3.2x the 5.5 V default (0.59 W)**; instability/knee immediately below 4.6 (one real collapse at 4.4 when stepped to from near-idle); **BQ supply telemetry under-reports harvest ~10 % vs the panel-lead INA** (sizing must use panel-side). Fully wireless: TSL2591 (saturated in full sun -> ir-ch1 normalization fallback, works well; diffuser optional), SHT31 panel-back temp, onboard SEN0291s (0x40 panel / 0x45 battery) all in the heartbeat (fw 2026-06-11.2). **Remaining: the cool-AM session** for Vmp(T) -> the fixed-vs-temp-comp-vs-P&O decision. Lessons baked in: anchor at 4.9 not 5.5 (5.5 is load-noise-dominated); run on a hungry battery (<~50 % SOC -- late-session demand-limiting flattens the curve); approach setpoints from above; beware the bright-sun input-latch on connect (see Firmware guard TODO) (Ben).
    - [x] **Voltaic ETFE P105/P126 outdoor MPP comparison** (2026-06-29, Oakland late sun, both panels about 15 deg tilted): P105 5 W best observed around `m46`/`m48`, panel-side INA about 3.8-3.9 W and charger input about 3.47 W; P126 smaller ETFE best around `m58`, panel-side INA about 1.89 W and charger input about 1.66-1.68 W. P126 is proportionally close to nominal/nameplate; P105 is plausible vs datasheet expected Vmp but may be demand-limited by LFP charge acceptance/taper. See LOG 2026-06-29. (Ben/Codex)
    - [ ] **Re-run P105 5 W with a hungry larger LFP**: use the 6-7.2 Ah cell intentionally discharged to roughly the mid-SOC voltage region (about 3.25-3.40 V resting, not 3.55+ V while charging), hold around `m46`/`m48`, and confirm whether panel-side power can climb beyond the 3.8-3.9 W seen with the 2 Ah cell. Goal: separate panel capability from cell IR/CV-taper demand limiting. (Ben)
    - [ ] **Analyze 7200 mAh HEX drawdown run before the next P105 test**: data path
      `ops/bench/data/ca/2026-06-29-ca-lfp-7200-hex-drawdown-9E5AF0.jsonl`; record stop
      condition, delivered mAh, final loaded/resting voltage, and whether the 12 h sleep
      preserved the desired hungry-but-not-precharge state. (Ben/Codex)
    - [ ] **Prototype simple production MPPT / hill-climb policy**: full sweep on first good-sun boot, then periodic 3-point perturb around the last best VINDPM (`best - 0.2`, `best`, `best + 0.2 V`) during daylight; skip or de-prioritize when battery voltage/current indicate CV/taper or near-full acceptance. Validate energy gained vs wake/sweep cost on P105 and P126. (Ben/Codex)
    - [ ] **MPPT decision** -- green-lit to *measure*, not yet to commit. After the clean sweep, choose: better fixed setpoint (~4.8-5.0) / temp-compensated Vmp(T) / software P&O (use `SET_MAINTAIN` to hill-climb `supply_W`). Optimum ~ 4.85 V hot vs 5.5 V cool -> a single fixed point can't be optimal across temp (Ben).
    - [ ] Full **0-100 % capacity** drawdown (USB top-up to full first) + buck-boost efficiency vs VBAT on LFP (needs rail-side metering for the latter -- SEN0291) (Ben).
    - [ ] Combine harvest-at-MPP (Wh/day) + load budget + a chosen LED-show profile -> the cell + panel spec; pair with the bottom-up nightly-budget re-derivation (Field reliability TODO) (Ben).
- [ ] **Firmware guard: don't enable charging if no battery detected** -- enabling charging into a missing battery (with `maintain` > supply V) browns out / crash-loops on USB. Also: `maintain` must be <= the supply you're powering from (Ben).
- [ ] **Firmware guard: make charger VINDPM/maintain USB-recovery-safe by construction**:
  keep boot default at ~4.6 V, treat higher panel-MPP setpoints as live/test state unless
  a persisted value can be clamped against observed supply voltage. A live `m<v10>`
  command can lower VINDPM only while the peer is still awake/listening; it cannot recover
  a board that already brownout-reset or left ESP-NOW for maint-AP (Ben).
- [x] **Firmware baseline: VBUS_OVP=1 + HIZ-toggle re-qualification kick (bright-sun connect latch) -- IMPLEMENTED 2026-06-29.** Shared helper `firmware/powerfeather_solar_guard.h` now forces BQ25628E `REG0x17[0] VBUS_OVP=1` at charger init and watches for the stuck signature (`supply_v` near panel Voc, `supply_good=false`, near-zero input current) before toggling `EN_HIZ` to synthesize a fresh input-qualification edge. Wired into `net_bench`, `power_bench`, and `led_studio` -- the Resonance sketches that enable PowerFeather charging. Root cause remains the 2026-06-12 datasheet read: low OVP is 6.1/6.4/6.7 V rising; wide OVP is 18.2/18.5/18.8 V; qualification is edge-triggered. **Still validate in bright-sun hardware:** deliberately reproduce the stuck state and confirm the guard clears it without a physical unplug before any panel buy with Voc > 6 V (Ben).
- [x] Clean LED-current runs -- DONE via `--bright-sweep` on battery + `--wifi-lowpower` (steadies the WiFi baseline so small LED currents resolve); the gauge `ima` is the metric, charging masks it so runs are on battery (Ben).
- [ ] Steve mirrors the bench in TN; merge JSONL via the repo (Steve).
- [ ] Add live telemetry readout to `ops/bench/cots-mode-dashboard.html` (Ben).
- [x] Configure LiFePO4 profile on V2 -- DONE: `--chem lfp` (`RES_PF_BATTERY_TYPE=Generic_LFP`) used throughout; LFP favored for safety/heat/cycles (counterpoint = buck-boost crossover tax, see Field reliability) (Ben).

## Battery-brownout investigation (see docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md -- LARGELY RESOLVED 2026-06-04: cause = IS31FL3741 chip on the V2's shared charger/gauge I2C bus + WiFi; fix = don't put LEDs on that bus -> direct-GPIO WS2812/SK6812. Remaining items are follow-ups, not blockers.)

- [x] Characterize exact conditions for VSYS power-on reset on battery -- ~~load-stacking~~ ~~not-reproducible~~ **UPDATED 2026-06-04: brownout CAME BACK** -- board 1 did a 794-reboot loop overnight on battery (poweron, healthy bv 3.24-3.46, all SOC, lightest load, dying at WiFi association). Real + intermittent on board 1 => H2 (marginal connection) strengthened. See doc Status (Ben).
- [x] Repeat brownout characterization on a second board + known-good cell -- DONE (n=3 all stable in short runs); **NOW extending: pristine board 2 multi-hour with fixed guard to see if it loops like board 1 overnight (board-specificity)** (Ben).
- [x] Fix the overnight auto-sleep guard -- RAM coulomb/timer state reset every reboot, so a brownout loop defeated it. Added **NVS-persisted reboot-loop breaker** (`--autosleep`, >=25 sub-survival boots => deep sleep before WiFi); fw power-bench-2026-06-04.1 (Ben).
- [ ] **Inspect/reflow board 1's battery + VDC solder joints** under magnification (cold joint / flux / hairline bridge); re-run overnight to confirm the loop is gone -- the direct H2 test now that it reproduces (Ben).
- [x] **Verify the reboot-loop breaker actually fires** -- DONE: board 2 looped and the breaker deep-slept it (validated in the wild) (Ben).
- [x] **FIX loop-breaker brick-risk** -- DONE (fw 2026-06-04.2): (1) **never deep-sleep while external supply is present** (USB/VDC -> stay flashable/recoverable -- the root cause of the stranding); (2) sleep with a **15-min timer wake** (not indefinite); (3) on a timer wake still on battery -> re-sleep, on supply -> run/charge. So plugging USB self-recovers within one wake interval; never bricks. **VALIDATED LIVE 2026-06-04** (3 mAh budget / 60 s wake test): ran on USB w/o sleeping -> coulomb-budget sleep on battery -> 124 s of timer-wake/re-sleep -> recovered (charging, fresh boot) on USB plug, no BOOT+RESET needed. Tuning flags added: `--budget-mah`, `--wake-s` (Ben/Claude).
- [ ] Keep a **VSYS bulk cap as cheap insurance** and bench-characterize it opportunistically -- demoted from "key fix" to "nice to have" now that 3 boards are stable without it (Ben).
- [ ] Watch for brownout **recurrence in the field**; if it returns, capture which connection/board and whether re-seating clears it (Ben).
- [x] Distinguish IS31-specific vs any-I2C-device: **NeoDriver (5766, SeeSaw I2C) on the same bus = STABLE** (371 s+, through heavy WiFi) where the IS31 loops in ~1 min => **brownout is IS31-SPECIFIC**, not the bus. NeoDriver+WS2812 is a viable no-solder LED path (Ben).
- [ ] **Confirm NeoDriver robustness with an hours/overnight run** (the IS31 was intermittent -- stable for minutes then failed; n=1/6min not enough). Do AFTER the wake-source fix (Ben).
- [x] **Direct-GPIO WS2812/SK6812 validated** (board 2, HEX on A0/GPIO10, off the I2C bus) -- works, brownout-safe by construction, and ~10% MORE efficient than via NeoDriver (no passthrough drop). **Strong candidate for the area/glow role -- but NOT a settled BOM front-runner**: it's roughly tied in viability with the 4 W RGBW point-source, which serves the crisp-gobo role HEX can't, and the efficiency edge is muddied by varying-SOC testbeds. Decide after gobo testing. 3-way plot `led-eff-3way.png` (Ben).
- [ ] **LED bring-up sequencing for production:** WS2812 latch last frame (send explicit all-off to blank); avoid full-white inrush on hot-connect (ramp gently); direct-GPIO's full VCC browns a marginal cell sooner -> cap brightness / healthy pack. **Now quantified (2026-06-11 hex_ramp):** ~350-400 mA LED draw pulls the bench cell to its ~3.0 V brownout edge even at 98% SOC -> production firmware needs a hard current cap = f(brightness x lit-count), scaled to the production cell's IR (the 32700 ~6 Ah cell lifts the ceiling substantially) (Ben).
- [ ] **Decide pixel-power architecture (NeoDriver only level-shifts the DATA signal, does NOT boost pixel power -- corrected; "3-5V vin/vlogic" = accepted *input* range):** pixels run at whatever Vin is. Options + a key power-mgmt axis (software-cuttability):
  - **(a) 3V3 header** -> dim (3.3 V under-volt) but **software-cuttable via `enable3V3(false)`** (free LED kill-switch; can't accidentally drain the pack), zero extra parts. Strong budget default (Ben's pick-direction).
  - **(b) VBAT** -> brighter (<=4.2 V Li-ion) but **always live** -> needs a load-switch/MOSFET + GPIO to be safe.
  - **(c) 5 V boost fed FROM the switchable 3V3** -> **full brightness AND still cuttable** (cut 3V3 -> boost+LEDs die), +1 boost part.
  - Bench-check: does cutting Vin-3V3 kill pixels while the SeeSaw stays alive on STEMMA 3.3 V (ideal: LEDs off, I2C up)? (Ben).
- [ ] **Re-check NeoHEX-vs-HEX efficiency at the actual ship pixel-voltage** (the 1.6x edge was measured at 3.3 V under-volt; SK6812 handles low V better, so the gap may differ at 5 V) (Ben).
- [x] **DECISION: IS31FL3741 13x9 ruled out for the V2 battery build** (shared-bus brownout). Revisit only if the 13x9 grid form factor is a hard requirement (then try VSYS bulk cap or 2nd-I2C-bus GPIO35/36). ~~**Flag ADR 0018 (IS31 primary module) for update**~~ -- DONE, ADR 0018 rewritten 2026-06-04 (Ben/Claude).
- [x] Confirm NeoDriver works powered from 3V3 (dim, under 1 A) on battery -- YES, board 1 stable, no brownout; `--brightness` flag added (Ben).
- [x] **NeoHEX (WS2812C-2020) vs HEX (SK6812) efficiency** -- DONE: **HEX ~1.6x more PAR/mA** (consistent across brightness), so HEX favored for the power budget. Tooling: `--bright-sweep` fw + `ops/bench/led_efficiency_sweep.py` + Apogee SQ-420 PAR sensor. Still TODO: visual **color/dimming** comparison (PAR can't capture it); optional higher-SNR re-run (sensor closer than 6") (Ben).
- [x] **Test single high-power RGBW LED** (Adafruit 5163, 4 W) -- DONE (first pass): brightest + most efficient at high brightness; single point source (-> crisp gobo). At 3.3 V the current curve goes non-monotonic in the mid-range (operating near its Vf) -- but **undervolting is viable (5 V NOT strictly required)** per Ben's prior experience; this run just didn't cleanly characterize its 3.3 V limits. **Open: map RGBW low-voltage behavior properly** (dimming range, color balance, max usable brightness at 3.3 V vs a small boost). `led-par-vs-draw.png` / `rgbw-undervolt.png` (Ben).
- [x] **LED axis decision recorded** -- ADR 0022 accepts a mixed fleet by role: SK6812
  HEX direct-GPIO for close-range animation / glow, and 4 W RGBW point source for
  long-throw crisp gobo. Open work moves to type mix, placement, power budget, and
  boost/current-cap characterization (Ben/Codex, 2026-06-17).
- [x] **Measure LED current vs brightness** -- DONE across NeoHEX/HEX/RGBW/warm-white via `--bright-sweep` + Apogee PAR sensor; full efficiency map in `led-par-vs-draw.png`. (Caveat: confounded by buck-boost efficiency vs SOC -- see Field reliability.) (Ben).
- [ ] Investigate disabling BQ25628E input source-detection to beat the 500 mA USB charge cap (bench convenience only; solar unaffected) (Ben).
- [x] ~~Build a SOLID LFP connection / re-run on solid connection~~ -- **SUPERSEDED**: the brownout turned out IS31-specific (its chip on the shared I2C bus), not the battery connection. The H2-marginal-connection thread is closed (Ben).
- [x] Test LFP full-SOC vs low-SOC under identical load (boost-mode hypothesis H3) -- evidence AGAINST H3: boards ran stable in **active boost** at 3.18-3.24 V (the harder regime), so low-LFP/boost is not the brownout cause (Ben).
- [ ] Run ported demo on battery (firmware/powerfeather_demo_port, AP + ~10 Hz) +/- LED; does the reference app reset? (Ben).
- [ ] If resets reproduce on a good connection, add a VSYS bulk cap and re-test (hypothesis H4) (Ben).
- [ ] Exercise ported demo web UI: connect phone to PowerFeather_Demo AP -> 192.168.1.1 (Ben).
- [ ] Test `VSQT` off-state leakage with IS31FL3741 attached (Ben).
- [ ] Test `VSQT` sleep/wake/reinitialize cycle (Ben).
- [ ] Test panel MPP/VINDPM settings for each panel (Ben).
- [ ] Test thermistor / battery-temperature path if accessible (Ben).

## Gobo / aesthetic LED testing (HEX Studio app -- `firmware/hex_studio/`, 2026-06-04)

- [x] Build interactive web app to dial in HEX looks (brightness/RGB sliders, shape rings, spiral/orbit/breathe/twinkle, Split-RGB fringing, Freeze+Step, settings readback) -- DONE, validated on hardware (PowerFeather ACM1, HEX pin 10); served at the IP from the boot banner (Ben/Claude).
- [x] Build interactive web app for the **4 W RGBW point source** (`firmware/rgbw_studio/`): R/G/B/W sliders, white/warmth presets + crossfade, hue/breathe/candle/fade animations, settings readback -- DONE, validated on hardware (ACM1, pin 10) (Ben/Claude).
- [x] Run the gobo session on the inverted-lantern rig -- **DONE 2026-06-11, VERDICT: BOTH module types, by role** (LOG 2026-06-12): HEX = animations/Split color-separation, best within ~6 ft (washes out at 10-15 ft); 4 W RGBW = crisp at 15 ft, Venn-diagram color fringing (overlaps mix NEW colors). Fleet = lanterns of both types. HEX looks best at 1-3 px (white or single-channel) -> realistic HEX gobo draw ~0.4-0.6 W battery-side = all-night budget (Ben).
- [x] **Record LED module = BOTH, by role** -- DONE 2026-06-17 via ADR 0022
  (`docs/decisions/0022-mixed-led-fleet-by-role.md`), preserving ADR 0018's IS31
  rejection and direct-GPIO constraint (Ben/Codex).
- [ ] Decide HEX/RGBW **type mix and placement**: which tree heights/positions get each
  module type, and whether panel size also splits by role (Ben + Steve + team).
- [ ] **Capture per-look settings**: when a look is a keeper, record the led_studio sliders + the UI Battery line voltage (brightness is SOC-dependent until the 4.2 V boost lands) (Ben + Steve).
- [ ] Compare Steve's **3 flat sample filters** through the rig; note which pattern reads best at the install throw (Ben).
- [ ] Capture ceiling photos per source/filter for the record; fold results into a gobo test write-up (Ben).
- [ ] If the swept-pixel / orbit moving-shadow looks good, decide whether it argues for a small multi-pixel array even in the "point source" role (Ben).
- [x] Re-test the **4 W RGBW** point source the same way -- DONE 2026-06-11 (led_studio dual-board session): point-vs-area settled as complementary, see verdict above (Ben).

## Presence sensing / interactivity bench (research note: docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md) -- Elliot ask, 2026-06-12

- [ ] Order the sensor bench kit (~$10): **VL53L1X ToF** (I2C, primary), **LD2420/LD2410
  mmWave** (UART, through-enclosure candidate), **LIS3DH/MPU6050 IMU** (sway-veto +
  wind-response + structure-touch) (Ben or Steve).
- [ ] Add the sensors to the net_bench heartbeat (append-only tail, same pattern as
  env/INA) so detection tuning happens wirelessly from the desk (Claude + whoever's bench).
- [ ] **ToF eye test**: downward VL53L1X at 2.5-3.5 m hang height -- detection vs
  false-positive rate with person under/standing/leaving vs sway (fan/manual swing);
  ground-baseline temporal filter; dirty-cover-glass crosstalk calibration check (Steve-runnable).
- [ ] **Radar test**: LD2420 duty-cycled power draw (continuous ~80 mA = LED-show-class,
  unacceptable); through-enclosure detection from inside a mock hat/lantern body; self-sway
  false positives +- IMU veto; 2-unit mutual-interference check (Steve-runnable).
- [ ] **Mesh-RSSI presence experiment (FREE)**: 3-5 net_bench nodes in the yard, someone
  walks figure-eights -- do per-link RSSI step-changes localize people? Existing JSONL
  tooling; zero new hardware (Ben or Steve).
- [ ] **Enclosure**: downward eye port placement vs the gobo aperture (they share the
  lantern bottom) -- small recessed window beside the gobo margin? (Steve).
- [ ] Sketch the PRESENCE event packet + a first ripple choreography (master-relayed or
  peer-flooded with hop-count) -- the mesh choreography IS the product; sensor is the seed
  (Ben + Claude).
- [ ] Report findings to Elliot (interactivity = his ask; BOM impact ~$3-6/lantern) (Ben).

## HEX 4.2 V boost bench test -- TPS63802 (STEVE-RUNNABLE while Ben travels, 2026-06-12)

Context (LOG 2026-06-12 cont.): at our sagged ~2.8-2.95 V LED rail the SK6812's blue/green
channels run in dropout (starved -> the "goldening"); a regulated 4.2 V V+ should recover
**~40-60 % more white lumens**, fix color balance, and make looks **SOC-invariant**
(same sliders = same light on every lantern at any charge). 4.2 V (NOT 5 V!) keeps the
WS-data threshold in spec for 3.3 V GPIO (VIH = 0.7 x VDD = 2.94 V; at 5 V it's 3.5 = broken).
Steve has duplicate components; firmware/tools all in-repo (led_studio has battery stats +
OTA; afk/PAR harness in ops/bench; site code for Steve's data = `tn`).

- [ ] Source a **TPS63802 buck-boost module with output-select solder jumpers (3V3/4V2/5V)**
  (Amazon; MT3608-class adjustable boost = acceptable fallback, set 4.2 V by pot + meter).
  Presoldered headers nice-to-have (Steve).
- [ ] Re-jumper output 3V3 -> 4V2: fully OPEN the 3V3 bridge first (both bridged = wrong
  feedback divider), close 4V2, **meter the output UNLOADED before any pixel sees it**.
  Leave the tiny EN pad untouched (tied to VIN; kill-switch = the GPIO4 3V3 rail). Leave
  the PS pad at default (power-save/PFM = light-load efficiency; flip ONLY if dim
  single-pixel flicker appears) (Steve).
- [ ] Wire: PowerFeather switchable 3V3 header -> module IN; module OUT 4.2 V -> HEX V+;
  common GND; HEX data direct to GPIO10 as usual. led_studio drives it unchanged (Steve).
- [ ] **Measure (the decision data): lumens-per-system-watt at rail-direct ~2.9 V vs
  boosted 4.2 V** -- PAR sensor (apogee_par.py) + INA harness (led_ina_sweep.py /
  hex_ramp.py), 1 px and 3 px, white + per-channel (R, G, B singly). Expectations to
  confirm/refute: white +40-60 %, blue/green large gains, red ~unchanged, goldening gone.
  Repeat at two battery SOCs to confirm brightness is now SOC-invariant. JSONL ->
  ops/bench/data/tn/ + commit (Steve).
- [ ] **Firmware count-cap for boosted builds**: all-37 full white at a regulated 4.2 V
  wants ~2 A out (~8 W in) = instant brownout -- cap n (or estimated total current) in
  led_studio when V+ is boosted. Trivial guard; needed before anyone slides "all" to max
  on a boosted rig (Steve or Ben).
- [ ] If validated: spec the **production variant on the NeoHEX adapter PCB rev** --
  boost fed from VBAT with EN routed to a GPIO + pull-down (single conversion ~92 % vs
  ~83 % two-stage, keeps software kill, frees the 3V3 rail for the ESP). The Amazon
  module proves the concept; the adapter PCB ships it (Ben).

## Networking feasibility -- 5x PowerFeather V2 (net_bench, 2026-06-07; de-risk the 100-buy)

See `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` + `firmware/net_bench/`.

- [x] Build the first ESP-NOW firmware + 5-node host harness (broadcast comms, master/peer roles, maintenance-mode WiFi OTA, watchdog, per-source PDR, scale-extrapolation summarizer). Bench-validated on 1 board (Ben/Claude).
- [ ] **Flash all 5 boards with `--channel <AP channel>`** (home AP "BubbyNet" = ch 11) and run T0-T7 -- channel MUST match the AP or ESP-NOW silently fails (Ben).
- [ ] Run the **rate sweep** (1/2/5/10/20/50 Hz) on master-multicast + peer-mesh -> find the PDR loss knee -> set the production heartbeat rate below it; extrapolate to 100 (Ben).
- [ ] **Range** sweep (open-field cliff) + **through-obstruction** (body/bamboo/foil/battery-behind-antenna) RSSI+PDR (Ben).
- [ ] **Parallel OTA cycle** on 5 nodes via `net_bench_ota.py` -- confirm 5/5 auto-recover with NO physical button (the field-reset requirement) (Ben).
- [ ] **Multi-hour battery stability** soak (Li-ion) -- zero unexplained resets, log mAh/h drain (Ben).
- [ ] **Master WiFi+ESP-NOW coexistence** current/stability run (Ben).
- [ ] **RE-VERIFY all battery/stability findings on LFP** once Steve's cell holders/connectors exist -- Li-ion is necessary-not-sufficient (LFP plateau = buck-boost crossover) (Ben).
- [ ] 20+ node confirmation run if the rate knee lands near the production point (Ben).
- [ ] Mock-hat antenna RF with panel/battery installed (Steve; COTS Phase 7).
- [ ] **Lengthen the identify/locate blink** -- 8 s is too short for human-in-the-loop / field use (missed a single blink due to chat latency; live sweeps work but one-at-a-time doesn't). Make it ~30 s, or toggle-until-stop, and add identify-by-specific-ID (Ben).
- [x] **Make battery capacity runtime-settable (NVS), not a build flag** -- DONE 2026-06-28 in `net_bench`: master serial `C<mah>` stores capacity in peer NVS and reboots to apply the gauge model; `G<mA>` stores/applies the charge-current cap live. Build flags remain defaults, not per-board destiny.
- [ ] **Validate fuel-gauge SOC over a real charge/discharge cycle** + confirm the field (sleep + low-load solar charge) anchors the gauge cleanly, unlike the always-pinging bench (the false-low was likely a bench artifact). Production low-battery logic must cross-check voltage (done for the LED) (Ben).
- [x] Promote results into **ADR 0021** -- DONE 2026-06-08: `docs/decisions/0021-powerfeather-v2-feasibility-validated.md` (go; networking + solar + field-OTA validated, open follow-ups listed) (Ben).

## Field reliability concerns (surfaced 2026-06-04 -- important for the deployed lantern)

- [ ] **Auto/remote reset is unreliable on the bench USB-JTAG path -- harden the FIELD reset paths so a deployed lantern NEVER needs a physical button press** (that would mean taking it down + disassembling = unacceptable). Observed: after a USB flash, the PowerFeather's "Hard reset via RTS pin" sometimes did NOT start the app (no liveness LED) until a *physical* reset or a serial-open nudge (chip verified healthy via esptool; worst on the heavily-abused board 2). Field paths: (1) **OTA `/update` software reset (`esp_restart`)** -- **VALIDATED 2026-06-08: ~17/17 battery-only OTAs recovered, no button, incl. 3/3 on LFP at the ~3.2 V buck-boost crossover; new image confirmed running, `rr=software` every time.** The JTAG-RTS flakiness is USB-flash-only, not OTA; (2) **watchdog** -- DONE + validated in `net_bench` (port to production); (3) `--autosleep` USB-supply recovery -- validated. **Remaining OTA-robustness (refinements, not blockers -- a failed OTA is safe: stays on / A-B rolls back, never bricks):** (a) OTA over a **marginal/lossy WiFi link** (field maintenance assumes a decent local AP). (b) **A/B rollback -- VALIDATED 2026-06-08:** a self-test-failing image (`extern "C" verifyOta()`->false) auto-reverts to the last-good image, no touch, battery-only. Gotcha: the hook is C-linkage (needs `extern "C"`, else it silently doesn't override and the bad image sticks). Goal met for the happy path: zero field scenarios needing the reset button (Ben).
- [ ] **Implement the production rollback/health pattern in the real firmware**: `extern "C" bool verifyOta()` self-test (power chip + radio + fuel-gauge reachable); PLUS `verifyRollbackLater()=true` to defer the mark-valid + extended self-test + watchdog so an image that PASSES verifyOta but crashes/hangs LATER still rolls back (otherwise it's marked valid and could brick). power_bench has the gated `RES_OTA_FAIL_SELFTEST` test fixture to verify it (Ben).
- [ ] **Re-derive the nightly power budget bottom-up** -- SYSTEM.md's ~120 mAh/night assumes ~5 mA time-avg LEDs, but measured HEX/RGBW draw is 400-500 mA at full; real budget = brightness x LED count x duty cycle (could be 5-20x the floor). Compute from measured LED draw + a realistic show duty cycle, then size battery (LFP capacity) + panel (W) to it. The COTS tests show PowerFeather has the headroom; this just sets the cell/panel spec (Ben).
- [~] **Buck-boost converter efficiency varies with VBAT -- and LFP's plateau sits on the crossover (real budget + chemistry finding).** **PARTIAL ANSWER 2026-06-10** (`ops/bench/bb_efficiency.py` on the full-discharge JSONL -- no new bench time): at full-RGBW show load the LFP *terminal* voltage sags to ~2.9-3.05 V, so the converter ran in **boost the entire pre-brownout discharge -- the 3.25-3.35 V crossover band was never visited under load**. Overhead (ESP+WiFi+converter, not separable) ~0.48-0.52 W and roughly flat; P_led/P_batt lower bound 0.61-0.64. So the crossover-tax concern does NOT apply at show loads; **the residual open regime is the production light/ambient load (tens of mA), where the plateau terminal V ~3.2-3.3 V DOES sit near the crossover** -- needs a light-load fixed-brightness discharge (or rail-side metering for absolute eta). Caveats: n=1 cell/board/load; apparent fine structure vs VBAT may be time-confounded (WiFi activity). `battery_mA` != LED current (TPS631013 buck-boost sits between them); efficiency dips in the buck<->boost crossover (~VBAT 3.25-3.35 V) where it 4-switch/mode-hunts. **LFP's flat plateau (~3.2-3.3 V) parks right there for most of the discharge** = a standing efficiency tax on everything; **Li-ion lives mostly in clean buck** (better converter efficiency, the counterpoint to LFP's safety/heat/cycle wins). **Test:** hold one fixed brightness, discharge full->empty, log `ima` vs `VBAT` -> maps converter efficiency vs SOC (the real budget input) + confirms the crossover bump; run on LFP and Li-ion to quantify the chemistry tax. NOTE this **confounds the existing PAR/mA efficiency plots** (each LED run was at a different SOC/load -> different converter point), so those slopes are *system* efficiency at as-measured conditions, not a clean LED-intrinsic ranking -- re-rank at a fixed VBAT (bench supply) or correct with this curve (Ben).
- [ ] **WS2812/SK6812 latch their last frame** -- firmware must send an explicit all-off on shutdown/sleep or the LEDs stay lit (and keep drawing) with no data; matters for low-power/shipping modes (Ben).

## Battery / solar sourcing

- [ ] Compare 18650 LiFePO4 sample capacity against rated capacity (Ben).
- [ ] Evaluate 26650 LiFePO4 only if 18650 sourcing or autonomy becomes a problem (Ben).
- [ ] Avoid multi-14430 production pack unless mechanical constraints force it (Ben + Steve).
- [~] Record panel dimensions, weight, output, connector type, and shipping lead time (Ben).
  P105/P126 Voltaic ETFE specs captured 2026-06-15 in
  `docs/tests/VOLTAIC_ETFE_PANEL_TEST_PREP_2026-06-15.md`; still record shipping/lead-time
  once procurement is active.
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
- [~] Add an autonomous low-VBAT park/cutoff policy for ordinary net_bench/production
  COMMS mode. **FIRST NET_BENCH PASS DONE 2026-06-30:** `--field-cycle` in
  `net-bench-2026-06-30.4` adds a production-ish solar day / radio-night lifecycle:
  charge-sleep on supply, wait-dark, always-awake 1 Hz radio drawdown, then protect
  timer-sleep at the LFP floor. Deployed to `9E5AB8` and logging to
  `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8.jsonl`. Remaining: analyze the
  first full cycle, tune full/taper and cutoff thresholds, then port the proven policy
  into production firmware (Ben/Codex).
- [ ] Analyze first `--field-cycle` run for `9E5AB8`: confirm 5-minute charge wakes,
  charge recovery from ~2.67 V on USB/solar, full-ish detection, transition to dark
  drawdown, protect cutoff reason/voltage, and mAh/Wh accounting quality. Data:
  `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8.jsonl` (Ben/Codex).
- [~] Add BQ25628E charger-state telemetry to `net_bench` and production telemetry:
  charge state, VBUS/source state, `CHG_EN`, `EN_HIZ`, BATFET control, input/charge
  faults, VINDPM/IINDPM, and effective charge-current limit. This is now important
  for interpreting low-VBAT solar/USB rescue: on 2026-06-30 a 5 V Anker bank was
  masked while solar held the input near 6.2 V; after solar removal USB worked but
  charged slowly at `supply_v=4.887`, `supply_ma=92`, `battery_ma=38`, likely due
  to 4.8 V solar VINDPM and/or low-VBAT precharge. Consider an automatic USB-rescue
  VINDPM policy around 4.6 V when the input is a 5 V bank rather than a panel
  (Ben/Codex). **NET_BENCH DONE 2026-06-30 in `.7`:** heartbeat/dashboard/logger now
  expose BQ VINDPM/ICHG/VREG plus raw control/status/fault registers and decoded
  `CHG_EN`, `EN_HIZ`, BATFET, VBUS, and charge-state bits. First USB-rescue sample:
  `bqv=4800`, `bqichg=1480`, `bqvreg=3600`, `CHG_EN=1`, `HIZ=0`, BATFET normal,
  VBUS adapter, charge-state CC bucket, `fault0=0`. Remaining: port to production
  telemetry and decide whether to add automatic USB-rescue VINDPM behavior.
- [ ] **WiFi re-associate guard (cheap roaming):** the ESP32 latches one Eero BSSID and won't auto-roam (no 802.11k/v/r -- LOG cont. 9, POWERFEATHER_NOTES). On link-loss / low-RSSI in maintenance mode, do `WiFi.disconnect()` + `WiFi.begin()` to re-pick the strongest beacon. Low field priority (deployed fixtures are stationary; the maintenance-OTA path already does a fresh `WiFi.begin`) -- but a belt-and-suspenders guard for OTA windows (Ben).
- [~] Implement ESP-NOW heartbeat/state packets with jitter and sequence numbers -- done in `firmware/net_bench/` (feasibility); port the validated packet/PDR design into production `core/packet` after the matrix run (Ben).
- [ ] Implement low-battery modes: dim, LED hard-off, shipping mode (Ben).
- [ ] **Revisit 8-bit LED low-end dimming for the ambient look** (deferred 2026-06-07): WS2812/SK6812 are 8-bit/channel, so gamma-on dims to OFF below ~brightness 24 (the ambient "~10%" spec sits in this dead-zone); gamma-off gives ultra-dim but non-linear steps. Options: dim-floor `max(1,gamma8(x))`, gentler gamma, gamma-on-color-only, or temporal dithering. See LOG 2026-06-07 + `firmware/POWERFEATHER_NOTES.md` (Ben).
- [ ] **Use the switchable 3V3 rail (GPIO4) as the LED kill-switch** in production firmware (`digitalWrite(4,LOW)` = LEDs off, can't drain the pack) -- folds into the pixel-power-architecture decision (option a). See `firmware/POWERFEATHER_NOTES.md` (Ben).
- [x] Implement watchdog/reset-reason/brownout logging -- DONE in `firmware/net_bench/` (esp_task_wdt + `--wdt-hangtest`); **validated 2026-06-07**: induced hang -> auto-reset -> `reset_reason=task_watchdog`, no human. Port to production firmware (Ben).
- [ ] Implement field telemetry logging schema for BM 2026 -> 2027 design data (Ben).
- [ ] Port `TalismanPatterns.cpp` into `firmware/core/pattern/` (Ben).
- [ ] Implement minimum-viable CA tick + render loop on bench (Ben).

## Production test / flashing

- [ ] Keep USB/pogo flashing as mandatory recovery path even if COTS boards support USB-C (Ben).
- [ ] Investigate JLCPCB / PCBWay firmware pre-flash only for custom-PCBA path (Ben).
- [ ] Write smoke-test host script: node ID, firmware version, battery, charge/fault, reset reason, peer count (Ben).
- [ ] Define production acceptance checklist for each fixture (Ben + Steve).

## Coordination with project team

- [ ] Confirm timing for Bamboo Pure air-ship of prototype lanterns to Steve in TN (Ben -> Elliot / Dipta).
- [ ] Align with Elliot on rope-attachment decision (Ben).
- [ ] Confirm hat OD / height / bamboo-overlap to Vishnu so he can finalize renders (Ben).
- [ ] Pull `INV_2026_00401`, decompose cost, compare to COTS/custom BOMs (Ben).
- [ ] Get Steve on project's official core build team wiki (Ben -> Elliot).
- [ ] Get shared access to Co-Work's wiki folder once cloud-hosted (Ben -> Elliot).
- [ ] Drop lighting workstream digest into WhatsApp after COTS bench results (Ben).

## Community Mandala Program (parked until Elliot signs off)

- [ ] Validate concept with Elliot + Vishnu (Ben).
- [ ] Pipeline: photo -> vectorize -> constraint check -> cone projection -> STL.
- [ ] Brightness normalization in firmware or per-filter metadata.
- [ ] Cataloging schema.
- [ ] Contributor brief / submission form.
- [ ] Submission window opens/closes early enough for printing and reprints.
