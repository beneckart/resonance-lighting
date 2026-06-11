#!/usr/bin/env python3
"""Guided VINDPM/MPP sweep over a serial-bridge net_bench master.

Drives the clean full-sun MPP sweep (TODO / LOG 2026-06-08 cont. 10): steps the
outdoor peer's charger VINDPM via the master's `m<v10>` serial command
(NB_SET_MAINTAIN broadcast) and dwells at each setpoint collecting the peer's
heartbeats. Re-visits a 5.5 V ANCHOR every few points so light/temperature
drift is measured instead of silently corrupting the curve (the 06-08 lesson).

Light + panel temp come over the AIR, in the heartbeat itself (net_bench fw
2026-06-10.1+): a TSL2591 (lux) and SHT31 (tape it to the panel BACK = ~cell
temp) on the peer's STEMMA-QT chain -> `lux=/ptc=` tokens -> no laptop, rpi,
or PAR tether outdoors. The Apogee SQ-420 (--par-port) remains an optional
host-side cross-check for the indoor dry run. IR-gun prompts stay as the
panel-temp spot-check (front-surface; SHT31 is back-surface contact).

Topology (the validated 06-08 solar setup):
  - Outdoor peer: battery+panel only, net_bench peer (--hb-hz 1) with the
    TSL2591+SHT31 chained on STEMMA. NO USB on the peer (maintain > USB supply
    browns out).
  - Desk master: `./build.sh --role master --serial-bridge ...` on USB. This
    script OWNS that serial port: it reads the bridged nb-* lines AND writes
    the m<v10> commands. nb-* lines are re-broadcast to UDP:54321 by default
    so net_bench_log.py / net_bench_monitor.py can co-record as usual.

Cautions baked in:
  - SET_MAINTAIN is broadcast, UNACKED: each setpoint is re-sent 3x during the
    settle window so a missed packet can't silently leave the dwell at the old
    setpoint.
  - Light instability within a dwell, TSL2591 SATURATION (full sun can exceed
    its range -- a paper/PTFE diffuser fixes it; relative use survives that),
    and anchor drift across the session are FLAGGED (with a redo offer), not
    averaged away.
  - Warns on supply_good=0 / ~0 W (the dark-panel reseated-connector gotcha).
  - On exit (incl. Ctrl-C) the setpoint is restored to --restore-v10.

Usage (one session; run once cool-AM and once hot-midday):
  ./mpp_sweep.py --port /dev/ttyACM0 --session cool-am
  ./mpp_sweep.py --port /dev/ttyACM0 --session hot-noon
  ./mpp_sweep.py --port /dev/ttyACM0 --no-prompt   # unattended (skips IR prompts)

Writes data/<site>/<date>-mpp-sweep-<session>-<hhmm>.jsonl:
  src="mpp-sample" rows (every heartbeat, with lux/temps) and
  src="mpp-point" summary rows (per setpoint visit, with IR temps + flags).
Analyze with mpp_analyze.py. Stdlib + pyserial.
"""
import argparse, json, os, re, socket, statistics, sys, time
from datetime import datetime, timezone

import serial  # pyserial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from apogee_par import read_par  # noqa: E402

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

# Keep in sync with net_bench_log.py rx_peer (the sv=/sma= and lux=/ptc= tails are what we need).
RX_PEER = re.compile(
    r"nb-peer id=(\w+) seq=(\d+) rx=(\d+) gaps=(\d+) pdr=([\d.]+) rssi=(-?\d+) bv=([\d.-]+) "
    r"ima=(-?\d+) soc=(-?\d+) rr=(\w+) ca=(\d+) mode=(\d+) dlpdr=([\d.]+) dlrssi=(-?\d+) up=(\d+) age=(\d+)"
    r"(?: sv=([\d.-]+) sma=(-?\d+) sgood=(\d+))?"
    r"(?: lux=([\w.\-]+) ch0=(\d+) ch1=(\d+) ptc=([\w.\-]+) prh=(-?\d+) btc=([\w.\-]+))?"
    r"(?: ipv=(-?\d+) ipa=(-?\d+) ibv=(-?\d+) iba=(-?\d+))?")
RX_MAINT_ECHO = re.compile(r"broadcast SET_MAINTAIN ([\d.]+) V")

ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
ap.add_argument("--port", required=True, help="serial-bridge master USB port (e.g. /dev/ttyACM0)")
ap.add_argument("--baud", type=int, default=115200)
ap.add_argument("--par-port", default=None,
                help="OPTIONAL Apogee SQ-420 port for a host-side cross-check (e.g. /dev/ttyUSB0); "
                     "the primary light channel is the peer's TSL2591 in the heartbeat")
