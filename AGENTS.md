# AGENTS.md

Read this if you are an AI agent picking up work in this repo for the first time.

## What this repo is

The power, electronics, firmware, and electronics-enclosure ("hat") workstream for the **Resonance Tree** -- a bamboo art installation for Burning Man 2026 + 2027. This repo is one slice of a larger project. The other slices (bamboo structure, structural engineering, parametric lighting design, project management) live elsewhere, mostly with the project lead Elliot Fabri and his Co-Work agent.

## Read order at session start

1. `README.md` -- orientation.
2. `LOG.md` -- what changed recently.
3. `TODO.md` -- what's open.
4. `BACKGROUND.md` -- full project context, team, history. Long but worth it on first session.
5. `docs/block-diagram/SYSTEM.md` -- the canonical system architecture and power budget.
6. `docs/decisions/` -- ADRs for every major architectural decision so far. Numbered. Read in order if you have time, otherwise look up by topic.

After session, append to `LOG.md` with a dated entry summarizing what changed and why. Append to `TODO.md` for new open items. Add to `docs/decisions/` when you make a new architectural call.

## High-priority bench gotchas

- **Artifact identity and shared-bench writes:** follow
  `docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md` (ADR 0040). New shared fixture
  revisions are generated as `fx-YYMMDD-<recipe7>-<variant>` and travel with an
  immutable manifest plus exact binary SHA-256. Never reuse a revision, select
  an image by newest mtime/`latest`, or treat a branch name as an artifact.
  Live color control may remain intentionally leaseless, but OTA, USB flash,
  profile/channel persistence, reboot, and NVS mutation have one declared
  operator across all bridges/laptops. Name explicit target short MACs.
- **OTA fleet path:** default to shared-WiFi / portable-router maintenance mode plus
  `ops/bench/net_bench_ota.py` parallel uploads. Do **not** build or recommend
  `--maint-ap` unless Ben explicitly asks for the deprecated one-board AP fallback;
  self-hosted AP mode is not scalable and has confused recent OTA debugging.
- **OTA power ride-through:** install the fixture LFP (or use a separately proven
  stable supply) before treating an OTA/A-B rollback test as valid. On 2026-08-10,
  bare-USB fixtures recorded brownouts and one repeatedly lost power during the
  20-second pending-verify window; rollback safely restored `.1`. The same exact
  `.2` artifact passed immediately once an LFP was installed. USB is still the
  rescue/data path, but the production battery is the expected reboot ride-through.
- **One mesh wire contract, one file:** `firmware/fixture/src/core/packet.h` is
  the ESP-NOW protocol for the whole fleet -- fixtures, `cores3_bridge`, and all
  host tooling parse these exact layouts, and `test_packet_layout.cpp` pins
  golden `sizeof`/`offsetof` so an accidental reorder fails at build time. Any
  new bridge, handheld, or publisher **includes** it; do not write a second
  protocol header (a design brief proposing `mesh_protocol.h` predates this
  file -- see ADR 0037). The header has no Arduino includes, so it compiles
  natively and on any ESP32 target. Struct evolution is append-only.
- **Arduino compile cache:** for iterative fixture work, use
  `firmware/fixture/build.sh --dev-cache ...`; its atomic lock guarantees one
  writer, its recipe fingerprint resets incompatible objects, and its binary
  reports `dev-local`. A killed/interrupted cache is untrusted: do not remove its
  marker or lock by hand, and run `./build.sh --recover-dev-cache` only after
  confirming no Arduino/Xtensa process remains. Do **not** run direct parallel
  `arduino-cli compile` commands against the same sketch/cache. Other wrappers
  still require sequential builds or unique `--build-path` values. Parallel
  builds against one Arduino path can collide with `unlinkat ... directory is
  not empty` and corrupt mixed artifacts. Shared/fleet artifacts never use the
  dev cache; they remain fresh, immutable named builds under ADR 0040.
