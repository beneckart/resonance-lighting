#!/usr/bin/env python3
"""Analyze mpp_sweep.py sessions: P-vs-VINDPM curves, light-normalized, anchor drift.

Takes one or more sweep JSONLs (e.g. the cool-AM and hot-midday sessions) and
produces:
  - per-session table: setpoint, n, median supply_w, light, W/klight, panel C, flags
  - anchor-drift report (the did-conditions-move check)
  - best setpoint per session + the "what does fixed 5.5 V give up" ratio
  - a 2-panel plot (raw W and light-normalized W vs VINDPM), sessions overlaid,
    anchors drawn as open markers -> <first-input>-curve.png

Light normalization: P_norm = supply_w * (light_ref / light_point) with
light_ref the session median; the light channel is the peer's TSL2591 lux
(light_med, over-the-air) with Apogee PAR (par_med) as fallback for old files.
Linear-in-irradiance is an approximation (fine for the relative curve shape,
rough for absolute W; neither lux nor PAR matches the panel's silicon spectral
response). Absolute watts come from the anchors agreeing, not from this.

  ./mpp_analyze.py data/ca/2026-06-11-mpp-sweep-cool-am-*.jsonl \
                   data/ca/2026-06-11-mpp-sweep-hot-noon-*.jsonl
"""
import argparse, json, statistics, sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
ap.add_argument("jsonl", nargs="+", help="mpp_sweep JSONL file(s), one per session")
ap.add_argument("--out", default=None, help="output PNG (default <first-input>-curve.png)")
ap.add_argument("--drift-warn", type=float, default=0.10)
a = ap.parse_args()


def load_session(path):
    """Last mpp-point row per visit (a redo supersedes its flagged attempt)."""
    by_visit, session = {}, None
    with open(path) as fh:
        for line in fh:
            row = json.loads(line)
            if row.get("src") != "mpp-point" or not row.get("n"):
                continue
            by_visit[row["visit"]] = row
            session = row.get("session", session)
    pts = [by_visit[k] for k in sorted(by_visit)]
    return session or path, pts


def light(p):
    return p.get("light_med") or p.get("par_med")  # lux preferred; PAR = legacy fallback


def normalized(p, light_ref):
    if light_ref and light(p):
        return p["supply_w_med"] * light_ref / light(p)
    return None


sessions = [load_session(p) for p in a.jsonl]
fig, axes = plt.subplots(1, 2, figsize=(11, 4.5), sharex=True)
colors = plt.cm.tab10.colors

