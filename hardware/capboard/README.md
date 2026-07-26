# Solarnoid cap-bank board (v0.3)

Inline XH pass-through capacitor bank for the striker drive. Drops into the
solar power chain between the Y-splitter leg and the Adafruit 5648 MOSFET
driver, putting the bulk capacitance electrically close to the pulse loop
(cap -> FET -> coil). Carries **D7 / VDC / GND** straight through (order per
the PowerFeather 3-pin header — GND on the board-corner pin).

Generated entirely by `generate_capboard.py` (KiCad 10 `pcbnew` scripting; no
hand layout). Rebuild + verify:

```bash
python3 generate_capboard.py
kicad-cli pcb drc --refill-zones --save-board --severity-error \
    --exit-code-violations -o build/drc.rpt build/capbank.kicad_pcb
kicad-cli pcb export gerbers --layers F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,F.Paste,Edge.Cuts \
    -o build/gerbers/ build/capbank.kicad_pcb
kicad-cli pcb export drill --excellon-separate-th -o build/gerbers/ build/capbank.kicad_pcb
kicad-cli pcb export pos --format csv --units mm -o build/capbank_cpl.csv build/capbank.kicad_pcb
```

`build/capbank_gerbers_v0.3.zip` is the fab upload; `build/capbank_cpl.csv` is
the placement file for JLC assembly (pair with a BOM csv carrying LCSC part
numbers). Zip-tie slots flank each can — lash the can bodies, not just the
leads, before the washboard drive in.

## Connectors (all the same part: JST S3B-XH-SM4-TB, right-angle SMT)

All four exit off the bottom board edge. Pin 1 is leftmost; every pin is
silkscreen-labeled.

| Ref | Role | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|
| J1 | pass-through in | D7 | VDC | GND |
| J2 | pass-through out | D7 | VDC | GND |
| J3 | remote fire button | BTN | VDC | GND |
| J4 | telemetry sense | D7S -> A5 | VSNS -> A4 | GND |

J3's button just bridges pin 1 to pin 2; GND is spare (illuminated-button
option). J4's GND gives the ADC sense loop its own return.

## Assembly split (JLCPCB economic PCBA)

Every part except the big caps is top-side SMD, connectors included — all on
the assembly line. Hand-soldering per board: **2–3 electrolytics, that's all**
(your AliExpress stock isn't in the JLC catalog). C1B is a through-hole
footprint precisely so field tuning stays iron-friendly.

## ⚠ Before ordering

- **Pin order** matches the reported PowerFeather header (1:D7, 2:VDC, 3:GND).
  With pre-crimped male-male XH cables, verify the batch is **straight
  (pin1->pin1), not mirrored (pin1->pin3)** — a mirrored cable swaps D7 and
  GND and puts the cap bank across VDC–D7 (the gate line). One continuity
  beep on the first cable of the batch settles it.
- **Connector MPN**: S3B-XH-SM4-TB (genuine JST) or an LCSC clone with the
  same footprint — verify JLC stock before submitting assembly.
- Cap footprint: 18 mm can, oblong drills accept 7.5–8.3 mm lead pitch
  (AliExpress 22,000 uF @ 8 mm and Rubycon 16,000 uF @ 7.5 mm both fit).
- **No blocking diode anywhere** — deliberate. Shade-to-disarm depends on the
  cap back-draining through the panel.
- KiCad's library has no 3D model for the SM4 connector, so renders show bare
  pads there — the fab data (courtyard, paste, copper) is complete regardless.

## Manual-fire one-shot (SW1 / J3)

`VDC -SW- R1 470R - C1 10uF -> D7 line`, R2 330k bleed across C1, R3 10k gate
pulldown, D1 BZX84C3V3 zener clamping the D7 line to 3.3 V.

- Press = single ~40 ms gate pulse (the series cap blocks DC). A stuck or
  held button **cannot** park the coil energized — without this, a lit panel
  would sustain ~1 A (2.5–4 W) into a stuck-on coil and cook it. Re-arms
  ~1.5 s after release.
- ~40 ms means the coil is still energized at mallet contact — a slightly
  damped thud vs the firmware's ballistic cut-before-impact strike. Fine for
  a test/demo button; performance strikes belong to firmware.
- C1B (THT disc, DNP) parallels C1 — solder one in to lengthen the pulse.
  Exact width also depends on the 5648's own input pulldown.
- Button only works with a lit panel — inherits the shade disarm.

## Telemetry (J4 -> PowerFeather A5/A4/GND)

Populated by default; ignore in firmware until wanted.

- **VSNS** = VDC x 33k/133k (÷4.03), 100 nF filter: 7 V bus -> 1.74 V.
  -> **A4 = GPIO2 = ADC1_CH1**.
- **D7S** = gate line through 1 k (already zener-clamped to 3.3 V).
  -> **A5 = GPIO1 = ADC1_CH0**.
- Both ADC1, so they read fine while ESP-NOW/WiFi is active (ADC2 would not).
- Uses: intra-strike droop curves (5–20 kHz continuous ADC on VSNS), stall vs
  strike detection from V(t) inflections, counting manual button fires (D7S
  pulses the MCU didn't command), auto-tuning power-cut lead per node.

## BOM (per board)

| Ref | Part | Package | Note |
|---|---|---|---|
| J1–J4 | JST S3B-XH-SM4-TB | SMT right-angle XH 3p | one MPN, qty 4 |
| C2–C4 | 22,000 uF 16 V radial, 18 mm | THT | populate 2 or 3, hand-solder |
| SW1 | 6x6 SMD tactile (1TS009 style) | SMD | stiff actuation preferred |
| R1 | 470 R | 0805 | one-shot series |
| R2 | 330 k | 0805 | C1 bleed / re-arm |
| R3 | 10 k | 0805 | gate pulldown (also FET-off at boot) |
| C1 | 10 uF X7R 16 V | 1206 | pulse width (~40 ms) |
| C1B | DNP | THT disc 5 mm | parallel pulse-width tuning |
| D1 | BZX84C3V3 | SOT-23 | K to D7 line |
| R4 | 100 k | 0805 | VSNS divider top |
| R5 | 33 k | 0805 | VSNS divider bottom |
| C5 | 100 n | 0805 | VSNS filter |
| R6 | 1 k | 0805 | D7S series |
