# fixture -- production fleet firmware

One image for all four fixture classes (downlight / perimeter / trunk /
chandelier), extracted from the proven bench sketches. The current code/NVS enum
still calls the trunk role `uplight`; treat that as a compatibility name until the
manifest/schema rename is coordinated (ADR 0032). `net_bench` remains the desk
**bridge** build (master + serial bridge); this sketch is peer-only.

## Cambium direct control

`NB_DIRECT_FRAME` (type 25) carries up to 18 addressed RGBW entries per ESP-NOW
packet. A fixture renders only the entry matching its three-byte short MAC. Frames
grant an expiring micro-lease; after one second of silence the last color fades to
half, and after three seconds the fixture returns to its autonomous program with the
existing crossfade. `NB_FORCE_LIFECYCLE` (type 26) provides RAM-only day/night/auto
bench control and never survives reboot. The local power tier and night gate still
outrank requested colors.

The nominal 130-light fleet requires eight broadcast packets per complete color
wave. At Cambium's intended 8 Hz rate that is 64 packets/s, below the previously
exercised ESP-NOW rate range. The Nevada City acceptance bench uses perimeter
fixtures `F3FD88`, `F2BE80`, and `F2BFEC`.

## Transport sleep and rig RSSI capture

`NB_TRANSPORT_SLEEP` provides a 32-bit multi-day timer sleep. The bridge's
`Q<hours>` command (1-168) cuts fixture loads/rails and wakes automatically. The
timer wake restores radio/telemetry but retains an RTC-backed electrically-dark
output latch until a valid program command; bare bridge `b` is the intended
post-unload release. This is not physical ship mode and requires no lid access.

`NB_LOCATE_CONTROL` makes RSSI capture explicitly temporary. During bridge
`L[seconds]` only, a fixture retains up to 160 heard peers and sends its fresh
heard roster in 16-entry report fragments about every 20 seconds. Outside the
bounded window it emits no report traffic. See `ops/locate/rssi_capture.py` and
ADR 0045.

## Architecture stance

**Cooperative main loop + ISR-enqueue rx queue + esp_timer one-shots. No
application FreeRTOS tasks.** This deviates from ADR 0005 deliberately: it is
the only architecture with field hours behind it (net_bench soaks), and the
conservative reading of ADR 0028 rule 3 (no power-management-bus I2C from
core-0-pinned tasks under WiFi) plus the 2026-07-29 blocking-driver lesson
(cooperative TMF8820 one-shot machine; presence_bench's core-0 task quarantine
is forbidden on this bus). All Wire1 traffic runs at 100 kHz from loop context.
Revisit tasks only if render jitter is measured, and never for power-bus I2C.

> 2026-07-30 amendment (ADR 0028 addendum): rule 3 is downgraded -- the sealed
> A/B + 46 h soak both ran power-bus I2C from a core-0 task at 100 kHz, so the
> clock, not core placement, is the load-bearing rule. A single-owner sensor
> task (presence_bench shape, ALL Wire1 in one task) is now permitted pending
> a pre-playa field soak. Motivation is measured art quality: sensor-to-LED
> latency drives how alive the demo feels, and loop bursts visibly stutter
> max-speed orbit/spiral. Peer-to-peer CA impact TBD. The cooperative loop
> stays the shipping architecture until a task build earns its field hours.

## Layout

```
fixture.ino          setup()/loop() ordering only
src/core/            platform-independent, natively unit-tested (tests/)
  packet.h           wire protocol v1 + type registry (THE fleet contract)
  power_policy       ADR 0023/0046 tier ladder + compound PROTECT release
  boot_guard         POR/reboot-loop stage ladder (Phase-4 matrix)
  class_probe        sensors -> class decision table
  lifecycle          day/night machine, bounded night, energy-gated wake
  choreo/            program runtime: IDLE, GH_CA, BRIDGE_SHOW + lease
  neighbor_table     RSSI + pinned adjacency modes
  hex_geometry, filters, power_integrator, tmf_recovery
src/esp32/           glue/drivers (board_power owns the solar guard include)
  sensors/           cooperative machines + the single vendored VL53L5CX ULD
tests/run_tests.sh   native g++ suite (~200 checks) -- run before every flash
```

## Build / flash

