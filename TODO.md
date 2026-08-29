# TODO

Active punch list. Status: `[ ]` open, `[~]` in progress, `[x]` done. Owner in parens.

## Immediate documentation / repo hygiene

- [x] **Prevent field day sleep from resetting a qualified PROTECT release --
  DONE 2026-08-29.** Exact outer downlight Toad `F2BEE4` proved the defect on
  `fx-260827-1254f04-p`: healthy +506 mA charging in durable PROTECT was cut
  short by the independent 12-second `day-charge` sleep, making a continuous
  60-second release proof impossible. Lifecycle now makes power policy the
  sole owner of sleep at `LedTier::PROTECT`; native coverage pins PROTECT awake
  while FULL/DIM/OFF day behavior remains unchanged. The full integrated native
  suite and ESP32-S3 build pass. Exact-target USB canary
  `fx-260829-b0ff5db-b`, SHA-256
  `276d6558116a40da15f32eccf6bc7a940ef6d827ba965ddb81a8a8f5b0e27ae0`,
  passed on Toad with field/channel 11, healthy charging/no-fault BQ, correct
  downlight class, MSA/TMF/BMP samples, ESP-NOW, and a later clean 120-second
  timer wake. Toad's retained stage was already DIM after the USB flash, so
  this exact run did not replay a live PROTECT -> OFF release; the pure
  regression is the direct branch proof. Groot `9F2724` subsequently arrived
  in genuine low-voltage PROTECT, released on the old image during the longer
  physical-reset window, and passed the same exact canary plus USB/WiFi gate;
  this confirms a second repaired fixture. Logan `9E5A88` then supplied the
  missing live branch proof: the exact canary remained awake at 21.6 s after a
  deliberate one-second deep sleep (past the old 12 s cutoff), completed the
  sustained full-battery PROTECT release, persisted OFF, clean-rebooted, and
  advanced to DIM with healthy MSA/TMF and WiFi. This closes the timer-wake
  hardware acceptance. Eevee `9F0E54` then arrived in genuine low-voltage
  PROTECT with a retained 3.030 V entry. Physical RESET gave the old image's
  long service window; healthy roughly 1.1 A charging released it cleanly to
  OFF. The same exact canary passed USB/WiFi commissioning with its physical
  15 Ah capacity restored and healthy MSA/TMF/BMP telemetry (Ben/Codex).
- [ ] **Repair quarantined downlight Shuckle `F4031C`.** Its old commission
  image repeatedly failed all four PowerFeather init attempts with charger,
  gauge, rails, and sensors inaccessible. Disconnecting the sensor chain made
  the board healthy; TMF8820-only and MSA311-only each passed, while the suspect
  inter-sensor cable also failed when connected directly to the known-good MSA.
  Ben then accidentally connected solar without the battery and the USB app
  stopped enumerating. Recover USB-only without erasing, discard/mark the bad
  cable, validate a different cable directly, rebuild the full chain, reconnect
  the confirmed 15 Ah cell, and complete exact USB/WiFi commissioning before
  removing quarantine (Ben/Codex).
- [~] **P0: hardware-validate ADR 0071's hourly 72-cymbal daytime ritual.**
  Source now decouples energy readiness from all-day radio reception, carries
  readiness across only one freshly requalified timer wake, aligns the final
  sleep to T-20 seconds, and implements the fixed unison -> deterministic roll
  -> sparse after-ring through T+47 seconds. Native and exact-target embedded
  compile gates pass; nothing was flashed. On one physically identified,
  battery-installed cymbal downlight, prove high-quality UTC wake alignment,
  T+5 unison attempt, stable hash slot, optional after-ring, hard end, return to
  sleep, and no duplicate after an in-window reset. Then prove unison abstention
  with >500 ms uncertainty and all schedule/energy/lease/class/mechanism vetoes.
  Widen only through immutable named artifacts and explicit target cohorts
  (Ben/Codex; ADR 0071).
- [ ] **P0: run the exact-target radio-off + perimeter-ToF sentinel power
  campaign before implementing sentinel wake behavior.** Physically identify
  one healthy perimeter fixture, record its exact current artifact/SHA/state,
  build one immutable `--sentinel-trace-target <MAC>` `-t` image, and run the
  automatic 10-minute off / 30-second warm-up / 10-minute ToF / 10-minute off
  A/B/A. First use battery-isolated or consistently shaded input for load
  delta; repeat in full sun for net energy and at least 20 deliberate close-palm
  approaches. Download with `capture_sentinel_trace.py`, reject any radio leak,
  rail mismatch, stalled VL53 reads, reset/fault, poor sunlight detection, or
  unacceptable 6 Ah/night-budget cost, then restore and verify the exact prior
  fleet binary. Do not implement visitor-triggered fleet wake until Ben accepts
  this evidence; any eventual session needs a non-extendable deadline and
  cooldown (Ben/Codex; ADR 0071).
  **2026-08-29 partial field run:** physically confirmed Spyro `F2BCF0` passed
  exact canary OTA and pending verify; Ben performed about 15-16 close hand
  holds during the active phase. Do not accept a power delta: rapid sunrise
  contaminated the A/B/A trend, and an 8-second task-watchdog reset during the
  synchronous maintenance WiFi scan erased the completed PSRAM trace. Exact
  prior artifact `fx-260827-1254f04-p` was restored and fully verified without
  opening the lid. Source now uses bounded asynchronous WiFi scanning with
  watchdog service; native, Python, and embedded compile gates pass. Before the
  next full campaign, hardware-prove retrieval with this fix, then repeat under
  stable shade/battery-only and stable full sun. Also resolve Spyro's corrected
  boot probe still reporting only VL53 bit 2 rather than MSA+VL53 bits 10.
  **Controlled-shade rerun:** exact canary `fx-260829-9f140c3-t` held input
  below `supply_good`, and Ben made about 14-15 close approaches, but retrieval
  independently reset with `task_watchdog` again and erased PSRAM. Do not accept
  a power delta or edge count. Async WiFi scan alone is therefore insufficient.
  Source now checkpoints the completed trace to the unused SPIFFS partition
  with exact-artifact/header/sample CRC and sequence validation, then reloads it
  into retrieval after any reset. Native, 20 focused Python, and embedded build
  gates pass; hardware persistence/retrieval proof remains required. Exact-prior
  no-lid restore job `612D848D` completed and formally verified Spyro back on
  `fx-260827-1254f04-p` at 27,720 ms uptime, field/perimeter, no recovery or BQ
  fault. A later exact `fx-260829-96862d8-t` persistence attempt passed OTA but
  did not yield an accepted trace; job `E1428FE2` restored the exact prior image
  and verified it. Source now writes a CRC-protected run marker before
  acquisition, read-validates a completed checkpoint before advertising it,
  enters rail-off recovery-only maintenance after an interrupted/corrupt run,
  exposes the named persistence state, and makes the host refuse recovery-only
  evidence. Native and focused host tests pass. Next hardware run must prove
  complete checkpoint retrieval plus a deliberate retrieval reset without
  silently re-running the experiment.
  Evidence: `docs/tests/PERIMETER_SENTINEL_CANARY_2026-08-29.md`.
- [~] **P0: canary ADR 0069/0070 synchronized palette, count-aware HEX power,
  and role-correct ToF interaction.** Source and native tests now provide a
  20-minute UTC-synchronized GH hue cadence; a 765-RGB-channel-unit physical
  HEX budget that leaves sparse gobos full-bright and scales dense washes; and
  a local ToF renderer that maps 150-1,800 mm to color while peeling perimeter
  output 37 -> 19 -> 7 -> 1 pixels. The art-site TMF census sampled 46/61 exact
  downlights: 42 close returns at 145-575 mm and four healthy all-zero/open-view
  nodes (Panther `9E5A84`, Gible `9E5B34`, Sakura `F2BE0C`, Leia `F40384`).
  Fleet Identify later proved Sakura was in a bin, so zero depth is not evidence
  of installation height. The reported roughly 15 ft canopy height exposed an
  artificial 2.5 m parser/presence cap; hardware supports 5 m and a standing
  head should be about 2.7-3.2 m away. ADR 0070 now accepts the full range,
  re-baselines after a close-bin -> far-tree move or sustained empty scene, and
  drives a persistent full-W downlight gobo from learned per-zone presence.
  Interaction applies after every visible program, including direct bridge
  frames, but cannot awaken blackout or bypass battery/rail policy. Complete
  native tests and an ESP32-S3 development compile pass. The perimeter canary
  passed on Dixie and the base downlight/full-output canary passed on Froakie
  under ADR 0069, but the close downlight response was not exercised and is now
  superseded. **2026-08-29 partial installed canary:** exact Sakura `F2BE0C`
  passed immutable artifact `fx-260829-8790f6d-p`, SHA-256
  `5bd65970990a34e511b44cb9a9ef1a7dc4e93a7b3cd3746df08abafb3e0ab6bf`,
  fresh exact-revision rejoin, pending verify, field/downlight/recovery checks,
  post-update healthy zero-depth TMF telemetry, visible low-white lease, no
  empty-scene false trigger across eight minutes, exact release, and mesh
  cleanup. Ben then used Fleet Identify, pointed the bin-resident Sakura at
  himself, and visually confirmed the intended full-white response. People
  walking under the installed tree also reliably turn nearby canopy lights
  white. The interaction therefore passes artistically and Sakura's new path
  passes functionally. Before fleet promotion, still quantify one named
  installed fixture's raw range/margin at the roughly 15 ft height; if it cannot
  see a standing person, height/aim/sunlight remains a P0 installation gate.
  The MSA311 accent
  seam is implemented but fleet-disabled pending
  quiet/wind/touch/swing/climb traces; barometric modulation and aggregate
  ring/all-perimeter presence remain open (Ben/Codex; ADR 0069/0070).
- [~] **P1: characterize the canopy as a swinging scanner before tuning ToF or
  MSA thresholds.** Windy-night field observation shows each downlight swings
  chaotically over two pendulum axes. A stationary person can be discovered by
  multiple nearby fixtures as their ToF cones sweep past, producing a lively
  multi-light white response that Ben likes. Record one named installed fixture
  through empty wind, a stationary person, walking, and leaving with TMF scalar/
  per-zone range and confidence, MSA311 sway envelope, and visible presence
  state on one clock. Exact-target source instrumentation now compiles and
  records about 5.5 minutes at 25 Hz during real show behavior: raw XYZ,
  low-pass gravity, tilt/sway, all nine TMF zones/confidences, production
  presence edge/latch, lifecycle/program/power, and rendered output. Its
  maintenance downloader refuses identity/revision/target/sensor mismatches and
  exclusive-creates evidence. Native tests and an embedded ESP32-S3 test build
  pass. Next: physically identify one actually hanging downlight, build its
  immutable `-t` artifact, run the wind/still/walk/leave scene, download, and
  restore its exact pre-trace fleet artifact. Measure empty-scene false triggers and response durations,
  then tune minimum white hold, clear hysteresis, and retrigger cooldown. Do not
  use MSA311 as a blanket swing veto: that would erase the desired scanning
  behavior. Keep the currently proven thresholds until this trace exists
  (Ben/Codex; follows ADR 0070).
- [~] **P0: canary ADR 0068 high-VBAT PROTECT recovery and provenance.**
  Exact outer downlight Rikku `9F26B0` is dark on
  `fx-260827-1254f04-p` with durable guard stage 4 / power tier 3 and an entry
  audit of 3.596 V in field profile at 2.809 s uptime. This is not a low-voltage
  transition. Fleet history proves the latch predates that artifact: OTA at
  3.582 V preserved it, and a later gather still found the 900 s PROTECT cadence
  at 3.536 V. The leading entry mechanism is an unexpected reset with the
  durable load marker armed, but current audit fields do not retain the
  predecessor stage/reset reason. ADR 0068 source now preserves origin,
  predecessor, reset reason, prior `load_arm`, and reset streak in the same-size
  durable audit record and append-only heartbeat. Good USB/no-fault telemetry at
  3.56-3.58 V / 99 percent accepted only about 0-2 mA, so the policy now adds a
  separate sustained full-battery proof: corroborated real cell, >=3.45 V,
  valid/good input, valid enabled/no-fault BQ, and CV/top-off/not-charging state
  for 60 seconds. Missing data or a proof change resets the clock; CC with low
  current is rejected. Fixture, T-Deck, parser, and embedded compile gates pass.
  **2026-08-28 positive hardware canary passed:** clean combined artifact
  `fx-260828-abd893c-p` from commit `5865282`, SHA-256
  `6a0126f205be2cb6be034a71de5c5caa75f0af0acd00e1684ddc27377fb175f5`,
  OTA-verified on exact Rikku through pending verify. The retained false PROTECT
  satisfied the sustained full-battery proof, persisted stage 3, clean-rebooted,
  and advanced OFF -> DIM -> FULL with the daytime rail off, downlight class,
  healthy TMF/MSA, no mismatch, and no NVS erase or bypass. Next: negative-gate
  fault injection, then fleet audit for other high-VBAT PROTECT entries
  (Ben/Codex).
- [x] **Finish ADR 0067 canary acceptance on exact uplight Togepi `9E5AB0` --
  DONE 2026-08-28.** Before its exact USB update, registry-rostered 6 Ah uplight
  Togepi reproduced the original defect on `fx-260828-658b7d2-p`: chandelier
  class, `sensor_bits=0`, and no MSA311. Combined artifact
  `fx-260828-abd893c-p` fixed class/MSA detection but exposed a second source
  defect: the RGB uplight was incorrectly driven with four-byte `NEO_RGBW`, so
  logical smoke/rail-on telemetry produced no physical light. Commit `0f08904`
  pins uplight to one three-byte `NEO_GRB` pixel and uses RGB white for its smoke
  frame. Complete native tests and embedded compile pass. Clean replacement
  artifact `fx-260828-d8f62c3-p`, SHA-256
  `57f40023e1e599d60cf2a309e6a7af2f94bf45716421309b2b3a15048b239097`,
  passed exact USB write verification, uplight/MSA/class/power checks, visible
  white breathing with about 119 mA real load delta, `L0`, reset, rail-off, and
  ESP-NOW recovery. Donkey `F2BE10` was later found by exact cohort job
  `444537E1` and passed upload, fresh exact-revision rejoin, pending verify, and
  field-profile checks on the same artifact (Ben/Codex).
- [x] **Promote ADR 0067 plus the RGB uplight format fix to every manufactured
  uplight -- DONE 2026-08-28.** Exact jobs `45208DD7` and `444537E1` verified
  19 OTA targets through fresh exact-revision rejoin and pending verify;
  together with USB canary Togepi, all 20 physically manufactured uplights run
  immutable artifact `fx-260828-d8f62c3-p`. A direct settled check on Psyduck
  confirmed uplight class, `sensor_bits=8`, no mismatch, and field profile. Ben
  confirmed the physical cohort is 20, not the 24 rows previously rostered from
  planning allocations. Ken `F2B8DC`, Pikachu `F2BCE0`, Kirby `F2BE64`, and
  Haunter `F40438` are not manufactured uplights and are role-unassigned; they
  require no uplight rollout action (Ben/Codex).
- [~] **Finish dual-site maintenance WiFi deployment and the split-fleet tail
  (ADR 0066).** Source commit `91663fd` and immutable artifact
  `fx-260828-658b7d2-p` implement two bounded gitignored credential profiles;
  native tests, production build, exact Swablu canary, and 98/110 intended
  fixture promotion contracts passed. No secret is tracked. Hardware-prove the
  second profile at the art site with one exact fixture before relying on it for
  USB-tail avoidance. Exact Rikku `9F26B0` subsequently passed combined artifact
  `fx-260828-abd893c-p`; 13 intended fixtures still lack the current dual-site
  fleet image. The set is `9E5A88`, `9F0E54`, `9F2724`, `F2B7DC`, `F2B900`,
  `F2BCF0`, `F2BCF4`, `F2BDC4`, `F2BDD4`, `F2BEE4`, `F40308`, `F4035C`, and
  `F403DC`. The working
  11-unit perimeter USB queue is Cammy `F2B900`, Spyro `F2BCF0`, Gambit
  `F2BCF4`, Batman `F2BDC4`, Gengar `F2BDD4`, uncalled `F2BE80`, Clank
  `F2BF60`, uncalled `F2BFEC`, Thor `F40344`, Dratini `F4035C`, and Sneasel
  `F403DC`. Read exact USB identity before flashing because the physical
  slot-to-MAC map is still incomplete. Clank needs supervised low-voltage/
  commission recovery (Ben/Codex).
- [ ] **Close the ADR 0062 fleet-OTA control-plane gaps exposed by the dual-site
  rollout.** A 101-target job received only 93 roster acknowledgements; a
  53-target partial-discovery job failed the whole wave when eight found
  endpoints vanished before fresh maintenance preflight; and a PROTECT cleanup
  saw a transient dashboard HTTP 500 during freeze, then rejected a later safe
  bridge phase-3 stopped/expired status. Make roster loading acknowledged and
  retry-bounded, demote vanished endpoints to deferred before UPLOAD, retry
  transport-only freeze failures without weakening exact job/target evidence,
  and accept a fresh matching stopped phase as cleanup proof. Preserve no retry
  after upload ACK or ambiguous upload (Ben/Codex).
- [~] **Canary and promote trustworthy short-wake solar telemetry (ADR 0064).**
  The 120 s / 3 s artifact exposed a real defect: boot disabled charging, sent
  a full heartbeat before the 6 s battery guard, and could return to sleep
  before either charge enable or a fresh MAX17260 Current conversion. Source
  now appends explicit power-sample validity, withholds IBAT trust until 12 s,
  sends a corrected full heartbeat on the validity transition, and blocks
  ordinary day sleep until that sample or a bounded 15 s gauge-fault fail-open.
  Native fixture/T-Deck/host tests and both embedded builds pass; exact T-Deck
  `8EB508` is flashed with the new UI. Clean immutable credentialed candidate
  `fx-260827-db0cb73-p` is retained from source `8c0e577`, 1,207,408 bytes,
  SHA-256 `e30c42802f34966ca5c959539d20bf9c737786c935dc81c3934a3ff638386b5c`.
  **2026-08-27 canary:** sole writer `8EB508` OTA-flashed only field-profile
  Hawkeye `9F2664`; job `192D6A8B` proved exact-revision mesh rejoin past the
  pending-verify gate with no commission fixture or mutation. Read-only
  post-reboot evidence showed +312 to +323 mA at 3.567-3.573 V, strong input,
  `CHARGING_CV`, and DAY_ACTIVE after the 60 s solar-surplus confirmation. The
  exact job/upload ledgers are retained under `ops/bench/data/Nevada City/`.
  This strong-solar run could not exercise ordinary sleep. Before widening,
  reduce/remove Hawkeye's input and prove early IBAT unverified -> charge enable
  -> validated signed current at about 12 s -> 120 s sleep -> later timer wake.
  Also prove negative current, check a terminating/full battery, Health VBAT/CHG
  colors and detail, the 15 s no-gauge fail-open, Wake Fleet capture, and long
  Blackout lease. Include a deliberately persisted commission canary with
  `--fix-commission-profile` and preserve exact-target detection, NVS correction,
  and fresh field-profile confirmation in its OTA ledger (Ben/Codex).
  **2026-08-27 fleet promotion:** the prior candidate was superseded by current
  clean source `d6c9234`, including ADR 0065, in immutable production artifact
  `fx-260827-1254f04-p` (1,207,376 bytes; SHA-256
  `2f9a93344e172b023ee8df473b7c747b26f38dc0ec5353f6efd00d50ec45f4af`).
  Hawkeye passed the exact canary and all 109 post-canary targets accepted that
  same binary. The roster reports all 110 intended targets on the exact revision
  and FIELD profile; 107 have full fresh pending-verify proof. Record later fresh
  exact-revision wakes for `F2BCF4`, `F2BEE4`, and `F2B7DC`, which report the new
  image but missed the host freshness/verification deadline. The deployed fleet
  can now supply broad signed-IBAT solar-health evidence, but the explicit
  no-gauge fail-open and terminating/full-battery checks remain open (Ben/Codex).
- [ ] **Close LED Studio semantic-white behavior during class-census gaps.**
  The planner correctly sends dedicated W to a known downlight, but maps class
  `unknown` to RGB white and currently consumes only the raw T-Deck census. It
  does not use the embedded registry-role fallback already used by Fleet, so a
  known registered downlight can briefly receive RGB white when its full
  class-bearing heartbeat was missed but a short heartbeat made it fresh.
  Choose and test a fail-safe rule: use the registry role for class 0, or refuse
  semantic white until class telemetry is known. Validate on named downlight
  and RGB-uplight canaries without weakening live sensor class authority
  (Ben/Codex). **2026-08-27 hardware evidence:** Leia `F40384`, which had shown
  RGB white in LED Studio the prior night, now directly reports class downlight,
  no mismatch, sensor bits 5, and healthy MSA/TMF/BMP; TMF has 85,330 reads with
  zero errors/recoveries. This rules out a current fixture sensor failure and
  makes the raw-census unknown window the supported failure mechanism.
- [ ] **Prototype the T-Deck WiFi camera viewer after the Windows UVC viewer is
  accepted.** Keep the USB camera on a laptop or Raspberry Pi host and serve a
  bounded 320x240 JPEG/MJPEG view to Bridge OS over the channel-11 WiFi network.
  Measure latency, decode/render cost, heap/PSRAM watermark, and coexistence with
  ESP-NOW before making it an app. The stock T-Deck Plus USB-C is wired as a
  device/power sink and does not source camera VBUS; do not treat a passive OTG
  adapter as a host solution (Ben/Codex).
- [~] **Hardware-validate the permanent Bridge OS shell bar.** Source,
  pure-model tests, the full native suite, and the embedded build pass. Hardware
  rejected the initial full-width bottom overlay because it covered LED
  Studio's Solid/Stop/Back row; the compact top-left pill avoided the buttons
  but still covered titles. The current design reserves y=0..25 on every screen:
  app identity at left, scrolling activity/idle status in the center, and fixed
  clock/Stop cells at right only for local direct streams or tracked leases.
  The clock updates separately so it cannot restart the ribbon animation.
  Streams count up as `LOCAL ... until STOP`; leases count down; fresh competing
  T-Deck, PUCA, CoreS3, or unknown publishers are passive warnings by exact ID.
  All app content now starts at y>=26. Physical polish gives app identity, ticker,
  and clock distinct colors, expands Stop to avoid clipping, and labels the idle
  fields `WiFi joining`/`mesh-only`, `ch11`, `live N/M`, and `bat N%`. The exact
  image is 1,565,280 bytes and is
  flashed and boot/mesh-checked on camp-labelled TSwift `979604`; SHA-256 is
  `3267a1b237a2a4708e5c999cf1730f497ecbc30bd80e299a8b76374764931d4c`.
  On TSwift and named fixture canaries: start Solid, Blink, Patterns, CA,
  Blackout, and Contagion;
  navigate the launcher, every major app, and a confirmation modal; confirm the
  shell ribbon and touch Stop remain available without covering app controls;
  verify
  direct frames go stale within
  three seconds and program Release clears the lease. Check countdown expiry,
  foreign-only status without a misleading Stop, local-plus-foreign conflict
  color, self-frame suppression, known publisher labels, unknown-source ID,
  and no false persistence after the three-second freshness window. Exercise a
  second T-Deck plus PUCA/CoreS3 only after declaring one command operator and
  naming every target under ADR 0040 (Ben/Codex).
- [~] **Hardware-validate the stable/filterable T-Deck Fleet view.** Source,
  the complete native Bridge suite, and the local embedded build pass. The new
  default is the complete callsign-stable registry plus unexpected live peers;
  off-air fixtures retain grey rows, selection and scroll context survive the
  two-second refresh, and View offers roster/seen/live scope, class and raw-
  VBAT-band and charger-phase filters, plus stable ID/name and dynamic
  voltage/age/signal sorts.
  The confirmed Blink action snapshots only fresh visible rows and paces exact
  30-second green identifies. On exact T-Deck `8EB508`, check all dropdowns,
  touch and trackball navigation, a long-list scroll through several refreshes,
  detail/back context, the 192-row heap/PSRAM watermark, and an explicitly
  named 2-3-fixture filtered canary cohort. Confirm the modal count, hidden and
  off-air exclusions, and no list shuffle in the default sort. Also confirm that
  default `DAY` is readable in direct morning sun at full backlight, the Fleet
  `DAY`/`NITE` quick toggle and Settings switch agree, night restores the saved
  level of 59, and live/header/selected/battery/off-air cells remain legible in
  both modes. The verified day-mode binary is 1,579,040 bytes, SHA-256
  `473510ba76ec5ee9ce47e76575556ac0a7783c78445d913548473e0b3d4b819a`;
  this exact image is flashed on `8EB508`. Esptool write verification, a full
  application-region readback with the same SHA-256, and post-reset channel-11
  mesh/peripheral/memory checks passed. The physical UI and named-canary checks
  above remain. **2026-08-27 source follow-up:** Fleet now keeps VBAT and signed
  battery current on the compact row, moves RSSI/PDR and advisory SOC to detail,
  renders never-seen age as `inf`, and distinguishes a reported IDLE from unknown
  full-heartbeat state. Detail spells out profile, lifecycle, tier, program, and
  network-mode names. View adds charge phase, program, and exact-reference/non-
  reference firmware rollout filters. Charge filters cover `CHARGING_CC`,
  `CHARGING_CV`, `TOP-OFF`, `DONE/OFF`, `FAULT`, unknown, and off air. The
  complete native suite passes. The combined
  local binary now flashed on exact `8EB508` is 1,581,168 bytes, SHA-256
  `c87b2805feb8bd95c0d6c9ae3022baaa40079483bca652de6c33f738c0e69e7e`;
  the linker reports 50 percent flash and 50 percent global RAM. Upload and an
  independent whole-application `verify-flash` digest comparison passed;
  post-reset identity, channel 11, mesh RX/TX, peripheral probes, and memory
  checks passed. Add physical checks for no row/header wrapping,
  signed-current legibility, IDLE versus `?`, detail-line/button clearance,
  scrolling through all eight view rows, each charge-phase canary, and mixed-
  revision match/non-match cohorts. **2026-08-28 daylight UX finding:** the
  fleet mixes lifecycle labels
  such as `boot` with selected-program labels such as `CA`, which can read as a
  daytime-lighting fault even though live `ledrail=false` and zero rendered
  pixels prove the fixtures dark. Visually distinguish lifecycle from an armed
  night program (for example `state: boot` and `armed: CA`) and keep rail/render
  state authoritative (Ben/Codex).
- [x] **Repair the T-Deck development-cache metadata boundary and prove warm
  incremental builds. DONE 2026-08-27:** cache schema 2 treats
  `build/dev-cache` as Arduino-owned and stores recipe plus interruption state
  in sibling `build/dev-cache.state`. Clean and recovery cover both paths while
  preserving the atomic single-writer lock and fail-closed interrupted-build
  behavior. A compile-free regression deletes every entry, including dotfiles,
  from the simulated Arduino build path and proves the next invocation retains
  its recipe, reports `DEV_CACHE_HIT`, and reuses the library object. The real
  ESP32-S3 seed took 3,051 s; two no-source-change builds took 143 s and 145 s
  (about 21x faster), left all `.o`/`.a` timestamps untouched, and reproduced
  the same 1,579,040-byte SHA-256
  `d4eb675e4c0423310325d9c35d8613c985d75edaea29833cfc71cb9c3b67f3db`.
  The complete native Bridge suite passes (Ben/Codex).
- [ ] **Make independent cold T-Deck builds byte-reproducible, or document the
  exact volatile input.** Three same-source, same-size 1,579,040-byte cold
  applications have produced SHA-256
  `2cabfda91dd2f28e0fbb4bfc9092834201b046197421087ac5100ad7156683b9`,
  `473510ba76ec5ee9ce47e76575556ac0a7783c78445d913548473e0b3d4b819a`,
  and `d4eb675e4c0423310325d9c35d8613c985d75edaea29833cfc71cb9c3b67f3db`;
  the new warm-cache rebuild correctly reproduced the last hash exactly.
  Identify whether ESP-IDF/Arduino embeds compile time, ordering, or another
  host input. Do not confuse this with immutable shared-artifact identity: the
  exact flashed/read-back day-mode image remains the second hash (Ben/Codex).
- [ ] **Close the two unsafe USB recovery exceptions and Dratini graduation.**
  The 2026-08-27 supervised recovery put 23/25 observed USB-powered fixtures on
  `fx-260826-51d1fe1-p`; Thor `F40344` intentionally retained the protected
  net-bench image. Clank `F2BF60` remains on old firmware at 0.86 V, below ADR
  0042's 2.20 V fleet recovery floor: isolate and replace/bench-diagnose the
  cell, do not lower the floor. A 2026-08-27 read-only T-Deck profile audit also
  found Clank as the sole observed `prof=0` commission fixture; correct it to
  field only during that powered exact-target repair, not as an ordinary fleet
  write. Tidus `F40424` already has current firmware but reports about 0.01 V/
  recovery refused and its same audit profile was unknown: inspect its battery
  connector/cell.
  Dratini `F4035C` rose from 2.342 V to about 2.46 V in active recovery; keep it
  on USB until a fresh sample proves recovery state 4 above the 2.55 V/60 s
  graduation gate. See
  `docs/tests/USB_POWERED_FIXTURE_RECOVERY_2026-08-27.md` (Ben).
