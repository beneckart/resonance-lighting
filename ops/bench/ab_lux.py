#!/usr/bin/env python3
"""A/B lux comparison: 3V3-rail-fed vs VBAT-fed 4 W RGBW, one module per header.

Drives the led_sol_bench firmware (>= 2026-07-11.6): feed A = 3V3 rail + A0,
feed B = VBAT + D13/GPIO11, VEML7700 on the STEMMA-QT port (gain 1/8, IT 100 ms).
Per look, feeds are measured in ABBA order to cancel linear drift (battery
droop + LED die heating). A dark baseline is read before every look; >2 lx
means ambient contamination and the row is flagged.

Usage:
  python3 ab_lux.py --run 1                 # first VEML position
  <reposition the VEML slightly>
  python3 ab_lux.py --run 2                 # second position, appends same CSV

Rows append to ops/bench/data/ab-lux-<date>.csv; re-running the same --run id
just adds more rows (they are timestamped -- analysis dedups by run+look+rep).
"""
import argparse
import csv
import datetime
import json
import statistics
import sys
import time
import urllib.request

LOOKS = [
    ("w_only", dict(r=0, g=0, b=0, w=255)),
    ("red", dict(r=255, g=0, b=0, w=0)),
    ("green", dict(r=0, g=255, b=0, w=0)),
    ("blue", dict(r=0, g=0, b=255, w=0)),
    ("rgb_white", dict(r=255, g=255, b=255, w=0)),
]
FEED_NAME = {0: "A_rail", 1: "B_vbat"}


def api(host, path, tries=4):
    # mDNS can blip mid-run (seen 2026-07-11): retry transient failures with backoff.
    for attempt in range(tries):
        try:
            with urllib.request.urlopen(f"http://{host}{path}", timeout=8) as r:
                return r.read().decode()
        except (urllib.error.URLError, OSError):
            if attempt == tries - 1:
                raise
            time.sleep(1.5 * (attempt + 1))


def resolve_once(host):
    """Pin .local names to an IP up front so avahi blips can't kill a run."""
    import socket
    try:
        return socket.getaddrinfo(host, 80)[0][4][0]
    except OSError:
        return host  # fall back to the name; api() retries will cope


def set_params(host, **kw):
    q = "&".join(f"{k}={v}" for k, v in kw.items())
    api(host, f"/set?{q}")


def read_lux(host, samples, spacing):
    vals, raws, sat = [], [], 0
    for _ in range(samples):
        j = json.loads(api(host, "/lux"))
        if not j["veml"]:
            sys.exit("VEML7700 not responding -- check SQT cable / VSQT rail")
        vals.append(j["lux"])
        raws.append(j["raw"])
        sat |= j["sat"]
        time.sleep(spacing)
    return vals, raws, sat


def state(host):
    return json.loads(api(host, "/state"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="ledsol.local")
    ap.add_argument("--run", required=True, help="run id; bump after each VEML reposition")
    ap.add_argument("--out", default=None, help="CSV path (default ops/bench/data/ab-lux-<date>.csv)")
    ap.add_argument("--samples", type=int, default=5, help="lux samples per measurement")
    ap.add_argument("--spacing", type=float, default=0.15, help="s between lux samples")
    ap.add_argument("--settle", type=float, default=1.5, help="s after lighting a look (LED + 100ms IT)")
    ap.add_argument("--cool", type=float, default=1.0, help="s dark gap between measurements")
    args = ap.parse_args()

    today = datetime.date.today().isoformat()
    out = args.out or f"{__file__.rsplit('/', 1)[0]}/data/ab-lux-{today}.csv"

    args.host = resolve_once(args.host)
    s = state(args.host)
    print(f"fw={s['fw']} bat={s['bv']:.3f}V soc={s['soc']}% supply={'ok' if s['sgood'] else 'BATTERY ONLY'}")
    if not s["sgood"]:
        print("note: on battery -- VBAT side will droop as SOC falls; ABBA ordering compensates linearly")

    set_params(args.host, bri=255, gamma=0, anim=0, flash=0, arm=0)

    fields = ["ts", "run", "look", "feed", "rep", "lux_med", "lux_min", "lux_max",
              "raw_med", "sat", "dark_lux", "bv", "sv", "sma", "soc"]
    try:
        write_header = not open(out).readline()
    except FileNotFoundError:
        write_header = True
    fh = open(out, "a", newline="")
    w = csv.writer(fh)
    if write_header:
        w.writerow(fields)

    results = {}  # (look, feed) -> [medians]
    for look, ch in LOOKS:
        set_params(args.host, r=0, g=0, b=0, w=0)  # dark for baseline
        time.sleep(max(args.cool, 0.5))
        dark_vals, _, _ = read_lux(args.host, 3, 0.12)
        dark = statistics.median(dark_vals)
        flag = "  AMBIENT?" if dark > 2.0 else ""
        print(f"\n[{look}] dark baseline {dark:.1f} lx{flag}")

        for rep, feed in enumerate([0, 1, 1, 0]):  # ABBA
            set_params(args.host, feed=feed)
            time.sleep(0.5)  # pin/rail switch settle
            set_params(args.host, **ch)
            time.sleep(args.settle)
            vals, raws, sat = read_lux(args.host, args.samples, args.spacing)
            set_params(args.host, r=0, g=0, b=0, w=0)  # dark + cool between meas
            med = statistics.median(vals)
            st = state(args.host)
            w.writerow([datetime.datetime.now().isoformat(timespec="seconds"), args.run,
                        look, FEED_NAME[feed], rep, f"{med:.1f}", f"{min(vals):.1f}",
                        f"{max(vals):.1f}", int(statistics.median(raws)), sat,
                        f"{dark:.1f}", f"{st['bv']:.3f}", f"{st['sv']:.2f}",
                        int(st["sma"]), st["soc"]])
            fh.flush()
            results.setdefault((look, feed), []).append(med)
            print(f"  {FEED_NAME[feed]:7s} rep{rep}: {med:8.1f} lx  "
                  f"(min {min(vals):.1f} / max {max(vals):.1f}){'  SATURATED' if sat else ''}")
            time.sleep(args.cool)

    set_params(args.host, r=0, g=0, b=0, w=0, bri=0)
    fh.close()

    print(f"\n=== run {args.run} summary (medians of ABBA pairs) ===")
    print(f"{'look':<10} {'A rail':>9} {'B vbat':>9} {'B/A':>6}")
    for look, _ in LOOKS:
        a = statistics.median(results[(look, 0)])
        b = statistics.median(results[(look, 1)])
        ratio = b / a if a > 0.5 else float("nan")
        print(f"{look:<10} {a:>9.1f} {b:>9.1f} {ratio:>6.2f}")
    print(f"\nrows appended to {out}")
    print("reposition the VEML and re-run with --run", int(args.run) + 1 if args.run.isdigit() else "<next>")


if __name__ == "__main__":
    main()
