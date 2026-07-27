# Visualization boards (not fab artifacts)

`capbank_rx_docked.kicad_pcb` is the v1.0 cap-bank board with an RX480E-4
receiver module posed in the RECVR socket, for viewing in the KiCad 3D viewer.
It is **visualization only** — it never feeds gerbers, drills, placement, or
BOM, and the fab board (`../build/capbank.kicad_pcb`) carries no such model.

Rebuild it:

```bash
./fetch_rx_model.sh                  # once: pulls radio-receiver.step
python3 generate_capboard.py         # regenerates the fab board
python3 make_rx_viz.py               # writes viz/capbank_rx_docked.kicad_pcb
```

## About the 3D model

`radio-receiver.step` comes from https://github.com/besi/kicad-radio-receiver
(3D model credited there to Alejandro Hurtado via GrabCAD). That project ships
**no license file**, so the model is deliberately not committed here — the
fetch script pulls it on demand. It represents the RX480E-4 family rather than
any specific vendor's board, so treat it as indicative, not dimensional truth.

## Fitted pose

`rot (90, 180, 90)`, `offset (-4.1, -12.595, 14.64)`, fitted by eye against the
real part. RECVR sits at 90°, so the offset axes are rotated relative to the
board: `offset.x` moves along board −Y (≈4.1 mm of standoff for the right-angle
pins' horizontal run before they turn down), `offset.y` along board −X (2.54 mm
= one pin pitch), `offset.z` straight up (socket body + header thickness).
