# Networking feasibility test -- 5x PowerFeather V2 (ESP-NOW)

**Date:** 2026-06-07
**Status:** Live -- harness built + bench-validated on 1 board; collecting multi-node data.
**Owners:** Ben (ca); Steve (tn -- mock-hat antenna RF follow-up).

## Why

The project plans to base ~100 autonomous bamboo fixtures on the PowerFeather V2
(ESP32-S3). Before committing the money, de-risk the **networking + radio + stability**
axis on the 5 boards in hand. The repo had mature power telemetry + WiFi OTA but **no
ESP-NOW** -- this builds the first ESP-NOW firmware (`firmware/net_bench/`) and a host
harness to answer: *is ESP-NOW + PowerFeather V2 reliable and power-feasible enough to
buy 100?* Respects ADR 0004 (ESP-NOW = control plane only) and ADR 0010 (OTA = standard
WiFi, never mesh firmware gossip; USB/pogo = recovery path).

## Topologies & axes

- **Master-multicast**: a master node broadcasts `SHOW_FRAME` commands; peers receive.
  Measured by the peer-reported downlink PDR (`dl_pdr`).
- **Peer broadcast mesh**: every peer broadcasts `HEARTBEAT`; all receive (the CA
  neighbor model). Measured by the master's uplink PDR per peer.
- Both run on **unencrypted broadcast** (`FF:FF:...`) -- the scalable pattern (encrypted
  peers cap at ~17). The master is WiFi-STA on the bench AP (locked channel), bridging
  per-peer stats to the host over UDP:54321 so the wireless peer fleet is observable
  from one tether.

| Axis | Values |
|---|---|
| Topology | master-multicast, peer-mesh |
| Rate sweep | per-node 1 / 2 / 5 / 10 / 20 / 50 Hz (aggregate 5-250 pkt/s at 5 nodes) |
| Range | bench tree-limb scale 1-6 m; open-field drop-off sweep (~30 m+); through-obstruction |
| Power | Li-ion JST-PH (`Generic_3V7`), true wireless. **LFP re-verify deferred** (see caveats) |

## Firmware build (`firmware/net_bench/build.sh`)

