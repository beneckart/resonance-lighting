#!/usr/bin/env python3
"""Plot LED efficiency-sweep results (NeoHEX vs HEX ...) from led_efficiency_sweep.py
JSON files. matplotlib is wedged on this box (NumPy 2 vs 1.x mpl), so render with Pillow.

    python3 ops/bench/plot_led_eff.py led-eff-neohex.json led-eff-hex.json [out.png]
"""
import json, sys
from PIL import Image, ImageDraw, ImageFont

paths = [a for a in sys.argv[1:] if a.endswith(".json")]
out = next((a for a in sys.argv[1:] if a.endswith(".png")), "ops/bench/data/ca/led-eff-compare.png")
series = []
for p in paths:
    d = json.load(open(p))
    series.append((d["label"], d["rows"]))

def font(sz):
    for q in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",):
        try: return ImageFont.truetype(q, sz)
        except Exception: pass
    return ImageFont.load_default()
F, FB, FS, FT = font(20), font(24), font(15), font(28)
COLORS = [(30, 90, 200), (210, 60, 40), (0, 150, 0), (160, 80, 200)]

W, H = 1500, 720
img = Image.new("RGB", (W, H), "white"); d = ImageDraw.Draw(img)
d.text((40, 18), "LED efficiency sweep -- " + " vs ".join(s[0] for s in series), fill=(0, 0, 0), font=FT)

def panel(x0, y0, x1, y1, xs, ys, xlabel, ylabel, title, xmax=None, ymax=None):
    d.rectangle([x0, y0, x1, y1], outline=(0, 0, 0), width=2)
    d.text((x0 + 60, y0 - 30), title, fill=(0, 0, 0), font=FB)
    xmax = xmax or max(xs) * 1.05 or 1; ymax = ymax or max(ys) * 1.1 or 1
    def sx(v): return x0 + v / xmax * (x1 - x0)
    def sy(v): return y1 - v / ymax * (y1 - y0)
    for k in range(6):
        gx = xmax * k / 5; X = sx(gx)
        d.line([X, y0, X, y1], fill=(230, 230, 230)); d.text((X - 14, y1 + 6), f"{gx:.0f}", fill=(0, 0, 0), font=FS)
        gy = ymax * k / 5; Y = sy(gy)
        d.line([x0, Y, x1, Y], fill=(230, 230, 230)); d.text((x0 - 40, Y - 8), f"{gy:.0f}", fill=(0, 0, 0), font=FS)
    d.text(((x0 + x1) // 2 - 40, y1 + 30), xlabel, fill=(0, 0, 0), font=F)
    im = Image.new("RGB", (len(ylabel) * 12, 24), "white"); ImageDraw.Draw(im).text((0, 0), ylabel, fill=(0, 0, 0), font=F)
    img.paste(im.rotate(90, expand=True), (x0 - 78, (y0 + y1) // 2 - len(ylabel) * 6))
    return sx, sy

# Panel A: PAR_net vs LED_mA (efficiency: higher line = more light per mA)
allx = [r["led_mA"] for _, rows in series for r in rows if r["led_mA"] > 0]
ally = [r["par_net"] for _, rows in series for r in rows if r["led_mA"] > 0]
sx, sy = panel(110, 70, 700, 620, allx, ally, "LED draw (mA)", "PAR (net)",
               "Light vs power  (higher = more efficient)", xmax=max(allx) * 1.05, ymax=max(ally) * 1.1)
for i, (lab, rows) in enumerate(series):
    pts = [(sx(r["led_mA"]), sy(r["par_net"])) for r in rows if r["led_mA"] > 0]
    for j in range(1, len(pts)): d.line([pts[j - 1], pts[j]], fill=COLORS[i], width=3)
    for px, py in pts: d.ellipse([px - 4, py - 4, px + 4, py + 4], fill=COLORS[i])
    d.text((130, 90 + i * 22), f"-- {lab}", fill=COLORS[i], font=F)

# Panel B: LED_mA vs brightness (power scaling with setting)
allb = [r["brightness"] for _, rows in series for r in rows]
alld = [r["led_mA"] for _, rows in series for r in rows]
sx2, sy2 = panel(820, 70, 1410, 620, allb, alld, "brightness (0-255)", "LED draw (mA)",
                 "Draw vs brightness", xmax=255, ymax=max(alld) * 1.1)
for i, (lab, rows) in enumerate(series):
    pts = [(sx2(r["brightness"]), sy2(r["led_mA"])) for r in rows]
    for j in range(1, len(pts)): d.line([pts[j - 1], pts[j]], fill=COLORS[i], width=3)
    for px, py in pts: d.ellipse([px - 4, py - 4, px + 4, py + 4], fill=COLORS[i])
    d.text((840, 90 + i * 22), f"-- {lab}", fill=COLORS[i], font=F)

img.save(out)
print("wrote", out)
