#!/usr/bin/env python3
"""Plot RSSI-vs-time from a net_bench_walk.py log (the walk-out-and-back "V").

Walker peer drawn bold; stationary reference peers faint (they should stay flat -- if
they dip too, it was environmental, not the walk). Dropouts (gaps in a peer's series)
are left as breaks in the line. Pillow only (matches the repo's other plotters).

  ./net_bench_walk_plot.py ops/bench/data/ca/<date>-rangewalk.jsonl --walker 9F2690
"""
import argparse, json
from PIL import Image, ImageDraw

ap = argparse.ArgumentParser()
ap.add_argument("log")
ap.add_argument("--walker", default="9F2690")
ap.add_argument("--out", default=None)
ap.add_argument("--gap", type=float, default=4.0, help="break the line if >gap seconds between samples")
ap.add_argument("--markers", default=None, help="JSONL of {t,steps,label} landmarks to annotate")
a = ap.parse_args()

series = {}  # id -> list of (t, rssi)
for line in open(a.log):
    line = line.strip()
    if not line:
        continue
    try:
        r = json.loads(line)
    except json.JSONDecodeError:
        continue
    series.setdefault(r["id"], []).append((r["t"], r["rssi"]))
if not series:
    raise SystemExit("no data")

W, H = 1100, 560
ML, MR, MT, MB = 70, 160, 40, 50
pw, ph = W - ML - MR, H - MT - MB
allt = [t for s in series.values() for t, _ in s]
allr = [r for s in series.values() for _, r in s]
tmin, tmax = 0, max(allt)
rmin, rmax = min(allr) - 3, max(max(allr) + 3, -20)


def X(t): return ML + (t - tmin) / (tmax - tmin or 1) * pw
def Y(r): return MT + (rmax - r) / (rmax - rmin or 1) * ph


img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)
# axes + gridlines (RSSI every 10 dB)
d.rectangle([ML, MT, ML + pw, MT + ph], outline="black")
r = int(rmax)
while r >= rmin:
    if r % 10 == 0:
        y = Y(r); d.line([ML, y, ML + pw, y], fill="#e0e0e0"); d.text((8, y - 6), f"{r} dBm", fill="black")
    r -= 1
for k in range(0, int(tmax) + 1, max(10, int(tmax // 10 // 10 * 10) or 10)):
    x = X(k); d.line([x, MT, x, MT + ph], fill="#f0f0f0"); d.text((x - 8, MT + ph + 6), f"{k}s", fill="black")
d.text((W // 2 - 30, H - 16), "elapsed (s)", fill="black")
d.text((ML, 14), "RSSI vs time -- range walk (out and back = V)", fill="black")

colors = ["#1f77b4", "#888", "#aaa", "#bbb", "#ccc"]
ids = [a.walker] + [i for i in series if i != a.walker]
for idx, pid in enumerate(ids):
    pts = series.get(pid, [])
    is_walk = (pid == a.walker)
    col = "#d62728" if is_walk else colors[min(idx, len(colors) - 1)]
    wdt = 3 if is_walk else 1
    prevpt = None
    for (t, rv) in pts:
        pt = (X(t), Y(rv))
        if prevpt and (t - prevpt[0]) <= a.gap:
            d.line([prevpt[1], pt], fill=col, width=wdt)
        prevpt = (t, pt)
    # legend
    ly = MT + 10 + idx * 18
    d.line([ML + pw + 12, ly, ML + pw + 30, ly], fill=col, width=wdt)
    d.text((ML + pw + 34, ly - 6), pid + (" (walker)" if is_walk else ""), fill="black")

# landmark markers: vertical dashed lines + stacked labels
if a.markers:
    mk = [json.loads(l) for l in open(a.markers) if l.strip()]
    for i, m in enumerate(mk):
        x = X(m["t"])
        for yy in range(MT, MT + ph, 6):  # dashed vertical
            d.line([x, yy, x, yy + 3], fill="#999")
        lab = f"{m['steps']}st {m['label'].replace('return: ','>')}"
        ty = MT + ph - 14 - (i % 3) * 14  # stagger to reduce overlap
        d.text((x + 2, ty), lab[:22], fill="#006400")

out = a.out or a.log.rsplit(".", 1)[0] + ".png"
img.save(out)
print("wrote", out)
