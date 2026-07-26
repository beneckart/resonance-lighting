#!/usr/bin/env python3
"""Generate the solarnoid cap-bank PCB (inline XH-XH pass-through module).

Board: 88 x 40 mm, 2 layer.
  - J1/J2: JST XH 3-pin in/out, straight-through VDC / D7 / GND
    (PIN_ORDER below is an ASSUMPTION -- confirm against the harness before fab!)
  - C2-C4: three 22,000 uF 16 V radial slots (18 mm can, oblong drills accept
    7.5-8.3 mm lead pitch -- fits both the AliExpress 8 mm and Rubycon 7.5 mm)
  - Manual-fire one-shot: SW1 tap -> single ~20 ms gate pulse on D7.
    VDC -SW1- R1(470R) - C1(4u7) -> D7 line; R2(330k) bleed across C1;
    R3(10k) gate pulldown; D1 3V3 zener clamps D7 line (ESP32-safe).
    Series C1 blocks DC: a stuck/held button cannot park the coil energized.
  - J3: aux 2-pin XH paralleling SW1 for a panel-mounted remote button (DNP ok).
  - GND is a full back-side pour; D7 runs on the back under the VDC spine.
  - Zip-tie slots flank each cap can; no other mounting hardware.

Run:  python3 generate_capboard.py   (writes build/capbank.kicad_pcb)
"""
import pcbnew
from pcbnew import VECTOR2I_MM, FromMM

LIB = "/usr/share/kicad/footprints/"
OUT = "build/capbank.kicad_pcb"

# ---- parameters -------------------------------------------------------------
PIN_ORDER = {"1": "VDC", "2": "D7", "3": "GND"}   # J1/J2 pad -> net  (CONFIRM!)
BOARD_W, BOARD_H = 88, 40
CAP_CENTERS = [20, 41, 62]        # can centers, X (mm); pitch 21 mm
CAP_Y = 19
CAP_PITCH = 7.5                   # library pitch; oblong drills add +-0.4 slack
SPINE_Y = 29.5                    # VDC spine (front)
D7_Y = 31.5                       # D7 spine (back)
RAIL_Y = 1.3                      # VDC rail to button cluster (front, thin)

board = pcbnew.CreateEmptyBoard()
board.GetDesignSettings().SetCopperLayerCount(2)

nets = {}
for name in ["VDC", "GND", "D7", "NETA", "NETB"]:
    n = pcbnew.NETINFO_ITEM(board, name)
    board.Add(n)
    nets[name] = n

def place(lib, name, ref, x, y, rot=0, value=None):
    fp = pcbnew.FootprintLoad(LIB + lib + ".pretty", name)
    fp.SetReference(ref)
    if value:
        fp.SetValue(value)
    fp.SetPosition(VECTOR2I_MM(x, y))
    try:
        fp.SetOrientationDegrees(rot)
    except AttributeError:
        fp.SetOrientation(pcbnew.EDA_ANGLE(rot, pcbnew.DEGREES_T))
    board.Add(fp)
    return fp

def assign(fp, mapping):
    """mapping: pad number -> net name"""
    for pad in fp.Pads():
        net = mapping.get(pad.GetNumber())
        if net:
            pad.SetNet(nets[net])

def track(x1, y1, x2, y2, w, layer, net):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(VECTOR2I_MM(x1, y1))
    t.SetEnd(VECTOR2I_MM(x2, y2))
    t.SetWidth(FromMM(w))
    t.SetLayer(layer)
    t.SetNet(nets[net])
    board.Add(t)

def silk(text, x, y, size=1.2, layer=None):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(text)
    t.SetPosition(VECTOR2I_MM(x, y))
    t.SetLayer(layer if layer is not None else pcbnew.F_SilkS)
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
for sx in [9.7, 30.5, 51.5, 72.3]:                 # flanking the cap cans
    edge_rect(sx - 0.9, CAP_Y - 5, sx + 0.9, CAP_Y + 5)

# ---- connectors -------------------------------------------------------------
j1 = place("Connector_JST", "JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical", "J1", 5, 35)
j2 = place("Connector_JST", "JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical", "J2", 74, 35)
assign(j1, PIN_ORDER)
assign(j2, PIN_ORDER)

# ---- capacitors -------------------------------------------------------------
for i, cx in enumerate(CAP_CENTERS):
    fp = place("Capacitor_THT", "CP_Radial_D18.0mm_P7.50mm", f"C{i+2}",
               cx - CAP_PITCH / 2, CAP_Y, value="22000u 16V")
    for pad in fp.Pads():
        pad.SetShape(pcbnew.PAD_SHAPE_OVAL)
        pad.SetSize(VECTOR2I_MM(3.6, 2.6))
        pad.SetDrillShape(pcbnew.PAD_DRILL_SHAPE_OBLONG)
        pad.SetDrillSize(VECTOR2I_MM(1.9, 1.1))
    assign(fp, {"1": "VDC", "2": "GND"})

