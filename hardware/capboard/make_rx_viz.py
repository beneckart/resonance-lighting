#!/usr/bin/env python3
"""Build the RX480E-docked visualization board (viz/capbank_rx_docked.kicad_pcb).

Visualization only -- never part of the fab flow. Takes the generated fab board
and attaches a 3D model of an RX480E-4 receiver to the RECVR socket, posed so
the module's right-angle pins seat in the female header exactly as they do in
hardware. Open the result in the KiCad 3D viewer.

The transform was fitted by eye against the real part (Ben, 2026-07-26):
  rot (90, 180, 90), offset (-4.1, -12.595, 14.64)
Because RECVR is placed at 90 degrees, the model offset axes are rotated:
  offset.x -> board -Y  (more negative = further from the caps; ~4.1mm is the
                         horizontal run of the right-angle pins before they
                         turn down)
  offset.y -> board -X  (more negative = further right; 2.54 = one pin pitch)
  offset.z -> board +Z  (14.64 = socket body + header thickness)

The .step model is NOT in git (see viz/README.md); run fetch_rx_model.sh first.

Usage:  python3 generate_capboard.py && python3 make_rx_viz.py
"""
import os
import shutil
import sys

import pcbnew
from pcbnew import VECTOR2I_MM  # noqa: F401  (imported for API parity)

SRC = "build/capbank.kicad_pcb"
OUT = "viz/capbank_rx_docked.kicad_pcb"
MODEL_LOCAL = "viz/radio-receiver.step"
MODEL_REF = "${KIPRJMOD}/radio-receiver.step"
ROT = (90, 180, 90)
OFFSET = (-4.1, -12.595, 14.64)

if not os.path.exists(SRC):
    sys.exit(f"{SRC} missing -- run generate_capboard.py first")
if not os.path.exists(MODEL_LOCAL):
    sys.exit(f"{MODEL_LOCAL} missing -- run ./fetch_rx_model.sh first")

os.makedirs("viz", exist_ok=True)
shutil.copy(SRC, OUT)
board = pcbnew.LoadBoard(OUT)

attached = False
for fp in board.GetFootprints():
    if fp.GetReference() != "RECVR":
        continue
    model = pcbnew.FP_3DMODEL()
    model.m_Filename = MODEL_REF
    model.m_Scale.x = model.m_Scale.y = model.m_Scale.z = 1.0
    model.m_Rotation.x, model.m_Rotation.y, model.m_Rotation.z = ROT
    model.m_Offset.x, model.m_Offset.y, model.m_Offset.z = OFFSET
    fp.Models().push_back(model)
    attached = True

if not attached:
    sys.exit("no RECVR footprint found in the source board")

pcbnew.SaveBoard(OUT, board)
print(f"wrote {OUT} (RX480E docked; open in the KiCad 3D viewer)")
