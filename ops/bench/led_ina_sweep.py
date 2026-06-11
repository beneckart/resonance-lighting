#!/usr/bin/env python3
"""LED current-vs-brightness sweep, gauge-independent.

Drives an `led_studio` SK6812-RGBW pixel over its HTTP /set API and reads the INA219
4-channel monitor (Metro S3, serial) at each step -- so we get the LED's real draw vs
brightness for {RGB, W, RGB+W} x gamma {off, on}. The LED's hot line goes through one
INA channel (default 0x41); any other present channels (e.g. 0x45 = board battery) are
logged too.

Wiring assumed: a PowerFeather runs `firmware/led_studio` (on WiFi, serves /set), driving
the RGBW pixel; the pixel's V+ passes through INA `IN+/IN-`; the Metro runs
`firmware/ina_monitor` on --ina-port.

  ./led_ina_sweep.py --led-ip 192.168.4.XX
  ./led_ina_sweep.py --led-ip 192.168.4.XX --levels 0,32,64,128,192,255 --avg 1.0

Writes JSONL to data/<site>/<date>-led-ina-sweep-<hhmm>.jsonl + a live table. Turns the
LED off at the end. Stdlib + pyserial.
"""
import argparse, json, os, re, time, urllib.request
import serial
from datetime import datetime, timezone

ap = argparse.ArgumentParser()
ap.add_argument("--led-ip", required=True, help="led_studio board IP (serves /set)")
ap.add_argument("--ina-port", default="/dev/ttyACM2")
ap.add_argument("--led-ch", default="0x41", help="INA channel on the LED hot line")
ap.add_argument("--levels", default="0,8,16,24,32,48,64,96,128,160,192,224,255",
                help="brightness (bri 0..255) steps to sweep")
ap.add_argument("--settle", type=float, default=0.5, help="s after /set before sampling")
ap.add_argument("--avg", type=float, default=0.8, help="s of INA samples to average/step")
ap.add_argument("--site", default="ca")
ap.add_argument("--notes", default="")
ap.add_argument("--out", default=None)
a = ap.parse_args()

levels = [int(x) for x in a.levels.split(",")]
# mode is RGBW (1) throughout; the channel pattern picks RGB-white / W-only / RGB+W.
MODES = [("RGB",  dict(r=255, g=255, b=255, w=0)),
         ("W",    dict(r=0, g=0, b=0, w=255)),
         ("RGBW", dict(r=255, g=255, b=255, w=255))]
ledch = a.led_ch.lower()

def led_set(**kw):
    q = "&".join(f"{k}={v}" for k, v in kw.items())
    urllib.request.urlopen(f"http://{a.led_ip}/set?{q}", timeout=4).read()

ser = serial.Serial(a.ina_port, 115200, timeout=0.3)
rx = re.compile(r"ina t=\d+ ch=(0x[0-9a-fA-F]+) bus_v=([\d.]+) shunt_mv=(-?[\d.]+) ma=(-?[\d.]+)")

def read_ina(secs):
    acc = {}
    t0 = time.time()
    while time.time() - t0 < secs:
        m = rx.search(ser.readline().decode("utf-8", "replace"))
        if m:
            ch = m.group(1).lower()
            try:
                acc.setdefault(ch, []).append((float(m.group(4)), float(m.group(2))))
            except ValueError:
                pass  # mangled serial line -- skip
    return {ch: (sum(x[0] for x in v) / len(v), sum(x[1] for x in v) / len(v), len(v))
            for ch, v in acc.items()}

now0 = datetime.now(timezone.utc)
out = a.out or os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", a.site,
                            now0.strftime("%Y-%m-%d") + "-led-ina-sweep-" + now0.strftime("%H%M") + ".jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)
led_set(mode=1, anim=0)  # RGBW, static
print(f"led_ina_sweep -> {out}")
print(f"{'gamma':5} {'mode':5} {'bri':>4} {'LED_mA':>9} {'bus_v':>6}  (n)")
with open(out, "w") as fh:
    for gamma in (0, 1):
        for mname, ch in MODES:
            for bri in levels:
                led_set(mode=1, anim=0, gamma=gamma, bri=bri, **ch)
                time.sleep(a.settle)
                ser.reset_input_buffer()  # drop stale, then collect fresh
                stats = read_ina(a.avg)
                led = stats.get(ledch, (float("nan"), float("nan"), 0))
                row = dict(ts_utc=datetime.now(timezone.utc).isoformat(), site=a.site,
                           notes=a.notes, gamma=gamma, mode=mname, **ch, bri=bri,
                           led_ch=ledch, led_ma=round(led[0], 2), led_bus_v=round(led[1], 3),
                           all_ch={k: round(v[0], 2) for k, v in stats.items()})
                fh.write(json.dumps(row) + "\n"); fh.flush()
                print(f"{'on' if gamma else 'off':5} {mname:5} {bri:>4} {led[0]:9.2f} {led[1]:6.3f}  ({led[2]})")
    led_set(mode=1, anim=0, bri=0, r=0, g=0, b=0, w=0)  # off
print("done; LED off. JSONL:", out)
