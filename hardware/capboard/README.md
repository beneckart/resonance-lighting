# Solarnoid cap-bank board (v0.2)

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

`build/capbank_gerbers_v0.2.zip` is the fab upload; `build/capbank_cpl.csv` is
the placement file for JLC assembly (pair it with a BOM csv carrying the LCSC
part numbers you pick). Zip-tie slots flank each can — lash the can bodies,
not just the leads, before the washboard drive in.

## Assembly split (JLCPCB economic PCBA)

All SMD parts sit on the top side only: SW1, R1–R6, C1, C5, D1, and the J1/J2
SMT connectors — those go on the JLC assembly line. Hand-solder per board is
just the 2–3 big caps (your AliExpress stock isn't in the JLC catalog) and the
optional THT connectors J3/J4. Economic assembly typically adds ~2–4 days and
a few dollars per board at this size.

## ⚠ Before ordering

- **Pin order** now matches the reported PowerFeather header (1:D7, 2:VDC,
  3:GND, silkscreen legend on board) — but the harness is hand-crimped, so the
  real rule is: crimp so the wire that lands on PowerFeather GND lands on the
  pin labeled GND here. XH keying prevents reversal after that.
- **J1/J2 MPN**: S3B-XH-SM4-TB (genuine JST SMT right-angle XH) or an LCSC
  clone with the same footprint — verify stock/footprint match before
  submitting assembly.
- Cap footprint: 18 mm can, oblong drills accept 7.5–8.3 mm lead pitch
  (AliExpress 22,000 uF @ 8 mm and Rubycon 16,000 uF @ 7.5 mm both fit).
- **No blocking diode anywhere** — deliberate. Shade-to-disarm depends on the
  cap back-draining through the panel.

## Manual-fire one-shot (SW1)

`VDC -SW1- R1 470R - C1 10uF -> D7 line`, R2 330k bleed across C1, R3 10k gate
pulldown, D1 BZX84C3V3 zener clamping the D7 line to 3.3 V.

- Press = single ~40 ms gate pulse (C1 = 10 uF; the series cap blocks DC). A
  stuck or held button **cannot** park the coil energized: after the pulse the
  gate settles at ~0.2 V. Re-arms ~1.5 s after release. Note ~40 ms means the
  coil is still energized at mallet contact — a slightly damped thud vs the
  firmware's ballistic cut-before-impact strike. Fine for a test/demo button;
  performance strikes belong to firmware.
- C1B is an unpopulated parallel 1206 footprint — solder a second cap there to
  lengthen the pulse, or swap C1 (2u2 ~10 ms, 4u7 ~20 ms, 10u ~40 ms). Exact
  width also depends on the 5648's own input pulldown.
- Zener protects the ESP32 pin (abs max 3.6 V) from the 5.5–7 V bus.
- Button only works with a lit panel — inherits the shade disarm.
- J3 (DNP, THT) parallels SW1 for a panel-mounted remote button.

## Telemetry (J4 -> PowerFeather A4/A5)

Populated by default; ignore it in firmware until wanted.

- **VSNS** = VDC x 33k/133k (÷4.03), 100 nF filter: 7 V bus -> 1.74 V.
  -> **A4 = GPIO2 = ADC1_CH1**.
- **D7S** = gate line through 1 k (already zener-clamped to 3.3 V).
  -> **A5 = GPIO1 = ADC1_CH0**.
- Both are ADC1, so they read fine while ESP-NOW/WiFi is active (ADC2 would
  not). GND returns through the main power harness.
- Uses: intra-strike droop curves (5–20 kHz continuous ADC on VSNS), stall vs
  strike detection from V(t) inflections, counting manual button fires (D7S
  pulses the MCU didn't command), auto-tuning power-cut lead per node.

## BOM (per board)

| Ref | Part | Package | Note |
|---|---|---|---|
| J1, J2 | JST S3B-XH-SM4-TB | SMT right-angle XH 3p | wires exit bottom edge |
| C2–C4 | 22,000 uF 16 V radial, 18 mm | THT | populate 2 or 3, hand-solder |
| SW1 | 6x6 SMD tactile (1TS009 style) | SMD | stiff actuation preferred |
| R1 | 470 R | 0805 | one-shot series |
| R2 | 330 k | 0805 | C1 bleed / re-arm |
| R3 | 10 k | 0805 | gate pulldown (also FET-off at boot) |
| C1 | 10 uF X7R 16 V | 1206 | pulse width (~40 ms) |
| C1B | DNP | 1206 | parallel pulse-width tuning |
| D1 | BZX84C3V3 | SOT-23 | K to D7 line |
| R4 | 100 k | 0805 | VSNS divider top |
| R5 | 33 k | 0805 | VSNS divider bottom |
| C5 | 100 n | 0805 | VSNS filter |
| R6 | 1 k | 0805 | D7S series |
| J3 | JST B2B-XH-A | THT vertical, DNP | aux remote button |
| J4 | JST S2B-XH-A | THT right-angle | to A4 (VSNS) / A5 (D7S) |
