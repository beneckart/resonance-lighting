# Networking feasibility test — 5x PowerFeather V2 (ESP-NOW)

**Date:** 2026-06-07
**Status:** Live — harness built + bench-validated on 1 board; collecting multi-node data.
**Owners:** Ben (ca); Steve (tn — mock-hat antenna RF follow-up).

## Why

The project plans to base ~100 autonomous bamboo fixtures on the PowerFeather V2
(ESP32-S3). Before committing the money, de-risk the **networking + radio + stability**
axis on the 5 boards in hand. The repo had mature power telemetry + WiFi OTA but **no
ESP-NOW** — this builds the first ESP-NOW firmware (`firmware/net_bench/`) and a host
harness to answer: *is ESP-NOW + PowerFeather V2 reliable and power-feasible enough to
buy 100?* Respects ADR 0004 (ESP-NOW = control plane only) and ADR 0010 (OTA = standard
WiFi, never mesh firmware gossip; USB/pogo = recovery path).

## Topologies & axes

- **Master-multicast**: a master node broadcasts `SHOW_FRAME` commands; peers receive.
  Measured by the peer-reported downlink PDR (`dl_pdr`).
- **Peer broadcast mesh**: every peer broadcasts `HEARTBEAT`; all receive (the CA
  neighbor model). Measured by the master's uplink PDR per peer.
- Both run on **unencrypted broadcast** (`FF:FF:…`) — the scalable pattern (encrypted
  peers cap at ~17). The master is WiFi-STA on the bench AP (locked channel), bridging
  per-peer stats to the host over UDP:54321 so the wireless peer fleet is observable
  from one tether.

| Axis | Values |
|---|---|
| Topology | master-multicast, peer-mesh |
| Rate sweep | per-node 1 / 2 / 5 / 10 / 20 / 50 Hz (aggregate 5–250 pkt/s at 5 nodes) |
| Range | bench tree-limb scale 1–6 m; open-field drop-off sweep (~30 m+); through-obstruction |
| Power | Li-ion JST-PH (`Generic_3V7`), true wireless. **LFP re-verify deferred** (see caveats) |

## Firmware build (`firmware/net_bench/build.sh`)

`-DPOWERFEATHER_BOARD_V2=1` always. **`--channel` MUST equal the AP channel** (the master
adopts the AP's channel on association; a mismatch silently kills ESP-NOW — the master
warns). Example (home AP on ch 11):

```
./build.sh --role master --channel 11 --port /dev/ttyACM0
./build.sh --role peer   --channel 11 --port /dev/ttyACM1   # x4
```

`--wdt-hangtest` enables the induced-hang command; `--autosleep` the overnight battery
guard; `+`/`-` serial keys sweep the rate live (master broadcasts `SET_RATE`).

## Run procedure

1. Find the bench AP channel; flash all 5 boards with that `--channel` (1 master, 4 peers).
2. Confirm the bridge: `ops/bench/net_bench_log.py --site ca --battery liion-XXXX
   --topology master-multicast --tx-rate 10 --duration <s>` → per-peer PDR/RSSI lines,
   reboots flagged inline. JSONL → `ops/bench/data/ca/<run-id>.jsonl`.
3. Run each test T0–T8 (below); use `+`/`-` on the master to sweep rates without reflashing.
4. OTA cycle: master `u` (announce maintenance) → peers join AP → `net_bench_ota.py
   --bin <net_bench.ino.bin> --nodes pf1=ip,… --jobs 5` → verify all `recovered:true`.
5. `git add` the JSONL; `net_bench_summary.py` prints per-peer stats + the scale knee.

## Host tooling

- **`ops/bench/net_bench_log.py`** — UDP:54321 master-bridge parser → site-partitioned
  JSONL (per-peer `pdr`, `dl_pdr`, `rssi_dbm`, `dl_rssi_dbm`, `reset_reason`, battery,
  reboot flag). Run-id `<date>-<site>-<battery>-net-<topology>-<rate>hz-<HHMM>`.
- **`ops/bench/net_bench_ota.py`** — parallel OTA (`ThreadPoolExecutor`) to N IPs +
  per-node `t_ack`/`t_ready`/`recovered`/`button_press_required` (the field-reset proof).
- **`ops/bench/net_bench_summary.py`** — per-peer + aggregate PDR/RSSI/reboots + scale
  extrapolation (fit PDR vs aggregate offered rate → loss knee → safe node count).
- Reuse `power_logger.py` for maintenance-mode `/telemetry`; `power_summary.py` for battery.

## Metrics & acceptance targets ("good enough to buy 100")

| Metric | Target |
|---|---|
| Uplink + downlink PDR (tree scale 1–6 m) | **>99%** every peer over a multi-hour run; <95% any peer = fail |
| RSSI margin | p10 RSSI **≥ −70 dBm** (≥20 dB over the ~−90 floor) at 6 m; record the open-field cliff |
| ESP-NOW send-failure rate | **< 0.5%** at the production rate |
| Reboots / resets | **zero unexplained** over a ≥4 h Li-ion run; any `brownout`/`panic` = fail *(LFP re-verify)* |
| Battery drain | record mAh/h for autonomy sizing *(LFP re-verify)* |
| OTA | **5/5 success + auto-recover, NO physical button** (any button = fail) |
| Watchdog | recovers an induced hang with no human (`reset_reason=task_watchdog`) — **VALIDATED** |

