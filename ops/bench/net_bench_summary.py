#!/usr/bin/env python3
"""Summarize net_bench JSONL: per-peer PDR/RSSI/reboots + scale extrapolation.

Unions ops/bench/data/**/*.jsonl, keeps net-bench rows (src=peer/master), and
reports per-peer and aggregate stats grouped by (run_id, topology, tx_rate_hz):
  - uplink PDR (peer->master) and downlink PDR (master multicast at the peer)
  - RSSI distribution + margin to a ~-90 dBm floor
  - reboot count (uptime drops), worst-peer PDR, send-fail rate (from master rows)

Scale block: across the tx-rate sweep it fits PDR vs aggregate offered rate
(per-node rate x node count) and reports the loss knee + implied safe node count
at a production heartbeat rate. Stdlib only.

Examples:
  ./net_bench_summary.py
  ./net_bench_summary.py --run 2026-06-07-ca-liion-4400-net-master-multicast-1432
  ./net_bench_summary.py --extrapolate-to 100 --prod-rate-hz 2
"""
import argparse, glob, json, os, statistics

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

ap = argparse.ArgumentParser()
ap.add_argument("--glob", action="append", default=None)
ap.add_argument("--run", default=None, help="filter to one run_id")
ap.add_argument("--extrapolate-to", type=int, default=100)
ap.add_argument("--prod-rate-hz", type=float, default=2.0)
ap.add_argument("--pdr-threshold", type=float, default=0.99)
ap.add_argument("--rssi-floor", type=float, default=-90.0)
a = ap.parse_args()

patterns = a.glob or [os.path.join(DATA_DIR, "**", "*.jsonl")]
rows = []
for pat in patterns:
    for path in glob.glob(pat, recursive=True):
        with open(path) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    r = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if r.get("src") in ("peer", "master"):
                    rows.append(r)
if a.run:
    rows = [r for r in rows if r.get("run_id") == a.run]

peer_rows = [r for r in rows if r.get("src") == "peer"]
if not peer_rows:
    print("no net-bench peer rows found.")
    raise SystemExit(0)


def grp_key(r):
    return (r.get("run_id"), r.get("topology"), r.get("tx_rate_hz"))


# group -> peer_id -> list of rows
groups = {}
for r in peer_rows:
    groups.setdefault(grp_key(r), {}).setdefault(r["peer_id"], []).append(r)

print("=== net_bench per-peer summary ===")
knee_points = []  # (aggregate_offered_rate, min_pdr) per (topology,rate) group
for key, by_peer in sorted(groups.items(), key=lambda kv: str(kv[0])):
    run_id, topo, rate = key
    print(f"\nrun={run_id}  topology={topo}  tx_rate_hz={rate}  peers={len(by_peer)}")
    worst_pdr = 1.0
    for pid, prs in sorted(by_peer.items()):
        # last cumulative PDR is the most representative (rx/(rx+gaps) over the run)
        last = prs[-1]
        rx = last.get("rx", 0)
        gaps = last.get("gaps", 0)
        pdr = rx / (rx + gaps) if (rx + gaps) else float("nan")
        dlpdr = statistics.mean([p["dl_pdr"] for p in prs if "dl_pdr" in p]) if prs else None
        rssis = [p["rssi_dbm"] for p in prs if "rssi_dbm" in p]
        rssi_med = statistics.median(rssis) if rssis else None
        rssi_p10 = sorted(rssis)[max(0, len(rssis) // 10)] if rssis else None
        margin = (rssi_p10 - a.rssi_floor) if rssi_p10 is not None else None
        ups = [p["uptime_ms"] for p in prs if "uptime_ms" in p]
        reboots = sum(1 for i in range(1, len(ups)) if ups[i] < ups[i - 1] - 2000)
        rrs = set(p.get("reset_reason") for p in prs)
        worst_pdr = min(worst_pdr, pdr) if pdr == pdr else worst_pdr
        print(f"  peer {pid}: uplink_pdr={pdr:.4f} dl_pdr={dlpdr if dlpdr is None else round(dlpdr,4)} "
              f"rssi_med={rssi_med} p10={rssi_p10} margin={margin}dB reboots={reboots} rr={sorted(rrs)}")
    if rate:
        knee_points.append((rate * len(by_peer), worst_pdr))
    print(f"  -> worst-peer uplink PDR = {worst_pdr:.4f}")

# master rows: send-fail rate
master_rows = [r for r in rows if r.get("src") == "master"]
if master_rows:
    last = master_rows[-1]
    sok, sfail = last.get("send_ok", 0), last.get("send_fail", 0)
    tot = sok + sfail
    print(f"\nmaster send: ok={sok} fail={sfail} fail_rate={sfail/tot if tot else 0:.4%} ch={last.get('channel')}")

# scale extrapolation
print("\n=== scale extrapolation ===")
if len(knee_points) >= 2:
    knee_points.sort()
    below = [agg for agg, pdr in knee_points if pdr < a.pdr_threshold]
    knee = min(below) if below else None
    print(f"  measured points (aggregate_offered_pkts_s, worst_pdr): {[(a_,round(p,4)) for a_,p in knee_points]}")
    if knee:
        print(f"  loss knee: worst-peer PDR drops below {a.pdr_threshold} at ~{knee} pkt/s aggregate offered rate.")
        safe_nodes = int(knee / a.prod_rate_hz)
        print(f"  at prod heartbeat {a.prod_rate_hz} Hz -> safe up to ~{safe_nodes} nodes "
              f"({'OK' if safe_nodes >= a.extrapolate_to else 'TIGHT/FAIL'} for {a.extrapolate_to}).")
    else:
        hi = max(knee_points)[0]
        print(f"  PDR stayed >= {a.pdr_threshold} through the swept range (max {hi} pkt/s aggregate).")
        print(f"  at prod {a.prod_rate_hz} Hz, {a.extrapolate_to} nodes => {a.extrapolate_to*a.prod_rate_hz} pkt/s "
              f"(within tested range: {'YES' if a.extrapolate_to*a.prod_rate_hz <= hi else 'NO -- extrapolating beyond data'}).")
else:
    print("  need >=2 tx-rate points for a knee fit. Run a rate sweep (e.g. 1,2,5,10,20,50 Hz).")
print("\nNOTE: 5-node small-N; collision/hidden-node at 100 not linearly inferable. "
      "All battery/stability results are Li-ion -- *re-verify on LFP*.")
