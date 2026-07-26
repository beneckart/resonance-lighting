#!/usr/bin/env python3
"""Generate the solarnoid cap-bank PCB, v1.0 (inline XH pass-through module).

Board: 88 x 40 mm, 2 layer. Changes from v0.6:
  - Ben's KiCad GUI pass folded back in: component nudges (cluster tightened,
    divider stack aligned, U1 +2mm), silkscreen breathing room, connector
    references renamed as labels (IN/OUT x2, RECVR, TELE-METRY), title split
    with a v1.0 stamp. All affected tracks rerouted to the new positions.
    Ben's hand-edited board is preserved at build/capbank_ben_edit.kicad_pcb.
  - Four M3 mounting holes for nylon standoffs (bare 3.2mm NPTH cutouts with
    silk rings; zip-tie slots retained). Bottom-left hole sits at (3.2,28.5)
    -- the true corner is occupied by the IN connector housing.
  - VDC rail riser moved from x=5 to x=6.8 to clear the top-left hole.

Port map (confusion-proof: each port a different family):
  IN/OUT XH 3p (the ONLY 3p XH -> unmistakable daisy chain): 1:D7 2:VDC 3:GND
  TELE-METRY XH 2p: 1:VSNS 2:D7S -> PowerFeather A4/A5 (reversal harmless)
  RECVR 1x7 female socket: RX480E dock, order GND 5V D0 D1 D2 D3 VT
    (verified vs product photos; beep one physical module on arrival)

Run:  python3 generate_capboard.py   then fill zones + DRC via kicad-cli
      (ZONE_FILLER.Fill segfaults headless in KiCad 10.0.3).
"""
import pcbnew
from pcbnew import VECTOR2I_MM, FromMM

LIB = "/usr/share/kicad/footprints/"
OUT = "build/capbank.kicad_pcb"
BXH = "JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical"

# ---- parameters -------------------------------------------------------------
PIN_ORDER = {"1": "D7", "2": "VDC", "3": "GND"}   # matches PowerFeather header
RX_ORDER = ["GND", "P5V", "BTNP", None, None, None, None]   # GND 5V D0 D1 D2 D3 VT
RX_LABELS = ["G", "5V", "D0", "D1", "D2", "D3", "VT"]
BOARD_W, BOARD_H = 88, 40
CAP_CENTERS = [20, 41, 62]
CAP_Y = 18.3
CAP_PITCH = 7.5
SPINE_Y = 26.5        # VDC spine (front)
D7_Y = 33.8           # D7 spine (back, under the connector row)
RAIL_X = 6.8          # VDC rail riser (clears the top-left mounting hole)
RAIL_Y = 1.3          # VDC rail to button cluster
LANE_Y = 9.5          # D7 lane in the button cluster
HOLES = [(3.2, 3.2), (84.8, 3.2), (84.8, 36.8), (3.2, 28.5)]   # M3 standoffs

board = pcbnew.CreateEmptyBoard()
board.GetDesignSettings().SetCopperLayerCount(2)
F, B = pcbnew.F_Cu, pcbnew.B_Cu

nets = {}
for name in ["VDC", "GND", "D7", "NETA", "NETB", "VSNS", "D7S", "P5V", "BTNP"]:
    n = pcbnew.NETINFO_ITEM(board, name)
    board.Add(n)
    nets[name] = n

def place(lib, name, ref, x, y, rot=0, value=None, dnp=False):
    fp = pcbnew.FootprintLoad(LIB + lib + ".pretty", name)
    fp.SetReference(ref)
    if value:
        fp.SetValue(value)
    fp.SetPosition(VECTOR2I_MM(x, y))
    try:
        fp.SetOrientationDegrees(rot)
    except AttributeError:
        fp.SetOrientation(pcbnew.EDA_ANGLE(rot, pcbnew.DEGREES_T))
    if dnp:
        try:
            fp.SetDNP(True)
        except AttributeError:
            pass
    board.Add(fp)
    return fp