- **Arduino build timeout and recovery:** an uncached ESP32-S3/PowerFeather build on
  this Windows bench normally takes about 2-3 minutes. Give the outer command at least
  300 seconds; if it yields a running cell, keep waiting on that cell instead of
  starting another build. If a compile is killed or times out, first confirm no
  `arduino-cli` or Xtensa compiler process remains, then abandon that build directory
  and retry with a fresh unique suffix (`...-r2`, `...-r3`, etc.). Never resume a
  killed build directory: a partially written `core/core.a` produces misleading linker
  floods such as `bad reloc symbol index`, even though the sketch source compiled.
  A valid build ends with the flash/RAM usage summary and a non-empty
  `net_bench.ino.bin`; inspect `build.options.json` to verify the exact deployed flags.
  For `fixture --dev-cache`, the wrapper's surviving `.build-in-progress` marker
  is the authority: stop lingering exact compiler PIDs, then quarantine with
  `--recover-dev-cache`; never resume or manually unlock it.
- **Build once, OTA the artifact:** for field-cycle work, use
  `ops/bench/field_cycle_ota.py ... --build-only` with a named build, verify it, then
  pass that `.bin` back with `--bin` for OTA. This avoids an accidental second compile
  or a changed flag set between validation and deployment.
- **OTA completion is fresh evidence, not upload ACK:** success requires a
  heartbeat newer than the job start, the expected reported revision, and
  survival through the fixture's 20-second pending-verify window. A cached
  `online=true` entry can remain true for 30 seconds and does not prove rejoin.
  An automatic USB boot salute likewise proves only a stable boot; only the
  host-side commissioning gate may declare a fixture ready to unplug/install.
- **Sleeping-peer OTA timing:** a field-cycle peer may deep-sleep for 300 seconds and
  listen for only 8 seconds. `field_cycle_ota.py` therefore defaults to a 360-second
  discovery deadline; do not shorten it below one full sleep cadence for an already
  sleeping peer, and leave maintenance resends enabled. A discovery timeout means no
  OTA was attempted; it is not a failed flash.
- **Field logger output safety:** `ops/bench/net_bench_log.py` exclusive-creates output
  by default and refuses an existing JSONL path. Use a new path for a new run. Use
  `--append --out <existing>` only to continue the same logical run after an outage;
  it preserves the original metadata and writes a numbered segment boundary. Never use
  `--overwrite` unless Ben explicitly wants the existing trace destroyed. After launch,
  verify the file is growing and contains each expected peer; process existence alone
  does not prove that dashboard/UDP forwarding is reaching disk.

## Who's working in this repo

