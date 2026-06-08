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
```

Recover the IP/banner via the pyserial RTS pulse (native USB-CDC; see
`../POWERFEATHER_NOTES.md`).

### build.sh flags
`--role master|peer`, `--channel N` (= AP channel), `--frame-hz N` (master rate, 0 =
pure bridge), `--hb-hz N` (peer rate), `--jitter-pct N`, `--wdt-s N`, `--wdt-hangtest`,
`--maint-timeout S`, `--start-maint`, `--autosleep`/`--budget-mah`/`--wake-s`,
`--wifi-lowpower`, `--chem 3v7|lfp`, `--cap MAH`, `--charge-ma`/`--no-charge`/`--maintain`,
`--port`/`--ota`.

## Onboard LED = battery level (GPIO46)

Every board shows its battery state-of-charge on the onboard user LED, so the wireless
fleet is readable at a glance: **>50% solid · 25-50% blink 1 Hz · 10-24% 2 Hz · <10%
4 Hz · no reading = off**.

## Serial commands (115200)
`t` telemetry · `r` report (role/mode/rate/txseq/sendok/fail/peers) · `+`/`-` step the
broadcast rate for a sweep (master broadcasts `SET_RATE` to peers) · `u` master: announce
maintenance + enter · `c` resume · `x` watchdog hang test (needs `--wdt-hangtest`).

## Host tooling
- `ops/bench/net_bench_log.py` — capture the master's UDP bridge → JSONL (per-peer
  PDR/RSSI/reboots).
- `ops/bench/net_bench_ota.py` — parallel OTA to N IPs + auto-recovery verification.
  Use `--reboot comms` when OTA'ing peers (they reboot OFF WiFi into ESP-NOW, so the
  OTA "Update complete. Rebooting." ack + software reset is the success signal; confirm
  rejoin via the master bridge). Default `--reboot maint` verifies via `/telemetry`.
- `ops/bench/net_bench_summary.py` — per-peer + aggregate stats + scale extrapolation.

## Caveats
Battery runs are **Li-ion (`Generic_3V7`)** for now — *re-verify every stability finding
on LFP* (LFP's plateau parks on the buck-boost crossover, a harder regime). USB/pogo
stays the guaranteed recovery path. Bare-board RF only; hat antenna detuning is deferred
to Steve's mock-hat.
