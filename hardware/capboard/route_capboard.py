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
D7LANE = 11.5
path([one("SW1", "NETA"), one("R1", "NETA")], 0.6, F, "NETA")
path([one("R1", "NETB"), one("C1", "NETB")], 0.6, F, "NETB")
c1b_b, c1b_d = one("C1B", "NETB"), one("C1B", "D7")
path([c1b_b, one("C1", "NETB")], 0.5, F, "NETB")
path([c1b_b, (one("R2", "NETB")[0], 6.6)], 0.5, B, "NETB")        # under C1B's D7 pad
via(one("R2", "NETB")[0], 6.6, "NETB")
path([(one("R2", "NETB")[0], 6.6), one("R2", "NETB")], 0.5, F, "NETB")
for ref in ("C1", "C1B", "R2", "R3", "D1"):
    p = one(ref, "D7")
    path([p, (p[0], D7LANE)], 0.5, F, "D7")
path([(one("C1", "D7")[0], D7LANE), (one("D1", "D7")[0], D7LANE)], 0.6, F, "D7")

# ------------------------------------------------------------------ VDCIN ---
TOPRAIL = 1.0
sw_v = one("SW1", "VDCIN")
path([one("PANEL", "VDCIN"), (13.5, 27.9), (6.0, 27.9), (6.0, TOPRAIL)],
     0.6, F, "VDCIN")
path([(6.0, 5.0), sw_v], 0.6, F, "VDCIN")
path([(6.0, TOPRAIL), (68.0, TOPRAIL), (68.0, 8.15), one("L1", "VDCIN")],
     0.8, F, "VDCIN")
path([one("L1", "VDCIN"), (76.0, 8.15), (76.0, 12.0), (88.0, 12.0),
      (88.0, 18.95), one("U2", "VDCIN")], 0.6, F, "VDCIN")
path([(88.0, 20.5), (93.5, 20.5), one("JP1", "VDCIN")], 0.6, F, "VDCIN")
path([(88.0, 18.95), (88.0, 20.5)], 0.6, F, "VDCIN")
via(88.0, 20.5, "VDCIN")                                          # under the spine
path([(88.0, 20.5), (88.0, 29.5)], 0.6, B, "VDCIN")
path([(88.0, 29.5), (one("R10", "VDCIN")[0], 29.5)], 0.6, B, "VDCIN")
via(one("R10", "VDCIN")[0], 29.5, "VDCIN")
path([(one("R10", "VDCIN")[0], 29.5), one("R10", "VDCIN")], 0.6, F, "VDCIN")
path([(88.0, 29.5), (one("C8", "VDCIN")[0], 29.5)], 0.6, B, "VDCIN")
via(one("C8", "VDCIN")[0], 29.5, "VDCIN")
path([(one("C8", "VDCIN")[0], 29.5), one("C8", "VDCIN")], 0.6, F, "VDCIN")

# --------------------------------------------------------------------- SW ---
l1sw, d2sw = one("L1", "SW"), one("D2", "SW")
u2sw = sorted(pads_on("U2", "SW"))
path([l1sw, (l1sw[0], 6.0), (d2sw[0], 6.0), d2sw], 0.9, F, "SW")
via(91.5, 6.0, "SW")
path([(91.5, 6.0), (91.5, 16.5), (84.14, 16.5)], 0.8, B, "SW")
via(84.14, 16.5, "SW")
path([(84.14, 16.5), (u2sw[0][0], 16.5)], 0.7, F, "SW")
path([(84.14, 16.5), (u2sw[1][0], 16.5)], 0.7, F, "SW")
for p in u2sw:
    path([(p[0], 16.5), p], 0.7, F, "SW")

# ----------------------------------------------------------------- VBOOST ---
SPINE = 26.0
d2k, c9v = one("D2", "VBOOST"), one("C9", "VBOOST")
path([d2k, (98.0, d2k[1]), (98.0, c9v[1]), c9v], 1.0, F, "VBOOST")
path([(98.0, one("JP1", "VBOOST")[1]), one("JP1", "VBOOST")], 0.6, F, "VBOOST")
path([(98.0, c9v[1]), (98.0, 27.5), (76.0, 27.5), (76.0, SPINE)],
     1.2, F, "VBOOST")
path([one("DRIVER", "VBOOST"), (one("DRIVER", "VBOOST")[0], 27.5)],
     1.2, F, "VBOOST")
path([one("R8", "VBOOST"), (one("R8", "VBOOST")[0], 27.5)], 0.6, F, "VBOOST")
path([(13.0, SPINE), (76.0, SPINE)], 2.5, F, "VBOOST")
for ref in ("C2", "C3", "C4"):
    x, y = one(ref, "VBOOST")
    path([(x, y), (x, SPINE)], 2.5, F, "VBOOST")
