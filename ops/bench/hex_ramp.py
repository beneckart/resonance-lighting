#!/usr/bin/env python3
"""Guarded multi-LED ramp for the HEX (37x SK6812) on power_bench: find the
stability ceiling WITHOUT crashing a battery-only board.

Two ramps (Ben's "all-on-max is unstable" concern -- approach the cliff from
below instead of stepping on it):
  1. COUNT ramp: full value (default 255), light 1 -> 37 pixels in steps.
  2. VALUE ramp: all 37 lit, value 16 -> 255 in steps.

At each step: /set, settle, read the INA (LED lead + battery lead) and the
board's /telemetry, then apply ABORT rules BEFORE stepping further:
  - gauge (terminal) battery V < --gauge-floor-v (default 3.05; the 06-10
    brownout cascade started at gauge ~2.97 under sustained load)
  - battery INA bus V, MEAN of the window, < --batt-floor-v (default 2.85;
    note the INA bus sits ~200 mV below the gauge terminal from harness+shunt
    drop, and single 10 Hz samples dip further on WiFi TX bursts -- means
    only, mins are normal transients)
  - LED rail bus V mean < --rail-floor-v (default 2.60; LEDs unstable ~2.7)
  - LED current > --max-led-ma (default 2500; INA PG/1 range is +-4 A)
  - /set or /telemetry HTTP failure, or board uptime went BACKWARD (= it
    reset) -- these two are the REAL instability detectors; the voltage
    floors are early warnings.
On abort: LEDs off, back-step recorded, ramp stops (the next ramp still runs,
starting safe). Everything logged to JSONL; the last safe step per ramp is the
headline. Ctrl-C = LEDs off + exit.

The board-side battery-floor guard (protect below ~2.90 V sustained) is the
backstop; this script is the polite layer that avoids ever invoking it.

  ./hex_ramp.py --led-ip 192.168.4.63
  ./hex_ramp.py --led-ip 192.168.4.63 --value 128   # gentler count ramp

Stdlib + pyserial.
"""
import argparse, json, os, re, time, urllib.request
from datetime import datetime, timezone

import serial

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
ap.add_argument("--led-ip", required=True)
ap.add_argument("--ina-port", default="/dev/ttyACM2")
ap.add_argument("--led-ch", default="0x41")
ap.add_argument("--batt-ch", default="0x45")
ap.add_argument("--counts", default="1,2,4,7,10,14,19,25,31,37")
ap.add_argument("--value", type=int, default=255, help="channel value for the count ramp")
ap.add_argument("--values", default="16,32,48,64,96,128,160,192,224,255",
                help="value steps for the all-pixels ramp")
ap.add_argument("--settle", type=float, default=1.5, help="s after /set before sampling")
ap.add_argument("--avg", type=float, default=2.5, help="s of INA samples per step")
ap.add_argument("--gauge-floor-v", type=float, default=3.05, help="abort: gauge terminal V (primary floor)")
ap.add_argument("--batt-floor-v", type=float, default=2.85, help="abort: battery INA bus V, window MEAN")
ap.add_argument("--rail-floor-v", type=float, default=2.60, help="abort: LED rail bus V, window MEAN")
ap.add_argument("--max-led-ma", type=float, default=2500.0)
ap.add_argument("--site", default="ca")
ap.add_argument("--notes", default="HEX 37px guarded instability ramp, battery-only")
ap.add_argument("--out", default=None)
a = ap.parse_args()

