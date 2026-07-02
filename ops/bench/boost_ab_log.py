#!/usr/bin/env python3
"""Boost A/B bench logger (TPS63802 4.2 V experiment).

Merges the KB2040 ina_monitor serial stream ('ina' + 'lux' lines) with the
led_studio /state JSON (look settings + fuel gauge) into one labeled JSONL
file, then prints a summary. One file per labeled run so bare vs boosted
captures diff cleanly.

Channel map for this harness (LOG 2026-07-02, cross-checked vs fuel gauge
and the 2026-06-11 single-px number):
  0x41 = LED power out (bare hex V+ or boost-module input when boosted)
  0x45 = battery (charge-positive: discharge reads NEGATIVE)

Examples:
  ./boost_ab_log.py --label bare-center-rgbwhite-full
  ./boost_ab_log.py --label boosted-center-rgbwhite-full --duration 120

Requires pyserial. Data -> ops/bench/data/boost_ab/<date>_<label>.jsonl
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


def fetch_state(url):
    try:
        with urllib.request.urlopen(url + "/state", timeout=3) as r:
            return json.loads(r.read().decode())
    except Exception as e:
        return {"error": str(e)}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--label", required=True,
                    help="run label, e.g. bare-center-rgbwhite-full")
    ap.add_argument("--port", default="/dev/ttyACM2", help="ina_monitor serial port")
    ap.add_argument("--studio", default="http://ledstudio.local",
                    help="led_studio base URL")
    ap.add_argument("--duration", type=float, default=60, help="seconds to capture")
    ap.add_argument("--state-every", type=float, default=5,
                    help="seconds between /state polls")
    ap.add_argument("--out-dir", default=None,
                    help="output dir (default: ops/bench/data/boost_ab next to this script)")
    ap.add_argument("--notes", default="", help="free-form run notes")
    args = ap.parse_args()

    out_dir = pathlib.Path(args.out_dir) if args.out_dir else \
        pathlib.Path(__file__).resolve().parent / "data" / "boost_ab"
    out_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    out_path = out_dir / f"{stamp}_{args.label}.jsonl"

    # Poke a re-render before capturing: led_studio only pushes static frames on an
    # actual VALUE CHANGE, so a hex hot-swapped after the last render sits BLANK until
    # something changes (bit us twice 2026-07-02 -- re-sending identical values does
    # NOT redraw). Wiggle brightness by 1 count and back to force two real renders.
    st = fetch_state(args.studio)
    if "bri" in st:
        bri = int(st["bri"])
        wiggle = bri - 1 if bri > 0 else 1
        try:
            urllib.request.urlopen(f"{args.studio}/set?bri={wiggle}", timeout=5).read()
            time.sleep(0.4)
            urllib.request.urlopen(f"{args.studio}/set?bri={bri}", timeout=5).read()
            time.sleep(1.0)
        except Exception as e:
            print(f"WARNING: render poke failed: {e}", file=sys.stderr)
    else:
        print(f"WARNING: no /state before run: {st.get('error')}", file=sys.stderr)

    ser = serial.Serial(args.port, 115200, timeout=0.3)
    time.sleep(0.3)
    ser.reset_input_buffer()

    ina = {}  # ch -> list of (bus_v, ma)
    lux = {}  # sensor -> list of (lux, sat)
    n_lines = 0
    t0 = time.time()
    next_state = t0
    with out_path.open("w") as f:
        f.write(json.dumps({"type": "meta", "label": args.label, "start": stamp,
                            "port": args.port, "studio": args.studio,
                            "notes": args.notes,
                            "channel_map": {"0x41": "led_out", "0x45": "battery"}}) + "\n")
        while time.time() - t0 < args.duration:
            now = time.time()
            if now >= next_state:
                next_state = now + args.state_every
                st = fetch_state(args.studio)
                st.update({"type": "state", "t_host": now - t0})
                f.write(json.dumps(st) + "\n")
            line = ser.readline().decode(errors="replace").strip()
            if not line:
                continue
            m = INA_RE.match(line)
            if m:
                row = {"type": "ina", "t_host": now - t0, "t_ms": int(m.group(1)),
                       "ch": m.group(2), "bus_v": float(m.group(3)),
                       "shunt_mv": float(m.group(4)), "ma": float(m.group(5))}
                f.write(json.dumps(row) + "\n")
                ina.setdefault(row["ch"], []).append((row["bus_v"], row["ma"]))
                n_lines += 1
                continue
            m = LUX_RE.match(line)
            if m:
                row = {"type": "lux", "t_host": now - t0, "t_ms": int(m.group(1)),
                       "sensor": m.group(2), "lux": float(m.group(6)),
                       "sat": int(m.group(7))}
                if m.group(3) is not None:
                    row["raw"] = int(m.group(3))
                else:
                    row["ch0"], row["ch1"] = int(m.group(4)), int(m.group(5))
                f.write(json.dumps(row) + "\n")
                lux.setdefault(row["sensor"], []).append((row["lux"], row["sat"]))
                n_lines += 1
    ser.close()

    print(f"wrote {out_path} ({n_lines} sensor lines)")
    summary = {"type": "summary", "label": args.label}
    for ch, vals in sorted(ina.items()):
        vs, mas = [v for v, _ in vals], [a for _, a in vals]
        summary[ch] = {"n": len(vals),
                       "bus_v": round(statistics.mean(vs), 4),
                       "ma": round(statistics.mean(mas), 2),
                       "ma_sd": round(statistics.stdev(mas), 2) if len(mas) > 1 else 0.0,
                       "w": round(statistics.mean(v * a for v, a in vals) / 1000, 4)}
        print(f"  {ch}: n={len(vals)} bus_v={summary[ch]['bus_v']} "
              f"ma={summary[ch]['ma']} (sd {summary[ch]['ma_sd']}) W={summary[ch]['w']}")
    for sname, vals in sorted(lux.items()):
        lxs, sats = [l for l, _ in vals], sum(s for _, s in vals)
        summary[sname] = {"n": len(vals), "lux": round(statistics.mean(lxs), 2),
                          "lux_sd": round(statistics.stdev(lxs), 3) if len(lxs) > 1 else 0.0,
                          "sat_count": sats}
        print(f"  {sname}: n={len(vals)} lux={summary[sname]['lux']} "
              f"(sd {summary[sname]['lux_sd']}) sat_count={sats}")
        if sats:
            print("  WARNING: saturation hit -- move the sensor back and re-run", file=sys.stderr)
    with out_path.open("a") as f:
        f.write(json.dumps(summary) + "\n")


if __name__ == "__main__":
    main()
