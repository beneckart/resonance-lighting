#!/usr/bin/env python3
"""Analyze an afk_sweep JSONL: LED power curves + INA-vs-fuel-gauge cross-check.

Answers two questions:
  1. LED power characterization: current vs channel level for {RGB,W,RGBW} x gamma.
  2. Can we make the MAX17260 fuel gauge more accurate, using INA ground truth?
     - instantaneous current bias (gauge vs INA 0x45) -> a correction factor
     - coulomb accounting (gauge integral vs INA integral)
     - true capacity from SOC-drop vs INA-integrated mAh (vs the gauge's DesignCap)
     - LFP resting discharge curve (V vs mAh removed)

  ./afk_analyze.py data/ca/2026-06-10-afk-sweep-0031.jsonl
Writes <jsonl>-power.png and <jsonl>-gauge.png + a printed summary. Stdlib+matplotlib.
"""
import json, sys, os, statistics
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

src = next((a for a in sys.argv[1:] if a.endswith(".jsonl")), None)
if not src:
    sys.exit("usage: afk_analyze.py <afk-sweep.jsonl>")
rows = [json.loads(l) for l in open(src) if l.strip()]
if not rows:
    sys.exit("no rows yet")
base = src.replace(".jsonl", "")
fnum = lambda x: isinstance(x, (int, float))

print(f"=== afk_analyze: {os.path.basename(src)} ({len(rows)} points) ===")

# Repair glitches: reject physically-impossible INA 0x45 samples (I2C/serial spikes -- e.g. a
# lone -21 A read) and RE-INTEGRATE mah_ina from the repaired current. The logged mah_ina_integ
# baked any glitch into the running total, so recompute it here (hold the previous good current
# across a glitch; trapezoidal on elapsed_s). Fixes both the scatter x-axis and the capacity.
MAXMA = 1500.0
ndrop = 0; cum = 0.0
for i, r in enumerate(rows):
    ma = r.get("batt_ina_ma")
    if fnum(ma) and abs(ma) > MAXMA:
        r["batt_ina_ma"] = rows[i - 1]["batt_ina_ma"] if i else 0.0
        r["glitch"] = True; ndrop += 1
    if i and fnum(r.get("batt_ina_ma")) and fnum(rows[i - 1].get("batt_ina_ma")) \
       and fnum(r.get("elapsed_s")) and fnum(rows[i - 1].get("elapsed_s")):
        cum += 0.5 * (abs(r["batt_ina_ma"]) + abs(rows[i - 1]["batt_ina_ma"])) * \
               (r["elapsed_s"] - rows[i - 1]["elapsed_s"]) / 3600.0
    r["mah_ina_integ"] = round(cum, 2)
if ndrop:
    print(f"[ablated {ndrop} INA glitch sample(s) (|>{MAXMA:.0f}| mA); re-integrated -> clean mah_ina = {cum:.0f} mAh]")
soc0 = next((r["gauge_soc"] for r in rows if fnum(r.get("gauge_soc"))), None)
socN = next((r["gauge_soc"] for r in reversed(rows) if fnum(r.get("gauge_soc"))), None)
elapsed = rows[-1].get("elapsed_s", 0) / 60.0
print(f"duration {elapsed:.1f} min | SOC {soc0}% -> {socN}% | "
      f"coulomb removed: INA {rows[-1].get('mah_ina_integ')} mAh, gauge {rows[-1].get('mah_gauge_integ')} mAh")

# ---- 1) LED power curves: average led_ma per (pattern, gamma, level) over cycles ----
COL = {"RGB": "#d62728", "W": "#1f77b4", "RGBW": "#9467bd"}
agg = {}
for r in rows:
    if not fnum(r.get("led_ma")) or "pattern" not in r or "level" not in r:
        continue  # discharge/constant-load rows have no sweep dims
    agg.setdefault((r["gamma"], r["pattern"]), {}).setdefault(r["level"], []).append(r["led_ma"])
if agg:
    fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=True)
    for ax, g in zip(axes, (0, 1)):
        for pat in ("RGB", "W", "RGBW"):
            pts = sorted((lvl, statistics.mean(v)) for lvl, v in agg.get((g, pat), {}).items())
            if pts:
                x, y = zip(*pts)
                ax.plot(x, y, "-o", ms=4, color=COL[pat], label=pat)
        ax.set_title(f"gamma {'ON' if g else 'OFF'}", fontweight="bold")
        ax.set_xlabel("channel level (0-255)"); ax.grid(True, alpha=0.3); ax.legend(title="pattern")
    axes[0].set_ylabel("LED current (mA)  [shunt-corrected 0.01 Ω]")
    fig.suptitle("4 W SK6812 RGBW — LED current vs level (avg over cycles)", fontsize=14, fontweight="bold")
    fig.tight_layout(rect=[0, 0.02, 1, 0.96]); fig.savefig(base + "-power.png", dpi=130)
    print("wrote", base + "-power.png")