- [ ] **Prevent long T-Deck `nb-peer` lines from truncating late fields.** The
  10 s fleet snapshot cadence now fits 115200 baud, but the fixed 1024-byte
  per-peer buffer can still truncate profile/recovery/sleep-audit tails on a
  fully instrumented heartbeat. Increase the bounded line or split it under an
  explicit parse contract, add a max-tail regression, and verify the dashboard
  can audit profile without entering maintenance WiFi (Ben/Codex).
- [ ] **Preserve discovered endpoints across an all-or-none OTA discovery
  timeout.** On 2026-08-27, an eight-target campaign discovered five exact
  maintenance endpoints and then correctly stopped before upload when three
  required peers timed out. `discover_batch()` raised before returning its
  local `found` map, leaving `main()`'s finalizer unable to request `/resume`
  from the five already found endpoints. Refactor discovery to return partial
  state with its error (or raise a structured exception carrying it), retain
  the acknowledged FREEZE behavior, add a regression for required-target
  timeout after partial discovery, and prove every known endpoint receives
  final cleanup (Ben/Codex).
  **2026-08-27 full-rollout case:** an allow-partial 49-target job found 18
  endpoints, then one (`F2BD00`) disappeared before the fresh maintenance-power
  preflight. The whole selected cohort correctly received no upload, but the
  other 17 had to be rediscovered in a later job. Under allow-partial semantics,
  demote a vanished found endpoint back to deferred, retain the other exact
  identity-ready endpoints, and continue only after a fresh acknowledged FREEZE.
- [~] **USB-data rescue Bidoof `9F26D8`. USB/A-B WORK COMPLETE 2026-08-27.**
  COM156 exact identity, application-only USB write, preserved NVS, explicit
  persisted field profile, working `Party In The Woods` endpoint, app1
  pending-verify -> valid transition, and stable battery-backed production boot
  all passed on immutable `fx-260826-51d1fe1-p` / SHA-256
  `57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`.
  Final state was field/DAY_ACTIVE, channel 11, ESP-NOW 114/0 sends, healthy
  power/charge and MSA311/TMF8820. Remaining: with the T-Deck physically
  connected, issue one exact `U9F26D8` maintenance command and verify the same
  identity endpoint, then `/resume`; this only excludes a secondary receive
  fault and is no longer needed to repair the missing-WiFi root cause (Ben).
- [ ] **Make application-only immutable fixture USB upload first-class.** The
  retained `fx-260826-51d1fe1-p` directory has the application binary, manifest,
  hash, identity header, recipe, and build options, but not the bootloader and
  partition binaries expected by `arduino-cli upload --build-path`. That generic
  command failed before writing; the ESP32 application-only recipe at the
  telemetry-proven app address succeeded and preserved NVS. Add an identity-
  gated host path that validates target MAC, manifest/hash, current app address,
  and write verification without requiring missing companion files; update the
  USB rescue runbook and add a no-write failure regression (Ben/Codex).
- [x] **Finish the 120 s sleep / 3 s listen production cadence rollout. DONE
  2026-08-27.** Clean
  production artifact `fx-260826-51d1fe1-p` is built from commit `64264b2`,
  1,206,784 bytes, SHA-256
  `57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`.
  Full fixture tests and manifest/build-option checks pass. Stage-1 targets are
  `9F2638`, `9F26BC`, and `F4019C`; stage 1 passed OTA and the end-to-end
  120 s timer-sleep observation. Widening completed 85/98 production fixtures,
  each with fresh exact revision and 25 s pending-verify survival. Thirteen
  prior-revision fixtures were not discovered and not flashed:
  `9F266C,9F26B0,9F2714,F2B7DC,F2BCF4,F2BDD4,F2BF7C,F3FD28,F40174,F401DC,F402A8,F40314,F4042C`.
  Most report PROTECT, whose cadence is 900 s sleep / 8 s grace. Finish in a
  strong-sun window with one complete PROTECT gather plus margin; keep protected
  `F40344`, downlink exception `9F26D8`, and non-production revisions outside
  the target set. See the 2026-08-26 fleet OTA post-mortem (Ben/Codex).
  **2026-08-27 update:** USB recovery completed and pending-verify-proved seven
  of those thirteen (`F2BCF4,F2BDD4,F2BF7C,F3FD28,F401DC,F402A8,F40314`) plus
  thirteen additional old/intermediate fixtures. The final six original
  holdbacks (`9F266C,9F26B0,9F2714,F2B7DC,F40174,F4042C`) were uploaded and
  pending-verify-proved after rotating USB power; Cynder and Meowth required the
  separate 930 s PROTECT pass. Ponyta was then corrected exactly from persisted
  commission to field. Protected Thor and the separate Bidoof downlink exception
  remain intentionally outside that rollout scope. **ADR 0064 reopening:** the
  3 s grace predated the 6 s charge-enable guard and could put a timer-woken
  fixture back to sleep with charging disabled and cached-zero IBAT. Do not
  treat the installed cadence as accepted solar behavior. The replacement
  source enforces a trustworthy approximately 12 s power window with a 15 s
  fail-open; complete the named-canary and immutable fleet promotion above.
  **2026-08-27 replacement rollout:** `fx-260827-1254f04-p` is now reported by
  all 110 intended targets and every one reports FIELD profile. The strict host
  ledger pending-verify-proved 107; retain the item in progress until fresh wakes
  close `F2BCF4`, `F2BEE4`, and `F2B7DC` and the remaining ADR 0064 fault-path
  checks are exercised.
- [~] **Hardware-validate the first-class multi-target OTA state machine.**
  Source implementation is complete under ADR 0062: the T-Deck has a native-
  tested 160-target, 10 ms round-robin job roster plus explicit begin/add/freeze/
  status; the dashboard carries fresh structured campaign evidence; and the host
  now enforces PLAN -> PREFLIGHT -> GATHER -> DISCOVER -> FREEZE -> UPLOAD ->
  VERIFY -> CLEANUP. Timing derives from ordinary versus PROTECT cadence, no
  gather traffic is allowed after acknowledged FREEZE, HTTP failures reconcile
  against endpoint/mesh truth, reachable endpoints receive `/resume`, and one
  exclusive-created ledger records every target transition. See
  `docs/howto/FLEET_OTA_10_MINUTE_RUNBOOK.md`. Exact T-Deck `8EB508` now passes a
  live 130-ID synthetic roster test at `cycle=1300`: 348/348 successful sends,
  zero failures, stable peer census, and no dispatch after freeze. Legacy
  `UF00001` also remained an exact-target `U<ID>` command. **Hardware update
  2026-08-27:** staged real batches updated 20 USB-powered fixtures with exact
  pending-verify proof, including a 2.324 V low-VBAT canary and two-device
  expansion. Failed-closed canary attempts exposed/fixed serial saturation,
  status-request flooding, and a ledger field collision. Remaining: promote a
  clean immutable T-Deck artifact (the validated image is `dev-local`), run the
  explicit injected-interruption case, and time one full fleet pass. The host
  now audits every verified target's runtime profile and optional
  `--fix-commission-profile` performs only exact-target persisted FIELD writes
  with fresh expected-revision confirmation; hardware-validate that path with
  the ADR 0064 canary (Ben/Codex).
  **Full-fleet timing 2026-08-27:** Hawkeye canary passed in 53.844 s. The first
  103-target ordinary job took 589.5 s, essentially the ten-minute target, but
  discovered only 54 and verified 53. All 109 post-canary uploads eventually
  ACKed across six waves; upload windows totaled 310.75 s and individual ACKs
  were 10.43-29.22 s (median 14.41 s), beating the predicted 7-9-minute upload
  component. End-to-end rollout was 45 minutes 9 seconds because sparse WiFi
  association forced repeated gathers, one found endpoint vanished before fresh
  preflight and aborted a cohort, and three sleeping targets exhausted strict
  verification deadlines. Scheduler/FREEZE/cleanup and job ledgers held. Still
  promote an immutable T-Deck artifact and run the explicit injected-interrupt
  regression; fix discovery and verification timing before calling the full
  roughly ten-minute goal hardware-validated.
- [ ] **Expose positive awake/gather evidence.** Add compiled day-sleep,
  wake-listen, PROTECT-sleep, and PROTECT-grace values plus last-accepted-control
  age to fixture/bridge telemetry. Preserve separate evidence ages for short
  heartbeat, full-heartbeat fields, and firmware revision so `fresh` cannot be
  mistaken for `held awake` or make a cached revision appear current
  (Ben/Codex).
- [x] **Make fixture USB sleep parsing fail closed. DONE 2026-08-26:** sleep
  now requires the bounded, newline-terminated `!S<short-mac>:<seconds>` form;
  the target must match, duration must be explicit/canonical in 1..65535, and
  partial, wrong-target, nonprintable, repeated-sentinel, and overlong input is
  non-actionable. Bare `S`, `S1`, and the historical implicit six-hour default
  are removed. The commissioning TMF reset caller now sends the targeted form.
  The native suite passes a 37-check parser regression including 250,000 bytes
  of deterministic junk, and exact Akuma `9E668C` hardware ignored legacy/ROM-
  style `S` input with no new sleep audit, then accepted only
  `!S9E668C:10`, woke by timer with a retained 10-second serial cause, and
  returned to DAY_ACTIVE. The field dev binary is 1,205,856 bytes, SHA-256
  `54A3118B10BB9A9FD58815429AC4470599B71EE621DF9B73AE717A001A354AB8`.
  ModemManager exclusion remains useful defense in depth (Ben/Codex).
- [x] **Make same-revision OTA verification fail closed. DONE 2026-08-26:**
  `field_cycle_ota.py` now snapshots pre-job uptime/sequence, treats
  `dev-local` and an unchanged expected revision as mutable identities, and
  requires a post-job reset transition, a fresh expected-revision heartbeat,
  and 25 seconds of pending-verify survival. Five host tests cover unique,
  cached, reset, missing-baseline, stale, and wrong-revision evidence. A second
  Akuma same-name OTA then failed closed when the dashboard only reprinted its
  pre-reboot record; direct USB independently proved the OTA valid. Keep ADR
  0040 unique revisions for shared artifacts (Ben/Codex).
- [~] **Measure Akuma's 120 s sleep / 3 s listen canary cadence before fleet
  promotion.** Production defaults remain 300 s/15 s. Build controls now make
  both values explicit and range-checked, the recipe fingerprint includes
  them, and USB telemetry reports the compiled values. Exact Akuma `9E668C`
  runs the 1,205,920-byte `dev-local` canary, SHA-256
  `7fca2ae63b3c840c31400d269f382cf0222a66e2e5bc0d839a9b8e7297d3283c`;
  direct USB proves software reset, valid `app0`, pending verify cleared,
  `day_sleep_s=120`, and `wake_listen_ms=3000`. With USB removed and the panel
  made opaque, three clean sustained addressed-maintenance trials caught true
  timer wakes in 86.4, 100.9, and 94.3 seconds (3/3). Each endpoint reported
  `reset_reason=deepsleep`, retained `last_sleep_reason=day-charge` / 120 s,
  and maintenance uptime 3.746-4.380 s; command receipt necessarily occurred
  inside the 3 s comms window before WiFi startup. Final `/resume` returned
  Akuma to natural cadence. No INA instrumentation is available at Burning Man,
  so use the explicit theory gate instead of leaving an impossible field test:
  model fixed wake/setup energy as `b` steady-awake-equivalent seconds. Comparing
  complete 120/3 and 300/15 cycles breaks even at `b=5 s`; 120/3 saves energy
  below that and costs energy above it. Akuma emitted a post-deep-sleep heartbeat
  at about 1.1 s uptime, and even a 2-3x boot-current multiplier remains below
  the 5 s boundary. Treat 120/3 as likely lower radio-awake energy as well as
  faster maintenance access; remaining gate is a staged fleet rollout and live
  census/catch observation, not unavailable current instrumentation (Ben/Codex).
- [ ] **Resolve the same-revision Akuma bridge/direct evidence disagreement.**
  During OTA, direct USB proved a new software-reset boot while the T-Deck
  dashboard still presented pre-reset uptime/sequence as recent enough to
  inspect. Later interleaved maintenance, deep-sleep, and software-boot records
  were difficult to order. `nb-peer` already includes the T-Deck's calculated
  radio age, so do not assume the missing fix is merely another age field.
  Reproduce on an isolated canary with timestamped raw T-Deck serial, direct
  USB, and HTTP telemetry; determine whether emission ordering, serial backlog,
  or census restart accounting is responsible. Until then, the OTA verifier
  must continue to fail closed and require direct or unique-revision evidence
  when reset identity is ambiguous (Ben/Codex).
- [ ] **Make laptop/T-Deck identify semantics visibly consistent.** The Fleet
  UI calls `meshIdentify(..., green, blink, 128)` and lights the production
  LEDs, but serial quick command `i<ID>:<secs>` uses legacy color 0 and only the
  enclosed PowerFeather status LED. The field manual currently describes the
  serial example as green. Add an explicit safe production-light syntax or
  align the default, cover it natively, and document status-only versus fixture
  output so a closed enclosure can actually be identified (Ben/Codex).
- [~] **Hardware-validate CoreS3 wireless two-app Bridge OS (ADR 0054).** The
  ordinary image now boots to a touch launcher with read-only Listener and
  Audio apps; USB remains optional, Module Audio is a hardware build variant,
  and exiting Audio stops its direct-frame publisher. The Module build now has
  a runtime Ambient/Aux Input selector with a zero-frame handoff and delayed-
  module retry (ADR 0057). Native tests pass 42 checks and the exact CoreS3
  `4D5DB0` (`80:45:6B:4D:5D:B0`) now runs the inspected
  `cores3-os-0.1.2-dev` Module Audio artifact: 1,168,352 bytes, SHA-256
  `FDDAC35CA9778D1698763F77FAABA88A5FBB56A8167C1D24EE6E0701F1742C65`.
  USB flash/hash verification, current boot identity, channel-11 passive mesh
  receive, Aux readiness, USB `nb-*` queries, paused/no-frame startup, and zero
  queue drops passed. The common Input action completed Aux -> Ambient -> Aux
  on hardware while paused with zero direct frames/read failures. The first
  standalone test exposed two blockers now fixed by ADR 0058: Audio Start
  automatically releases an explicit CA/Contagion/Dark lease in RAM, and the
  target selector recognizes current ADR 0040 `fx-*` firmware identities. The
  exact app-region readback matched the artifact SHA. A live Ambient run selected
  Builtin Dual Mic, calibrated, published four fleet chunks per tick with zero
  failures, and an independent T-Deck observed fresh fixtures enter program 3
  Direct instead of CA. Remaining:
  physically verify all three touch targets, battery-only operation,
  more-than-24-peer Listener paging, named-fixture health/detail parity with the
  host dashboard, fresh built-in-only/Cambium compile smoke checks, all four
  RODE looks, active-stream Input handoff, pause/start, app-exit zero frame,
  mixed HEX/RGBW stale fallback, and unplugged Listener/publishing runtime. Use
  explicit canaries; do not start Audio while T-Deck LED Studio/Patterns or
  another publisher owns the look. Ben confirmed the standalone `AMBIENT MIC`
  speech response works really well across the awake cohort. Pause/leave Audio
  and observe the roughly three-second fallback; active source handoff and a full
  300-second sleeper-cadence run remain open (Ben/Codex).
- [~] **Hardware-validate CoreS3 spectral Audio (ADR 0061).** Source and native
  tone tests cover the 512-sample FFT, bass/mid/high separation, centroid order,
  stereo-to-mono Aux handling, and BANDS RGB/BANDS SPLIT color planning. Exact
  CoreS3 `4D5DB0` now passes firmware cadence: Ambient held 24.802 Hz analysis,
  24.799 Hz display, and 9.999 Hz fleet TX for 65 seconds with no read, send, or
  RX queue failures; Aux held 24.020/24.006/10.000 Hz for 70 seconds but had one
  send failure. Remaining: verify the scrolling plot has no false high-frequency
  rail, exercise touch/UI through a 30-minute zero-failure soak, and use an
  explicitly owned awake cohort for bass-, mid-, and treble-dominant material in
  BANDS RGB, BANDS SPLIT, and TIMBRE HUE. Finish with app-exit/autonomous-fallback
  observations and confirm again that no fixture NVS or OTA changed (Ben/Codex).
- [~] **Execute the low-latency audio-reactivity development plan.** Sound-to-
  photon latency and cross-fixture skew are now first-class artistic gates; the
  staged plan is
  `docs/projects/LOW_LATENCY_AUDIO_REACTIVITY_DEV_PLAN.md`. Milestone A source is
  implemented on CoreS3: independent phase-locked 25 Hz analysis, 10 Hz publish,
  and 25 Hz display deadlines remove the permanent 120 ms / 8.3 Hz quantization;
  achieved-rate, interval, skip, lateness, and maximum blocking telemetry expose
  real cadence; calibration now spans two seconds of contiguous successful
  captures; and input older than 200 ms stops publishing plus sends one black
  frame instead of replaying stale brightness. The native CoreS3 suite passes
  167 checks. Exact CoreS3 `4D5DB0` is running the fresh exclusive Module Audio/
  channel-11 `r6` build: 1,189,072 bytes, SHA-256
  `4AB7DB1D7F09526CD82A6E6FE7B22C5455F5DAA951C3956AED8ABB10A6F2E6C2`.
  Both Ambient and Aux pass the 60-second analysis/display/publish rate gates;
  the zero-failure 30-minute soak and physical Aux photodiode/scope plus Ambient
  video median/p95/max and cohort-skew measurements remain open. Milestone B
  adds 8 ms transient blocks
  beside the rolling 512-sample FFT and measures the safe legacy direct-frame
  rate. Only measured failure of the current fixture ceiling advances to the
  proposed type-31 compact feature packet, local 50 Hz audio render, USB
  HEX/RGBW canaries, and a separately authorized gated OTA. This work does not
  authorize any fixture flash or OTA (Ben/Codex).
- [~] **Hardware-validate DG1022Z -> PUCA HEARTBEAT mode.** The standalone
  hardware/radio baseline passes on the exact PUCA with `0.4.1-dev`:
  HEARTBEAT + line input + LOCKED boot, powered paw/knob inputs, codec/stereo
  capture, 70+ eligible fixture census, and 8,318 successful send callbacks
  over 207 s with all reported error/clipping counters zero. The current 103
  native checks pass; installed `0.4.1-dev` binary is 964,752 bytes with SHA-256
  `e8ec74680564f96f10c2f6e87b37eb807b9d9ba3b355ccf41c72f8301c4984b6`.
  ADR 0063 `0.5.0-dev` is now credentialed and installed. The 2026-08-27 bench
  pass proved no-hold SAFE-IDLE with zero direct frames, `A4EB10` Bridge OS
  identity, exact-target shared-WiFi maintenance, retained-binary OTA, dark
  software-reset rejoin, and 25 s pending-verify survival. Still prove the
  physical paw-held DJ-first/setup gestures, `/resume`, maintenance timeout,
  fleet-wide-maintenance rejection without disturbing the live fleet, no PUCA
  softAP, and forced-self-test rollback.
  On the no-human bench, prove `peak`/`wave`/deep-red output from the performer's
  exact arbitrary waveform with zero clip blocks. Compare the analog waveform
  to one fixture by photodiode/scope and record whether the 100 ms peak window
  and current 10 Hz fixture latch preserve the intended single/double pulse.
  Do not T-split or otherwise electrically join PUCA to the body-connected
  ceremony chain until its owner documents the complete circuit and a qualified
  reviewer approves a separately isolated monitor boundary with all field power
  and USB connections present. The rear Sync output is timing-only, not a copy
  of the arbitrary waveform; direct connection to the AC-coupled audio input
  may detect both square-wave edges (Ben/Codex).
- [ ] **Physically confirm the inferred full MACs for the three restored fleet
  identities.** The retained 2026-08-18 OTA/RSSI evidence proves short IDs
  `F2BDFC`, `F402A4`, and `F40348`; their `68:EE:8F` OUIs were reconstructed
  from the surrounding PowerFeather batch so the canonical registry and
  generated Health roster can remain internally consistent. Confirm each full
  MAC from a live identity surface or device label before treating that prefix
  as hardware evidence. Short-ID mesh targeting and the bridge-side Magmar,
  Magneto, and Marill callsigns do not depend on this check (Ben/Codex).
- [ ] **Capture real Bridge OS/CoreS3/PUCA manual photographs.** The new
  `docs/howto/BRIDGE_OS_FIELD_MANUAL.md` includes diffable source-derived role
  and launcher visuals. Add clean field photos of the launcher, Health, Fleet
  detail, LED Studio, Sleep confirmation, Schedule, both RF pages, CoreS3
  normal/audio screens, and labeled PUCA ribbon/front-panel audio connections
  as those states are deliberately staged. Do not send a control merely to make
  a documentation photo; use the named-canary acceptance passes already queued
  below (Ben/Codex).
- [ ] **Support spaced WiFi credentials in T-Deck serial provisioning.** The
  current `set wifi <ssid> <psk>` tokenizer requires both values to be one token
  and cannot safely enter the exact production maintenance SSID
  `Party In The Woods`. Add quoted/escaped fields or another bounded secret
  provisioning path, native-test it, and keep `show` from exposing more of the
  API key than operators need. Until then, use only an approved one-token
  bridge SSID; do not pretend quoting works (Ben/Codex).
- [~] **Hardware-validate fixture callsigns on the T-Deck.** The canonical table,
  17 host policy/dashboard tests, the complete native Bridge suite, and the
  final Magic-Wand-aware embedded build pass. The reconciled 141-fixture image
  is 1,550,224 bytes with SHA-256
  `3026593615bd58304c2a6b8893bf4f92cd8f9f92211f9222a5a28517fedf6e32`.
  The current combined image now flashed on exact `8EB508` carries all 144
  identities, including `Magmar [F2BDFC]`, `Magneto [F402A4]`, and
  `Marill [F40348]`.
  It is USB-flashed to exact T-Deck `8EB508`; upload verification, hardware
  probes, channel 11 rejoin, zero-failure mesh transmission, and fresh receive
  from all seven newly registered IDs passed. Check
  Fleet rows/details, Health selection/details, and Claude lookup/identify on one
  explicitly named canary. Confirm every action surface shows both callsign and
  short MAC, compact grids remain readable, unknown peers fall back to MAC, and
  the wire target is unchanged. Do not send a fixture command without naming and
  accounting for the exact canary first (Ben/Codex).
- [~] **Hardware-validate Bridge OS Patterns v1 on named canaries.** The pure
  deterministic model passes 190 checks and the combined embedded image builds.
  The combined image is USB-flashed on exact T-Deck `8EB508`: 1,542,448 bytes,
  SHA-256 `705119167e51ae8dff399a6c46cfd442b1610d14d0acb5d8a470c63461242b46`.
  On one awake HEX plus one point-source fixture, exercise Wash, Chase, Wave,
  and Twinkle; all five palettes; speed/intensity edges; class and stable cohort
  filters; explicit Stop; three-second stale fallback; and owner replacement in
  both directions with LED Studio. This is live direct-frame artistic control,
  so name and account for the canaries before starting a pattern (Ben/Codex).
- [ ] **Hardware-validate Bridge OS commission defaults on named canaries.**
  Source and both native/embedded builds pass for the exact-target Default app
  and fixture type-30 receiver; the current combined image is flashed on exact
  T-Deck `8EB508`. On one downlight, perimeter, and RGB uplight,
  exercise ready beacon, light-only wildfire CA, and strict rails-off dark;
  active-lease override and release/expiry; until-reboot reset; one persisted
  CA reboot followed by a persisted restore to listener; field-profile
  non-interference; and a small `ALL: targeted fresh` cohort with no non-target
  mutation. Declare the T-Deck as the sole NVS writer and name every short MAC
  before the persistent pass (ADR 0052; Ben/Codex).
- [ ] **Hardware-recheck LED Studio semantic white on mixed hardware.** On one
  named RGB uplight and RGBW downlight, confirm white produces full `R=G=B` /
  `W=0` on the RGB-class target and dedicated `W` on the downlight, including
  dim, blink, all-class planning, Stop, and three-second fallback. Add a named
  RGB chandelier canary when deployed (Ben/Codex).
- [~] **Hardware-validate the read-only T-Deck RF Diagnostics app.** Check the
  summary and frame-tail pages, touch/trackball navigation, live/seen/stale and
  roster-unobserved counts, foreign-live labelling, strongest/weakest ordering,
  unavailable/partial observation labels, RX/TX counters, and WiFi/AP/channel
  guard state. The app must send no mesh command and alter no fixture state
  during validation. It is flashed in the combined `8EB508` image identified
  above (Ben/Codex).
- [~] **Hardware-validate the T-Deck Health app.** Source, native tests, and the
  merged image now flashed to T-Deck `8EB508` cover the 134-entry production registry,
  stable no-scroll tile ordering,
  fresh/off-air merge, unexpected live IDs, and exact raw-VBAT bands: green
  >3.20 V, yellow >3.10 V, red <=3.10 V. On T-Deck `8EB508`, verify all tiles
  fit at once, red/yellow/green/grey/blue rendering, touch and trackball detail
  entry, a known off-air fixture, a known low canary, and heap/PSRAM watermarks
  after repeated open/detail/back cycles. This app is read-only; do not send a
  fleet command during validation. Health remains present in the combined
  1,542,448-byte image identified above. Ben reports the first physical
  Health/Schedule smoke check looks good on 2026-08-24; retain the explicit
  color/off-air and memory matrix before closing this item (Ben/Codex).
- [~] **Field-validate the T-Deck LED Studio / Sleep app on explicit canaries.**
  T-Deck `8EB508` on `COM152` now runs `tdeck-dev-local` and passed exact flash
  verification, clean boot, peripherals, memory, correct machine identity, and
  live fleet receive. Ben exercised LED Studio, Sleep / Dark, Knocker, and CA
  Studio on the night of 2026-08-23/24 and reports that all behaved as designed.
  This is a broad functional field pass, not yet the full canary matrix. Retained
  `field2` source fixes `field1`'s stale telemetry-only version label. Name one
  HEX plus one point-source fixture, then verify RGB, class filtering, 1 Hz
  blink, stop fallback, 10-minute dark expiry, and a short rails-off timer
  sleep/rejoin. Before any overnight fleet sleep, compare live/seen counts and
  account for radio-silent fixtures separately (Ben).
- [~] **Hardware-validate durable bridge-action and fixture-sleep provenance
  (ADR 0059).** Source now keeps four checksummed T-Deck actions in NVS before
  RF; fixtures keep the immediate timer-sleep cause in RTC memory, persist rare
  operator-sleep receipts plus first PROTECT entry, and refuse an operator sleep
  whose audit cannot persist. The 192-byte heartbeat tail, T-Deck emitter, host
  dashboard/logger, USB JSON, native suites, parser tests, and both embedded
  development builds pass. Fixture-side hardware now has a partial pass on
  exact Akuma: addressed serial sleep woke by timer with retained
  `last_sleep_reason=serial` and the exact duration, and the later cadence
  canary produced three more true timer wakes retaining
  `last_sleep_reason=day-charge`, `last_sleep_s=120`, battery voltage, profile,
  lifecycle state, and tier. The older durable operator receipt remained
  intact alongside the immediate RTC record. The 99-fixture Color Virus fleet artifact predates
  ADR 0059, and the provenance-capable T-Deck development image was compiled
  but not flashed. On an isolated radio cohort, validate Dark,
  Release, Schedule, and a short Sleep; exact source/sequence correlation;
  T-Deck reboot survival; fixture timer-wake and later power-cycle survival;
  automatic day/PROTECT RTC evidence; one-write-per-PROTECT behavior; and both
  fail-closed storage paths. Do not perform the Sleep pass against an
  unaccounted live fleet (Ben/Codex).
- [~] **Hardware-validate the conditional solar-probe wake (ADR 0060).** Source
  now preserves the 300 s/15 s field cadence until a FULL-tier fixture measures
  at least 150 mA good input, then holds only that wake for a continuous
  60-second proof before entering `DAY_ACTIVE`. Battery voltage alone no longer
  counts as surplus; active hysteresis is 100 mA for 300 seconds, while strikes
  still require 150 mA and every existing gate. The full native suite and a
  field-profile ESP32 development compile pass. Exact Akuma `9E668C` proved the
  positive hardware path: about 4.64 V external input held the wake through the
  60-second confirmation into `DAY_ACTIVE` and remained continuously reachable.
  The Akuma cadence canary also proved the below-100 mA exit side: an upside-down
  panel on reflective playa still delivered about 538 mA, while opaque backing
  reduced it through 42 mA to 0; after hysteresis it returned to DAY_CHARGE and
  repeatedly timer-slept. Still prove the production 15-second weak-input path,
  transient cancellation, the 100-149 mA
  awake-but-no-strike band, below-100 mA fallback, commission no-sleep, and
  unchanged PROTECT timing. Measure cloudy threshold behavior before creating
  an immutable fleet artifact (Ben/Codex).
