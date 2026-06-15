# net_bench — ESP-NOW networking feasibility bench

Throwaway-friendly firmware to de-risk basing ~100 fixtures on PowerFeather V2 by
validating ESP-NOW comms, range, OTA, watchdog, and battery stability on 5 boards.
Forked from `../power_bench` (reuses telemetry, OTA `/update`, autosleep guard).
See `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` for the full test plan.

## What it does

- **Roles** (`--role master|peer`): the **master** broadcasts `SHOW_FRAME` commands
  AND WiFi-STA-joins the bench AP to **bridge per-peer stats to the host** over
  UDP:54321; **peers** run pure ESP-NOW on battery and broadcast `HEARTBEAT`
  (id, seq, battery, downlink-PDR/RSSI, CA state).
- **Broadcast-only, unencrypted** ESP-NOW (`FF:FF:FF:FF:FF:FF`) — the 100-node-
  scalable pattern (encrypted peers cap at ~17). Per-source seq tracking → PDR.
- **Maintenance mode**: master broadcasts an ESP-NOW `ENTER_MAINT` *metadata* packet
  → peers switch to WiFi-STA and serve `/update` + `/telemetry` for **standard WiFi
  OTA** (ADR 0010 — never firmware over ESP-NOW). Auto-resume timeout backstops.
- **Watchdog** (`esp_task_wdt`, net-new) + `--wdt-hangtest` to prove auto-recovery.
- **Autosleep** reboot-loop breaker (`--autosleep`) for unattended battery runs.

## CRITICAL: channel must match the AP

ESP-NOW only reaches nodes **on the same WiFi channel**, and the master's channel is
forced by the AP it joins. So **`--channel` MUST equal the bench AP's channel** on
every board. The master prints `WiFi.channel()` on boot and **warns loudly** on a
mismatch (validated: on a ch-11 AP with `--channel 6`, every send fails with
`Peer channel is not equal to the home channel`). Find the AP channel and build all
boards with that value (e.g. the home AP "BubbyNet" is on channel 11 → `--channel 11`),
or set a dedicated bench AP/router to a fixed channel.

## Build / flash

```
# all 5 boards on the AP's channel (example: AP on ch 11)
./build.sh --role master --channel 11 --port /dev/ttyACM0   # 1 master (also the host bridge)
./build.sh --role peer   --channel 11 --port /dev/ttyACM1   # 4 peers
# OTA (node must be in maintenance mode first):
./build.sh --role peer   --channel 11 --ota 192.168.4.61

# travel / client-isolated WiFi fallback:
# peer starts its own temporary AP in maintenance mode instead of joining bench WiFi
./build.sh --role peer   --channel 11 --maint-ap --port /dev/ttyACM1
```

Recover the IP/banner via the pyserial RTS pulse (native USB-CDC; see
`../POWERFEATHER_NOTES.md`).

### build.sh flags
`--role master|peer`, `--channel N` (= AP channel), `--frame-hz N` (master rate, 0 =
pure bridge), `--hb-hz N` (peer rate), `--jitter-pct N`, `--wdt-s N`, `--wdt-hangtest`,
`--maint-timeout S`, `--start-maint`, `--autosleep`/`--budget-mah`/`--wake-s`,
`--wifi-lowpower`, `--chem 3v7|lfp`, `--cap MAH`, `--charge-ma`/`--no-charge`/`--maintain`,
`--serial-bridge`, `--maint-ap`, `--scan-report`/`--scan-s S`/`--scan-max N`, `--batt-ntc` (battery
thermistor on charger TS — ONLY with the NTC physically taped to the cell, see
POWERFEATHER_NOTES), `--port`/`--ota`.

The live master command `m<v10>` sets charger maintain/VINDPM in volts x10 across the
fleet and accepts the PowerFeather SDK range 4.0-16.8 V, e.g. `m46` for 4.6 V or `m71`
for a 7.1 V panel MPP.

### Env sensors (MPP sweep: light + panel temp over the air)
A TSL2591 (lux, 0x29) and/or SHT31-D (temp/RH, 0x44) chained on the peer's STEMMA-QT
are **auto-probed at boot** (no build flag; one image serves sensored and bare boards)
and appended to the heartbeat: `lux=` (`sat` = saturated — full sun can exceed the
TSL2591's range even at min gain; a paper/PTFE diffuser fixes it, and the relative
normalization use survives the unknown attenuation), `ch0=`/`ch1=` raw counts,
`ptc=`/`prh=` (tape the SHT31 to the panel BACK ≈ cell temp), `btc=` (battery NTC,
needs `--batt-ntc`). Master `m<v10>` (e.g. `m48`) sets an explicit VINDPM for scripted
sweeps; bare `m` cycles presets. Host: `ops/bench/mpp_sweep.py` + `mpp_analyze.py`.

