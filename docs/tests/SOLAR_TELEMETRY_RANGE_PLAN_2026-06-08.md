# Plan: outdoor solar telemetry over ESP-NOW + a WiFi range diagnostic

**Date:** 2026-06-08
**Status:** (a) **BUILT + VALIDATED on hardware 2026-06-08** — supply/panel telemetry
(`supply_mv/supply_ma/supply_good`) rides the heartbeat over ESP-NOW; logs panel V/I with
no WiFi-STA; `net_bench_log.py` derives `supply_w/battery_w/load_w`. See LOG cont. 8.
(b) **VALIDATED on hardware 2026-06-08** (2 boards, OTA) via
the ESP-NOW serial-bridge route below (Ben's call: avoid a laptop tether), not the
standalone tethered sketch. Coverage map streams to the desk; 3 Eero nodes resolved by
RSSI; scan↔ESP-NOW coexistence holds; a shared-seq PDR bug was found+fixed. Remaining for
(b): the actual yard walk (coverage-at-distance) + the AP-placement note. See "Update
2026-06-08" + LOG 2026-06-08 (cont. 7).
**Owner:** Ben

## For the agent picking this up — read these first

This continues the PowerFeather V2 COTS de-risking. Orient with:
- `docs/decisions/0021-powerfeather-v2-feasibility-validated.md` — the go decision + what's validated.
- `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` — the ESP-NOW bench work, tools, results.
- `LOG.md` (2026-06-07/08 entries) — narrative incl. the solar + WiFi-range findings.
- `firmware/POWERFEATHER_NOTES.md` — board gotchas (3V3 rail/GPIO4, `maintain` ≤ supply, chemistry flash order, native-USB-CDC reset flakiness, OTA `verifyOta` C-linkage).
- Code you'll reuse: `firmware/net_bench/net_bench.ino` (ESP-NOW master/peer + the master→host UDP bridge + `NbHeartbeat` packet), `firmware/power_bench/power_bench.ino` (full `/telemetry` incl. `supply_v`/`supply_ma` via `Board.getSupplyVoltage/Current`), `ops/bench/net_bench_log.py` (bridge → JSONL), `ops/bench/power_logger.py`.

## Why (the problem, already diagnosed — don't re-derive)

Outdoor solar telemetry needs the board reachable while it's in the sun. But the **ESP32-S3 is 2.4 GHz only**, and from the yard it can't hold a WiFi-STA association to the house Eero (Pro 6E mesh) — the link drops and it doesn't roam between Eero nodes; meanwhile 5/6 GHz devices (Shield/TVs) are happy on a different band. **ESP-NOW, by contrast, reached the back fence** (connectionless, no association/roaming) in the range walk. WiFi was run at **full TX power** (not low-power mode), so that's not the limiter. UDP-vs-TCP is a red herring — the failure is at the *link/association* layer, not the transport. Conclusion: **use ESP-NOW (the deployment data plane) to carry outdoor telemetry; WiFi-STA from the yard is the wrong tool.**

These two work items are independent: **(a) is the fix** (log solar from anywhere ESP-NOW reaches); **(b) confirms the WiFi "why"** and informs the field maintenance-AP plan. Do (a) first if the goal is solar data; (b) is a ~1-hour confirmation. ESP-NOW carrying small telemetry is ADR-0010-compliant (it's state/metadata, not firmware).

---

## (a) Solar telemetry over ESP-NOW (the fix)

**Goal:** the solar board runs pure ESP-NOW on battery+panel (no WiFi) and broadcasts its
power telemetry; a bridge node in the house (on WiFi, near an Eero node) relays it to the
host; log from anywhere ESP-NOW reaches. This reuses ~90% of `net_bench` (which already does
master/peer ESP-NOW + a master→UDP:54321 bridge, and whose `NbHeartbeat` already carries
battery `batt_mv/batt_ma/soc`).

**What's missing:** the heartbeat/bridge don't carry the **supply (panel)** side, and the
peer doesn't read it. Add supply + charge fields end-to-end.

**Implementation steps:**
1. **Cache supply in the peer.** In `net_bench.ino` `readBattery()` (refreshed ~1 Hz), also
   read `Board.getSupplyVoltage()` / `Board.getSupplyCurrent()` into new globals (`csV`,`csMa`).
   (Pattern: see `power_bench.ino` `telemetryJson()` supply reads.)
2. **Extend the packet.** Add `int16_t supply_mv; int16_t supply_ma;` to `struct NbHeartbeat`
   (bump `NB_PROTO_VER`). Fill them in `sendHeartbeat()`. Keep it small/packed.
3. **Master stores + bridges them.** Add the supply fields to `NbPeerStat` + the snapshot in
   `processRx()` (NB_HEARTBEAT case), and emit `sv=` / `sma=` in the `nb-peer` line in
   `bridgeStats()`.
4. **Host logger.** Extend `ops/bench/net_bench_log.py`'s `nb-peer` regex + JSONL row with
   `supply_v` / `supply_ma` (so `battery_ma` net charge AND panel input are both logged).
   Optionally a small `net_bench_solar_summary.py` (mWh harvested vs time), or reuse
   `net_bench_log.py` + a plot.
5. **Flash + deploy.** Solar board = **peer** with the real solar/LFP config:
   `./build.sh --role peer --channel <AP channel> --chem lfp --cap <real mAh> --maintain <panel MPP> --hb-hz 1`
   (charging on by default; `--maintain` = panel MPP, e.g. 5.5 V for the Seeed 3W; remember
   `maintain` must be ≤ any USB supply if USB is attached — see notes). Bridge node = a second
   board as **master** on the same `--channel` (must equal the house AP's 2.4 GHz channel),
   placed where it can hear the yard over ESP-NOW. Host runs `net_bench_log.py`.
6. **Heartbeat rate:** solar changes slowly → `--hb-hz` 0.2–1 is plenty and low-power.

**Test / acceptance:**
- Solar board in the yard/sun on battery+panel (no WiFi); bridge in the house; host logging.
- `net_bench_log.py` shows the peer's `supply_v`/`supply_ma`/`battery_ma`/`soc` updating from
  the yard — i.e. **solar telemetry captured with zero WiFi-STA on the solar board.** PASS =
  continuous capture through the conditions WiFi-STA couldn't reach.
- Then the actual solar runs (full-sun harvest, `--maintain` sweep) per the NETWORKING/solar TODOs.

**Gotchas:** channel must match the AP the master joins (mismatch silently kills ESP-NOW —
master warns); set the fuel-gauge `--cap` to the real cell + let it take a charge cycle; LFP
flash-order safety (flash `--chem lfp` before connecting the cell).

---

## (b) WiFi range diagnostic (confirm the 2.4 GHz / roaming story)

**Goal:** quantify why the ESP32 falls off WiFi in the yard while other devices are fine —
confirm it's 2.4 GHz coverage + poor roaming (vs a config/AP issue), and inform where a field
**maintenance AP** must sit.

**Implementation:** a tiny standalone sketch `firmware/wifi_diag/wifi_diag.ino` (or a `d`
serial command added to `power_bench`). On an interval, connect/stay-associated and print over
serial:
- `WiFi.RSSI()` (dBm to the associated AP), `WiFi.BSSIDstr()` (which Eero node), `WiFi.channel()`.
- A `WiFi.scanNetworks()` of all visible **2.4 GHz** SSIDs/BSSIDs + RSSI + channel (shows the
  2.4 GHz landscape and whether a *closer* Eero node was available but not chosen).
- Also force/print `WiFi.setTxPower()` at max for an apples-to-apples baseline.

**Test / acceptance** (board on USB + laptop, or within serial reach; mind the native-USB-CDC
reset quirk in POWERFEATHER_NOTES):
- Walk from office → yard logging RSSI/BSSID/channel. Expected to confirm: RSSI degrades into
  the ~−85…−90 dBm association-collapse zone in the yard, and the board **clings to the
  original BSSID** instead of roaming to a nearer Eero node (a scan showing a closer node with
  better RSSI that it didn't pick = smoking gun). Compare to a 5/6 GHz device's experience.
- Deliverable: a short note in `LOG.md` (or this doc's Results) — the 2.4 GHz RSSI map + the
  roaming behavior, and the implication for the field maintenance AP (place a 2.4 GHz AP near
  the tree for OTA windows; don't rely on the house mesh reaching the canopy on 2.4 GHz).

**Levers if more WiFi range is ever needed** (mostly AP-side; ESP32 antenna is fixed PCB):
a nearby 2.4 GHz AP (travel router / hotspot), forced max TX power (minor), a clean/low 2.4 GHz
channel. ESP-NOW Long-Range mode is **not** usable for WiFi-STA-to-Eero (only ESP-NOW links).

---

## Sequencing

- **(a)** is the real fix and the field-representative architecture — do it for any outdoor
  solar logging. ~half-day firmware + a logger tweak; reuses net_bench.
- **(b)** is a ~1-hour confirmation; nice for understanding + maintenance-AP placement, not a
  prerequisite for (a). Optional but cheap.

---

## Update 2026-06-08 — (b) reworked as a wireless ESP-NOW bridge (no laptop tether)

Ben didn't want to walk a tethered laptop, so (b) was implemented as the **same
wireless architecture (a) needs**, killing two birds: a **desk-tethered ESP-NOW
"serial bridge"** prints the field fleet to USB on the PC, and a **field peer
scan-reports** the 2.4 GHz landscape over ESP-NOW. This is *scan-only* (the field board
never associates) — which sidesteps the channel problem (ESP-NOW rides the WiFi-STA
channel; an associated board is locked to the Eero's channel, an unassociated one we
pin freely) and still delivers the plan's own stated smoking gun ("a scan showing a
closer node with better RSSI"). The empirical *roaming-decision* test (does it cling to
the far BSSID?) is deferred to the tethered `firmware/wifi_diag/` sketch.

**Built (compiles clean, all 4 net_bench variants; NOT yet hardware-tested):**
- `firmware/net_bench/`: `--serial-bridge` (master relays `nb-*` to USB serial, no
  WiFi) and `--scan-report` (peer async-scans 2.4 GHz, broadcasts `NB_SCANAP` packets:
  per-AP BSSID/RSSI/channel/SSID, strongest-first, radio re-pinned to the ESP-NOW
  channel after each scan). New packet `NB_SCANAP=7` (proto ver unchanged — additive).
- `ops/bench/net_bench_serial_bridge.py`: serial→UDP:54321 relay so the existing
  `net_bench_log.py` / `net_bench_monitor.py` work from the desk bridge unchanged.
  `net_bench_log.py` extended with an `nb-scanap` row (logs `ssid/bssid/ap_rssi/...`).
- `firmware/wifi_diag/`: the standalone tethered association/roaming probe (kept as the
  complementary "does it actually roam" tool; RSSI/BSSID/channel + scan + missed-roam flag).

**To run it (hardware step, Ben):**
```
# desk bridge on the PC's USB:
cd firmware/net_bench && ./build.sh --role master --channel <clean 2.4 ch> --serial-bridge --port /dev/ttyACMx
# field board (battery, walked):
./build.sh --role peer --channel <same> --scan-report --scan-s 15 --hb-hz 1 --port /dev/ttyACMy
# on the PC:
ops/bench/net_bench_serial_bridge.py --port /dev/ttyACMx &
ops/bench/net_bench_log.py --site ca --notes "yard 2.4GHz coverage"
```
PASS = `nb-scanap` rows update from the yard (per-Eero-node RSSI) with zero WiFi-STA on
the field board, through positions WiFi-STA couldn't hold. Deliverable unchanged: the
2.4 GHz RSSI map + maintenance-AP placement note in `LOG.md`.

**Caveats / unknowns (be cautious — n=0 on hardware):** async `WiFi.scanNetworks()`
coexisting with ESP-NOW on the S3 is assumed-fine but unverified; the post-scan channel
re-pin is the load-bearing line. Confirm a scan doesn't wedge ESP-NOW and that the
bridge actually receives `NB_SCANAP` across the yard before trusting any map.
