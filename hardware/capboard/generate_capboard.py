#!/usr/bin/env python3
"""Generate the solarnoid cap-bank PCB, v1.1 (boosted strike rail).

Board: 88 x 40 mm, 2 layer. Changes from v1.0:
  - ON-BOARD BOOST: MT3608 takes raw panel VDC on PANEL and feeds the cap bank
    + DRIVER at ~11.4 V (180k/10k feedback). R12 is a THT DNP trim across the
    lower leg to raise it deliberately. JP1 bypasses the boost if depopulated.
  - PANEL and DRIVER are NO LONGER the same net (PANEL = raw, DRIVER = boosted).
  - EN divider 47k/47k off VDC_IN -> boost self-enables with nothing plugged in;
    a GPIO on TELE pin 2 pulls it low to disable. GPIO never sees >3.0 V.
  - AMS1117 -> HT7550-1 (uA-class Iq; the 8 mA parasitic is gone).
  - IN/OUT -> S3B-XH-SM4-TB right-angle SMT; RX dock -> SMD socket. Only THT
    parts left are the 3 electrolytics, TELE, C1B and R12.
  - D7S/R6 deleted. TELE is 2p THT: 1=VSNS (A4), 2=BOOST_EN (A5).
  - VSNS divider 47k/6.8k + 1nF (v1.0's 100k/33k + 100nF was a 64 Hz low-pass
    that would have erased the 25 ms transient it exists to capture).

Previous (v1.0) notes:
  - Ben's KiCad GUI pass folded back in: component nudges (cluster tightened,
    divider stack aligned, U1 +2mm), silkscreen breathing room, connector
    references renamed as labels (IN/OUT x2, RECVR, TELE-METRY), title split
    with a v1.0 stamp. All affected tracks rerouted to the new positions.
    Ben's hand-edited board is preserved at build/capbank_ben_edit.kicad_pcb.
  - Four M3 mounting holes for nylon standoffs (bare 3.2mm NPTH cutouts with
    silk rings; zip-tie slots retained), one in each true corner: the left
    connector group (IN + RECVR dock) sits +3mm right to free the bottom-left
    corner; U1/C7 nudged right to preserve courtyard gaps.
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
SM4 = "JST_XH_S3B-XH-SM4-TB_1x03-1MP_P2.50mm_Horizontal"
B2XH = "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical"

# ---- parameters -------------------------------------------------------------
PIN_ORDER = {"1": "D7", "2": "VDC", "3": "GND"}   # matches PowerFeather header
RX_ORDER = ["GND", "P5V", "BTNP", None, None, None, None]   # GND 5V D0 D1 D2 D3 VT
RX_LABELS = ["G", "5V", "D0", "D1", "D2", "D3", "VT"]
BOARD_W, BOARD_H = 88, 40
CAP_CENTERS = [20, 41, 62]
CAP_Y = 18.3
CAP_PITCH = 7.5
SPINE_Y = 26.5        # VBOOST spine (front)
D7_Y = 33.8           # D7 spine (back, under the connector row)
RAIL_X = 6.8          # VDC rail riser (clears the top-left mounting hole)
RAIL_Y = 1.3          # VDC rail to button cluster
LANE_Y = 9.5          # D7 lane in the button cluster
HOLES = [(3.2, 3.2), (84.8, 3.2), (84.8, 36.8), (3.2, 36.8)]   # M3 standoffs

board = pcbnew.CreateEmptyBoard()
board.GetDesignSettings().SetCopperLayerCount(2)
F, B = pcbnew.F_Cu, pcbnew.B_Cu

nets = {}
for name in ["VDCIN", "VBOOST", "GND", "D7", "NETA", "NETB", "VSNS",
             "P5V", "BTNP", "SW", "FB", "EN"]:
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

# ---- outline + mounting holes (zip-tie slots dropped in v1.1) ----------------------------------
edge_rect(0, 0, BOARD_W, BOARD_H)
for hx, hy in HOLES:
    circle(hx, hy, 1.6, pcbnew.Edge_Cuts)          # M3 clearance NPTH
    circle(hx, hy, 2.4, pcbnew.F_SilkS, 0.15)      # marker ring

# ---- connectors (bottom row) ------------------------------------------------
jy = 34.0          # SM4 right-angle: signal pads -3.25, MP anchors +3.55
j1 = place("Connector_JST", SM4, "PANEL", 12, jy, value="RAW VDC IN")
assign(j1, {"1": "D7", "2": "VDCIN", "3": "GND"})
j2 = place("Connector_JST", SM4, "DRIVER", 76, jy, value="11.4V OUT")
assign(j2, {"1": "D7", "2": "VBOOST", "3": "GND"})
j3 = place("Connector_PinSocket_2.54mm",
           "PinSocket_1x07_P2.54mm_Vertical_SMD_Pin1Left",
           "RECVR", 32, jy, rot=90, value="RX480E DOCK")
for pad in j3.Pads():
    net = RX_ORDER[int(pad.GetNumber()) - 1]
    if net:
        pad.SetNet(nets[net])
j4 = place("Connector_JST", B2XH, "TELE", 56, 35.5, value="VSNS/EN")
assign(j4, {"1": "VSNS", "2": "EN"})

# ---- capacitors (THT, hand-solder) ------------------------------------------
bigcaps = []
for i, cx in enumerate(CAP_CENTERS):
    fp = place("Capacitor_THT", "CP_Radial_D18.0mm_P7.50mm", f"C{i+2}",
               cx - CAP_PITCH / 2, CAP_Y, value="22000u 16V")
    # True-height 35.5mm can (the library default for this footprint is a 20mm
    # model). Rebuild the list: mutating fp.Models() entries in place is a
    # no-op because the iterator hands back copies.
    tall = pcbnew.FP_3DMODEL()
    tall.m_Filename = ("${KICAD10_3DMODEL_DIR}/Capacitor_THT.3dshapes/"
                       "C_Radial_D18.0mm_H35.5mm_P7.50mm.step")
    tall.m_Scale.x = tall.m_Scale.y = tall.m_Scale.z = 1.0
    fp.Models().clear()
    fp.Models().push_back(tall)
    bigcaps.append(fp)
    for pad in fp.Pads():
        pad.SetShape(pcbnew.PAD_SHAPE_OVAL)
        pad.SetSize(VECTOR2I_MM(3.6, 2.6))
        pad.SetDrillShape(pcbnew.PAD_DRILL_SHAPE_OBLONG)
        pad.SetDrillSize(VECTOR2I_MM(1.9, 1.1))
    assign(fp, {"1": "VBOOST", "2": "GND"})

# ---- one-shot button cluster (positions per Ben's GUI pass) ------------------
sw1 = place("Button_Switch_SMD", "SW_Push_1TS009xxxx-xxxx-xxxx_6x6x5mm",
            "SW1", 16, 4)
assign(sw1, {"1": "VDCIN", "2": "NETA"})
r1 = place("Resistor_SMD", "R_0805_2012Metric", "R1", 31, 6.5, value="470R")
assign(r1, {"1": "NETA", "2": "NETB"})
c1 = place("Capacitor_SMD", "C_1206_3216Metric", "C1", 35.5, 6.5, value="10uF X7R 16V")
assign(c1, {"1": "NETB", "2": "D7"})
c1b = place("Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "C1B",
            37.5, 3.2, value="DNP", dnp=True)
assign(c1b, {"1": "NETB", "2": "D7"})
r2 = place("Resistor_SMD", "R_0805_2012Metric", "R2", 39.91, 6.4, value="330k")
assign(r2, {"1": "NETB", "2": "D7"})
r3 = place("Resistor_SMD", "R_0805_2012Metric", "R3", 47, 7.09, rot=90, value="10k")
assign(r3, {"1": "D7", "2": "GND"})   # after rot90, pad1 lands toward the lane
d1 = place("Package_TO_SOT_SMD", "SOT-23", "D1", 51, 7.0, rot=270,
           value="BZX84C3V3")
assign(d1, {"1": "GND", "3": "D7"})   # SOT-23 zener: 1=anode, 3=cathode

# ---- telemetry divider + gate sense (right of METRY) -------------------------
r4 = place("Resistor_SMD", "R_0805_2012Metric", "R4", 62, 30.8, rot=90,
           value="47k")
assign(r4, {"1": "VSNS", "2": "VBOOST"})   # pad1 lands low (VSNS), pad2 high
r5 = place("Resistor_SMD", "R_0805_2012Metric", "R5", 64, 30.8, rot=90,
           value="6.8k")
assign(r5, {"1": "GND", "2": "VSNS"})
c5 = place("Capacitor_SMD", "C_0805_2012Metric", "C5", 66, 30.8, rot=90,
           value="1n")
assign(c5, {"1": "GND", "2": "VSNS"})

# ---- remote-dock 5V rail + D0 series protection -----------------------------
u1 = place("Package_TO_SOT_SMD", "SOT-89-3", "U1", 49.5, 30.8, rot=180,
           value="HT7550-1")
assign(u1, {"1": "GND", "2": "P5V", "3": "VBOOST"})   # SOT-89: 1=GND 2=OUT 3=IN
c7 = place("Capacitor_SMD", "C_1206_3216Metric", "C7", 42.5, 30.8,
           value="10uF X7R 16V")
assign(c7, {"1": "GND", "2": "P5V"})

# ---- boost: MT3608, raw VDCIN -> ~11.4V VBOOST (top strip, linear) ----------
r7 = place("Resistor_SMD", "R_0805_2012Metric", "R7", 26.74, 28, rot=90,
           value="1k")
r7p = sorted(r7.Pads(), key=lambda q: q.GetPosition().y)
r7p[0].SetNet(nets["NETA"]); r7p[1].SetNet(nets["BTNP"])

c8 = place("Capacitor_SMD", "C_1206_3216Metric", "C8", 75.5, 8, rot=90,
           value="22uF 16V")
assign(c8, {"1": "VDCIN", "2": "GND"})
l1 = place("Inductor_SMD", "L_Bourns_SRN6045TA", "L1", 82, 8.6, value="22uH 2.4A")
assign(l1, {"1": "VDCIN", "2": "SW"})
u2 = place("Package_TO_SOT_SMD", "SOT-23-6", "U2", 76, 14, value="MT3608")
assign(u2, {"1": "SW", "2": "GND", "3": "FB", "4": "EN", "5": "VDCIN", "6": "SW"})
d2 = place("Diode_SMD", "D_SMA", "D2", 83, 14, value="SS34")
assign(d2, {"1": "VBOOST", "2": "SW"})
c9 = place("Capacitor_SMD", "C_1206_3216Metric", "C9", 76, 18.4, rot=90,
           value="10uF 25V")
assign(c9, {"1": "VBOOST", "2": "GND"})
r9 = place("Resistor_SMD", "R_0805_2012Metric", "R9", 81, 18.4, value="10k")
assign(r9, {"1": "GND", "2": "FB"})
r8 = place("Resistor_SMD", "R_0805_2012Metric", "R8", 85.2, 18.4, value="180k")
assign(r8, {"1": "FB", "2": "VBOOST"})
r11 = place("Resistor_SMD", "R_0805_2012Metric", "R11", 81, 21.4, value="47k")
assign(r11, {"1": "GND", "2": "EN"})
r10 = place("Resistor_SMD", "R_0805_2012Metric", "R10", 85.2, 21.4, value="47k")
assign(r10, {"1": "EN", "2": "VDCIN"})
r12 = place("Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "R12",
            74.5, 23.8, value="DNP TRIM", dnp=True)
assign(r12, {"1": "FB", "2": "GND"})
jp1 = place("Resistor_SMD", "R_0805_2012Metric", "JP1", 84, 23.8,
            value="BYPASS DNP", dnp=True)
assign(jp1, {"1": "VDCIN", "2": "VBOOST"})

# ---- routing ----------------------------------------------------------------
# Layer lanes (front unless noted):  y=26.5 VBOOST spine | back y=28.0 EN |
# back y=29.5 VDCIN | y=30.75 SM4 signal pads | back y=33.8 D7 spine
j1d, j1v, j1g = pxy(j1, "1"), pxy(j1, "2"), pxy(j1, "3")
j2d, j2v, j2g = pxy(j2, "1"), pxy(j2, "2"), pxy(j2, "3")
VIA_Y = 32.0

# --- boost switching loop (right region) ---
c8v, c8g = pxy(c8, "1"), pxy(c8, "2")
l1a, l1b = pxy(l1, "1"), pxy(l1, "2")
u2sw1, u2g, u2fb = pxy(u2, "1"), pxy(u2, "2"), pxy(u2, "3")
u2en, u2vin, u2sw6 = pxy(u2, "4"), pxy(u2, "5"), pxy(u2, "6")
d2k, d2a = pxy(d2, "1"), pxy(d2, "2")
c9v, c9g = pxy(c9, "1"), pxy(c9, "2")

track(*c8v, *l1a, 0.9, F, "VDCIN")
track(*l1b, l1b[0], 11.8, 0.9, F, "SW")
track(l1b[0], 11.8, u2sw6[0], 11.8, 0.9, F, "SW")
track(u2sw6[0], 11.8, *u2sw6, 0.9, F, "SW")
track(*u2sw1, u2sw1[0], 11.8, 0.6, F, "SW")
track(u2sw1[0], 11.8, l1b[0], 11.8, 0.6, F, "SW")
track(*u2sw6, *d2a, 0.9, F, "SW")
track(*d2k, d2k[0], 16.6, 1.0, F, "VBOOST")
track(d2k[0], 16.6, c9v[0], 16.6, 1.0, F, "VBOOST")
track(c9v[0], 16.6, *c9v, 1.0, F, "VBOOST")
track(*u2vin, u2vin[0], 11.0, 0.5, F, "VDCIN")
via(u2vin[0], 11.0, "VDCIN")
track(u2vin[0], 11.0, c8v[0], 11.0, 0.5, B, "VDCIN")
track(c8v[0], 11.0, c8v[0], 5.6, 0.5, B, "VDCIN")
via(c8v[0], 5.6, "VDCIN")
track(c8v[0], 5.6, *c8v, 0.5, F, "VDCIN")
track(*u2g, 73.0, 14.0, 0.5, F, "GND")
via(73.0, 14.0, "GND")
track(*c8g, 73.0, 9.8, 0.5, F, "GND")
via(73.0, 9.8, "GND")
track(*c9g, 73.0, 20.2, 0.5, F, "GND")
via(73.0, 20.2, "GND")

# --- feedback divider + THT trim ---
r8f, r8h = pxy(r8, "1"), pxy(r8, "2")
r9g, r9f = pxy(r9, "1"), pxy(r9, "2")
track(*u2fb, u2fb[0], 15.8, 0.4, F, "FB")
track(u2fb[0], 15.8, 83.1, 15.8, 0.4, F, "FB")
track(83.1, 15.8, 83.1, 18.4, 0.4, F, "FB")
track(83.1, 18.4, *r9f, 0.4, F, "FB")
track(*r9f, *r8f, 0.4, F, "FB")
track(*r8h, 87.0, 18.4, 0.4, F, "VBOOST")
track(87.0, 18.4, 87.0, 15.0, 0.4, F, "VBOOST")
track(87.0, 15.0, d2k[0], 15.0, 0.4, F, "VBOOST")
track(d2k[0], 15.0, *d2k, 0.4, F, "VBOOST")
track(*r9g, 78.8, 18.4, 0.4, F, "GND")
via(78.8, 18.4, "GND")
track(83.1, 18.4, 83.1, 23.8, 0.4, F, "FB")
track(83.1, 23.8, 79.5, 23.8, 0.4, F, "FB")

# --- EN divider ---
r11g, r11e = pxy(r11, "1"), pxy(r11, "2")
r10e, r10v = pxy(r10, "1"), pxy(r10, "2")
track(*u2en, u2en[0], 21.4, 0.4, F, "EN")
via(u2en[0], 21.4, "EN")
track(u2en[0], 21.4, 79.8, 21.4, 0.4, B, "EN")
via(79.8, 21.4, "EN")
track(79.8, 21.4, *r11e, 0.4, F, "EN")
track(*r11e, *r10e, 0.4, F, "EN")
track(*r11g, 78.8, 21.4, 0.4, F, "GND")
via(78.8, 21.4, "GND")
track(*r10v, 87.0, 21.4, 0.4, F, "VDCIN")
track(87.0, 21.4, 87.0, 23.8, 0.4, F, "VDCIN")
track(u2en[0], 21.4, 70.0, 25.5, 0.4, B, "EN")
track(70.0, 25.5, 57.0, 29.5, 0.4, B, "EN")
via(57.0, 29.5, "EN")
j4v_, j4e = pxy(j4, "1"), pxy(j4, "2")
track(57.0, 29.5, *j4e, 0.4, F, "EN")

# --- JP1 bypass (DNP) ---
jp1v, jp1b_ = pxy(jp1, "1"), pxy(jp1, "2")
track(*jp1v, 87.0, 23.8, 0.6, F, "VDCIN")
track(*jp1b_, 82.0, 23.8, 0.6, F, "VBOOST")
via(82.0, 23.8, "VBOOST")

# --- VBOOST spine ---
track(*c9v, c9v[0], SPINE_Y, 1.6, F, "VBOOST")
track(82.0, 23.8, 82.0, SPINE_Y, 0.8, B, "VBOOST")
via(82.0, SPINE_Y, "VBOOST")
track(RAIL_X + 8, SPINE_Y, 82.0, SPINE_Y, 2.5, F, "VBOOST")
for cx in CAP_CENTERS:
    track(cx - CAP_PITCH / 2, CAP_Y, cx - CAP_PITCH / 2, SPINE_Y, 2.5, F, "VBOOST")
track(*j2v, j2v[0], SPINE_Y, 1.6, F, "VBOOST")
u1vin, u1gnd, u1out = pxy(u1, "3"), pxy(u1, "1"), pxy(u1, "2")
track(*u1vin, u1vin[0], SPINE_Y, 0.5, F, "VBOOST")
r4s, r4v = pxy(r4, "1"), pxy(r4, "2")
track(*r4v, r4v[0], SPINE_Y, 0.4, F, "VBOOST")

# --- VDCIN: PANEL -> left margin -> top rail (all front) ---
sw_vdc = pxy(sw1, "1")
track(*j1v, 7.0, 29.0, 0.8, F, "VDCIN")
track(7.0, 29.0, 7.0, RAIL_Y, 0.8, F, "VDCIN")
track(7.0, RAIL_Y, c8v[0], RAIL_Y, 0.8, F, "VDCIN")
track(sw_vdc[0], RAIL_Y, *sw_vdc, 0.6, F, "VDCIN")

# --- D7 pass-through (back spine) ---
for d in (j1d, j2d):
    track(*d, d[0], VIA_Y, 0.6, F, "D7")
    via(d[0], VIA_Y, "D7")
    track(d[0], VIA_Y, d[0], D7_Y, 0.8, B, "D7")
track(j1d[0], D7_Y, j2d[0], D7_Y, 0.8, B, "D7")
for g in (j1g, j2g):
    track(*g, g[0], VIA_Y, 0.8, F, "GND")
    via(g[0], VIA_Y, "GND")

# --- RECVR dock: D0 -> R7 -> NETA -> one-shot ---
d0 = pxy(j3, "3")
track(*d0, d0[0], 29.0, 0.5, F, "BTNP")
track(d0[0], 29.0, r7p[1].GetPosition().x / 1e6, r7p[1].GetPosition().y / 1e6, 0.5, F, "BTNP")
track(r7p[0].GetPosition().x / 1e6, r7p[0].GetPosition().y / 1e6, 20.0, 24.0, 0.5, F, "NETA")
via(20.0, 24.0, "NETA")
track(20.0, 24.0, 20.0, 6.5, 0.5, B, "NETA")
via(20.0, 6.5, "NETA")

# --- HT7550: VBOOST in, 5V out to the dock ---
track(*u1gnd, u1gnd[0], 32.4, 0.5, F, "GND")
via(u1gnd[0], 32.4, "GND")
c7v, c7g = pxy(c7, "1"), pxy(c7, "2")
track(*u1out, *c7v, 0.5, F, "P5V")
p5 = pxy(j3, "2")
track(*c7v, c7v[0], 34.6, 0.5, F, "P5V")
track(c7v[0], 34.6, p5[0], 34.6, 0.5, F, "P5V")
track(p5[0], 34.6, *p5, 0.5, F, "P5V")
track(*c7g, c7g[0], 28.0, 0.4, F, "GND")
via(c7g[0], 28.0, "GND")

# --- one-shot cluster (row at y=6.5) ---
sw_a = pxy(sw1, "2")
track(*sw_a, sw_a[0], 6.5, 0.6, F, "NETA")
r1a, r1b = pxy(r1, "1"), pxy(r1, "2")
track(sw_a[0], 6.5, *r1a, 0.6, F, "NETA")
c1a, c1k = pxy(c1, "1"), pxy(c1, "2")
track(*r1b, *c1a, 0.6, F, "NETB")
r2a, r2k = pxy(r2, "1"), pxy(r2, "2")
track(*c1a, c1a[0], 4.5, 0.4, F, "NETB")
track(c1a[0], 4.5, r2a[0], 4.5, 0.4, F, "NETB")
track(r2a[0], 4.5, *r2a, 0.4, F, "NETB")
track(37.5, 3.2, 37.5, 4.5, 0.4, F, "NETB")
track(*c1k, 38.5, LANE_Y, 0.6, F, "D7")
track(38.5, LANE_Y, 51.0, LANE_Y, 0.6, F, "D7")
track(*r2k, r2k[0], LANE_Y, 0.4, F, "D7")
track(42.5, 3.2, 42.5, LANE_Y, 0.4, F, "D7")
r3d, r3g = pxy(r3, "1"), pxy(r3, "2")
track(*r3d, r3d[0], LANE_Y, 0.4, F, "D7")
track(*r3g, r3g[0], 4.9, 0.4, F, "GND")
via(r3g[0], 4.9, "GND")
d1a = pxy(d1, "1")
track(*d1a, d1a[0], 4.9, 0.4, F, "GND")
track(d1a[0], 4.9, r3g[0], 4.9, 0.4, F, "GND")
d1k = pxy(d1, "3")
track(*d1k, d1k[0], LANE_Y, 0.4, F, "D7")
via(48.0, LANE_Y, "D7")
track(48.0, LANE_Y, 48.0, 24.0, 0.8, B, "D7")
track(48.0, 24.0, 40.0, D7_Y, 0.8, B, "D7")

# --- VSNS divider -> TELE pin 1 ---
r5g, r5s = pxy(r5, "1"), pxy(r5, "2")
c5g, c5s = pxy(c5, "1"), pxy(c5, "2")
track(*r4s, *r5s, 0.4, F, "VSNS")
track(*r5s, *c5s, 0.4, F, "VSNS")
track(*r5g, *c5g, 0.4, F, "GND")
track(c5g[0], c5g[1], 68.5, 32.2, 0.4, F, "GND")
via(68.5, 32.2, "GND")
track(*r4s, r4s[0], 37.8, 0.4, F, "VSNS")
track(r4s[0], 37.8, j4v_[0], 37.8, 0.4, F, "VSNS")
track(j4v_[0], 37.8, *j4v_, 0.4, F, "VSNS")

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
silk("D7 VDC GND", 13, 31.9, 0.65)
silk("D7 VDC GND", 74, 31.9, 0.65)
for i, lbl in enumerate(RX_LABELS):
    silk(lbl, 21.6 + 2.54 * i, 33.2, 0.55)
silk("VSNS D7S", 58, 32.1, 0.6)
silk("TELE-", 51, 35.5, 1.0)

pcbnew.SaveBoard(OUT, board)
print("wrote", OUT)
