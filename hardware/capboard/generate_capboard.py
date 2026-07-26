#!/usr/bin/env python3
"""Generate the solarnoid cap-bank PCB, v0.3 (inline XH pass-through module).

Board: 88 x 40 mm, 2 layer. Changes from v0.2:
  - All four connectors are the SAME part: JST S3B-XH-SM4-TB right-angle SMT
    (one BOM line, all JLC-assembled, all wires exit off the bottom edge).
    J3 gained a GND pin (illuminated-button option); J4 gained a dedicated
    GND return for clean ADC sensing.
  - C1B (optional pulse-stretch cap) is now through-hole for hand-soldering.
  - Back-side D7 spine runs straight (no more through-holes in its path).
  - Silkscreen decluttered: title top-right, button label left of SW1.

Connector pinouts (left to right, silkscreen-labeled):
  J1/J2 (pass-through): 1:D7  2:VDC  3:GND   -- matches PowerFeather header
  J3 (remote button):   1:BTN 2:VDC  3:GND
  J4 (telemetry):       1:D7S 2:VSNS 3:GND   -- to PowerFeather A5/A4/GND

Run:  python3 generate_capboard.py   then fill zones + DRC via kicad-cli
      (ZONE_FILLER.Fill segfaults headless in KiCad 10.0.3).
"""
import pcbnew
from pcbnew import VECTOR2I_MM, FromMM

LIB = "/usr/share/kicad/footprints/"
OUT = "build/capbank.kicad_pcb"
SM4 = "JST_XH_S3B-XH-SM4-TB_1x03-1MP_P2.50mm_Horizontal"

# ---- parameters -------------------------------------------------------------
PIN_ORDER = {"1": "D7", "2": "VDC", "3": "GND"}   # J1/J2, matches PowerFeather
BOARD_W, BOARD_H = 88, 40
CAP_CENTERS = [20, 41, 62]
CAP_Y = 18.3
CAP_PITCH = 7.5
SPINE_Y = 26.5        # VDC spine (front)
D7_Y = 31.5           # D7 spine (back, straight)
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

# ---- connectors (all S3B-XH-SM4, bottom edge) -------------------------------
probe = pcbnew.FootprintLoad(LIB + "Connector_JST.pretty", SM4)
mp_half = max(p.GetSize().y for p in probe.Pads()
              if p.GetNumber() == "MP") / 2e6
jy = BOARD_H - 0.5 - mp_half - 3.55
j1 = place("Connector_JST", SM4, "J1", 10, jy)
j2 = place("Connector_JST", SM4, "J2", 74, jy)
assign(j1, PIN_ORDER)
assign(j2, PIN_ORDER)
j3 = place("Connector_JST", SM4, "J3", 25, jy, value="BTN")
assign(j3, {"1": "NETA", "2": "VDC", "3": "GND"})
j4 = place("Connector_JST", SM4, "J4", 59.3, jy, value="SENSE")
assign(j4, {"1": "D7S", "2": "VSNS", "3": "GND"})

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

# ---- one-shot button cluster ------------------------------------------------
sw1 = place("Button_Switch_SMD", "SW_Push_1TS009xxxx-xxxx-xxxx_6x6x5mm",
            "SW1", 16, 4)
assign(sw1, {"1": "VDC", "2": "NETA"})
r1 = place("Resistor_SMD", "R_0805_2012Metric", "R1", 34, 7.5, value="470R")
assign(r1, {"1": "NETA", "2": "NETB"})
c1 = place("Capacitor_SMD", "C_1206_3216Metric", "C1", 40, 7.5, value="10uF X7R 16V")
assign(c1, {"1": "NETB", "2": "D7"})
c1b = place("Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "C1B",
            37.5, 2.5, value="DNP", dnp=True)
assign(c1b, {"1": "NETB", "2": "D7"})
r2 = place("Resistor_SMD", "R_0805_2012Metric", "R2", 40, 5.4, value="330k")
assign(r2, {"1": "NETB", "2": "D7"})
r3 = place("Resistor_SMD", "R_0805_2012Metric", "R3", 47, 8.59, rot=90, value="10k")
assign(r3, {"1": "D7", "2": "GND"})   # after rot90, pad1 lands on the D7 lane
d1 = place("Package_TO_SOT_SMD", "SOT-23", "D1", 51, 8.56, rot=270,
           value="BZX84C3V3")
assign(d1, {"1": "GND", "3": "D7"})   # SOT-23 zener: 1=anode, 3=cathode

# ---- telemetry divider + gate sense -----------------------------------------
r4 = place("Resistor_SMD", "R_0805_2012Metric", "R4", 38, 28.9, value="100k")
assign(r4, {"1": "VDC", "2": "VSNS"})
r5 = place("Resistor_SMD", "R_0805_2012Metric", "R5", 43, 28.9, value="33k")
assign(r5, {"1": "VSNS", "2": "GND"})
c5 = place("Capacitor_SMD", "C_0805_2012Metric", "C5", 43, 30.9, value="100n")
assign(c5, {"1": "VSNS", "2": "GND"})
r6 = place("Resistor_SMD", "R_0805_2012Metric", "R6", 48.4, 29.9, value="1k")
assign(r6, {"1": "D7", "2": "D7S"})

# ---- routing ----------------------------------------------------------------
j1d, j1v, j1g = pxy(j1, "1"), pxy(j1, "2"), pxy(j1, "3")
j2d, j2v, j2g = pxy(j2, "1"), pxy(j2, "2"), pxy(j2, "3")

