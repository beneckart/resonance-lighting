# Power-bench data

JSON Lines logs from `ops/bench/power_logger.py`, polling PowerFeather power-bench
boards over WiFi. One sample per board per poll. Analyze with
`ops/bench/power_summary.py` (unions everything here).

## Layout

```
data/
  ca/<run-id>.jsonl     # Ben, California
  tn/<run-id>.jsonl     # Steve, Tennessee
```

Site-partitioned, append-only, one file per run -> two people can log in parallel
and commit to the repo with no merge conflicts. These `.jsonl` files are committed
(this is the cross-site merge mechanism).

## run-id convention

`<yyyy-mm-dd>-<site>-<battery>-<led>-<panel>-<HHMM>`
e.g. `2026-06-02-ca-liion-4400-is31_13x9-p1w-1432`

The logger builds this automatically from the `--site/--battery/--led/--panel-w`
flags; override with `--run-id`.

## Row schema

Each line is a JSON object: run metadata + host timestamp + the firmware's
`/telemetry` fields.

| Field | Source | Notes |
|---|---|---|
| `run_id`, `site`, `operator`, `battery`, `panel_w`, `led_option_run`, `notes` | logger metadata | the 3 axes + context |
| `ts_utc` | host | ISO-8601 UTC poll time |
| `board_name`, `board_ip`, `reachable` | host | `reachable:false` rows carry an `error` and no telemetry |
| `board`, `fw`, `fixture_id`, `led_option`, `led_mode` | firmware | `led_option` is the build variant; `led_mode` is 0-5 |
| `battery_v` (V), `battery_ma` (mA) | firmware/SDK | `battery_ma` >0 charging into cell, <0 discharging |
| `soc_pct`, `health_pct`, `cycles`, `time_left_min` | firmware/SDK | may be `null` until the MAX17260 gauge is initialized (see caveat) |
| `supply_v` (V), `supply_ma` (mA), `supply_good` | firmware/SDK | USB or solar (VDC) input |
| `uptime_ms`, `heap_free`, `reset_reason`, `pf_ready`, `battery_type`, `telemetry_errors` | firmware | `telemetry_errors` lists fields that returned non-Ok |

## How to add a run

```sh
ops/bench/power_logger.py --boards pf1=<ip> \
  --site ca --operator ben --battery liion-4400 --panel-w 1 --led is31_13x9 \
  --interval 30 --notes "<conditions>"
```
Let it run, Ctrl-C (or `--duration`), then `git add` the new `.jsonl` and commit.

## Caveats that affect interpretation

- **Baseline = mode `0`, not `q`.** Mode `q` stops WiFi (true idle baseline for a
  USB power meter), which drops the board off the network so the logger can't poll
  it. Use mode `0` (LEDs off, radio on) as the logged baseline.
- **WiFi-on confound.** Continuous WiFi draws more than the production ESP-NOW +
  light-sleep duty cycle. Fine for LED-current deltas (subtract the mode-0
  baseline); for autonomy/solar sizing, note it in `--notes`. A duty-cycled
  field-emulation mode is a later firmware refinement.
- **Charging masks LED deltas.** With charging enabled, the charge current (e.g.
  ~200 mA) dominates and is fairly constant, so small LED-current changes are hard
  to see in `supply_ma`. For clean LED-current measurement, build with
  `-DRES_PF_ENABLE_CHARGING=0` (battery discharge only) or use an inline meter.
- **SOC needs the V2 build flag.** If `soc_pct/health_pct/cycles/time_left_min` are
  null, the firmware was built without `-DPOWERFEATHER_BOARD_V2=1` (SDK fell back to
  the V1 gauge). Build with `firmware/power_bench/build.sh`. With the flag they
  populate; SOC may read rough until the gauge learns over a charge/discharge cycle,
  so for precise sizing also coulomb-count from `battery_ma`.
