#!/usr/bin/env python3
"""Generate the solarnoid cap-bank PCB, v0.2 (inline XH pass-through module).

Board: 88 x 40 mm, 2 layer. Changes from v0.1:
  - Pin order corrected to the PowerFeather harness: 1:D7 / 2:VDC / 3:GND.
  - Small parts converted to SMD (single side) for JLCPCB economic assembly;
    only the big caps + optional THT connectors remain hand-solder.
  - J1/J2 are JST S3B-XH-SM4-TB right-angle SMT, wires exit off the bottom edge.
  - One-shot C1 locked at 10uF (~40 ms strike); C1B parallel DNP pads for tuning.
  - Telemetry: populated VDC/4 divider (VSNS) + 1k-buffered gate sense (D7S)
    to J4, a 2-pin right-angle XH -> PowerFeather A4 (VSNS) / A5 (D7S).
    Both are ADC1 channels (GPIO2/GPIO1) so they work while ESP-NOW is active.

Run:  python3 generate_capboard.py   then fill zones + DRC via kicad-cli
      (ZONE_FILLER.Fill segfaults headless in KiCad 10.0.3).
"""
import pcbnew
from pcbnew import VECTOR2I_MM, FromMM

LIB = "/usr/share/kicad/footprints/"
OUT = "build/capbank.kicad_pcb"

# ---- parameters -------------------------------------------------------------
PIN_ORDER = {"1": "D7", "2": "VDC", "3": "GND"}   # matches PowerFeather header
BOARD_W, BOARD_H = 88, 40
CAP_CENTERS = [20, 41, 62]
CAP_Y = 19
CAP_PITCH = 7.5
SPINE_Y = 26.5        # VDC spine (front), clears the 4.5mm-tall SM4 pads
D7_Y = 31.5           # D7 spine (back)
VIA_Y = 34.3          # via row between connector pads and MP anchors
RAIL_Y = 1.3          # VDC rail to button cluster
LANE_Y = 9.5          # D7 lane in the button cluster

board = pcbnew.CreateEmptyBoard()
board.GetDesignSettings().SetCopperLayerCount(2)
F, B = pcbnew.F_Cu, pcbnew.B_Cu

nets = {}
for name in ["VDC", "GND", "D7", "NETA", "NETB", "VSNS", "D7S"]:
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

# ---- outline + zip-tie slots ------------------------------------------------
edge_rect(0, 0, BOARD_W, BOARD_H)
for sx in [9.7, 30.5, 51.5, 72.3]:
    edge_rect(sx - 0.9, CAP_Y - 5, sx + 0.9, CAP_Y + 5)

# ---- connectors J1/J2 (SMT right-angle, exit off bottom edge) ---------------
probe = pcbnew.FootprintLoad(LIB + "Connector_JST.pretty",
                             "JST_XH_S3B-XH-SM4-TB_1x03-1MP_P2.50mm_Horizontal")
mp_half = max(p.GetSize().y for p in probe.Pads()
              if p.GetNumber() == "MP") / 2e6
jy = BOARD_H - 0.5 - mp_half - 3.55
j1 = place("Connector_JST", "JST_XH_S3B-XH-SM4-TB_1x03-1MP_P2.50mm_Horizontal",
           "J1", 10, jy)
j2 = place("Connector_JST", "JST_XH_S3B-XH-SM4-TB_1x03-1MP_P2.50mm_Horizontal",
           "J2", 74, jy)
assign(j1, PIN_ORDER)
assign(j2, PIN_ORDER)

# ---- capacitors (THT, hand-solder) ------------------------------------------
for i, cx in enumerate(CAP_CENTERS):
    fp = place("Capacitor_THT", "CP_Radial_D18.0mm_P7.50mm", f"C{i+2}",
               cx - CAP_PITCH / 2, CAP_Y, value="22000u 16V")
    for pad in fp.Pads():
        pad.SetShape(pcbnew.PAD_SHAPE_OVAL)
        pad.SetSize(VECTOR2I_MM(3.6, 2.6))
        pad.SetDrillShape(pcbnew.PAD_DRILL_SHAPE_OBLONG)
        pad.SetDrillSize(VECTOR2I_MM(1.9, 1.1))
    assign(fp, {"1": "VDC", "2": "GND"})