# VDC (front)
track(*j1v, j1v[0], SPINE_Y, 1.3, F, "VDC")
track(*j2v, j2v[0], SPINE_Y, 1.3, F, "VDC")
j3v = pxy(j3, "2")
track(*j3v, j3v[0], SPINE_Y, 1.3, F, "VDC")
track(5, SPINE_Y, j2v[0], SPINE_Y, 2.5, F, "VDC")
for cx in CAP_CENTERS:
    track(cx - CAP_PITCH / 2, CAP_Y, cx - CAP_PITCH / 2, SPINE_Y, 2.5, F, "VDC")
track(5, SPINE_Y, 5, RAIL_Y, 0.8, F, "VDC")
sw_vdc = pxy(sw1, "1")
track(5, RAIL_Y, sw_vdc[0], RAIL_Y, 0.8, F, "VDC")
track(sw_vdc[0], RAIL_Y, *sw_vdc, 0.6, F, "VDC")
track(37.09, 28.9, 37.09, SPINE_Y, 0.5, F, "VDC")           # R4.1 tap

# D7 pass-through: front stubs down to vias, straight spine on back
for d in (j1d, j2d):
    track(*d, d[0], VIA_Y, 0.6, F, "D7")
    via(d[0], VIA_Y, "D7")
    track(d[0], VIA_Y, d[0], D7_Y, 0.8, B, "D7")
track(j1d[0], D7_Y, j2d[0], D7_Y, 0.8, B, "D7")

# GND stubs from SMT connector pads to the back pour
for g in (j1g, j2g, pxy(j3, "3"), pxy(j4, "3")):
    track(*g, g[0], VIA_Y, 0.8, F, "GND")
    via(g[0], VIA_Y, "GND")

# J3 remote button: NETA rides the back layer up to the cluster row
j3a = pxy(j3, "1")
track(*j3a, 20, 29.3, 0.5, F, "NETA")
via(20, 29.3, "NETA")
track(20, 29.3, 20, 7.5, 0.5, B, "NETA")
via(20, 7.5, "NETA")

# one-shot cluster
sw_a = pxy(sw1, "2")
track(*sw_a, sw_a[0], 7.5, 0.6, F, "NETA")
track(sw_a[0], 7.5, 33.09, 7.5, 0.6, F, "NETA")
track(34.91, 7.5, 38.53, 7.5, 0.6, F, "NETB")               # R1.2 -> C1.1
track(39.09, 5.4, 38.53, 7.5, 0.4, F, "NETB")               # R2.1
track(37.5, 2.5, 39.09, 5.4, 0.4, F, "NETB")                # C1B.1
track(40.91, 5.4, 41.47, 7.5, 0.4, F, "D7")                 # R2.2
track(42.5, 2.5, 40.91, 5.4, 0.4, F, "D7")                  # C1B.2
track(41.47, 7.5, 43, LANE_Y, 0.6, F, "D7")                 # C1.2 -> lane
track(43, LANE_Y, 54, LANE_Y, 0.6, F, "D7")
track(47, 7.68, 47, 5.8, 0.4, F, "GND")                     # R3.2
via(47, 5.8, "GND")
d1a = pxy(d1, "1")
track(*d1a, d1a[0], 5.8, 0.4, F, "GND")
track(d1a[0], 5.8, 47, 5.8, 0.4, F, "GND")
d1k = pxy(d1, "3")
track(*d1k, d1k[0], LANE_Y, 0.4, F, "D7")
via(54, LANE_Y, "D7")                                       # lane -> back spine
track(54, LANE_Y, 54, D7_Y, 0.8, B, "D7")

# telemetry
track(38.91, 28.9, 42.09, 28.9, 0.5, F, "VSNS")             # R4.2 -> R5.1
track(42.09, 28.9, 42.09, 30.9, 0.4, F, "VSNS")             # -> C5.1
track(43.91, 28.9, 43.91, 30.9, 0.4, F, "GND")              # R5.2 / C5.2
track(43.91, 28.9, 45.5, 28.9, 0.4, F, "GND")
via(45.5, 28.9, "GND")
j4s, j4v_ = pxy(j4, "1"), pxy(j4, "2")
track(42.09, 30.9, 42.09, 35, 0.5, F, "VSNS")               # around the pads
track(42.09, 35, j4v_[0], 35, 0.5, F, "VSNS")
track(j4v_[0], 35, *j4v_, 0.5, F, "VSNS")
via(47.49, D7_Y, "D7")                                      # tap back spine
track(47.49, D7_Y, 47.49, 29.9, 0.4, F, "D7")               # -> R6.1
track(49.31, 29.9, *j4s, 0.5, F, "D7S")                     # R6.2 -> J4.1

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
silk("SOLARNOID CAPBANK v0.3", 72, 3.2, 1.4)
silk("TAP=1 KNOCK", 6.9, 6.7, 0.9)
silk("D7 VDC GND", 10, 25.6, 0.65)
silk("D7 VDC GND", 74, 25.6, 0.65)
silk("BTN VDC GND", 25, 27.85, 0.6)
silk("D7S VSNS GND", 59.3, 27.85, 0.6)
silk("NO DIODE! (shade = disarm)", 42, 37, 0.85)

pcbnew.SaveBoard(OUT, board)
print("wrote", OUT)
