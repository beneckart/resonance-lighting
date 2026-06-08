#!/usr/bin/env python3
"""Drive a net_bench broadcast-rate sweep and measure per-rate PDR (the scale knee).

Steps the master's broadcast rate through {1,2,5,10,20,50} Hz over serial ('+'/'-',
which the master broadcasts as SET_RATE to all peers), and for each rate measures the
*incremental* uplink packet-delivery-ratio per peer from the master's UDP bridge
(:54321) -- delta rx / (delta rx + delta gaps) over a dwell window, so prior rates
don't contaminate the number. Finds the rate at which worst-peer PDR drops below the
threshold, and extrapolates the safe node count at a production heartbeat rate.

The dominant scaling risk is shared-channel airtime: aggregate offered load ~
(num_peers + 1 master) * rate. 5 nodes can't reproduce 100-node hidden-node effects,
so this is a screen, not a guarantee. Battery results are Li-ion -- re-verify on LFP.

  ./net_bench_ratesweep.py --port /dev/ttyACM1 --dwell 30 [--out data/ca/<run>.jsonl]
"""
import argparse, json, os, re, socket, time
from datetime import datetime, timezone

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
RATES = [1, 2, 5, 10, 20, 50]  # must match the firmware rate table

ap = argparse.ArgumentParser()
ap.add_argument("--port", default="/dev/ttyACM1", help="master serial port")
ap.add_argument("--dwell", type=float, default=30, help="measure window per rate (s)")
ap.add_argument("--settle", type=float, default=4, help="wait after a rate change (s)")
ap.add_argument("--udp-port", type=int, default=54321)
ap.add_argument("--pdr-threshold", type=float, default=0.99)
ap.add_argument("--prod-rate-hz", type=float, default=2.0)
ap.add_argument("--target-nodes", type=int, default=100)
ap.add_argument("--rssi-floor", type=float, default=-90.0)
ap.add_argument("--end-rate-idx", type=int, default=1, help="rate index to leave the fleet at (1 = 2Hz)")
ap.add_argument("--site", default="ca")
ap.add_argument("--out", default=None)
a = ap.parse_args()

import serial  # pyserial
ser = serial.Serial(a.port, 115200, timeout=0.2)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("", a.udp_port)); sock.settimeout(0.4)

peer_re = re.compile(r"nb-peer id=(\w+) seq=\d+ rx=(\d+) gaps=(\d+) pdr=[\d.]+ rssi=(-?\d+).*dlpdr=([\d.]+)")
rate_re = re.compile(r"rate -> (\d+) Hz")


def step(direction):
    """Press '+'/'-' on the master; return the rate it reports."""
    ser.reset_input_buffer()
    ser.write(direction.encode())
    t = time.time(); rate = None
    while time.time() - t < 2:
        l = ser.readline().decode(errors="replace")
        m = rate_re.search(l)
        if m: rate = int(m.group(1)); break
    return rate


def measure(secs):
    """Collect bridge samples; return {id: (drx, dgap, pdr, rssi, dlpdr)} over the window."""
    first, last = {}, {}
    end = time.time() + secs
    while time.time() < end:
        try:
            d, _ = sock.recvfrom(1024)
        except socket.timeout:
            continue
        m = peer_re.search(d.decode(errors="replace"))
        if not m:
            continue
        pid, rx, gaps, rssi, dl = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4)), float(m.group(5))
        if pid not in first: first[pid] = (rx, gaps)
        last[pid] = (rx, gaps, rssi, dl)
    out = {}
    for pid in last:
        if pid not in first: continue
        drx = last[pid][0] - first[pid][0]
        dgap = last[pid][1] - first[pid][1]
        pdr = drx / (drx + dgap) if (drx + dgap) > 0 else float("nan")
        out[pid] = (drx, dgap, pdr, last[pid][2], last[pid][3])
    return out


print("=== net_bench rate sweep ===", flush=True)
print("flooring rate to 1 Hz...", flush=True)
for _ in range(6):
    step("-"); time.sleep(0.3)

