#!/usr/bin/env python3
"""Capture bounded fixture neighbor reports from the dashboard UDP stream.

The output is already shaped like the locate pairwise JSONL contract. Each row
is one directed, ranked EWMA observation (n=1); offline ingestion/analysis can
aggregate the repeated observations into per-link windows. The output file is
exclusive-created so a field trace cannot be silently overwritten.
"""

import argparse
from collections import Counter
from datetime import datetime, timezone
import json
import os
import re
import socket
import time


RSSI_LINE = re.compile(
    r"nb-rssi report=(\d+) rx=([0-9A-Fa-f]{6}) tx=([0-9A-Fa-f]{6}) "
    r"rssi=(-?\d+) n=(\d+) expected=(\d+) censored=([01]) "
    r"idx=(\d+) count=(\d+) linkrssi=(-?\d+)"
)


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, help="exclusive-created JSONL output")
    ap.add_argument("--duration", type=float, default=135.0,
                    help="capture seconds (start the bridge survey after this logger)")
    ap.add_argument("--port", type=int, default=54321)
    ap.add_argument("--notes", default="")
    args = ap.parse_args()

    if args.duration <= 0:
        ap.error("--duration must be positive")
    out = os.path.abspath(args.out)
    os.makedirs(os.path.dirname(out), exist_ok=True)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", args.port))
    sock.settimeout(1.0)

    started = utc_now()
    reporters = Counter()
    transmitters = Counter()
    pairs = Counter()
    rows = 0
    t0 = time.time()
    print(f"RSSI capture -> {out} ({args.duration:.0f}s)", flush=True)
    with open(out, "x", encoding="utf-8") as fh:
        fh.write(json.dumps({
            "src": "rssi_capture",
            "schema": "resonance-pairwise-rssi-observation-v1",
            "ts_utc": started,
            "duration_s": args.duration,
            "udp_port": args.port,
            "notes": args.notes,
            "observation": "ranked fixture neighbor-table EWMA; n=1 per report",
        }, sort_keys=True) + "\n")
        fh.flush()

        while time.time() - t0 < args.duration:
            try:
                payload, address = sock.recvfrom(4096)
            except socket.timeout:
                continue
            text = payload.decode("utf-8", "replace")
            match = RSSI_LINE.search(text)
            if not match:
                continue
            (report, rx, tx, rssi, n, expected, censored, idx, count,
             link_rssi) = match.groups()
            rx = rx.upper()
            tx = tx.upper()
            row = {
                "ts_utc": utc_now(),
                "elapsed_s": round(time.time() - t0, 3),
                "tx": tx,
                "rx": rx,
                "rssi_dbm": int(rssi),
                "n": int(n),
                "n_expected": int(expected),
                "censored": bool(int(censored)),
                "report_seq": int(report),
                "rank_index": int(idx),
                "report_count": int(count),
                "bridge_link_rssi_dbm": int(link_rssi),
                "bridge_ip": address[0],
            }
            fh.write(json.dumps(row, sort_keys=True) + "\n")
            rows += 1
            reporters[rx] += 1
            transmitters[tx] += 1
            pairs[(tx, rx)] += 1
            if rows % 250 == 0:
                fh.flush()
                print(f"  {rows} rows, {len(reporters)} reporters, "
                      f"{len(pairs)} directed pairs", flush=True)
        fh.flush()

    print(f"done: {rows} rows, {len(reporters)} reporters, "
          f"{len(transmitters)} transmitters, {len(pairs)} directed pairs",
          flush=True)
    if reporters:
        print("reporters:", " ".join(sorted(reporters)), flush=True)


if __name__ == "__main__":
    main()