# ---- one-shot button cluster (SMD, top strip) -------------------------------
sw1 = place("Button_Switch_SMD", "SW_Push_1TS009xxxx-xxxx-xxxx_6x6x5mm",
            "SW1", 16, 4)
assign(sw1, {"1": "VDC", "2": "NETA"})
j3 = place("Connector_JST", "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", "J3",
           26, 5.5, rot=90, value="AUX_BTN", dnp=True)
jp = sorted(j3.Pads(), key=lambda q: q.GetPosition().y)
jp[0].SetNet(nets["VDC"]); jp[1].SetNet(nets["NETA"])
r1 = place("Resistor_SMD", "R_0805_2012Metric", "R1", 34, 7.5, value="470R")
assign(r1, {"1": "NETA", "2": "NETB"})
c1 = place("Capacitor_SMD", "C_1206_3216Metric", "C1", 40, 7.5, value="10uF X7R 16V")
assign(c1, {"1": "NETB", "2": "D7"})
c1b = place("Capacitor_SMD", "C_1206_3216Metric", "C1B", 40, 2.6, value="DNP",
            dnp=True)
assign(c1b, {"1": "NETB", "2": "D7"})
r2 = place("Resistor_SMD", "R_0805_2012Metric", "R2", 40, 5, value="330k")
assign(r2, {"1": "NETB", "2": "D7"})
r3 = place("Resistor_SMD", "R_0805_2012Metric", "R3", 46, 8.59, rot=90, value="10k")
assign(r3, {"1": "D7", "2": "GND"})   # after rot90, pad1 lands on the D7 lane
d1 = place("Package_TO_SOT_SMD", "SOT-23", "D1", 51, 8.56, rot=270,
           value="BZX84C3V3")
assign(d1, {"1": "GND", "3": "D7"})   # SOT-23 zener: 1=anode, 3=cathode

# ---- telemetry divider + gate sense (SMD) + J4 ------------------------------
j4 = place("Connector_JST", "JST_XH_S2B-XH-A_1x02_P2.50mm_Horizontal", "J4",
           30, 32.5, value="A4/A5")
assign(j4, {"1": "D7S", "2": "VSNS"})
r6 = place("Resistor_SMD", "R_0805_2012Metric", "R6", 25.5, 32.8, rot=90,
           value="1k")
r6p = sorted(r6.Pads(), key=lambda q: q.GetPosition().y)
r6p[0].SetNet(nets["D7"]); r6p[1].SetNet(nets["D7S"])       # top pad -> D7
r4 = place("Resistor_SMD", "R_0805_2012Metric", "R4", 40, 31, value="100k")
assign(r4, {"1": "VDC", "2": "VSNS"})
r5 = place("Resistor_SMD", "R_0805_2012Metric", "R5", 44, 31, value="33k")
assign(r5, {"1": "VSNS", "2": "GND"})
c5 = place("Capacitor_SMD", "C_0805_2012Metric", "C5", 44, 33, value="100n")
assign(c5, {"1": "VSNS", "2": "GND"})

# ---- routing ----------------------------------------------------------------
j1d, j1v, j1g = pxy(j1, "1"), pxy(j1, "2"), pxy(j1, "3")
j2d, j2v, j2g = pxy(j2, "1"), pxy(j2, "2"), pxy(j2, "3")

# VDC (front)
track(*j1v, j1v[0], SPINE_Y, 1.3, F, "VDC")
track(*j2v, j2v[0], SPINE_Y, 1.3, F, "VDC")
track(5, SPINE_Y, j2v[0], SPINE_Y, 2.5, F, "VDC")
for cx in CAP_CENTERS:
    track(cx - CAP_PITCH / 2, CAP_Y, cx - CAP_PITCH / 2, SPINE_Y, 2.5, F, "VDC")
track(5, SPINE_Y, 5, RAIL_Y, 0.8, F, "VDC")
track(5, RAIL_Y, 26, RAIL_Y, 0.8, F, "VDC")
sw_vdc = pxy(sw1, "1")
track(sw_vdc[0], RAIL_Y, *sw_vdc, 0.6, F, "VDC")
track(26, RAIL_Y, jp[0].GetPosition().x / 1e6, jp[0].GetPosition().y / 1e6,
      0.6, F, "VDC")
track(39.09, 31, 39.09, SPINE_Y, 0.6, F, "VDC")             # R4.1 tap

