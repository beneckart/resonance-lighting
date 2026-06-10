#!/usr/bin/env python3
"""Reconcile the INA219 monitor against PowerFeather SDK telemetry on the LED line.

Settles the "400 mA (power_bench) vs 36 mA (wattmeter)" discrepancy empirically.
`firmware/ina_monitor` computes current as shunt_mv / R with R = INA_RSHUNT_OHMS = 0.1,
but the DFRobot SEN0291 hardware uses a 10 mOhm (0.01) alloy shunt -> the monitor
UNDER-reports current by 0.1/0.01 = 10x.

This drives the LED via power_bench's /mode HTTP endpoint, reads INA ch 0x41 (the LED
hot line) over serial, and reads PowerFeather battery/supply current over WiFi, then
compares the LED-on-minus-off delta seen by each instrument:

    PF_current_delta  ?=  INA_reported x (R_code / R_true)   [expect ~10x]

  ./reconcile_ina_pf.py --led-ip 192.168.4.63 --ina-port /dev/ttyACM2
  ./reconcile_ina_pf.py --led-ip 192.168.4.63 --modes 1,5,4   # a few drive levels
"""
import argparse, json, re, time, urllib.request, statistics
import serial

ap = argparse.ArgumentParser()
ap.add_argument("--led-ip", required=True, help="power_bench board IP (serves /mode, /telemetry)")
ap.add_argument("--ina-port", default="/dev/ttyACM2")
ap.add_argument("--ina-ch", default="0x41", help="INA channel in series with the LED")
ap.add_argument("--modes", default="1", help="power_bench LED modes to step (comma sep); 1 = full white")
ap.add_argument("--r-code", type=float, default=0.1, help="R_shunt the firmware currently assumes")
ap.add_argument("--r-true", type=float, default=0.01, help="true SEN0291 shunt, 10 mOhm")
ap.add_argument("--avg", type=float, default=4.0, help="seconds to average INA per state")
ap.add_argument("--pf-window", type=float, default=20.0, help="max seconds to poll PF telemetry per state")
ap.add_argument("--pf-want", type=int, default=8, help="good PF samples to collect per state")
a = ap.parse_args()
corr = a.r_code / a.r_true  # factor to correct reported mA to true mA (= 10)

ser = serial.Serial(a.ina_port, 115200, timeout=0.3)
rx = re.compile(r"ina t=\d+ ch=(0x[0-9a-fA-F]+) bus_v=([\-\d.]+) shunt_mv=(-?[\d.]+) ma=(-?[\d.]+)")
ch = a.ina_ch.lower()

def read_ina(secs):
    ser.reset_input_buffer()
    ma, bv, t0 = [], [], time.time()
    while time.time() - t0 < secs:
        m = rx.search(ser.readline().decode("utf-8", "replace"))
        if m and m.group(1).lower() == ch:
            ma.append(float(m.group(4))); bv.append(float(m.group(2)))
    if not ma:
        return dict(ma=float("nan"), bus=float("nan"), n=0, sd=float("nan"))
    return dict(ma=statistics.mean(ma), bus=statistics.mean(bv), n=len(ma),
                sd=statistics.pstdev(ma) if len(ma) > 1 else 0.0)

def pf_avg(secs, want):
    # WiFi telemetry is brownout-flaky on battery; short timeout + many retries, stop
    # once we have `want` good samples or the window expires.
    sm, bm, sv, bvv, fails, t0 = [], [], [], [], 0, time.time()
    while time.time() - t0 < secs and len(bm) < want:
        try:
            d = json.loads(urllib.request.urlopen(f"http://{a.led_ip}/telemetry", timeout=2.0).read())
            sm.append(d.get("supply_ma") or 0.0); bm.append(d.get("battery_ma") or 0.0)
            sv.append(d.get("supply_v") or 0.0); bvv.append(d.get("battery_v") or 0.0)
        except Exception:
            fails += 1
        time.sleep(0.1)
    f = lambda x: statistics.mean(x) if x else float("nan")
    return dict(supply_ma=f(sm), batt_ma=f(bm), supply_v=f(sv), batt_v=f(bvv), n=len(bm), fails=fails)

def set_mode(m):
    urllib.request.urlopen(f"http://{a.led_ip}/mode?m={m}", timeout=6).read()

print(f"reconcile: INA ch {ch} @ {a.ina_port}  vs  PowerFeather telemetry @ {a.led_ip}")
print(f"INA correction = R_code {a.r_code} / R_true {a.r_true} = x{corr:.0f}\n")

set_mode("0"); time.sleep(1.0)
i0 = read_ina(a.avg); p0 = pf_avg(a.pf_window, a.pf_want)
print(f"OFF     INA ma={i0['ma']:+6.2f} (n={i0['n']}, sd={i0['sd']:.2f}) bus={i0['bus']:.3f} | "
      f"PF batt_ma={p0['batt_ma']:+.2f} supply_ma={p0['supply_ma']:.1f} "
      f"batt_v={p0['batt_v']:.3f} (pf n={p0['n']}/{p0['n']+p0['fails']})")

for m in [s.strip() for s in a.modes.split(",") if s.strip()]:
    set_mode(m); time.sleep(1.0)
    i1 = read_ina(a.avg); p1 = pf_avg(a.pf_window, a.pf_want)
    ina_d = i1["ma"] - i0["ma"]
    sup_d = p1["supply_ma"] - p0["supply_ma"]
    bat_d = p0["batt_ma"] - p1["batt_ma"]   # discharge is negative; on draws more -> positive delta
    ref = bat_d if abs(bat_d) > abs(sup_d) else sup_d
    src = "battery" if abs(bat_d) > abs(sup_d) else "supply"
    print(f"\nMODE {m} INA ma={i1['ma']:+6.2f} (n={i1['n']}, sd={i1['sd']:.2f}) bus={i1['bus']:.3f} | "
          f"PF batt_ma={p1['batt_ma']:+.2f} supply_ma={p1['supply_ma']:.1f} "
          f"batt_v={p1['batt_v']:.3f} (pf n={p1['n']}/{p1['n']+p1['fails']})")
    print(f"  LED delta:  INA reported {ina_d:+.2f} mA  ->  corrected x{corr:.0f} = {ina_d * corr:+.1f} mA")
    print(f"              PF {src} delta = {ref:+.1f} mA")
    if abs(ina_d) > 0.05 and abs(ref) > 1:
        print(f"  ==> PF_delta / INA_reported = {ref / ina_d:.1f}x   (expect ~{corr:.0f} if the shunt is {a.r_true} ohm)")

set_mode("0")
print("\nLED off. done.")
