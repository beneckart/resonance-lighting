# net_bench -- ESP-NOW networking feasibility bench

Throwaway-friendly firmware to de-risk basing ~100 fixtures on PowerFeather V2 by
validating ESP-NOW comms, range, OTA, watchdog, and battery stability on 5 boards.
Forked from `../power_bench` (reuses telemetry, OTA `/update`, autosleep guard).
See `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` for the full test plan.

## What it does

- **Roles** (`--role master|peer`): the **master** broadcasts `SHOW_FRAME` commands
  AND WiFi-STA-joins the bench AP to **bridge per-peer stats to the host** over
  UDP:54321; **peers** run pure ESP-NOW on battery and broadcast `HEARTBEAT`
  (id, seq, battery, downlink-PDR/RSSI, CA state).
- **Broadcast-only, unencrypted** ESP-NOW (`FF:FF:FF:FF:FF:FF`) -- the 100-node-
  scalable pattern (encrypted peers cap at ~17). Per-source seq tracking -> PDR.
- **Maintenance mode**: master broadcasts an ESP-NOW `ENTER_MAINT` *metadata* packet
  -> peers switch to WiFi-STA and serve `/update` + `/telemetry` for **standard WiFi
  OTA** (ADR 0010 -- never firmware over ESP-NOW). Auto-resume timeout backstops.
  Peers now report a power preflight before leaving ESP-NOW; by default this is
  advisory so the low-voltage OTA boundary can be measured instead of guessed.
- **Watchdog** (`esp_task_wdt`, net-new) + `--wdt-hangtest` to prove auto-recovery.
- **Autosleep** reboot-loop breaker (`--autosleep`) for unattended battery runs.

## CRITICAL: channel must match the AP

ESP-NOW only reaches nodes **on the same WiFi channel**, and the master's channel is
forced by the AP it joins. So **`--channel` MUST equal the bench AP's channel** on
every board. The master prints `WiFi.channel()` on boot and **warns loudly** on a
mismatch (validated: on a ch-11 AP with `--channel 6`, every send fails with
`Peer channel is not equal to the home channel`). Find the AP channel and build all
boards with that value (e.g. the home AP "BubbyNet" is on channel 11 -> `--channel 11`),
or set a dedicated bench AP/router to a fixed channel.

## Build / flash

```
# all 5 boards on the AP's channel (example: AP on ch 11)
./build.sh --role master --channel 11 --port /dev/ttyACM0   # 1 master (also the host bridge)
./build.sh --role peer   --channel 11 --port /dev/ttyACM1   # 4 peers
# OTA (node must be in maintenance mode first):
./build.sh --role peer   --channel 11 --ota 192.168.4.61

# deprecated/emergency single-board fallback only:
# peer starts its own temporary AP in maintenance mode instead of joining bench WiFi
./build.sh --role peer   --channel 11 --maint-ap --port /dev/ttyACM1
```

Recover the IP/banner via the pyserial RTS pulse (native USB-CDC; see
`../POWERFEATHER_NOTES.md`).

### build.sh flags
Default maintenance OTA requires `wifi_secrets.h`. `build.sh` reuses an existing
gitignored secrets file from `../power_bench/` or `../led_studio/` when
`firmware/net_bench/wifi_secrets.h` is absent.

`build.sh` uses a unique temporary Arduino `--build-path` per run to avoid the ESP32
Arduino cache collision seen when compiling multiple variants in parallel. If you call
`arduino-cli` directly, also provide a unique `--build-path` or build variants
sequentially.

`--role master|peer`, `--channel N` (= AP channel), `--frame-hz N` (master rate, 0 =
pure bridge), `--hb-hz N` (peer rate), `--jitter-pct N`, `--wdt-s N`, `--wdt-hangtest`,
`--maint-timeout S`, `--start-maint`, `--autosleep`/`--budget-mah`/`--wake-s`,
`--wifi-lowpower`, `--chem 3v7|lfp`, `--cap MAH`, `--charge-ma`/`--no-charge`/`--maintain`,
`--serial-bridge`, `--maint-ap`, `--scan-report`/`--scan-s S`/`--scan-max N`,
`--field-cycle`/`--field-charge-s S`/`--field-wait-s S`/`--field-protect-s S`,
`--batt-ntc` (battery
thermistor on charger TS -- ONLY with the NTC physically taped to the cell, see
POWERFEATHER_NOTES), `--port`/`--ota`.

