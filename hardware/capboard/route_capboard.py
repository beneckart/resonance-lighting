#!/usr/bin/env python3
"""Route the v1.1 cap-bank board on top of Ben's hand placement.

Loads `capbank_placement_ben.kicad_pcb` (footprints + silkscreen, no tracks),
adds copper, pours the back GND plane, and writes `build/capbank.kicad_pcb`.

Placement and silkscreen are Ben's and are never modified here -- this script
only adds tracks, vias and the zone. Re-run it any time he re-arranges.

Usage:  python3 route_capboard.py
"""
import os
import sys

import pcbnew
from pcbnew import VECTOR2I_MM, FromMM

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "capbank_placement_ben.kicad_pcb")
OUT = os.path.join(HERE, "build", "capbank.kicad_pcb")

board = pcbnew.LoadBoard(SRC)
if board is None:
    sys.exit(f"could not load {SRC}")
F, B = pcbnew.F_Cu, pcbnew.B_Cu

nets = {n.GetNetname(): n for n in board.GetNetInfo().NetsByNetcode().values()}
fps = {fp.GetReference(): fp for fp in board.GetFootprints()}


def P(ref, pad):
    """Position of a pad, in mm."""
    for p in fps[ref].Pads():
        if p.GetNumber() == str(pad):
            v = p.GetPosition()
            return (v.x / 1e6, v.y / 1e6)
    raise KeyError(f"{ref}.{pad}")


def pads_on(ref, netname):
    """Every pad of a footprint carrying the given net, as (x, y) mm."""
    out = []
    for p in fps[ref].Pads():
        if p.GetNetname() == netname:
            v = p.GetPosition()
            out.append((v.x / 1e6, v.y / 1e6))
    return out


def one(ref, netname):
    got = pads_on(ref, netname)
    if not got:
        raise KeyError(f"{ref} has no {netname} pad")
    return got[0]


def track(x1, y1, x2, y2, w, layer, net):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(VECTOR2I_MM(x1, y1))
    t.SetEnd(VECTOR2I_MM(x2, y2))
    t.SetWidth(FromMM(w))
    t.SetLayer(layer)
    t.SetNet(nets[net])
    board.Add(t)


def path(pts, w, layer, net):
    for a, b in zip(pts, pts[1:]):
        track(a[0], a[1], b[0], b[1], w, layer, net)


def via(x, y, net):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(VECTOR2I_MM(x, y))
    v.SetDrill(FromMM(0.4))
    v.SetWidth(FromMM(0.8))
    v.SetNet(nets[net])
    board.Add(v)


def stitch(ref, netname="GND", d=1.7, w=0.5, force=None):
    """Drop every GND pad into the back pour, pushing OUTWARD from the part
    centre so the via never lands on a neighbouring pad."""
    c = fps[ref].GetPosition()
    cx, cy = c.x / 1e6, c.y / 1e6
    for (x, y) in pads_on(ref, netname):
        vx, vy = (force if force else (x - cx, y - cy))
        n = (vx * vx + vy * vy) ** 0.5
        if n < 0.05:
            vx, vy, n = 0.0, 1.0, 1.0
        px, py = x + d * vx / n, y + d * vy / n
        track(x, y, px, py, w, F, netname)
        via(px, py, netname)


# ---------------------------------------------------------------- one-shot ---
path([one("SW1", "NETA"), one("R1", "NETA")], 0.6, F, "NETA")
path([one("R1", "NETB"), one("C1", "NETB")], 0.6, F, "NETB")
# C1B is THT, so hop to R2 on the back and dodge C1B's own D7 pad
c1b_b = one("C1B", "NETB")
path([c1b_b, one("C1", "NETB")], 0.5, F, "NETB")
path([c1b_b, (one("R2", "NETB")[0], 6.6)], 0.5, B, "NETB")
via(one("R2", "NETB")[0], 6.6, "NETB")
path([(one("R2", "NETB")[0], 6.6), one("R2", "NETB")], 0.5, F, "NETB")

D7LANE = 11.4
path([one("C1", "D7"), (one("C1", "D7")[0], D7LANE)], 0.6, F, "D7")
path([one("C1B", "D7"), (one("C1B", "D7")[0], 8.2), (35.4, 8.2),
      (35.4, one("R2", "D7")[1]), one("R2", "D7")], 0.5, F, "D7")