`-DPOWERFEATHER_BOARD_V2=1` always. **`--channel` MUST equal the AP channel** (the master
adopts the AP's channel on association; a mismatch silently kills ESP-NOW -- the master
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
   --topology master-multicast --tx-rate 10 --duration <s>` -> per-peer PDR/RSSI lines,
   reboots flagged inline. JSONL -> `ops/bench/data/ca/<run-id>.jsonl`.
3. Run each test T0-T8 (below); use `+`/`-` on the master to sweep rates without reflashing.
4. OTA cycle: master `u` (announce maintenance) -> peers join AP -> `net_bench_ota.py
   --bin <net_bench.ino.bin> --nodes pf1=ip,... --jobs 5` -> verify all `recovered:true`.
5. `git add` the JSONL; `net_bench_summary.py` prints per-peer stats + the scale knee.

## Host tooling

- **`ops/bench/net_bench_log.py`** -- UDP:54321 master-bridge parser -> site-partitioned
  JSONL (per-peer `pdr`, `dl_pdr`, `rssi_dbm`, `dl_rssi_dbm`, `reset_reason`, battery,
  reboot flag). Run-id `<date>-<site>-<battery>-net-<topology>-<rate>hz-<HHMM>`.
- **`ops/bench/net_bench_ota.py`** -- parallel OTA (`ThreadPoolExecutor`) to N IPs +
  per-node `t_ack`/`t_ready`/`recovered`/`button_press_required` (the field-reset proof).
- **`ops/bench/net_bench_summary.py`** -- per-peer + aggregate PDR/RSSI/reboots + scale
  extrapolation (fit PDR vs aggregate offered rate -> loss knee -> safe node count).
- Reuse `power_logger.py` for maintenance-mode `/telemetry`; `power_summary.py` for battery.

## Metrics & acceptance targets ("good enough to buy 100")

| Metric | Target |
|---|---|
| Uplink + downlink PDR (tree scale 1-6 m) | **>99%** every peer over a multi-hour run; <95% any peer = fail |
| RSSI margin | p10 RSSI **>= -70 dBm** (>=20 dB over the ~-90 floor) at 6 m; record the open-field cliff |
| ESP-NOW send-failure rate | **< 0.5%** at the production rate |
| Reboots / resets | **zero unexplained** over a >=4 h Li-ion run; any `brownout`/`panic` = fail *(LFP re-verify)* |
| Battery drain | record mAh/h for autonomy sizing *(LFP re-verify)* |
| OTA | **5/5 success + auto-recover, NO physical button** (any button = fail) |
| Watchdog | recovers an induced hang with no human (`reset_reason=task_watchdog`) -- **VALIDATED** |

**Scale extrapolation:** the dominant risk is shared-channel airtime/collision, which
scales with aggregate offered rate = per-node rate x node count. Sweep the rate at 5
nodes -> find the PDR loss knee -> 100 nodes is safe iff `100 x prod_rate < knee` with
margin. Small-N caveat: recommend a 20+ node confirmation run if the prod point is within
~2x of the knee.

## Test matrix

| # | Test | Varied | Measured | Acceptance | Power |
|---|---|---|---|---|---|
| T0 | Bring-up / identity | -- | all peers visible, seq increments, bridge reaches host | 5/5 visible | USB |
| T1 | Master-multicast PDR/latency vs rate (1-6 m) | tx 1-50 Hz | dl_pdr, send-fail | PDR>99% at prod rate; find knee | battery peers |
| T2 | Peer-mesh aggregate loss vs rate -> scale | per-node 1-50 Hz | uplink PDR (worst peer) | identify loss knee | battery |
| T3 | Range sweep / cliff (open field) | 1-40+ m | RSSI & PDR vs distance | >=20 dB margin thru 6 m; record cliff | battery |
| T4 | Through-obstruction | body/bamboo/foil/battery-behind-antenna | RSSI & PDR drop | quantify; flag PDR<95% | battery |
| T5 | Parallel OTA + maintenance cycle | jobs=5 vs 1 | t_ack, t_ready, recovered | 5/5 + auto-recover, no button | USB/battery |
| T6 | Multi-hour battery stability | long soak | reboots, mAh/h, brownouts | zero unexplained resets >=4 h | battery |
| T7 | Master WiFi+ESP-NOW coexistence | idle vs heavy bridge | master current, stability | master stable, no resets | USB master |
| T8 | Induced-hang watchdog | hang | WDT fires? recovers? | recovers, no human -- **VALIDATED** | battery |

## Results

### Bench validation (2026-06-07, board 9E5B0C, single board)
- Peer boots, `Board.init Ok`, **ESP-NOW up on ch 6**, heartbeats broadcasting (txseq/
  sendok increment, 0 failures). Watchdog initialized (handles core's pre-init).
- **T8 watchdog -- PASS**: `x` induces hang -> task WDT fires at 8 s -> reboot -> clean
  re-init; post-reboot `reset_reason=task_watchdog`. No human, no button.
- **Bridge + host capture -- PASS**: master joins WiFi, `net_bench_log.py` captures
  `nb-master`/`nb-peer` lines to JSONL with full metadata.
- **Channel-lock -- CONFIRMED real**: home AP "BubbyNet" is ch 11; with `--channel 6` the
  master warns and every send fails (`Peer channel is not equal to the home channel`).
  Fix = build all boards `--channel 11`.

### Multi-node first light (2026-06-07, 1 master + peers, ch 11, Li-ion)
- **T0 PASS** for the boards that came up: master + **3 peers** at first power-on (one of
  the flashed peers never started -- a silent no-boot, NOT caught by the watchdog since it
  never entered loop(); likely the documented post-flash boot flakiness or a flat cell).
- **Early T1 point** (10 Hz, co-located): uplink & downlink **PDR ~99.5%**, RSSI -25 to
  -33 dBm, master `sendok` with **0 send-fail**. Clean `poweron` boots.
- **T5 parallel OTA -- effectively PASS**: deployed a new firmware (battery-level LED,
  v07.2) to the master + 2 reachable peers via the maintenance-mode cycle. All recovered
  via **software reset, no physical button** (master verified by `/telemetry`; peers
  verified by rejoining ESP-NOW with `rr=software`). Note: `net_bench_ota.py` initially
  false-FAILED the peers because they reboot OFF WiFi into comms -- fixed with `--reboot
  comms` (the OTA ack + software reset is the success signal; confirm rejoin via bridge).
- **Brownout finding (de-risk):** the low-SOC peer (`9F2690`, ~4% / 3.44 V) **dropped out
  when entering maintenance mode** -- the WiFi-association inrush on a nearly-empty Li-ion
  cell is exactly the brownout failure mode. Implication at 100x: gate OTA/maintenance on
  sufficient SOC, or rely on the autosleep guard; a depleted fixture can't be field-updated
  until charged. *(Re-verify on LFP -- different converter regime.)*

### Board inventory + fuel-gauge finding (2026-06-07)
Mapped board ID <-> battery via the new **identify/locate** command (master `i` blinks a
board's `..-` pattern): master `9E5B0C`=2200, `9F2690`/`9E5AB8`=4400, `9E5AF0`/`9F26F8`=
10050 mAh. OTA'd each with its correct `--cap` (all recovered, no button).

**MAX17260 DesignCap must be set once + needs a learn cycle.** Changing `--cap`
re-initializes the gauge and **resets learned SOC -> a transient bad reading**: `9F26F8`
(10050) read 27% at 3.73 V with the wrong cap=2000, then **100% at 3.72 V** right after
re-seeding to 10050 (true ~50%). So neither raw reading is trustworthy until the gauge
re-converges. Production implications: **set DesignCap once at first boot, charge the cell
to full to anchor 100%, and let the gauge learn over a charge/discharge cycle**; never
change cap in the field. This is *more* critical on LFP (flat OCV -> the gauge can't lean
on voltage, so a correct cap + coulomb learning is the only SOC source). Folds into T6
prep: fully charge every cell before the autonomy/drain run.

### T1/T2 rate sweep -- PASS (2026-06-07, master + 4 peers, ch 11, co-located, Li-ion)
Swept the broadcast rate 1->50 Hz (`ops/bench/net_bench_ratesweep.py`), aggregate offered
load = (4 peers + master) x rate. **Aggregate uplink PDR stayed >=97% across the whole
range** with a clean, gentle airtime trend (no collapse / no knee):

| rate | aggregate | samples | agg PDR |
|---|---|---|---|
| 1 Hz | 5 pkt/s | 113 | 100.0% |
| 2 Hz | 10 | 260 | 99.6% |
| 5 Hz | 25 | 619 | 99.7% |
| 10 Hz | 50 | 1279 | 99.5% |
| 20 Hz | 100 | 2401 | 99.1% |
| 50 Hz | 250 | 6434 | 97.2% |

Linear fit `loss ~ 1.05e-4 x pkt/s + 0.06%` -> **extrapolation to 100 nodes**: @1 Hz/node
(100 pkt/s) ~ **98.9% PDR**, @2 Hz (200 pkt/s) ~ **97.8%**, @5 Hz (500 pkt/s) ~ 94.7%.
Downlink (master->peer) `dl_pdr` held ~0.99 until 50 Hz (~0.985). RSSI -23 to -42 dBm
(co-located). **Verdict: ESP-NOW broadcast comfortably supports 100 fixtures at a sane
heartbeat rate (1-2 Hz/node) with ~98-99% PDR** -- and lighting tolerates partial loss.
*Note:* worst-*peer* PDR is noisy at low rates (one lost packet of ~60 = 98%), so the knee
must be read off **aggregate** loss, not worst-peer (the tool now does this).

Caveats: 5-node small-N can't reproduce 100-node hidden-node/capture effects (screen, not
guarantee -- recommend a 20+ node confirm if the chosen prod rate is high); co-located
(range/obstruction derate is T3/T4); Li-ion -- re-verify on LFP.

### T3/T4 preliminary (2026-06-07, incidental) -- PASS
Two peers placed ~25 ft away **through a wall + a large metal storage box**, two next to
the master. All 4 still seen at 10 Hz: near pair -28/-32 dBm @ 100% PDR; **far pair
-67/-71 dBm @ 98.7% PDR** (dl_pdr ~0.988). The obstruction cost ~35-40 dB but left ~20 dB
margin over the ~-90 dBm floor -- no dropouts. This is *harsher* than the real install
(lanterns see ~20 ft of open air + bamboo, no walls/metal), so it bodes well. Still TODO:
a proper open-field cliff sweep (find the actual drop-off distance) and controlled
single-obstruction deltas (body/bamboo/foil).

### T4 obstruction mapping (2026-06-08, via identify/locate) -- informative
Placed each peer in a different obstruction and used the identify blink to label which is
which. **Settled re-capture** (25 s median +/- spread; the first single-snapshot pass caught
some boards mid-placement and read 8-17 dB off -- see the RSSI-variability note):

| obstruction | board | RSSI (median) | spread | PDR |
|---|---|---|---|---|
| 3D-printed lantern cylinder (board inside) | 9E5AB8 | -23 dBm | 3 dB | 100% |
| ceramic coffee cup | 9F2690 | -33 dBm | 8 dB | 100% |
| glass+metal **solar panel** on a box | 9F26F8 | -43 dBm | 2 dB | 99% |
| metal laptop in a metal+glass cabinet | 9E5AF0 | -48 dBm | 4 dB | 99% |

**Two findings for the build:** (1) the **lantern enclosure is ~RF-transparent** (-23 dBm
with the board *inside* the printed cylinder, the least-attenuated of all) -- the housing
won't detune/block the mesh; (2) the **solar panel (glass+metal) is a real attenuator
(~20 dB)** -- it sits over the antenna in the hat, so this is the antenna-keepout concern made
concrete. Even so it held 99% PDR at bench range. Deployment worst case = panel attenuation
+ full tree distance stacked -> the mock-hat RF test (Steve, COTS Phase 7).

**RSSI variability caveat (important):** absolute RSSI is NOT a repeatable per-placement
constant indoors -- it's dominated by multipath, so the "same placement" can read 10-20 dB
differently if anything in the room changes (board orientation, a person, a door). Within a
25 s window each board was steady (2-8 dB spread), but readings shifted 8-17 dB vs a sweep
taken minutes earlier (partly mid-placement, partly multipath drift). Treat RSSI as an
approximate topology signal, not distance (per ADR 0004). For clean obstruction *deltas*,
measure baseline vs obstruction back-to-back on the same board/spot/minute -- don't compare
across times.

### T3 range walk (2026-06-08) -- PASS (out-and-back, ~100 steps through house+yard)
Walked the cup board (`9F2690`) slowly from the office out the back door, across the yard to
the fence (behind a big oak), and back the same path; the 3 stationary boards logged as
controls. Tooling: `ops/bench/net_bench_walk.py` (continuous per-peer RSSI/PDR log) +
`net_bench_walk_plot.py` (the V plot) + live landmark markers. Data + graph:
`ops/bench/data/ca/2026-06-08-rangewalk.{jsonl,png}` (+ `-markers.jsonl`).

Result -- a clean V/bathtub: -19 dBm (office) -> broad floor **-80 to -87 dBm** at the
fence/oak (a few brief dropouts behind the trunk; loss clustered entirely there) -> back to
-30 on return. Findings: (1) **the house doorway dominated the loss** (~50 dB in the first
~30 steps) while 60 steps of open yard added little; (2) **the oak trunk caused the deepest
dips/dropouts**, recovering a few dB at the fence past it; (3) **RSSI is path-asymmetric**
(doorway -69 out vs -47 back -- multipath/orientation, not repeatable); (4) **the 3 reference
boards stayed flat** the whole 5.5 min -> the walker's swing is real, environment stable.
The link **held through a house door + full backyard + behind an oak (~100 steps)** --
marginal at the bottom but mostly connected. That path is *far* harsher than the tree
(fixtures see ~20 ft of open air + bamboo, no house/doorway), so this is a strong
range result for the deployment. (Open-field clean-LoS cliff distance still un-measured --
this run was through-the-house; the doorway masked the pure-distance falloff.)

### Remaining matrix (5 boards) -- PENDING
Optional: open-field clean-LoS cliff (no house in the path) to get pure distance falloff;
T5 already PASS (parallel OTA); T6 multi-hour drain (cells charged + correct caps done);
T7 master coexistence; mock-hat RF (panel+battery installed, Steve).

## Deployment notes (Q&A 2026-06-08)

- **Link "budget":** ~-90 dBm is the *receiver sensitivity floor* (where ESP-NOW packets
  start failing at ESP32's low rates), not a budget. Margin = measured RSSI - (-90). At the
  fence we had only ~3-10 dB (RSSI -80..-87), hence the loss there. Unused headroom levers:
  full TX power was already on (~+19 dBm, not the 8.5 dBm low-power mode), and ESP32's
  **ESP-NOW long-range (LR) mode (~-110 dBm sensitivity, +15-20 dB reach)** is available if
  range ever matters.
- **Lanterns on posts beyond the tree diameter (entrance path idea):** fine -- open-air LoS
  is far easier than the house+yard+oak walk that held ~100 steps. The mesh is peer-to-peer
  with nearest-neighbor awareness, so each post only needs to hear *some* neighbor (the next
  post / the tree), not a central point; spaced posts chain naturally. Watch only for an
  isolated post with no neighbor in range; a far-flung centralized-controller topology would
  want a multi-hop relay (future lever).
- **No reboots/brownouts during the range walk** -- verified two ways: the master's per-source
  `rx` for the walked board was strictly monotonic (a reboot resets the peer's seq -> rx would
  snap to ~1; it never did), and current uptime is ~54 min / `reset_reason=software` (the OTA),
  with the 5.5-min walk entirely inside it. Healthy Li-ion at 4.14 V; none of our brownout
  triggers (marginal/low cell, LFP buck-boost crossover, IS31-on-I2C-bus) apply here. The
  walk logger now records `reset_reason`/`uptime`/`seq` + flags reboots explicitly. (The
  low-cell / multi-hour-drain brownout case is still T6/LFP, untested.)
- **Security posture -- open by design.** We use unencrypted ESP-NOW broadcast (the 100-node
  scalable pattern; encrypted-peer table caps at ~6-17). No encryption needed for BM (a
  prankster hijacking the show is welcome). If a future public/festival/grant install needs
  tamper-resistance, do it at the **app layer** (sign/authenticate broadcast payloads, command
  allowlist/rolling code), NOT ESP-NOW per-peer crypto. Deferred; record in ADR 0021.

## Known issues / caveats

- **The V-graph's synchronized all-peer trace gaps are a BRIDGE artifact, not the mesh.**
  The master->host stats path (master WiFi-STA + UDP *broadcast*, ~1/s, unretried) drops in
  bursts when the master is busy time-sharing its single radio between ESP-NOW (4 peers @
  10 Hz) and the AP link. Verified: across every gap the master's ESP-NOW `rx` kept climbing
  at full rate (~10/s) with **~0 added loss** (e.g. a 7 s bridge silence = `rx +80, gaps +0`)
  -- it was receiving every packet, including the adjacent cup board. **All PDR/scaling
  numbers come from the master's cumulative seq-counting (`rx`/`gaps`), independent of bridge
  delivery, so they're unaffected.** In deployment there is no WiFi bridge -- ESP-NOW is the
  data plane. Design note: a production WiFi-*gateway* node would have a bursty real-time
  WiFi side under ESP-NOW load (fine for TCP/OTA; don't rely on it for real-time telemetry).

- **Li-ion != LFP (asterisk on everything battery):** all stability/current/brownout
  results are on `Generic_3V7`. LFP's ~3.2-3.3 V plateau parks on the TPS631013
  buck-boost crossover (a harder regime for current spikes) -- stable-on-Li-ion is
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

- Run T0-T7 on 5 boards (matched channel) -> fill Results -> set the production heartbeat
  rate below the measured knee.
- LFP re-verify pass once Steve's cell holders/connectors exist.
- 20+ node confirmation run if the knee is near the production point.
- Mock-hat antenna RF with panel/battery installed (Steve; ties to COTS Phase 7).
- Promote the go/no-go into **ADR 0021 -- ESP-NOW networking feasibility / 100-fixture
  commit** once data is in.