- **Ben Eckart** (`ben.eckart@gmail.com`) -- power systems, firmware, mesh networking, project lead for the lighting workstream within Resonance. Primary committer to `/firmware/` and `/hardware/`.
- **Steve Eckart** (Ben's dad) -- enclosure design, 3D printing, mechanical integration. Primary committer to `/enclosure/`.
- **Claude** (this) -- pair-programmer for both Ben and Steve. Cowork instance handles project management and review (this side). Claude Code instances handle daily implementation iteration.

The wider Resonance project team is in `BACKGROUND.md` -- read it for names and roles. Don't message them or assume their context; coordinate through Ben, who interfaces with them via WhatsApp.

## What's known vs assumed

**Decided** (see ADRs; superseded entries kept for history -- do not build on them):
- ~~ESP32-C3-MINI-1 module for production (ADR 0001)~~ -- superseded by ADR 0011/0021: ESP32-S3 PowerFeather V2.
- LiFePO4 battery chemistry (ADR 0002).
- ~~CN3058 LiFePO4 charger IC (ADR 0003)~~ -- superseded by ADR 0014; reality is the PowerFeather's BQ25628E.
- ESP-NOW mesh, no infrastructure required (ADR 0004; the mesh-gossip OTA part alone was superseded by ADR 0010).
- FreeRTOS task architecture, not Arduino loop() (ADR 0005; constrained by ADR 0028 -- no power-management I2C from core-0 tasks under WiFi).
- ~~Custom PCB with reflowed module, not dev-board-on-carrier (ADR 0006)~~ -- superseded by ADR 0012; resolved to COTS production by ADR 0024.
- Electronics in a separable hat on top of the bamboo lantern, not crammed inside (ADR 0007).
- ~~WS2812B powered direct from Vbat, no level shifter (ADR 0008)~~ -- superseded by ADR 0013; the VBAT-direct idea won the fat-wire bench but LOST the production-cabling A/B -- both LED roles ship rail-fed (ADR 0029 + 2026-07-11 amendment).
- Minimize per-fixture operations at scale: no soldering on receipt, no per-unit configuration, jig-automated flashing (ADR 0009).
- PowerFeather V2 (ESP32-S3) confirmed as the COTS reference after feasibility de-risking -- networking, solar, and battery-only no-touch OTA all validated (ADR 0021).
- Mixed LED fleet by optical role: SK6812 HEX + 4 W RGBW point source (ADR 0022).
- **Production locked: COTS PowerFeather V2 with a nominal 130-fixture Nevada City layout in four classes** -- 72 downlights (3 rings x 24) + 24 all-HEX perimeter + about 16 trunk lights trending RGBW + 18 mixed HEX/RGBW chandelier. The team intends the full layout barring an unforeseen issue; canonical counts are in `docs/block-diagram/SYSTEM.md` (ADR 0032 supersedes ADR 0024's allocation only).
- Production batteries, TWO-TIER since 2026-07-24 (ADR 0025 + annotations): 33140 15 Ah (batteryhookup, 130 bought -- QUALIFICATION PENDING) for large-enclosure fixtures/downlights; 32700 6 Ah (fullbattery, qualified n=2 at ~5.75 Ah) for small-enclosure classes + chandelier. The Amazon "7.2 Ah" was measured and rejected; ADR 0023 thresholds are 6 Ah-derived -- re-derive for the 33140 before trusting.
- Solar panels: Voltaic ETFE P105 5 W (downlights) / P126 2 W (perimeter), bought and outdoor-measured (ADR 0026).
- Sensors and automatic class identity: ID-verified TMF8820/TMF8821-family sensor
  -> canopy/downlight; else VL53L5CX -> perimeter; else MSA311 at `0x62` ->
  trunk/uplight; else no class sensors -> trunk/uplight for the installed 2026
  fleet. Preserve `class_last` and flag a mismatch when a known ToF-bearing class
  loses its ToF. Future chandelier PowerFeathers are selected by exact MAC and
  persist `class_ovr=4` before installation; sensorless automatic records left by
  older firmware migrate from chandelier to uplight (ADR 0067 supersedes ADR
  0041's no-sensor fallback). BMP581 never determines class; the 30 bought units
  are allocated as 24 outer-ring downlights + 6 spares (ADR 0034). Fused IMUs
  were rejected -- per-device calibration (ADR 0027).
- **Production show timing uses deterministic site/date schedules from sparse time
  anchors, not panel-current dusk consensus:** four purchased SAM-M8Q modules are
  initial GPS/GNSS soft anchors for absolute UTC and four purchased Adafruit DS3231
  modules are initial RTC holdover anchors. ESP-NOW distributes time quality to the
  rest of the fleet, so all roughly 130 fixtures do not need RTCs (ADR 0031). Reception,
  energy, drift/backup behavior, final counts, schedule offsets, and invalid-time
  fallback remain open.
- **Power-management bus integrity: 100 kHz on any bus shared with the charger/gauge, never raised; dedicated bus on any custom PCBA (ADR 0028).** This closed the two-month reboot epidemic.
- **Build-week commission defaults to the listener posture (ADR 0039):** low-red
  ready beacon plus fresh/confident local ToF signature-color response, with
  bridge/direct commands overriding it. This supersedes ADR 0038's no-command
  rail-off default only; no-command still means no autonomous show, and all
  power/boot/OTA safety vetoes remain. Strict rail-off commission stays available
  for explicit rail-cycle diagnostics.
- LED electrical drive by role (ADR 0029 + 2026-07-11 amendment): BOTH LED roles on the switchable 3V3 rail -- the instrumented A/B through production-realistic cabling inverted the fat-wire VBAT result (rail +2.5 % mean, 22/25). One harness, one pinout; the rail is the hard kill; boost shelved with complete numbers.
- Noisemaker: solenoid mallet striking an installed finger cymbal on each of the
  72 canopy bamboo assemblies -- daytime solar-surplus percussion; the #3885
  speaker-synth path abandoned once strikes proved out (ADR 0030 + 0071).
- **Performance-audio source: received PUCA DSP Original Edition + Eurorack
  expansion + RODE VideoMic NTG is the primary optional bridge; CoreS3 + Module
  Audio remains the independent fallback (ADR 0035). Hardware is on hand but
  PUCA firmware is NOT implemented or field-validated.** Read
  `hardware/puca-audio-bridge/README.md`; do not mistake the factory Eurorack
  oscillator/effect image for Resonance firmware.
- **Camp network AP is pinned to the mesh channel (11), HT20, WPA2-PSK, on a
  dedicated 2.4 GHz SSID (ADR 0036).** One radio means WiFi STA and ESP-NOW share
  a channel and the AP picks it, so an auto-channel AP silently deafens any
  device that associates while on the mesh. Any Resonance device that associates
  while using ESP-NOW must read the actual channel after association and, on
  mismatch, **drop WiFi and keep the mesh** -- never the reverse. This does not
  apply to fixtures in OTA maintenance mode, which have already left ESP-NOW by
  design. Router ordered, not configured; runbook in
  `docs/howto/CAMP_NETWORK_SETUP.md`.
- **Fixture maintenance supports two site WiFi profiles (ADR 0066).** Real
  credentials remain in gitignored `firmware/fixture/wifi_secrets.h`; recipes,
  manifests, logs, and serial output use non-secret profile labels only. One
  scan ranks visible known APs by RSSI, with declaration-order fallback inside
  one bounded join budget. Ordinary COMMS never associates.
- **LFP power-policy thresholds (LED dim / off / sleep) are measured, not folklore -- read ADR 0023 before setting any battery floor in bench or production firmware.** It has the voltage-to-remaining-capacity map, the tiered thresholds, the hysteresis/load-compensation/coulomb-hybrid requirements, and the recipe to re-derive on a new cell or load.

**Open** (see TODO.md and ROADMAP.md):
- Camp network bring-up: Beryl AX ordered but not received or configured; the
  channel guard is specified but not implemented in any simultaneous
  mesh-plus-WiFi firmware. Distinct camp and art-site fixture maintenance
  profiles are implemented and fleet-promoted; second-site hardware association
  remains to be proven (ADR 0036 + ADR 0066).
- Claude mesh bridge handheld: direction recorded only. Hardware IS on hand
  (2x LilyGO **T-Deck Plus**, LCD variant + 1x M5Stack Cardputer ADV) but no
  firmware is written and no bring-up is done. T-Deck Plus is the primary target;
  do not port from **T-Deck Pro** documentation, which is a different device
  (e-paper, CST328 touch, TCA8418 keypad) whose drivers do not transfer.
  Class/spatial addressing still needs its own wire-format decision -- no group
  addressing exists today. The Plus's GPS is a noted but unadopted adjacency to
  the ADR 0031 time anchors; its 2000 mAh cell against a radio-RX-dominated
  always-on receiver is an open runtime question. Post-2026-event unless Ben
  re-prioritizes (ADR 0037).
- Rope attachment point: hat / bamboo / hybrid. Pending team input.
- Hat dimensions: placeholder, awaiting Vishnu input.
- Trunk-light integration: the production direction is about 16 mostly/all RGBW
  fixtures, with a smaller lensed 3 W RGB variant under test for extra throw. Final
  LED choice, power, mounting, enclosure, and sensor allocation remain open (ADR 0032).
- Chandelier light electronics scope/ownership (18 lights, internals fungible with
  the fleet -- ADR 0032) and its exact HEX/RGBW mix.
- ~~Noisemaker verdict~~ -- DECIDED (ADR 0030 + 0071): solenoid mallet into the
  installed canopy finger cymbal; the #3885 speaker path is abandoned. Open:
  voltage variant, strike power source, mounting, scope.
- Bottom-up nightly energy budget by role; MPPT policy.
- SAM-M8Q GPS and DS3231 RTC anchor qualification; final anchor counts/placement,
  power/backup strategy, time-quality protocol, schedule versioning, and invalid-time
  fallback (ADR 0031).
- Retired 2026-07-08: `INV_2026_00401` cost decomposition (invoice identity unclear
  -- probably the Bamboo Pure lantern invoice; no longer a useful baseline now that
  real procurement is recorded in `ops/PROCUREMENT.md`). The Community Mandala
  Program was pulled for time; gobos are now in-house + generative bamboo-leaf
  patterns (see BACKGROUND.md).

**Validated on hardware** (2026-06, PowerFeather V2 COTS bench -- see ADR 0021 +
`docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` + LOG 2026-06-07/08):
- **ESP-NOW networking** scales to ~100 fixtures (5-node bench ~99% PDR, clean rate-knee) and
  the radio reaches well past tree scale (held through a house + yard + oak, ~100 steps). The
  lantern enclosure is RF-transparent; the solar panel is the main ~20 dB attenuator (antenna
  keep-out matters). Note: the extrapolation was computed at 100 nodes; the fleet now plans
  about 130 -- re-running the projection at 130 is a queued TODO, with 150 still useful as a
  conservative stress case (physics gives margin, but the claim should say 100 until re-run).
- **Battery-only, no-touch OTA + A/B rollback** (the "never take a lantern off the tree"
  requirement): software-reset OTA recovered ~17/17 incl. worst-case LFP voltage; a
  self-test-failing image auto-reverts to last-good. Watchdog + autosleep recovery validated.
- **Solar charge path** end-to-end: net-positive into an LFP even in weak/partial light.

**Assumed** but not yet validated on hardware:
- The exact **nightly power budget** -- the old ~120 mAh/night napkin number is RETIRED
  (2026-07-02): pre-hardware math that crisp-gobo light levels invalidate. Derive
  bottom-up from measured LED draw (400-500 mA at full) x show duty cycle, then size
  cell/panel. Full-sun harvest number + LFP re-verify of the battery/stability runs
  still pending.
- WS2812B-from-Vbat on LiFePO4 -- superseded direction: LED axis is now direct-GPIO --
  data on a free GPIO (e.g. GPIO10/A0), V+ from the regulated switchable 3V3 header
  rail, deliberately NOT on the I2C/STEMMA bus the IS31 shared with the charger/gauge
  (ADR 0018). Note the rail is not stiff at show loads: the 2026-06-10 discharge
  measured ~2.96-2.97 V at the LED at ~290 mA (see LOG 2026-07-02).
- 1-3 LEDs at ~10% brightness gives the desired ambient look. Gobo + ambient tuning pending
  (note the 8-bit dimming low-end limit -- ADR 0018 / POWERFEATHER_NOTES).

## What this repo does NOT cover

- Bamboo lantern fabrication (Bamboo Pure / Vishnu, Bali).
- Tree structural design (Ed Wilkes, Bristol).
- Wind chime cluster electronics (separate workstream, Vishnu). Note: the 18
  chandelier *lights* are now a fleet class in this repo (ADR 0032);
  scope/ownership still being clarified with the team.
- Project-wide logistics, budget, container shipping (Elliot, Co-Work agent).
- The Resonance project's grant strategy / fundraising (Elliot).

If a task touches one of these areas, do not assume; ask Ben to relay it through the right channel.

## Style for this repo

- Markdown for everything that isn't code or CAD. Plain text, no emojis.
- Keep Markdown/docs ASCII-only unless there is a project-critical reason not to. Use
  `--`, `->`, `>=`, `<=`, `deg C`, `ohm`, `uA`, etc. instead of Unicode punctuation or
  symbols; Windows shells have repeatedly rendered those as mojibake.
- ASCII diagrams beat external image files. Easier to diff, easier for agents to read.
- Schematics-as-code via atopile. Layout in KiCad. No proprietary CAD source.
- Firmware split: platform-independent C++ in `firmware/core/` (compiles native, has unit tests), platform glue in `firmware/esp32/` (links to ESP-IDF / Arduino-ESP32). See `firmware/ARCHITECTURE.md`.
- ADRs are the contract for any decision worth remembering. Append, don't edit. Supersede with a new ADR.

## Cross-references with other tools

- **Project's WhatsApp threads** ("Resonance Tree", "Resonance Agentic Wiki") -- primary team comms, not directly accessible from this repo. Ben relays.
- **Co-Work agent** (Elliot's PM agent) -- maintains a separate wiki Co-Work syncs from WhatsApp + Fireflies meeting transcripts. Plan: get read access to Co-Work's wiki folder once Elliot has it cloud-hosted, then this repo's `LOG.md` and Co-Work's wiki cross-reference each other.
- **`beneckart/future-robotics`** GitHub repo -- Ben's prior Burning Man projects (Talisman v1/v2/v2rev2, Marquee, MaraudersMap, Winduino). Source of reusable code (TalismanPatterns.cpp, packet codec patterns) and lessons (see BACKGROUND.md "Lessons from 2018 Talisman v2" section).
- **Drive folder** with original Talisman v2 design docs and measured power numbers -- referenced in `BACKGROUND.md`.

## When in doubt

Ask Ben. Don't assume context. The project has a real timeline (BM 2026 ships in August) and real costs ($K of bamboo + electronics already in-flight). Wrong decisions are recoverable but expensive.
