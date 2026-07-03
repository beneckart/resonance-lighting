#!/usr/bin/env python3
"""Plot an led_ina_sweep JSONL: LED current vs brightness, per mode x gamma.

  ./plot_led_ina_sweep.py data/ca/2026-06-09-led-ina-sweep-2152.jsonl [out.png] [scale=10]

scale=N multiplies the logged led_ma (use scale=10 for sweeps recorded BEFORE the
SEN0291 shunt fix, when ina_monitor assumed 0.1 ohm instead of the true 0.01 ohm).
"""
import json, sys, os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

src = next((a for a in sys.argv[1:] if a.endswith(".jsonl")), None)
if not src:
    sys.exit("usage: plot_led_ina_sweep.py <sweep.jsonl> [out.png] [scale=N]")
out = next((a for a in sys.argv[1:] if a.endswith(".png")), src.replace(".jsonl", ".png"))
scale = next((float(a.split("=")[1]) for a in sys.argv[1:] if a.startswith("scale=")), 1.0)

rows = [json.loads(l) for l in open(src) if l.strip()]
notes = rows[0].get("notes", "") if rows else ""
COLORS = {"RGB": "#d62728", "W": "#1f77b4", "RGBW": "#9467bd"}

# Baseline-correct the INA zero-offset: subtract the median LED-off (bri=0) reading.
zeros = sorted(r["led_ma"] for r in rows if r["bri"] == 0)
base = zeros[len(zeros) // 2] if zeros else 0.0

fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=True)
ymax = max(((r["led_ma"] - base) * scale for r in rows), default=30) * 1.12
for ax, g in zip(axes, (0, 1)):
    for mode in ("RGB", "W", "RGBW"):
        pts = sorted([(r["bri"], (r["led_ma"] - base) * scale) for r in rows
                      if r["gamma"] == g and r["mode"] == mode])
        if not pts:
            continue
        x, y = zip(*pts)
        ax.plot(x, y, "-o", ms=4, lw=1.8, color=COLORS[mode], label=mode)
    ax.set_title(f"gamma {'ON' if g else 'OFF'}", fontsize=12, fontweight="bold")
    ax.set_xlabel("brightness setting (bri, 0-255)")
    ax.grid(True, alpha=0.3)
    ax.set_xlim(0, 255)
    ax.set_ylim(0, ymax)
    ax.legend(title="channels", loc="upper left")
axes[0].set_ylabel("LED current (mA)")
corr = f"  *  shunt-corrected x{scale:g} (SEN0291 0.01 ohm)" if scale != 1.0 else ""
fig.suptitle("SK6812 RGBW @ 3.3 V rail -- LED current vs brightness" + corr,
             fontsize=14, fontweight="bold")
fig.text(0.5, 0.005, (notes + "   *   " if notes else "")
         + f"baseline-corrected (-{base * scale:+.2f} mA LED-off offset)"
         + (f", x{scale:g} shunt fix" if scale != 1.0 else "")
         + f"   *   {os.path.basename(src)}",
         ha="center", fontsize=8, color="#666")
fig.tight_layout(rect=[0, 0.03, 1, 0.96])
fig.savefig(out, dpi=130)
print("wrote", out)