def assign(fp, mapping):
    for pad in fp.Pads():
        net = mapping.get(pad.GetNumber())
        if net:
            pad.SetNet(nets[net])

def pxy(fp, num):
    for pad in fp.Pads():
        if pad.GetNumber() == num:
            return pad.GetPosition().x / 1e6, pad.GetPosition().y / 1e6
    raise KeyError(num)

def track(x1, y1, x2, y2, w, layer, net):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(VECTOR2I_MM(x1, y1))
    t.SetEnd(VECTOR2I_MM(x2, y2))
    t.SetWidth(FromMM(w))
    t.SetLayer(layer)
    t.SetNet(nets[net])
    board.Add(t)

def via(x, y, net):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(VECTOR2I_MM(x, y))
    v.SetDrill(FromMM(0.4))
    v.SetWidth(FromMM(0.8))
    v.SetNet(nets[net])
    board.Add(v)

def silk(text, x, y, size=1.2):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(text)
    t.SetPosition(VECTOR2I_MM(x, y))
    t.SetLayer(pcbnew.F_SilkS)
    t.SetTextSize(pcbnew.VECTOR2I(FromMM(size), FromMM(size)))
    t.SetTextThickness(FromMM(max(0.15, size * 0.15)))
    board.Add(t)

def edge_rect(x1, y1, x2, y2):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_RECT)
    s.SetStart(VECTOR2I_MM(x1, y1))
    s.SetEnd(VECTOR2I_MM(x2, y2))
    s.SetLayer(pcbnew.Edge_Cuts)
    s.SetWidth(FromMM(0.1))
    board.Add(s)

def circle(cx, cy, r, layer, w=0.1):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_CIRCLE)
    s.SetStart(VECTOR2I_MM(cx, cy))
    s.SetEnd(VECTOR2I_MM(cx + r, cy))
    s.SetLayer(layer)
    s.SetWidth(FromMM(w))
    board.Add(s)

# ---- outline, zip-tie slots, mounting holes ----------------------------------
edge_rect(0, 0, BOARD_W, BOARD_H)
for sx in [9.7, 30.5, 51.5, 72.3]:
    edge_rect(sx - 0.9, CAP_Y - 5, sx + 0.9, CAP_Y + 5)
for hx, hy in HOLES:
    circle(hx, hy, 1.6, pcbnew.Edge_Cuts)          # M3 clearance NPTH
    circle(hx, hy, 2.4, pcbnew.F_SilkS, 0.15)      # marker ring

# ---- connectors (bottom row) ------------------------------------------------
jy = 35.5
j1 = place("Connector_JST", BXH, "IN/OUT", 7.5, jy)
j2 = place("Connector_JST", BXH, "IN/OUT", 71.5, jy)
assign(j1, PIN_ORDER)
assign(j2, PIN_ORDER)
j3 = place("Connector_PinSocket_2.54mm", "PinSocket_1x07_P2.54mm_Vertical",
           "RECVR", 18.6, jy, rot=90, value="RX480E DOCK")
for pad in j3.Pads():
    net = RX_ORDER[int(pad.GetNumber()) - 1]
    if net:
        pad.SetNet(nets[net])
j4 = place("Connector_JST", "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", "METRY",
           56.8, jy, value="SENSE")
assign(j4, {"1": "VSNS", "2": "D7S"})

# ---- capacitors (THT, hand-solder) ------------------------------------------
bigcaps = []
for i, cx in enumerate(CAP_CENTERS):
    fp = place("Capacitor_THT", "CP_Radial_D18.0mm_P7.50mm", f"C{i+2}",
               cx - CAP_PITCH / 2, CAP_Y, value="22000u 16V")
    bigcaps.append(fp)
    for pad in fp.Pads():
        pad.SetShape(pcbnew.PAD_SHAPE_OVAL)
        pad.SetSize(VECTOR2I_MM(3.6, 2.6))
        pad.SetDrillShape(pcbnew.PAD_DRILL_SHAPE_OBLONG)
        pad.SetDrillSize(VECTOR2I_MM(1.9, 1.1))
    assign(fp, {"1": "VDC", "2": "GND"})

