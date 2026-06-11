#!/usr/bin/env python3
"""Buck-boost (TPS631013) efficiency-vs-VBAT from a fixed-load discharge JSONL.

Input: an afk_discharge.py run (fixed LED command, full->empty) with both INA
channels logged -- e.g. data/ca/2026-06-10-discharge-1357.jsonl. At a fixed
load the converter input power vs VBAT maps the converter's behavior across
the LFP plateau, the thing TODO's "buck-boost efficiency vs VBAT" test asks
for; in particular whether the buck<->boost crossover (~VBAT 3.25-3.35 V,
where it 4-switch/mode-hunts) shows up as an efficiency dip.

What it computes per row, then medians into VBAT bins (pre-brownout only):
  p_batt    = batt_ina_bus_v * |batt_ina_ma|     (converter input, ground truth)
  p_led     = led_bus_v * led_ma                 (LED slice of the 3V3 output)
  overhead  = p_batt - p_led                     (ESP+WiFi+converter loss)
  ratio     = p_led / p_batt                     (efficiency LOWER BOUND)

Honest limits (named, per the cautious-framing convention):
  - The ESP32+WiFi share the converter output and are NOT separately metered,
    so absolute efficiency is unrecoverable; ratio is a lower bound and the
    *shape* of overhead vs VBAT is the signal (valid if ESP draw ~constant).
  - led_bus_v sags with load/VBAT, so p_led is not perfectly constant -- that
    is measured (it's in the plot), not assumed away.
  - n=1 cell, n=1 board, one load point (~full RGBW).
  - VBAT axis = gauge_battery_v (board-terminal, under load). The INA bus V
    sits ~80 mV lower (shunt + harness drop at ~430 mA); both are under-load
    voltages, not resting.

  ./bb_efficiency.py data/ca/2026-06-10-discharge-1357.jsonl
"""
import argparse, json, statistics

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
ap.add_argument("jsonl")
ap.add_argument("--out", default=None, help="output PNG (default <input>-bb-eff.png)")
ap.add_argument("--bin-mv", type=float, default=20.0, help="VBAT bin width, mV")
ap.add_argument("--glitch-ma", type=float, default=1500.0, help="drop rows with |batt_ina_ma| above this")
ap.add_argument("--min-bin-n", type=int, default=3)
ap.add_argument("--vbat-field", default="gauge_battery_v",
                help="x-axis voltage field (board-terminal gauge V; or batt_ina_bus_v)")
a = ap.parse_args()

rows = [json.loads(l) for l in open(a.jsonl)]
first_reset = next((r["elapsed_s"] for r in rows if r.get("reset")), float("inf"))
clean, dropped_glitch = [], 0
for r in rows:
    if abs(r["batt_ina_ma"]) > a.glitch_ma:
        dropped_glitch += 1
        continue
    if r["elapsed_s"] >= first_reset:
        continue  # brownout cascade: a different regime, not converter behavior
    r["p_batt"] = r["batt_ina_bus_v"] * abs(r["batt_ina_ma"]) / 1000.0
    r["p_led"] = r["led_bus_v"] * r["led_ma"] / 1000.0
    r["overhead"] = r["p_batt"] - r["p_led"]
    r["ratio"] = r["p_led"] / r["p_batt"] if r["p_batt"] > 0 else None
    clean.append(r)
n_post = sum(1 for r in rows if r["elapsed_s"] >= first_reset)
print(f"{len(rows)} rows: {len(clean)} pre-brownout kept, {n_post} post-first-reset excluded, "
      f"{dropped_glitch} glitch rows dropped (|ma|>{a.glitch_ma:.0f})")

bins = {}
for r in clean:
    b = round(r[a.vbat_field] / (a.bin_mv / 1000.0)) * (a.bin_mv / 1000.0)
    bins.setdefault(round(b, 3), []).append(r)
bx, p_batt, p_led, ovh, ratio, led_v, counts = [], [], [], [], [], [], []
for b in sorted(bins):
    rs = bins[b]
    if len(rs) < a.min_bin_n:
        continue
    bx.append(b)
    p_batt.append(statistics.median(r["p_batt"] for r in rs))
    p_led.append(statistics.median(r["p_led"] for r in rs))
    ovh.append(statistics.median(r["overhead"] for r in rs))
    ratio.append(statistics.median(r["ratio"] for r in rs))
    led_v.append(statistics.median(r["led_bus_v"] for r in rs))
    counts.append(len(rs))

BANDS = [("buck (>3.40 V)", 3.40, 9.9), ("crossover (3.25-3.35 V)", 3.25, 3.35),
         ("boost (3.05-3.25 V)", 3.05, 3.25), ("boost deep (2.90-3.05 V)", 2.90, 3.05),
         ("sag (<2.90 V)", 0.0, 2.90)]
print(f"\nband medians by {a.vbat_field} (pre-brownout, fixed {rows[0]['load']} bri={rows[0]['bri']}):")
print("  band                          n   P_batt  P_led   overhead  P_led/P_batt")
band_stats = {}
for name, lo, hi in BANDS:
    rs = [r for r in clean if lo <= r[a.vbat_field] < hi]
    if not rs:
        print(f"  {name:27s} {0:5d}  (no samples)")
        continue
    m = lambda k: statistics.median(r[k] for r in rs)
    band_stats[name] = (m("p_batt"), m("p_led"), m("overhead"), m("ratio"))
    print(f"  {name:27s} {len(rs):5d}  {m('p_batt'):5.3f}W {m('p_led'):5.3f}W"
          f"  {m('overhead'):5.3f}W   {m('ratio'):.3f}")
if "crossover (3.25-3.35 V)" not in band_stats:
    print("\nNOTE: the 3.25-3.35 V crossover band was NEVER visited under this load --"
          "\nthe terminal voltage sags below it, i.e. the converter runs in BOOST for the"
          "\nwhole discharge at show loads. Crossover/mode-hunt characterization needs a"
          "\nLIGHT-load discharge (or near-full SOC), where the terminal V actually sits there.")

fig, axes = plt.subplots(1, 3, figsize=(14, 4.2), sharex=True)
for ax in axes:
    ax.axvspan(3.25, 3.35, color="orange", alpha=0.15)
    ax.set_xlabel(f"VBAT under load ({a.vbat_field})")
    ax.grid(alpha=0.3)
axes[0].plot(bx, p_batt, "-o", ms=3, label="P_batt (converter in)")
axes[0].plot(bx, p_led, "-o", ms=3, label="P_led (rail slice)")
axes[0].plot(bx, ovh, "-o", ms=3, label="overhead (ESP+loss)")
axes[0].set_ylabel("W"); axes[0].set_title("power vs VBAT (bin medians)")
axes[0].legend(fontsize=8)
axes[1].plot(bx, ratio, "-o", ms=3, color="tab:green")
axes[1].set_ylabel("P_led / P_batt"); axes[1].set_title("efficiency lower bound")
axes[2].plot(bx, led_v, "-o", ms=3, color="tab:red")
axes[2].set_ylabel("LED rail V (under load)"); axes[2].set_title("3V3-rail sag vs VBAT")
fig.suptitle(f"buck-boost behavior vs VBAT -- fixed {rows[0]['load']} load, pre-brownout "
             f"(shaded = nominal crossover)")
fig.tight_layout()
out = a.out or a.jsonl.rsplit(".jsonl", 1)[0] + "-bb-eff.png"
fig.savefig(out, dpi=130)
print(f"plot -> {out}")
