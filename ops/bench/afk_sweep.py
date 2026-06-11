#!/usr/bin/env python3
"""Unattended LED power-characterization + fuel-gauge cross-check sweep.

Repeatedly sweeps the PowerFeather's single RGBW pixel over (RGB, W, RGBW) x
(gamma off/on) x brightness, via power_bench's /set endpoint over WiFi. At each
point it records, in one JSONL row:
  - INA 0x41  : LED current (ground truth, shunt-corrected in firmware)
  - INA 0x45  : battery current (ground truth board+LED draw; battery side)
  - fuel gauge: battery_ma, battery_v, soc, health, cycles, supply_v/ma (over /telemetry)
plus running coulomb integrals from BOTH the INA battery channel and the gauge,
so the gauge's instantaneous current and its SOC/coulomb accounting can be checked
against the INA ground truth (-> can we recalibrate the MAX17260?).

Designed to run for ~2 h on battery, AFK:
  - Conservative host-side voltage floor (default 2.95 V): stop sweeping + LED off.
  - Tolerates board resets / WiFi drops (retries; logs 'reset'/'gap' events).
  - The firmware has its own 2.90 V backstop (cuts LED rail + WiFi) if WiFi is lost.

  ./afk_sweep.py --led-ip 192.168.4.63 --ina-port /dev/ttyACM2
  ./afk_sweep.py --led-ip 192.168.4.63 --max-minutes 120 --floor-v 2.95

Stdlib + pyserial. Writes data/<site>/<date>-afk-sweep-<hhmm>.jsonl + live table.
"""
import argparse, json, os, re, time, urllib.request, statistics
import serial
from datetime import datetime, timezone

ap = argparse.ArgumentParser()
ap.add_argument("--led-ip", required=True)
ap.add_argument("--ina-port", default="/dev/ttyACM2")
ap.add_argument("--led-ch", default="0x41")
ap.add_argument("--batt-ch", default="0x45")
ap.add_argument("--levels", default="0,16,32,48,64,96,128,160,192,224,255")
ap.add_argument("--patterns", default="RGB,W,RGBW")
ap.add_argument("--gammas", default="0,1")
ap.add_argument("--settle", type=float, default=1.2, help="s after /set before sampling (INA 128-avg lags)")
ap.add_argument("--avg", type=float, default=2.5, help="s of INA samples to average/point")
ap.add_argument("--floor-v", type=float, default=2.90, help="RESTING (LED-off) battery-V cutoff; not under-load sag")
ap.add_argument("--budget-mah", type=float, default=200.0, help="coulomb budget: stop after this much removed (sag-immune, primary)")
ap.add_argument("--max-minutes", type=float, default=120.0)
ap.add_argument("--down-stop-s", type=float, default=180.0, help="give up if board unreachable this long")
ap.add_argument("--site", default="ca")
ap.add_argument("--notes", default="AFK RGBW/RGB/W x gamma sweep + INA-vs-gauge; shunt-corrected (0.01 ohm)")
ap.add_argument("--out", default=None)
a = ap.parse_args()

# Which channels each pattern drives. We sweep the channel VALUE (0..255) at full
# brightness so gamma (applied per-channel in firmware) actually bends the mid-range
# (sweeping `bri` with channels pinned at 255 makes gamma a no-op -- gamma8(255)=255).
PAT_CH = {"RGB": ("r", "g", "b"), "W": ("w",), "RGBW": ("r", "g", "b", "w")}
levels = [int(x) for x in a.levels.split(",")]
patterns = [p.strip() for p in a.patterns.split(",") if p.strip()]
gammas = [int(x) for x in a.gammas.split(",")]
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
            try:
                if abs(float(m.group(4))) > 4500:  # beyond the INA's +-4A range = corrupt-but-parseable line
                    continue
                acc.setdefault(m.group(1).lower(), []).append(
                    (float(m.group(4)), float(m.group(2)), float(m.group(3))))
            except ValueError:
                pass  # mangled serial line -- skip
    out = {}
    for ch, v in acc.items():
        out[ch] = dict(ma=statistics.mean(x[0] for x in v), bus=statistics.mean(x[1] for x in v),
                       shmv=statistics.mean(x[2] for x in v), n=len(v))
    return out

def http(path, timeout=3.0):
    return urllib.request.urlopen(f"http://{a.led_ip}{path}", timeout=timeout).read().decode()

def set_led(r, g, b, w, bri, gamma):
    http(f"/set?r={r}&g={g}&b={b}&w={w}&bri={bri}&gamma={gamma}")

def telemetry(retries=3):
    for _ in range(retries):
        try:
            return json.loads(http("/telemetry", timeout=4.0))
        except Exception:
            time.sleep(0.4)
    return None

def wait_reachable(deadline_s):
    """Block until /telemetry responds or down-stop deadline; return telem or None."""
    t0 = time.time()
    while time.time() - t0 < deadline_s:
        d = telemetry(retries=1)
        if d:
            return d
        time.sleep(2.0)
    return None

now0 = datetime.now(timezone.utc)
out = a.out or os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", a.site,
                            now0.strftime("%Y-%m-%d") + "-afk-sweep-" + now0.strftime("%H%M") + ".jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)

print(f"afk_sweep -> {out}")
print(f"patterns={patterns} gammas={gammas} levels={levels}")
print(f"floor={a.floor_v} V  max={a.max_minutes} min")

d0 = wait_reachable(60)
if not d0:
    print("board not reachable at start; aborting"); raise SystemExit(1)
soc_start = d0.get("soc_pct")
print(f"start: battery_v={d0.get('battery_v')} soc={soc_start} supply_v={d0.get('supply_v')}")