Maintenance-entry power preflight defaults to report-only (`NB_MAINT_POWER_ENFORCE=0`).
The advisory thresholds are `NB_MAINT_MIN_LFP_MV=3200`, `NB_MAINT_MIN_3V7_MV=3600`,
and `NB_MAINT_SUPPLY_OVERRIDE_MA=250`. A peer below the advisory floor sends one last
ESP-NOW heartbeat with `mt=2` before attempting maintenance, and the dashboard shows
`OTA power warn`. Only build with `-DNB_MAINT_POWER_ENFORCE=1` after the real lower
bound is measured.

The live master command `m<v10>` sets charger maintain/VINDPM in volts x10 across the
fleet and accepts the PowerFeather SDK range 4.0-16.8 V, e.g. `m46` for 4.6 V or `m71`
for a 7.1 V panel MPP.

The live master command `S` parks peers in timed deep sleep for 6 hours by default
(`S900` = 15 minutes). Peers cut the switchable 3V3/STEMMA rails before sleeping and
wake by timer, preserving no-touch recoverability for the next maintenance window.

The live master command `R<hz>` directly sets the ESP-NOW heartbeat/frame rate across
the fleet, e.g. `R1` for low-overhead charging telemetry or `R5` for faster bench
feedback. This changes transmission cadence only; the peer remains awake. For a bigger
charge-rate win on a near-empty solar peer, use targeted park.

The live master command `P<id>[:seconds]` parks one peer in timed deep sleep while other
peers keep running, e.g. `P9E5AB8:3600` gives the outdoor solar peer a one-hour charge
nap without stopping an indoor HEX drawdown. Peers cut switchable rails, wake by timer,
and rejoin ESP-NOW.

The live master command `D<id>[:mah]` starts a **targeted HEX drawdown** on one peer,
then the peer blanks the pixels and timed-sleeps when it reaches the mAh budget or the
LFP voltage guard. Example: `D9E5AF0:3500` lights the 37px HEX on GPIO10 at the
bench-default load, integrates discharge current, stops around 3500 mAh or the guarded
voltage floor, and sleeps 12 hours so the battery is still hungry for a next-day solar
run. Use the target id whenever more than one peer is online.

Battery capacity and charger current are runtime bench config. Bare commands are fleet
broadcasts; include a peer id to target one node:

- `C<mah>` stores battery capacity in all peers' NVS, then peers reboot so the next
  `Board.init()` uses the new gauge capacity. Example: `C6000` for a fleet of 32700
  LFP cells. `C<id>:<mah>` targets one peer, e.g. `C9E5AB8:6000`.
- `G<mA>` stores and live-applies the charger current cap across the fleet. Example:
  `G1500` for supervised 6 Ah solar runs. `G<id>:<mA>` targets one peer, e.g.
  `G9E5AB8:1500`. Valid range is 40-2000 mA.

The dashboard intentionally sends capacity and charge changes only to the selected peer;
All view refuses those actions so a row click cannot accidentally become a fleet command.

Chemistry is still a build-time flag (`--chem lfp|3v7`) because that controls charge
voltage and is safety-critical. The `--cap` and `--charge-ma` build flags remain useful
as first-boot defaults; NVS overrides win after a command.

Shared-WiFi maintenance can be fleet-wide or targeted. Bare `U` remains the sustained
fleet `ENTER_MAINT` wake and is also the migration path for old peers that do not yet
understand targeted maintenance. `U<id>` sends sustained targeted maintenance to one
peer, e.g. `U9E5AB8`, so a single OTA does not pull drawdown or solar-cycle peers off
ESP-NOW. The dashboard's `Peer maint` button sends `U<id>` for the selected peer.

## Field-cycle day/night lifecycle mode

`--field-cycle` is the first production-ish lifecycle test mode. It keeps the normal
ESP-NOW heartbeat, supply telemetry, shared-WiFi OTA, and serial-bridge dashboard, but
adds an autonomous peer state machine:

