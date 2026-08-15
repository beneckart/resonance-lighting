#!/usr/bin/env python3
"""One-time INA channel-map + polarity check for a multi-rig bench (rat's-nest insurance).

Lights each rig's LEDs GREEN in turn (others dark) and watches which INA channels move:
the mover must be that rig's configured --led channel. On battery power the rig's batt
channel must also move, NEGATIVE (discharge); on USB / battery-less it won't move at all
(rerun at cell-connect time for the battery-polarity half of the check).

Verdicts printed per rig: LED ch mapped/mismapped, LED lead polarity, batt ch response +
sign, and any expected channel entirely absent from the stream (DIP switch / QT chain).

  ./ina_mapcheck.py --ina-port /dev/ttyACM3 \\
      --rig 9E5AF0=192.168.4.30:0x45:0x41 --rig 9E5B0C=192.168.4.31:0x40:0x44
  ./ina_mapcheck.py --ina-file data/ca/<date>-ina.log --rig ...   # if ina_logger owns the port

Stdlib + pyserial.
"""
import argparse, re, statistics, time, urllib.request

ap = argparse.ArgumentParser()
ap.add_argument("--rig", action="append", required=True,
                help="name=ip:battch:ledch  e.g. 9E5AF0=192.168.4.30:0x45:0x41")
ap.add_argument("--ina-port", default="/dev/ttyACM2")
ap.add_argument("--ina-file", default=None, help="tail an ina_logger.py file instead of the port")
ap.add_argument("--bri", type=int, default=60, help="test brightness (keep modest on USB power)")
ap.add_argument("--n", type=int, default=37)
ap.add_argument("--read-s", type=float, default=4.0)
ap.add_argument("--settle-s", type=float, default=1.5)
ap.add_argument("--thresh-ma", type=float, default=30.0, help="delta that counts as 'moved'")
a = ap.parse_args()

rigs = []
for spec in a.rig:
    name, rest = spec.split("=", 1)
    ip, battch, ledch = rest.split(":")
    rigs.append(dict(name=name, ip=ip, batt=battch.lower(), led=ledch.lower()))

ina_fh = ser = None
if a.ina_file:
    ina_fh = open(a.ina_file, "r")
    ina_fh.seek(0, 2)
else:
    import serial
    ser = serial.Serial(a.ina_port, 115200, timeout=0.3)
rx = re.compile(r"ina t=\d+ ch=(0x[0-9a-fA-F]+) bus_v=([\-\d.]+) shunt_mv=(-?[\d.]+) ma=(-?[\d.]+)")

def read_ina(secs):
    acc = {}
    def ingest(line):
        m = rx.search(line)
        if not m:
            return
        try:
            if abs(float(m.group(4))) > 4500:
                return
            acc.setdefault(m.group(1).lower(), []).append(float(m.group(4)))
        except ValueError:
            pass
    if ina_fh is not None:
        time.sleep(secs)
        for line in ina_fh.read().splitlines():
            ingest(line)
    else:
        ser.reset_input_buffer()
        t0 = time.time()
        while time.time() - t0 < secs:
            ingest(ser.readline().decode("utf-8", "replace"))
    return {ch: statistics.mean(v) for ch, v in acc.items() if v}

def set_led(ip, r, g, b, w, bri, n):
    urllib.request.urlopen(f"http://{ip}/set?r={r}&g={g}&b={b}&w={w}&bri={bri}&gamma=0&n={n}", timeout=5).read()

def all_off():
    for rg in rigs:
        set_led(rg["ip"], 0, 0, 0, 0, 0, 0)

fails = []
all_off(); time.sleep(a.settle_s)
base = read_ina(a.read_s)
print("baseline (all LEDs off):", " ".join(f"{ch}={ma:+.1f}mA" for ch, ma in sorted(base.items())) or "NO INA LINES SEEN")
if not base:
    raise SystemExit("no INA lines at all -- check monitor port/cable/firmware before going further")
for rg in rigs:
    for ch in (rg["batt"], rg["led"]):
        if ch not in base:
            fails.append(f"{rg['name']}: expected channel {ch} ABSENT from stream (DIP switch / QT chain)")

for rg in rigs:
    print(f"\n--- {rg['name']} ({rg['ip']}): GREEN bri={a.bri} n={a.n} ---")
    set_led(rg["ip"], 0, 255, 0, 0, a.bri, a.n)
    time.sleep(a.settle_s)
    lit = read_ina(a.read_s)
    all_off()
    deltas = {ch: lit.get(ch, 0.0) - base.get(ch, 0.0) for ch in set(base) | set(lit)}
    print("   deltas:", " ".join(f"{ch}={d:+.1f}mA" for ch, d in sorted(deltas.items())))
    movers = {ch for ch, d in deltas.items() if abs(d) >= a.thresh_ma}
    exp_led, exp_batt = rg["led"], rg["batt"]

    if exp_led in movers and deltas[exp_led] > 0:
        print(f"   PASS led ch {exp_led} +{deltas[exp_led]:.0f} mA (mapped, polarity ok)")
    elif exp_led in movers:
        print(f"   FAIL led ch {exp_led} moved NEGATIVE ({deltas[exp_led]:.0f} mA) -> VIN+/VIN- swapped on that shunt")
        fails.append(f"{rg['name']}: LED shunt polarity")
    else:
        wrong = [ch for ch in movers - {exp_batt} if abs(deltas[ch]) > 0]
        print(f"   FAIL led ch {exp_led} did not move" + (f"; {wrong} moved instead -> channel map is wrong" if wrong else " and nothing else did -> LED not wired/lit?"))
        fails.append(f"{rg['name']}: LED channel map")
    if exp_batt in movers:
        lvl = lit.get(exp_batt, 0.0)
        sign = "ok (negative = discharge)" if lvl < 0 else "WRONG SIGN -> battery shunt VIN+/VIN- swapped"
        print(f"   batt ch {exp_batt} responded ({deltas[exp_batt]:+.0f} mA, level {lvl:+.0f} mA): {sign}")
        if lvl >= 0:
            fails.append(f"{rg['name']}: battery shunt polarity")
    else:
        print(f"   batt ch {exp_batt} no response (expected if USB-powered/battery-less -- RERUN ON BATTERY before the real runs)")
    cross = [ch for ch in movers if ch not in (exp_led, exp_batt)]
    if cross:
        print(f"   WARN channels {cross} also moved -- crossed wiring between rigs?")
        fails.append(f"{rg['name']}: crosstalk {cross}")
    time.sleep(a.settle_s)

print("\n" + ("ALL CHECKS PASSED (rerun on battery for batt-shunt polarity if this was USB)" if not fails
      else "FAILURES:\n  " + "\n  ".join(fails)))