mah_ina = 0.0       # coulomb integral from INA battery channel (ground truth)
mah_gauge = 0.0     # coulomb integral from gauge battery_ma
resting_bv = d0.get("battery_v")  # LED-off baseline V (sag-immune floor reference)
t_run0 = time.time()
t_last = t_run0
last_uptime = d0.get("uptime_ms", 0)
fh = open(out, "w")
cyc = 0
stop_reason = "max-minutes"
print(f"\n{'cyc':>3} {'g':>1} {'pat':>4} {'lvl':>4} {'LED_mA':>7} {'battINA':>7} {'gauge_mA':>8} "
      f"{'bv':>5} {'soc':>3} {'mAh_ina':>7} {'mAh_g':>6}")

try:
    while (time.time() - t_run0) / 60.0 < a.max_minutes:
        cyc += 1
        for gamma in gammas:
            for pat in patterns:
                chans = PAT_CH[pat]
                for level in levels:
                    r = level if "r" in chans else 0
                    g = level if "g" in chans else 0
                    b = level if "b" in chans else 0
                    w = level if "w" in chans else 0
                    bri = 255
                    # drive; tolerate transient WiFi/reset
                    for attempt in range(3):
                        try:
                            set_led(r, g, b, w, bri, gamma); break
                        except Exception:
                            d = wait_reachable(a.down_stop_s)
                            if not d:
                                stop_reason = "unreachable"; raise KeyboardInterrupt
                    time.sleep(a.settle)
                    ina = read_ina(a.avg)
                    tel = telemetry()
                    if not tel:
                        d = wait_reachable(a.down_stop_s)
                        if not d:
                            stop_reason = "unreachable"; raise KeyboardInterrupt
                        tel = d
                    # reset detection (uptime went backwards) -> board rebooted (LED now off)
                    up = tel.get("uptime_ms", 0)
                    reset = up < last_uptime
                    last_uptime = up

                    led = ina.get(ledch, {})
                    batt = ina.get(battch, {})
                    gauge_ma = tel.get("battery_ma")
                    bv = tel.get("battery_v")

                    # coulomb integrate over wallclock since last sample
                    tnow = time.time(); dt_h = (tnow - t_last) / 3600.0; t_last = tnow
                    if batt.get("ma") is not None:
                        mah_ina += abs(batt["ma"]) * dt_h
                    if gauge_ma is not None:
                        mah_gauge += abs(gauge_ma) * dt_h

                    row = dict(
                        ts_utc=datetime.now(timezone.utc).isoformat(), site=a.site, notes=a.notes,
                        cycle=cyc, gamma=gamma, pattern=pat, level=level, bri=bri, r=r, g=g, b=b, w=w,
                        led_ma=round(led.get("ma", float("nan")), 2),
                        led_bus_v=round(led.get("bus", float("nan")), 3),
                        led_shunt_mv=round(led.get("shmv", float("nan")), 3), led_n=led.get("n", 0),
                        batt_ina_ma=round(batt.get("ma", float("nan")), 2),
                        batt_ina_bus_v=round(batt.get("bus", float("nan")), 3), batt_ina_n=batt.get("n", 0),
                        gauge_battery_ma=gauge_ma, gauge_battery_v=bv,
                        gauge_soc=tel.get("soc_pct"), gauge_health=tel.get("health_pct"),
                        gauge_cycles=tel.get("cycles"), gauge_time_left_min=tel.get("time_left_min"),
                        supply_v=tel.get("supply_v"), supply_ma=tel.get("supply_ma"),
                        mah_ina_integ=round(mah_ina, 2), mah_gauge_integ=round(mah_gauge, 2),
                        elapsed_s=round(tnow - t_run0, 1), reset=reset)
                    fh.write(json.dumps(row) + "\n"); fh.flush()
                    print(f"{cyc:>3} {gamma:>1} {pat:>4} {level:>4} {row['led_ma']:>7.1f} "
                          f"{row['batt_ina_ma']:>7.1f} {str(gauge_ma):>8} {str(bv):>5} {str(tel.get('soc_pct')):>3} "
                          f"{mah_ina:>7.1f} {mah_gauge:>6.1f}" + ("  <reset>" if reset else ""))

                    # update resting baseline at LED-off points (sag-immune reference)
                    if level == 0 and bv is not None and bv > 0.5:
                        resting_bv = bv
                    # cutoffs: coulomb budget (primary, sag-immune) + resting-V floor.
                    # Deliberately do NOT cut on under-load bv -- LFP sags hard then recovers.
                    if max(mah_ina, mah_gauge) >= a.budget_mah:
                        stop_reason = f"coulomb-budget {a.budget_mah}mAh (ina={mah_ina:.0f} gauge={mah_gauge:.0f})"
                        raise KeyboardInterrupt
                    if resting_bv is not None and 0.5 < resting_bv < a.floor_v:
                        stop_reason = f"resting-floor {a.floor_v}V (resting_bv={resting_bv})"
                        raise KeyboardInterrupt
except KeyboardInterrupt:
    pass
finally:
    try:
        set_led(0, 0, 0, 0, 0, 0)  # LED off
    except Exception:
        pass
    fh.close()

dur_min = (time.time() - t_run0) / 60.0
print(f"\nDONE ({stop_reason}) after {dur_min:.1f} min, {cyc} cycles.")
print(f"coulomb removed: INA-battery {mah_ina:.1f} mAh | gauge {mah_gauge:.1f} mAh | "
      f"SOC {soc_start}% -> (see last row)")
print("JSONL:", out)