- [~] **Hardware-validate the revised Knocker fleet modes.** Field use exposed that
  the old `knock all` selected at most 32 fresh fixtures in heartbeat order and
  paced targeted sends every 300 ms. Current source plans the complete
  192-entry census in deterministic short-ID order, dispatches every 80 ms, and
  labels that action as a targeted roll. It now also offers immediate fleet
  multicast and a shared +1.0 s multicast deadline through the existing
  `NbEvent.fire_in_ms` field. Fixtures deduplicate the event, timestamp radio
  receipt, arm only one pending strike, and drop it if more than 250 ms late.
  ADR 0065 now makes deliberate Knocker traffic a best-effort mechanism attempt:
  it bypasses lifecycle/solar/tier qualification while retaining arm, rest,
  maintenance, pulse, load-marker, deadline, and failsafe gates. Native
  coverage and both embedded development builds pass. Before flashing the new
  T-Deck UI, deploy the matching fixture image only to an isolated, explicitly
  armed cohort.
  Confirm one strike per event despite six RF copies, old-firmware ignore,
  immediate multicast spread, +1.0 s deadline skew, hard-gate refusal, and no
  late fire after a blocked loop/maintenance interval. Do not exercise either
  multicast mode around an unaccounted strike-permitted fleet. **Compatibility
  observation 2026-08-25:** targeted roll still strikes the deployed pre-event
  fixture image while both new multicast choices are silent, exactly matching
  the documented old-firmware ignore path; this is not evidence that the new
  receiver failed. The isolated matching-image canary remains required
  (Ben + Codex). **2026-08-27 field diagnosis:** a reported 92-live roll was
  audibly sparse. A later read-only snapshot had 83 peers under the five-second
  fresh threshold but only four with affirmative FIELD + DAY_ACTIVE + FULL +
  >=150 mA input evidence; 18 known rows were DAY_CHARGE and 60 fresh serial
  rows lacked usable full-state tails. Ben rejected using those advisory fields
  as an operator-knock filter: an empty capacitor simply produces weak/no
  motion. Current source therefore continues to roll every fresh ID and removes
  the receiver energy refusal for targeted/multicast operator traffic. It still
  takes about 7.36 s for 92 targets, so compare targeted roll with broadcast-now
  and sync-+1.0 s on the matching-image cohort. Hardware-prove deliberate
  DAY_CHARGE/night/non-FULL attempts plus every retained hard gate before fleet
  promotion (Ben + Codex).
- [x] **Seed and validate the locked T-Deck development cache -- DONE
  2026-08-24.** The fixture cache's lock, recipe fingerprint, interruption
  marker, quarantine recovery, `tdeck-dev-local` identity, and fresh-artifact
  boundary are ported. Compile-free wrapper checks, native tests, interrupted
  recovery, cold seed, identical-SHA warm reuse, exact USB flash, and clean
  boot/mesh verification passed on `8EB508`. The heavy-load cold seed took
  about 55 minutes and the first warm no-op about 76 seconds; later timing was
  variable. The old interrupted `tdeck-ledsleep-20260824-field2` directory was
  not reused (Ben + Codex).

- [ ] **Recover the exact `fx-260819-7afe0a6-b` bench artifact from a deployed
  prototype if needed.** A mistaken ignored-build cleanup removed the last found
  filesystem copy. Do not rebuild or reuse the revision. Read exactly the
  1,170,736 app bytes from `9E5AF0` or `9E5AB8` only when one is explicitly on
  USB, then accept the recovery only if SHA-256 equals
  `95e8d74727089c9bc309ae66109c2f26c1cb7cb7888d84c8fe90158f8bc9fcbc`
  (Ben + Codex).
- [ ] **Finish optional fixture build job-count/hardware follow-ups.** During one
  stable awake session, repeat controlled cold `--jobs 1/4/0` trials; the Aug
  22-24 suspended/aborted trials are invalid. If useful, flash `dev-local` to one
  explicitly named sacrificial USB fixture and verify fresh telemetry. Cache
  reuse itself is already host-adopted and does not depend on this tuning (Ben +
  Codex).
- [ ] **NeoHex-Magic-Wand playa follow-up.** The installed `.1` image is working
  on battery and its OTA transport/reboot bench test passed. Before replacing
  it, build the current `.2` source as one immutable artifact, target only
  `F40344` / `68:EE:8F:F4:03:44`, use the installed LFP for ride-through, and
  complete the fresh-heartbeat/pending-verify acceptance. Then map MSA311
  movement and BMP581 relative elevation into reviewed pattern behavior. Replace
  the Tennessee maintenance WiFi profile with the agreed playa router profile
  first. Registry role `magic_wand` and the batch-OTA sole-target interlock are
  complete (ADR 0050; Ben + Steve).
- [x] **Capture the Nevada City rig RSSI matrix before pack-out -- DONE
  2026-08-17.** Bridge `4D5DB0` and 84 qualified fixtures received the immutable
  transport/RSSI image. The 140-second `L` survey preserved 25,154 observations
  from 48 reporters hearing 96 transmitters, with 4,558 unique directed pairs,
  in `ops/locate/data/field/20260818-0300-nevada-city-rig-rssi.jsonl`
  (Ben/Codex).
- [x] **Arm a multi-day transport sleep without creating a lid-open wake task --
  DONE 2026-08-17.** All 84 exact-revision fixtures accepted `Q99` at about
  20:11 PDT and disappeared from fresh telemetry. Timer wake is expected about
  23:11 PDT Friday; RESET wake was field-confirmed without flashing. Ten
  then-reachable old fixtures received only the `S65535` 18.2-hour fallback.
  After unload, bridge `b` clears the retained LED-dark latch (Ben/Codex + field
  team).
- [ ] **Recover a planar/grid embedding from the Nevada City RSSI matrix.** Start
  with the raw 48-reporter/96-transmitter directed capture, symmetrize with
  explicit missing-edge handling, compare rank-only and calibrated-RSSI distance
  models, and score the result against only the known 6x12, 2x12, and 1x8 rig
  shapes. The captured rows are one-sample neighbor-table EWMAs, not independent
  window medians, so uncertainty and censoring must remain explicit (Ben/Codex).
- [ ] **Close the awake test cohort before final pack-out.** Elliot may use
  `9E5954`, `9F0E30`, `9F0E5C`, `9F26E4`, `F40174`, and `9F2720` while Ben is in
  Oakland. All six had qualified USB input when left. On Ben's Thursday/Friday
  return, verify power, then transport-sleep this awake subset for the remaining
  transit interval. Do not disturb the 84-device long-sleep cohort whose Friday
  night timer already aligns with unload (Ben + Elliot).
- [ ] **Reconcile `9F2720` battery capacity.** USB commissioning proved healthy
  canopy/downlight sensors and current firmware, but the persisted/registry
  capacity is still 6,000 mAh. Physically identify the installed cell before
  changing it to the 15,000 mAh large-enclosure setting (Ben).

- [x] **Promote the 2026-08-17 small-fixes fixture candidate -- DONE
  2026-08-17.** Built immutable `fx-260818-05ed4b3-b` once from clean commit
  `e09f46f`; binary SHA-256 is
  `2986a0294827ef6be970d2ffe50066c885f3107f139f8601d5054d797467e1db`.
  Battery-backed canary `F40174`, the 56-target main wave, and the three-target
  safe tail all returned fresh exact-revision heartbeats and survived the
  20-second pending-verify gate: 60/60 attempted and verified. All 60 later
  reported LED rail off and zero lit pixels under the dark lease. No low-voltage,
  recovery, downlink-anomalous, or slot-anomalous fixture was flashed; bridge
  `4D5DB0` was untouched (Ben/Codex).
- [x] **Complete small-rig perimeter QC and battery replacement -- DONE
  2026-08-26.** Ben confirmed that all 24 perimeter fixtures were QC'd: every
  installed cell below 3.10 V was removed and replaced with a factory-fresh cell
  individually verified above 3.20 V. None of the installed perimeters should
  now be batteryless. Treat older registry and TODO batteryless annotations as
  stale until the physical map is reconciled (Ben + field team).
- [ ] **Diagnose the post-QC perimeter energy collapse.** On the evening of
  2026-08-26 only 2 of 24 perimeters responded visibly to a healthy LED Studio
  stream, versus roughly 8-12 dark canopy and 2 dark installed trunk fixtures.
  The two live perimeters were already near the LEDs-off/protect thresholds,
  and several silent known perimeters had prior PROTECT evidence. Treat
  persisted commission profile plus old charge recovery as the leading
  firmware-mediated hypothesis: replaced perimeter `F403DC` fell from its
  QC-qualified >3.20 V cell to 2.755 V while retaining commission profile, and
  exact USB perimeter `F4035C` is commission on `fx-260816-prtrel1-b` at
  2.324 V with good USB but charging disabled. Commission's measured dark-awake
  126-144 mA floor is 3.0-3.5 Ah/day before the listener pixel and ToF, while
  the old image refuses a real cell below 2.5 V. Sample several more exact dark
  perimeter slots for firmware revision, persisted profile, battery voltage,
  supply, charging state/current, and recovery state before applying a cohort
  fix. A default-field OTA does not overwrite persisted profile. Any rescue
  must use declared exact targets, the current guarded recovery artifact,
  positive charge-current/voltage-rise evidence, and deliberate persisted field
  promotion. In the next strong-sun window, keep artistic LED streams stopped;
  collect fresh supply voltage/current and battery current for named perimeter
  slots; then inspect
  P126 orientation, shading/dust, polarity, connectors, and the charger input
  path. Separate insufficient 2 W / 6 Ah energy budget from a shared harvest or
  charge-path fault before another nighttime load test. Include a controlled
  named-perimeter VL53L5CX A/B with identical radio and LED state: read back
  ranging mode and integration time, average battery current while ranging,
  then stop ranging or cut VSQT and repeat. Firmware currently relies on the
  driver's default autonomous mode rather than verifying it; ST's 3.3 V figures
  differ sharply between autonomous 4x4 / 5 Hz / 5 ms (about 19 mW) and
  continuous mode (about 313 mW). **2026-08-28 canopy cross-check:** exact
  outer-ring downlight Ponyta `F2B7DC` was dark in durable PROTECT. Its retained
  entry audit records 3.236 V and persisted commission profile; later correction
  to field did not clear the latch. Good USB produced about +192 to +260 mA with
  no BQ fault, completed the 60-second guarded release, and clean-rebooted into
  LEDS_OFF with healthy MSA311/TMF8820/BMP581. This supports the same
  commission-energy/protection chain outside the perimeter cohort but does not
  close the P126 harvest question. Ben confirmed every canopy/downlight without
  exception has a 15 Ah cell, so correct Ponyta's live persisted 6,000 mAh
  capacity to 15,000 mAh through the declared exact-target configuration path
  before relying on its gauge SOC/capacity telemetry (Ben + field team).
- [ ] **Turn the physical census into a slot-to-MAC dark-matter map.** Current
  visual inventory is 74 canopy, 8 installed trunk, and 24 perimeter; all 24
  perimeters should now contain QC'd cells. Roughly 20 additional trunk lights
  are boxed and batteryless. Give every rig slot a durable label, use addressed
  identify to record its short MAC, and explicitly record empty, batteryless,
  bootloader, and silent slots. Reconcile the expected 106 powered
  fixtures against dashboard identities instead of inferring missing units from
  aggregate counts (Ben + Elliot/field team).

- [~] **Choose the daytime recovery posture after the dark-awake baseline.**
  `B3600` proved that removing the LED load swings the updated awake fleet from
  aggregate discharge to aggregate charge, but battery-only fixtures still draw
  roughly 126-144 mA because ESP-NOW RX stays awake. Decide whether diagnostic
  reachability justifies that floor or whether to use the existing field-profile
  300 s sleep / 15 s listen duty cycle for solar recovery. ADR 0049 keeps
  300/15 as the production default and fixes the RX hold so peer heartbeat,
  choreography, and time traffic cannot defeat it; only accepted operator
  controls hold a fixture awake for ten minutes. A selectable 60 s sleep / 8 s
  listen build-week posture remains open, as do external-INA averages including
  boot overhead for both cadences. Do not call `F1` an isolated 0.2 Hz test: it
  also changes lifecycle and sleep behavior (Ben/Codex).

- [ ] **Morning field repair: inspect every flagged false chandelier.** Ben
  flagged all 11 zero-sensor fixtures in the dashboard. Visual inspection says
  nine are trunk lights and only two remain otherwise unidentified, strongly
  suggesting the trunk assembler omitted the STEMMA cable rather than a class
  algorithm error. Open/reseat one trunk canary, use `P<id>:10` for a cold
  VSQT re-probe, then repair the remainder. Preserve the dashboard flags until
  the physical mapping is written down (Ben + Elliot/field team).
- [ ] **Close the remaining visibly red/non-OTA fixtures after the overnight
  test.** A couple of lanterns still show the prior red posture because they did
  not accept the current OTA. The exact 12-ID observed holdback census remains
  below; do not spend tonight looping retries. Diagnose from fresh power and
  image evidence in the morning (Ben/Codex + field team).
- [x] **Field-validate ADR 0044's TMF presence wipe -- DONE 2026-08-16.** The
  per-zone learned background prevented the occluding rig from acting as a fixed
  threshold trigger, and the field team repeatedly triggered complete-looking
  color wipes. Ben reported that activation was a little difficult but RSSI
  propagation itself was remarkably clean; the team captured video. Telemetry
  observed 60/63 and 57/63 fresh hardened peers change in short windows
  (Ben/Codex + field team).
- [ ] **Tune canopy presence sensitivity from field traces, without restoring a
  fixed-distance trigger.** The initial robust setting requires one confident
  zone to move >=300 mm closer than its learned background for three consecutive
  reports. It passed the rig-occlusion proof but was a little difficult to
  activate. Log one normal walk plus the low-ground broom stress case with zone
  deltas and confidence. Compare one isolated downlight against a close group
  with adjacent TMF emitters ranging; do not attribute the intermittent starts
  to optical crosstalk until that A/B supports it. Then adjust delta,
  consecutive count, or sensor timing while retaining per-zone learning and
  clear-to-rearm hysteresis (ADR 0053; Ben/Codex).
- [ ] **Hardware-validate optional ToF-seeded CA on named canaries.** With one
  sensor-verified downlight and at least two non-sensor CA neighbors, set
  `spark /256 = 0` and prove ToF off stays quiescent, ToF on emits exactly one
  excitation per enter/clear/re-enter, and neighbor state propagates it. Repeat
  for light and knock output; prove a refused physical knock does not suppress
  CA state, the separate color wipe stays absent during the lease, and release
  restores the selected autonomous/listener fallback. Name and account for
  every canary before the fleet-wide program lease (ADR 0053; Ben/Codex).
- [~] **Qualify the perimeter ToF palm-cover easter egg -- CLOUDY-DAYLIGHT USB
  PASS 2026-08-25 on `Magmar [F2BDFC]`.** A synchronized trace separated a
  stable clear 0/16 zones from a palm held 5-10 cm above the sensor at sustained
  15-16/16 valid near zones (about 60-93 mm). Touching/too close can disappear,
  so operator language says hover rather than touch. The >=4-zone, two-report
  edge with four clear reports to re-arm is enabled for opted-in CA/Contagion
  leases. A later live light-mode hover turned Magmar blue, proving one local
  program-5 seed edge. Ordered repeat strains now make a second Color Virus
  edge visible: after fleet promotion, prove exactly one new color wave per
  palm/clear/palm, no held-state repeat, and deterministic convergence after
  two near-simultaneous seeds. Then repeat in direct sun and final installed
  geometry (ADR 0055/0056; Ben/Codex).
- [ ] **Hardware-validate Contagion on a named updated mini-graph.** Prove Color
  Virus exact-target hue propagation/persistence/release and Epidemic infected
  -> immune -> susceptible -> reinfected behavior in light mode first. Verify
  downlight ToF off/on, lease expiry, and one old-image gap. Then pair perimeter
  seed `F2BDFC` with one exact updated, explicitly armed daylight solarnoid and
  prove the perimeter remains a silent relay while the infected downlight
  requests one bounded pulse. Also prove a refused strike still relays state,
  and a newer strain recolors already-infected nodes exactly once without an
  older or equal-losing hue bouncing back (ADR 0055/0056; Ben/Codex).
- [~] **Finish the repeated-strain fixture promotion.** Immutable
  `fx-260826-024e508-p` (binary SHA-256 in LOG) is accepted on 99 of the 101
  eligible fixtures in the 2026-08-25 live census. Protected Magic Wand
  `F40344` was correctly excluded. Do not loop radio retries on the two
  holdbacks: recover low-voltage PROTECT sleeper `9E5AD4` on supervised power
  (**in progress 2026-08-26:** exact USB telemetry on COM105 proved
  `fx-260818-f80f315-b`, persisted commission, 2.851 -> 2.868 V in 93 s at
  about +293 mA with verified 300 mA precharge and no BQ fault; keep attached
  and later prove recovery above the release floor before field promotion),
  and bench-diagnose the already known targeted-maintenance downlink exception
  `9F26D8`; both remain on known-good old images and neither received an upload.
  After each recovery, use this same immutable image and require a fresh exact
  revision beyond the pending-verify gate (Ben/Codex + field team).
- [ ] **Make fleet OTA verification aware of PROTECT sleep cadence.** The
  default 420-second post-upload window expired for five healthy fixtures whose
  field policy sleeps for 900 seconds after the roughly 30-second boot grace.
  Each later supplied a fresh exact-revision `deepsleep` heartbeat, proving no
  rollback. Derive the deadline from reported lifecycle/power tier or make the
  safe default cover one full PROTECT cycle; continue requiring fresh evidence
  rather than accepting cached online state (Ben/Codex).
  **2026-08-27 repeat:** F2B7DC accepted the new artifact and later reported its
  exact revision at 32.096 s uptime, but the host never saw that row within its
  5 s freshness threshold before the fixture resumed 900 s PROTECT sleep. The
  T-Deck intentionally emits a full fleet serial snapshot only every 10 s, so a
  one-heartbeat peer can be invisible to a 5 s verifier even while polling is
  healthy. F2BCF4 and low-voltage F2BEE4 similarly report exact-revision
  `deepsleep` state after their job deadlines. Preserve the fresh-heartbeat rule,
  but forward proof immediately or make the evidence window/cadence coherent and
  keep the verifier alive through one full reported sleep cycle.
- [~] **Validate Magmar-to-old-fleet legacy Contagion fanout.** Source, native
  edge/dedup/class-filter tests, embedded build, and exact flashes pass on
  `Magmar [F2BDFC]` plus T-Deck `8EB508`. The explicit legacy output observes
  only the selected program-5 source and converts its infection edge into one
  proven 40 ms addressed roll over fresh downlights. A 120-second attempted
  live watch ended before Start was confirmed: Magmar remained program 4 and no
  command was sent. Complete one palm edge, verify one roll and no duplicate
  from held state, then Stop; repeat once after clear/re-arm (ADR 0055;
  Ben/Codex).
- [x] **Verify one overnight sleeper resumes autonomous charging after sunrise
  -- DONE 2026-08-17.** Multiple non-USB-roster fixtures reported positive
  battery current after the timed wake, including `9E5A74`, `F3FD60`, `9E5B34`,
  `9E5AE0`, `9F0E5C`, and `F2BE48`. The later dark-awake test moved 27 additional
  stable-input fixtures into net charge. Old-firmware `9E5B44` remains a separate
  swap/bench case at about 2.40 V and 0 mA despite input (Ben/Codex).

- [x] **Promote the BQ-qualified recovery/self-identifying listener artifact --
  DONE 2026-08-16.** Immutable `fx-260817-29ac840-b` (binary SHA-256 in LOG)
  passed healthy `F40364`, low-USB `9F268C`, the remaining qualified recovery
  cohort, and explicit awake/sleeper batches. Both CoreS3 bridges were updated;
  the final observed census was 74 exact-revision fixtures of 86 seen. Tag and
  returned LED-state telemetry passed end to end. Twelve named holdbacks remain
  below; the artifact promotion itself is complete (Ben/Codex + field observers).

- [x] **Retire the previous universal-default rollout -- SUPERSEDED
  2026-08-16.** The 37-fixture `fx-260816-19c6bbb-b` wave was replaced by the
  accepted self-identifying/recovery artifact above. Previously problematic
  `F2BE6C`, `F2BE20`, and the named powered recovery cohort now hold the new
  exact revision. Use the current holdback item below, not the old artifact
  roster (Ben/Codex + field team).
- [ ] **Close the 12 observed holdbacks from the self-identifying rollout.** Do
  not loop OTA retries. Bench-diagnose self-test rollbacks `9E5B44` and
  `F40424`; recover maintenance entry on `9E5A84`, `9F26D8`, `F2BCF4`, and
  legacy `F3FD88`; provide supervised external power before attempting
  `F2BE08`, `F3FD28`, `F401DC`, `F40308`, `F40314`, or `F4035C`. Re-census for
  production fixtures not among the 86 IDs seen in this session (Ben/Codex +
  field team).
- [ ] **Close the three hardened-presence holdbacks.** Do not loop OTA retries.
  `9E5AE0` timed out while updating from the eager presence image and was parked
  for one hour; `9E5B34` was not discovered and remains on the eager image;
  `9F2638` was not discovered and remains on `fx-260817-29ac840-b`. The latter
  two were about 2.85/2.89 V and naturally sleeping. Recover on supervised power
  in the morning and upload the exact `fx-260817-ec7f28d-b` binary (Ben/Codex +
  field team).
- [ ] **Reseat the 11 false-chandelier STEMMA chains.** No chandelier fixture is
  powered, so sensor bits 0 on `9E5AB0`, `9F0E30`, `9F26B4`, `F2BE1C`,
  `F2BE3C`, `F2BE6C`, `F2BEF4`, `F2BF90`, `F3FD50`, `F402A8`, and `F40310`
  means the full chain was dark at boot. `9E5AB0` stayed at zero after both a
  1-second and a 10-second addressed VSQT-off/deep-sleep re-probe. Physically
  reseat/inspect one canary, then issue `P<id>:10`; a recovered downlight should
  report TMF/MSA sensor bits, change from diamond to circle, and use warm white
  (Ben + field team).
- [ ] **Explain and harden `F2BE20`'s post-valid app-slot fallback.** It twice
  reported the new `app1` as `valid` with pending false, but after the first
  later power-state reboot ran old undefined `app0`. The 2026-08-16 universal
  artifact attempt additionally timed out, ended in a task-watchdog reset, and
  retained the prior valid image. Capture the next deep-sleep wake, check
  OTA-data/boot selection, surface every OTA update error and return code from
  `esp_ota_mark_app_valid_cancel_rollback()`, and do not count an upload ACK as
  acceptance (Ben/Codex).
- [ ] **Diagnose `F2BE6C` fixture OTA HTTP 500.** The universal-artifact parallel
  attempt lost its response; a later identity-verified sequential attempt
  explicitly returned HTTP 500 and the fixture resumed its old valid image.
  Preserve it on that image until the update error body/serial reason and
  partition state are captured; do not loop retries (Ben/Codex).
- [x] **Raise and verify the deeply depleted solar-recovery current -- DONE
  2026-08-16.** Ben selected 300 mA. Corrected firmware uses a full 16-bit
  REG0x10 transaction, exposes target/readback/raw telemetry, and makes matching
  readback an OTA-validity condition. Two low-VBAT canaries and four additional
  critical fixtures passed; solar input rose from roughly 0.45-0.52 W to about
  1.5 W when available and battery current held near 300 mA. Keep input DPM,
  thermal/trickle protections, and the hardware fast-charge transition unchanged
  (Ben/Codex).
- [ ] **Recover depleted fixtures that could not accept the fleet OTA.** The
  current no-external-power holdbacks are `F2BE08`, `F3FD28`, `F401DC`,
  `F40308`, `F40314`, and `F4035C`. **`F4035C` update 2026-08-26:** exact USB
  telemetry on COM155 proved old `fx-260816-prtrel1-b`, perimeter class, 6 Ah,
  durable PROTECT stage 4, 2.324 V cell, and good 4.883 V input, but charging
  disabled and 0 mA battery current because that pre-ADR-0042 image rejects a
  cell below 2.5 V. Do not assume it is recovering while merely plugged in;
  use the supervised USB flash path to a current guarded-recovery fleet image
  and require recovery-active/charging-enabled, positive current, and voltage
  rise before release. Use supervised USB power and require fresh
  voltage/current ride-through before flashing the exact fleet artifact. The
  target-only `fx-260816-625fab1-t` served its canary purpose and must never be
  uploaded to another MAC; `F2BFE0` is back on the fleet image (Ben + field
  team).
- [x] **Fix dashboard firmware-revision freshness semantics -- DONE
  2026-08-17.** Short heartbeats
  update age/uptime but retain the last full-heartbeat firmware string, which
  briefly made a successful `F40384` update look rolled back. The dashboard now
  preserves the rich identity separately and displays its independently aging
  evidence timestamp rather than presenting cached firmware as current-packet
  truth (Ben/Codex).

- [x] **Fix fixture OTA self-test for maintenance-mode verification -- DONE
  2026-08-15.** A healthy
  `fx-260816-railoff-b` OTA on `F40364` entered maintenance with ESP-NOW
  deliberately down, then failed the unconditional `espNowUp/sendOk` predicate
  at t+20 s and rolled back. `otafix1-b` now uses the radio posture appropriate
  to the active mode (ESP-NOW in COMMS; associated maintenance WiFi/HTTP readiness
  in MAINT). Good-image acceptance and distinct `otafail-b` forced rollback both
  passed on battery-backed `F40364`; see LOG (Ben/Codex).
- [x] **Expose OTA partition identity in fixture telemetry -- DONE 2026-08-15.**
  Added running partition label/address and string image state. The canary trace
  explicitly showed accepted `app1`, failing `app0`, then rollback to `app1`.
  Distinct artifact version strings supplied the build identity (Ben/Codex).

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

**2026-07-08: the live procurement record is `ops/PROCUREMENT.md`** (orders ledger,
to-buy queue, lead-time risks). Items below are follow-ups, not the ledger.

- [x] Buy R&D candidate set: PowerFeather, FeatherS2 Neo, Atom Matrix, NeoHEX, Adafruit IS31FL3741 matrix, DFR0559, panels, battery samples (Ben).
- [x] Contact PowerFeather creator re: V2 availability and KiCad files (Ben).
- [ ] Follow up on PowerFeather forum thread if no reply within a few days (Ben).
- [x] Confirm whether Elecrow boards are V2 or V1 on arrival -- **DONE 2026-06-02**: V2.R2 confirmed on the bench (Ben).
- [ ] Confirm whether PowerFeather V2 KiCad/Gerbers can be shared or licensed (Ben).
- [x] ~~Call/email BatterySpace re: 18650 LiFePO4~~ -- **SUPERSEDED 2026-07-08**: production cell is the fullbattery 32700 6 Ah (ADR 0025). BatterySpace returns only as the 20 Ah #6832 candidate below (Ben).
- [x] ~~Buy alternate LiFePO4 18650/26650 sources~~ -- **SUPERSEDED** by ADR 0025 (175x 32700 bought) (Ben).
- [x] ~~Confirm Elecrow batch-2 invoices AND ships 2026-07-10~~ -- **ORDERED
  2026-07-09 as 90 boards** ($3,494.24 incl. s&h/bank fee/tariff); spares risk
  resolved (158 production boards total) (Ben).
- [ ] Track pf-batch-2 (90 boards) CN transit; chase the rep if no tracking by
  ~07-16 -- must land before the ~Jul 20-31 TN trip / Aug 1 parts-on-hand line (Ben).
- [x] ~~Place the JST-XH harness order~~ -- **DONE ~2026-07-12/13, in deliberate
  abundance across multiple vendors** (lead-time hedge): 300x 10 cm red/black
  (Keszoox) + 1,800x multi-length/color (AliExpress) + 160x 5-pin Y-splitters +
  60x PH pigtails + receptacle/header smalls, ~$575 total (Ben).
- [x] ~~Buy hat enclosures~~ (was implicit in the hat plan) -- **BOUGHT
  ~2026-07-12/13: 172x COTS enclosures + screws** ($5,306.50; 111 large for
  downlights, 61 small for perimeter/candidate trunk-light use; 22 to TN,
  150 to CA). Record vendor/part details in `ops/PROCUREMENT.md` (TBC) (Ben).
- [ ] Confirm Polycase counts on receipt (current mapping, ADR 0032: LARGE ->
  72 downlights; SMALL -> 24 perimeter + up to about 16 trunk lights, leaving
  healthy inventory headroom). Set aside the 2 transparent-lid demo units
  (1 large + 1 small) for show-and-tell (Ben + Steve).
- [ ] **Lock the trunk-light build** for about 16 fixtures: compare 4 W RGBW with
  the smaller lensed 3 W RGB module for throw, appearance, draw, heat, and mounting;
  then select the power, enclosure, optic protection, and attachment hardware
  (Steve + Ben).
- [ ] Receive + count the 2026-07-07 orders (MSA311/STEMMA, VL53L5CX, ToF covers, TMF8820-mini, 100x 6 Ah) as they land; update `ops/PROCUREMENT.md` statuses (Ben).
- [ ] Buy JST-XH right-angle headers + pre-crimped harness set (LED/battery wiring, ADR 0029 fat conductors) once counts firm (Ben).
- [x] ~~Buy Grove breakout(s) for the HEX HY2.0 connector adaptation~~ -- **DONE,
  twice over: 70x RobotShop 2026-06-18 (shipped to Steve, order recovered from
  memory 07-12) + 55x Electromaker 2026-07-10** ($64.86 + $85.26; 125 total) (Ben).
