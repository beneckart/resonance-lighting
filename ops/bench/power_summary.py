#!/usr/bin/env python3
"""Summarize Resonance power-bench JSONL logs across all sites.

Unions every ops/bench/data/**/*.jsonl, then prints per-(run, LED mode) averages
of the key power metrics plus a derived input/output power. This is the cross-site
analysis entry point: Ben (ca) and Steve (tn) both commit run files, and this
script reads them all.

Stdlib only.

Examples:
  ./power_summary.py                      # summarize everything under ops/bench/data
  ./power_summary.py --run 2026-06-02-ca-liion-4400-is31_13x9-p1w-1432
  ./power_summary.py --glob 'ops/bench/data/ca/*.jsonl'
"""

import argparse
import glob
import json
import os
import statistics

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")


def load_rows(patterns):
    rows = []
    for pat in patterns:
        for path in glob.glob(pat, recursive=True):
            with open(path) as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        rows.append(json.loads(line))
                    except json.JSONDecodeError:
                        pass
    return rows


def mean(vals):
    vals = [v for v in vals if isinstance(v, (int, float))]
    return statistics.mean(vals) if vals else None


def fmt(v, suffix=""):
    return f"{v:.1f}{suffix}" if isinstance(v, (int, float)) else "  -  "


def main():
    ap = argparse.ArgumentParser(description="Summarize power-bench JSONL logs.")
    ap.add_argument("--glob", action="append", default=None,
                    help="glob(s) for jsonl files (default: ops/bench/data/**/*.jsonl)")
    ap.add_argument("--run", default=None, help="only this run_id")
    args = ap.parse_args()

    patterns = args.glob or [os.path.join(DATA_DIR, "**", "*.jsonl")]
    rows = load_rows(patterns)
    if args.run:
        rows = [r for r in rows if r.get("run_id") == args.run]
    rows = [r for r in rows if r.get("reachable", True)]
    if not rows:
        print("no rows found")
        return

    # group by (run_id, led_option, led_mode)
    groups = {}
    for r in rows:
        key = (r.get("run_id", "?"), r.get("led_option", r.get("led_option_run", "?")),
               r.get("led_mode", "?"))
        groups.setdefault(key, []).append(r)

    header = (f"{'run_id':<42} {'led':<12} {'mode':<4} {'n':>4} "
              f"{'battV':>7} {'battmA':>8} {'supV':>7} {'supmA':>8} "
              f"{'Pin_W':>7} {'Pbatt_W':>8} {'soc':>5}")
    print(header)
    print("-" * len(header))
    for key in sorted(groups):
        run_id, led, mode = key
        g = groups[key]
        bv = mean([r.get("battery_v") for r in g])
        bma = mean([r.get("battery_ma") for r in g])
        sv = mean([r.get("supply_v") for r in g])
        sma = mean([r.get("supply_ma") for r in g])
        soc = mean([r.get("soc_pct") for r in g])
        pin = (sv * sma / 1000.0) if (sv and sma) else None
        pbatt = (bv * bma / 1000.0) if (bv and bma is not None) else None
        print(f"{run_id:<42} {str(led):<12} {str(mode):<4} {len(g):>4} "
              f"{fmt(bv):>7} {fmt(bma):>8} {fmt(sv):>7} {fmt(sma):>8} "
              f"{fmt(pin):>7} {fmt(pbatt):>8} {fmt(soc):>5}")

    print()
    print("Pin_W = supply_v * supply_ma (power drawn from USB/solar)")
    print("Pbatt_W = battery_v * battery_ma (>0 charging into cell, <0 discharging)")
    print("For autonomy: integrate battery_ma over a night-representative run "
          "(mAh) and compare to cell capacity / panel harvest.")


if __name__ == "__main__":
    main()