```
./build.sh                          # compile only
./build.sh --port /dev/ttyACM0      # USB flash
./build.sh --ota <ip>               # OTA via POST /update
./build.sh --artifact-dir build/r1  # stable artifact for fleet_usb_bringup.py
./build.sh --fw-rev fx-YYMMDD-xxxxxxx-b  # override reported artifact identity
./build.sh --channel 11 --profile commission
./build.sh --channel 11 --profile commission --basic-listener
./build.sh --precharge-ma 300            # low-VBAT recovery current
./build.sh --wifi-source <gitignored-header>  # solenoid capability is universal
./build.sh --wifi-source <gitignored-header> --solenoid-test  # targeted bench image
```

For any shared/fleet artifact, the manual `fixture-YYYY-MM-DD.N` counter is
retired. Follow `../../docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md`: clean committed
source, generated `fx-YYMMDD-<recipe7>-<variant>`, immutable manifest, exact
binary SHA-256, and explicit target MACs. Build/manifest automation is pending;
until it lands, do not publish a new shared artifact under a legacy reused name.

Always `-DPOWERFEATHER_BOARD_V2=1` (build.sh injects it). Chemistry is
build-time (`--chem lfp` default); everything else is runtime NVS.
Solenoid capability is enabled by default on every PowerFeather image. The
one-time NVS policy migration enables devices that retained the historical
disarmed value; a later explicit runtime disarm remains persistent. D7/GPIO37
stays INPUT/high-Z while armed and idle, so rev-1 receiver/manual sources remain
usable and a Feather without a capboard has no connected load. A strike still
requires an addressed command or local input and retains the normal lifecycle,
power-tier, pulse-width, rest-time, maintenance, and failsafe gates.

`--canopy-solenoid` remains accepted as a deprecated no-op so older build recipes
do not fail; it is no longer required and must not be used to infer artifact
capability.

`--solenoid-test` is deliberately not a fleet option: it forces the arm bit and
relaxes only the daytime solar-surplus gate while retaining the night and FULL-
tier battery vetoes. Use a named artifact and a specific peer.

Rev-2 solarnoid SW1 shares D7 with the MCU through a hardware one-shot. An armed
fixture releases D7 to INPUT/high-Z between strikes. After observing a released
LOW, firmware accepts one external rising edge and extends it to the same bounded
40 ms pulse as the PowerFeather USER button; another strike requires release and
a new edge. Boot-high/stuck-high inputs do not fire or retrigger, normal MCU
requests refuse an already-high external line, and the timer plus loop failsafe
remain authoritative. External edges are ignored during OTA maintenance.

The default battery-side charge-current ceiling is 2,000 mA (ADR 0033). The
BQ25628E may deliver less because of input-current/voltage regulation, source
capability, system load, CV taper, or thermal regulation. `G<ma>` remains an
explicit lower override for a smaller or otherwise limited cell.

The BQ25628E precharge limit defaults to 300 mA (`--precharge-ma 300`). This
replaces the charger's 30 mA POR value, which left deeply discharged production
LFPs near 2.8 V treading water despite valid solar input. Firmware performs a
two-byte, little-endian, reserved-bit-preserving REG0x10 read/modify/write and
verifies the readback;
pending OTA images roll back if it does not match. Trickle charge below 2.25 V,
input DPM, thermal protection, and the hardware transition to fast charge near
3.0 V remain unchanged. Maintenance telemetry exposes `precharge_target_ma`,
`precharge_configured`, `bq_precharge_ma`, and raw `bq_reg10`.

Bringup: `fleet_usb_bringup.py commission --sketch-dir fixture --build-path
firmware/fixture/build/<r> --expect-fw <version> ...` -- the serial/HTTP
contract (`t` JSON keys, `u` + "maintenance WiFi up, ip=" banner, /telemetry,
/resume, /update) is preserved byte-for-byte from net_bench.

## Sensor-domain recovery

Every non-parked boot gives the separate VSQT/STEMMA rail a verified 100 ms
off -> on cycle before class probing. This matters on OTA and warm resets because
PowerFeather otherwise preserves the RTC-held rail state, allowing a stale TMF
firmware/ranging state to survive the MCU reset. `Board.enableVSQT()` is retried
and GPIO14 is read back for both transitions; Wire1 remains fixed at 100 kHz.

The normal TMF timeout path still performs the cheap cooperative stop/start.
Three consecutive failed measurement cycles escalate once per boot to a full
VSQT cycle and reconstruction of the SparkFun driver, which forces TMF init,
open, firmware upload, application-mode switch, configuration, and ranging
start. MSA311 and BMP581 are reinitialized because they share the rail. Further
failures stay degraded rather than flapping the domain. Telemetry exposes
`tmf_domain_resets` in addition to reads/errors/recoveries.

