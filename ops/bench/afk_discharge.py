#!/usr/bin/env python3
"""Full-discharge capacity + fuel-gauge learn run (AGGRESSIVE -- runs to empty/brownout).

From a (near-)full cell on battery, holds a FIXED LED load and INA-integrates the battery
current (0x45) = TRUE usable capacity, while logging the gauge's SOC/V so its flat-plateau
behavior and learn cycle are captured end-to-end. Runs deep on purpose (this bench cell is a
mule; degradation OK -- the goal is true capacity + failure modes). Captures the LFP knee and,
if pushed far enough, the board's brownout cascade.

Pairs with afk_sweep.py / afk_analyze.py (reuse afk_analyze for the gauge-vs-INA plots).

PRE-REQS (do these in the morning, board present so you can tap reset if the 3V3 rail sulks):
  1. Cell charged ~full overnight on USB (charging is enabled in the flashed firmware).
  2. For a deep run PAST the 2.90 V firmware guard, flash a low floor first (board on USB):
       firmware/power_bench/build.sh --led rgbw1 --cap 2000 --chem lfp --charge-ma 500 \
         --pixel-pin 10 --batt-floor 2.3 --port /dev/ttyACM1
     Then tap the physical reset (RTS/3V3 gotcha). Without this, the board self-protects at
     2.90 V -> you still get a clean *capacity-to-2.90 V* run, just not the deep tail.
  3. UNPLUG USB so the cell discharges (this script waits for the unplug unless --allow-usb).

  ./afk_discharge.py --led-ip 192.168.4.63 --ina-port /dev/ttyACM2
  ./afk_discharge.py --led-ip ... --load RGBW --cutoff-v 2.5     # standard LFP empty
  ./afk_discharge.py --led-ip ... --cutoff-v 2.1                 # let it ride to brownout

Stdlib + pyserial. Writes data/<site>/<date>-discharge-<hhmm>.jsonl + a live table.
"""
import argparse, json, os, re, time, urllib.request, statistics
import serial
from datetime import datetime, timezone

LOADS = {"RGBW": (255, 255, 255, 255), "RGB": (255, 255, 255, 0), "W": (0, 0, 0, 255)}

ap = argparse.ArgumentParser()
ap.add_argument("--led-ip", required=True)
ap.add_argument("--ina-port", default="/dev/ttyACM2")
ap.add_argument("--led-ch", default="0x41")
ap.add_argument("--batt-ch", default="0x45")
ap.add_argument("--load", default="RGBW", choices=list(LOADS) + ["custom"])
ap.add_argument("--r", type=int, default=255); ap.add_argument("--g", type=int, default=255)
ap.add_argument("--b", type=int, default=255); ap.add_argument("--w", type=int, default=255)
ap.add_argument("--bri", type=int, default=255); ap.add_argument("--gamma", type=int, default=0)
ap.add_argument("--cutoff-v", type=float, default=2.5, help="stop when battery_v (under load) drops below this")
ap.add_argument("--sample-s", type=float, default=10.0, help="seconds between samples")
ap.add_argument("--ina-avg", type=float, default=2.0, help="seconds of INA averaging per sample")
ap.add_argument("--max-min", type=float, default=720.0, help="absolute time cap (min)")
ap.add_argument("--down-stop-s", type=float, default=120.0, help="stop if unreachable this long (=brownout/protect)")
ap.add_argument("--allow-usb", action="store_true", help="skip the wait-for-USB-unplug at start")
ap.add_argument("--site", default="ca")
ap.add_argument("--notes", default="aggressive full discharge: capacity + gauge learn (mule cell)")
ap.add_argument("--out", default=None)
a = ap.parse_args()

r, g, b, w = (a.r, a.g, a.b, a.w) if a.load == "custom" else LOADS[a.load]
ledch, battch = a.led_ch.lower(), a.batt_ch.lower()

ser = serial.Serial(a.ina_port, 115200, timeout=0.3)
rx = re.compile(r"ina t=\d+ ch=(0x[0-9a-fA-F]+) bus_v=([\-\d.]+) shunt_mv=(-?[\d.]+) ma=(-?[\d.]+)")

def read_ina(secs):
    ser.reset_input_buffer()
    acc = {}
    t0 = time.time()
    while time.time() - t0 < secs:
        m = rx.search(ser.readline().decode("utf-8", "replace"))
        if m:
            acc.setdefault(m.group(1).lower(), []).append((float(m.group(4)), float(m.group(2))))
    return {ch: dict(ma=statistics.mean(x[0] for x in v), bus=statistics.mean(x[1] for x in v), n=len(v))
            for ch, v in acc.items()}

def http(path, timeout=3.0):
    return urllib.request.urlopen(f"http://{a.led_ip}{path}", timeout=timeout).read().decode()

def telem(retries=4):
    for _ in range(retries):
        try:
            return json.loads(http("/telemetry", timeout=4.0))
        except Exception:
            time.sleep(0.4)
    return None

def drive():
    http(f"/set?r={r}&g={g}&b={b}&w={w}&bri={a.bri}&gamma={a.gamma}")

def wait_reachable(deadline_s):
    t0 = time.time()
    while time.time() - t0 < deadline_s:
        d = telem(retries=1)
        if d:
            return d
        time.sleep(2.0)
    return None