results = []
for i, rate in enumerate(RATES):
    if i > 0:
        got = step("+")
        # resync if the firmware table and ours disagree
        while got is not None and got < rate:
            got = step("+"); time.sleep(0.2)
    time.sleep(a.settle)
    npeers_guess = None
    res = measure(a.dwell)
    npeers = len(res)
    agg = rate * (npeers + 1)  # peers + master share the channel
    worst = min((v[2] for v in res.values()), default=float("nan"))
    print(f"\n[rate {rate:>2} Hz | ~{agg} pkt/s aggregate | {npeers} peers]", flush=True)
    for pid, (drx, dgap, pdr, rssi, dl) in sorted(res.items()):
        print(f"   {pid}: uplink_pdr={pdr:.4f} (rx+{drx}/gaps+{dgap}) rssi={rssi} dl_pdr={dl:.3f}", flush=True)
    print(f"   -> worst-peer uplink PDR = {worst:.4f}", flush=True)
    results.append(dict(rate_hz=rate, peers=npeers, aggregate_pkts_s=agg, worst_pdr=worst,
                        per_peer={pid: dict(drx=v[0], dgap=v[1], pdr=v[2], rssi=v[3], dl_pdr=v[4])
                                  for pid, v in res.items()}))

# leave the fleet at a calm rate
for _ in range(6): step("-"); time.sleep(0.2)
for _ in range(a.end_rate_idx): step("+"); time.sleep(0.2)

# Aggregate loss per rate is the robust metric -- worst-peer PDR is noisy at low
# rates (one lost packet of ~60 reads as 98%), so don't knee off it.
for r in results:
    trx = sum(p["drx"] for p in r["per_peer"].values())
    tgap = sum(p["dgap"] for p in r["per_peer"].values())
    r["samples"] = trx + tgap
    r["agg_loss"] = (tgap / r["samples"]) if r["samples"] else 0.0
    r["agg_pdr"] = 1 - r["agg_loss"]

print("\n=== aggregate PDR vs offered load ===", flush=True)
print(f"  {'rate':>5} {'pkt/s':>6} {'samples':>8} {'agg PDR':>8} {'worst':>7}", flush=True)
for r in results:
    print(f"  {r['rate_hz']:>3}Hz {r['aggregate_pkts_s']:>6} {r['samples']:>8} "
          f"{r['agg_pdr']:>8.4f} {r['worst_pdr']:>7.4f}", flush=True)

# Linear fit of aggregate loss vs offered load over statistically-solid points.
solid = [(r["aggregate_pkts_s"], r["agg_loss"]) for r in results if r["samples"] > 200]
print("\n=== scale extrapolation ===", flush=True)
if len(solid) >= 2:
    n = len(solid); sx = sum(x for x, _ in solid); sy = sum(y for _, y in solid)
    sxx = sum(x * x for x, _ in solid); sxy = sum(x * y for x, y in solid)
    m = (n * sxy - sx * sy) / (n * sxx - sx * sx); b = (sy - m * sx) / n
    print(f"  loss(load) ~= {m*1e4:.3f}e-4 * pkt/s + {b*100:.3f}%  (fit over {n} solid points)", flush=True)
    for rate in (1, 2, 5):
        load = a.target_nodes * rate; pred = max(0.0, m * load + b)
        ok = "OK" if (1 - pred) >= a.pdr_threshold else "below %.0f%%" % (a.pdr_threshold * 100)
        print(f"  {a.target_nodes} nodes @ {rate} Hz = {load} pkt/s -> ~{100*(1-pred):.1f}% PDR [{ok}]", flush=True)
else:
    print("  need >=2 solid (>200-sample) rate points to fit.", flush=True)
print("  NOTE: 5-node small-N (no hidden-node/capture at scale); screen not guarantee. "
      "Lighting tolerates partial loss. Li-ion -- re-verify on LFP.", flush=True)

out = a.out or os.path.join(DATA_DIR, a.site,
                            datetime.now(timezone.utc).strftime("%Y-%m-%d") + "-ratesweep.jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w") as fh:
    for r in results:
        r["ts_utc"] = datetime.now(timezone.utc).isoformat()
        fh.write(json.dumps(r) + "\n")
print(f"\n-> {out}", flush=True)
ser.close(); sock.close()