path([one("R3", "D7"), (one("R3", "D7")[0], 8.2)], 0.5, F, "D7")
path([one("D1", "D7"), (one("D1", "D7")[0], 8.2)], 0.5, F, "D7")
path([(one("C1", "D7")[0], D7LANE), (46.0, D7LANE)], 0.6, F, "D7")
path([(one("R3", "D7")[0], 8.2), (46.0, 8.2), (46.0, D7LANE)], 0.5, F, "D7")

# ------------------------------------------------------------------ VDCIN ---
TOPRAIL = 1.1
sw_v = one("SW1", "VDCIN")
path([one("PANEL", "VDCIN"), (13.5, 27.0), (6.6, 27.0), (6.6, TOPRAIL),
      (sw_v[0], TOPRAIL), sw_v], 0.8, F, "VDCIN")
path([(sw_v[0], TOPRAIL), (65.0, TOPRAIL)], 0.8, F, "VDCIN")
via(65.0, TOPRAIL, "VDCIN")
path([(65.0, TOPRAIL), (65.0, 21.0), (68.4, 21.0)], 0.8, B, "VDCIN")
via(68.4, 21.0, "VDCIN")
c8v = one("C8", "VDCIN")
path([(68.4, 21.0), (68.4, c8v[1]), c8v], 0.8, F, "VDCIN")
path([c8v, (71.4, c8v[1]), (71.4, 13.0), (79.6, 13.0), (79.6, 16.45),
      one("U2", "VDCIN")], 0.6, F, "VDCIN")
path([(79.6, 13.0), (79.6, 11.0), one("L1", "VDCIN")], 0.8, F, "VDCIN")
path([one("L1", "VDCIN"), (80.92, 7.0), (88.5, 7.0), (88.5, 23.83),
      one("R10", "VDCIN")], 0.6, F, "VDCIN")
path([one("R10", "VDCIN"), (85.6, 23.83), (85.6, 28.6),
      (one("JP1", "VDCIN")[0], 28.6), one("JP1", "VDCIN")], 0.6, F, "VDCIN")

# --------------------------------------------------------------------- SW ---
path([one("L1", "SW"), (85.08, 14.4), (79.4, 14.4), (79.4, 15.5),
      one("U2", "SW")], 0.9, F, "SW")
u2sw = pads_on("U2", "SW")
path([u2sw[0], (u2sw[0][0], 14.4), (79.4, 14.4)], 0.6, F, "SW")
path([(79.4, 14.4), (79.4, 20.8), (one("D2", "SW")[0], 20.8),
      one("D2", "SW")], 0.9, F, "SW")

# ----------------------------------------------------------------- VBOOST ---
SPINE = 25.0
d2k = one("D2", "VBOOST")
c9v = one("C9", "VBOOST")
path([d2k, (70.5, d2k[1]), (70.5, 19.0), (81.0, 19.0), (81.0, c9v[1]), c9v],
     1.0, F, "VBOOST")
via(81.0, 19.0, "VBOOST")
path([(81.0, 19.0), (88.0, 19.0), (88.0, 26.83), one("R8", "VBOOST")],
     0.6, B, "VBOOST")
via(one("R8", "VBOOST")[0], 26.83, "VBOOST")
path([(88.0, 26.83), (88.0, 30.33), one("JP1", "VBOOST")], 0.6, B, "VBOOST")
via(one("JP1", "VBOOST")[0], 30.33, "VBOOST")
path([d2k, (70.5, d2k[1]), (70.5, SPINE)], 1.6, F, "VBOOST")
path([(9.0, SPINE), (74.0, SPINE)], 2.5, F, "VBOOST")
for ref in ("C2", "C3", "C4"):
    x, y = one(ref, "VBOOST")
    path([(x, y), (x, SPINE)], 2.5, F, "VBOOST")
dv = one("DRIVER", "VBOOST")
path([dv, (dv[0], SPINE)], 1.6, F, "VBOOST")
path([one("U1", "VBOOST"), (52.9, 28.0), (52.9, SPINE)], 0.6, F, "VBOOST")
path([one("R4", "VBOOST"), (48.47, 32.6), (45.0, 32.6), (45.0, SPINE)],
     0.5, F, "VBOOST")

# --------------------------------------------------------------------- FB ---
fb_u2 = one("U2", "FB")
path([fb_u2, (71.4, fb_u2[1]), (71.4, 11.6), (one("R9", "FB")[0], 11.6),
      one("R9", "FB")], 0.4, F, "FB")
path([one("R9", "FB"), (one("R9", "FB")[0], 5.6), (one("R12", "FB")[0], 5.6),
      one("R12", "FB")], 0.4, F, "FB")