ap.add_argument("--no-par", action="store_true", help="don't try to open a host-side PAR sensor")
ap.add_argument("--peer-id", default=None, help="only accept heartbeats from this peer id (hex)")
ap.add_argument("--points", default="55,52,50,49,48,47,46,44",
                help="setpoints, volts x10 (peer accepts 40..58)")
ap.add_argument("--anchor", type=int, default=55, help="anchor setpoint (v10), re-visited for drift")
ap.add_argument("--anchor-every", type=int, default=3, help="re-visit the anchor every N swept points")
ap.add_argument("--settle-s", type=float, default=8.0, help="s after SET_MAINTAIN before sampling")
ap.add_argument("--dwell-s", type=float, default=60.0, help="s of heartbeat+PAR collection per point")
ap.add_argument("--light-cv-flag", type=float, default=0.08,
                help="flag a point when light (p95-p5)/median exceeds this within its dwell")
ap.add_argument("--anchor-drift-flag", type=float, default=0.10,
                help="warn when anchor PAR-normalized power drifts more than this across the session")
ap.add_argument("--batt-temp-every", type=int, default=3, help="prompt for battery/board IR temp every N points")
ap.add_argument("--no-prompt", action="store_true", help="skip IR-temp prompts and redo offers (unattended)")
ap.add_argument("--no-udp-relay", action="store_true", help="don't re-broadcast nb-* lines to UDP:54321")
ap.add_argument("--udp-port", type=int, default=54321)
ap.add_argument("--restore-v10", type=int, default=55, help="setpoint restored on exit")
ap.add_argument("--session", default="session", help="label, e.g. cool-am / hot-noon")
ap.add_argument("--site", default="ca")
ap.add_argument("--operator", default="ben")
ap.add_argument("--notes", default="")
a = ap.parse_args()

now0 = datetime.now(timezone.utc)
out = os.path.join(DATA_DIR, a.site,
                   f"{now0.strftime('%Y-%m-%d')}-mpp-sweep-{a.session}-{now0.strftime('%H%M')}.jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)
META = dict(site=a.site, operator=a.operator, session=a.session, notes=a.notes,
            settle_s=a.settle_s, dwell_s=a.dwell_s)

ser = serial.Serial(a.port, a.baud, timeout=0.2)
par_ser = None
if a.par_port and not a.no_par:  # optional cross-check only; lux rides the heartbeat
    try:
        par_ser = serial.Serial(a.par_port, baudrate=115200, xonxoff=False, timeout=0.5)
        print(f"PAR cross-check: {a.par_port} -> {read_par(par_ser):.0f} umol/m2/s")
    except (serial.SerialException, IOError) as e:
        print(f"WARNING: PAR sensor open failed ({e}); continuing without it.")

udp = None
if not a.no_udp_relay:
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)


def read_lines(max_s):
    """Yield decoded serial lines for up to max_s seconds, relaying nb-* to UDP."""
    end = time.time() + max_s
    while time.time() < end:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").rstrip("\r\n")
        if udp and line.startswith("nb-"):
            udp.sendto((line + "\n").encode(), ("255.255.255.255", a.udp_port))
        yield line


def _numok(s):  # "nan"/"sat"/None -> None
    if s is None:
        return None
    try:
        v = float(s)
        return None if v != v else v
    except ValueError:
        return None


def parse_peer(line):
    m = RX_PEER.search(line)
    if not m:
        return None
    g = m.groups()
    if a.peer_id and g[0].lower() != a.peer_id.lower():
        return None
    if g[16] is None:  # pre-supply-telemetry firmware: useless for harvest
        return None
    sv, sma = float(g[16]), int(g[17])
    s = dict(peer_id=g[0], battery_v=float(g[6]), battery_ma=int(g[7]),
             soc_pct=int(g[8]), supply_v=sv, supply_ma=sma,
             supply_good=bool(int(g[18])), supply_w=round(sv * sma / 1000.0, 3))
    if g[19] is not None:  # env tail: lux (number|sat|nan), panel/battery temps
        s.update(lux=_numok(g[19]), lux_sat=(g[19] == "sat"),
                 ch1=int(g[21]),  # TSL2591 IR channel: unsaturated fallback in full sun
                 panel_c=_numok(g[22]), batt_c=_numok(g[24]))
    if g[25] is not None:  # onboard INA tail: ground-truth panel power; -32768 = absent
        pv, pa = int(g[25]), int(g[26])
        if pv != -32768 and pa != -32768:
            s["ina_panel_w"] = round(pv * pa / 1e6, 3)
        bv2, ba = int(g[27]), int(g[28])
        if bv2 != -32768 and ba != -32768:
            s["ina_batt_ma"] = ba
    return s


