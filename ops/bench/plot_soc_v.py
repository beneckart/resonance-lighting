#!/usr/bin/env python3
"""Render SOC-vs-voltage and time-series plots from a power_logger JSONL trace.

matplotlib on this box is wedged (NumPy 2 vs a 1.x-built mpl), so this draws the
figure directly with Pillow -- no matplotlib/numpy-C dependency. Usage:

    python3 ops/bench/plot_soc_v.py <trace.jsonl> [out.png]
"""
import json, sys
from datetime import datetime
from PIL import Image, ImageDraw, ImageFont

src = sys.argv[1]
out = sys.argv[2] if len(sys.argv) > 2 else src.rsplit(".", 1)[0] + "-soc_v.png"
TITLE = sys.argv[3] if len(sys.argv) > 3 else "PowerFeather V2 — Li-ion 4400 mAh USB charge: SOC vs battery voltage"

rows = []
with open(src) as fh:
    for line in fh:
        line = line.strip()
        if not line:
            continue
        try:
            r = json.loads(line)
        except json.JSONDecodeError:
            continue
        if not r.get("reachable", True):
            continue
        v = r.get("battery_v"); soc = r.get("soc_pct")
        if v is None or soc is None:
            continue
        try:
            t = datetime.fromisoformat(r["ts_utc"]).timestamp()
        except Exception:
            continue
        rows.append((t, float(v), float(soc), r.get("battery_ma"),
                     r.get("reset_reason", ""), r.get("led_mode", "")))

rows.sort(key=lambda x: x[0])
t0 = rows[0][0]
ts = [(r[0] - t0) / 3600.0 for r in rows]   # elapsed hours
vs = [r[1] for r in rows]
socs = [r[2] for r in rows]
print(f"rows={len(rows)}  V[{min(vs):.3f},{max(vs):.3f}]  SOC[{min(socs):.0f},{max(socs):.0f}]  span={ts[-1]:.1f}h")
# adaptive voltage axis (works for both Li-ion charge ~3.6-4.2 and LFP plateau ~3.2-3.6)
vlo = int((min(vs) - 0.02) * 20) / 20.0
vhi = (int((max(vs) + 0.02) * 20) + 1) / 20.0
step = 0.05 if (vhi - vlo) <= 0.8 else 0.1
vticks = [round(vlo + step * k, 2) for k in range(int((vhi - vlo) / step) + 1)]

# ---- fonts ----
def font(sz):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
        try:
            return ImageFont.truetype(p, sz)
        except Exception:
            pass
    return ImageFont.load_default()
F = font(20); FB = font(24); FS = font(16); FT = font(30)

W, H = 1500, 1200
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

def lerp(a, b, f): return a + (b - a) * f
def heat(f):  # time gradient blue->red
    f = max(0.0, min(1.0, f))
    return (int(lerp(40, 220, f)), int(lerp(90, 50, f)), int(lerp(200, 40, f)))

def axes(x0, y0, x1, y1, xmin, xmax, ymin, ymax, xlabel, ylabel, title,
         xticks, yticks, xfmt="{:.2f}", yfmt="{:.0f}"):
    d.rectangle([x0, y0, x1, y1], outline=(0, 0, 0), width=2)
    d.text(((x0 + x1) / 2 - len(title) * 7, y0 - 38), title, fill=(0, 0, 0), font=FB)
    def sx(v): return x0 + (v - xmin) / (xmax - xmin) * (x1 - x0)
    def sy(v): return y1 - (v - ymin) / (ymax - ymin) * (y1 - y0)
    for xt in xticks:
        X = sx(xt)
        d.line([X, y0, X, y1], fill=(225, 225, 225), width=1)
        d.line([X, y1, X, y1 + 6], fill=(0, 0, 0), width=2)
        d.text((X - 18, y1 + 9), xfmt.format(xt), fill=(0, 0, 0), font=FS)
    for yt in yticks:
        Y = sy(yt)
        d.line([x0, Y, x1, Y], fill=(225, 225, 225), width=1)
        d.line([x0 - 6, Y, x0, Y], fill=(0, 0, 0), width=2)
        d.text((x0 - 52, Y - 9), yfmt.format(yt), fill=(0, 0, 0), font=FS)
    d.text(((x0 + x1) / 2 - len(xlabel) * 6, y1 + 36), xlabel, fill=(0, 0, 0), font=F)
    img2 = Image.new("RGB", (len(ylabel) * 13, 26), "white")
    ImageDraw.Draw(img2).text((0, 0), ylabel, fill=(0, 0, 0), font=F)
    img.paste(img2.rotate(90, expand=True), (x0 - 92, int((y0 + y1) / 2 - len(ylabel) * 6)))
    return sx, sy