# ---- one-shot button cluster (positions per Ben's GUI pass) ------------------
sw1 = place("Button_Switch_SMD", "SW_Push_1TS009xxxx-xxxx-xxxx_6x6x5mm",
            "SW1", 16, 4)
assign(sw1, {"1": "VDC", "2": "NETA"})
r1 = place("Resistor_SMD", "R_0805_2012Metric", "R1", 31, 6.5, value="470R")
assign(r1, {"1": "NETA", "2": "NETB"})
c1 = place("Capacitor_SMD", "C_1206_3216Metric", "C1", 35.5, 6.5, value="10uF X7R 16V")
assign(c1, {"1": "NETB", "2": "D7"})
c1b = place("Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "C1B",
            37.5, 2.5, value="DNP", dnp=True)
assign(c1b, {"1": "NETB", "2": "D7"})
r2 = place("Resistor_SMD", "R_0805_2012Metric", "R2", 39.91, 6.4, value="330k")
assign(r2, {"1": "NETB", "2": "D7"})
r3 = place("Resistor_SMD", "R_0805_2012Metric", "R3", 47, 7.09, rot=90, value="10k")
assign(r3, {"1": "D7", "2": "GND"})   # after rot90, pad1 lands toward the lane
d1 = place("Package_TO_SOT_SMD", "SOT-23", "D1", 51, 7.0, rot=270,
           value="BZX84C3V3")
assign(d1, {"1": "GND", "3": "D7"})   # SOT-23 zener: 1=anode, 3=cathode

# ---- telemetry divider + gate sense (right of METRY) -------------------------
r4 = place("Resistor_SMD", "R_0805_2012Metric", "R4", 63.5, 30.12, rot=90,
           value="100k")
assign(r4, {"1": "VSNS", "2": "VDC"})     # pad1 lands low (VSNS), pad2 high
r5 = place("Resistor_SMD", "R_0805_2012Metric", "R5", 65.5, 30.12, rot=90,
           value="33k")
assign(r5, {"1": "GND", "2": "VSNS"})
c5 = place("Capacitor_SMD", "C_0805_2012Metric", "C5", 67.5, 30.12, rot=90,
           value="100n")
assign(c5, {"1": "GND", "2": "VSNS"})
r6 = place("Resistor_SMD", "R_0805_2012Metric", "R6", 54.8, 29.9, value="1k")
assign(r6, {"1": "D7", "2": "D7S"})

# ---- remote-dock 5V rail + D0 series protection -----------------------------
u1 = place("Package_TO_SOT_SMD", "SOT-223-3_TabPin2", "U1", 42.3, 31.7, rot=180,
           value="AMS1117-5.0")
assign(u1, {"1": "GND", "2": "P5V", "3": "VDC"})
c7 = place("Capacitor_SMD", "C_1206_3216Metric", "C7", 38.8, 37.4,
           value="10uF X7R 16V")
assign(c7, {"1": "P5V", "2": "GND"})
r7 = place("Resistor_SMD", "R_0805_2012Metric", "R7", 23.68, 31, rot=90, value="1k")
r7p = sorted(r7.Pads(), key=lambda q: q.GetPosition().y)
r7p[0].SetNet(nets["NETA"]); r7p[1].SetNet(nets["BTNP"])

# ---- strip stray 'REF**' silk texts some library footprints ship -------------
for fp in board.GetFootprints():
    for it in [g for g in fp.GraphicalItems()
               if hasattr(g, "GetText") and g.GetText().startswith("REF**")
               and g.GetLayer() in (pcbnew.F_SilkS, pcbnew.B_SilkS)]:
        fp.Remove(it)