def set_maintain(v10):
    """Send m<v10>; SET_MAINTAIN is unacked broadcast -> re-send 3x across the settle."""
    echoed = False
    for i in range(3):
        ser.reset_input_buffer()
        ser.write(f"m{v10}\n".encode())
        for line in read_lines(min(2.0, a.settle_s / 3)):
            if RX_MAINT_ECHO.search(line):
                echoed = True
    if not echoed:
        print(f"  WARNING: no SET_MAINTAIN echo from master for {v10} (sent blind)")
    # remainder of the settle window
    for _ in read_lines(max(0.0, a.settle_s - 6.0)):
        pass


def prompt_temp(label):
    if a.no_prompt:
        return None
    raw = input(f"  IR {label} temp (e.g. 142F or 61C, blank=skip): ").strip()
    if not raw:
        return None
    try:
        val = float(raw[:-1]) if raw[-1] in "FfCc" else float(raw)
        if raw[-1] in "Ff":
            val = (val - 32.0) * 5.0 / 9.0
        return round(val, 1)
    except ValueError:
        print("  (unparsed, skipped)")
        return None


def pctile(xs, p):
    xs = sorted(xs)
    return xs[min(len(xs) - 1, max(0, int(round(p * (len(xs) - 1)))))]


def dwell(v10, visit_idx, is_anchor, fh):
    """Collect one setpoint visit; returns the summary row."""
    samples, pars = [], []
    last_par_t = 0.0
    end = time.time() + a.dwell_s
    while time.time() < end:
        for line in read_lines(0.5):
            s = parse_peer(line)
            if not s:
                continue
            if par_ser and time.time() - last_par_t >= 1.0:  # optional host-side cross-check
                try:
                    s["par"] = round(read_par(par_ser), 1)
                    pars.append(s["par"])
                except IOError:
                    s["par"] = None
                last_par_t = time.time()
            row = dict(META, src="mpp-sample", ts_utc=datetime.now(timezone.utc).isoformat(),
                       visit=visit_idx, maintain_v=v10 / 10.0, is_anchor=is_anchor, **s)
            fh.write(json.dumps(row) + "\n")
            samples.append(s)
        fh.flush()
    if not samples:
        print("  WARNING: no heartbeats received during dwell (peer out of range / asleep?)")
        return dict(META, src="mpp-point", ts_utc=datetime.now(timezone.utc).isoformat(),
                    visit=visit_idx, maintain_v=v10 / 10.0, is_anchor=is_anchor, n=0, flags=["no-data"])
    ws = [s["supply_w"] for s in samples]
    med_w = statistics.median(ws)
    flags = []
    if not all(s["supply_good"] for s in samples):
        flags.append("supply-not-good")
    if med_w < 0.02:
        flags.append("dark-panel")  # the reseated-connector gotcha
    # light: peer-side TSL2591 lux (over the air) preferred; Apogee PAR fallback
    luxes = [s["lux"] for s in samples if s.get("lux") is not None]
    ch1s = [s["ch1"] for s in samples if s.get("ch1")]
    n_sat = sum(1 for s in samples if s.get("lux_sat"))
    if n_sat > len(samples) * 0.2:
        flags.append(f"light-saturated({n_sat}/{len(samples)})")
    # light source priority: unsaturated lux -> IR ch1 (full-sun fallback) -> host PAR
    if luxes and n_sat <= len(samples) * 0.2:
        lights, light_src = luxes, "lux"
    elif ch1s:
        lights, light_src = ch1s, "ir-ch1"
    elif pars:
        lights, light_src = pars, "par"
    else:
        lights, light_src = [], None
    light_med = statistics.median(lights) if lights else None
    light_cv = None
    if lights and light_med:
        light_cv = (pctile(lights, 0.95) - pctile(lights, 0.05)) / light_med
        if light_cv > a.light_cv_flag:
            flags.append(f"light-unstable({light_cv:.0%})")
    elif not lights:
        flags.append("no-light")
    ptcs = [s["panel_c"] for s in samples if s.get("panel_c") is not None]
    btcs = [s["batt_c"] for s in samples if s.get("batt_c") is not None]
    point = dict(META, src="mpp-point", ts_utc=datetime.now(timezone.utc).isoformat(),
                 visit=visit_idx, maintain_v=v10 / 10.0, is_anchor=is_anchor, n=len(samples),
                 supply_w_med=round(med_w, 3), supply_w_mean=round(statistics.fmean(ws), 3),
                 supply_w_p5=round(pctile(ws, 0.05), 3), supply_w_p95=round(pctile(ws, 0.95), 3),
                 supply_v_med=round(statistics.median(s["supply_v"] for s in samples), 3),
                 supply_ma_med=int(statistics.median(s["supply_ma"] for s in samples)),
                 battery_v_med=round(statistics.median(s["battery_v"] for s in samples), 3),
                 battery_ma_med=int(statistics.median(s["battery_ma"] for s in samples)),
                 soc_pct=samples[-1]["soc_pct"],
                 light_med=round(light_med, 1) if light_med is not None else None,
                 light_cv=round(light_cv, 4) if light_cv is not None else None,
                 light_src=light_src,
                 par_med=(statistics.median(pars) if pars else None),
                 supply_w_ina_med=(round(statistics.median(
                     [s["ina_panel_w"] for s in samples if s.get("ina_panel_w") is not None]), 3)
                     if any(s.get("ina_panel_w") is not None for s in samples) else None),
                 panel_c_med=(round(statistics.median(ptcs), 1) if ptcs else None),
                 batt_c_med=(round(statistics.median(btcs), 1) if btcs else None),
                 flags=flags)
    return point


