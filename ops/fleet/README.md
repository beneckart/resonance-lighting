# Fleet registry

This directory is the canonical identity record for production PowerFeather
fixtures.

## Identity model

- `fixture_id` is the final six hexadecimal digits of the ESP32 WiFi MAC, matching
  the ID used by `net_bench`.
- `mac` and `fixture_id` identify the electronics. COM ports, USB paths, WiFi IPs,
  fixture roles, and installation locations can change and are not identity.
- `registry.csv` is the compact current-state index. Keep one row per MAC.
- `callsigns.csv` is the operator-friendly alias table. It assigns one permanent
  callsign to every production-health fixture and retains unassigned spare names.
  The short MAC remains the device identity for logs, OTA, flashing, persistence,
  and every other safety-sensitive operation.
- `bringup/*.jsonl` is append-only commissioning evidence. It may contain several
  observations for the same board.

## Fixture callsigns

Callsigns make field conversation readable (`Luigi is green`) without changing
the ESP-NOW wire contract or the MAC-derived identity model.

- Callsigns are unique case-insensitively, ASCII-only, one word, and 3-7
  characters.
- `assignment=assigned` rows map the 141 commissioned or commission-failed
  production-health PowerFeathers. `assignment=spare` rows reserve the remaining
  names for future fixtures.
- A callsign stays bound to its short MAC and is never silently reassigned or
  reused after retirement. Unknown and non-production devices continue to display
  their short MAC.
- Operator displays may lead with the callsign, but detail and confirmation
  surfaces show both, for example `Luigi [F98CEF]`.
- Firmware artifacts and state-changing maintenance jobs continue to name explicit
  short MAC targets as required by ADR 0040.

The initial allocation was a deterministic shuffle of the reviewed 160-name pool
using the seed `resonance-tree-callsigns-v1`, followed by assignment to the
numerically sorted production-health roster. That explains the initial mapping;
it is not a regeneration rule. The checked-in table is authoritative from this
point forward.

Do not assign a ring position merely from USB-port order. Physical role and
installation location remain blank until the fixture is deliberately labeled and
assigned.

## One-off fixture protection

Fixture `F40344` / `68:EE:8F:F4:03:44` is the one-of-a-kind NeoHex Magic Wand.
Its registry role is `magic_wand`, which keeps it visible as a fleet peer while
marking it as dedicated hardware. `ops/bench/fleet_dashboard_ota.py` refuses a
protected role in an ordinary fleet batch. Updating the wand requires the exact
short MAC again via `--allow-special-target F40344`, and the wand must be the
only target. That acknowledgement does not relax artifact identity, battery
ride-through, or post-pending-verify evidence requirements.

## Commissioning profile

The first production-board profile is:

- PowerFeather V2 / ESP32-S3
- `Generic_LFP`
- 6,000 mAh gauge capacity
- 500 mA charge-current cap (historical packing artifact; superseded by ADR 0033)
- shared-WiFi maintenance on the `WonkyHouse` profile
- firmware `net-bench-2026-07-27.3`
- guarded D7/GPIO37 solenoid support

All 26 California-bench fixtures received this profile on 2026-07-27. USB and
serial verification passed, but WonkyHouse was not locally visible, so the registry
correctly leaves `ota_verified=false` until the first Tennessee association,
`/telemetry`, and `/resume` check.

The next fleet reflash uses ADR 0033's 2,000 mA battery-side ceiling. Both
`fixture` and `net_bench` carry a one-time NVS policy migration, because an
application reflash alone does not erase the historical `chg_ma=500` value.

Fixture `F2BFA0` is the first Tennessee enclosure exception to the common image:
it now runs the opt-in `net-bench-2026-07-29.4` sensor-triad diagnostic. Targeted
OTA, MSA311/TMF8820/BMP581 maintenance telemetry, `/resume`, and sustained
ESP-NOW rejoin all passed on 2026-07-29. The other 25 fixtures remain on the
uniform `.3` packing image.

Capacity can be changed without a firmware rebuild. From a `net_bench` master or
serial bridge, `C6000` broadcasts and persists a 6,000 mAh capacity, while
`C<fixture_id>:6000` targets one peer. The board reboots so the gauge model can be
re-applied. The battery chemistry is still selected at build time.

## USB batch tool

`ops/bench/fleet_usb_bringup.py` recognizes the ESP32-S3 native USB VID/PID,
records MAC-derived identities, performs an `esptool flash-id` preflight, uploads
an already-built Arduino artifact, and verifies serial telemetry. `--wifi-check`
also makes each peer join the configured shared WiFi, validates `/telemetry`, and
requests `/resume`.

The qualified Sabrent/Anker bench default is 12 simultaneous flash/preflight/serial
workers, which gives two waves per 24-fixture ring. WiFi verification is independently
capped at four simultaneous transitions; an eight-way WiFi transition stress produced
one transient Windows COM error even though all eight uploads and serial checks passed.

The tool never compiles. Build one named artifact first, then reuse the exact build
directory:

```powershell
python ops/bench/fleet_usb_bringup.py inventory `
  --out ops/fleet/bringup/2026-07-27-ca-usb-stage.jsonl

python ops/bench/fleet_usb_bringup.py commission `
  --build-path firmware/net_bench/build/fleet-tn-wonkyhouse-20260727-r1 `
  --expect-fw net-bench-2026-07-27.3 `
  --expect-count 12 --pending-only --max-parallel 12 --wifi-check --wifi-parallel 4 `
  --ota-profile WonkyHouse `
  --out ops/fleet/bringup/2026-07-27-ca-usb-stage.jsonl --append
```

The session log exclusive-creates by default. `--append` is required to continue
the same physical bring-up session. Bare-board charging-off verification remains
the default; use `--allow-battery-present` only for a deliberate mixed batch with an
installed battery.
