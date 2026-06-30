#!/usr/bin/env python3
"""Measure LED efficiency vs brightness: pair PAR (light) with battery_ma (power).

Runs alongside a board flashed with `--bright-sweep` (loadgen steps brightness through
levels, reporting br= in its UDP heartbeat). This script reads the Apogee PAR sensor on
USB *and* listens for the board's heartbeat over WiFi, groups both by the brightness
step, and prints an efficiency table (PAR per LED-mA) -- for comparing LED modules
(NeoHEX vs HEX vs ...). The board runs on battery so battery_ma reflects real draw.

    python3 ops/bench/led_efficiency_sweep.py --label neohex --ip 192.168.4.199 \
        --par-port /dev/ttyACM0 --secs 260

Pair-of-runs workflow: run with --label neohex, swap the module, run with --label hex,
then compare the two saved JSON files.
"""
import argparse, socket, re, json, time, struct, statistics, sys
from datetime import datetime, timezone
import serial

ap = argparse.ArgumentParser()
ap.add_argument("--label", required=True, help="module label, e.g. neohex / hex")
ap.add_argument("--ip", default=None, help="board IP (heartbeat source filter)")
ap.add_argument("--par-port", default="/dev/ttyACM0")
ap.add_argument("--udp-port", type=int, default=54321)
ap.add_argument("--secs", type=float, default=260)
ap.add_argument("--out", default=None)
a = ap.parse_args()
out = a.out or f"ops/bench/data/ca/led-eff-{a.label}.json"

def read_par(ser):
    ser.reset_input_buffer()
    ser.write(b"\x55\x21")
    raw = ser.read(5)
    if len(raw) < 5:
        return None
    _status, v = struct.unpack("<bf", raw)
    return (v - 0.00171) * 26010

par = serial.Serial(a.par_port, baudrate=115200, xonxoff=False, timeout=0.5)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", a.udp_port)); s.settimeout(0.2)
pat = re.compile(r"up=(\d+) bv=([\d.]+) ima=(-?[\d.]+).*br=(\d+)")

print(f"sweep '{a.label}': PAR={a.par_port}, board={a.ip or 'any'}, {a.secs:.0f}s. "
      f"point the PAR sensor at the module, fixed distance.", flush=True)
samples = []  # (t, br, ima, parval)
t0 = time.time(); last_par = 0.0; last_par_t = 0; last_br = None
last_up = None; aborted = False
while time.time() - t0 < a.secs:
    now = time.time()
    if now - last_par_t > 1.0:  # poll PAR ~1/s
        p = read_par(par)
        if p is not None: last_par = p
        last_par_t = now
    try:
        d, addr = s.recvfrom(600)
        if a.ip and addr[0] != a.ip:
            continue
        m = pat.search(d.decode(errors="replace"))
        if m:
            up = int(m.group(1)); bv = float(m.group(2)); ima = float(m.group(3)); br = int(m.group(4))
            # SAFETY: a brightness step browned out the board (uptime dropped) -> abort
            # before it loops, and report the level it died at (the safe ceiling).
            if last_up is not None and up < last_up - 2000:
                print(f"\n!!! REBOOT DETECTED (up {last_up}->{up}, bv~{bv:.3f}) -- brightness ~{last_br} "
                      f"browned out the board. ABORTING. Replug USB to stop the loop. !!!", flush=True)
                aborted = True
                break
            last_up = up
            samples.append((now - t0, br, ima, last_par))
            if br != last_br:
                print(f"+{now-t0:5.0f}s  brightness -> {br:3d}   (PAR~{last_par:.0f}, ima~{ima:.0f}, bv~{bv:.3f})", flush=True)
                last_br = br
    except socket.timeout:
        pass
par.close(); s.close()
if aborted:
    print(f">>> aborted at/after brightness {last_br} -- treat that as the unstable ceiling on this rail/connection.", flush=True)

# Per-brightness summary: drop the first 40% of each step's samples (settling), median the rest.
by_br = {}
for t, br, ima, p in samples:
    by_br.setdefault(br, []).append((t, ima, p))
rows = []
for br in sorted(by_br):
    g = sorted(by_br[br])
    settled = g[int(len(g) * 0.4):] or g
    ima_med = statistics.median(x[1] for x in settled)
    par_med = statistics.median(x[2] for x in settled)
    rows.append({"brightness": br, "ima_mA": round(ima_med, 1),
                 "draw_mA": round(abs(ima_med), 1), "par": round(par_med, 1),
                 "n": len(settled)})
# baselines from the br=0 step (LED off)
base = next((r for r in rows if r["brightness"] == 0), None)
base_draw = base["draw_mA"] if base else 0.0
base_par = base["par"] if base else 0.0
print(f"\n=== {a.label}: efficiency (board+lightWiFi baseline: {base_draw:.0f} mA, {base_par:.0f} PAR) ===", flush=True)
print(f"{'bright':>6} {'draw_mA':>8} {'LED_mA':>7} {'PAR':>7} {'PARnet':>7} {'PAR/LED_mA':>11}", flush=True)
for r in rows:
    led_mA = r["draw_mA"] - base_draw
    par_net = r["par"] - base_par
    eff = (par_net / led_mA) if led_mA > 1 else float("nan")
    r["led_mA"] = round(led_mA, 1); r["par_net"] = round(par_net, 1)
    r["eff_par_per_mA"] = round(eff, 3) if eff == eff else None
    print(f"{r['brightness']:>6} {r['draw_mA']:>8.0f} {led_mA:>7.0f} {r['par']:>7.0f} {par_net:>7.0f} {eff:>11.3f}", flush=True)
result = {"label": a.label, "ts_utc": datetime.now(timezone.utc).isoformat(),
          "baseline_draw_mA": base_draw, "baseline_par": base_par, "rows": rows}
with open(out, "w") as fh:
    json.dump(result, fh, indent=2)
print(f"\nwrote {out}", flush=True)
print("DONE", flush=True)
