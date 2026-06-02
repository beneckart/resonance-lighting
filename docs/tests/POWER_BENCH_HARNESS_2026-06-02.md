# Power-bench harness — three-axis test matrix

**Date:** 2026-06-02
**Status:** Live (Phase A validated; collecting data)
**Owners:** Ben (ca), Steve (tn)

The PowerFeather V2 power-bench (`firmware/power_bench/` + `ops/bench/power_logger.py`)
measures real power behavior to answer the procurement questions for ~100 fixtures:
**which board, battery, LED module, and panel to buy.** This doc is the test
matrix, data schema, and run procedure. See ADR 0020 for the framework decision and
`ops/bench/data/README.md` for the row schema.

## The three axes

| Axis | Values to sweep |
|---|---|
| **Battery** | Li-ion (e.g. 4400 mAh PKCell) `Generic_3V7`; LiFePO4 (e.g. 1500 mAh 18650) `Generic_LFP`; others on hand |
| **LED option** | IS31FL3741 13x9 (`is31_13x9`); M5Stack NeoHEX 37px (`neohex37`); single high-power RGBW (`rgbw_single`); none |
| **Solar panel** | 1 W, 2 W, 3 W, 5 W; office/indoor (baseline), shade, full sun |

Battery type + LED option are firmware build choices; panel and conditions are
operator-supplied run metadata. Each `power_logger.py` run pins one combination.

## Firmware build per LED option

Use `firmware/power_bench/build.sh` (always sets `-DPOWERFEATHER_BOARD_V2=1`, which
is REQUIRED for the V2 MAX17260 fuel gauge — see the known-issues note below):

```sh
# IS31FL3741 13x9 over STEMMA-QT, 4400 mAh Li-ion, build + flash
firmware/power_bench/build.sh --led is31 --cap 4400 --port /dev/ttyACM0

# NeoHEX / RGBW: --led neohex | --led rgbw1
# LiFePO4: --chem lfp --cap 1500
# Clean LED-current runs (no charge masking): --no-charge
# Panel MPP for solar runs: --maintain <volts>
```

## LED measurement modes (set via /mode?m= or serial)

Same as smoke_test: `0` off · `1` center max white · `2` 3-pixel RGB fringe ·
`3` center 3x3 dim warm · `4` full-array very low · `5` full-array capped (brief) ·
`q` quiet (LEDs off + WiFi off — USB-meter baseline only, NOT for WiFi logging).

## Run procedure

1. Flash the firmware variant for the LED option + battery (above).
2. Confirm the board joins WiFi and serves telemetry: `curl http://<ip>/telemetry`.
3. Start logging the combination:
   ```sh
   ops/bench/power_logger.py --boards pf1=<ip> --site ca --operator ben \
     --battery liion-4400 --panel-w 1 --led is31_13x9 --interval 30 \
     --notes "office low light"
   ```
4. Step LED modes during the run (`curl http://<ip>/mode?m=1` ...) to capture per-mode
   current; or leave at mode `0` for a baseline / long autonomy/solar run.
5. Ctrl-C (or `--duration`), `git add` the new `ops/bench/data/<site>/*.jsonl`, commit.
6. Analyze across sites: `ops/bench/power_summary.py`.

## What we read (per `/telemetry`)

`battery_v`, `battery_ma` (+charge/−discharge), `supply_v`, `supply_ma`,
`supply_good`; `soc_pct/health_pct/cycles/time_left_min` when the gauge is up;
plus board/fw/led identifiers. Derived: input power `supply_v*supply_ma`, battery
power `battery_v*battery_ma`. For autonomy, integrate `battery_ma` over a
night-representative run and compare to cell capacity and panel harvest.

## Phase A validation results (2026-06-02, board 9E5AB8)

- V2 confirmed (MAX17260 0x36 + BQ25628E 0x6A on Wire1/STEMMA-QT). `Board.init(4400,
  Generic_3V7)` -> Ok. Full telemetry over WiFi with `-DPOWERFEATHER_BOARD_V2=1`:
  battery 3.60 V / +220 mA charging, soc 7%, health 100%, cycles 0, time_left ~140 min,
  supply 4.67 V / ~238 mA, `telemetry_errors []`. Logger + summary pipeline works
  end-to-end (`ops/bench/data/ca/2026-06-02-...jsonl`).

## Known issues / interpretation caveats

- **V2 gauge flag is mandatory**: build with `-DPOWERFEATHER_BOARD_V2=1` (build.sh
  does this), else the SDK uses the V1 LC709204F gauge and SOC/health/cycles fail.
  With the flag they populate (SOC may read rough until the gauge learns).
- **Baseline = mode 0, not q** (q drops WiFi).
- **Charging masks LED deltas**: build `-DRES_PF_ENABLE_CHARGING=0` or use an inline
  meter for clean LED-current numbers.
- **WiFi-on confound**: continuous WiFi > production duty cycle; note in run notes
  for autonomy/solar runs.

## Next

- LED optics + iso-current comparison across the three modules (ties into
  `ISO_CURRENT_LED_BRIGHTNESS_TEST_2026-05-18.md`).
- Solar harvest sweep across panels and conditions; `setSupplyMaintainVoltage` at
  panel MPP (`-DRES_PF_MAINTAIN_V=`).
- Steve mirrors the bench in TN; merge JSONL via the repo.