- [x] ~~Buy USB cabling + panel-mount USB-C ports~~ -- **ORDERED 2026-07-10, and
  bigger than planned: 150x waterproof panel-mount USB-C extension cables** ($540
  portion of an $860.34 Adafruit order) -- the rescue/charge port goes on EVERY
  fixture, not just solar-free classes (Ben).
- [x] ~~Order the RGBW top-up~~ -- **DONE 2026-07-10: 50 more** ($247.50, same
  order) -- 150 RGBW total, spares healthy at any chandelier mix (Ben).
- [ ] Hat design: gasketed USB-C port cutout on ALL four hat variants (part on
  hand once the 07-10 order lands) (Steve).
- [ ] Source ~100 JST 2-pin Y-cables (~$0.50 each found; verify quantity availability) -- CONDITIONAL on the RGBW VBAT-feed decision (Ben).
- [ ] Commit `enclosure/references/DOWN LIGHTS DRAWINGS.pdf` to the repo, or re-point the three references to its actual home (Ben/Steve).

## Camp network and field infrastructure

- [ ] **Configure the Beryl AX on arrival and verify channel 11 over the air
  (ADR 0036).** 2.4 GHz fixed to channel 11, 20 MHz width (HT20), WPA2-PSK, on a
  dedicated SSID separate from 5 GHz. Do not accept the config page as proof --
  confirm with a scan (phone analyzer or the bridge `NB_SCANAP` path) that the
  SSID reports `ch=11`. Push laptops and phones to the 5 GHz SSID. Full runbook
  in `docs/howto/CAMP_NETWORK_SETUP.md` (Ben).
- [ ] **Rehearse Starlink bypass mode at home before the event (ADR 0036).**
  Leaving bypass mode requires a factory reset, so the undo must be known rather
  than discovered in the field. **Dish generation resolved 2026-08-15: Gen 3 +
  Gen 4 on hand (possibly all Gen 4), Ethernet built in, no Starlink Ethernet
  Adapter needed.** Remaining: confirm the port physically on the unit that
  travels and that bypass mode is present in its app settings (Ben).
- [ ] **Prove simultaneous WiFi-plus-mesh coexistence on the pinned AP.**
  Associate one device to the channel-11 SSID while a fixture beacons and
  confirm ESP-NOW RX continues both directions for at least an hour. Then
  deliberately set the AP to channel 1 and confirm the guard behavior: WiFi
  dropped, mesh retained, mismatch surfaced (Ben/Claude).
- [ ] **Implement the ADR 0036 channel guard in any firmware that associates
  while using ESP-NOW.** After STA association, read the actual operating
  channel; on mismatch with the compiled mesh channel, drop the association and
  keep the mesh, then surface it on display or serial. Does **not** apply to
  fixtures in OTA maintenance mode, which have already left ESP-NOW by design.
  No current firmware needs this yet -- it lands with the first
  mesh-plus-internet device (Ben/Claude).
- [ ] **Run one parallel shared-WiFi OTA over the Beryl**, not just a house
  network, using the normal targeted `U<id>` + `field_cycle_ota.py` path. Do not
  build or use the deprecated per-board maintenance-AP fallback (Ben/Claude).
- [ ] **Measure the Beryl's actual draw** powered the way it will actually be
  powered (USB-C off the camp battery, not a wall wart) and record it against the
  camp energy budget. Nominal is about 5 W; confirm rather than assume (Ben).
- [~] **Hardware-prove distinct camp and art-site fixture maintenance profiles
  (ADR 0066).** The one-virtual-SSID requirement is retired: both credential
  pairs are compiled from a gitignored local header, and maintenance chooses a
  visible known AP within one bounded join budget. The image has been promoted
  over the established network; associate one exact fixture through the second
  site AP, verify identity/telemetry, resume COMMS, and retain no secret in the
  log or job ledger. ADR 0036 channel 11 still applies to devices using WiFi and
  ESP-NOW simultaneously, not fixtures that already left the mesh for OTA
  (Ben/Codex).
- [ ] **Claude mesh bridge handheld -- decide whether to build at all (ADR 0037).**
  Direction and corrected design brief are recorded in
  `docs/research/CLAUDE_MESH_BRIDGE_DESIGN_2026-08-15.md`; nothing is committed.
  **Hardware is on hand (2026-08-15): 2x LilyGO T-Deck Plus (LCD) + 1x M5Stack
  Cardputer ADV.** Board is settled -- T-Deck Plus first (two units means a
  spare, plus the larger display and better keyboard), Cardputer ADV port after,
  behind the display/input HAL. Do **not** port from T-Deck Pro documentation --
  the Pro is a different device (e-paper, CST328 touch, TCA8418 keypad) whose
  drivers do not transfer. Two milestone-0 measurements before any real build,
  both cheap and both able to kill the concept: put a test pattern on the panel
  in direct sun with sunglasses, and measure continuous-census runtime on the
  2000 mAh cell (this repo measured an always-on ESP-NOW peer at about 168 mA
  radio-RX-dominated, LOG 2026-06-08, so expect well under a night). If runtime
  forces radio duty-cycling, the census must distinguish "node was quiet" from
  "I was not listening". Remaining decision: how
  class/spatial targeting works, since no group addressing exists on the wire
  (start with client-side expansion from the census; a class byte or named
  groups would need their own ADR). Non-negotiables if it is built: include
  `firmware/fixture/src/core/packet.h` rather than defining a second protocol
  header, build the census on the ordinary ESP-NOW receive callback rather than
  promiscuous mode, keep OTA/reboot/profile opcodes off the tool surface, and
  clamp actuator parameters in firmware. Post-2026-event unless re-prioritized
  (Ben).

## COTS bench testing

- [~] **P0: USB-rescue the one known fixture left off the basic-listener image.**
  After the competing OTA writer stopped, `fx-260816-otafix1-b` reached 31 of
  the original 32 eligible fixtures plus newly classified lighting peer
  `9F26D8` (32 of 33 total). Only `F3FD88` remains: its old
  `fixture-2026-08-06.5` image hears maintenance commands but reports
  maintenance-start failure and does not join `Party In The Woods`, consistent
  with credentials baked before the SSID migration. Positively identify it over
  USB, flash the inspected artifact, and require target revision, OTA state
  `valid`, healthy sensors/power, and steady-red listener behavior. Converted
  solarnoid `9E5B8C` remains excluded from the light-only image (Ben/Codex).
- [ ] **Canary automatic recovery from durable PROTECT before fleet OTA.**
  `fx-260816-prtrel1-b` persists the same compound 60-second charge release,
  then software-reboots so the previously parked boot can perform the skipped
  sensor-domain, class, sensor, and LED initialization. Ordinary battery-backed
  USB boot is now proven on factory-fresh `9F26B0`, `9F266C`, and `9F2724`;
  the latter two also passed TMF/MSA downlight and shared-WiFi endpoint gates.
  Next use a genuinely stage-4 silent fixture to prove
  `PROTECT -> LEDS_OFF -> automatic clean reboot -> red listener` without BOOT,
  erase, or an operator-timed reboot. Do not use physical RESET as the
  `otafix1-b` fallback: `F2BF5C` proved that an unexpected reset from LEDS_OFF
  or DIM can correctly re-enter PROTECT, while a deliberate software reset at
  recovered FULL boots unparked. Prefer USB-installing `prtrel1-b`; enter
  download mode only when normal USB CDC/flashing is unavailable (Ben/Codex).
- [x] **Reconcile the first 2026-08-16 ten-board OTA-bootstrap batch -- DONE.**
  `F3FC9C`, `F4019C`, `F2BE3C`, `F2BEB4`, `F40310`, `F2BD00`, `F3FD50`,
  `F2BE10`, and `F2BE1C` became 6 Ah trunk/uplights. `F2BE64` was a planning
  allocation only and remains role-unassigned. Runtime class probing remains
  automatic for assembled sensor stacks (Ben/Codex).
- [x] **Reconcile the second 2026-08-16 OTA-bootstrap batch -- DONE.**
  `F402B8`, `F2BEF4`, `F2BE6C`, `F3FC8C`, `F2BF90`, `F401CC`, and reused board
  `9F2694` became 6 Ah trunk/uplights. `F2B8DC`, `F2BCE0`, and `F40438` were
  planning allocations only and remain role-unassigned (Ben/Codex).
- [ ] **Finalize the fifty-four-board canopy/extra-candidate allocation.** Batch 1:
  `9F2684`, `9E5AD8`, `9F2688`, `9E5A70`, `9D7884`, `F40380`, `F2BED4`,
  `F2BDC0`, `F402D0`, and `F4044C`. Batch 2: `9E5AD4`, `F2BE70`, `F40174`,
  `F40358`, `F4042C`, `F2BE60`, `F2BE38`, `F40424`, `F2BDB4`, and `F4031C`.
  Batch 3: `9E5A9C`, `9E5954`, `F3FD60`, `9E5B48`, `9F26C0`, `9E5AA0`,
  `9E5B98`, `9E5AE4`, `9F2714`, and `F40330`. Batch 4: `F402C4`, `F403F0`,
  `9F26D0`, `9E5B14`, `9F26A4`, `9F26D4`, `9E5B68`, `9E5AF4`, `9F0E30`, and
  `F2BE94`. Batch 5: `9E5A58`, `9F2780`, `9E5B04`, `9F0E7C`, `9E5B10`,
  `9E5AE0`, `9E5B00`, `F401A8`, `F40350`, and `F40254`. Final four: `9F2678`,
  `9E5AC8`, `9F26B4`, and `9F2708`. All are OTA-ready with blank roles, AUTO
  class override, and a provisional 6 Ah profile. If assigned to
  large-enclosure inner-ring downlights, change them to 15 Ah before
  battery-backed validation. Under ADR 0067 a sensorless automatic probe reports
  uplight, but that is not an inventory assignment; the registry role remains
  authoritative (Ben/Codex).
- [ ] **Roster the future chandelier PowerFeathers by exact MAC before
  installation (ADR 0067).** Set registry role `chandelier`, persist `O4` /
  `class_ovr=4` through the exact-target commissioning path, verify the sensorless
  chandelier result has no mismatch, and audit the full roster. Clear with `O0`
  before repurposing any rostered board (Ben/Codex).
- [~] **Census and USB-rescue any forgotten boards already inside enclosures.**
  Elliot is flashing some concurrently. Treat these as installed fixtures, use
  the `docs/howto/FIXTURE_USB_RESCUE_HANDOFF.md` installed-battery path, separate
  batches by 6 Ah/15 Ah capacity and physical class, never use bare-board `X`,
  and record every MAC/result so the registry can prove the fleet is complete
  (Ben/Elliot/Codex).
- [ ] **Retire `F2BED4` from Cambium's fallback-bridge references.** The physical
  board has been reflashed as an unassigned canopy candidate and its lighting
  registry bridge role is cleared. Update the Cambium roster/docs so an operator
  does not later expect this board to be available as a serial bridge (Ben/Elliot/Justin).
- [ ] **Add a single-writer interlock for shared-WiFi OTA.** This is distinct
  from deliberately leaseless lighting control. On 2026-08-16 another host
  replaced accepted firmware on `F2BE0C` and captured `9E5A84` while Ben's
  maintenance hail exposed them on the LAN. Immediate runbook: one active OTA
  daemon/operator at a time. Follow-up: choose an authenticated/allowlisted OTA
  owner or another mechanism that prevents a second host from POSTing an image
  to a maintenance fixture; document recovery and takeover semantics before
  the next unattended fleet update (Ben + Elliot + Justin).
- [ ] **Reproduce the fleet bulk-direct color test.** The accepted `F40364`
  canary obeyed targeted direct colors, and the 2026-08-16 fleet obeyed a
  60-second broadcast-identify blue blink, proving radio RX and LED rendering.
  However, a rapid host loop of per-fixture direct blue/off commands was not
  visually observed across the fleet even though the bridge reported every
  transmit successful. Re-test with one sustained canonical multi-entry frame,
  inspect `direct_seen`/`direct_matched` on a maintenance canary, and separate
  host pacing/addressing from interference by the second controller (Ben/Codex).
- [x] **Canary the basic supervised listener before any fleet OTA -- DONE
  2026-08-16.** Candidate
  `fx-260816-f2bb4cd-b`, channel 11, default commission, SHA-256
  `c792d8c28e8a9c57a0e19455394a5b19c161030cdcf016d741001b672797f965`.
  With no bridge lease it must hold steady red at linear level 128. Commands use
  direct linear 0..255 values; stale direct control returns to red within three
  seconds. Gamma, boot salute, supply carousel, identity pop, and local ToF color
  are absent. Hard battery/rail/OTA rollback protection remains. First prove on
  one USB canary: red persists with every bridge/controller stopped; black and
  primary-color commands obey; stopping frames restores red; RMT survives an
  explicit rail off/on cycle. Then OTA one battery-backed canary, wait beyond the
  20-second pending-verify window, verify the exact revision and fresh heartbeat,
  and only then expand to five supervised fixtures. **2026-08-15 USB canary is
  now on the positively identified COM46 / `F40364`: exact revision, healthy
  PowerFeather/battery/charging, channel-11 ESP-NOW, LED rail, three sensors, and
  no pending OTA verify all passed. Steady red after USB removal passed, and a
  handshake-gated targeted Cambium command changed only `F40364` to solid blue
  with bridge TX success. Black, green, blue, dedicated white, stale fallback
  to red, true rail off/on with post-cycle breathing, reset directly to red,
  good-image acceptance, and forced-failure rollback all subsequently passed.
  The accepted fleet artifact is `fx-260816-otafix1-b`; see LOG (Ben/Codex).
- [x] **Fix Cambium one-shot serial command readiness -- DONE 2026-08-15.** The older local CLI
  starts its asynchronous COM connection and can call `send_identify()` before
  the transport is connected. The transport correctly drops stale/not-connected
  frames, but the CLI prints `identify ...` as if it succeeded. Require a bridge
  STATUS handshake before one-shot mutations and surface not-ready/drop as a
  failure. A persistent handshake-gated retry advanced COM43 TX success and
  made the same targeted blue command work. Fixed from Elliot's latest branch in
  Cambium branch `origin/codex/serial-ready-gate`, complete at `078071c`: require
  fresh STATUS, stamp the real bridge ID, wait for the USB write, then hold the
  port until STATUS reports `tx_ok`/`tx_fail`. The committed CLI passed the live
  targeted-blue proof on `F40364`; 349 tests pass, 1 skipped (Ben/Codex; ready
  for Justin/Elliot review).
- [ ] **Make USB target identity a pre-flash interlock.** The first basic-listener
  attempt selected COM43 from Windows arrival time and temporarily overwrote the
  CoreS3 Cambium bridge instead of COM46 / `F40364`; the bridge was rebuilt,
  restored, and verified. Require a matching live board/firmware identity or a
  physical-reset uptime correlation before upload. Never use COM arrival time as
  sufficient identity when more than one USB device is attached (Ben/Codex).
- [ ] **Throttle or latch repeated LED rail-on failures.** When PowerFeather
  initialization is unavailable, the basic listener currently asks for its red
  frame each loop and `renderTick()` retries the impossible rail enable at loop
  speed, flooding serial. Keep the rail parked but retry at a bounded service
  cadence and expose a counter/last result in telemetry (Ben/Codex).
- [~] **P0: reconcile and qualify the accepted commission-listener image before
  broader fleet OTA (ADRs 0039/0040).** Elliot's `Lighting-Controller` at
  `b047986` contains Ben's complete `d4b1405` hardening/RMT fix plus low-red
  listener, identity pulse, and local ToF response. The observed change from
  `.4` to `.2` on `9E5A94` was not A/B rollback: Elliot's separate bridge OTA'd
  the attached fixture. Both materially different builds reported
  `fixture-2026-08-15.4`, so that string is retired and cannot prove source.
  Integrate Elliot's boot-salute work without parallel edits to the same fixture
  files, then build one newly identified listener artifact. Prove command
  override/fallback, fresh ToF enter/leave, OFF/PROTECT rail cut, non-target
  maintenance enter/resume rail cycling, 20-minute census, and the unexplained
  task-watchdog reset before broader promotion (Ben + Elliot + Codex).
- [x] **Move listener selection from `RES_QUIET_AUTONOMY` into runtime
  configuration. DONE 2026-08-25:** the normal listener-capable image now has a
  wire/NVS-stable commission default: listener (unset default), light-only GH
  CA, or strict rails-off dark. Bridge OS type 30 applies one exact target or a
  deterministic fresh-ID walk, until reboot or explicitly persisted. Field
  profile ignores the setting. Native suites and both embedded development
  builds pass; named hardware/NVS validation remains queued above (ADR 0052;
  Ben/Codex).
- [ ] **P0: implement ADR 0040 artifact automation.** Generate
  `fx-YYMMDD-<recipe7>-<variant>` from clean source/config/toolchain inputs,
  write immutable `manifest.json` + SHA-256 beside the binary, refuse revision
  overwrite/dirty fleet builds, and teach USB/OTA tooling to require an explicit
  manifest rather than newest `fixture.ino.bin` (Ben/Codex).
- [ ] **P0: harden and test Cambium OTA completion.** Reject unknown/invalid MACs
  before maintenance broadcast; capture pre-job state; require a fresh post-job
  heartbeat, expected revision, and survival beyond the 20-second A/B verify
  window. Add direct `OtaJobs` tests. The current 30-second cached-online window
  can falsely report success four seconds after upload (Justin/Elliot/Ben).
- [~] **Qualify the optional USB boot salute without making it false authority.**
  Elliot owns the current firmware implementation. Automatic USB-specific salute
  may mean stable boot only; the final "unplug and install" salute should be
  requested by host tooling after artifact/MAC/profile/channel/power/class/sensor
  checks and `ota_pending_verify=false`. Keep it visually distinct from low-red
  listener, identity pulse, and errors (Elliot + Ben).
- [ ] **Give TMF listener presence freshness/debounce semantics.** A successful
  report with no usable target must clear or age out the prior close distance;
  prove a visitor departure returns to low red instead of latching a stale ToF
  sample (Elliot/Ben).
- [ ] **Make leaseless multi-bridge control observable without blocking it.**
  Preserve last-writer-wins artistic control, but give each bridge a stable
  source identity and surface the fixture's latest direct-frame source/age (and
  rapid source changes) in Cambium/the controller. Contention should read as
  "Ben and Elliot are both driving," not as a mysterious LED or firmware fault
  (Justin/Elliot/Ben).
- [ ] **Run and save a full-cadence census before classifying the missing deployed
  lanterns.** Old field firmware uses 300-second ordinary day sleeps and
  900-second PROTECT sleeps. Listen at least 16 minutes without resetting/opening
  the bridge midway. A powered PROTECT peer should still timer-wake and send setup
  heartbeats; a peer still absent is more likely unpowered, in BMS cutoff, or out
  of range. Record every seen ID and last battery/supply/reset fields, then use the
  panel-mount rescue USB only for the residual set (Ben/Codex).
- [ ] **At playa unpack, flag and replace the seven struggling batteries before
  fixture testing:** `9E5A84`, `F2BCF0`, `F2BF60`, `F3FCAC`, `F402A8`,
  `F403DC`, and `F4043C`. The 2026-08-21 worksite pack census observed about
  2.30-3.06 V; `F403DC` was already radio-silent at 2.296 V. After replacement,
  inspect polarity/connectors, confirm 4.6 V VINDPM, and run a fresh battery and
  mesh census before any OTA or lighting load. **`9E5A84` closed 2026-08-25:**
  replacement LFP, qualified USB charge, exact `fx-260818-f80f315-b` app-only
  recovery, restored 15 Ah capacity, three-sensor downlight gate, Party endpoint,
  and channel-11 ESP-NOW resume all passed. Six battery swaps remain. Keep
  `9F26D8` on its known-good old image until its targeted-maintenance downlink is
  understood (Ben/Codex).
- [~] **Qualify a load-armed boot marker before relaxing conservative reset
  escalation.** IMPLEMENTED 2026-08-18 (ADR 0051): durable `load_arm` NVS
  marker written before any LED-rail or solenoid energize, cleared by
  `allLoadsOff` and a 60 s quiet debounce; `bootGuardDecide` now escalates only
  when the marker was set, so panel-first/no-battery/bench power-ordering
  resets preserve the stored stage (native matrix in test_boot_guard). Same
  commit also defers the durable PROTECT persist until the battery is
  corroborated (current / BQ presence test / recovery detection /
  battery-only), closing the floating-BAT 2.5-3.05 V false-latch window, and
  freezes the ladder during an active low-VBAT recovery lane. STILL OPEN
  before this counts as qualified: run the full battery/panel/USB ordering
  matrix on the bench (panel-first, battery-first, USB-mid-show, bare-board
  X-clear) and two deliberate collapse loops: one with the marker armed
  (ladder escalation) and one loads-off (the ADR 0028 rule-4 streak path,
  escalates at 3 consecutive unexpected resets). Also from the audit
  residuals: verify BQ25628E ADC_EN self-clears after a one-shot; observe the
  coulomb-integrator anchor gap across an EMPTY-veto/recovery freeze; confirm
  a rev-1 433 MHz hardware strike (marker disarmed) behaves sanely through a
  brownout (Ben/Codex).

- [~] **Productionize reduced-access Atom Matrix campmate clickers for 2026.**
  Direction: distribute simple Atom Matrix + Atomic Battery Base mini-bridges
  instead of prioritizing 433 MHz receivers this year; keep the wide-input
  dry-contact receiver retrofit as an open fallback. First proof
  `atom-clicker-2026-08-09.1` is USB-flashed on Atom `54AD9C` (COM42), fixed to
  channel 11, target `9E5B8C`, and a 40 ms type-17 strike. Its only runtime
  control is the pressable 5x5 face; it requires release, debounces, rate-limits,
  and exposes no WiFi/OTA/serial/fleet configuration commands. Remaining:
  physically confirm one press -> one strong strike; inventory/source Atom
  Matrix units and 200 mAh Atomic Battery Bases; measure runtime and implement
  button-wake/deep-sleep; define target provisioning, labels, charging, and
  lost-device revocation; add authenticated/authorized strike commands because
  the current ESP-NOW packet is unauthenticated; add honest actuator ACK/feedback;
  and test multi-clicker coexistence, coverage, cooldown, and abuse behavior
  against the production fleet (Ben/Codex).
- [ ] **P0: quarantine the RX480E dock on all fabricated solarnoid v2.0 boards.**
  First bring-up on 2026-08-09 smoked two receivers. The ordered PCB assigns
  HT7550-1 pin 2 to P5V and pin 3 to VBOOST, but Holtek specifies SOT-89
  1=GND, 2=VIN, 3=VOUT. Do not install another receiver. With RECVR empty,
  `+V`-to-`G` measured **11.76 V**, confirming boost-rail overvoltage. Choose and
  qualify an ECO: leave/depopulate U1 for receiver-free production, cross-wire
  VIN/VOUT, or use a separately powered wide-input dry-contact receiver harness
  (KR1201MINI2-V05B is the leading 31 x 14 x 7 mm candidate). Do not use the
  damaged P5V pad as an improvised 12 V source. A new board revision
  must correct the nets and pass loaded 5 V measurement before release. Treat
  the two smoked receivers as quarantined/e-waste, not retest candidates
  (Ben/Codex).
- [ ] **Spot-check the receiver rail on the 24 boosted rev-1 outer-ring
  downlights before lid closure.** These are rev-1 capboards with the external
  approximately 12 V boost spliced into capboard VDC/GND. Rev 1 correctly routes
  its AMS1117-5.0 (unlike the fabricated rev-2 HT7550 fault), and Ben reports the
  populated 433 MHz receiver paths working. On representative units, verify P5V
  remains near 5 V under receiver load and confirm U1 stays acceptably cool in
  an extended powered test; record the receiver current and test duration
  (Ben).
- [x] **P0 before closing the 24 outer-ring downlights: USB-reflash the production
  maintenance credential.** Binary inspection on 2026-08-09 found that the exact
  Aug-8 `fixture-2026-08-08.1` artifact embeds retired `WonkyHouse`, not the
  `BubbyNet` profile previously recorded. Production peers must use the exact
  case-sensitive `Party In The Woods` SSID and will never use WonkyHouse or
  BubbyNet. Rebuild one locked fleet artifact, inspect its strings/flags, then use
  each fixture's already-required final USB pass to flash and verify all 24 before
  lid closure. **DONE 2026-08-10: 24/24 complete** on locked
  `fixture-2026-08-10.1`; all five batches include live Party maintenance
  verification. `9E5B8C` also passed a deliberate rescue-USB reflash with its
  production LFP installed, replacing its temporary solenoid-test image. Future:
  validate one virtual SSID across the BM camp
  and art-site Starlinks when that network exists (Ben/Codex).
- [x] **Harden TMF8820 boot and runtime recovery before lid closure -- DONE
  2026-08-10 in `fixture-2026-08-10.2`.** On
  2026-08-10, `F40268` passed TMF ID detection and driver `begin()` but produced
  zero measurements with 22 timeout/recovery cycles while downstream MSA311 and
  BMP581 stayed healthy; a PowerFeather reset alone restored 223 clean reads with
  no cable movement. At boot, perform a verified VSQT off/on cycle before class
  probe/sensor initialization so USB/OTA resets cannot inherit stale sensor state.
  After a small bounded number of consecutive ranging failures, escalate beyond
  stop/start to one full TMF or VSQT reinitialization, then report degraded rather
  than flapping the shared rail. Update commissioning to retry one reset when
  `tmf8820_present=true` but reads remain zero; reserve cable intervention for an
  absent ID or failure after the reset retry. Keep Wire1 at 100 kHz (Ben/Codex).
  Firmware now verifies a 100 ms VSQT off/on cycle before every non-parked class
  probe, escalates three consecutive TMF failures to one full shared-domain and
  driver rebuild per boot, and reports `tmf_domain_resets`. Native recovery
  policy coverage passes; commissioning gives the present/zero-read signature
  one `S1` rail-reset retry but never retries an absent ID. Hardware proof:
  `F2B7DC` entered OTA on `.1` with TMF present, zero reads, and rising errors;
  the `.2` boot produced 298 clean reads, zero errors, and healthy MSA/BMP. The
  exact artifact OTA-passed all four connected downlights after an LFP was added
  to `F2B7DC` for ride-through; its repeated bare-USB power losses correctly
  triggered A/B rollback rather than leaving a partial image.
- [x] **USB-commission all 24 BMP-equipped outer-ring downlights -- DONE
  2026-08-08:** final units `F2BEA4`, `F40384`, `F2BDB0`, and `9F0E54` pass the
  locked image/configuration and complete sensor gate. `F40384` required a local
  TMF STEMMA connection repair and cold boot, then held zero TMF errors. Registry
  count is 24/24 commissioned outer-ring units; six BMP581s remain spares and
  quarantined `F2BE74` is excluded (Ben/Codex).
- [x] **USB-commission production downlight batch 4 -- DONE 2026-08-08:**
  `F40268`, `F2BF54`, `9E668C`, `F2BE8C`, and replacement `F2BF5C` pass the
  locked image/configuration and three-sensor gate. Original fifth PowerFeather
  `F2BE74` is quarantined after two TMFs/cables reproduced its loaded sensor-rail
  fault; do not install a battery in it. The BMP-equipped outer hanging ring is
  now 20/24 commissioned (Ben/Codex).
- [x] **USB-commission production downlight batch 3 -- DONE 2026-08-08:**
  `F2BEE4`, `9F26AC`, `F2BE0C`, `9F26E4`, and `F3FC90` passed the locked
  image/configuration and three-sensor gate on the first attempt. The
  BMP-equipped outer hanging ring is now 15/24 commissioned (Ben/Codex).
- [x] **USB-commission production downlight batch 2 -- DONE 2026-08-08:**
  `9F26BC`, `9E5A84`, `9E5B8C`, `F2BE20`, and `F2BF8C` pass the same locked
  image/configuration and three-sensor gate as batch 1. A replacement STEMMA
  cable restored `9E5A84`'s initially failing TMF8820 without reflashing. The
  BMP-equipped outer hanging ring is now 10/24 commissioned (Ben/Codex).
- [x] **USB-commission the first five production downlights -- DONE
  2026-08-08:** `F40364`, `F2B7DC`, `9E5A94`, `9F275C`, and `F2BE48` all pass
  exact-artifact upload and sustained serial acceptance on
  `fixture-2026-08-08.1`, production/channel 11, Generic_LFP 15,000 mAh,
  2,000 mA charge limit, and 4.6 V VINDPM. Each auto-classified as a
  downlight and returned healthy MSA311, TMF8820, and BMP581 samples with zero
  TMF errors. The artifact credential was later proven to be `WonkyHouse`, not
  the `BubbyNet` label recorded at commissioning; correction is queued above.
  Evidence and artifact hash are recorded in `LOG.md` (Ben/Codex).