Commissioning likewise treats `tmf8820_present=true` plus zero reads as a
recoverable state: it performs one `S1` timed-sleep/VSQT reset and retests before
failing. `tmf8820_present=false` does not get that retry and remains the signal
to inspect the module, first cable, and power contacts.

## Serial commands (peer)

`t` telemetry JSON | `u` local ENTER_MAINT | `c` resume | `C<mah>` capacity
(reboots) | `G<ma>` charge cap | `K<id>:<ms>` solenoid (gated) | `S[<s>]`
deep sleep | `O<0-4>` class override | `F<0|1>` profile dev/prod | `N<0|1|2>`
force day/night/auto | `L0` force LED rail off until `L1` or reset | `L1` clear
the override and run the bench smoke render | `X` guarded bare-board
PROTECT clear | `r` status line

`X` works only with verified good USB/VDC, no plausible battery, charging off,
and no charger fault. It does not clear PROTECT automatically or over ESP-NOW;
`fleet_usb_bringup.py` uses it only in the default battery-absent workflow.

### Installed-battery PROTECT rescue

Do not start with `RESET-BOOT-RESET`, erase NVS, or use `X`. Leave the fixture's
LFP installed, connect the enclosure rescue USB port to a proven supply, and
observe telemetry over normal USB CDC. A valid battery, good external supply,
no charger fault, at least +20 mA charge current, and at least 3.25 V must all
hold continuously for 60 seconds before the durable PROTECT latch releases.

On `fx-260816-otafix1-b`, do not rely on the physical RESET button. A RESET
asserts a power-on-class reset; if the durable stage is still LEDS_OFF or DIM,
the cause-independent boot guard can correctly return to PROTECT. The observed
`F2BF5C` recovery held good USB and +340 mA charge long enough to climb
PROTECT -> LEDS_OFF -> DIM -> FULL, but the parked boot still left its LED rail
and sensors off. A deliberate software reboot at FULL then booted unparked,
initialized all three sensors, rejoined ESP-NOW, and returned to steady red.

The preferred rescue is to USB-install `fx-260816-prtrel1-b`; it replaces that
operator-timed software reboot with an automatic clean reboot immediately after
persisting the qualified release. The reboot is required because a parked boot
deliberately skipped rail cycling, class probe, sensor initialization, and LED
profiling. Use BOOT/download mode only if normal CDC never enumerates or the
ordinary USB flash tool cannot connect. Never erase NVS as a routine recovery;
it also destroys per-fixture configuration and bypasses the intended safety
qualification.

## NVS (namespace `resfx`)

`cap_mah` `chg_ma` `chg_policy` `class_ovr` `class_last` `fc_stage` `boots` `profile`
`batt_tier` `dim_mv/off_mv/slp_mv` `sol_en` `maint_v10` `channel`
`channel_policy` `night_max`.
The USB command `H<1..13>` persists a new ESP-NOW channel and reboots so the
radio is cleanly re-pinned; bare `H` reports the current channel.
First boot migrates `netbench:{cap_mah,chg_ma}` and carries a parked
`fc_led_stage` (production must not un-park a protected unit). Charge-policy v1
then replaces legacy 500/1,000/1,500 mA NVS values with the 2,000 mA default
once. A nonstandard pre-existing value is preserved as a possible deliberate
cell limit; later `G<ma>` overrides remain persistent.

Channel 11 is the production default. Channel-policy v1 migrates an absent key
or the historical channel-6 fallback to the compiled channel (11 for production)
once, while preserving any other explicit lab channel. Later `H<1..13>` choices
are marked current and remain persistent.

## Commission vs field profile

**Source-status note (2026-08-15):** this branch's current implementation is the
strict ADR 0038 commission-dark posture described below. ADR 0039 accepts
Elliot's later commission-listener extension (low red plus fresh/confident ToF
signature response) as the next default after integration and a newly identified
artifact. The two materially different builds both used
`fixture-2026-08-15.4`; that label cannot distinguish them and must not be reused.
Strict commission-dark remains required as an explicit rail-cycle diagnostic.

`PROFILE_DEV=0` retains its wire/NVS value for compatibility but is now the
operator-facing **commission** profile. It is the pre-build/build-week posture:
the ESP-NOW control plane stays awake, heartbeats run at 1 Hz, solar current does
not trigger a lifecycle transition, and bridge command leases are the only
artistic authority. No command (or a stale/expired command) means electrically
dark with the LED rail off -- never an automatic CA fallback. A genuinely
critical battery still parks all loads; its commission-mode retry is 60 s, and a
verified external source keeps the parked control plane awake for service.

