# KiCad Starter Project

Status: starter PCB layout, not order-ready.

Files:

- `neohex-passive-rev-a.kicad_pro` - KiCad 10 project file.
- `neohex-passive-rev-a.kicad_pcb` - routed starter PCB layout.
- `generate_starter_pcb.py` - reproducible generator for the starter board.
- `fp-lib-table` and `resonance.pretty/` - local footprint library for the
  M5Stack HY2.0-4P SMD candidate.

Important caveats:

- J1 uses the local `resonance:M5Stack_HY2.0-4P_SMD_A118` candidate footprint.
  It is intended to match M5Stack's A118 HY2.0-4P SMD connector, but still
  needs physical verification against the M5Stack NeoHEX/HEX cable before
  ordering boards.
- J2 uses stock `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal`.
- J5 uses stock `Connector_JST:JST_SH_BM04B-SRSS-TB_1x04-1MP_P1.00mm_Vertical`
  as a fallback output for Adafruit 4528-style Grove-to-STEMMA-QT cables.
- Verify J1 cable orientation before ordering. The intended net order is
  `1 GND`, `2 VLED`, `3 DATA`, `4 NC`.
- Verify J5 cable orientation before ordering. The intended net order is
  `1 GND`, `2 VLED`, `3 NC`, `4 DATA`, matching the NeoHEX signal on the
  Grove yellow/SCL-position conductor.
- No schematic has been captured yet; the authoritative electrical intent is
  still the parent directory's `README.md` and `netlist.csv`.
- This first layout is intentionally roomy at 72 mm x 35 mm so the routing and
  connector intent are easy to inspect. Shrink only after the connector family
  and mounting approach are settled.
- 3D view caveat: J1 has no local STEP model yet, and the stock J2 footprint
  references a JST-PH-SM4 STEP model that is missing from the current KiCad
  package. J3/J4/J5 use stock JST-SH models and should appear in 3D view.

Validation commands:

```sh
python3 hardware/led-adapter/neohex-passive-rev-a/kicad/generate_starter_pcb.py
kicad-cli pcb drc --output /tmp/res-neohex-kicad/drc.rpt hardware/led-adapter/neohex-passive-rev-a/kicad/neohex-passive-rev-a.kicad_pcb
kicad-cli pcb export gerbers --output /tmp/res-neohex-kicad/gerbers hardware/led-adapter/neohex-passive-rev-a/kicad/neohex-passive-rev-a.kicad_pcb
kicad-cli pcb export drill --output /tmp/res-neohex-kicad/drill hardware/led-adapter/neohex-passive-rev-a/kicad/neohex-passive-rev-a.kicad_pcb
```