- [x] **Qualify 24-board production USB commissioning through the two powered
  Sabrent hubs -- DONE 2026-07-27:** Windows enumerated 24 unique PowerFeathers
  simultaneously through the Anker USB-C adapter. All 24 are registered and pass
  ESP32-S3/8 MB flash/2 MB PSRAM preflight, exact-artifact USB upload, PowerFeather
  controller telemetry, bare-board charging-off safety,
  live 6 Ah LFP profile, and BubbyNet OTA/resume. A post-commission hold/census kept
  all 24 present with zero present USB devices in an error state. This qualifies one
  24-fixture bare-board intake batch. The completeness follow-up also enumerated,
  registered, and commissioned all 26 hub ports with zero present USB errors.
  Two independent 12-way upload/serial runs and one 12-way upload plus four-wide WiFi
  run passed; 12 flash workers / 4 WiFi workers is the qualified production setting.
  This does not qualify simultaneous battery/LED loading. **TN credential migration
  DONE 2026-07-27:** all 26 passed USB flash + serial verification on uniform peer
  `net-bench-2026-07-27.3`, WonkyHouse profile, channel 11, and guarded D7 support.
  WonkyHouse was not visible from the CA bench, so all 26 initially remained
  `ota_verified=false`. **FIRST TN NETWORK PASS 2026-07-29:** enclosed peer `F2BFA0`
  was heard over ESP-NOW, targeted into maintenance, joined WonkyHouse at observed
  DHCP address `10.0.0.200`, served matching `/telemetry`, accepted `/resume`, and
  rejoined ESP-NOW. Its registry row is now verified; repeat in sensible batches for
  the other 25 as they are assembled (Ben/Codex).
- [~] **Qualify hub-powered indoor VDC operation before enclosure/solar availability:**
  use a USB-A-to-C cable from a switched Sabrent port into the existing female
  USB-C-to-XH breakout, with only V+ -> VDC and GND -> GND. With all power off,
  confirm XH polarity and about 5 V before connecting; install the already-profiled
  LFP battery only while unpowered. Start with one board at `maintain_v=4.6` and the
  ADR 0033 2 A battery-side ceiling, verify `supply_good`, input voltage/current,
  `battery_present`, and `charging_enabled`, then scale 1 -> 4 -> 12 -> 26 while
  checking hub/cable temperature and brownouts. Do not override source detection or
  IINDPM beyond the measured per-port capability, and avoid a high LED load until
  that capability is measured. This validates indoor
  power/charging and fleet firmware, not panel MPP, shade, dusk, or energy harvest
  (Ben/Codex). **ONE-BOARD PASS 2026-07-27:** `F4044C` ran with PowerFeather USB
  disconnected and hub power entering VDC/GND. Telemetry showed a present 3.39 V
  6 Ah LFP, charging enabled, about 4.80 V / 472-474 mA input, and `supply_good`.
  Targeted shared-WiFi OTA completed in 4.03 s with no button press, and the guarded
  D7 solenoid path recorded two 40 ms strikes while the supply remained good. Ben
  confirmed both produced physical kicks. Remaining: scale 1 -> 4 -> 12 -> 26,
  inspect temperatures/brownouts, and measure the 10-port hub's per-port margin.
- [~] **Finish DFR0991 local-solenoid-button validation on P126 `9F2690`:** firmware
  support is OTA-deployed in `net-bench-2026-07-16.3`, but the first post-OTA probe
  reported `present=false`/address 0. Verify the Gravity PH2.0-to-STEMMA adapter has
  3V3/GND/SDA/SCL in the correct order and is seated on the PowerFeather STEMMA/Wire1
  connector. Yellow -> SCL and blue -> SDA are confirmed; next measure red/black at
  the button while VSQT is awake and inspect the female Dupont contacts. **Power is
  now confirmed at 3.3 V on the button board.** One 3.33 V `brownout` occurred during
  the post-reseat maintenance catch but likely came from accidentally bridging 3V3/GND
  with meter probes. Next, with power removed, continuity-check yellow -> C/SCL and
  blue -> D/SDA and verify neither data line is shorted. Then wake/reset the peer,
  confirm telemetry detects `0x23`-`0x2A`, and
  physically validate one 40 ms D7 strike per press/release. Do not claim the I2C
  trigger proven until that press test passes (Ben/Codex).
  **2026-07-16 `.4` diagnostic:** power and continuity pass; the module ACKs at `0x2A`
  (`ack_mask=0x80`) but returns PID `0x0000` after seven delayed probes instead of
  `0x43DF`. Next measure INT-to-GND at 3.3 V idle/pressed. If INT toggles, wire it to a
  confirmed-free 3.3 V GPIO and use a debounced digital trigger; if not, isolate/test
  or replace the module. Do not apply 5 V to the live ESP32 I2C/INT lines.
  **TN observation 2026-07-29:** enclosed `F2BFA0` also has the baseline `0x2A` /
  PID `0x0000` probe result seen in bare-board fleet records, so that result does
  not prove an external DFR0991 is attached. Remote D7 strike and USER/GPIO0 paths
  are healthy, but no supported external I2C button was detected. Keep the original
  `9F2690` module/INT diagnosis separate.
- [~] **Physically validate the SparkFun PRT-27576 Qwiic Navigation Switch DOWN
  trigger on P126 `9F2690`:** opt-in `--solenoid-d7` firmware probes PCA9554 addresses
  `0x20`-`0x27` read-only, accepts the expected five input/non-inverted switch bits,
  and maps debounced active-LOW DOWN/GPIO1 to the normal guarded 40 ms strike. Confirm
  telemetry presence/address, one strike per DOWN press/release, no repeats while
  held, maintenance suppression, and normal field-cycle sleep/rejoin. INT is
  deliberately unused, so this is an awake-only trigger; USER remains the wake path.
  (Ben/Codex).

- [x] **Make field logging outage-safe -- DONE 2026-07-17:** `net_bench_log.py` now
  exclusive-creates by default and refuses an existing output before binding/listening.
  `--append` validates the first/last JSON rows, inherits the original run identity, and
  writes a numbered resume boundary; `--overwrite` is the only destructive mode. Every
  new data row carries its segment index/start timestamp. Five focused tests cover
  create/refuse/resume/malformed-tail/overwrite behavior. A fresh seven-day P105/P126
  segment is live at `ops/bench/data/ca/2026-07-17-ca-field-cycle-9F26F8-9F2690-
  weather-range-r2.jsonl`; dashboard and logger restart commands are documented in the
  net-bench README. The July 15 Windows Update reboot exposed the old failure (Ben/Codex).
- [ ] **Retain the previous completed field-cycle summary across sunrise:** current
  firmware resets cycle Ah/Wh/min/max at `fieldCycleStartNewCycle()`. If the host is
  absent across dawn, the exact completed-night endpoint is gone by reconnection even
  though RTC counters survived the outage. Snapshot the previous cycle number, phase
  durations, charge/discharge Ah/Wh, min/max VBAT, peak powers, DIM/PROTECT/reset
  outcome, and completion reason before reset; expose it in heartbeat/telemetry with a
  validity marker. Avoid per-second NVS writes (Ben/Codex).
- [~] **Implement and qualify sparse GPS/RTC time anchors plus scheduled shows
  (ADR 0031/0049):** source implementation landed 2026-08-24: T-Deck active-RMC
  GPS publication, read-only DS3231 UTC with OSF refusal, bounded monotonic
  eight-source selection, Black Rock City civil twilight, 30-minute stale
  fallback, and RAM-only T-Deck Auto / Day Dark / Night Show campaigns. Native
  time/parser/schedule tests pass. Production direction is deterministic
  site/date UTC start/stop, not
  panel-current consensus. Four SAM-M8Q modules are already bought as GPS soft anchors
  for absolute UTC, and four Adafruit DS3231 STEMMA modules with backup batteries are
  already bought as initial RTC holdover anchors. Distribute source/age/uncertainty
  over ESP-NOW so all roughly 130 fixtures do not need time hardware. Qualify the SAM-M8Qs
  through the real hat/panel/battery geometry and the DS3231s across temperature,
  reset, and backup-power cases; select final anchor counts; measure acquisition
  energy, RTC drift/backup current, and local-clock holdover; define schedule
  versioning, safe slew/correction, POR and partition recovery, and the
  invalid/stale-time fallback. Remove Starlink/host and pass one compressed plus one
  real overnight scheduled cycle before production use. Keep the SAM-M8Q
  autolocation/true-north benefits in the same anchor plan rather than treating the
  four receivers as separate inventory. Hardware-presence inventory is deployed
  on 60 fixtures using sensor bits 4/5 and dashboard G/R badges. It found GPS
  anchor `F2BDB4` and RTC anchors `9F0E7C` and `9F26C0`; the other five
  purchased boards remain to be found in the held-back or silent population.
  First non-anchor fixture canary `F2BE08` received immutable
  `fx-260825-d374034-b` and remained fresh on that revision past pending verify.
  It retained its existing commission profile, so this proves the integrated
  image/OTA/rejoin path but not autonomous field scheduling. First RTC anchor
  canary Navi `9F0E7C` then received the same exact artifact as the sole OTA
  target and remained fresh beyond pending verify; its new-revision heartbeat
  still reports DS3231 presence. Bridge time-quality diagnostics and guarded
  exact-target DS3231 commissioning then landed in commits `055a58c` and
  `7963816`. Immutable `fx-260825-9ef9d64-b` (binary SHA-256
  `2fc29583a526a995e8bf35c4d4f2b5db3e32c45052a094aa44611baa1f27f9b9`)
  was sole-target OTA-validated on Navi and Zorua `9F26C0`; both survived the
  pending-verify gate, reported present-but-invalid RTCs before the guarded
  write, read back within 2 seconds of fresh T-Deck GPS, and then emitted three
  independent valid/date-valid RTC mesh observations each. Measured live GPS
  disagreement was 624-669 ms. This closes GPS publication, two-anchor RTC
  validity, OSF refusal, guarded initialization, mesh publication, and live
  disagreement visibility. Remaining hardware validation: run compressed
  dusk/dawn plus a real overnight; measure holdover, drift, backup current, and
  acquisition energy; find the other purchased anchor boards; and qualify
  SAM-M8Q acquisition through the actual hat. Fixture GPS reading, peer relay,
  schedule versioning, safe slew/correction, recovery cases, and final anchor
  counts also remain open (Ben/Codex).
- [~] **P105 production-harness A/B of `net-bench-2026-07-13.2`:** remove the external
  panel/battery INAs and instrumented interconnects, but leave firmware, cell, panel,
  load, and thresholds unchanged for the first complete dusk/show/recovery cycle.
  Compare ramp and steady-state sag against the July 13 roughly 500 mA / 2.93 V FULL
  and 300 mA / 3.07-3.09 V DIM points. Confirm at most one DIM retry, durable rail-off
  park after any second reset, and no false sunrise from a missing TSL2591 sample. If a
  reset persists on production wiring, investigate BQ/BATFET disconnect evidence before
  moving thresholds. P105 remains 4.6 V VINDPM; P126 remains 5.8 V (Ben/Codex).

- [ ] **Production low-battery policy hardening and qualification:** execute the plan
  in `docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md`: replace the one-sample +20 mA
  PROTECT release with sustained charger/current/voltage recovery, fix and validate the
  coulomb integrator, derive loaded compensation per production electrical class and
  cold boundary, run the deterministic POR/OTA/I2C/transient matrix, and complete a
  multi-day field acceptance cycle. Preserve default-off rail sequencing, durable
  FULL/DIM/PROTECT stages, and the one bounded DIM retry (Ben/Codex).

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
- [ ] Bracket the true low-VBAT OTA boundary on the current shared-WiFi path with
  historical confounders removed. Use known-good WiFi secrets, no deprecated
  `NB_MAINT_AP` images, targeted `U<id>` / `field_cycle_ota.py` maintenance discovery,
  and explicit pre/post voltage plus supply state. Record separate brackets for:
  battery-only/no-supply, solar/VDC-assisted, and USB-assisted. Current clean
  successes: about 3.10 V loaded battery-only, 2.901 V solar-assisted, and 2.496 V
  USB-assisted. Current lower-voltage "failures" around 2.57 V, 2.95 V, and
  3.02-3.03 V are pre-upload/wrong-path/stale-secret/AP-contaminated data, not clean
  OTA voltage cutoffs (Ben/Codex).
- [x] Add a targeted shared-WiFi maintenance command (`U<id>` or dashboard peer action)
  so a single-peer OTA does not pull every awake peer off ESP-NOW. **DONE 2026-06-30
  in `net-bench-2026-06-30.6`:** the bridge accepts `U9E5AB8`-style sustained targeted
  maintenance, peers handle `NB_TARGET_ENTER_MAINT`, and the dashboard `Peer maint`
  button sends `U<selected-id>`. Bare `U` remains available for deliberate fleet wake
  and for first-hop migration of older peers that cannot parse the targeted packet
  yet (Ben/Codex).
- [x] Harden the targeted `U<id>` OTA workflow so a fresh image cannot be immediately
  recaptured by the still-sustained maintenance command after reboot. Candidate fixes:
  host-side OTA helper waits for the 35 s `U` window to expire before upload, or firmware
  adds a one-shot targeted-maintenance guard after a software OTA reset. Gotcha observed
  2026-07-03 on `9F26F8`: OTA succeeded, first heartbeat appeared, then the peer was
  caught back into maintenance by the command tail. **DONE 2026-07-05 for the recommended
  scripted path:** `ops/bench/field_cycle_ota.py` waits out the 35 s targeted-maintenance
  tail before invoking `net_bench_ota.py --reboot comms`; direct manual dashboard OTAs
  should use the helper or wait out the tail by hand (Ben/Codex).
- [x] Add a host-side maintenance discovery helper for targeted sleeping-peer OTA:
  after sending `U<id>` through the bridge, scan the shared-WiFi subnet for `/telemetry`
  with the matching `fixture_id`. Once a peer leaves ESP-NOW for WiFi maintenance, the
  dashboard cannot learn or display its IP through the serial bridge; this added manual
  scan step was needed for the 2026-07-05 `9F26F8` v5 OTA. **DONE 2026-07-05:**
  `ops/bench/field_cycle_ota.py` scans auto-discovered local /24s plus `192.168.4.0/24`
  and matches `/telemetry` by `fixture_id` before upload (Ben/Codex).
- [ ] After a few more bench/field passes, extract the reusable OTA workflow primitives
  from `ops/bench/field_cycle_ota.py` into a small shared module for future firmware and
  deployment tooling: targeted maintenance command retry, fixture-ID `/telemetry`
  discovery, command-tail wait, OTA upload invocation, and post-reboot rejoin
  verification. Keep `field_cycle_ota.py` field-cycle-specific until production
  software needs the shared path (Ben/Codex).
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
- [ ] Test-print the 50 mm bamboo-leaf aperture in `enclosure/gobo-templates/`, photograph
  its projection through the lantern rig, and widen slots if the slicer closes them (Steve).
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

- [ ] **Bench-measure BQ25628E VBAT_LOWV (the precharge->fast-charge knee) and close ADR 0046's REVISIT clause.** Oakland-runnable with a spare PowerFeather, a bench supply, and no cell: feed the BAT pins from the bench supply through a ~1 ohm series resistor (the supply mimics the cell; the resistor keeps the charger's current step visible and bounded), power VBUS from USB, and watch `bq` charge state plus battery current on the serial CLI while stepping the "cell" 2.80 -> 3.20 V in 25 mV holds of ~30 s. The knee is the step where charge current jumps from the 300 mA precharge plateau toward ICHG; sweep down as well to catch the comparator's hysteresis. Record both edges in LOG, then re-derive the ADR 0046 ladder: protect stays ~50 mV above the falling edge; relax toward the knee if it measures below 3.00 V, raise if above (Ben).
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
    - [x] **External ammeter -- BUILT: 4-channel INA219 monitor** (`firmware/ina_monitor/`, Adafruit Metro S3 + 4x SEN0291 @ 0x40/41/44/45, separate-monitor topology; since 2026-07-02 also runs on the KB2040 with optional VEML7700/TSL2591 lux). Reads a board-under-test's current through deep sleep, gauge-independent. **Next:** (a) **fast-sample capture** (raise rate) to nail per-wake energy -- 10 Hz may miss a <100 ms radio-init spike; (b) **calibrate R_shunt** (0.1 ohm provisional; raw shunt_mV logged so recoverable); (c) wire **panel-lead + LED-rail** channels for the full power-flow map; (d) sharpen sleep resolution (drop PGA range); (e) **I2C robustness (2026-07-02): clear a channel's present flag after N consecutive ERRs, and attempt bus recovery (9 SCL pulses + Wire re-init) when the whole bus errors** -- unplugging the INA harness mid-session wedged the bus (SDA held low) and blinded the still-attached VEML until a reboot (LOG 2026-07-02 audit entry) (Ben).
    - [~] **Clean full-sun MPP sweep -- HOT SESSION DONE 2026-06-11** (LOG 06-11 cont. 2; data `2026-06-11-mpp-sweep-hot-pm-*.jsonl` + knee re-sweep): hot panel (~60 deg C back / 68 deg C front IR) optimum **4.6-4.7 V -> 1.73 W BQ-side / 1.91 W panel-INA ground truth = 3.2x the 5.5 V default (0.59 W)**; instability/knee immediately below 4.6 (one real collapse at 4.4 when stepped to from near-idle); **BQ supply telemetry under-reports harvest ~10 % vs the panel-lead INA** (sizing must use panel-side). Fully wireless: TSL2591 (saturated in full sun -> ir-ch1 normalization fallback, works well; diffuser optional), SHT31 panel-back temp, onboard SEN0291s (0x40 panel / 0x45 battery) all in the heartbeat (fw 2026-06-11.2). **Remaining: the cool-AM session** for Vmp(T) -> the fixed-vs-temp-comp-vs-P&O decision. Lessons baked in: anchor at 4.9 not 5.5 (5.5 is load-noise-dominated); run on a hungry battery (<~50 % SOC -- late-session demand-limiting flattens the curve); approach setpoints from above; beware the bright-sun input-latch on connect (see Firmware guard TODO) (Ben).
    - [x] **Voltaic ETFE P105/P126 outdoor MPP comparison** (2026-06-29, Oakland late sun, both panels about 15 deg tilted): P105 5 W best observed around `m46`/`m48`, panel-side INA about 3.8-3.9 W and charger input about 3.47 W; P126 smaller ETFE best around `m58`, panel-side INA about 1.89 W and charger input about 1.66-1.68 W. P126 is proportionally close to nominal/nameplate; P105 is plausible vs datasheet expected Vmp but may be demand-limited by LFP charge acceptance/taper. See LOG 2026-06-29. (Ben/Codex)
    - [~] **P126 production-cabling perimeter/HEX field cycle** -- deployed 2026-07-10 on former speaker board `9E5B0C`: 2 W panel at fixed 5.8 V VINDPM, 6 Ah production LFP, no INAs/Dupont, and three full-bright R/G/B pixels spiraling in/out at symmetric 120-degree offsets. MAX17260 current and onboard mAh/Wh totals are corrected `/1.08` in firmware. Logger: `ops/bench/data/ca/2026-07-10-ca-field-cycle-9E5B0C-p126-production-cabling.jsonl`; consolidated analysis: `docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md`. That original peer was disassembled and retired after a July 13 header-rework hardware failure. Replacement `9F2690` was USB-flashed and safety-verified on `.3` with the same P126 profile; its next phase is the VDC solenoid trial. Treat BQ supply power as end-to-end onboard telemetry, not panel-side ground truth. (Ben/Codex)
      - [x] Quick onboard MPP re-check 2026-07-10: broad optimum at 5.8-6.0 V; 6.0 V showed +3.8% BQ-input W but no battery-current gain, 6.2 V rolled over, and two 5.8 V anchors agreed within 0.4%. Keep the external-INA-qualified 5.8 V fixed setpoint. (Codex)
      - [~] **Fix the nightly show window and active-time integration before sizing:**
        Ben confirmed that the measured roughly 158 mA draw from the three-pixel spiral
        is intentionally representative of a deployed HEX show; do not raise the load
        merely to force a one-night empty. The clean July 11-12 session actually ran
        18:07:33-08:53:54 PDT (14 h 46 min) and logger-time integration gives 2.33 Ah.
        The peer reported only 13.02 h / 2.08 Ah because `fieldCycleIntegrateActive()`
        discards the fractional part of every `dt / 1000` step. Carry milliseconds
        across integrations. **Integrator carry fixed and OTA-deployed to P105
        `9F26F8` in `net-bench-2026-07-14.1`; first live validation passed at RGBW
        turn-on: 178 mAh over 1,535 DRAW seconds = 417.5 mA average versus about
        414-420 mA direct telemetry.** Counters are cycle-total, so subtract the DRAW
        boundary; `field_elapsed_s` alone is phase-local. Separately, the no-lux
        solar-current fallback turns the
        show on well before visual sunset and leaves it on until useful morning input.
        Use a provisional 9-10 h production HEX show window for the next emulation:
        during the Aug 30-Sep 7 event, civil dusk to civil dawn is about 9 h 53 min to
        10 h 15 min at Black Rock Desert. **SEVEN-DAY FOLLOW-UP 2026-07-17 through
        07-24:** the artificial 13-15 h P126 policy consumed about 48.6 Wh while about
        35.0 Wh reached the battery. Normalizing the same week to a scheduled 9-10 h
        show gives roughly 32-36 Wh of load, so the 2 W role was approximately
        break-even, not proven undersized. Future plots must show both as-run and
        schedule-normalized load. **PRODUCTION TRIGGER DECIDED 2026-07-26:** use sparse
        GPS/GNSS plus battery-backed RTC anchors and explicit UTC show windows per
        ADR 0031; implementation/qualification remains open. (Ben/Codex)
      - [ ] Repeat the clean overnight capture after host-power reliability is fixed: the first production-cabling run has a 13 h 04 min laptop-suspend gap (2026-07-10 18:20 -> 2026-07-11 07:25 PDT). Device-retained counters preserve the total, but the overnight time series is missing. (Ben/Codex)
      - [x] **Observe the P126 daily harvest range until this peer is needed elsewhere:**
        leave `net-bench-2026-07-10.1` and fixed 5.8 V in place rather than OTA solely
        to shorten the artificial night. For every additional day, record BQ-input Ah
        and Wh, positive corrected battery Ah/Wh, weather, coverage, and any reset.
        Current complete/provisional weather points are about 1.55 Ah / 9.02 Wh BQ input
        on July 11 and 1.12 Ah / 6.51 Wh through about 18:00 on the overcast July 12.
        The observation ended when Ben disassembled the peer for the next bench on July
        13. The old-board record remains under fixture `9E5B0C`; do not splice replacement
        `9F2690` samples into that fixture history without an explicit run boundary.
        (Ben/Codex)
      - [x] **Paired P105/P126 seven-day weather-range capture -- complete
        2026-07-17 10:18 -> 2026-07-24 10:18 PDT:** the clean 604800 s logger run is
        `ops/bench/data/ca/2026-07-17-ca-field-cycle-9F26F8-9F2690-weather-range-r2.jsonl`.
        Approximate corrected totals were 113 Wh panel input, 94 Wh positive battery
        charge, 102 Wh load, and -8 Wh battery net for P105; P126 measured 52 Wh panel
        input, 35 Wh positive battery charge, 49 Wh load, and -14 Wh battery net.
        Neither peer reported an active BQ fault. The P126 result is an intentionally
        severe 13-15 h show, not a production-sizing result: at its observed load,
        scheduled 9-10 h nights normalize to about 32-36 Wh/week versus about 35 Wh
        charged. Preserve both as-run and schedule-normalized views in future analysis.
        (Ben/Codex)
      - [x] **Literal P105 + production RGBW ceiling run -- complete:** OTA-deployed
        `net-bench-2026-07-14.1` to `9F26F8` on July 14 with fixed 4.6 V P105 policy,
        one rail-fed `NEO_RGBW` pixel on A0/GPIO10, `R=G=B=255`, `W=0`, brightness
        255, and all dusk/dawn/load-protection settings otherwise unchanged. The RGBW
        was installed before dusk and drew about 1.35-1.4 W, or roughly 14-16 Wh on
        the long bench nights. Across the seven-day follow-up, P105 supplied about
        94 Wh of positive battery charge against about 102 Wh of load and reached
        protect after the cloudier deficit. Full RGB all night is therefore a useful
        ceiling test, not yet the production show budget; duty cycle and brightness
        still need to be set bottom-up by fixture role. (Ben/Codex)
    - [ ] **Re-run P105 5 W with a hungry larger LFP**: use the 6-7.2 Ah cell intentionally discharged to roughly the mid-SOC voltage region (about 3.25-3.40 V resting, not 3.55+ V while charging), hold around `m46`/`m48`, and confirm whether panel-side power can climb beyond the 3.8-3.9 W seen with the 2 Ah cell. Goal: separate panel capability from cell IR/CV-taper demand limiting. (Ben)
    - [ ] **Analyze 7200 mAh HEX drawdown run before the next P105 test**: data path
      `ops/bench/data/ca/2026-06-29-ca-lfp-7200-hex-drawdown-9E5AF0.jsonl`; record stop
      condition, delivered mAh, final loaded/resting voltage, and whether the 12 h sleep
      preserved the desired hungry-but-not-precharge state. (Ben/Codex)
    - [~] **Prototype simple production MPPT / hill-climb policy**: full sweep on first good-sun boot, then periodic 3-point perturb around the last best VINDPM (`best - 0.2`, `best`, `best + 0.2 V`) during daylight; skip or de-prioritize when battery voltage/current indicate CV/taper or near-full acceptance. **BUILT 2026-07-06 for field-cycle bench v6, not deployed yet:** `--field-mppt` samples fixed P105 candidates 4.6/4.8/5.0 V during charge wakes after the OTA listen window, logs candidate W and skip/run reasons, and clamps back to 4.6 V before sleep/maintenance unless a future `--field-mppt-hold` build is explicitly chosen. Next: flash matching serial bridge first, then OTA peer, then validate candidate powers/wake cost on a sunny day. (Ben/Codex)
    - [ ] **MPPT decision** -- green-lit to *measure*, not yet to commit. After the clean sweep, choose: better fixed setpoint (~4.8-5.0) / temp-compensated Vmp(T) / software P&O (use `SET_MAINTAIN` to hill-climb `supply_W`). Optimum ~ 4.85 V hot vs 5.5 V cool -> a single fixed point can't be optimal across temp (Ben).
    - [ ] Full **0-100 % capacity** drawdown (USB top-up to full first) + buck-boost efficiency vs VBAT on LFP (needs rail-side metering for the latter -- SEN0291) (Ben).
    - [ ] Combine harvest-at-MPP (Wh/day) + load budget + a chosen LED-show profile -> the cell + panel spec; pair with the bottom-up nightly-budget re-derivation (Field reliability TODO) (Ben).
    - [ ] **Analyze field-cycle v2 multi-day run**:
      `ops/bench/data/ca/2026-07-01-ca-field-cycle-9E5AB8-v2.jsonl`. Check whether
      the 18/37px brightness-128 draw load reaches protect nightly, whether the 3.15 V
      soft floor + 30 s debounce avoids one-sample false cutoffs, and whether measured
      panel Wh/day covers the configured night load. Tune thresholds/load after 2-3
      full cycles. (Ben/Codex)
    - [ ] **Capture one clean 24 h solar-cycle dataset before treating field-cycle data
      as sizing-grade**: corrected JSONL logger, laptop disk headroom, stable panel
      placement, lux sensor aimed consistently, and no manual device moves. Goal is an
      uninterrupted sunrise-to-sunrise file with charge peak, taper/full decision,
      night drawdown, and protect/dim behavior all in one comparable run. (Ben/Codex)
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

## Battery-brownout investigation (see docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md -- RESOLVED + UNIFIED 2026-07-03: mechanism class = power-management-bus signal integrity -> BQ25628E power-path register upsets (BATFET/HIZ) -> instant battery-path loss. June's disturbance source = the IS31 chip on the bus; July's = our 400 kHz bus clock. Rules in POWERFEATHER_NOTES; retro-analysis atop the investigation doc re-grades all hypotheses (H2 connectors retired, H3/H4/H5 dead). Remaining open items below are follow-ups.)

- [x] Characterize exact conditions for VSYS power-on reset on battery -- ~~load-stacking~~ ~~not-reproducible~~ **UPDATED 2026-06-04: brownout CAME BACK** -- board 1 did a 794-reboot loop overnight on battery (poweron, healthy bv 3.24-3.46, all SOC, lightest load, dying at WiFi association). Real + intermittent on board 1 => H2 (marginal connection) strengthened. See doc Status (Ben).
- [x] Repeat brownout characterization on a second board + known-good cell -- DONE (n=3 all stable in short runs); **NOW extending: pristine board 2 multi-hour with fixed guard to see if it loops like board 1 overnight (board-specificity)** (Ben).
- [x] Fix the overnight auto-sleep guard -- RAM coulomb/timer state reset every reboot, so a brownout loop defeated it. Added **NVS-persisted reboot-loop breaker** (`--autosleep`, >=25 sub-survival boots => deep sleep before WiFi); fw power-bench-2026-06-04.1 (Ben).
- [x] ~~Inspect/reflow board 1's battery + VDC solder joints~~ -- **RETIRED
  2026-07-03**: H2 (connection impedance) demoted from leading explanation by the
  July root-cause (power-bus signal integrity -> BQ register upsets; identical
  signature reproduced with soldered welded-tab leads on two boards). Board 1's
  identity was also never durably tracked. See the retro-analysis atop
  BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md (Ben/Claude).