1. If external supply/solar is present, the peer enters `charge`, emits telemetry, cuts
   both switchable rails, and timer-sleeps in chunks while the charger works.
2. When voltage/current indicate "full enough" (`NB_FIELD_FULL_MV` plus taper current),
   it enters `wait-dark` and keeps sleeping/checking until supply disappears.
3. In dark/no-supply, it enters `draw` and stays awake at the field-cycle heartbeat rate
   (`NB_FIELD_DRAWDOWN_HZ`, default 1 Hz). This intentionally uses the always-awake radio
   load as a repeatable night drawdown. `--field-led-load` optionally adds a direct-GPIO
   HEX/SK6812 load during this draw phase; tune it with `--drawdown-lit` and
   `--drawdown-brightness`.
4. At the critical LFP floor it sleeps immediately. At the soft floor it now requires
   `NB_FIELD_LOW_CONFIRM_S` seconds of sustained low voltage first, so one transient
   telemetry sample does not end the night. Before sleeping it sends final heartbeats,
   blanks pixels, cuts both rails, and enters `protect` timer sleep. The next
   solar/USB/supply window wakes it on a timer, it sees supply, starts the next cycle,
   and resumes charging.

Solar does not need to electrically wake the ESP32 in the normal case: the charger keeps
charging while the ESP32 is in timer deep sleep, and the next timer wake observes the
supply. This is more recoverable than deliberately running the cell into protection.

Example peer build for the current BubbyNet/channel-11 field rig:

```
./build.sh --role peer --channel 11 --field-cycle \
  --field-charge-s 300 --field-wait-s 300 --field-protect-s 900 \
  --field-wake-ms 8000 --field-cold-ms 30000 \
  --field-low-mv 3150 --field-critical-mv 3050 --field-low-confirm-s 30 \
  --field-led-load --drawdown-lit 18 --drawdown-brightness 128 \
  --chem lfp --cap 6000 --charge-ma 1500 --maintain 4.6
```

The heartbeat tail adds `fc=` (phase), `fcr=` (reason), `fcc=` (cycle), `fce=` (phase
elapsed seconds), `fcchg=`/`fcdis=` (rough charge/discharge mAh integrated from sampled
battery current), and `fcmin=`/`fcmax=` (cycle voltage bounds). The dashboard and
`net_bench_log.py` parse these fields.

`net-bench-2026-07-01.1` adds field-cycle v2 summaries while keeping the heartbeat at
128 bytes for old-bridge compatibility:

- `fcwhc=` / `fcwhd=`: cycle charge/discharge Wh x10 from sampled battery power.
- `fcpw=`, `fcbw=`, `fcdw=`: peak panel, battery-charge, and battery-draw W x100.
- `fclow=`: current soft-low debounce seconds, capped at 255.
- `fcmchg=`, `fcmwait=`, `fcmdraw=`, `fcmprot=`: time spent in each field-cycle phase,
  in minutes, capped at 255. The `/telemetry` JSON exposes the same phase totals in
  seconds for an OTA-maintenance check.

`net-bench-2026-06-30.7` also appends BQ25628E charger telemetry for low-VBAT
solar/USB rescue debugging:

- `bqv=` VINDPM in mV, `bqichg=` charge-current limit in mA, `bqvreg=` CV limit in mV.
- Raw BQ bytes: `bq16`, `bq18`, `bq1d`, `bq1e`, `bq1f`, `bq20`, `bq21`, `bq22`,
  and `bq38`.
- The dashboard/log derive the most useful bits: `bq_chg_en`, `bq_en_hiz`,
  `bq_batfet_ctrl`, `bq_vbus_stat`, and `bq_chg_stat`, while preserving raw bytes for
  later decode.