# ---- reference-label placement (Ben's GUI pass, captions only) ---------------
REF_STYLE = [
    (sw1, 21.00, 1.44, 0),   (r1, 31.00, 4.85, 0),   (c1, 35.50, 4.65, 0),
    (c1b, 40.00, 2.50, 0),   (r2, 39.91, 4.75, 0),   (r3, 45.35, 7.09, 90),
    (d1, 53.40, 7.00, 270),  (r4, 63.35, 33.12, 90), (r5, 65.35, 33.12, 90),
    (c5, 67.32, 33.12, 90),  (r6, 54.80, 28.25, 0),  (r7, 22.03, 31.00, 90),
    (u1, 42.30, 36.20, 180), (c7, 38.80, 35.55, 0),
    (j1, 10.00, 30.45, 0),   (j2, 74.00, 30.45, 0),
    (j3, 31.00, 31.50, 0),   (j4, 51.00, 36.95, 0),
    (bigcaps[0], 20.00, 8.05, 0), (bigcaps[1], 41.00, 8.05, 0),
    (bigcaps[2], 62.00, 8.05, 0),
]
for fp, rx, ry, rang in REF_STYLE:
    ref = fp.Reference()
    ref.SetPosition(VECTOR2I_MM(rx, ry))
    try:
        ref.SetTextAngleDegrees(rang)
    except AttributeError:
        ref.SetTextAngle(pcbnew.EDA_ANGLE(rang, pcbnew.DEGREES_T))

# ---- routing ----------------------------------------------------------------
j1d, j1v, j1g = pxy(j1, "1"), pxy(j1, "2"), pxy(j1, "3")
j2d, j2v, j2g = pxy(j2, "1"), pxy(j2, "2"), pxy(j2, "3")

# VDC (front)
track(*j1v, j1v[0], SPINE_Y, 1.3, F, "VDC")
track(*j2v, j2v[0], SPINE_Y, 1.3, F, "VDC")
track(RAIL_X, SPINE_Y, j2v[0], SPINE_Y, 2.5, F, "VDC")
for cx in CAP_CENTERS:
    track(cx - CAP_PITCH / 2, CAP_Y, cx - CAP_PITCH / 2, SPINE_Y, 2.5, F, "VDC")
track(RAIL_X, SPINE_Y, RAIL_X, RAIL_Y, 0.8, F, "VDC")
sw_vdc = pxy(sw1, "1")
track(RAIL_X, RAIL_Y, sw_vdc[0], RAIL_Y, 0.8, F, "VDC")
track(sw_vdc[0], RAIL_Y, *sw_vdc, 0.6, F, "VDC")

# D7 pass-through: THT pins reach the back layer directly, spine on back
track(*j1d, j1d[0], D7_Y, 0.8, B, "D7")
track(*j2d, j2d[0], D7_Y, 0.8, B, "D7")
track(j1d[0], D7_Y, j2d[0], D7_Y, 0.8, B, "D7")
# (connector GND pins are THT -- the back pour picks them up, no tracks)

# RECVR dock: D0 pin -> R7 -> NETA; front column crosses the back D7 spine,
# via above the VDC spine, then a back column up to the cluster row (y=6.5)
d0 = pxy(j3, "3")
track(*d0, d0[0], r7p[1].GetPosition().y / 1e6, 0.5, F, "BTNP")
track(d0[0], r7p[0].GetPosition().y / 1e6, d0[0], 28.5, 0.5, F, "NETA")
via(d0[0], 28.5, "NETA")
track(d0[0], 28.5, 20, 27, 0.5, B, "NETA")
track(20, 27, 20, 6.5, 0.5, B, "NETA")
via(20, 6.5, "NETA")

# AMS1117: VIN from spine, GND to pour, VOUT (tab) -> C7.1 -> socket 5V pin
u1vin, u1gnd, u1out = pxy(u1, "3"), pxy(u1, "1"), (42.3 - 3.15, 31.7)
track(42.3 + 3.15, 31.7, *u1out, 0.5, F, "P5V")   # pin-side pad 2 to tab
track(*u1vin, u1vin[0], SPINE_Y, 0.5, F, "VDC")
track(*u1gnd, u1gnd[0], 35.4, 0.5, F, "GND")
via(u1gnd[0], 35.4, "GND")
c7v, c7g = pxy(c7, "1"), pxy(c7, "2")
track(*u1out, c7v[0], 33.5, 0.5, F, "P5V")        # tab down through C7.1
track(c7v[0], 33.5, *c7v, 0.5, F, "P5V")
track(*c7v, c7v[0], 38.7, 0.5, F, "P5V")
p5 = pxy(j3, "2")
track(c7v[0], 38.7, p5[0], 38.7, 0.5, F, "P5V")
track(p5[0], 38.7, *p5, 0.5, F, "P5V")
track(*c7g, c7g[0], 36.5, 0.4, F, "GND")          # C7.2
via(c7g[0], 36.5, "GND")