- [x] **Verify the reboot-loop breaker actually fires** -- DONE: board 2 looped and the breaker deep-slept it (validated in the wild) (Ben).
- [x] **FIX loop-breaker brick-risk** -- DONE (fw 2026-06-04.2): (1) **never deep-sleep while external supply is present** (USB/VDC -> stay flashable/recoverable -- the root cause of the stranding); (2) sleep with a **15-min timer wake** (not indefinite); (3) on a timer wake still on battery -> re-sleep, on supply -> run/charge. So plugging USB self-recovers within one wake interval; never bricks. **VALIDATED LIVE 2026-06-04** (3 mAh budget / 60 s wake test): ran on USB w/o sleeping -> coulomb-budget sleep on battery -> 124 s of timer-wake/re-sleep -> recovered (charging, fresh boot) on USB plug, no BOOT+RESET needed. Tuning flags added: `--budget-mah`, `--wake-s` (Ben/Claude).
- [x] ~~Keep a VSYS bulk cap as cheap insurance and bench-characterize it~~ --
  **RETIRED 2026-07-03**: capacitance answers a sag mechanism; the confirmed kill
  class is a power-path switch OPENING (BQ register upset), which no cap prevents.
  Bulk capacitance remains ordinary good design, not a brownout fix (Ben/Claude).
- [ ] Watch for poweron-reset **recurrence in the field** -- through the July lens:
  battery-only `rr=poweron` at healthy voltage = suspect the power-management bus
  first (what shares it, what clocks it), not connectors/cells. Production
  firmware should carry the boot-counter + reset-reason + pre-death-breadcrumb
  telemetry idiom from presence_bench (Ben).
- [x] Distinguish IS31-specific vs any-I2C-device: **NeoDriver (5766, SeeSaw I2C) on the same bus = STABLE** (371 s+, through heavy WiFi) where the IS31 loops in ~1 min => **brownout is IS31-SPECIFIC**, not the bus. NeoDriver+WS2812 is a viable no-solder LED path (Ben).
- [x] ~~Confirm NeoDriver robustness with an hours/overnight run~~ -- **RETIRED
  2026-07-03**: moot; the LED axis went direct-GPIO (ADR 0022) and the standing
  rule is now "nothing optional on the power-management bus" regardless of how
  benign a given device tested (Ben/Claude).
- [x] **Direct-GPIO WS2812/SK6812 validated** (board 2, HEX on A0/GPIO10, off the I2C bus) -- works, brownout-safe by construction, and ~10% MORE efficient than via NeoDriver (no passthrough drop). **Strong candidate for the area/glow role -- but NOT a settled BOM front-runner**: it's roughly tied in viability with the 4 W RGBW point-source, which serves the crisp-gobo role HEX can't, and the efficiency edge is muddied by varying-SOC testbeds. Decide after gobo testing. 3-way plot `led-eff-3way.png` (Ben).
- [ ] **LED bring-up sequencing for production:** WS2812 latch last frame (send explicit all-off to blank); avoid full-white inrush on hot-connect (ramp gently); direct-GPIO's full VCC browns a marginal cell sooner -> cap brightness / healthy pack. **Now quantified (2026-06-11 hex_ramp):** ~350-400 mA LED draw pulls the bench cell to its ~3.0 V brownout edge even at 98% SOC -> production firmware needs a hard current cap = f(brightness x lit-count), scaled to the production cell's IR (the 32700 ~6 Ah cell lifts the ceiling substantially) (Ben).
- [ ] **Qualify the nine-RGB chandelier chain with instruments, not rail
  saturation.** Jimmy's three-controller handoff uses nine RGB modules per
  PowerFeather. Targeted tester `fx-260818-926d4c2-t` keeps the measured
  257 mA/module model and an 800 mA LED budget (nine-pixel full-RGB cap 88/255;
  900 mA remains the supervised diagnostic maximum). With the production battery,
  log input and rail current, PowerFeather 3V3 voltage, last-pixel voltage/color,
  radio activity, resets, and regulator/connector temperature through an
  extended pattern run. Keep the software current cap: the 1 A rating is shared
  by the board, 3V3 header, and VSQT rail, and saturation is not a safe limiter
  (Ben/Codex). The three bare-USB controllers passed exact-artifact boot and
  serial rail off/on recovery; a real nine-module strand and physical USER-button
  press remain the first visual handoff check.
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
- [ ] Investigate overriding BQ25628E input source-detection to beat the roughly
  500 mA USB input cap (bench convenience only; solar unaffected). Do this only
  for a source with known advertised capability, and never exceed the PowerFeather
  input/connector 2 A rating; ADR 0033's 2 A ICHG ceiling does not itself authorize
  more USB input current (Ben).
- [x] ~~Build a SOLID LFP connection / re-run on solid connection~~ -- **SUPERSEDED**: the brownout turned out IS31-specific (its chip on the shared I2C bus), not the battery connection. The H2-marginal-connection thread is closed (Ben).
- [x] Test LFP full-SOC vs low-SOC under identical load (boost-mode hypothesis H3) -- evidence AGAINST H3: boards ran stable in **active boost** at 3.18-3.24 V (the harder regime), so low-LFP/boost is not the brownout cause (Ben).
- [ ] Run ported demo on battery (firmware/powerfeather_demo_port, AP + ~10 Hz) +/- LED; does the reference app reset? (Ben).
- [x] ~~If resets reproduce on a good connection, add a VSYS bulk cap and re-test
  (H4)~~ -- **RETIRED 2026-07-03**: resets DID reproduce on gold-plated
  connections (July) and the mechanism (switch-opening, not sag) is one a cap
  cannot fix. H4 closed (Ben/Claude).
- [ ] Exercise ported demo web UI: connect phone to PowerFeather_Demo AP -> 192.168.1.1 (Ben).
- [ ] Test `VSQT` off-state leakage with IS31FL3741 attached (Ben).
- [ ] Test `VSQT` sleep/wake/reinitialize cycle (Ben).
- [ ] Test panel MPP/VINDPM settings for each panel (Ben).
- [ ] **Qualify the 103AT battery-thermistor path before relying on unattended 2 A
  charging in sealed hats:** attach it physically to each test cell, verify
  telemetry and BQ JEITA suspend/derate behavior at the production temperature
  bounds, and decide the production sensor/harness plan. Charger-die thermal
  regulation does not measure cell temperature (ADR 0033) (Ben).

## Gobo / aesthetic LED testing (led_studio -- `firmware/led_studio/`, merged from hex_studio/rgbw_studio 2026-06-07)

- [x] Build interactive web app to dial in HEX looks (brightness/RGB sliders, shape rings, spiral/orbit/breathe/twinkle, Split-RGB fringing, Freeze+Step, settings readback) -- DONE, validated on hardware (PowerFeather ACM1, HEX pin 10); served at the IP from the boot banner (Ben/Claude).
- [x] Build interactive web app for the **4 W RGBW point source** (`firmware/rgbw_studio/`): R/G/B/W sliders, white/warmth presets + crossfade, hue/breathe/candle/fade animations, settings readback -- DONE, validated on hardware (ACM1, pin 10) (Ben/Claude).
- [x] Run the gobo session on the inverted-lantern rig -- **DONE 2026-06-11, VERDICT: BOTH module types, by role** (LOG 2026-06-12): HEX = animations/Split color-separation, best within ~6 ft (washes out at 10-15 ft); 4 W RGBW = crisp at 15 ft, Venn-diagram color fringing (overlaps mix NEW colors). Fleet = lanterns of both types. HEX looks best at 1-3 px (white or single-channel) -> realistic HEX gobo draw ~0.4-0.6 W battery-side = all-night budget (Ben).
- [x] **Record LED module = BOTH, by role** -- DONE 2026-06-17 via ADR 0022
  (`docs/decisions/0022-mixed-led-fleet-by-role.md`), preserving ADR 0018's IS31
  rejection and direct-GPIO constraint (Ben/Codex).
- [~] Decide HEX/RGBW **type mix and placement** -- **PRODUCTION ALLOCATION UPDATED
  2026-08-06** (ADR 0032 + SYSTEM.md fleet table): RGBW on 72 downlights in three
  rings of 24, HEX on all 24 perimeter hooks, mixed on 18 chandelier lights, and
  about 16 trunk lights trending all RGBW. The full nominal 130 is the target barring
  an unforeseen issue. Remaining LED decision: qualify the smaller lensed 3 W RGB
  trunk variant against 4 W RGBW (Ben + Steve + team).
- [ ] Rename or explicitly map the legacy firmware/manifest `uplight` class to the
  physical trunk-light role without creating a second fixture image (Ben/Codex).
- [ ] **Capture per-look settings**: when a look is a keeper, record the led_studio sliders + the UI Battery line voltage (brightness is SOC-dependent until the 4.2 V boost lands) (Ben + Steve).
- [ ] Compare Steve's **3 flat sample filters** through the rig; note which pattern reads best at the install throw (Ben).
- [ ] Capture ceiling photos per source/filter for the record; fold results into a gobo test write-up (Ben).
- [ ] If the swept-pixel / orbit moving-shadow looks good, decide whether it argues for a small multi-pixel array even in the "point source" role (Ben).
- [x] Re-test the **4 W RGBW** point source the same way -- DONE 2026-06-11 (led_studio dual-board session): point-vs-area settled as complementary, see verdict above (Ben).

## Noisemaker / audio bench

- [~] **Qualify the finalized solarnoid VDC + 22,000 uF daytime-solenoid power
  path:** the first 10,000 uF cap test passed qualitatively on 2026-07-14 and the
  22,000 uF follow-up bought headroom for stronger solenoids; ADR 0030 now fixes the
  production concept at VDC tap + 22,000 uF + solenoid + bulk mallet on large-hat
  downlights. The no-cap P126-panel kick was weak; a 10,000 uF/16 V electrolytic across
  V+/GND at the female-USB-C-to-XH breakout made it dramatically stronger. The leads
  align directly with the breakout holes, so installation was roughly one minute of
  soldering; prototype cost was about $1. Historical stack and design reasoning:
  `docs/research/AUTONOMOUS_DISTRIBUTED_CHOREOGRAPHY_CONCEPT_2026-07-13.md`.
  Remaining: commit and analyze the 22,000 uF/stronger-solenoid bench data; capture
  coil/pulse/VDC droop/recharge and physical-strike data; test sun/
  cloud/shade, P105/P126, charge/taper, hot-plug inrush, repeated strikes, BQ/reset/fault
  behavior, residual energy at dusk and possible bleeder, ESR/tolerance/temp/lifetime,
  strain relief, polarity/keying, and cold-Voc margin. P126 nominal Voc is about 8.59 V;
  measure worst-case cold Voc/tolerance to document margin for the 16 V part.
  Keep the 815-strike-proven 3V3/XH path as fallback (Ben).
- [~] **Finish root-causing and qualify the rev-1 3P boosted-capbank PowerFeather
  reset:** the corrected 2026-08-06 test used no USB and held
  4.914 V at the PowerFeather plus 12.17 V at the bank before the command, but a
  targeted 5 ms D7 pulse immediately restarted `F3FD7C` while visibly moving the
  HS-0730B. **BATTERY A/B STRONGLY NARROWS IT TO SOURCE-PATH TRANSIENT HEADROOM:**
  with the 3.33 V 32700 attached and A4/A5 unchanged, the same 5 ms pulse did not
  reset (uptime 182.3 -> 184.7 s; sequence 179 -> 181) and Ben judged the strike
  stronger. The loaded source already sat at its 4.66 V VINDPM floor. Scope the
  PowerFeather-facing VDC, local 3V3, GND, and D7 during the
  edge; A/B the shared 5 V source branch, control/telemetry harness, boost input
  decoupling, and isolation/ORing. Static checks (D7S 0 V, VSNS 3.00 V, D7 0 V)
  plus the successful battery run with A4/A5 attached make telemetry injection
  unlikely as the primary cause, but do not rule out ground bounce or switching
  noise. **BATTERY-INSTALLED BENCH ENVELOPE PASSED:** single 5-50 ms pulses,
  10x 20 ms at 15 s intervals, and radio-quiet 8/12/20/35/50 ms DMA captures all
  completed without reset/failsafe/overflow or warmth. D7S timing was within
  0.014 ms and robust 2 ms-median bank drop grew to about 1.77 V at 50 ms.
  **PAIRED DIRECT-5-V REFERENCE PASSED:** after discharging and removing the
  boost, the identical five-width sweep again completed cleanly. At 50 ms the
  direct bank fell 5.086 -> 4.800 V (5.63 percent; about 0.083 J net capacitor
  contribution), versus 12.284 -> 10.511 V boosted (14.44 percent; about
  1.193 J), a 14.3x boosted/direct bank-energy ratio. Keep this explicitly
  labeled net capacitor contribution, not total coil or mechanical energy,
  because VDC supplies current concurrently. Raw paired traces live under
  `ops/bench/data/ca/capbank/`.
  **2026-08-09 PHYSICAL RESULT:** the PowerFeather USER button produces a strong
  bounded 40 ms strike, proving the firmware-controlled D7/driver/cap-bank path.
  SW1 originally produced only a tiny strike with RECVR empty and none with the damaged
  receiver installed. Keep receivers out because the separate reversed-U1 fault
  above overvolts them. Firmware edge-detect plus bounded 40 ms takeover is now
  deployed for physical validation; scope SW1 D7 before deciding whether C1B or
  other component tuning is also useful.
  Remaining before production: scope PowerFeather VDC/3V3/GND/D7, test low-SOC
  and hot/solar cases, explicitly deduplicate strike event IDs, and isolate the
  separate USB+VDC/flash-failure mechanism. **HIGH-Z/SW1-TAKEOVER FIRMWARE CANDIDATE OTA-DEPLOYED
  2026-08-09, PARTIALLY VALIDATED:** an armed fixture now releases D7 to
  INPUT/high-Z at idle and after every MCU cutoff so rev-2 SW1 and optional
  RX480E D0 can drive the board's hardware one-shot through its own 10k pulldown;
  disarmed fixtures still clamp D7 LOW. The `Party In The Woods` targeted
  `fixture-2026-08-09.2` image is OTA-flashed on `9E5B8C`; firmware/sensor/bridge
  telemetry and the strong USER strike pass. SW1 requires a released LOW after
  boot, accepts one rising edge, and extends it to the independently bounded
  40 ms MCU pulse; boot-high/stuck-high cannot fire or retrigger. Next physically
  verify the SW1 extension, MCU/external collision
  refusal, timer/failsafe behavior, no reset,
  and return to high-Z. Do not close or choose this over a hardware diode OR until
  those checks pass. Quarantined
  flash-failed boards: `F402F4`, `F402B4`; surviving
  probe board: `F3FD7C` (Ben/Codex).
- [ ] **P0: root-cause PowerFeather V2 VUSB+VDC hot-plug/flash failures before
  deployed USB service is trusted:** two boards (`F402F4`, `F402B4`) developed
  invalid-header loops and invalid/unreadable JEDEC responses when the repeated
  condition of concern was VDC from one powered-hub branch plus native VUSB/data
  from the same hub. Neither had energized 12 V boost exposure. PowerFeather's
  official design explicitly Schottky-ORs VUSB and VDC and permits simultaneous
  use, so do not close this as a generic ground loop without transient evidence.
  Execute
  `docs/tests/POWERFEATHER_V2_DUAL_INPUT_USB_SERVICE_PLAN_2026-08.md`: preserve and
  compare the quarantined boards; probe VUSB/VDC/VS/internal `+3.3VP`/EN/reset and
  both input currents; reproduce first without capboard hardware under current
  limits; then add the Y harness, large bank, boost, telemetry, and strike one at a
  time. **Interim field rule:** use verified data-only/VBUS-blocked USB whenever
  VDC/panel is live; if USB must provide power, disconnect or shade VDC first. A
  switchable service adapter should default to DATA ONLY. Pass requires valid flash
  ID/hash and no rail/reset/thermal fault through >=50 actual hot-panel USB-service
  cycles. **2026-08-06 partial pass:** `F3FD7C` passed 5.0/5.8 V, both insertion
  orders, same-hub inputs, full Y/boost/charged-bank/D7/A4/A5 wiring, and 20/20
  consecutive native-USB reset/flash-ID cycles (`20:4017`, 8 MB) with no physical
  anomaly. Boost+capboard alone was only 0.17 W; the observed approximately 3 W
  was measured PowerFeather battery charging. Cambium has no fixture OTA uploader
  and the concurrent integration flashed only `F3FD88`, so an unlogged fleet OTA
  is not supported by current evidence. Still required: preserve/no-stub-test and
  dump both dead boards, scope rails/EN, use an actual panel, and complete 50
  physical service-cable cycles (Ben/Codex).
- [x] ~~Evaluate lantern noisemaker options on the Metro bench~~ -- **CLOSED
  2026-07-15 (ADR 0030)**: the shootout ended with the solenoid bamboo-strike;
  relay/speaker options are not pursued. `clacker_demo` stays as bench history (Ben).