path([one("R8", "FB"), (79.6, 26.83), (79.6, 22.0)], 0.4, F, "FB")
via(79.6, 22.0, "FB")
path([(79.6, 22.0), (79.6, 11.6), (71.4, 11.6)], 0.4, B, "FB")
via(71.4, 11.6, "FB")

# --------------------------------------------------------------------- EN ---
en_u2 = one("U2", "EN")
path([en_u2, (78.4, en_u2[1]), (78.4, 9.0), one("R11", "EN")], 0.4, F, "EN")
path([one("R10", "EN"), (80.6, 23.83), (80.6, 21.4), (78.4, 21.4)],
     0.4, F, "EN")
via(78.4, 21.4, "EN")
path([(78.4, 21.4), (66.0, 29.4), (one("TELE", "EN")[0], 29.4)], 0.4, B, "EN")
via(one("TELE", "EN")[0], 29.4, "EN")
path([(one("TELE", "EN")[0], 29.4), one("TELE", "EN")], 0.4, F, "EN")

# --------------------------------------------------------- D7 pass-through ---
D7BACK = 33.4
for ref in ("PANEL", "DRIVER"):
    x, y = one(ref, "D7")
    path([(x, y), (x, D7BACK)], 0.6, F, "D7")
    via(x, D7BACK, "D7")
path([(one("PANEL", "D7")[0], D7BACK), (one("DRIVER", "D7")[0], D7BACK)],
     0.8, B, "D7")
via(46.0, D7LANE, "D7")
path([(46.0, D7LANE), (46.0, 21.0), (57.0, 21.0), (57.0, D7BACK)],
     0.6, B, "D7")

# ------------------------------------------------------------- RECVR chain ---
d0 = one("RECVR", "BTNP")
path([d0, (d0[0], d0[1] + 1.8), (one("R7", "BTNP")[0], d0[1] + 1.8),
      one("R7", "BTNP")], 0.5, F, "BTNP")
na = one("R7", "NETA")
path([na, (na[0], 27.0)], 0.5, F, "NETA")
via(na[0], 27.0, "NETA")
path([(na[0], 27.0), (23.0, 27.0), (23.0, 13.4)], 0.5, B, "NETA")
via(23.0, 13.4, "NETA")
path([(23.0, 13.4), (one("SW1", "NETA")[0], 13.4), one("SW1", "NETA")],
     0.5, F, "NETA")

# --------------------------------------------------------------- LDO / 5V ---
path([one("U1", "P5V"), one("C7", "P5V")], 0.5, F, "P5V")
p5 = one("RECVR", "P5V")
path([one("C7", "P5V"), (one("C7", "P5V")[0], 33.8), (p5[0], 33.8), p5],
     0.5, F, "P5V")

# ------------------------------------------------------------------- VSNS ---
r4s = one("R4", "VSNS")
path([r4s, one("R5", "VSNS")], 0.4, F, "VSNS")
path([one("R5", "VSNS"), one("C5", "VSNS")], 0.4, F, "VSNS")
path([r4s, (r4s[0], 38.9), (one("TELE", "VSNS")[0], 38.9),
      one("TELE", "VSNS")], 0.4, F, "VSNS")

# ------------------------------------------------------------------- GND ---
# parts hugging an edge get an explicit inward push so the via never lands on
# the top VDCIN rail or the bottom VSNS run
FORCED = {"R3": (0, 1), "D1": (0, 1), "R5": (0, -1), "C5": (0, -1)}
for ref in ("SW1", "R3", "D1", "C7", "U1", "R5", "C5", "C8", "C9", "R9",
            "R11", "U2", "R12", "PANEL", "DRIVER", "RECVR", "C2", "C3", "C4"):
    if pads_on(ref, "GND"):
        stitch(ref, force=FORCED.get(ref))

# ---------------------------------------------------------- back GND pour ---
z = pcbnew.ZONE(board)
z.SetLayer(B)
z.SetNetCode(nets["GND"].GetNetCode())
z.Outline().NewOutline()
for x, y in [(0.5, 0.5), (99.5, 0.5), (99.5, 39.5), (0.5, 39.5)]:
    z.Outline().Append(FromMM(x), FromMM(y))
z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
z.SetThermalReliefSpokeWidth(FromMM(1.0))
z.SetThermalReliefGap(FromMM(0.5))
z.SetMinThickness(FromMM(0.25))
z.SetLocalClearance(FromMM(0.3))
board.Add(z)

os.makedirs(os.path.join(HERE, "build"), exist_ok=True)
pcbnew.SaveBoard(OUT, board)
print(f"routed -> {OUT}")
