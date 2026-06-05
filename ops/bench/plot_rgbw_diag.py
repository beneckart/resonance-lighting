#!/usr/bin/env python3
"""Diagnostic plot for the 4W RGBW (Adafruit 5163) on the 3.3V rail: shows it's
voltage-starved (Vf ~3.0-3.2V right at the rail). Light (PAR) rises smoothly but the
current is non-linear (flat-low until the PWM is high enough to draw resolvable
current), and the rail sags into the LED's Vf zone under load. Pillow (mpl is wedged).
"""
from PIL import Image, ImageDraw, ImageFont

# clean RGBW sweep (3.3V, --wifi-lowpower); bv per step from the run log
br     = [0, 5, 15, 30, 60, 100, 160, 255]
led_mA = [0, 1, 7, 0, 14, 221, 287, 430]
par    = [1, 2, 3, 6, 10, 16, 25, 40]
bv     = [3.41, 3.404, 3.398, 3.380, 3.363, 3.348, 3.205, 3.111]
# hex-direct (smooth, for contrast) — same rig, 37x SK6812
hb, hled = [0, 5, 15, 30, 60], [0, 24, 78, 166, 362]
VF_LO, VF_HI = 3.0, 3.2  # warm-white LED forward-voltage band

def font(s):
    try: return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", s)
    except Exception: return ImageFont.load_default()
F, FB, FS, FT = font(19), font(22), font(14), font(26)
W, H = 1500, 640
img = Image.new("RGB", (W, H), "white"); d = ImageDraw.Draw(img)
d.text((30, 16), "4W RGBW (Adafruit 5163) on 3.3V — voltage-starved", fill=(0, 0, 0), font=FT)

def axes(x0, y0, x1, y1, xmax, ymin, ymax, title, ylab):
    d.rectangle([x0, y0, x1, y1], outline=(0, 0, 0), width=2)
    d.text((x0 + 40, y0 - 28), title, fill=(0, 0, 0), font=FB)
    def sx(v): return x0 + v / xmax * (x1 - x0)
    def sy(v): return y1 - (v - ymin) / (ymax - ymin) * (y1 - y0)
    for k in range(6):
        gx = xmax * k / 5; d.line([sx(gx), y0, sx(gx), y1], fill=(232, 232, 232))
        d.text((sx(gx) - 10, y1 + 6), f"{gx:.0f}", fill=(0, 0, 0), font=FS)
        gy = ymin + (ymax - ymin) * k / 5; d.line([x0, sy(gy), x1, sy(gy)], fill=(232, 232, 232))
        d.text((x0 - 44, sy(gy) - 8), f"{gy:.0f}" if ymax > 10 else f"{gy:.2f}", fill=(0, 0, 0), font=FS)
    d.text(((x0 + x1) // 2 - 50, y1 + 30), "brightness (0-255)", fill=(0, 0, 0), font=F)
    im = Image.new("RGB", (len(ylab) * 11, 22), "white"); ImageDraw.Draw(im).text((0, 0), ylab, fill=(0, 0, 0), font=F)
    img.paste(im.rotate(90, expand=True), (x0 - 76, (y0 + y1) // 2 - len(ylab) * 5))
    return sx, sy

def line(sx, sy, xs, ys, col, dot=True):
    pts = [(sx(x), sy(y)) for x, y in zip(xs, ys)]
    for i in range(1, len(pts)): d.line([pts[i-1], pts[i]], fill=col, width=3)
    if dot:
        for px, py in pts: d.ellipse([px-4, py-4, px+4, py+4], fill=col)

# Panel A: current (blue) + PAR (green, right scale) vs brightness
sx, sy = axes(95, 70, 700, 560, 255, 0, 460, "Current vs Light", "LED draw (mA)")
line(sx, sy, br, led_mA, (30, 90, 200))
line(sx, sy, hb, hled, (0, 150, 0))
# PAR on a right scale (0-45 -> reuse 0-460 by *10)
def syp(v): return 560 - v / 45 * (560 - 70)
line(sx, syp, br, par, (210, 60, 40))
for v in (0, 9, 18, 27, 36, 45):
    d.line([700, syp(v), 706, syp(v)], fill=(210, 60, 40), width=2); d.text((710, syp(v)-8), f"{v}", fill=(210, 60, 40), font=FS)
d.text((715, 300), "PAR", fill=(210, 60, 40), font=F)
d.text((120, 90), "— LED draw (RGBW): flat-low, then jumps", fill=(30, 90, 200), font=FS)
d.text((120, 110), "— hex-direct draw (smooth, for contrast)", fill=(0, 150, 0), font=FS)
d.text((120, 130), "— PAR (light): rises smoothly the whole way", fill=(210, 60, 40), font=FS)

# Panel B: bv vs brightness with the Vf band
sx2, sy2 = axes(840, 70, 1430, 560, 255, 2.9, 3.45, "Rail sags into the LED's Vf zone", "battery_v (V)")
# shade Vf band
for yy in range(int(sy2(VF_HI)), int(sy2(VF_LO))):
    d.line([841, yy, 1429, yy], fill=(255, 235, 200))
d.text((1140, sy2(VF_HI) - 22), "LED Vf ~3.0-3.2V", fill=(200, 120, 0), font=FS)
line(sx2, sy2, br, bv, (120, 30, 160))
d.text((860, 90), "bv sags to ~3.11V at full -> no headroom above Vf -> starved", fill=(120, 30, 160), font=FS)

img.save("ops/bench/data/ca/rgbw-undervolt.png")
print("wrote ops/bench/data/ca/rgbw-undervolt.png")