### Env sensors (MPP sweep: light + panel temp over the air)
A TSL2591 (lux, 0x29) and/or SHT31-D (temp/RH, 0x44) chained on the peer's STEMMA-QT
are **auto-probed at boot** (no build flag; one image serves sensored and bare boards)
and appended to the heartbeat: `lux=` (`sat` = saturated -- full sun can exceed the
TSL2591's range even at min gain; a paper/PTFE diffuser fixes it, and the relative
normalization use survives the unknown attenuation), `ch0=`/`ch1=` raw counts,
`ptc=`/`prh=` (tape the SHT31 to the panel BACK ~ cell temp), `btc=` (battery NTC,
needs `--batt-ntc`). Master `m<v10>` (e.g. `m48`) sets an explicit VINDPM for scripted
sweeps; bare `m` cycles presets. Host: `ops/bench/mpp_sweep.py` + `mpp_analyze.py`.

## Serial bridge + field scan-report (no laptop in the field)

Two modes that let you log the field fleet **from your desk over USB**, instead of
tethering a laptop in the yard (the WiFi range diagnostic, item (b) of
`docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md`). Why ESP-NOW and not WiFi-STA:
the ESP32-S3 is 2.4 GHz-only and won't hold an Eero association from the yard, but
ESP-NOW reached the back fence -- so carry the data on ESP-NOW.

- **`--serial-bridge`** (a master): does **not** join WiFi -- stays pinned to
  `--channel` and **relays everything it hears to USB serial** (the same `nb-master` /
  `nb-peer` / `nb-scanap` lines). A desk-tethered board thus logs the whole fleet.
- **`--scan-report`** (a field peer): periodically WiFi-**scans** 2.4 GHz (never
  associates -> radio stays on `--channel` for ESP-NOW), and broadcasts the strongest
  `--scan-max` APs (BSSID/RSSI/channel/SSID) as `NB_SCANAP` packets. This maps which
  Eero node is reachable at what RSSI from anywhere in the yard -- the coverage map +
  the "a stronger node was available" half of the missed-roam story, streamed wirelessly.

```
# desk bridge (USB to your PC):
./build.sh --role master --channel 11 --serial-bridge --port /dev/ttyACM0
# field board (battery, walked); scans every 15 s:
./build.sh --role peer --channel 11 --scan-report --scan-s 15 --hb-hz 1 --port /dev/ttyACM1
# on the PC: relay serial -> UDP so the usual loggers work, then log:
ops/bench/net_bench_serial_bridge.py --port /dev/ttyACM0   &
ops/bench/net_bench_log.py --site ca --notes "yard 2.4GHz coverage"
```

Channel note: because the field board never associates, `--channel` is **yours to
pick** (a clean 2.4 GHz channel) -- no coupling to the Eero's channel. Both boards must
share it. The **association/roaming** empirical test (does it actually cling to the far
node?) is deferred -- `firmware/wifi_diag/` is the tethered probe for that.

## Emergency single-board maintenance AP

Default maintenance mode is still the fleet path: peers join a normal bench/router WiFi
with client isolation disabled, so `net_bench_ota.py` can update many nodes in parallel.

For one-off recovery where the only available WiFi is client-isolated, build a single
field peer with `--maint-ap`. Treat this as deprecated unless Ben explicitly asks for it.
It is intentionally **not** the normal bench or fleet OTA mode: each peer creates its own
AP, so updates are serialized and the laptop has to switch networks. Normal telemetry
still rides ESP-NOW to the USB serial-bridge master. The fleet-scalable path is
shared-WiFi/portable-router maintenance plus `net_bench_ota.py` parallel uploads.
When the master sends `U` (sustained `ENTER_MAINT`), the peer leaves ESP-NOW and starts a
temporary AP named `ResonanceMaint-<nodeid>` with password `resonance`; the peer serves
`/`, `/telemetry`, `/resume`, and `/update` at `http://192.168.4.1/`.

```
# USB desk bridge (keeps relaying telemetry; no WiFi dependency):
./build.sh --role master --channel 11 --serial-bridge --port /dev/ttyACM0

# Field peer: solar/INA/env telemetry over ESP-NOW, self-AP only during maintenance:
./build.sh --role peer --channel 11 --maint-ap --chem lfp --cap 6000 --port /dev/ttyACM1

# Later, on the bridge serial console: press `U`.
# Then connect the laptop to WiFi SSID ResonanceMaint-<nodeid> / password resonance.
# OTA from that temporary AP:
./build.sh --role peer --channel 11 --maint-ap --chem lfp --cap 6000 --ota 192.168.4.1
```