for si, (label, pts) in enumerate(sessions):
    if not pts:
        print(f"== {label}: no usable points ==")
        continue
    lights = [light(p) for p in pts if light(p)]
    light_ref = statistics.median(lights) if lights else None
    temps = [p["panel_c_med"] for p in pts if p.get("panel_c_med") is not None] or \
            [p["ir_panel_c"] for p in pts if p.get("ir_panel_c") is not None]

    print(f"\n== {label}"
          + (f"  (light_ref {light_ref:.0f})" if light_ref else "  (NO light channel)")
          + (f"  panel {min(temps):.0f}-{max(temps):.0f} C" if temps else "") + " ==")
    print("  V     n   W_med   light   W/klight  norm_W  ptC   flags")
    for p in pts:
        wk = (1000 * p["supply_w_med"] / light(p)) if light(p) else None
        nw = normalized(p, light_ref)
        li_s = f"{light(p):7.0f}" if light(p) else "    n/a"
        wk_s = f"{wk:8.3f}" if wk is not None else "     n/a"
        nw_s = f"{nw:6.3f}" if nw is not None else "   n/a"
        pt = p.get("panel_c_med")
        if pt is None:
            pt = p.get("ir_panel_c")
        pt_s = f"{pt:5.1f}" if pt is not None else "  n/a"
        flags = ",".join(f for f in p["flags"] if f not in ("no-par", "no-light")) or "-"
        print(f"  {p['maintain_v']:.1f}{'*' if p['is_anchor'] else ' '} {p['n']:4d}"
              f"  {p['supply_w_med']:6.3f} {li_s} {wk_s}  {nw_s} {pt_s}  {flags}")

    # anchor drift (light-normalized when available; raw otherwise)
    anchors = [p for p in pts if p["is_anchor"]]
    if len(anchors) >= 2:
        vals = [normalized(p, light_ref) or p["supply_w_med"] for p in anchors]
        drift = (max(vals) - min(vals)) / statistics.median(vals)
        tag = "light-normalized" if light_ref else "RAW (no light)"
        print(f"  anchor drift ({tag}): {drift:.0%}"
              + (f"  ** > {a.drift_warn:.0%}: conditions moved, absolute W suspect **"
                 if drift > a.drift_warn else "  (stable)"))

    # best setpoint: median across visits of the same setpoint, normalized
    by_v = {}
    for p in pts:
        by_v.setdefault(p["maintain_v"], []).append(normalized(p, light_ref) or p["supply_w_med"])
    curve = {v: statistics.median(ws) for v, ws in by_v.items()}
    best_v = max(curve, key=curve.get)
    print(f"  best setpoint: {best_v:.1f} V at {curve[best_v]:.3f} W"
          + (f"  | 5.5 V gives {curve[5.5]/curve[best_v]:.0%} of best"
             f" (x{curve[best_v]/curve[5.5]:.2f} available)" if 5.5 in curve and curve[5.5] > 0 else ""))

    c = colors[si % len(colors)]
    for ax, key in ((axes[0], "raw"), (axes[1], "norm")):
        # line = per-setpoint median (anchors included, so the curve reaches 5.5 V);
        # open markers = the individual anchor visits (their spread IS the drift)
        vals = {}
        xs_a, ys_a = [], []
        for p in pts:
            y = p["supply_w_med"] if key == "raw" else normalized(p, light_ref)
            if y is None:
                continue
            vals.setdefault(p["maintain_v"], []).append(y)
            if p["is_anchor"]:
                xs_a.append(p["maintain_v"])
                ys_a.append(y)
        line = sorted((v, statistics.median(ys)) for v, ys in vals.items())
        ax.plot([v for v, _ in line], [w for _, w in line], "-o", color=c,
                label=label if key == "raw" else None, ms=5)
        ax.plot(xs_a, ys_a, "o", mfc="none", mec=c, ms=9,
                label=(label + " anchors") if key == "raw" else None)

axes[0].set_title("raw median supply_w")
axes[1].set_title("light-normalized supply_w (curve shape)")
for ax in axes:
    ax.set_xlabel("VINDPM / maintain (V)")
    ax.set_ylabel("panel power into charger (W)")
    ax.grid(alpha=0.3)
axes[0].legend(fontsize=8)
fig.suptitle("MPP sweep: harvest vs VINDPM setpoint (anchors = open markers)")
fig.tight_layout()
out = a.out or a.jsonl[0].rsplit(".jsonl", 1)[0] + "-curve.png"
fig.savefig(out, dpi=130)
print(f"\nplot -> {out}")

if len(sessions) >= 2:
    bests = []
    for label, pts in sessions:
        if not pts:
            continue
        lights = [light(p) for p in pts if light(p)]
        light_ref = statistics.median(lights) if lights else None
        by_v = {}
        for p in pts:
            by_v.setdefault(p["maintain_v"], []).append(normalized(p, light_ref) or p["supply_w_med"])
        curve = {v: statistics.median(ws) for v, ws in by_v.items()}
        bests.append((label, max(curve, key=curve.get)))
    if len(bests) >= 2:
        vs = [b for _, b in bests]
        print(f"Vmp shift across sessions: {' vs '.join(f'{l}={v:.1f}V' for l, v in bests)}"
              f"  (delta {max(vs)-min(vs):.1f} V -> a single fixed setpoint"
              f" {'cannot be optimal across temps' if max(vs)-min(vs) >= 0.2 else 'looks adequate'})")