now0 = datetime.now(timezone.utc)
out = a.out or os.path.join(DATA_DIR, a.site,
                            f"{now0.strftime('%Y-%m-%d')}-hex-ramp-{now0.strftime('%H%M')}.jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)

ser = serial.Serial(a.ina_port, 115200, timeout=0.3)
rx = re.compile(r"ina t=\d+ ch=(0x[0-9a-fA-F]+) bus_v=([\-\d.]+) shunt_mv=(-?[\d.]+) ma=(-?[\d.]+)")
ledch, battch = a.led_ch.lower(), a.batt_ch.lower()


def led_set(**kw):
    q = "&".join(f"{k}={v}" for k, v in kw.items())
    urllib.request.urlopen(f"http://{a.led_ip}/set?{q}", timeout=4).read()


def telemetry():
    raw = urllib.request.urlopen(f"http://{a.led_ip}/telemetry", timeout=4).read()
    return json.loads(raw)


def read_ina(secs):
    ser.reset_input_buffer()
    acc = {}
    t0 = time.time()
    while time.time() - t0 < secs:
        m = rx.search(ser.readline().decode("utf-8", "replace"))
        if m:
            acc.setdefault(m.group(1).lower(), []).append((float(m.group(4)), float(m.group(2))))
    return {ch: dict(ma=sum(x[0] for x in v) / len(v), bus_v=sum(x[1] for x in v) / len(v),
                     bus_v_min=min(x[1] for x in v), n=len(v))
            for ch, v in acc.items()}


def off():
    try:
        led_set(r=0, g=0, b=0, bri=0, n=0)
    except OSError:
        pass


last_uptime = 0


def step(ramp, n, val, fh):
    """One guarded step. Returns (ok, reasons)."""
    global last_uptime
    reasons = []
    try:
        led_set(r=val, g=val, b=val, bri=255, gamma=0, n=n)
    except OSError:
        time.sleep(1.5)  # battery WiFi power-save can stall the first request
        try:
            led_set(r=val, g=val, b=val, bri=255, gamma=0, n=n)
        except OSError as e:
            print(f"  {ramp}: n={n:>2} val={val:>3}  SET FAILED ({e}) - aborting ramp")
            return False, [f"set-failed({e})"]
    time.sleep(a.settle)
    ina = read_ina(a.avg)
    led = ina.get(ledch, {})
    batt = ina.get(battch, {})
    try:
        t = telemetry()
        if t["uptime_ms"] < last_uptime:
            reasons.append(f"BOARD RESET (rr={t.get('reset_reason')})")
        last_uptime = t["uptime_ms"]
        if t.get("battery_v", 99) < a.gauge_floor_v:
            reasons.append(f"gauge-sag {t['battery_v']:.3f}V < {a.gauge_floor_v}")
    except OSError as e:
        t = {}
        reasons.append(f"telemetry-failed({e})")
    if batt.get("bus_v", 99) < a.batt_floor_v:
        reasons.append(f"batt-sag mean {batt['bus_v']:.3f}V < {a.batt_floor_v}")
    if led.get("bus_v", 99) < a.rail_floor_v:
        reasons.append(f"rail-sag mean {led['bus_v']:.3f}V < {a.rail_floor_v}")
    if abs(led.get("ma", 0)) > a.max_led_ma:
        reasons.append(f"led-current {led['ma']:.0f}mA > {a.max_led_ma:.0f}")
    row = dict(site=a.site, notes=a.notes, ts_utc=datetime.now(timezone.utc).isoformat(),
               ramp=ramp, n=n, value=val,
               led_ma=round(led.get("ma", float("nan")), 1),
               led_bus_v=round(led.get("bus_v", float("nan")), 3),
               led_bus_v_min=round(led.get("bus_v_min", float("nan")), 3),
               batt_ina_ma=round(batt.get("ma", float("nan")), 1),
               batt_ina_bus_v=round(batt.get("bus_v", float("nan")), 3),
               batt_ina_bus_v_min=round(batt.get("bus_v_min", float("nan")), 3),
               gauge_battery_v=t.get("battery_v"), gauge_battery_ma=t.get("battery_ma"),
               soc_pct=t.get("soc_pct"), reset_reason=t.get("reset_reason"),
               abort=bool(reasons), abort_reasons=reasons)
    fh.write(json.dumps(row) + "\n")
    fh.flush()
    print(f"  {ramp}: n={n:>2} val={val:>3}  led={row['led_ma']:>7.1f}mA "
          f"rail={row['led_bus_v']:.3f}V(min {row['led_bus_v_min']:.3f}) "
          f"batt={row['batt_ina_bus_v']:.3f}V(min {row['batt_ina_bus_v_min']:.3f}) "
          f"{'ABORT: ' + '; '.join(reasons) if reasons else 'ok'}")
    return not reasons, reasons


print(f"hex_ramp -> {out}")
print(f"guards: gauge>{a.gauge_floor_v}V battINA-mean>{a.batt_floor_v}V rail-mean>{a.rail_floor_v}V "
      f"led<{a.max_led_ma:.0f}mA + reset/HTTP")
counts = [int(x) for x in a.counts.split(",")]
values = [int(x) for x in a.values.split(",")]
try:
    last_uptime = telemetry()["uptime_ms"]
except OSError:
    pass
ceilings = {}
try:
    with open(out, "w") as fh:
        print(f"\n[1/2] COUNT ramp at value {a.value} (watch the HEX for flicker/glitches):")
        safe = None
        for n in counts:
            ok, _ = step("count", n, a.value, fh)
            if not ok:
                off()
                break
            safe = n
        ceilings["count"] = safe
        off()
        time.sleep(2)
        print(f"\n[2/2] VALUE ramp at n=37:")
        safe = None
        for v in values:
            ok, _ = step("value", 37, v, fh)
            if not ok:
                off()
                break
            safe = v
        ceilings["value"] = safe
finally:
    off()
    ser.close()
print(f"\nDONE -> {out}")
print(f"last SAFE step: count ramp n={ceilings.get('count')} @ value {a.value}; "
      f"value ramp val={ceilings.get('value')} @ n=37")
print("(electrical guards only -- visual flicker/data glitches are yours to call; "
      "re-run with --counts/--values to bisect finer)")