This AP mode is intentionally one-peer-at-a-time. Use the default shared-WiFi maintenance
mode whenever you want parallel OTA.

## Onboard LED = battery level (GPIO46)

Every board shows its battery state-of-charge on the onboard user LED, so the wireless
fleet is readable at a glance: **>50% solid * 25-50% blink 1 Hz * 10-24% 2 Hz * <10%
4 Hz * no reading = off**. A **voltage cross-check** floors the level so a healthy cell
never shows "critical" -- the MAX17260 SOC can read wildly wrong after a `DesignCap`
change / before a learn cycle (we saw a full 4.19 V cell report 1%); a loaded-Li-ion
voltage floor vetoes a false-low gauge reading. (Production low-battery logic should do
the same -- never trust gauge SOC blindly.)

**Locate / identify:** the master can tell a specific board (or all) to blink a distinct
`..-` pattern for 8 s (overriding the battery display) so you can find it physically --
the data-center "chassis ID LED" pattern. Master serial `i` cycles through peers one at a
time (prints the ID it's pinging); `I` lights all peers at once. Forward-looking: a field
build would assign each fixture an install-time index (NVS) for a readable per-fixture
beacon; on-demand locate is the primitive that matters now.

## Serial commands (115200)
`t` telemetry * `r` report (role/mode/rate/txseq/sendok/fail/peers) * `+`/`-` step the
broadcast rate for a sweep (master broadcasts `SET_RATE` to peers) * `i` identify next
peer (locate, blinks `..-` 8 s) * `I` identify all peers * `U[peerid]` master:
sustained maintenance wake, bare = fleet and `U9E5AB8` = one peer * `c` resume *
`m<v10>` set VINDPM * `S[seconds]` timed deep-sleep peers (bare `S` = 6 h) *
`D<id>[:mah]` targeted HEX drawdown + sleep * `C<mah>` set capacity and reboot peers *
`G<mA>` set charge-current cap * `x` watchdog hang test (needs
`--wdt-hangtest`).

Additional solar-charge helpers: `R<hz>` sets the heartbeat/frame rate directly, and
`P<id>[:seconds]` parks one selected peer without sleeping the rest of the bench.

## Host tooling
- `ops/bench/net_bench_log.py` -- capture the master's UDP bridge -> JSONL (per-peer
  PDR/RSSI/reboots, firmware revision, and maintenance status when present).
- `ops/bench/net_bench_ota.py` -- parallel OTA to N IPs + auto-recovery verification.
  Use `--reboot comms` when OTA'ing peers (they reboot OFF WiFi into ESP-NOW, so the
  OTA "Update complete. Rebooting." ack + software reset is the success signal; confirm
  rejoin via the master bridge). Default `--reboot maint` verifies via `/telemetry`.
- `ops/bench/net_bench_summary.py` -- per-peer + aggregate stats + scale extrapolation.
- `ops/bench/net_bench_serial_bridge.py` -- relay a `--serial-bridge` board's USB serial
  to UDP:54321, so all the UDP tooling above works from a desk-tethered bridge (no
  laptop in the field). `net_bench_log.py` also logs `nb-scanap` coverage rows.

- `ops/bench/net_bench_dashboard.py` -- local browser dashboard for a USB
  `--serial-bridge` master. It owns the serial port, serves `http://127.0.0.1:8765/`,
  shows live solar/battery/RF telemetry, writes safe controls back to the bridge
  (`m<v10>`, targeted `U<id>`, `c`, identify, refresh), and can still forward `nb-*` lines to
  UDP:54321 for the older tools. Travel example:
  `python ops/bench/net_bench_dashboard.py --port COM7`.

## Caveats
Battery runs are **Li-ion (`Generic_3V7`)** for now -- *re-verify every stability finding
on LFP* (LFP's plateau parks on the buck-boost crossover, a harder regime). USB/pogo
stays the guaranteed recovery path. Bare-board RF only; hat antenna detuning is deferred
to Steve's mock-hat.