now0 = datetime.now(timezone.utc)
out = a.out or os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", a.site,
                            now0.strftime("%Y-%m-%d") + "-discharge-" + now0.strftime("%H%M") + ".jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)
print(f"afk_discharge -> {out}")
print(f"load {a.load} rgbw=({r},{g},{b},{w}) bri={a.bri} gamma={a.gamma} | cutoff {a.cutoff_v} V | max {a.max_min} min")

d0 = wait_reachable(60)
if not d0:
    print("board not reachable; aborting"); raise SystemExit(1)
# wait for the cell to be on battery (USB unplugged) so it actually discharges
if not a.allow_usb:
    while (d0.get("supply_v") or 0) > 4.0:
        print(f"  >>> UNPLUG USB to start the discharge (supply_v={d0.get('supply_v')}, battery_v={d0.get('battery_v')}, soc={d0.get('soc_pct')})")
        time.sleep(5)
        d0 = telem() or d0
soc_start = d0.get("soc_pct")
print(f"start (on battery): battery_v={d0.get('battery_v')} soc={soc_start}% supply_v={d0.get('supply_v')}")

mah_ina = mah_gauge = 0.0
t_run0 = t_last = time.time()
last_uptime = d0.get("uptime_ms", 0)
resets = 0
fh = open(out, "w")
stop = "max-min"
lowstreak = 0
print(f"\n{'min':>5} {'LED_mA':>7} {'battINA':>7} {'gauge_mA':>8} {'bv':>5} {'soc':>3} {'mAh_ina':>8} {'mAh_g':>7} {'note':>6}")
try:
    while (time.time() - t_run0) / 60.0 < a.max_min:
        # (re)assert the load each sample so a brownout-reset that blanked the LED re-drives
        try:
            drive()
        except Exception:
            d = wait_reachable(a.down_stop_s)
            if not d:
                stop = "unreachable (brownout/protect)"; break
        ina = read_ina(a.ina_avg)
        tel = telem()
        if not tel:
            d = wait_reachable(a.down_stop_s)
            if not d:
                stop = "unreachable (brownout/protect)"; break
            tel = d
        up = tel.get("uptime_ms", 0)
        reset = up < last_uptime
        if reset:
            resets += 1
        last_uptime = up

        led = ina.get(ledch, {}); batt = ina.get(battch, {})
        gma = tel.get("battery_ma"); bv = tel.get("battery_v")
        tnow = time.time(); dt_h = (tnow - t_last) / 3600.0; t_last = tnow
        if batt.get("ma") is not None:
            mah_ina += abs(batt["ma"]) * dt_h
        if gma is not None:
            mah_gauge += abs(gma) * dt_h

        row = dict(ts_utc=datetime.now(timezone.utc).isoformat(), site=a.site, notes=a.notes,
                   load=a.load, r=r, g=g, b=b, w=w, bri=a.bri, gamma=a.gamma,
                   led_ma=round(led.get("ma", float("nan")), 2), led_bus_v=round(led.get("bus", float("nan")), 3),
                   batt_ina_ma=round(batt.get("ma", float("nan")), 2),
                   batt_ina_bus_v=round(batt.get("bus", float("nan")), 3),
                   gauge_battery_ma=gma, gauge_battery_v=bv, gauge_soc=tel.get("soc_pct"),
                   gauge_health=tel.get("health_pct"), gauge_cycles=tel.get("cycles"),
                   supply_v=tel.get("supply_v"), mah_ina_integ=round(mah_ina, 2),
                   mah_gauge_integ=round(mah_gauge, 2), elapsed_s=round(tnow - t_run0, 1),
                   reset=reset, resets_total=resets)
        fh.write(json.dumps(row) + "\n"); fh.flush()
        print(f"{(tnow-t_run0)/60.0:>5.1f} {row['led_ma']:>7.1f} {row['batt_ina_ma']:>7.1f} "
              f"{str(gma):>8} {str(bv):>5} {str(tel.get('soc_pct')):>3} {mah_ina:>8.1f} {mah_gauge:>7.1f} "
              f"{('RESET' if reset else ''):>6}")

        # cutoff: sustained under-load voltage below floor (2 in a row -> real, not a blip)
        if bv is not None and 0.5 < bv < a.cutoff_v:
            lowstreak += 1
            if lowstreak >= 2:
                stop = f"cutoff {a.cutoff_v} V (bv={bv})"; break
        else:
            lowstreak = 0
        time.sleep(a.sample_s)
except KeyboardInterrupt:
    stop = "interrupted"
finally:
    try:
        http("/set?w=0&bri=0")  # LED off
    except Exception:
        pass
    fh.close()

socN = None
dN = telem()
if dN:
    socN = dN.get("soc_pct")
dur = (time.time() - t_run0) / 60.0
print(f"\nDONE ({stop}) after {dur:.1f} min | resets={resets}")
print(f"USABLE CAPACITY (INA 0x45 integral) = {mah_ina:.0f} mAh ; gauge integral = {mah_gauge:.0f} mAh")
print(f"gauge SOC {soc_start}% -> {socN}%  (compare INA mAh to SOC swing for the DesignCap fix)")
print("JSONL:", out, "\nNext: ./afk_analyze.py", out)
