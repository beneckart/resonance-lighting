#!/usr/bin/env python3
"""Resonance power-bench host logger.

Polls one or more PowerFeather power-bench boards over WiFi (the firmware's
GET /telemetry JSON endpoint) on an interval, stamps each sample with run
metadata, and appends JSON Lines to ops/bench/data/<site>/<run-id>.jsonl.

Per-run, site-partitioned files mean Ben (ca) and Steve (tn) can log in parallel
and commit to the repo without merge conflicts; power_summary.py unions them.

Stdlib only (urllib/json/argparse) so it runs anywhere Python 3.8+ is installed.

Examples:
  # one board, default 10 s interval, until Ctrl-C
  ./power_logger.py --boards 192.168.4.185 \\
      --site ca --operator ben --battery liion-4400 --panel-w 1 --led is31_13x9

  # two boards with friendly names, 30 s interval, 2-hour run
  ./power_logger.py --boards pf1=192.168.4.185,pf2=192.168.4.186 \\
      --site ca --operator ben --battery lifepo4-1500 --panel-w 2 \\
      --led neohex37 --interval 30 --duration 7200 --notes "afternoon sun"

Notes:
  - Use LED mode 0 (LEDs off, radio on) as the baseline, NOT mode q -- q stops
    WiFi and the board drops off the network.
  - Continuous WiFi inflates active current vs the production ESP-NOW + light
    sleep duty cycle; record that in --notes for autonomy/solar runs.
"""

import argparse
import json
import os
import signal
import sys
import time
import urllib.request
from datetime import datetime, timezone

REPO_DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")


def parse_boards(spec):
    """'pf1=192.168.4.185,192.168.4.186' -> [(name, ip), ...]."""
    out = []
    for i, item in enumerate(spec.split(",")):
        item = item.strip()
        if not item:
            continue
        if "=" in item:
            name, ip = item.split("=", 1)
        else:
            name, ip = f"board{i+1}", item
        out.append((name.strip(), ip.strip()))
    return out


def fetch_telemetry(ip, timeout):
    url = f"http://{ip}/telemetry"
    with urllib.request.urlopen(url, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def make_run_id(args, now):
    if args.run_id:
        return args.run_id
    date = now.strftime("%Y-%m-%d")
    panel = f"p{args.panel_w}w" if args.panel_w is not None else "pNA"
    parts = [date, args.site, args.battery, args.led, panel]
    base = "-".join(str(p) for p in parts if p)
    # append a short start-time suffix to keep repeated same-day runs distinct
    return f"{base}-{now.strftime('%H%M')}"


def main():
    ap = argparse.ArgumentParser(description="Poll power-bench /telemetry and log JSONL.")
    ap.add_argument("--boards", required=True,
                    help="comma list of ip or name=ip (e.g. pf1=192.168.4.185,192.168.4.186)")
    ap.add_argument("--site", required=True, help="ca | tn | <other>")
    ap.add_argument("--operator", default="", help="who is running this (ben/steve)")
    ap.add_argument("--battery", default="batt",
                    help="cell label, e.g. liion-4400 or lifepo4-1500")
    ap.add_argument("--panel-w", type=float, default=None, help="panel watts (0 if none)")
    ap.add_argument("--led", default="", help="LED option label, e.g. is31_13x9/neohex37/rgbw_single/none")
    ap.add_argument("--notes", default="", help="free-text run notes (conditions, sun, etc.)")
    ap.add_argument("--interval", type=float, default=10.0, help="seconds between polls")
    ap.add_argument("--duration", type=float, default=None, help="stop after N seconds (default: until Ctrl-C)")
    ap.add_argument("--timeout", type=float, default=8.0, help="per-request HTTP timeout (s)")
    ap.add_argument("--run-id", default=None, help="override the auto run-id")
    ap.add_argument("--out-dir", default=None, help="override output dir (default ops/bench/data/<site>)")
    args = ap.parse_args()

    boards = parse_boards(args.boards)
    if not boards:
        ap.error("no boards parsed from --boards")

    start = datetime.now(timezone.utc)
    run_id = make_run_id(args, start)
    out_dir = args.out_dir or os.path.join(REPO_DATA_DIR, args.site)
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"{run_id}.jsonl")

    meta = {
        "run_id": run_id,
        "site": args.site,
        "operator": args.operator,
        "battery": args.battery,
        "panel_w": args.panel_w,
        "led_option_run": args.led,
        "notes": args.notes,
    }

    print(f"run_id   : {run_id}")
    print(f"out      : {out_path}")
    print(f"boards   : {', '.join(f'{n}@{ip}' for n, ip in boards)}")
    print(f"interval : {args.interval}s   duration: {args.duration or 'until Ctrl-C'}")
    print("-" * 72)

    stop = {"flag": False}
    signal.signal(signal.SIGINT, lambda *_: stop.update(flag=True))

    deadline = (time.time() + args.duration) if args.duration else None
    n = 0
    with open(out_path, "a", buffering=1) as f:
        while not stop["flag"]:
            ts = datetime.now(timezone.utc).isoformat()
            for name, ip in boards:
                row = dict(meta)
                row["ts_utc"] = ts
                row["board_name"] = name
                row["board_ip"] = ip
                try:
                    tel = fetch_telemetry(ip, args.timeout)
                    row["reachable"] = True
                    row.update(tel)  # firmware telemetry fields
                    bv = tel.get("battery_v")
                    bma = tel.get("battery_ma")
                    sma = tel.get("supply_ma")
                    print(f"{ts}  {name:<8} mode={tel.get('led_mode')} "
                          f"bV={bv} bmA={bma} sV={tel.get('supply_v')} smA={sma} "
                          f"soc={tel.get('soc_pct')}")
                except Exception as e:  # noqa: BLE001 - log and continue
                    row["reachable"] = False
                    row["error"] = str(e)
                    print(f"{ts}  {name:<8} UNREACHABLE ({e})")
                f.write(json.dumps(row) + "\n")
            n += 1
            if deadline and time.time() >= deadline:
                break
            # interruptible sleep
            slept = 0.0
            while slept < args.interval and not stop["flag"]:
                time.sleep(min(0.25, args.interval - slept))
                slept += 0.25

    print("-" * 72)
    print(f"wrote {n} sample-rounds to {out_path}")


if __name__ == "__main__":
    main()
