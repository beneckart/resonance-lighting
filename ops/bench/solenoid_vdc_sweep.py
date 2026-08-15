#!/usr/bin/env python3
"""Solenoid VDC-tap strike sweep: strike energy (MSA311 impact peak) vs pulse
width vs supply voltage. Unattended scoring -- no ears required.

Setup: solenoid driver V+/GND Y-tapped off the VDC/GND JST-XH port, bench PSU
(or panel) on VDC, gate on D12, MSA311 strapped RIGIDLY to the strike surface
and daisy-chained on STEMMA-QT. Firmware led_sol_bench >= 2026-07-11.8
(/probe_strike returns peak_mg = peak |accel - baseline| through the pulse
window, in milli-g).

Scoring: a strike "hits" when peak_mg >= --hit-mg (default 300; check the
printed peaks on your first pass and adjust -- a clean hit vs a dud should
differ by an order of magnitude). Loudness proxy = median peak_mg of the hits.

Usage: python3 solenoid_vdc_sweep.py [--hit-mg 300]
       set the PSU, enter its voltage at the prompt; everything else is
       automatic. 'q' at the prompt to finish.
"""
import argparse
import csv
import datetime
import json
import statistics
import time
import urllib.request

HOST_DEFAULT = "ledsol.local"
WIDTHS = [5, 6, 8, 10, 12, 15, 20, 25, 30, 40]
N = 5
GAP_S = 0.6


def api(host, path, tries=4):
    for attempt in range(tries):
        try:
            with urllib.request.urlopen(f"http://{host}{path}", timeout=10) as r:
                return r.read().decode()
        except OSError:
            if attempt == tries - 1:
                raise
            time.sleep(1.5 * (attempt + 1))


def resolve_once(host):
    import socket
    try:
        return socket.getaddrinfo(host, 80)[0][4][0]
    except OSError:
        return host


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=HOST_DEFAULT)
    ap.add_argument("--hit-mg", type=int, default=300, help="peak_mg threshold for a hit")
    args = ap.parse_args()
    host = resolve_once(args.host)

    s = json.loads(api(host, "/state"))
    print(f"fw={s['fw']} supply={s['sv']:.2f}V ({int(s['sma'])}mA) "
          f"bat={s['bv']:.3f}V soc={s['soc']}%")
    probe = json.loads(api(host, "/probe_strike?ms=5"))  # also confirms MSA presence
    if not probe.get("msa"):
        raise SystemExit("MSA311 not detected on the SQT chain -- mount it and re-run")
    api(host, "/set?arm=1&flash=0")

    out = f"{__file__.rsplit('/', 1)[0]}/data/solenoid-vdc-sweep-{datetime.date.today().isoformat()}.csv"
    fields = ["ts", "psu_v", "width_ms", "hits", "n", "peak_mg_med", "peak_mg_min",
              "peak_mg_max", "sv_pre_med", "sv_150_med", "sma_pre_med", "good_flap", "note"]
    try:
        write_header = not open(out).readline()
    except FileNotFoundError:
        write_header = True
    fh = open(out, "a", newline="")
    w = csv.writer(fh)
    if write_header:
        w.writerow(fields)

    while True:
        psu = input("\nPSU voltage now on VDC (e.g. 5.0), or q to quit: ").strip()
        if psu.lower() == "q":
            break
        min_reliable, prev_full, ms_prev = None, False, None
        for ms in WIDTHS:
            peaks, pre, r150, ima = [], [], [], []
            flaps = 0
            for _ in range(N):
                j = json.loads(api(host, f"/probe_strike?ms={ms}"))
                if not j["ok"]:
                    time.sleep(0.5)
                    j = json.loads(api(host, f"/probe_strike?ms={ms}"))
                    if not j["ok"]:
                        continue
                peaks.append(j["peak_mg"])
                pre.append(j["sv_pre"]); r150.append(j["sv_150"]); ima.append(j["sma_pre"])
                flaps += int(j["good_pre"] != j["good_post"])
                time.sleep(GAP_S)
            hits = sum(1 for p in peaks if p >= args.hit_mg)
            med = lambda v: statistics.median(v) if v else 0
            print(f"  {ms:>3} ms: hits {hits}/{len(peaks)}  peak_mg {sorted(peaks)}  "
                  f"sv {med(pre):.2f}->{med(r150):.2f}V"
                  f"{'  GOOD-FLAP x' + str(flaps) if flaps else ''}")
            w.writerow([datetime.datetime.now().isoformat(timespec="seconds"), psu, ms,
                        hits, len(peaks), int(med(peaks)), min(peaks) if peaks else 0,
                        max(peaks) if peaks else 0, f"{med(pre):.3f}", f"{med(r150):.3f}",
                        f"{med(ima):.0f}", flaps, ""])
            fh.flush()
            if hits == len(peaks) == N:
                if prev_full:
                    min_reliable = ms_prev
                    print(f"  >> min reliable width at {psu} V: {min_reliable} ms "
                          f"(two consecutive {N}/{N} widths)")
                    break
                prev_full, ms_prev = True, ms
            else:
                prev_full = False
        # burst stress: 20 strikes at 1.5x min reliable (or 60 ms fallback)
        bw = int((min_reliable or 40) * 1.5)
        g0 = json.loads(api(host, "/state"))
        api(host, f"/set?pulse={bw}")
        api(host, "/burst?n=20")
        time.sleep(20 * 0.35 + 3)
        g1 = json.loads(api(host, "/state"))
        print(f"  burst20 @ {bw} ms: strikes {g0['strikes']}->{g1['strikes']}, "
              f"failsafes {g1['failsafes']}, supply {g1['sv']:.2f}V good={g1['sgood']}")
        w.writerow([datetime.datetime.now().isoformat(timespec="seconds"), psu, bw,
                    "", 20, "", "", "", "", "", "", "", "burst20"])
        fh.flush()
    fh.close()
    api(host, "/set?arm=0")
    print(f"\nrows in {out}; solenoid disarmed.")


if __name__ == "__main__":
    main()