# one-shot cluster (row at y=6.5 per Ben's layout)
sw_a = pxy(sw1, "2")
track(*sw_a, sw_a[0], 6.5, 0.6, F, "NETA")
r1a, r1b = pxy(r1, "1"), pxy(r1, "2")
track(sw_a[0], 6.5, *r1a, 0.6, F, "NETA")
c1a, c1k = pxy(c1, "1"), pxy(c1, "2")
track(*r1b, *c1a, 0.6, F, "NETB")                 # R1.2 -> C1.1
r2a, r2k = pxy(r2, "1"), pxy(r2, "2")
track(*c1a, c1a[0], 4.5, 0.4, F, "NETB")          # NETB detour over C1 body
track(c1a[0], 4.5, r2a[0], 4.5, 0.4, F, "NETB")
track(r2a[0], 4.5, *r2a, 0.4, F, "NETB")
track(37.5, 2.5, 37.5, 4.5, 0.4, F, "NETB")       # C1B.1
track(*c1k, 38.5, LANE_Y, 0.6, F, "D7")           # C1.2 -> lane
track(38.5, LANE_Y, 54, LANE_Y, 0.6, F, "D7")
track(*r2k, r2k[0], LANE_Y, 0.4, F, "D7")         # R2.2 -> lane
track(42.5, 2.5, 42.5, LANE_Y, 0.4, F, "D7")      # C1B.2 -> lane
r3d, r3g = pxy(r3, "1"), pxy(r3, "2")
track(*r3d, r3d[0], LANE_Y, 0.4, F, "D7")         # R3.1 -> lane
track(*r3g, r3g[0], 4.9, 0.4, F, "GND")           # R3.2
via(r3g[0], 4.9, "GND")
d1a = pxy(d1, "1")
track(*d1a, d1a[0], 4.9, 0.4, F, "GND")
track(d1a[0], 4.9, r3g[0], 4.9, 0.4, F, "GND")
d1k = pxy(d1, "3")
track(*d1k, d1k[0], LANE_Y, 0.4, F, "D7")
via(54, LANE_Y, "D7")                             # lane -> back spine
track(54, LANE_Y, 54, D7_Y, 0.8, B, "D7")

# telemetry (aligned stack right of METRY, zigzag between offset net rows)
r4s, r4v = pxy(r4, "1"), pxy(r4, "2")
r5g, r5s = pxy(r5, "1"), pxy(r5, "2")
c5g, c5s = pxy(c5, "1"), pxy(c5, "2")
track(*r4v, r4v[0], SPINE_Y, 0.5, F, "VDC")       # R4.2 tap
track(*r4s, *r5s, 0.5, F, "VSNS")                 # zigzag R4.1 -> R5.2
track(*r5s, *c5s, 0.4, F, "VSNS")
track(*r5g, *c5g, 0.4, F, "GND")
track(66.5, r5g[1], 66.5, 32.2, 0.4, F, "GND")
via(66.5, 32.2, "GND")
j4v_, j4s = pxy(j4, "1"), pxy(j4, "2")
track(*r4s, r4s[0], 37.6, 0.5, F, "VSNS")         # around the pads
track(r4s[0], 37.6, j4v_[0], 37.6, 0.5, F, "VSNS")
track(j4v_[0], 37.6, *j4v_, 0.5, F, "VSNS")
track(53.89, 29.9, 53.89, D7_Y, 0.4, F, "D7")     # R6.1 tap
via(53.89, D7_Y, "D7")
track(55.71, 29.9, j4s[0], 31.8, 0.5, F, "D7S")   # R6.2 -> METRY.2
track(j4s[0], 31.8, *j4s, 0.5, F, "D7S")