d.text((40, 24), TITLE, fill=(0, 0, 0), font=FT)
d.text((40, 66), f"source: {src.split('/')[-1]}   ({len(rows)} samples, {ts[-1]:.1f} h)",
       fill=(90, 90, 90), font=FS)

# ===== Panel A: SOC (y) vs Voltage (x), colored by time =====
vmin, vmax = vlo, vhi
sx, sy = axes(150, 150, 720, 720, vmin, vmax, 0, 100,
              "battery_v (V)", "soc_pct (%)", "SOC vs Voltage",
              vticks, [0, 20, 40, 60, 80, 100])
n = len(rows)
for i in range(1, n):
    if i % 3:  # light downsample for speed
        continue
    d.line([sx(vs[i - 1]), sy(socs[i - 1]), sx(vs[i]), sy(socs[i])],
           fill=heat(ts[i] / ts[-1]), width=3)
# legend
d.text((430, 165), "color = time", fill=(0, 0, 0), font=FS)
for k in range(60):
    d.rectangle([430 + k * 3, 188, 433 + k * 3, 200], fill=heat(k / 59))
d.text((430, 204), "start", fill=(40, 90, 200), font=FS)
d.text((560, 204), "end", fill=(220, 50, 40), font=FS)

# ===== Panel B: V and SOC vs time =====
sx2, sy2 = axes(900, 150, 1430, 720, 0, ts[-1], vlo, vhi,
                "elapsed (h)", "battery_v (V)", "Voltage & SOC vs time",
                [round(x, 1) for x in (0, ts[-1]/4, ts[-1]/2, 3*ts[-1]/4, ts[-1])],
                vticks, xfmt="{:.1f}", yfmt="{:.2f}")
def sy2(v): return 720 - (v - vlo) / (vhi - vlo) * (720 - 150)
def sy_soc(v): return 720 - (v - 0) / (100 - 0) * (720 - 150)
for yt in (0, 25, 50, 75, 100):  # right axis (SOC)
    Y = sy_soc(yt)
    d.line([1430, Y, 1436, Y], fill=(0, 130, 0), width=2)
    d.text((1440, Y - 9), f"{yt}", fill=(0, 130, 0), font=FS)
d.text((1455, 420), "SOC %", fill=(0, 130, 0), font=FS)
for i in range(1, n):
    if i % 3:
        continue
    d.line([sx2(ts[i-1]), sy2(vs[i-1]), sx2(ts[i]), sy2(vs[i])], fill=(30, 60, 200), width=2)
    d.line([sx2(ts[i-1]), sy_soc(socs[i-1]), sx2(ts[i]), sy_soc(socs[i])], fill=(0, 150, 0), width=2)
d.text((950, 690), "blue = V", fill=(30, 60, 200), font=FS)
d.text((1050, 690), "green = SOC", fill=(0, 150, 0), font=FS)

# ===== takeaway box =====
def slope_note():
    # voltage spanned while SOC went 20->80 (the "useful middle")
    lo = next((v for v, s in zip(vs, socs) if s >= 20), vs[0])
    hi = next((v for v, s in zip(vs, socs) if s >= 80), vs[-1])
    return lo, hi
lo, hi = slope_note()
notes = [
    "Reading this plot:",
    f"- Li-ion (Generic_3V7): SOC 20%->80% spans ~{hi-lo:.2f} V ({lo:.2f}->{hi:.2f} V) — a usable slope, gauge can map V->SOC.",
    "- This is a CHARGE curve, so V sits ABOVE the resting curve (IR rise from charge current); resting V-SOC is lower/flatter.",
    "- Contrast LFP: its plateau is ~3.2-3.3 V across roughly 20-90% SOC — almost no slope, so V->SOC is unreliable there.",
    "- Under load the cell sags (we saw ~2.85 V), which on LFP reads near-empty even at decent true SOC. Coulomb-count, don't trust V.",
]
y = 770
d.text((150, y), "Takeaways", fill=(0, 0, 0), font=FB); y += 38
for ln in notes:
    d.text((150, y), ln, fill=(20, 20, 20), font=F); y += 32

img.save(out)
print("wrote", out)
