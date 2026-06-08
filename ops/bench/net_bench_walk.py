#!/usr/bin/env python3
"""Continuously log per-peer RSSI/PDR from the net_bench master bridge (for a range walk).

Writes one JSONL row per bridge sample for every peer (the moving board + the stationary
references), timestamped from a t0 at start, so the time-series can be graphed into the
"V" of a walk-out-and-back. Run in the background; stop with Ctrl-C / kill. Dropouts show
as gaps in a peer's series (the master stops hearing it). Graph with net_bench_walk_plot.py.

  ./net_bench_walk.py --out ops/bench/data/ca/<date>-rangewalk.jsonl --walker 9F2690
"""
import argparse, json, re, socket, time
from datetime import datetime, timezone

ap = argparse.ArgumentParser()
ap.add_argument("--out", required=True)
ap.add_argument("--walker", default="9F2690", help="the moving board (for the live readout)")
ap.add_argument("--port", type=int, default=54321)
a = ap.parse_args()

peer_re = re.compile(
    r"nb-peer id=(\w+) seq=\d+ rx=(\d+) gaps=(\d+) pdr=[\d.]+ rssi=(-?\d+) "
    r"bv=([\d.-]+) ima=-?\d+ soc=(-?\d+) rr=\w+ ca=\d+ mode=\d+ dlpdr=([\d.]+)")

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", a.port)); s.settimeout(0.5)

prev = {}      # id -> (rx, gaps) for incremental PDR
t0 = time.time()
n = 0
lastprint = 0
print(f"range-walk logging -> {a.out}  (walker={a.walker}). Ctrl-C to stop.", flush=True)
with open(a.out, "w") as fh:
    try:
        while True:
            try:
                d, _ = s.recvfrom(1024)
            except socket.timeout:
                continue
            m = peer_re.search(d.decode(errors="replace"))
            if not m:
                continue
            pid = m.group(1); rx = int(m.group(2)); gaps = int(m.group(3))
            rssi = int(m.group(4)); bv = float(m.group(5)); soc = int(m.group(6)); dl = float(m.group(7))
            ipdr = None
            if pid in prev:
                drx = rx - prev[pid][0]; dg = gaps - prev[pid][1]
                if drx + dg > 0:
                    ipdr = round(drx / (drx + dg), 4)
            prev[pid] = (rx, gaps)
            t = round(time.time() - t0, 2)
            fh.write(json.dumps({"ts_utc": datetime.now(timezone.utc).isoformat(), "t": t,
                                 "id": pid, "rssi": rssi, "rx": rx, "gaps": gaps,
                                 "inc_pdr": ipdr, "dl_pdr": dl, "bv": bv, "soc": soc}) + "\n")
            fh.flush(); n += 1
            if pid == a.walker and time.time() - lastprint >= 2:
                lastprint = time.time()
                print(f"  t={t:6.0f}s  {pid}  RSSI={rssi:>4} dBm  inc_pdr={ipdr}  (rows={n})", flush=True)
    except KeyboardInterrupt:
        pass
print(f"\nstopped: {n} rows -> {a.out}", flush=True)
s.close()
