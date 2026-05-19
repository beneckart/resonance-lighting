# KiCad Starter Project

Status: starter PCB layout, not order-ready.

Files:

- `neohex-passive-rev-a.kicad_pro` - KiCad 10 project file.
- `neohex-passive-rev-a.kicad_pcb` - routed starter PCB layout.
- `generate_starter_pcb.py` - reproducible generator for the starter board.

Important caveats:

- J1 is a placeholder `JST_PH_B4B-PH-K` 1x04 2.0 mm footprint, used only
  because the stock KiCad library does not include the exact M5Stack
  Grove/HY2.0 socket footprint.
- Replace J1 with the exact Grove/HY2.0 part footprint and verify cable pin
  order before ordering boards.
- No schematic has been captured yet; the authoritative electrical intent is
  still the parent directory's `README.md` and `netlist.csv`.
- This first layout is intentionally roomy at 60 mm x 35 mm so the routing and
  connector intent are easy to inspect. Shrink only after the connector family
  and mounting approach are settled.

Validation commands:

```sh
python3 hardware/led-adapter/neohex-passive-rev-a/kicad/generate_starter_pcb.py
kicad-cli pcb drc --output /tmp/res-neohex-kicad/drc.rpt hardware/led-adapter/neohex-passive-rev-a/kicad/neohex-passive-rev-a.kicad_pcb
kicad-cli pcb export gerbers --output /tmp/res-neohex-kicad/gerbers hardware/led-adapter/neohex-passive-rev-a/kicad/neohex-passive-rev-a.kicad_pcb
kicad-cli pcb export drill --output /tmp/res-neohex-kicad/drill hardware/led-adapter/neohex-passive-rev-a/kicad/neohex-passive-rev-a.kicad_pcb
```
