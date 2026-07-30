# fixture -- production fleet firmware

One image for all four fixture classes (downlight / perimeter / uplight /
chandelier), extracted from the proven bench sketches. `net_bench` remains the
desk **bridge** build (master + serial bridge); this sketch is peer-only.

## Architecture stance

**Cooperative main loop + ISR-enqueue rx queue + esp_timer one-shots. No
application FreeRTOS tasks.** This deviates from ADR 0005 deliberately: it is
the only architecture with field hours behind it (net_bench soaks), and the
conservative reading of ADR 0028 rule 3 (no power-management-bus I2C from
core-0-pinned tasks under WiFi) plus the 2026-07-29 blocking-driver lesson
(cooperative TMF8820 one-shot machine; presence_bench's core-0 task quarantine
is forbidden on this bus). All Wire1 traffic runs at 100 kHz from loop context.
Revisit tasks only if render jitter is measured, and never for power-bus I2C.

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

`cap_mah` `chg_ma` `class_ovr` `class_last` `fc_stage` `boots` `profile`
`batt_tier` `dim_mv/off_mv/slp_mv` `sol_en` `maint_v10` `channel` `night_max`.
First boot migrates `netbench:{cap_mah,chg_ma}` and carries a parked
`fc_led_stage` (production must not un-park a protected unit).

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
