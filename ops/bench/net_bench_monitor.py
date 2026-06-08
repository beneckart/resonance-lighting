#!/usr/bin/env python3
"""Live per-peer RSSI / PDR monitor for net_bench (terminal, refreshes in place).

Listens to the master's UDP bridge (:54321) and redraws a per-peer table ~1/s:
RSSI (with a signal bar), uplink + downlink PDR, SOC/voltage, and last-heard age.
Handy for a range walk -- watch a peer's RSSI climb/fall as you move it. Stdlib only.

  ./net_bench_monitor.py            # run until Ctrl-C
  ./net_bench_monitor.py --port 54321
"""
import argparse, re, socket, time

ap = argparse.ArgumentParser()
ap.add_argument("--port", type=int, default=54321)
ap.add_argument("--stale", type=float, default=8.0, help="grey out peers not heard in N s")
a = ap.parse_args()

peer_re = re.compile(
    r"nb-peer id=(\w+) seq=\d+ rx=(\d+) gaps=(\d+) pdr=([\d.]+) rssi=(-?\d+) "
    r"bv=([\d.-]+) ima=(-?\d+) soc=(-?\d+) rr=(\w+) ca=\d+ mode=\d+ dlpdr=([\d.]+) dlrssi=(-?\d+)")

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", a.port)); s.settimeout(0.3)

peers = {}  # id -> dict(latest fields + ts)


def bar(rssi):
    # map -90 dBm (empty) .. -25 dBm (full) to a 24-char bar
    n = max(0, min(24, round((rssi + 90) / 65 * 24)))
    return "#" * n + "-" * (24 - n)


def draw():
    now = time.time()
    out = ["\033[2J\033[H", "  net_bench live RSSI monitor  (Ctrl-C to quit)", ""]
    out.append(f"  {'id':6} {'RSSI':>5}  {'signal (-90..-25)':24}  {'up_PDR':>7} {'dl':>5} {'soc':>4} {'bv':>5} {'age':>4}")
    out.append("  " + "-" * 78)
    # weakest RSSI first (the board you're walking tends to be weakest)
    for pid, d in sorted(peers.items(), key=lambda kv: kv[1]["rssi"]):
        age = now - d["ts"]
        stale = age > a.stale
        dim = "\033[90m" if stale else ""
        rst = "\033[0m"
        out.append(f"  {dim}{pid:6} {d['rssi']:>4}d  [{bar(d['rssi'])}]  "
                   f"{d['pdr']*100:>6.1f}% {d['dlpdr']*100:>4.0f}% {d['soc']:>4} {d['bv']:>5.2f} {age:>3.0f}s{rst}")
    if not peers:
        out.append("  (waiting for the master bridge on :%d ...)" % a.port)
    print("\n".join(out), flush=True)


last = 0
try:
    while True:
        try:
            d, _ = s.recvfrom(1024)
            m = peer_re.search(d.decode(errors="replace"))
            if m:
                pid = m.group(1)
                peers[pid] = dict(rx=int(m.group(2)), gaps=int(m.group(3)), pdr=float(m.group(4)),
                                  rssi=int(m.group(5)), bv=float(m.group(6)), ima=int(m.group(7)),
                                  soc=int(m.group(8)), rr=m.group(9), dlpdr=float(m.group(10)),
                                  dlrssi=int(m.group(11)), ts=time.time())
        except socket.timeout:
            pass
        if time.time() - last >= 1.0:
            last = time.time(); draw()
except KeyboardInterrupt:
    print("\nbye", flush=True)
finally:
    s.close()