else:
    print("(constant-load / discharge run — no sweep dims; skipping power-curve plot)")

# ---- 2) gauge cross-check ----
pairs = [(r["batt_ina_ma"], r["gauge_battery_ma"]) for r in rows
         if fnum(r.get("batt_ina_ma")) and fnum(r.get("gauge_battery_ma"))
         and abs(r["batt_ina_ma"]) > 1]
fig, ax = plt.subplots(1, 3, figsize=(16, 4.6))

# 2a) instantaneous: gauge vs INA (both are discharge-negative)
if len(pairs) >= 3:
    xi = [abs(a) for a, _ in pairs]; yg = [abs(b) for _, b in pairs]
    ax[0].scatter(xi, yg, s=10, alpha=0.5)
    lim = max(max(xi), max(yg)) * 1.05
    ax[0].plot([0, lim], [0, lim], "k--", lw=1, label="1:1")
    # robust median ratio (a linear fit is fooled by multi-regime discharge data + the
    # brownout tail; skip tiny currents where the ratio is just division noise)
    ratios = [g / i for i, g in zip(xi, yg) if i > 50]
    if ratios:
        rmed = statistics.median(ratios)
        ax[0].plot([0, lim], [0, rmed * lim], "r-", lw=1.5, label=f"median {rmed:.3f}×")
        print(f"\nGAUGE current bias (median, n={len(ratios)}, |INA|>50 mA): gauge/INA = {rmed:.3f}"
              f"  -> gauge reads ~{(rmed-1)*100:+.1f}% high; correct gauge current by /{rmed:.3f}")
    ax[0].set_xlabel("|INA 0x45| mA (truth)"); ax[0].set_ylabel("|gauge battery_ma|")
    ax[0].set_title("instantaneous current"); ax[0].legend(fontsize=8); ax[0].grid(alpha=0.3)

# 2b) coulomb integrals over time
t = [r.get("elapsed_s", 0) / 60.0 for r in rows]
ax[1].plot(t, [r.get("mah_ina_integ") for r in rows], label="INA 0x45 (truth)")
ax[1].plot(t, [r.get("mah_gauge_integ") for r in rows], label="gauge")
ax[1].set_xlabel("min"); ax[1].set_ylabel("mAh removed"); ax[1].set_title("coulomb count")
ax[1].legend(fontsize=8); ax[1].grid(alpha=0.3)

# 2c) SOC vs true mAh removed -> capacity; + resting discharge curve
soc_pts = [(r["mah_ina_integ"], r["gauge_soc"]) for r in rows
           if fnum(r.get("mah_ina_integ")) and fnum(r.get("gauge_soc"))]
rest = [(r["mah_ina_integ"], r["gauge_battery_v"]) for r in rows
        if r.get("level") == 0 and fnum(r.get("gauge_battery_v")) and fnum(r.get("mah_ina_integ"))]
if not rest:  # discharge/constant-load run: use the full under-load V-vs-mAh discharge curve
    rest = [(r["mah_ina_integ"], r["gauge_battery_v"]) for r in rows
            if fnum(r.get("mah_ina_integ")) and fnum(r.get("gauge_battery_v"))]
if soc_pts:
    xm, ys = zip(*soc_pts); ax[2].plot(xm, ys, ".", ms=4, label="gauge SOC")
    drop = (soc_pts[0][1] - soc_pts[-1][1])
    used = soc_pts[-1][0] - soc_pts[0][0]
    if drop >= 3 and used > 0:
        mah_per_pct = used / drop
        cap_true = mah_per_pct * 100
        print(f"\nCAPACITY: {used:.0f} mAh (INA) for a {drop:.0f}% SOC drop "
              f"-> {mah_per_pct:.1f} mAh/% -> implied full capacity ~{cap_true:.0f} mAh")
        print(f"  gauge DesignCap is set to 2000 mAh (20 mAh/%); measured {mah_per_pct:.1f} mAh/% "
              f"=> set DesignCap ~{cap_true:.0f} mAh for accurate SOC")
    else:
        print(f"\nCAPACITY: SOC moved only {drop:.0f}% ({used:.0f} mAh) — too little; need a longer run.")
ax[2].set_xlabel("mAh removed (INA)"); ax[2].set_ylabel("gauge SOC %")
ax[2].set_title("SOC vs true coulombs"); ax[2].grid(alpha=0.3)
if rest:
    xr, vr = zip(*rest); ax2b = ax[2].twinx(); ax2b.plot(xr, vr, "g-^", ms=4, label="resting V")
    ax2b.set_ylabel("resting battery_v (LED off)", color="g")
ax[2].legend(fontsize=8, loc="upper right")

fig.suptitle("INA (ground truth) vs MAX17260 fuel gauge", fontsize=13, fontweight="bold")
fig.tight_layout(rect=[0, 0.02, 1, 0.94]); fig.savefig(base + "-gauge.png", dpi=130)
print("wrote", base + "-gauge.png")