**Scale extrapolation:** the dominant risk is shared-channel airtime/collision, which
scales with aggregate offered rate = per-node rate × node count. Sweep the rate at 5
nodes → find the PDR loss knee → 100 nodes is safe iff `100 × prod_rate < knee` with
margin. Small-N caveat: recommend a 20+ node confirmation run if the prod point is within
~2× of the knee.

## Test matrix

| # | Test | Varied | Measured | Acceptance | Power |
|---|---|---|---|---|---|
| T0 | Bring-up / identity | — | all peers visible, seq increments, bridge reaches host | 5/5 visible | USB |
| T1 | Master-multicast PDR/latency vs rate (1–6 m) | tx 1–50 Hz | dl_pdr, send-fail | PDR>99% at prod rate; find knee | battery peers |
| T2 | Peer-mesh aggregate loss vs rate → scale | per-node 1–50 Hz | uplink PDR (worst peer) | identify loss knee | battery |
| T3 | Range sweep / cliff (open field) | 1–40+ m | RSSI & PDR vs distance | ≥20 dB margin thru 6 m; record cliff | battery |
| T4 | Through-obstruction | body/bamboo/foil/battery-behind-antenna | RSSI & PDR drop | quantify; flag PDR<95% | battery |
| T5 | Parallel OTA + maintenance cycle | jobs=5 vs 1 | t_ack, t_ready, recovered | 5/5 + auto-recover, no button | USB/battery |
| T6 | Multi-hour battery stability | long soak | reboots, mAh/h, brownouts | zero unexplained resets ≥4 h | battery |
| T7 | Master WiFi+ESP-NOW coexistence | idle vs heavy bridge | master current, stability | master stable, no resets | USB master |
| T8 | Induced-hang watchdog | hang | WDT fires? recovers? | recovers, no human — **VALIDATED** | battery |

## Results

### Bench validation (2026-06-07, board 9E5B0C, single board)
- Peer boots, `Board.init Ok`, **ESP-NOW up on ch 6**, heartbeats broadcasting (txseq/
  sendok increment, 0 failures). Watchdog initialized (handles core's pre-init).
- **T8 watchdog — PASS**: `x` induces hang → task WDT fires at 8 s → reboot → clean
  re-init; post-reboot `reset_reason=task_watchdog`. No human, no button.
- **Bridge + host capture — PASS**: master joins WiFi, `net_bench_log.py` captures
  `nb-master`/`nb-peer` lines to JSONL with full metadata.
- **Channel-lock — CONFIRMED real**: home AP "BubbyNet" is ch 11; with `--channel 6` the
  master warns and every send fails (`Peer channel is not equal to the home channel`).
  Fix = build all boards `--channel 11`.

### Multi-node (5 boards) — PENDING
T0–T7 to be run once 5 boards are flashed on a matched channel. Tables below to be filled:
per-peer PDR/RSSI, OTA ack/ready, rate-sweep knee, multi-hour reboot count.

## Known issues / caveats

- **Li-ion ≠ LFP (asterisk on everything battery):** all stability/current/brownout
  results are on `Generic_3V7`. LFP's ~3.2–3.3 V plateau parks on the TPS631013
  buck-boost crossover (a harder regime for current spikes) — stable-on-Li-ion is
  **necessary but not sufficient**; re-run every battery test on LFP before the 100-buy.
- **Channel coexistence:** master STA adopts the AP channel; `--channel` must equal it or
  ESP-NOW silently dies. Pin it; the master asserts `WiFi.channel()` and warns.
- **Broadcast is unacked:** delivery is measured at the receiver (seq-gap PDR), not the
  send callback. Per-send jitter is mandatory at scale.
- **RSSI is approximate:** use for margin trends / obstruction deltas, not absolute budget.
- **5-node small-N:** collision/hidden-node at 100 isn't linearly inferable; the rate-sweep
  knee is a screen, not a guarantee.
- **WiFi-poll confound:** keep the master on USB and use the UDP bridge (not `/telemetry`
  polling) for clean peer autonomy numbers.
- **Bare-board antenna:** hat detuning (panel/battery/metal) deferred to Steve's mock-hat
  (COTS Phase 7).
- **Latency** not measured in v1 (single-hop sub-frame; PDR/RSSI are the gating metrics).

## Next

- Run T0–T7 on 5 boards (matched channel) → fill Results → set the production heartbeat
  rate below the measured knee.
- LFP re-verify pass once Steve's cell holders/connectors exist.
- 20+ node confirmation run if the knee is near the production point.
- Mock-hat antenna RF with panel/battery installed (Steve; ties to COTS Phase 7).
- Promote the go/no-go into **ADR 0021 — ESP-NOW networking feasibility / 100-fixture
  commit** once data is in.