## Serial bridge + field scan-report (no laptop in the field)

Two modes that let you log the field fleet **from your desk over USB**, instead of
tethering a laptop in the yard (the WiFi range diagnostic, item (b) of
`docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md`). Why ESP-NOW and not WiFi-STA:
the ESP32-S3 is 2.4 GHz-only and won't hold an Eero association from the yard, but
ESP-NOW reached the back fence — so carry the data on ESP-NOW.

- **`--serial-bridge`** (a master): does **not** join WiFi — stays pinned to
  `--channel` and **relays everything it hears to USB serial** (the same `nb-master` /
  `nb-peer` / `nb-scanap` lines). A desk-tethered board thus logs the whole fleet.
- **`--scan-report`** (a field peer): periodically WiFi-**scans** 2.4 GHz (never
  associates → radio stays on `--channel` for ESP-NOW), and broadcasts the strongest
  `--scan-max` APs (BSSID/RSSI/channel/SSID) as `NB_SCANAP` packets. This maps which
  Eero node is reachable at what RSSI from anywhere in the yard — the coverage map +
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
pick** (a clean 2.4 GHz channel) — no coupling to the Eero's channel. Both boards must
share it. The **association/roaming** empirical test (does it actually cling to the far
node?) is deferred — `firmware/wifi_diag/` is the tethered probe for that.

## Travel maintenance AP (client-isolated networks)

Default maintenance mode is still the fleet path: peers join a normal bench/router WiFi
with client isolation disabled, so `net_bench_ota.py` can update many nodes in parallel.

For travel benches where the only available WiFi is client-isolated, build a field peer
with `--maint-ap`. Normal telemetry still rides ESP-NOW to the USB serial-bridge master.
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
mode whenever you have a non-isolated router and want parallel OTA.

## Onboard LED = battery level (GPIO46)

Every board shows its battery state-of-charge on the onboard user LED, so the wireless
fleet is readable at a glance: **>50% solid · 25-50% blink 1 Hz · 10-24% 2 Hz · <10%
4 Hz · no reading = off**. A **voltage cross-check** floors the level so a healthy cell
never shows "critical" — the MAX17260 SOC can read wildly wrong after a `DesignCap`
change / before a learn cycle (we saw a full 4.19 V cell report 1%); a loaded-Li-ion
voltage floor vetoes a false-low gauge reading. (Production low-battery logic should do
the same — never trust gauge SOC blindly.)

**Locate / identify:** the master can tell a specific board (or all) to blink a distinct
`..-` pattern for 8 s (overriding the battery display) so you can find it physically —
the data-center "chassis ID LED" pattern. Master serial `i` cycles through peers one at a
time (prints the ID it's pinging); `I` lights all peers at once. Forward-looking: a field
build would assign each fixture an install-time index (NVS) for a readable per-fixture
beacon; on-demand locate is the primitive that matters now.

## Serial commands (115200)
`t` telemetry · `r` report (role/mode/rate/txseq/sendok/fail/peers) · `+`/`-` step the
broadcast rate for a sweep (master broadcasts `SET_RATE` to peers) · `i` identify next
peer (locate, blinks `..-` 8 s) · `I` identify all peers · `u` master: announce
maintenance + enter · `c` resume · `x` watchdog hang test (needs `--wdt-hangtest`).

## Host tooling
- `ops/bench/net_bench_log.py` — capture the master's UDP bridge → JSONL (per-peer
  PDR/RSSI/reboots).
- `ops/bench/net_bench_ota.py` — parallel OTA to N IPs + auto-recovery verification.
  Use `--reboot comms` when OTA'ing peers (they reboot OFF WiFi into ESP-NOW, so the
  OTA "Update complete. Rebooting." ack + software reset is the success signal; confirm
  rejoin via the master bridge). Default `--reboot maint` verifies via `/telemetry`.
- `ops/bench/net_bench_summary.py` — per-peer + aggregate stats + scale extrapolation.
- `ops/bench/net_bench_serial_bridge.py` — relay a `--serial-bridge` board's USB serial
  to UDP:54321, so all the UDP tooling above works from a desk-tethered bridge (no
  laptop in the field). `net_bench_log.py` also logs `nb-scanap` coverage rows.

## Caveats
Battery runs are **Li-ion (`Generic_3V7`)** for now — *re-verify every stability finding
on LFP* (LFP's plateau parks on the buck-boost crossover, a harder regime). USB/pogo
stays the guaranteed recovery path. Bare-board RF only; hat antenna detuning is deferred
to Steve's mock-hat.