path([one("U1", "VBOOST"), (53.0, SPINE)], 0.6, F, "VBOOST")
path([one("R4", "VBOOST"), (44.0, 35.5), (44.0, SPINE)], 0.5, F, "VBOOST")

# --------------------------------------------------------------------- FB ---
fb = one("U2", "FB")
path([fb, (80.4, fb[1] + 1.2)], 0.4, F, "FB")
via(80.4, fb[1] + 1.2, "FB")
path([(80.4, fb[1] + 1.2), (74.5, fb[1]), (74.5, one("R9", "FB")[1])], 0.4, B, "FB")
via(74.5, one("R9", "FB")[1], "FB")
path([(74.5, one("R9", "FB")[1]), one("R9", "FB")], 0.4, F, "FB")
path([(74.5, fb[1]), (74.5, one("R8", "FB")[1])], 0.4, B, "FB")
via(74.5, one("R8", "FB")[1], "FB")
path([(74.5, one("R8", "FB")[1]), one("R8", "FB")], 0.4, F, "FB")
path([(74.5, one("R9", "FB")[1]), (74.5, one("R12", "FB")[1]),
      (one("R12", "FB")[0], one("R12", "FB")[1])], 0.4, B, "FB")

# --------------------------------------------------------------------- EN ---
en = one("U2", "EN")
path([en, (en[0], 22.2), (79.5, 22.2), one("R11", "EN")], 0.4, F, "EN")
via(79.5, 22.2, "EN")
path([(79.5, 22.2), (81.0, 24.0), (81.0, 31.0),
      (one("R10", "EN")[0], 31.0)], 0.4, B, "EN")
via(one("R10", "EN")[0], 31.0, "EN")
path([(81.0, 31.0), (one("TELE", "EN")[0], 31.5)], 0.4, B, "EN")
via(one("TELE", "EN")[0], 31.5, "EN")
path([(one("TELE", "EN")[0], 31.5), one("TELE", "EN")], 0.4, F, "EN")

# --------------------------------------------------------- D7 pass-through ---
D7BACK = 33.4
for ref in ("PANEL", "DRIVER"):
    x, y = one(ref, "D7")
    path([(x, y), (x, D7BACK)], 0.6, F, "D7")
    via(x, D7BACK, "D7")
path([(one("PANEL", "D7")[0], D7BACK), (one("DRIVER", "D7")[0], D7BACK)],
     0.8, B, "D7")
via(one("D1", "D7")[0], D7LANE, "D7")
path([(one("D1", "D7")[0], D7LANE), (49.5, 20.0), (49.5, D7BACK)], 0.6, B, "D7")

# ------------------------------------------------------------- RECVR chain ---
d0, r7b = one("RECVR", "BTNP"), one("R7", "BTNP")
path([d0, (d0[0], 33.9), (r7b[0], 33.9), r7b], 0.4, F, "BTNP")
na = one("R7", "NETA")
path([na, (28.0, na[1])], 0.5, F, "NETA")
via(28.0, na[1], "NETA")
path([(28.0, na[1]), (16.0, 24.0), (16.0, 6.6)], 0.5, B, "NETA")
via(16.0, 6.6, "NETA")
path([(16.0, 6.6), (one("SW1", "NETA")[0], 6.6), one("SW1", "NETA")],
     0.5, F, "NETA")

# --------------------------------------------------------------- LDO / 5V ---
u1p, c7p, p5 = one("U1", "P5V"), one("C7", "P5V"), one("RECVR", "P5V")
path([u1p, (47.0, u1p[1])], 0.5, F, "P5V")
via(47.0, u1p[1], "P5V")
path([(47.0, u1p[1]), (42.6, c7p[1])], 0.5, B, "P5V")
via(42.6, c7p[1], "P5V")
path([(42.6, c7p[1]), c7p], 0.5, F, "P5V")
path([(42.6, c7p[1]), (42.6, 31.8), (p5[0], 31.8)], 0.5, B, "P5V")
via(p5[0], 31.8, "P5V")
path([(p5[0], 31.8), p5], 0.5, F, "P5V")

# ------------------------------------------------------------------- VSNS ---
r4s, r5s, c5s = one("R4", "VSNS"), one("R5", "VSNS"), one("C5", "VSNS")
path([r4s, (49.7, r4s[1]), (49.7, r5s[1]), r5s], 0.4, F, "VSNS")
path([r5s, c5s, one("TELE", "VSNS")], 0.4, F, "VSNS")

# ------------------------------------------------------------------- GND ---
FORCED = {"R3": (1, 0), "D1": (1, 0), "R5": (0, 1), "C5": (0, 1),
          "C9": (-1, 0), "R12": (1, 0), "U1": (2, -1), "C5": (1, 0)}
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
