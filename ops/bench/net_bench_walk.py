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
    r"nb-peer id=(?P<id>\w+) seq=(?P<seq>\d+) rx=(?P<rx>\d+) gaps=(?P<gaps>\d+) pdr=[\d.]+ "
    r"rssi=(?P<rssi>-?\d+) bv=(?P<bv>[\d.-]+) ima=-?\d+ soc=(?P<soc>-?\d+) rr=(?P<rr>\w+) "
    r"ca=\d+ mode=\d+ dlpdr=(?P<dl>[\d.]+) dlrssi=-?\d+ up=(?P<up>\d+)")

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", a.port)); s.settimeout(0.5)

prev = {}      # id -> (rx, gaps) for incremental PDR
lastup = {}    # id -> last uptime_ms, for reboot detection
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
            pid = m["id"]; rx = int(m["rx"]); gaps = int(m["gaps"]); rssi = int(m["rssi"])
            bv = float(m["bv"]); soc = int(m["soc"]); dl = float(m["dl"])
            seq = int(m["seq"]); rr = m["rr"]; up = int(m["up"])
            ipdr = None
            if pid in prev:
                drx = rx - prev[pid][0]; dg = gaps - prev[pid][1]
                if drx + dg > 0:
                    ipdr = round(drx / (drx + dg), 4)
            prev[pid] = (rx, gaps)
            t = round(time.time() - t0, 2)
            reboot = pid in lastup and up < lastup[pid] - 2000  # uptime dropped -> rebooted
            if reboot:
                print(f"  *** t={t:.0f}s {pid} REBOOT (up {lastup[pid]}->{up} ms, rr={rr}) -- "
                      f"would disrupt the light-show state! ***", flush=True)
            lastup[pid] = up
            fh.write(json.dumps({"ts_utc": datetime.now(timezone.utc).isoformat(), "t": t,
                                 "id": pid, "rssi": rssi, "rx": rx, "gaps": gaps, "seq": seq,
                                 "inc_pdr": ipdr, "dl_pdr": dl, "bv": bv, "soc": soc,
                                 "reset_reason": rr, "uptime_ms": up, "reboot": reboot}) + "\n")
            fh.flush(); n += 1
            if pid == a.walker and time.time() - lastprint >= 2:
                lastprint = time.time()
                print(f"  t={t:6.0f}s  {pid}  RSSI={rssi:>4} dBm  inc_pdr={ipdr}  rr={rr}  (rows={n})", flush=True)
    except KeyboardInterrupt:
        pass
print(f"\nstopped: {n} rows -> {a.out}", flush=True)
s.close()
