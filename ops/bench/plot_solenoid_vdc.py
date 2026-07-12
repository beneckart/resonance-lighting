#!/usr/bin/env python3
"""Plot the MSA311-instrumented solenoid VDC-tap sweep: strike impact energy
vs supply node voltage, per pulse width, with the P105 panel's real operating
band overlaid. Reads the new-format (peak_mg) rows from the sweep CSV; the
older by-ear rows are a different measurement and are skipped.

Usage: python3 plot_solenoid_vdc.py [csv_path] [out_png]
"""
import csv
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

CSV = sys.argv[1] if len(sys.argv) > 1 else \
    f"{__file__.rsplit('/', 1)[0]}/data/solenoid-vdc-sweep-2026-07-11.csv"
OUT = sys.argv[2] if len(sys.argv) > 2 else CSV.replace(".csv", ".png")

SURFACE = "#fcfcfb"
INK, INK2, MUTED = "#1a1a19", "#5f5e56", "#8a897f"
SERIES = {5: "#2a78d6", 6: "#1baf7a"}  # categorical slots 1-2, validated

# rows: new format = 13 cols with peak_mg_med at [5]; skip bursts + old by-ear rows
data = defaultdict(list)  # width -> [(node_v, med, lo, hi)]
for row in csv.reader(open(CSV)):
    if len(row) != 13 or row[12] == "burst20" or row[0] == "ts":
        continue
    try:
        width = int(row[2])
        med, lo, hi = float(row[5]), float(row[6]), float(row[7])
        node_v = float(row[8])
    except ValueError:
        continue
    if med > 100:  # peak_mg magnitude; excludes any stray old-format row
        data[width].append((node_v, med, lo, hi))

fig, ax = plt.subplots(figsize=(8.6, 5.4), dpi=150)
fig.patch.set_facecolor(SURFACE)
ax.set_facecolor(SURFACE)

# P105 operating overlay: MPPT-loaded window + Voc line
ax.axvspan(4.6, 5.8, color="#e9e8e2", zorder=0)
ax.text(5.2, 6650, "P105 under MPPT\n(VINDPM window)", ha="center", va="top",
        fontsize=9, color=INK2, linespacing=1.3)
ax.axvline(6.9, color=MUTED, lw=1.2, ls=(0, (4, 3)), zorder=1)
ax.text(6.93, 6650, "P105 Voc\n(charger idle)", ha="left", va="top",
        fontsize=9, color=INK2, linespacing=1.3)

for width, off in ((5, -0.04), (6, +0.04)):
    pts = sorted(data[width])
    xs = [p[0] + off for p in pts]
    med = [p[1] for p in pts]
    lo = [p[1] - p[2] for p in pts]
    hi = [p[3] - p[1] for p in pts]
    ax.errorbar(xs, med, yerr=[lo, hi], color=SERIES[width], lw=2,
                marker="o", ms=7, capsize=3, elinewidth=1.2, zorder=3,
                label=f"{width} ms pulse")
    ax.annotate(f"{width} ms", (xs[-1], med[-1]), xytext=(8, 0),
                textcoords="offset points", va="center", fontsize=10, color=INK)

ax.set_title("Solenoid strike impact vs VDC node voltage",
             fontsize=13, color=INK, loc="left", pad=14)
ax.text(0, 1.015, "5/5 strikes at every point, down to 5 ms pulses -- whiskers are min-max of 5 strikes",
        transform=ax.transAxes, fontsize=9.5, color=INK2, va="bottom")
ax.set_xlabel("supply node voltage under charger load (V, BQ ADC)", fontsize=10, color=INK2)
ax.set_ylabel("impact peak (milli-g, MSA311 on strike surface)", fontsize=10, color=INK2)
ax.set_ylim(0, 6900)
ax.grid(axis="y", color="#e4e3dc", lw=0.8, zorder=0)
ax.tick_params(colors=INK2, labelsize=9)
for side in ("top", "right"):
    ax.spines[side].set_visible(False)
for side in ("left", "bottom"):
    ax.spines[side].set_color("#d4d3ca")
ax.legend(loc="upper left", frameon=False, fontsize=9.5, labelcolor=INK)
fig.text(0.055, 0.012,
         "Scatter is real: 1 kHz accel sampling aliases the ~ms impact transient; mount is tape + rubber band.\n"
         "Bench PSU on VDC; charger co-drawing 0-456 mA from the same node. n=5 strikes/point, 2026-07-11.",
         fontsize=8, color=MUTED, linespacing=1.4)
fig.tight_layout(rect=(0, 0.05, 1, 1))
fig.savefig(OUT, facecolor=SURFACE)
print(f"wrote {OUT}")
