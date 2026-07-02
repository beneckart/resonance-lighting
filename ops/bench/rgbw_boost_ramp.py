#!/usr/bin/env python3
"""RGBW boost A/B brightness ramp with live rail-droop aborts.

Steps the single 4 W SK6812 RGBW point source (led_studio mode=1) through a
brightness ladder per look, logging the KB2040 ina/lux stream per step. This is
both the RGBW boost A/B capture AND the 3V3-rail capability characterization
(step 0): full RGB-white projects past the ~1 A rail ceiling, so the ladder
watches the LED-branch bus voltage and stops before browning out the board.

Channel map (LOG 2026-07-02): 0x41 = LED branch (boost input when boosted),
0x45 = battery (charge-positive; discharge reads negative).

Abort logic:
  - HARD: any 0x41 bus_v sample < --hard-floor (default 2.60 V) -> LEDs off, stop.
  - SOFT: step median 0x41 bus_v < --soft-floor (default 2.80 V) -> finish look,
    skip higher brightness steps.
  - /state unreachable after a step (possible ESP reset) -> stop.
  - lux sat=1 -> keep going (electrical data still valid), flag loudly.

Examples:
  ./rgbw_boost_ramp.py --config boosted --runtag r1
  ./rgbw_boost_ramp.py --config bare --runtag r2

One JSONL per invocation in ops/bench/data/boost_ab/, rows tagged look/bri.
"""

import argparse
import datetime
import json
import pathlib
import re
import statistics
import sys
import time
import urllib.request

import serial

INA_RE = re.compile(
    r"ina t=(\d+) ch=(0x[0-9A-Fa-f]+) bus_v=([-\d.]+) shunt_mv=([-\d.]+) ma=([-\d.]+)")
LUX_RE = re.compile(
    r"lux t=(\d+) sensor=(\w+) (?:raw=(\d+)|ch0=(\d+) ch1=(\d+)) lux=([-\d.]+) sat=(\d)")

LOOKS = {
    "wonly": "r=0&g=0&b=0&w=255",
    "rgbwhite": "r=255&g=255&b=255&w=0",
    "red": "r=255&g=0&b=0&w=0",
    "green": "r=0&g=255&b=0&w=0",
    "blue": "r=0&g=0&b=255&w=0",
}


def set_url(studio, q, tries=3):
    # Retry transient mDNS/WiFi hiccups -- a failed set mid-ladder is worth 2 s of
    # patience before giving up (the exception path blanks the LEDs and aborts).
    for i in range(tries):
        try:
            urllib.request.urlopen(f"{studio}/set?{q}", timeout=6).read()
            return
        except Exception:
            if i == tries - 1:
                raise
            time.sleep(1.0)