- [x] **Candidate A: STEMMA speaker #3885 percussion synth -- ABANDONED 2026-07-15
  (ADR 0030): the solenoids strike the bamboo so well the speaker path is dropped.**
  The percussion synth + `speaker_demo` survive as bench/preview instruments only;
  the spare-#3885 buy is cancelled; the listen-test/loudness/RC-filter follow-ups
  below are moot for production. Original item preserved: BENCH APP LIVE 2026-07-07
  (`firmware/speaker_demo/` at `speakerdemo.local`, LOG same date): organic percussion
  (knock/marimba/chime/drip + ripple/grove scenes) instead of the square waves everyone
  hated. Bench unit's trim pot is broken (LOG item 6): solder-bridge the tweezer-found
  pad pair + order 1-2 spare #3885s before any crowd test. Remaining: controlled
  re-listen for residual whine on fw .8 (FIXED speaker mounting -- placement confounded
  tonight's spectra), Ben's listen test, crowd re-test, idle + playing current draw
  (amp quiescent on the 3V3 rail matters for the night budget), speaker-coupled-into-
  lantern acoustics (mounting coupling is first-order; the bamboo tube is a resonator),
  and volume ceiling outdoors. Amp-oscillator beats confirmed on this unit (LOG item 7):
  the PAM8302's free-running oscillator varies per chip, so for the fleet either add an
  inline RC low-pass (~1k + 10 nF) on SIG or evaluate the MAX98357A I2S amp (no pot,
  true DAC path, ~same price, 3 data wires); the dashboard carrier A/B button is the
  per-unit probe (Ben).
- [x] ~~Gather wider noisemaker opinions~~ -- **OVERTAKEN 2026-07-15 (ADR 0030)**:
  the solenoid's physical strikes settled the question on the bench (Ben).
- [~] **Candidate B -> THE noisemaker (DECIDED 2026-07-15, ADR 0030): MOSFET-driver
  + push-pull solenoid mallet** (physically striking
  the bamboo -- the authentic knock the synth imitates). **FIRST BENCH DONE
  2026-07-10** (`firmware/solenoid_demo/`, LOG same date): 815 strikes, no resets,
  no failsafes. **FLEET PARTS ORDERED**: 100x MOSFET drivers (Adafruit 07-10, $345;
  110 total with the 10 prior) + 150x solenoids (75x 3 V + 75x 5 V, AliExpress
  07-10, $319.12). Remaining: strike-power source decision (VDC-tap sweep tooling
  landed 07-11: Y-cable + storage cap vs battery/VS pin -- check strike transients
  don't confuse the BQ input), 3 V vs 5 V voltage A/B, driver control cabling
  (JST PH 3-pin), mallet/mounting design vs the O(1)-ops constraint (ADR 0009),
  and the formal candidate verdict vs the speaker synth. **P126 bright-sun trial
  staged 2026-07-13:** D7/GPIO37 targeted, timer-bounded manual strike firmware and
  dashboard control are USB-flashed and safety-verified on replacement peer `9F2690`;
  the original `9E5B0C` failed during header rework and was retired. **JULY 14 RESULT:**
  the no-cap panel strike was weak; adding 10,000 uF/16 V at the panel adapter produced
  a qualitatively excellent kick with an unexpectedly easy mechanical/solder fit. VDC +
  cap now leads; the candidate verdict itself is DECIDED (ADR 0030) and the
  **"solarnoid" design was finalized around July 24**, including craft-store bulk
  mallets (order TBC). Scope is settled: large-enclosure fixtures/downlights only, with
  surplus drivers expected. **BENCH STATUS 2026-07-16 (Ben-reported; data on the bench
  laptop, commit pending):** 22,000 uF buys headroom for stronger solenoids; the
  solenoid bake-off is mid-flight with the 0730B 6 V / 1 A as the primary candidate
  and the in-transit 3 V / 5 V units potentially returnable. **TRANSIENT QUESTION
  REOPENED 2026-08-06:** the external-boost rev-1 3P configuration power-cycled a
  PowerFeather on a 5 ms strike even with USB absent; the earlier direct-VDC/cap
  result remains valid for that topology but does not clear the boosted Y-cable
  variant. Remaining: boosted-path scope/isolation, bake-off verdict, possible stronger-solenoid
  order/return, driver control cabling, mounting, and daytime gating.
  **JULY 16 LOCAL CONTROL:** corrected
  `net-bench-2026-07-16.2` is OTA-deployed to `9F2690`; one debounced PowerFeather
  USER/GPIO0 press now wakes the peer if needed and requests the same fail-safe 40 ms
  D7 strike, with physical-release re-arm and maintenance suppression. The first `.1`
  implementation woke once but its RTC-retained re-arm state could remain disarmed after
  re-sleep; `.2` removes that state, treats EXT0 wake itself as the one-shot event, and
  re-enables EXT0 before every sleep only when GPIO0 is actually HIGH. Physical repeat
  validation remains: sleep -> press/strike -> re-sleep -> press/strike, plus no held-
  button loop, normal ESP-NOW rejoin, and no BQ/reset fault. The DFR0991 illuminated I2C
  button is an optional awake-mode trigger; its separate active-HIGH INT pin could wake
  an RTC GPIO only if the module remains powered, while current field sleep cuts both
  external 3V3 rails. (Ben/Codex).
- [ ] **Commit the solenoid bake-off experiments from the bench laptop** -- the
  post-07-11 work (22k uF headroom sweeps, stronger-solenoid bake-off) is not in
  the repo; only solenoid_demo/led_sol_bench + the 07-11 VDC sweep are (Ben).
- [ ] **Decide the 3 V/5 V solenoid return** (150 units, $319.12, still in
  transit) once the 0730B verdict lands -- watch the AliExpress return window
  (Ben).

## Audio ingest bench (PÚCA DSP + VideoMic NTG, bought 2026-08-04; setup notes: docs/research/AUDIO_INGEST_NTG_PUCA_2026-08-04.md)

- [ ] **Bench-verify NTG auto-power into the PÚCA's bias-less line-in** -- the NTG
  auto-powers by sensing camera plug-in power, which the line-in doesn't supply.
  Confirm it powers on and stays on (manual power is the expected fallback). Plain
  TRS-TRS cable, not TRRS (Ben).
- [ ] **Gain-staging sweep on the bench**: NTG output knob hot, WM8978 PGA low; HPF
  (75/150 Hz) engaged; record clean levels for the DJ-line and mic paths so playa
  setup is dial-to-numbers (Ben).
- [ ] **Sketch the feature message format** (band energies, envelope, beat flag,
  `source` field for multi-publisher blending) before firmware milestone 2 locks
  the control-plane schema (Ben).

## Presence sensing / interactivity bench (research note: docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md) -- Elliot ask, 2026-06-12

- [x] **Stand up the 4-sensor comparison bench -- DONE 2026-07-02** (LOG same date):
  `firmware/presence_bench/` on the repurposed spare PowerFeather V2, wireless
  dashboard at `presencebench.local` (live thermal heatmap, tap-a-zone multizone ToF
  grids, radar depth strip, browser-side baseline/delta/occlusion-mask/PRESENT
  tiles), `ops/bench/presence_logger.py` JSONL logging with Enter-key ground-truth
  marks. Sensors: MLX90640 (0x33), VL53L5CX 8x8 w/ vendored 2-target driver (0x29),
  TMF8821 (0x41), XM125 (0x52, ships with the Acconeer DISTANCE app). Multi-target
  per zone VERIFIED on hardware (10 desk zones with near+far pairs). Supersedes the
  original "$10 kit" plan for the imaging axis; LD2420 mmWave + IMU remain open
  below (Ben/Claude).
- [ ] **Dashboard eyeball pass + first walk-under session** (Ben): capture a
  baseline, walk under the rig, check the four PRESENT tiles + event-log ordering;
  log a run with presence_logger.py.
- [ ] **Rig session on the actual lantern** (Ben + Steve): hang under the solar
  overhang pointing down, capture baseline -> occlusion hatching over the bamboo
  splay, record **"usable zones X/64 (VL53), Y/9 (TMF)"** -- the self-occlusion
  deliverable -- and check whether occluded zones still range the floor via T1.
- [x] Add the TOF400C/VL53L1X (the ~$3 original primary candidate) as a 5th bench
  sensor -- **DONE 2026-07-02** (`.13` + TCA9548A mux): both 0x29 ToFs behind
  their own mux ports, all five sensors verified streaming (L1X 1612 mm status-0,
  agreeing with the VL53's far targets). Software VL53L5CX address relocation was
  ABANDONED after a reproducible zombie-until-power-cycle (known ST issue; LOG
  cont. 2); the mux is the architecture. XSHUT jumper on A0 retained as the
  no-mux fallback gate (Ben/Claude).
- [ ] Investigate XM125 distance-app decode against the Acconeer A121 register
  spec: peak strengths return a 0xEEEEEE00 sentinel, and peaks 2+ read beyond the
  configured 0.2-5 m window (10.4 m / 31.6 m) -- treat as one bug. Also consider
  reflashing the module with the PRESENCE app (motion-tuned intra/inter scores;
  the distance app reports all static reflectors, which made desk testing
  uncorrelatable with motion) (Ben/Claude).
- [x] **Presence-bench battery-only reboots -- SOLVED 2026-07-03 (LOG 07-02
  cont. 5-10 + 07-03): root cause was OUR 400 kHz Wire1 clock on the shared
  power-management bus** (the "measured exception" to POWERFEATHER_NOTES).
  Controlled A/B: identical firmware at 400 kHz died in seconds on battery; at
  100 kHz the full 5-sensor bench runs indefinitely (7.3 h + 900 s formal
  soaks). Mechanism: corrupted charger (BQ25628E) transactions under WiFi TX
  open the power path (BATFET/HIZ class) -> instant poweron reset; USB immune.
  100 kHz now the compiled default; rules added to POWERFEATHER_NOTES. The
  elimination ladder also formally exonerated: sensors, A0 jumper, 26650
  holder, both cells, both boards, SoftAP beaconing, the TPS631013 crossover
  band (stable at 3.33 V under heavy TX), NVS writes, charge-enable (Ben/Claude).
- [ ] Reboot-hunt residuals: reflash led_studio onto the desk board (r10 still
  queued); sharpie-label both boards (spare = ex-9F2690 master, desk =
  led_studio home); OPTIONAL Test B (400 kHz round-robin from core 1) if the
  core-aggravator question ever matters; custom-PCBA design rule captured in
  POWERFEATHER_NOTES: dedicated I2C bus for charger/gauge (Ben/Claude).
- [ ] **Fleet hygiene: chemistry profile must match the attached cell.** The old
  net_bench master image (Li-ion profile) was found actively overcharging the
  4 Ah LFP toward 4.2 V on USB (real terminal reading 4.19 V; relaxed to plateau
  once charging was disabled). Audit any board that gets a cell: image `--chem`
  vs physical chemistry (Ben).
- [ ] Confirm which cell is attached to the presence-bench PowerFeather; its gauge
  telemetry is inconsistent (bv 4.12 vs 3.68, ma -290 vs 0) and this sketch
  deliberately leaves charging OFF (Ben).
- [x] ~~Order the remaining kit (LD2420/LD2410 mmWave, LD2450, LIS3DH/MPU6050 IMU)~~
  -- **SUPERSEDED 2026-07-08 by the production sensor buy (ADR 0027)**: MSA311 +
  TMF8820-mini + VL53L5CX ordered at fleet scale on 2026-07-07; mmWave dropped
  (continuous-power appetite); fused/other IMUs rejected (per-device cal). LD2450
  remains a possible future choreography experiment, not a fleet part (Ben).
- [ ] **Finish per-class sensor validation** (ADR 0027 + ADR 0034): TMF8820-mini
  downward on all downlights, VL53L5CX outward on perimeter, and BMP581 on the
  complete 24-light outer hanging ring with 6 spares. The first five outer-ring
  downlights passed live MSA311/TMF8820/BMP581 production-firmware sampling on
  2026-08-08. Remaining: physically count the received BMP581 inventory and run
  one downlight-height test on the exact ordered TMF8820-mini part (earlier height
  work used the TMF8821) (Ben).
- [x] ~~Bring up the BMP581 on the STEMMA chain and add temp/pressure to
  maintenance telemetry~~ -- **DONE 2026-07-29 on enclosed `F2BFA0`:** BMP581,
  MSA311, and exact ordered TMF8820-mini all initialized and returned live data
  on the shared 100 kHz bus after targeted OTA. The opt-in diagnostic samples
  every two seconds; it is not yet production energy/timing qualified (Ben/Codex).
- [x] ~~Make a sensor-reactive RGBW proof of concept~~ -- **DONE 2026-07-29 on
  enclosed `F2BFA0`:** LED Studio now offers ToF-depth, relative-tilt, and
  pressure-derived relative-elevation modes plus live sensor readout and manual
  re-zero controls. The TMF filter ignored the repeatable 20-21 mm enclosure
  return and reacted to a usable target around 219 mm (Ben/Codex).
- [ ] Field-tune the LED Studio mappings: ToF near/far endpoints (currently
  120/1200 mm), tilt full-scale (35 deg), and elevation span (+-1.5 m) (Ben).
- [x] ~~Remove LED Studio sensor stalls and expose WiFi health~~ -- **DONE
  2026-07-29 on `F2BFA0`:** replaced the blocking TMF convenience call with a
  cooperative start/process/stop state machine on the main 100 kHz loop, prevented
  overlapping browser polls, and added RSSI/request-latency/TMF-age diagnostics.
  State requests fell from 0.7-1.8 s to 112 ms mean / 204 ms p95; button commands
  averaged 35 ms; 776 TMF frames ran with zero errors/recoveries (Ben/Codex).
- [ ] Guard LED Studio charge enable on confirmed battery presence, matching the
  safer net_bench/sway pattern, before using a sensor-triad build on a bare board
  with external VDC but no cell (Ben/Codex).
- [ ] Add the winning sensor(s) to the net_bench heartbeat (append-only tail, same
  pattern as env/INA) and dashboard for yard/field tuning. Current triad readings
  are available only through maintenance `/telemetry`; interpret/calibrate the
  TMF8820's high-confidence 20 mm near return against the enclosure/window before
  treating it as presence (Claude + whoever's bench).
- [ ] Build, validate, and intentionally deploy a named fleet artifact that
  publishes the ADR 0041 class/render telemetry tail. The 2026-08-16 live fleet
  artifacts omit that tail, so the dashboard correctly leaves their glyphs
  unknown. Follow ADR 0040: build once, record manifest/SHA-256, name explicit
  target short MACs, and require fresh post-OTA revision plus 20-second
  pending-verify survival. This source change alone does not authorize OTA
  (Ben + current firmware operator).
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

- [x] Source a **TPS63802 buck-boost module** -- DONE 2026-07-02: Ben bulk-bought
  Amazon modules pre-jumpered to 4.2 V (Ben).
- [ ] Re-jumper output 3V3 -> 4V2: fully OPEN the 3V3 bridge first (both bridged = wrong
  feedback divider), close 4V2, **meter the output UNLOADED before any pixel sees it**.
  Leave the tiny EN pad untouched (tied to VIN; kill-switch = the GPIO4 3V3 rail). Leave
  the PS pad at default (power-save/PFM = light-load efficiency; flip ONLY if dim
  single-pixel flicker appears) (Steve).
- [ ] Wire: PowerFeather switchable 3V3 header -> module IN; module OUT 4.2 V -> HEX V+;
  common GND; HEX data direct to GPIO10 as usual. led_studio drives it unchanged (Steve).
- [x] **Measure (the decision data): lumens-per-system-watt rail-direct vs boosted
  4.2 V** -- **MEASURED 2026-07-02, decision recorded in ADR 0029 (HEX stays on the
  3V3 rail; boost NOT worth it for the HEX gobo regime).** VEML7700 photopic harness + boost_ab_log.py/
  boost_ab_suite.sh, 4-swap A/B/A/B series, seating error bounded <=2 %. Single white
  px full: +1.6 % light for +60 % LED-branch power (lumens/W ~40 % WORSE); blue single
  +5.1 %, ring1 7 px bri128 +6.9 % -- gain grows with load per dropout physics, but
  Ben's product call is that >1 full-white px washes out the gobo, so the heavy-load
  regime is moot for HEX. The +40-60 % white expectation REFUTED at plateau voltage
  (it assumed the 2.8-2.95 V heavy-load sag, which the single-px look never causes).
  REMAINING (demoted 2026-07-02 after topology correction: hex V+ is the regulated
  3V3 rail, so bare is SOC-invariant by construction until deep discharge): low-SOC
  spot-check = watch for rail droop under load at low VIN only, 10 min on a drained
  cell, unlikely to flip; 4 W RGBW point source is a separate question (Ben/Steve).
- [x] **Redo the boost A/B for the 4 W RGBW point source** -- **CAMPAIGN COMPLETE
  2026-07-02, matrix recorded in ADR 0029 (boost shelved with complete numbers;
  the rail-vs-VBAT production feed decision is OPEN there). r10 residual + the
  feed-decision item carved out below.** Detail: rail-fed variant
  MEASURED 2026-07-02 (gold standard r6, LOG same date): boost = 2.2x clean white
  (1044 vs 470 lux W-full) at ~37 % efficacy tax; rgbwhite rail-walls at bri=128
  (3x replicated); bare rgbwhite-full ~1310 lux is the free bright option and Ben's
  production GO.** **VBAT-fed variant MEASURED 2026-07-02 (r7, LOG same date):
  ~11 % battery-side saving vs two-stage (tax ~37 % -> ~28 % aim-corrected), ESP
  decoupling PROVEN (board draw constant 116-118 mA through a branch collapse), and
  the rgbwhite wall becomes a harness-wiring limit (~0.3 ohm loop) instead of the
  rail regulator.** BOTH variants now characterized; boost stays SHELVED (bare is
  the GO). **MATRIX COMPLETED 2026-07-02 (r8/r9, LOG same date): VBAT-direct + fat
  wire is the production topology with or without boost -- bare-VBAT rgbwhite 1746
  lux (+33 % over rail-fed, no wall), boosted-VBAT-fat 3044 lux rgbwhite / 1016 W-only,
  no wall anywhere on proper wiring; the walls were the rail regulator and harness R,
  never the architecture. LED power must tap DOWNSTREAM of the gauge shunt (header
  tap is coulomb-invisible).** If boost is ever revived: VBAT-fed single conversion
  on the adapter PCB, EN->GPIO + pull-down for software kill (bench module EN is tied
  to VIN = always-live V+; SK6812 latches -- blank before unplugging), and
  connector/trace quality specced (worth ~25 % of top-end light).
  (r10 detail moved to its own item below.) Also note for any "boost for top-end only" revival: the
  efficacy tax is per-lumen and duty-independent (dies at 4.2 V whenever lit), so
  selective-boost needs a bypass path around the TPS63802 (EN-low disconnects the
  output entirely) -- a real adapter-PCB circuit decision (Ben).
  Original design note below stands: the
  PowerFeather 3V3 header is ~1 A-limited (per Ben), so full-power RGBW white is
  supply-limited in BOTH configs off that rail: rail-direct 4 W white already wants
  ~1.2 A at 3.3 V, and a boost fed from the header tops out around 3 W in minus
  conversion loss. Step 0 is characterizing what the rail actually delivers (the HEX
  A/B peaked at 0.21 A; the 2026-06-10 discharge gives one heavier anchor: ~2.97 V at
  the LED at ~290 mA -- LOG 2026-07-02. Also probe Ben's recalled radio-burst-during-
  LED-hogging brownout: rail voltage during WiFi/radio bursts at high LED load). The honest
  full-power comparison is likely rail-direct vs **boost-fed-from-VBAT** (the
  adapter-PCB production topology), battery INA as truth source. W channel is real on
  this module (unlike the RGB-only NeoHEX) -- include W-only and W-vs-RGB-white looks.
  Harness/tooling reuse as-is: boost_ab_log.py + a boost_ab_suite variant with
  RGBW-appropriate looks (Ben).
- [ ] **r10: battery-plane watts for the two uninstrumented configs** -- bare-VBAT-fat
  and boosted-VBAT-fat (the 3044 lux point has NO measured power; current estimate
  ~3.0-4.5 W, ~700-1000 lux/W, estimate-on-estimate). Method that avoids re-poisoning
  the loop: SEN0291 IN+/IN- are SCREW TERMINALS -- clamp the fat wire directly, no
  duponts; shunt adds only 10 mOhm. One ladder per config, update the report's fig 4.
  Requires reflashing led_studio onto the desk board first (Ben).
- [ ] **Firmware count-cap for boosted builds** -- **moot unless the boost is revived
  (ADR 0029 shelved it)**: all-37 full white at a regulated 4.2 V wants ~2 A out
  (~8 W in) = instant brownout -- cap n (or estimated total current) in led_studio
  when V+ is boosted (Steve or Ben).
- [x] ~~Spec the production boost variant on the NeoHEX adapter PCB rev~~ -- **CLOSED
  2026-07-08 as SKIP per ADR 0029** (no boost for either role). The revival spec
  (VBAT-fed single conversion, EN->GPIO + pull-down, fat wiring) is preserved in
  ADR 0029 if a future look needs the ceiling (Ben).
- [ ] **DECIDE the RGBW production feed: 3V3 rail (as wired today) vs VBAT-direct
  (+33 % fringed white, free)** -- before the harness buy, since it forks connector
  set and firmware pinout (ADR 0029 has the full trade). Ben's sketched conversion:
  solder a 4-pin header along {VBAT | EN | VS | D13} pulling VBAT -> V+ and
  D13 -> signal, GND via a cheap JST 2-pin Y-cable (~$0.50) off the GND pin next to
  VDC/solar+ -- needs ~100 Y-cables sourced at quantity, firmware A0 -> D13, and a
  fail-safe redesign (no 3V3-rail shutoff means a stuck-on frame can kill the
  battery -- verify all-off + a default-off switch element per ADR 0013). Reasons
  to stay on the rail: clean W-only is unchanged, the rail cut is a robust hard
  kill, and the rail hookup is easy/robust. Side benefit of converting: frees
  3V3/GND/A0 for a clacker/relay payload (Ben).

## Networking feasibility -- 5x PowerFeather V2 (net_bench, 2026-06-07; de-risked the buy -- fleet now nominally 130, ADR 0032)

See `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` + `firmware/net_bench/`.

- [x] Build the first ESP-NOW firmware + 5-node host harness (broadcast comms, master/peer roles, maintenance-mode WiFi OTA, watchdog, per-source PDR, scale-extrapolation summarizer). Bench-validated on 1 board (Ben/Claude).
- [~] **Flash all 5 boards with `--channel <AP channel>`** (home AP "BubbyNet" = ch 11) and run T0-T7 -- channel MUST match the AP or ESP-NOW silently fails. Partial 2026-06-07/08: master + 3-4 peers ran the matrix (one board never booted); full 5-board pass still open (Ben).
- [x] Run the **rate sweep** (1/2/5/10/20/50 Hz) -- **PASS 2026-06-07** (LOG): >=97 % PDR to 250 pkt/s aggregate, clean knee; ~100-node projection at 1-2 Hz = 98-99 % PDR (Ben).
- [x] **Range** T3 -- **PASS 2026-06-08**: link held through house + yard + oak (~100 steps); solar panel is the main ~20 dB attenuator; obstruction mapping captured (Ben).
- [ ] **Re-run the scale extrapolation at 130 deployed nodes** (it was computed at
  100); optionally include 150 as a conservative inventory-backed stress case, and
  restate the projected PDR honestly (Ben/Claude).
- [x] **Cambium three-fixture acceptance** -- DONE 2026-08-06 on the three Nevada
  City perimeter units: CoreS3 COBS bridge status, 3/3 heartbeat census, roll-call,
  direct program 3 on all nodes, and >3 s silence fallback to autonomous program 1;
  67/67 bridge sends, zero TX failures/CRC errors/RX drops. The old LED Studio
  artifacts and channel 6 were restored afterward, but channel 6 was not a
  presence-bench requirement: it came from one fixture build that omitted the
  channel-11 flag plus the explicit `H6` restore. Follow-up `.5` fixture firmware
  now migrates that known legacy state to the production channel 11; all three
  perimeter units reported channel 11 on hardware (Ben/Codex).
- [x] **CoreS3 audio-reactive three-perimeter demo** -- DONE 2026-08-06 after
  moving Module Audio's physical I2S selector to B (CoreS3): Rode input reached
  RMS 3,423 / 98.7% envelope; F3FD88/F2BE80/F2BFEC all entered direct program 3
  and matched every addressed frame. Stopping the bridge returned all three to
  autonomous program 1 after the three-second stale-frame limit. All fixtures
  acknowledged `N2`, remained on channel 11, and the bridge was left audio-off
  (Ben/Codex).
- [~] **Bring up the received PUCA performance-audio bridge (ADR 0035):** the
  `firmware/puca_bridge/` target now reuses the shared envelope and canonical
  packet definitions, captures/averages stereo, reports peak/clipping, and
  chunks the full sorted census into 18-entry `NB_DIRECT_FRAME` packets at about
  10 Hz. The 2026-08-26 powered-Pod20 pass verified exact USB/MAC/4 MB identity,
  codec + stereo capture, powered knob readings, normal-paw status-only lock,
  channel-11 census, and more-than-18-fixture sender chunking. The accepted
  207 s run reached 70+ eligible fixtures and 8,318/0 successful/failed send
  callbacks with zero audio/I2C/radio queue errors. The 2026-08-27 `0.5.0-dev`
  pass then proved USB bootstrap, no-hold SAFE-IDLE/zero direct frames, Bridge
  OS identity, exact-target shared-WiFi OTA, dark rejoin, and pending-verify
  survival. Next: physical DJ-first boot hold/setup gestures, rollback,
  `/resume`/timeout/broadcast-rejection/no-softAP checks, full knob sweeps,
  exact RODE/DG faceplate route and gain, visible named-
  fixture response plus three-second fallback, mixed HEX/RGBW fidelity, intended-
  placement RF/PDR, overrun/reset, and multi-hour stability. Full checklist:
  `hardware/puca-audio-bridge/README.md` (Ben/Codex).
- [x] Merge/review `codex/cambium-direct-frames` -- DONE 2026-08-06: rebased
  over the 2 A policy, native/build/hardware-smoke verified, and fast-forwarded
  to `beneckart/resonance-lighting` `origin/main` at `d9333ab` (Ben/Codex).
- [x] Publish the companion Cambium integration -- DONE 2026-08-06: forked
  `justinlange/cambium`, published `codex/fleet-130-bench3`, and
  fast-forwarded `beneckart/cambium` `main` to `b071542` (340 passed / 1
  skipped). License remains pending Justin (Ben/Codex).
- [ ] Repeat direct streaming on a mixed HEX/RGBW fleet before production
  rollout (Ben/Codex).
- [ ] **Parallel OTA cycle** on 5 nodes via `net_bench_ota.py` -- confirm 5/5 auto-recover with NO physical button (the field-reset requirement) (Ben).
- [ ] **Rehearse shared-WiFi OTA at tree scale (about 100 awake peers) on the actual
  portable router:** measure maintenance discovery/DHCP success, safe upload
  concurrency, aggregate completion time, reboot/rejoin verification, and retry
  rate. Current approximately 1.02 MB images imply about 2 minutes of pure transfer
  at five jobs, but operational planning should reserve 20-30 minutes until router
  client capacity and the full workflow are measured (Ben/Codex).
- [ ] **Multi-hour battery stability** soak (Li-ion) -- zero unexplained resets, log mAh/h drain (Ben).
- [ ] **Master WiFi+ESP-NOW coexistence** current/stability run (Ben).
- [ ] **RE-VERIFY all battery/stability findings on LFP** once Steve's cell holders/connectors exist -- Li-ion is necessary-not-sufficient (LFP plateau = buck-boost crossover) (Ben).
- [ ] 20+ node confirmation run if the rate knee lands near the production point (Ben).
- [ ] Mock-hat antenna RF with panel/battery installed (Steve; COTS Phase 7).
- [ ] **Lengthen the identify/locate blink** -- 8 s is too short for human-in-the-loop / field use (missed a single blink due to chat latency; live sweeps work but one-at-a-time doesn't). Make it ~30 s, or toggle-until-stop, and add identify-by-specific-ID (Ben).
- [x] **Make battery capacity runtime-settable (NVS), not a build flag** -- DONE 2026-06-28 in `net_bench`: master serial `C<mah>` stores capacity in peer NVS and reboots to apply the gauge model; `G<mA>` stores/applies the charge-current cap live. Build flags remain defaults, not per-board destiny.
- [ ] **Validate fuel-gauge SOC over a real charge/discharge cycle** + confirm the field (sleep + low-load solar charge) anchors the gauge cleanly, unlike the always-pinging bench (the false-low was likely a bench artifact). Production low-battery logic must cross-check voltage (done for the LED) (Ben).
- [x] Promote results into **ADR 0021** -- DONE 2026-06-08: `docs/decisions/0021-powerfeather-v2-feasibility-validated.md` (go; networking + solar + field-OTA validated, open follow-ups listed) (Ben).

## Fixture auto-localization (RSSI + ToF -> CAD registration; sim study 2026-07-12)

See `docs/tests/AUTOLOCATE_RSSI_SIM_FEASIBILITY_2026-07-12.md` + `ops/locate/`.

- [x] **Try an RSSI-only 3D cloud on the Nevada City rig capture -- DONE
  2026-08-18.** The no-roster/no-ToF/no-CAD ordinal solver recovers a stable
  relative latent cloud and improves hidden-link RMSE from 9.99 dB (radio biases
  only) to 6.77 dB in 3D. It does not recover a defensible physical point cloud:
  4D/5D continue improving, the 2D -> 3D rank gain is only 0.008, and no grid is
  visible. Keep the result as topology/initialization evidence, not coordinates.
  Report: `docs/tests/RSSI_ONLY_POINT_CLOUD_2026-08-18.md` (Ben/Codex).
- [ ] **Repeat the RSSI-only survey with all fixtures reporting true window
  aggregates.** Record per-packet/window medians with expected counts and an
  independent second survey after orientation churn. Re-run the dimension sweep;
  require a 3D validation elbow and cross-survey distance stability before
  promoting the cloud beyond topology/initialization use (Ben/Codex).

- [ ] **Small-N real pairwise capture (the calibration gate)**: 10-20 boards in the
  backyard, full pairwise RSSI for ~1 min, solve with `locate_run.py --pairwise`.
  Measures the real sigma_link (playa-band estimate is 2-6 dB, unverified) -- the
  sim verdict is conditional on this number (Ben).
- [ ] **Firmware pairwise neighbor-RSSI dump** emitting the `ops/locate` JSONL
  contract: every device reports per-neighbor median RSSI per window, WITH expected
  packet counts and the on-device censoring-corrected median (reference:
  `locate/rssi.py:_directional_median`; the neighbor table in firmware/ARCHITECTURE.md
  already holds per-neighbor RSSI; bridge collects) (Ben/Claude).
- [ ] **Perimeter ring: 24 lights at R ~6.5 m (DIRECTION, Ben 2026-07-27)** --
  redundancy waived (bridge health visits every few days + swap flow); geometry:
  N >= 2*pi*(R+x)/(0.83x) for VL53L5CX 45-deg FoV -> N=24 closes blind spots at
  exactly x=3 m (the reliable night range) when R <= 6.5 m; spacing 1.70 m;
  closure distance scales as 0.46*R if the ring moves. ZERO-margin geometry:
  a ~10-deg yawed hook opens a seam -- do an aim-audit pass at install (the
  8x8 ground-plane fit doubles as an automatic aim check via the bridge).
  Bonuses: ring sits outside canopy shade (P126 panels in clean sky); wider
  ring = longer rotation-gauge lever arm for localization (96 vs 112 anchors,
  minor cost per the anchors sweep). Re-run the ops/locate sim with
  perimeter_n=24, radius 6.5 m when the layout firms up (Ben/Claude).
- [ ] **GPS soft anchors (4x SAM-M8Q)**: bought for dusk/dawn time; also usable as
  hands-free gauge anchors -- not slot-level (2-2.5 m CEP vs 0.7-1 m spacing) but
  4 units spread wide give mirror + rotation to ~8-10 deg (= the 3-hand-beacon
  coarse gauge, inside the solver's +-20 deg refinement) + true-north/geo-reference
  of the as-built tree (measures real door-axis azimuth vs sun arc) + mesh time.
  Needs: soft 3D position priors in ops/locate refine (sibling of z-anchors,
  sigma ~2 m) + a sim arm vs the 0/3-beacon baselines; antenna needs sky view
  (NOT under the panel); SBAS on, hours-long static average; ADR 0028 bus rules
  (Ben/Claude).
- [ ] **Replacement/swap flow** (validated in sim 2026-07-15, report addendum: 6/6
  correct even at 8 dB): unconfigured node beacons "whoami" -> bridge roll-call
  diffs live MACs vs fleet map -> ~1 min neighbor RSSI capture -> pinned solve
  (existing pipeline, survivors = known) -> write fixture_id/xyz/neighbor-table NVS
  OTA. Firmware needs: unconfigured-beacon state, NVS config schema + epoch, bridge
  assign command. Same machinery doubles as a drift watchdog (node compares live
  neighbor RSSI vs stored expectations, self-reports if moved/fallen) (Ben/Claude).
- [~] **Design the light placement layout** -- Nevada City production layout
  converged 2026-08-06 (ADR 0032): 72 downlights in three rings of 24, 24 all-HEX
  perimeter, 18 mixed chandelier, and about 16 trunk lights trending RGBW. Per
  Elliot (2026-07-15, via Ben),
  the build-dashboard STRUCTURAL geometry is correct (6.5 m tree, 10 m canopy,
  24 limbs, 2.7 m waist) but its lighting sketch (~90 lights) is NOT the plan.
  Remaining design inputs to
  reconcile in one layout: (1) gobo non-overlap -- at 7 ft hang with the LED
  dropped 6", ground-pattern diameter ~0.84 m, so downlight spacing >= ~0.85 m
  (the 10 m canopy makes this feasible; the 0.3.1 CAD's inner ring at ~0.5 m
  spacing does not); (2) inner ring moves outward anyway (bamboo criss-cross
  above it shades the panels; better hang points at the bamboo split
  criss-crosses, per Ben + Steve); (3) mild rotational ASYMMETRY in the perimeter
  ring helps the auto-localization registration gauge (even spacing is the worst
  case, see AUTOLOCATE report); (4) hang points must exist on the real limbs --
  get the structural export/hang-point map from Elliot. Output: our own
  fixtures.json (schema resonance.fixtures/0.x) that replaces the 0.3.1 patch as
  ops/locate ground truth and feeds the Lighting-Controller app (Ben + Steve +
  Claude).
- [ ] **Plan THREE surveyed "beacon" fixtures into installation** -- record which
  fixture slot 3 distinctive devices occupy (hand note or bridge identify-blink).
  This pins the registration gauge; without it the rotational alignment rests on a
  ~1-2 % cost margin, and 2 beacons cannot pin the mirror (measured + regression-
  tested, see report) (Ben).
- [ ] Confirm perimeter VL53L5CX **mount downtilt** with Steve: at 5 ft with ~4 m
  range, ground zones need ~15 deg downtilt for the plane-fit height anchor the
  study assumes (Steve).
- [ ] **Write an ADR after Ben reviews the verdict**: fixture auto-location =
  RSSI+ToF (+beacons) / photogrammetry / manual -- decision + consequences. (Ben).

## Field reliability concerns (surfaced 2026-06-04 -- important for the deployed lantern)

- [ ] **Auto/remote reset is unreliable on the bench USB-JTAG path -- harden the FIELD reset paths so a deployed lantern NEVER needs a physical button press** (that would mean taking it down + disassembling = unacceptable). Observed: after a USB flash, the PowerFeather's "Hard reset via RTS pin" sometimes did NOT start the app (no liveness LED) until a *physical* reset or a serial-open nudge (chip verified healthy via esptool; worst on the heavily-abused board 2). Field paths: (1) **OTA `/update` software reset (`esp_restart`)** -- **VALIDATED 2026-06-08: ~17/17 battery-only OTAs recovered, no button, incl. 3/3 on LFP at the ~3.2 V buck-boost crossover; new image confirmed running, `rr=software` every time.** The JTAG-RTS flakiness is USB-flash-only, not OTA; (2) **watchdog** -- DONE + validated in `net_bench` (port to production); (3) `--autosleep` USB-supply recovery -- validated. **Remaining OTA-robustness (refinements, not blockers -- a failed OTA is safe: stays on / A-B rolls back, never bricks):** (a) OTA over a **marginal/lossy WiFi link** (field maintenance assumes a decent local AP). (b) **A/B rollback -- VALIDATED 2026-06-08:** a self-test-failing image (`extern "C" verifyOta()`->false) auto-reverts to the last-good image, no touch, battery-only. Gotcha: the hook is C-linkage (needs `extern "C"`, else it silently doesn't override and the bad image sticks). Goal met for the happy path: zero field scenarios needing the reset button (Ben).
- [ ] **Implement the production rollback/health pattern in the real firmware**: `extern "C" bool verifyOta()` self-test (power chip + radio + fuel-gauge reachable); PLUS `verifyRollbackLater()=true` to defer the mark-valid + extended self-test + watchdog so an image that PASSES verifyOta but crashes/hangs LATER still rolls back (otherwise it's marked valid and could brick). power_bench has the gated `RES_OTA_FAIL_SELFTEST` test fixture to verify it (Ben).
- [ ] **Derive the nightly power budget bottom-up from measured draw** -- the old ~120 mAh/night napkin floor is RETIRED (2026-07-02): it assumed ~5 mA time-avg LEDs, pre-hardware, and crisp-gobo light levels invalidate it; do not anchor on it. Real budget = brightness x LED count x duty cycle from measured HEX/RGBW draw (400-500 mA at full) + a realistic show duty cycle; then size battery (LFP capacity) + panel (W) to it.
  **Runtime math notes (2026-07-22, Ben + Claude, vs the solarsim harvest numbers):**
  measured full-RGBW draw 1.364 W battery-side (ADR 0026 INA chain); single-channel
  warm white ~0.34 W (1/4 of full). Reference show profile: 5 h bright at 50% duty
  (3.4 Wh) + 4 h dim single-channel at 50% (0.7 Wh) = ~4.1 Wh/night, spanning the
  ~10 h dark window. Two opposing corrections when reading ops/solarsim figures:
  plotted runtimes omit the ~0.85 LFP discharge round-trip (the x0.63 chain is
  charge-side only), but the python sim runs ~0.70x of the SketchUp reference --
  net usable ~1.21x plotted, i.e. plan against plotted Wh as usable and carry ~20%
  implicit margin. Fleet check (hinged canopy): worst lantern 4.6 Wh plotted
  (~5.6 usable) covers the reference show; median 9.2 Wh = ~2x headroom; the
  one-rule uniform-30 hinge floor (3.1 Wh) does NOT cover it in the north arc --
  the two-rule hinge is what buys the whole fleet the baseline show. Caveats:
  0.70 is model-vs-model median (per-light scatter, rank corr 0.87) -- bank the
  floor only after Elliot's re-raytrace or real panel telemetry. The COTS tests show PowerFeather has the headroom; this just sets the cell/panel spec (Ben).
- [~] **Buck-boost converter efficiency varies with VBAT -- and LFP's plateau sits on the crossover (real budget + chemistry finding).** **PARTIAL ANSWER 2026-06-10** (`ops/bench/bb_efficiency.py` on the full-discharge JSONL -- no new bench time): at full-RGBW show load the LFP *terminal* voltage sags to ~2.9-3.05 V, so the converter ran in **boost the entire pre-brownout discharge -- the 3.25-3.35 V crossover band was never visited under load**. Overhead (ESP+WiFi+converter, not separable) ~0.48-0.52 W and roughly flat; P_led/P_batt lower bound 0.61-0.64. So the crossover-tax concern does NOT apply at show loads; **the residual open regime is the production light/ambient load (tens of mA), where the plateau terminal V ~3.2-3.3 V DOES sit near the crossover** -- needs a light-load fixed-brightness discharge (or rail-side metering for absolute eta). Caveats: n=1 cell/board/load; apparent fine structure vs VBAT may be time-confounded (WiFi activity). `battery_mA` != LED current (TPS631013 buck-boost sits between them); efficiency dips in the buck<->boost crossover (~VBAT 3.25-3.35 V) where it 4-switch/mode-hunts. **LFP's flat plateau (~3.2-3.3 V) parks right there for most of the discharge** = a standing efficiency tax on everything; **Li-ion lives mostly in clean buck** (better converter efficiency, the counterpoint to LFP's safety/heat/cycle wins). **Test:** hold one fixed brightness, discharge full->empty, log `ima` vs `VBAT` -> maps converter efficiency vs SOC (the real budget input) + confirms the crossover bump; run on LFP and Li-ion to quantify the chemistry tax. NOTE this **confounds the existing PAR/mA efficiency plots** (each LED run was at a different SOC/load -> different converter point), so those slopes are *system* efficiency at as-measured conditions, not a clean LED-intrinsic ranking -- re-rank at a fixed VBAT (bench supply) or correct with this curve (Ben).
- [ ] **WS2812/SK6812 latch their last frame** -- firmware must send an explicit all-off on shutdown/sleep or the LEDs stay lit (and keep drawing) with no data; matters for low-power/shipping modes (Ben).

## Battery / solar sourcing

- [x] Qualify a second 32700 sample before committing to the bulk buy -- **DONE
  2026-07-06**: second fullbattery cell delivered 5,752 mAh (+0.5 % vs June's 5,726),
  n=2, 75-unit purchase validated (100 more bought 2026-07-07 -- ADR 0025). The Amazon
  Palowextra "7.2 Ah" alternative measured 5,643 mAh (78 % of label) with 2.3x IR and
  was REJECTED -- see LOG 2026-07-06/07 and
  `docs/tests/BATTERY_32700_SHOOTOUT_REPORT_2026-07-06.html` (Ben/Claude).
- [x] ~~Compare 18650 LiFePO4 sample capacity against rated capacity~~ -- **SUPERSEDED
  2026-07-08**: production format is the 32700 (ADR 0025); the 18650 remains a bench
  cell only (Ben).
- [x] ~~Evaluate 26650 LiFePO4~~ -- **SUPERSEDED**: the open big-cell question is now
  the 20 Ah #6832 for solar-free classes, below (Ben).
- [x] **Bench-test the 20 Ah LFP samples -- CLOSED 2026-07-15 (option cancelled).**
  Sample 1 verified honest 2026-07-12 (19,412 mAh, 97.1% of label; report
  `docs/tests/BATTERY_20AH_UPLIGHT_REPORT_2026-07-12.html`), but batteryspace could
  not supply ~40 cells in time and the Alibaba counterpart (~$4.50/cell bulk!)
  needs ocean freight that misses 2026. The immediate replacement plan was
  hinged-solar-wing + 6 Ah uplights, later superseded by the about-16 trunk-light
  allocation in ADR 0032; sample-2 qualification and the end-cap fixture are moot
  for 2026 -- the Alibaba route is the 2027 lead (Ben).
- [ ] **Qualify the 33140 15 Ah** (new large-hat fleet standard, 130 bought 07-24,
  QUALIFICATION PENDING): full charge->discharge capacity + IR run per the ADR
  0023 recipe (shootout rig; n>=2), then re-derive the dim/off/sleep voltage map
  on the new cell -- the current ADR 0023 tiers are 6 Ah-derived. Gauge:
  DesignCap 15,000 fits under the MAX17260 16,383 cap. Also verify physical fit
  in the large Polycase with panel + board + LED installed (Ben).
  **2026-08-07 sample-1 run:** ET5406A+ moved to COM44 in Nevada City and the
  1.000 A CC / 2.500 V discharge is running under the guarded JSONL logger;
  tooling and test plan are in
  `ops/bench/et5406_discharge.py` and
  `docs/tests/BATTERY_33140_GOTION_ET5406_PLAN_2026-08.md`. After corrected LFP
  charging and an overnight disconnected rest, sample 1 measured 3.362 V by DMM
  / 3.365 V by ET at about 25.6 deg C. Live data:
  `ops/bench/data/ca/2026-08-08-et5406-discharge-041818Z-gotion-33140-15ah-sample-1.jsonl`.
  Finish and analyze sample 1, then repeat sample 2 and derive the threshold map.
  (Ben/Codex)
- [ ] Avoid multi-14430 production pack unless mechanical constraints force it (Ben + Steve).
- [~] Record panel dimensions, weight, output, connector type, and shipping lead time (Ben).
  P105/P126 Voltaic ETFE specs captured 2026-06-15 in
  `docs/tests/VOLTAIC_ETFE_PANEL_TEST_PREP_2026-06-15.md`; order/lead-time record now
  lives in `ops/PROCUREMENT.md` (panels bought 2026-06-24).
- [x] ~~Search for round/circular panels~~ -- **CLOSED 2026-07-08**: production panels
  are the rectangular Voltaic ETFE (ADR 0026); round panels dropped for 2026 (Ben).
- [ ] Design hat top so the rectangular production panels mount cleanly with backup
  retention (round-panel accommodation no longer needed) (Steve).

## Custom hardware track

- [x] Decide whether custom board is needed after COTS tests -- **DECIDED 2026-07-08
  (ADR 0024): 2026 production is COTS PowerFeather V2; custom PCBA is the 2027
  option.** Items below apply only if/when that option activates (Ben + Steve).
- [ ] If custom board proceeds, use PowerFeather V2 as reference architecture, with a
  dedicated power-management I2C bus (ADR 0028) and the ADR 0029 LED wiring rules (Ben).
- [ ] Select charger/fuel-gauge/regulator architecture: BQ25628E + MAX17260 + buck-boost is current leading reference (Ben).
- [ ] Keep LED module/daughterboard separate until optics are frozen (Ben + Steve).
- [ ] Add keyed solar connector/pigtail plan; do not rely on direct panel wires to board pads for production (Ben + Steve).
- [ ] Add input protection review for outdoor solar cable (Ben).
- [ ] Add hardware reviewer before any custom board order (Ben).
- [ ] Use PCB-antenna WROOM module by default; do not use u.FL unless RF tests fail (Ben).

## Enclosure track

- [ ] **Integrate the bought COTS enclosures** (strategy update 2026-07-13 --
  hats are purchased boxes now, see `enclosure/README.md`): panel mounting,
  bamboo clamp/attachment, internal mounting for PowerFeather + cell + both LED
  roles, USB-C gasket cutout, ToF windows, strain relief; then thermal/RF proof
  on the real boxes (Steve + Ben).
- [ ] Coordinate the chandelier carpenter box: dimensions for 18 fixtures,
  venting, service access, USB-charging reach (Ben -> Elliot/carpenter).
- [ ] **Design the trunk-light variant** (see `enclosure/README.md`): tree
  attachment, battery/panel arrangement, 4 W RGBW vs lensed 3 W RGB mount and
  protection, and gasketed panel-mount USB-C charge/flash access (Steve + Ben).
- [ ] **ToF apertures**: downward eye port beside the gobo (downlights), outward
  window with protective cover (perimeter hats) (Steve).
- [ ] Add strain-relief plan for panel pigtail / VDC connector (Steve + Ben).
- [ ] Keep antenna region away from solar panel, battery, screws, and metal (Steve + Ben).
- [ ] Decide rope attachment point with team; hybrid primary-hat + secondary-bamboo safety tie remains current recommendation (Ben + Steve).
- [ ] Material test for filter/gobo: matte paint on PLA, translucent PLA, frosted resin (Steve).
- [ ] Thermal test sealed hat in sun/heat with charger and LEDs operating (Ben + Steve).
- [ ] Send hat v1/v2 STL out for MJF evaluation after COTS fit is known (Steve).

## Firmware track

### Fixture-side gaps surfaced by Bridge OS (2026-08-19; Bridge OS sends the full
### intended contract now — these make the fixture honor it. Plan of record:
### `firmware/tdeck_bridge/README.md` + the Bridge OS ADR.)

- [ ] **NB_SET_MAINTAIN must apply-then-confirm, never persist-then-apply**
  (2026-08-19 incident: a 5.5 V VINDPM broadcast persisted to NVS killed
  USB-powered, battery-less peers at every boot — bootloader-level recovery).
  `applyMaintainV10` (`board_power.cpp:479`) writes NVS BEFORE applying. Mirror
  the OTA pending-verify pattern: apply in RAM, persist only after the supply
  survives a confirmation window; revert on brownout/reboot. Consider also
  refusing maintain > measured supply voltage. Related hardening: the CoreS3
  bare-`m` preset landmine (first preset was 55!) is already defused
  2026-08-20 — explicit digits now required (Ben/Claude).
- [ ] **Blacklist ModemManager for bench USB-serial devices** (suspected
  incident trigger: MM probes freshly enumerated ports and its bytes land in
  bridge serial parsers). Add
  `/etc/udev/rules.d/99-mm-bench-blacklist.rules` with
  `ATTRS{idVendor}=="303a", ENV{ID_MM_DEVICE_IGNORE}="1"` and
  `ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ENV{ID_MM_DEVICE_IGNORE}="1"`,
  then `sudo udevadm control --reload` — needs sudo, on both the Ubuntu PC and
  any bench laptop running a desktop Linux (Ben).
- [x] **Re-apply seed/params when the same program is re-leased. DONE
  2026-08-25:** `ChoreoRuntime::applyProgramSet` now resets the active program
  with the received seed/params, so CA light/knock changes no longer require a
  release/re-lease blip. Release/expiry also resets a leased GH program back to
  autonomous defaults even when both have program ID 1. Native regression and
  both embedded development builds pass; hardware recheck remains in the CA
  acceptance item below (Ben/Codex). (The alternative was to implement
  the declared-but-unbuilt `NbProgramSet.flags bit1` params-are-delta,
  `packet.h:270`; that delta form remains unimplemented.)
- [ ] **Make `ProgBridge` honor `NbShowFrame.bright` / `beat_phase` / `energy`.**
  The fields are plumbed end-to-end into `ShowFrameState` but no program reads
  them (grep-verified) — they are the cheapest fleet-wide dim + audio-reactivity
  path (one broadcast packet). Bridge OS populates them as of M5 (Ben/Claude).
- [ ] **`NB_SENSOR_REPORT` (next free type, 29): bounded-window fixture→bridge
  sensor snapshot** mirroring `NB_NEIGHBOR_REPORT`'s pattern, gated like
  `NB_LOCATE_CONTROL`. Today only `sensor_bits`/`class_mismatch` ride hb-full;
  tilt/ToF depth/error counters need maintenance-mode HTTP — a radio sensor
  app needs this opcode (Ben/Claude).
- [ ] **Honor `NbEvent.fire_in_ms` + add a strike event kind.** The presence-wave
  handler ignores `fire_in_ms` and fires immediately
  (`behavior_glue.cpp:148`); a synchronized ring-wide knock schedule (ADR 0031
  adjacency) needs scheduled fire plus an `NB_EVENT_STRIKE` kind routed through
  `behaviorStrikePermitted()` (Ben/Claude).
- [x] **CA->strike seam. DONE 2026-08-25:** GH params byte 5 selects a
  sound-only knock wildfire. Each quiescent->excited edge emits one bounded
  40 ms `ProgramOutputs` request; ESP32 glue routes it through
  `behaviorStrikePermitted()` and the ordinary solenoid mechanism gates. The
  program suppresses LED rendering, keeps distributed CA state live during
  production day states, and does not depend on ToF. Native regression and
  both embedded development builds pass (Ben/Codex).
- [ ] **Hardware-validate CA knock wildfire on a named daylight cohort:** use
  explicitly armed solarnoid downlights plus at least one non-solenoid relay
  participant. Confirm no LED-rail activity, one physical strike at most per
  excitation edge, neighbor propagation, lease expiry/release, same-program
  knob reapply with no blip, refusal outside DAY_ACTIVE/solar surplus in field
  profile, and sane RSSI vs pinned adjacency. Do not treat ToF as a prerequisite
  for this test (Ben/Codex).
- [ ] **Implement `NB_NEIGHBOR_SET` flags bit0 NVS persistence** (declared "M2
  item" at `behavior_glue.cpp:292`); the Bridge OS CA studio is about to become
  the first real sender of type 24 (Ben/Claude).

- [ ] **A/B the ESP32-S3 alternative internal RTC sleep source:** compare the current
  default low-frequency RC against `CONFIG_RTC_CLK_SRC_INT_8MD256` on several
  PowerFeathers for actual deep-sleep current, 300/900 s timer error, temperature drift,
  build-toolchain impact, and unchanged radio/power behavior. The documented +5 uA is
  only 0.12 mAh/day, but adopt it only if the measured holdover improvement is real and
  integration stays boring. It may improve rendezvous efficiency; never make POR
  recovery or autonomous correctness depend on it. See the July 13 distributed
  choreography concept note (Ben/Codex).
- [ ] **Prototype rootless fleet phase + future event fabric in `firmware/core/`:** native
  tests first for phase convergence, boot/session IDs, event dedupe/expiry, future
  scheduling, raw-observation freshness, packet loss, duplicates, partitions, stale
  state, random wake phase, and POR reacquisition; then a five-node sleep/rendezvous
  bench. No permanent coordinator. Validate aligned wake collision behavior and measure
  residual event skew before selecting an algorithm (Ben/Codex).
- [ ] **Build the production behavior layer as a generic distributed choreography
  runtime, not a CA-only engine:** exercise at least one CA, one deterministic scheduled
  timeline, one spatial solenoid/light ripple, and one distributed presence-triggered
  easter egg through the same program/event interface. Bridge control is an expiring
  overlay for DJ shows/diagnostics/registration; bridge loss returns to autonomous
  operation; local power policy always wins. Design space and phased validation:
  `docs/research/AUTONOMOUS_DISTRIBUTED_CHOREOGRAPHY_CONCEPT_2026-07-13.md` (Ben/Codex).
- [ ] Create board definitions for `powerfeather_v2` and the possible `resonance_custom` target (bake-off boards retired -- ADR 0016 annotation) (Ben).
- [ ] Implement telemetry abstraction for charger/fuel gauge / battery monitor (Ben).
- [ ] Implement LED driver abstraction per ADR 0029 roles: SK6812/WS2812 via NeoPixelBus (3V3 rail) + 4 W RGBW point source (VBAT-direct); no I2C LED controllers (Ben).
- [ ] Implement LED rail power abstraction (`VSQT`, onboard LED LDO, external rail enable) (Ben).
- [ ] Implement standard OTA maintenance mode; no ESP-NOW firmware chunks (Ben).
- [~] Add an autonomous low-VBAT park/cutoff policy for ordinary net_bench/production
  COMMS mode. **FIRST NET_BENCH PASS DONE 2026-06-30:** `--field-cycle` in
  `net-bench-2026-06-30.4` adds a production-ish solar day / radio-night lifecycle:
  charge-sleep on supply, wait-dark, always-awake 1 Hz radio drawdown, then protect
  timer-sleep at the LFP floor. Deployed to `9E5AB8` and logging to
  `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8.jsonl`. Remaining: analyze the
  first full cycle, tune full/taper and cutoff thresholds, then port the proven policy
  into production firmware (Ben/Codex). **THRESHOLDS NOW MEASURED (2026-07-07): use
  ADR 0023's tiers (standard: dim 3.00 / off 2.95 / sleep 2.90 under full load) -- the
  current field-cycle floors (3.10/3.00) strand capacity; hysteresis + coulomb-primary
  hybrid requirements are in the ADR.**
- [ ] Implement the ADR 0023 low-battery state machine in field-cycle/production
  firmware: latched dim/off/sleep transitions with 60 s confirm + >=150 mV re-entry
  hysteresis, load-compensated voltage (`bv + 0.15 x I_A`), coulomb-remaining primary
  (DesignCap ~5750, gauge current /1.08), voltage tiers as backstop, watchdogged
  sleep (Ben/Claude).
  - [ ] Add the missing LED-off state. Current field-cycle treats confirmed 2.95 V as
    `FC_PROTECT` and immediately timer-sleeps; ADR 0023 calls for LEDs off with
    duty-cycled OTA reachability until the separate 2.90 V sparse-sleep threshold.
    This did not cause P105's July 11 early POR failure at about 3.04 V, but it leaves
    the 2.95-to-2.90 V reserve unused in a clean run. (Codex)
  - [~] Make low-VBAT protection survive rail-collapse/POR loops. P105 `9F26F8`
    produced 31 `poweron` resets in about 19 minutes on 2026-07-11 while the HEX load
    held VBAT near 3.0 V; every hard reset erased the RTC cycle state and in-RAM 60 s
    debounce, then boot re-enabled the load. **P105 FIX DEPLOYED 2026-07-12:**
    `net-bench-2026-07-12.1` persists idle/full/dim/protect session stages before
    rail-on. A POR from full consumes one staged retry at dim brightness; a POR from
    dim or protect hard-parks until verified charge. The exact P105 artifact is
    `build/field-cycle-peer-20260712-p105-dusk-dim-retry-r3/net_bench.ino.bin` with
    dim 3.10 V / 10 s, low 2.95 V / 60 s, and critical 2.90 V immediate. OTA ack,
    explicit `/resume`, ESP-NOW rejoin, and one natural five-minute charge-sleep wake
    are verified. Remaining: validate tonight's real dim/POR behavior; optionally
    induce one full-stage reset and a second dim-stage reset under supervision. The
    The original P126 peer remained on `.1`; replacement `9F2690` received `.3` by USB
    on July 13 and now carries the same durable reset/protect logic. Hardware fault
    injection during its lighter three-pixel load remains unvalidated. (Ben/Codex)
  - [x] **Keep externally powered, battery-absent fixtures serviceable even when
    PROTECT was persisted -- DONE 2026-08-08:** a persisted PROTECT stage now keeps
    rails parked but does not timer-sleep while a good external supply is present
    and no plausible battery voltage exists. A guarded serial `X` clear additionally
    requires PowerFeather readiness, charging off, >=4.5 V good supply, no BQ fault,
    and no plausible attached battery. Removing external power restores the normal
    sleep decision. Native regression coverage and production-board `9E5A94` both
    pass; the latter recovered over USB with its BAT and VDC ports empty (Ben/Codex).
- [~] Replace the field-cycle dusk/dawn shortcut with the ADR 0031 scheduled production
  gate. Current net_bench enters draw from local panel/lux classification:
  `fieldCycleSupplyPresent()` requires `csV >= 4.0 V` plus useful input/charge current
  >=20 mA; TSL2591 peers use five minutes at <=200 lux and >=500 lux for dawn; bare
  peers use 30 minutes without useful input. These remain useful bench classifiers, but
  clouds, shade, panel angle, charge taper, or moving a fixture can still start early,
  and bare peers have produced 13-15 h shows. Production should wake before a versioned
  UTC transition, reacquire time from sparse GPS/RTC anchors, start/stop on schedule,
  and expose source/age/uncertainty. Panel/lux telemetry becomes a sanity check and
  optional explicitly bounded degraded-mode input. Exact invalid-time fallback remains
  open; it must not silently recreate the artificial long show. (Ben/Codex).
  - [ ] **Bench false-dusk observation 2026-08-27:** a battery-powered fixture
    separated from GPS/RTC/bridge time in bench shade can lose the RAM-only
    consensus estimate (30-minute maximum hold) and then satisfy the bare-peer
    30-minute no-input dusk gate. Its autonomous CA appears as random daytime
    light. Decide a fail-dark or tightly bounded degraded behavior before field
    release; shade/no-anchor must not silently authorize a normal-length show.
    USB input >=20 mA should end an existing false night after the five-minute
    dawn confirm, which also provides a bench discriminator from commission
    mode. (Ben/Codex)
  - [x] **Confirmed failure mode 2026-07-11/12:** charge termination drives both
    `supply_ma` and `battery_ma` below 20 mA while panel voltage remains high, so the
    peer declares false dark, pulses the LED load, then declares sunrise when current
    returns. This produced dozens of false P105 cycles and repeatedly reset coulomb
    counters. Addressed for P105 by the qualified 200/500 lux gate above; the bare-peer
    fallback uses a 30-minute confirm rather than panel voltage, which remains high at
    zero current. (Codex)
- [ ] Optional backlog: one cold-night discharge at a representative dim load to
  sharpen ADR 0023's tiers for winter (they're currently 79.9 deg F, n=2 data;
  conservative tier is the hedge until then) (Ben).
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
- [ ] Qualify an explicit USB-vs-panel input discriminator before making the fleet
  dashboard source glyph authoritative. Current production telemetry proves a good
  charger input but cannot universally identify what is feeding it, so the dashboard
  honestly uses fixture class (panel-bearing class -> sun; chandelier -> plug) and
  labels panel-loss from daylight fleet consensus as suspect rather than certain.
  Ben elected to keep the icons as a useful visual convention; do not silently
  relabel the inference as measured source identity (Ben/Codex).
- [ ] **WiFi re-associate guard (cheap roaming):** the ESP32 latches one Eero BSSID and won't auto-roam (no 802.11k/v/r -- LOG cont. 9, POWERFEATHER_NOTES). On link-loss / low-RSSI in maintenance mode, do `WiFi.disconnect()` + `WiFi.begin()` to re-pick the strongest beacon. Low field priority (deployed fixtures are stationary; the maintenance-OTA path already does a fresh `WiFi.begin`) -- but a belt-and-suspenders guard for OTA windows (Ben).
- [~] Implement ESP-NOW heartbeat/state packets with jitter and sequence numbers -- done in `firmware/net_bench/` (feasibility); port the validated packet/PDR design into production `core/packet` after the matrix run (Ben).
- [ ] Implement low-battery modes: dim, LED hard-off, shipping mode (Ben).
- [x] **8-bit LED low-end dimming: gamma REJECTED, render stays linear** (decided 2026-08-18, deferred from 2026-06-07): gamma correction is scrubbed from the codebase -- the 1..23 -> 0 dead zone that plagued the ambient look was a gamma artifact, and linear 8-bit render has no floor-to-zero problem at all. The salvaged dim-floor table (`resGamma8`) was deleted the same day it was salvaged; recover it from git history (commit 9260ae3) if ever needed. If the ambient look later needs finer sub-24 granularity, reach for temporal dithering, NOT gamma (Ben).
- [ ] **Use the switchable 3V3 rail (GPIO4) as the LED kill-switch** in production firmware (`digitalWrite(4,LOW)` = LEDs off, can't drain the pack) -- folds into the pixel-power-architecture decision (option a). See `firmware/POWERFEATHER_NOTES.md` (Ben).
- [x] Implement watchdog/reset-reason/brownout logging -- DONE in `firmware/net_bench/` (esp_task_wdt + `--wdt-hangtest`); **validated 2026-06-07**: induced hang -> auto-reset -> `reset_reason=task_watchdog`, no human. Port to production firmware (Ben).
- [ ] Implement field telemetry logging schema for BM 2026 -> 2027 design data (Ben).
- [ ] Port `TalismanPatterns.cpp` into `firmware/core/pattern/` (Ben).
- [ ] Implement minimum-viable CA tick + render loop on bench as the first program
  family within the generic choreography runtime; compare generation-based and
  intentionally asynchronous updates rather than assuming one model (Ben).

## Production test / flashing

- [x] NC 4th PowerFeather enumeration -- RESOLVED 2026-08-06: it is now present
  as COM25 / `68:EE:8F:F4:02:F4`. It was not reflashed; the dedicated CoreS3
  `E39F1C` on COM40 now owns the desk-bridge role. (Ben/Codex)
- [x] CoreS3 standalone power check -- RESOLVED 2026-08-06: Ben removed the
  ESP32-H2 Gateway module and DIN base, and the dedicated CoreS3 bridge booted
  normally from its own USB-C connection. The earlier no-boot symptom is not
  reproducible after the bridge flash. (Ben/Codex)
- [ ] CoreS3 bridge receive/control proof: return one PowerFeather to channel-11
  `fixture` or `net_bench` peer firmware, confirm its heartbeat appears on the
  CoreS3 screen/dashboard, then run one harmless targeted identify or maintenance
  command and verify receipt. TX-only and dashboard parsing passed 2026-08-06;
  the connected perimeter trio currently runs WiFi-only LED Studio. (Ben/Codex)
- [ ] Trio OTA: endpoint GET verified on all 3 NC perimeter lights
  (led-studio-2026-08-01.3); run one full artifact POST /update cycle to
  exercise the real path before the boards go battery-only (Ben).
- [~] Fleet-wide: retire WonkyHouse and BubbyNet from production images and use
  the exact case-sensitive `Party In The Woods` SSID. Port the dual-SSID pattern
  from led_studio only if it remains useful; neither legacy bench SSID belongs on
  a production peer. `net_bench/build.sh` already rejects WonkyHouse unless Dad's
  legacy personal bench explicitly opts in, but `fixture` inherited stale local
  credentials into the Aug-8 artifact and the 24-unit correction is now P0 above.
  Still open: validate the planned virtual `Party In The Woods` SSID across both
  camp and art-site Starlinks, including channel behavior, before field reliance
  (Ben + Codex).
- [ ] Keep USB/pogo flashing as mandatory recovery path even if COTS boards support USB-C (Ben).
- [ ] Investigate JLCPCB / PCBWay firmware pre-flash only for custom-PCBA path (Ben).
- [ ] Write smoke-test host script: node ID, firmware version, battery, charge/fault, reset reason, peer count (Ben).
- [ ] Define production acceptance checklist for each fixture (Ben + Steve).

## Coordination with project team

- [ ] Confirm timing for Bamboo Pure air-ship of prototype lanterns to Steve in TN (Ben -> Elliot / Dipta).
- [ ] Align with Elliot on rope-attachment decision (Ben).
- [ ] Confirm hat OD / height / bamboo-overlap to Vishnu so he can finalize renders (Ben).
- [x] ~~Pull `INV_2026_00401`, decompose cost, compare to COTS/custom BOMs~~ --
  **RETIRED 2026-07-08**: the invoice's identity is unclear (probably the Bamboo
  Pure lantern invoice, possibly the early custom-PCBA quote), and with real COTS
  procurement recorded in `ops/PROCUREMENT.md` the comparison baseline is no longer
  useful (Ben).
- [ ] Clarify chandelier-light scope/ownership with Elliot + Vishnu (18 lights,
  internals fungible with the fleet -- ADR 0032) and decide the exact HEX/RGBW mix
  (Ben).
- [ ] Decide the TENTATIVE TN trip (~3rd-4th week of July): fleet-scale test of the
  ~70 boards at Steve's -- production-firmware mesh lighting effects + presence
  detection, indoors if enclosures aren't ready; back for the Aug 1-2 container
  unload (Ben + Steve).
- [ ] Get Steve on project's official core build team wiki (Ben -> Elliot).
- [ ] Get shared access to Co-Work's wiki folder once cloud-hosted (Ben -> Elliot).
- [ ] Drop lighting workstream digest into WhatsApp -- the 2026-07-08 team write-up
  (`docs/`) is the candidate artifact (Ben).

## Gobo pattern program (community submissions PULLED 2026-07-08)

The community Mandala submission program was pulled for time. Current plan: in-house
designs + generative-AI-modulated bamboo-leaf patterns per bamboo species in the tree
(BACKGROUND.md has the full record). Surviving work items:

- [ ] Generate/curate the in-house + bamboo-leaf pattern set (Ben + Steve + Vishnu).
- [ ] When Steve's printable "Resonance Tree" gobo source lands, measure the
  thinnest connected solid web and the smallest surviving aperture from its native
  geometry; record the nozzle, material, slicer profile, and print orientation; then
  use those proven dimensions as the generative validator baseline (Ben + Steve +
  Codex).
- [ ] Pipeline: pattern -> vectorize -> constraint check -> cone projection -> STL
  (unchanged from the original program design).
- [ ] Brightness normalization in firmware or per-filter metadata.
- [ ] Cataloging schema (per-fixture pattern identity -- useful regardless of source).
- [ ] Print schedule backward from ~Aug 10 filters-in-hand (Steve's Bambu + possible
  batch service).

## Solar layout levers (solarsim studies, 2026-07-20/21)

- [~] **Hinged downlight panels -- NOT ADOPTED for the 72 large enclosures**
  (Ben + Steve, 2026-07-24); the trunk lights DO get a hinged "wing" (per-light
  optimal angles in the rotation-sweep data). Downlight harvest reverts to the
  FLAT baseline (550 Wh fleet, worst ~3.4 Wh, 6 lights under 3 h full runtime) --
  re-check the power-budget note's show floor against flat numbers. Study kept
  for the record: tangential bracket+hinge (pitch-only) measured
  +15.7% fleet solar (free-azimuth ideal +25.3%); two-family install rule (pitch
  outward ~40 deg, northern arc inward ~30) needs no per-unit config; a ONE-rule
  aesthetic variant (uniform outward 30 deg) measured +10.7% (-4.4% vs optimal,
  cost concentrated in the north arc: -27%, 5 lights back under 3 h). Blockers to
  resolve: whole-lantern tilt would wreck the gobo projection (wants a panel-only
  flap -- Steve); no-spin harness freezes RF panel azimuths at KNOWN radial angles
  -- run the frozen-radial arm in ops/locate before committing (data + figures in
  ops/solarsim/data). HARNESS CANDIDATE (Ben + Steve, 2026-07-22): V-hang between
  adjacent attach points, two ropes to the polycase short-edge lips (3 holes each,
  already there) -- solves anti-spin + keeps rope shade off the panel (the lit^2 x
  0.75 mismatch penalty makes even a thin center-rope shadow expensive; unmodeled
  in the sim). Simulated between-limbs positions: solar-NEUTRAL (+0.8% hinged,
  -1.0% flat; middle ring +7%, inner -3%) -- the roof weave dominates occlusion,
  not the limbs, so the V-hang is justified mechanically, not by harvest
  (Ben + Steve).
- [ ] **Inner ring at 3.26 m** (middle equidistant): +28% inner-ring solar, fixes
  gobo spacing; door-axis finding: treev4 tunnels sit at az 75.4/255.4 (rotated
  14.6 deg CCW, near-sunrise-aligned) -- reconcile orientation + door lights with
  Elliot; rise/set chord can NEVER hit both doors (non-antipodal), belly crossings
  at ~07:25/~18:25 are the symmetric photo-op (Ben -> Elliot).