`PROFILE_PROD=1` is the operator-facing **field** profile: 300 s/15 s day-charge
duty cycle (energy-gated), 0.2 Hz hb-short, scheduled/autonomous behavior, and the
normal 900 s PROTECT sleep. Local power and solenoid safety vetoes are identical
in both profiles; commission changes reachability and fallback behavior, not load
safety. Flip via `NB_PROFILE` (type 21) or per-unit serial `F` (`F0` commission,
`F1` field). New build flags accept `--profile commission|field`; `dev|prod` remain
compatibility aliases.

The supervised `--basic-listener` posture is deliberately minimal and class
aware. With no active bridge lease, canopy/downlights hold their dedicated warm
white channel at linear 128, 37-pixel perimeter HEX modules hold red at linear
16, and single-pixel RGB trunk/uplights hold red at linear 128. A bridge command
or dashboard tag overrides it, and stale-command fallback returns directly to
the class default within three seconds. LED channel values are linear: 0 is off
and 255 is the 8-bit bright endpoint. There is no gamma correction, boot salute,
external-supply carousel, identity pop, or local sensor-created color. The old
`--quiet-autonomy` option remains only as a build-script alias.

The 1 Hz commission heartbeat remains the compact 29-byte `hb-short`. A length-
gated full heartbeat follows every 5 s in commission and every 60 s in field. Its
append-only output tails report fixture class, raw sensor-signature bits, class
mismatch, LED-rail state, the post-cap RGBW average actually written to lit
pixels, lit-pixel count, and low-VBAT recovery state/BQ presence-test voltage.
The fleet dashboard uses measured render state rather than inferring color from
the requested program. Older bridges remain compatible because every tail is
length-gated against the one canonical `src/core/packet.h` layout.

An installed LFP in the 2.2-2.5 V window is no longer confused with a missing
battery solely by voltage. On a qualified external source the fixture runs the
BQ25628E's documented 30 mA BAT-discharge presence test with charging disabled.
Only a surviving battery ADC reading at or above 2.2 V permits a 100 mA recovery
ceiling; LED and sensor rails remain parked. A fault stops recovery, and one
continuous minute at or above 2.55 V restores the persisted normal charge cap.
See ADR 0042. Cells below 2.2 V remain a bench-recovery case.

Maintenance exit clears the ESP-NOW receive queue before reinitialization, so a
queued maintenance command cannot replay after `/resume`. If radio initialization
fails during the WiFi-to-ESP-NOW transition, the fixture remains in COMMS posture
and retries once per second until `espnow_up=true`. Telemetry exposes
`espnow_up`, `comms_init_attempts`, `comms_init_failures`, `led_rail_on`, and
`smoke_render` so an operator can distinguish radio recovery, command receipt,
power veto, and physical LED output.

## OTA rollback

`verifyRollbackLater()`/`verifyOta()` are `extern "C"` -- the weak hooks live
in a C file; a mangled C++ override silently never runs. Deferred self-test at
t+20 s (power chip, gauge sanity, mode-appropriate network path, NVS, watchdog)
marks the image valid or reboots into rollback. COMMS requires ESP-NOW plus a
completed send; MAINT requires associated WiFi plus the active OTA HTTP server
because ESP-NOW is deliberately down there. Drill: flash a
`--ota-fail-selftest` build over OTA
and watch it revert unattended. Rollback support requires one full USB flash
to install the current bootloader config (the M1 fleet reflash provides it).
Telemetry exposes `ota_partition`, `ota_address`, `ota_state`, and the legacy
`ota_pending_verify` boolean so same-family A/B transitions remain observable.

## Hardware-gate checklist (owed before fleet flash; see plan)

- [ ] `fleet_usb_bringup.py --sketch-dir fixture` full commission incl. --wifi-check
- [ ] appears on an UNMODIFIED net_bench serial-bridge master + dashboard
- [ ] per-class render smoke from one binary (probe log + `O` override + RGBW order)
- [ ] bench-supply power matrix (ADR 0046): 3.15 dim / 3.10 off+OTA window / 3.05 sleep;
      PROTECT survives POR/WDT/SW reset; compound release only; retry once
- [ ] OTA good-image valid + fail-selftest auto-revert on battery
- [ ] GH wave on the 2x10 rig via pinned adjacency (RSSI-mode A/B for the record)
- [ ] commission bridge lease grant plus command-loss hard dark/rail-off <= 5 s
- [ ] commission time-to-maintenance <= 10 s; field worst case one sleep period
- [ ] 24 h outdoor prod soak: dusk, bounded night (set night_max low to prove
      it fires), dawn, maintenance reachable in every state, night strike refused
