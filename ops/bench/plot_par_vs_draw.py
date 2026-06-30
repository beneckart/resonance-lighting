#!/usr/bin/env python3
"""Single-panel PAR-vs-LED-draw for all tested LED options, with iso-efficiency
reference lines (PAR/mA). Slope = efficiency; reach on the PAR axis = max output.
Pillow (matplotlib wedged). RGBW: only resolvable (high-brightness) points used."""
import json
from PIL import Image, ImageDraw, ImageFont

# (label, json, color, min_led_mA to include)  -- RGBW low end is below the meas. floor
SERIES = [
    ("neohex (NeoDriver)",     "led-eff-neohex.json",        (30, 90, 200),  0),
    ("neohex-direct",          "led-eff-neohex-direct.json",  (90, 170, 230), 0),
    ("hex (NeoDriver)",        "led-eff-hex.json",            (0, 150, 0),    0),
    ("hex-direct",             "led-eff-hex-direct.json",     (150, 60, 170), 0),
    ("rgbw 4W all-white (orig/noisy)", "led-eff-rgbw.json",   (245, 170, 90), 50),
    ("rgbw 4W all-white (clean)",      "led-eff-rgbw-clean.json", (200, 40, 0), 50),
    ("rgbw warm-white only",   "led-eff-rgbw-white.json",     (205, 165, 40), 10),
]
DIR = "ops/bench/data/ca/"

def font(s):
    try: return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", s)
    except Exception: return ImageFont.load_default()
F, FB, FS, FT = font(20), font(23), font(15), font(27)

data = []
for lab, fn, col, mn in SERIES:
    try: d = json.load(open(DIR + fn))
    except FileNotFoundError: continue
    pts = [(r["led_mA"], r["par_net"]) for r in d["rows"] if r.get("led_mA", 0) > mn]
    if pts: data.append((lab, col, sorted(pts)))

xmax = max(x for _, _, pts in data for x, _ in pts) * 1.08
ymax = max(y for _, _, pts in data for _, y in pts) * 1.12
W, H = 1100, 760
img = Image.new("RGB", (W, H), "white"); d = ImageDraw.Draw(img)
d.text((30, 16), "LED output vs power -- PAR per mA (slope = efficiency)", fill=(0, 0, 0), font=FT)
x0, y0, x1, y1 = 110, 70, 1060, 690
d.rectangle([x0, y0, x1, y1], outline=(0, 0, 0), width=2)
def sx(v): return x0 + v / xmax * (x1 - x0)
def sy(v): return y1 - v / ymax * (y1 - y0)
for k in range(6):
    gx = xmax * k / 5; d.line([sx(gx), y0, sx(gx), y1], fill=(235, 235, 235)); d.text((sx(gx)-14, y1+6), f"{gx:.0f}", fill=(0,0,0), font=FS)
    gy = ymax * k / 5; d.line([x0, sy(gy), x1, sy(gy)], fill=(235, 235, 235)); d.text((x0-40, sy(gy)-8), f"{gy:.0f}", fill=(0,0,0), font=FS)
d.text(((x0+x1)//2-60, y1+32), "LED draw (mA)", fill=(0,0,0), font=F)
im = Image.new("RGB", (90, 22), "white"); ImageDraw.Draw(im).text((0,0), "PAR (net)", fill=(0,0,0), font=F)
img.paste(im.rotate(90, expand=True), (x0-78, (y0+y1)//2-45))

# iso-efficiency dashed reference lines (PAR = eff * mA)
for eff in (0.04, 0.06, 0.08):
    yend = eff * xmax
    if yend > ymax: xend = ymax / eff; yend = ymax
    else: xend = xmax
    # dashed
    n = 40
    for i in range(0, n, 2):
        xa = xend * i / n; xb = xend * (i+1) / n
        d.line([sx(xa), sy(eff*xa), sx(xb), sy(eff*xb)], fill=(180, 180, 180), width=1)
    d.text((sx(xend)-70, sy(yend)-18), f"{eff:.02f} PAR/mA", fill=(150, 150, 150), font=FS)

for i, (lab, col, pts) in enumerate(data):
    P = [(sx(x), sy(y)) for x, y in pts]
    for j in range(1, len(P)): d.line([P[j-1], P[j]], fill=col, width=3)
    for px, py in P: d.ellipse([px-5, py-5, px+5, py+5], fill=col)
    d.text((x0+30, y0+20+i*24), f"-- {lab}", fill=col, font=F)

img.save(DIR + "led-par-vs-draw.png")
print("wrote " + DIR + "led-par-vs-draw.png")
