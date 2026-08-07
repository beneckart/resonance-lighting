# fixture -- production fleet firmware

One image for all four fixture classes (downlight / perimeter / uplight /
chandelier), extracted from the proven bench sketches. `net_bench` remains the
desk **bridge** build (master + serial bridge); this sketch is peer-only.

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
  power_policy       ADR 0023 tier ladder + compound PROTECT release
  boot_guard         POR/reboot-loop stage ladder (Phase-4 matrix)
  class_probe        sensors -> class decision table
  lifecycle          day/night machine, bounded night, energy-gated wake
  choreo/            program runtime: IDLE, GH_CA, BRIDGE_SHOW + lease
  neighbor_table     RSSI + pinned adjacency modes
  hex_geometry, gamma, filters, power_integrator
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
./build.sh --channel 11 --profile prod
```

Always `-DPOWERFEATHER_BOARD_V2=1` (build.sh injects it). Chemistry is
build-time (`--chem lfp` default); everything else is runtime NVS.

The default battery-side charge-current ceiling is 2,000 mA (ADR 0033). The
BQ25628E may deliver less because of input-current/voltage regulation, source
capability, system load, CV taper, or thermal regulation. `G<ma>` remains an
explicit lower override for a smaller or otherwise limited cell.

Bringup: `fleet_usb_bringup.py commission --sketch-dir fixture --build-path
firmware/fixture/build/<r> --expect-fw <version> ...` -- the serial/HTTP
contract (`t` JSON keys, `u` + "maintenance WiFi up, ip=" banner, /telemetry,
/resume, /update) is preserved byte-for-byte from net_bench.

## Serial commands (peer)

`t` telemetry JSON | `u` local ENTER_MAINT | `c` resume | `C<mah>` capacity
(reboots) | `G<ma>` charge cap | `K<id>:<ms>` solenoid (gated) | `S[<s>]`
deep sleep | `O<0-4>` class override | `F<0|1>` profile dev/prod | `N<0|1|2>`
force day/night/auto | `L<0|1>` bench smoke render | `r` status line

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

## Dev vs prod profile

Dev (default in this image, `RES_PROFILE_DEFAULT`): no daytime deep sleep,
1 Hz heartbeats, 60 s dusk/dawn confirms, solenoid surplus-gate relaxed.
Prod: 300 s/15 s day-charge duty cycle (energy-gated: any solar/USB surplus
keeps the radio fully awake), 0.2 Hz hb-short, 30 min dusk confirm.
PROTECT behavior is identical in both -- dev cannot weaken battery protection.
Flip fleet-wide via `NB_PROFILE` (type 21) or per-unit serial `F`.

## OTA rollback

`verifyRollbackLater()`/`verifyOta()` are `extern "C"` -- the weak hooks live
in a C file; a mangled C++ override silently never runs. Deferred self-test at
t+20 s (power chip, gauge sanity, radio, NVS, watchdog) marks the image valid
or reboots into rollback. Drill: flash a `--ota-fail-selftest` build over OTA
and watch it revert unattended. Rollback support requires one full USB flash
to install the current bootloader config (the M1 fleet reflash provides it).

## Hardware-gate checklist (owed before fleet flash; see plan)

- [ ] `fleet_usb_bringup.py --sketch-dir fixture` full commission incl. --wifi-check
- [ ] appears on an UNMODIFIED net_bench serial-bridge master + dashboard
- [ ] per-class render smoke from one binary (probe log + `O` override + RGBW order)
- [ ] bench-supply power matrix: 3.00 dim / 2.95 off+OTA window / 2.90 sleep;
      PROTECT survives POR/WDT/SW reset; compound release only; retry once
- [ ] OTA good-image valid + fail-selftest auto-revert on battery
- [ ] GH wave on the 2x10 rig via pinned adjacency (RSSI-mode A/B for the record)
- [ ] bridge lease grant/expiry fallback <= 5 s, no blank frames
- [ ] dev time-to-maintenance <= 10 s; prod worst case one sleep period
- [ ] 24 h outdoor prod soak: dusk, bounded night (set night_max low to prove
      it fires), dawn, maintenance reachable in every state, night strike refused