# D7 pass-through: front stubs down to vias, spine on back (jogs around J4)
for d in (j1d, j2d):
    track(*d, d[0], VIA_Y, 0.6, F, "D7")
    via(d[0], VIA_Y, "D7")
    track(d[0], VIA_Y, d[0], D7_Y, 0.8, B, "D7")
track(j1d[0], D7_Y, 28, D7_Y, 0.8, B, "D7")
track(28, D7_Y, 28, 34.9, 0.8, B, "D7")                     # jog under J4 pads
track(28, 34.9, 34.5, 34.9, 0.8, B, "D7")
track(34.5, 34.9, 34.5, D7_Y, 0.8, B, "D7")
track(34.5, D7_Y, j2d[0], D7_Y, 0.8, B, "D7")

# GND stubs from the SMT connector pads to the back pour
for g in (j1g, j2g):
    track(*g, g[0], VIA_Y, 0.8, F, "GND")
    via(g[0], VIA_Y, "GND")

# one-shot cluster
sw_a = pxy(sw1, "2")
track(*sw_a, sw_a[0], 7.5, 0.6, F, "NETA")
track(sw_a[0], 7.5, 33.09, 7.5, 0.6, F, "NETA")
track(jp[1].GetPosition().x / 1e6, jp[1].GetPosition().y / 1e6,
      jp[1].GetPosition().x / 1e6, 7.5, 0.6, F, "NETA")
track(34.91, 7.5, 38.53, 7.5, 0.6, F, "NETB")               # R1.2 -> C1.1
track(39.09, 5, 38.53, 7.5, 0.4, F, "NETB")                 # R2.1
track(38.53, 2.6, 39.09, 5, 0.4, F, "NETB")                 # C1B.1
track(40.91, 5, 41.47, 7.5, 0.4, F, "D7")                   # R2.2
track(41.47, 2.6, 40.91, 5, 0.4, F, "D7")                   # C1B.2
track(41.47, 7.5, 43, LANE_Y, 0.6, F, "D7")                 # C1.2 -> lane
track(43, LANE_Y, 54, LANE_Y, 0.6, F, "D7")
track(46, 7.68, 46, 5.8, 0.4, F, "GND")                     # R3.2
via(46, 5.8, "GND")
d1a = pxy(d1, "1")
track(*d1a, d1a[0], 5.8, 0.4, F, "GND")
track(d1a[0], 5.8, 46, 5.8, 0.4, F, "GND")
d1k = pxy(d1, "3")
track(*d1k, d1k[0], LANE_Y, 0.4, F, "D7")
via(54, LANE_Y, "D7")                                       # lane -> back spine
track(54, LANE_Y, 54, D7_Y, 0.8, B, "D7")

# telemetry
via(25.5, D7_Y, "D7")                                       # tap back spine
r6d, r6s = r6p[0].GetPosition(), r6p[1].GetPosition()
track(25.5, D7_Y, r6d.x / 1e6, r6d.y / 1e6, 0.4, F, "D7")
j4s, j4v = pxy(j4, "1"), pxy(j4, "2")
track(r6s.x / 1e6, r6s.y / 1e6, *j4s, 0.5, F, "D7S")
track(40.91, 31, 40.91, 32.8, 0.5, F, "VSNS")               # around R4 body
track(40.91, 32.8, *j4v, 0.5, F, "VSNS")
track(40.91, 31, 43.09, 31, 0.5, F, "VSNS")                 # R4.2 -> R5.1
track(43.09, 31, 43.09, 33, 0.4, F, "VSNS")                 # -> C5.1
track(44.91, 31, 44.91, 33, 0.4, F, "GND")                  # R5.2 / C5.2
track(44.91, 31, 46, 31, 0.4, F, "GND")
track(46, 31, 46, 29.9, 0.4, F, "GND")
via(46, 29.9, "GND")

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

# ---- silkscreen -------------------------------------------------------------
silk("SOLARNOID CAPBANK v0.2", 52, 37, 1.5)
silk("1:D7 2:VDC 3:GND", 24.5, 29.0, 0.9)
silk("D7S", 30, 30.7, 0.7)
silk("VSNS", 32.9, 30.7, 0.7)
silk("TAP=1 KNOCK", 48, 2, 1.0)
silk("NO DIODE! (shade = disarm)", 52, 34.8, 0.9)

pcbnew.SaveBoard(OUT, board)
print("wrote", OUT)