# ---- one-shot button cluster (top strip, above the cans) --------------------
sw1 = place("Button_Switch_THT", "SW_PUSH_6mm", "SW1", 16, 3)
assign(sw1, {"1": "VDC", "2": "NETA"})
j3 = place("Connector_JST", "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", "J3",
           28, 5.5, rot=90, value="AUX_BTN")
# after rotation: whichever pad sits higher (smaller y) gets VDC
p = sorted(j3.Pads(), key=lambda q: q.GetPosition().y)
p[0].SetNet(nets["VDC"]); p[1].SetNet(nets["NETA"])
j3_top_y = p[0].GetPosition().y / 1e6
j3_bot_y = p[1].GetPosition().y / 1e6
r1 = place("Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal",
           "R1", 34, 7.5, value="470R")
assign(r1, {"1": "NETA", "2": "NETB"})
c1 = place("Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "C1",
           46, 7.5, value="4u7")
assign(c1, {"1": "NETB", "2": "D7"})
r2 = place("Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal",
           "R2", 46, 3, value="330k")
assign(r2, {"1": "NETB", "2": "D7"})
r3 = place("Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal",
           "R3", 57, 7.5, value="10k")
assign(r3, {"1": "D7", "2": "GND"})
d1 = place("Diode_THT", "D_DO-35_SOD27_P7.62mm_Horizontal", "D1",
           68, 7.5, value="BZX55C3V3")
assign(d1, {"1": "D7", "2": "GND"})   # pad1 = cathode -> D7 line (clamp)

# ---- front copper: VDC ------------------------------------------------------
F, B = pcbnew.F_Cu, pcbnew.B_Cu
track(5, 35, 5, SPINE_Y, 2.5, F, "VDC")            # J1.1 to spine
track(5, SPINE_Y, 74, SPINE_Y, 2.5, F, "VDC")      # spine
track(74, SPINE_Y, 74, 35, 2.5, F, "VDC")          # spine to J2.1
for cx in CAP_CENTERS:                              # cap + stubs
    track(cx - CAP_PITCH / 2, CAP_Y, cx - CAP_PITCH / 2, SPINE_Y, 2.5, F, "VDC")
track(5, SPINE_Y, 5, RAIL_Y, 0.8, F, "VDC")        # riser to button rail
track(5, RAIL_Y, 28, RAIL_Y, 0.8, F, "VDC")        # rail
track(16, RAIL_Y, 16, 3, 0.6, F, "VDC")            # SW1 contact 1 stubs
track(22.5, RAIL_Y, 22.5, 3, 0.6, F, "VDC")
track(28, RAIL_Y, 28, j3_top_y, 0.6, F, "VDC")     # J3 VDC pad

# ---- front copper: button one-shot ------------------------------------------
track(16, 7.5, 34, 7.5, 0.6, F, "NETA")            # SW1.2 / J3 / R1.1
track(28, j3_bot_y, 28, 7.5, 0.6, F, "NETA")       # J3 lower pad to NETA row
track(41.62, 7.5, 46, 7.5, 0.6, F, "NETB")         # R1.2 -> C1.1
track(46, 7.5, 46, 3, 0.6, F, "NETB")              # -> R2.1
track(51, 7.5, 57, 7.5, 0.6, F, "D7")              # C1.2 -> R3.1
track(53.62, 3, 53.62, 7.5, 0.6, F, "D7")          # R2.2 down to D7 row
track(55, 7.5, 55, 5, 0.6, F, "D7")                # branch over R3 body to D1
track(55, 5, 68, 5, 0.6, F, "D7")
track(68, 5, 68, 7.5, 0.6, F, "D7")

# ---- back copper: D7 pass-through + riser (GND zone flows around it) --------
track(7.5, 35, 7.5, D7_Y, 0.8, B, "D7")
track(7.5, D7_Y, 76.5, D7_Y, 0.8, B, "D7")
track(76.5, D7_Y, 76.5, 35, 0.8, B, "D7")
track(51, 7.5, 48, 10, 0.8, B, "D7")               # cluster riser (avoids slot)
track(48, 10, 48, D7_Y, 0.8, B, "D7")

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
# NOTE: ZONE_FILLER.Fill segfaults headless in KiCad 10.0.3 -- zones are
# filled afterwards via: kicad-cli pcb drc --refill-zones --save-board

# ---- silkscreen -------------------------------------------------------------
silk("SOLARNOID CAPBANK v0.1", 42, 37, 1.5)
for jx in [5, 74]:
    for i, lbl in enumerate(["VDC", "D7", "GND"]):
        silk(lbl, jx + 2.5 * i, 32.3, 0.9)
silk("TAP=1 KNOCK", 33.5, 1.2, 1.0)
silk("NO DIODE! (shade = disarm)", 42, 34.8, 0.9)

pcbnew.SaveBoard(OUT, board)
print("wrote", OUT)