def get_state(studio):
    try:
        with urllib.request.urlopen(f"{studio}/state", timeout=4) as r:
            return json.loads(r.read().decode())
    except Exception as e:
        return {"error": str(e)}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--config", required=True, help="bare | boosted (label only)")
    ap.add_argument("--runtag", required=True)
    ap.add_argument("--looks", default="wonly,rgbwhite",
                    help=f"comma list from {sorted(LOOKS)}")
    ap.add_argument("--ladder", default="32,64,128,192,255")
    ap.add_argument("--step-duration", type=float, default=15)
    ap.add_argument("--port", default="/dev/ttyACM2")
    ap.add_argument("--studio", default="http://ledstudio.local")
    ap.add_argument("--led-ch", default="0x41")
    ap.add_argument("--hard-floor", type=float, default=2.60)
    ap.add_argument("--soft-floor", type=float, default=2.80)
    # No-INA / production-similar runs: with the branch fed from the VBAT header,
    # BOTH the 0x45 INA and the fuel gauge's current shunt are bypassed (r7 finding),
    # so an uninstrumented run has NO current numbers -- lux decides the outcome and
    # the gauge's terminal VOLTAGE (bv, which does see cell sag) is the safety abort.
    ap.add_argument("--bv-floor", type=float, default=3.00,
                    help="stop the ladder if /state bv drops below this (per-step)")
    ap.add_argument("--notes", default="")
    args = ap.parse_args()

    looks = [l.strip() for l in args.looks.split(",") if l.strip()]
    for l in looks:
        if l not in LOOKS:
            sys.exit(f"unknown look {l!r}; choose from {sorted(LOOKS)}")
    ladder = [int(b) for b in args.ladder.split(",")]

    out_dir = pathlib.Path(__file__).resolve().parent / "data" / "boost_ab"
    out_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    out_path = out_dir / f"{stamp}_rgbw-ramp-{args.config}-{args.runtag}.jsonl"

    ser = serial.Serial(args.port, 115200, timeout=0.3)
    time.sleep(0.3)

    # Enter RGBW mode dark, then wiggle bri to force renders (led_studio only
    # redraws on change).
    set_url(args.studio, "mode=1&r=0&g=0&b=0&w=0&bri=0")
    time.sleep(0.5)
    set_url(args.studio, "bri=1")
    time.sleep(0.3)
    set_url(args.studio, "bri=0")
    time.sleep(0.5)

    results = []
    hard_abort = False
    f = out_path.open("w")
    f.write(json.dumps({"type": "meta", "config": args.config, "runtag": args.runtag,
                        "looks": looks, "ladder": ladder, "start": stamp,
                        "notes": args.notes,
                        "channel_map": {"0x41": "led_branch", "0x45": "battery"}}) + "\n")
    print(f"{'look':>9} {'bri':>4} {'led_V':>6} {'led_mA':>7} {'led_W':>6} "
          f"{'batt_mA':>8} {'lux':>8} {'sat':>3}")
    try:
        for look in looks:
            soft_stop = False
            for bri in ladder:
                if soft_stop or hard_abort:
                    break
                set_url(args.studio, f"{LOOKS[look]}&bri={bri}")
                time.sleep(1.5)
                ser.reset_input_buffer()
                ina = {}
                lux_vals, sat_n = [], 0
                t0 = time.time()
                while time.time() - t0 < args.step_duration:
                    line = ser.readline().decode(errors="replace").strip()
                    if not line:
                        continue
                    m = INA_RE.match(line)
                    if m:
                        row = {"type": "ina", "look": look, "bri": bri,
                               "ch": m.group(2), "bus_v": float(m.group(3)),
                               "ma": float(m.group(5))}
                        f.write(json.dumps(row) + "\n")
                        ina.setdefault(row["ch"], []).append((row["bus_v"], row["ma"]))
                        if row["ch"] == args.led_ch and row["bus_v"] < args.hard_floor:
                            print(f"HARD ABORT: {args.led_ch} bus_v={row['bus_v']:.3f} "
                                  f"< {args.hard_floor}", file=sys.stderr)
                            hard_abort = True
                            break
                        continue
                    m = LUX_RE.match(line)
                    if m:
                        row = {"type": "lux", "look": look, "bri": bri,
                               "sensor": m.group(2), "lux": float(m.group(6)),
                               "sat": int(m.group(7))}
                        f.write(json.dumps(row) + "\n")
                        lux_vals.append(row["lux"])
                        sat_n += row["sat"]
                led = ina.get(args.led_ch, [])
                led_v = statistics.median(v for v, _ in led) if led else float("nan")
                led_ma = statistics.median(a for _, a in led) if led else float("nan")
                # Stability flag: the 3V3 rail regulator burst-modes at light-mid
                # loads and the INA's 68 ms window aliases it into slow wander --
                # current medians there are unreliable (lux stays good). Full-bri
                # steps run continuous-PWM and are stable. (LOG 2026-07-02.)
                unstable = False
                if len(led) >= 30:
                    mas = [a for _, a in led]
                    fifth = max(1, len(mas) // 5)
                    drift = statistics.median(mas[-fifth:]) - statistics.median(mas[:fifth])
                    sd = statistics.stdev(mas)
                    unstable = abs(drift) > 0.15 * max(abs(led_ma), 1) or \
                        sd > 0.2 * max(abs(led_ma), 1)
                batt = ina.get("0x45", [])
                batt_ma = statistics.median(a for _, a in batt) if batt else float("nan")
                lux = statistics.median(lux_vals) if lux_vals else float("nan")
                summary = {"type": "step_summary", "look": look, "bri": bri,
                           "led_bus_v": round(led_v, 3), "led_ma": round(led_ma, 1),
                           "led_w": round(led_v * led_ma / 1000, 3),
                           "batt_ma": round(batt_ma, 1), "lux": round(lux, 1),
                           "sat_n": sat_n, "n": len(led), "ma_unstable": unstable}
                f.write(json.dumps(summary) + "\n")
                results.append(summary)
                print(f"{look:>9} {bri:>4} {led_v:6.3f} {led_ma:7.1f} "
                      f"{led_v * led_ma / 1000:6.3f} {batt_ma:8.1f} {lux:8.1f} "
                      f"{'YES' if sat_n else '':>3}"
                      f"{'  ~mA UNSTABLE (rail burst-mode; trust lux)' if unstable else ''}")
                if sat_n:
                    print(f"  NOTE: lux saturated on {sat_n} samples -- optical data "
                          f"invalid at this step, electrical still good", file=sys.stderr)
                if hard_abort:
                    break
                if led and led_v < args.soft_floor:
                    print(f"  SOFT STOP: median {args.led_ch} bus_v {led_v:.3f} < "
                          f"{args.soft_floor}; skipping higher steps for {look}",
                          file=sys.stderr)
                    soft_stop = True
                st = get_state(args.studio)
                if "error" in st:
                    print(f"ABORT: /state unreachable after step ({st['error']}) -- "
                          f"possible board reset", file=sys.stderr)
                    hard_abort = True
                else:
                    st.update({"type": "state", "look": look, "bri": bri})
                    f.write(json.dumps(st) + "\n")
                    if st.get("bv", 99) < args.bv_floor:
                        print(f"ABORT: gauge bv {st['bv']:.3f} < {args.bv_floor} "
                              f"(cell sag floor)", file=sys.stderr)
                        hard_abort = True
    finally:
        try:
            set_url(args.studio, "r=0&g=0&b=0&w=0&bri=0")
        except Exception as e:
            print(f"WARNING: could not blank LEDs at exit: {e}", file=sys.stderr)
        f.close()
        ser.close()
    print(f"\nwrote {out_path}")
    if hard_abort:
        sys.exit(2)


if __name__ == "__main__":
    main()