# ---- back copper: GND pour --------------------------------------------------
z = pcbnew.ZONE(board)
z.SetLayer(B)
z.SetNetCode(nets["GND"].GetNetCode())
z.Outline().NewOutline()
for x, y in [(0.5, 0.5), (BOARD_W - 0.5, 0.5),
             (BOARD_W - 0.5, BOARD_H - 0.5), (0.5, BOARD_H - 0.5)]:
    z.Outline().Append(FromMM(x), FromMM(y))
z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
z.SetThermalReliefSpokeWidth(FromMM(1.0))
z.SetThermalReliefGap(FromMM(0.5))
z.SetMinThickness(FromMM(0.25))
z.SetLocalClearance(FromMM(0.3))
board.Add(z)
# zones filled afterwards: kicad-cli pcb drc --refill-zones --save-board

# ---- silkscreen (caption positions per Ben's GUI pass) ------------------------
def silk_bitmap(path, cx, cy, height_mm, layer, mirror=False, pitch=0.15,
                rotate=0, autocrop=False):
    """Render a black-on-white bitmap as run-length silkscreen rectangles."""
    import os
    if not os.path.exists(path):
        print(f"note: {path} missing, skipping badge")
        return
    from PIL import Image, ImageOps
    im = Image.open(path)
    if im.mode in ("RGBA", "LA", "P"):
        bg = Image.new("RGB", im.size, "white")
        bg.paste(im, mask=im.convert("RGBA").split()[-1])
        im = bg
    im = im.convert("L")
    if autocrop:
        im = im.crop(ImageOps.invert(im).getbbox())
    if rotate:
        im = im.rotate(rotate, expand=True, fillcolor=255)
    rows = int(round(height_mm / pitch))
    cols = int(round(im.size[0] / im.size[1] * rows))
    im = im.resize((cols, rows))
    px = im.load()
    x0, y0 = cx - cols * pitch / 2, cy - rows * pitch / 2
    n = 0
    for r in range(rows):
        c = 0
        while c < cols:
            if px[c, r] < 128:
                c2 = c
                while c2 < cols and px[c2, r] < 128:
                    c2 += 1
                xa, xb = x0 + c * pitch, x0 + c2 * pitch
                if mirror:
                    xa, xb = 2 * cx - xa, 2 * cx - xb
                s = pcbnew.PCB_SHAPE(board)
                s.SetShape(pcbnew.SHAPE_T_RECT)
                s.SetStart(VECTOR2I_MM(min(xa, xb), y0 + r * pitch))
                s.SetEnd(VECTOR2I_MM(max(xa, xb), y0 + (r + 1) * pitch))
                s.SetFilled(True)
                s.SetWidth(0)
                s.SetLayer(layer)
                board.Add(s)
                n += 1
                c = c2
            else:
                c += 1
    print(f"badge {path} -> {n} silk rects on {'B' if mirror else 'F'}")

silk_bitmap("logo_shell.png", 58, 4.7, 7.6, pcbnew.F_SilkS)
silk_bitmap("logo_shell.png", 44, 20, 37, pcbnew.B_SilkS, mirror=True,
            pitch=0.2, rotate=90, autocrop=True)
silk("SOLARNOID CAPBANK", 70, 3.2, 1.2)
silk("v1.0", 74, 5.1, 0.8)
silk("TAP=1 KNOCK", 11.9, 8.2, 0.9)
silk("D7 VDC GND", 10, 31.9, 0.65)
silk("D7 VDC GND", 74, 31.9, 0.65)
for i, lbl in enumerate(RX_LABELS):
    silk(lbl, 18.6 + 2.54 * i, 33.2, 0.55)
silk("VSNS D7S", 58, 32.1, 0.6)
silk("TELE-", 51, 35.5, 1.0)

pcbnew.SaveBoard(OUT, board)
print("wrote", OUT)
