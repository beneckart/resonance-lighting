#!/usr/bin/env python3
"""Listen for power_bench --loadgen UDP heartbeats and log them to JSONL.

The loadgen heartbeat (broadcast to :54321) carries phase + uptime + battery
V/I + SOC, so this gives a remote, battery-powered trace with no USB tether:
reboot detection (uptime drops), the V-SOC discharge curve, and an LED-current
A/B (LED-on vs LED-off phases at matched WiFi). Stdlib only.

    python3 ops/bench/loadgen_log.py <out.jsonl> [--ip 192.168.4.199] [--secs 1800]
"""
import socket, sys, re, json, time, argparse
from datetime import datetime, timezone

ap = argparse.ArgumentParser()
ap.add_argument("out")
ap.add_argument("--ip", default=None, help="only log this board IP")
ap.add_argument("--secs", type=float, default=3600)
ap.add_argument("--port", type=int, default=54321)
a = ap.parse_args()

pat = re.compile(
    r"ph=(\d+) led=(\d+) heavy=(\d+) up=(\d+) bv=([\d.]+) ima=(-?[\d.]+) "
    r"soc=(-?\d+) mah=([\d.]+) rr=(\w+) lb=(\d)")
psleep = re.compile(r"SLEEPING why=(\S+) up=(\d+) bv=([\d.]+) soc=(-?\d+) mah=([\d.]+)")

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", a.port)); s.settimeout(1.0)

t0 = time.time(); last_up = None; reb = 0; n = 0
# LED-current A/B accumulators: mean battery_ma per (heavy, led)
acc = {(h, l): [0.0, 0] for h in (0, 1) for l in (0, 1)}
print(f"logging loadgen -> {a.out}  (ip={a.ip or 'any'}, {a.secs:.0f}s). reboots flagged inline.", flush=True)
with open(a.out, "w") as fh:
    while time.time() - t0 < a.secs:
        try:
            d, addr = s.recvfrom(600)
        except socket.timeout:
            continue
        if a.ip and addr[0] != a.ip:
            continue
        text = d.decode(errors="replace")
        now = time.time()
        sm = psleep.search(text)
        if sm:  # board announced deep sleep -> log a marker row and note it
            print(f"+{now-t0:6.0f}s  *** BOARD SLEEPING why={sm.group(1)} bv={sm.group(3)} "
                  f"soc={sm.group(4)} mah={sm.group(5)} *** (overnight guard tripped)", flush=True)
            fh.write(json.dumps({"ts_utc": datetime.now(timezone.utc).isoformat(),
                                 "elapsed_s": round(now - t0, 1), "ip": addr[0], "event": "sleeping",
                                 "why": sm.group(1), "battery_v": float(sm.group(3)),
                                 "soc_pct": int(sm.group(4)), "mah_used": float(sm.group(5))}) + "\n")
            fh.flush()
            continue
        m = pat.search(text)
        if not m:
            continue
        ph, led, heavy, up, bv, ima, soc, mah, rr, lb = m.groups()
        ph, led, heavy, up = int(ph), int(led), int(heavy), int(up)
        bv, ima, soc, mah, lb = float(bv), float(ima), int(soc), float(mah), int(lb)
        if last_up is not None and up < last_up - 2000:
            reb += 1
            tag = "DEPLETION?" if (bv < 3.0 or lb) else "HEALTHY-bv!"
            print(f"+{now-t0:6.0f}s  REBOOT #{reb}  up {last_up}->{up}  rr={rr} bv~{bv:.3f} soc={soc} lb={lb} [{tag}]", flush=True)
        last_up = up
        if not lb:  # only count clean (non-backoff) samples in the A/B
            acc[(heavy, led)][0] += ima; acc[(heavy, led)][1] += 1
        fh.write(json.dumps({
            "ts_utc": datetime.now(timezone.utc).isoformat(),
            "elapsed_s": round(now - t0, 1), "ip": addr[0],
            "phase": ph, "led": led, "heavy": heavy, "uptime_ms": up,
            "battery_v": bv, "battery_ma": ima, "soc_pct": soc, "mah_used": mah,
            "lb": lb, "reset_reason": rr}) + "\n")
        fh.flush()
        n += 1
        if n % 200 == 0:
            print(f"+{now-t0:6.0f}s  n={n} ph={ph} led={led} heavy={heavy} bv={bv:.3f} soc={soc} mah={mah:.0f} ima={ima:.0f} reboots={reb}", flush=True)
s.close()

def mean(h, l):
    tot, c = acc[(h, l)]
    return (tot / c) if c else None

print("=== LED-current A/B (mean battery_ma, lb=0 samples; discharge negative) ===", flush=True)
for h in (0, 1):
    off, on = mean(h, 0), mean(h, 1)
    lab = "heavy-WiFi" if h else "light-WiFi"
    if off is not None and on is not None:
        print(f"  {lab}: LED off={off:.0f} mA, on={on:.0f} mA  -> LED draw ~{abs(on-off):.0f} mA", flush=True)
    else:
        print(f"  {lab}: insufficient samples (off={off}, on={on})", flush=True)
print(f"=== DONE  rows={n} reboots={reb} -> {a.out} ===", flush=True)
print("DONE", flush=True)