def build_visits():
    pts = [int(p) for p in a.points.split(",") if p.strip()]
    swept = [p for p in pts if p != a.anchor]
    visits = [(a.anchor, True)]
    for i, p in enumerate(swept):
        visits.append((p, False))
        if (i + 1) % a.anchor_every == 0 and i != len(swept) - 1:
            visits.append((a.anchor, True))
    visits.append((a.anchor, True))
    return visits


visits = build_visits()
bad = [v for v, _ in visits if not 40 <= v <= 58]
if bad:
    sys.exit(f"setpoints out of the peer's 40..58 accept window: {bad}")
est = len(visits) * (a.settle_s + a.dwell_s) / 60.0
print(f"mpp_sweep -> {out}")
print(f"visits: {' '.join(f'{v/10:.1f}' + ('*' if anc else '') for v, anc in visits)}  (*=anchor)")
print(f"~{est:.0f} min of dwell+settle, plus prompts. Ctrl-C exits cleanly (restores "
      f"{a.restore_v10/10:.1f} V).\n")

points = []
try:
    with open(out, "w") as fh:
        for idx, (v10, is_anchor) in enumerate(visits):
            while True:
                print(f"[{idx+1}/{len(visits)}] VINDPM {v10/10:.1f} V"
                      f"{' (anchor)' if is_anchor else ''}: settle {a.settle_s:.0f}s + "
                      f"dwell {a.dwell_s:.0f}s ...")
                set_maintain(v10)
                p = dwell(v10, idx, is_anchor, fh)
                p["ir_panel_c"] = prompt_temp("panel")
                if idx % a.batt_temp_every == 0:
                    p["ir_batt_c"] = prompt_temp("battery/board")
                fh.write(json.dumps(p) + "\n")
                fh.flush()
                norm = (f"  W/klight={1000*p['supply_w_med']/p['light_med']:.3f}"
                        if p.get("light_med") else "")
                ptc = f"  panel={p['panel_c_med']}C" if p.get("panel_c_med") is not None else ""
                print(f"  -> n={p['n']} supply_w med={p.get('supply_w_med')} "
                      f"light={p.get('light_med')}({p.get('light_src')}){norm}{ptc} "
                      f"flags={p['flags'] or 'none'}")
                points.append(p)
                if p["flags"] and not a.no_prompt:
                    if input("  point flagged - redo it? [y/N]: ").strip().lower() == "y":
                        points.pop()
                        continue
                break
except KeyboardInterrupt:
    print("\n(interrupted - writing what we have)")
finally:
    try:
        ser.write(f"m{a.restore_v10}\n".encode())
        time.sleep(0.3)
        ser.write(f"m{a.restore_v10}\n".encode())
    except serial.SerialException:
        pass
    ser.close()
    if par_ser:
        par_ser.close()

anchors = [p for p in points if p["is_anchor"] and p.get("n")]
if len(anchors) >= 2:
    vals = [(1000 * p["supply_w_med"] / p["light_med"]) if p.get("light_med") else p["supply_w_med"]
            for p in anchors]
    unit = "W/klight" if anchors[0].get("light_med") else "W (no light channel!)"
    drift = (max(vals) - min(vals)) / statistics.median(vals)
    print(f"\nanchor {a.anchor/10:.1f}V across session ({unit}): "
          + " ".join(f"{v:.3f}" for v in vals) + f"  drift={drift:.0%}")
    if drift > a.anchor_drift_flag:
        print(f"WARNING: anchor drift > {a.anchor_drift_flag:.0%} - conditions moved; "
              "treat absolute watts with suspicion (light-normalized curve is the robust output).")
print(f"\nDONE {len(points)} points -> {out}\nNext: ./mpp_analyze.py {out}")
