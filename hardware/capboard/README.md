# Solarnoid cap-bank board (v0.1)

Inline XH-XH pass-through capacitor bank for the striker drive. Drops into the
solar power chain between the Y-splitter leg and the Adafruit 5648 MOSFET
driver, putting the bulk capacitance electrically close to the pulse loop
(cap -> FET -> coil). Carries VDC / D7 / GND straight through.

Generated entirely by `generate_capboard.py` (KiCad 10 `pcbnew` scripting; no
hand layout). Rebuild + verify:

```bash
python3 generate_capboard.py
kicad-cli pcb drc --refill-zones --save-board --severity-error \
    --exit-code-violations -o build/drc.rpt build/capbank.kicad_pcb
kicad-cli pcb export gerbers --layers F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
    -o build/gerbers/ build/capbank.kicad_pcb
kicad-cli pcb export drill --excellon-separate-th -o build/gerbers/ build/capbank.kicad_pcb
```

`build/capbank_gerbers_v0.1.zip` is the fab upload (JLCPCB defaults: 2 layer,
1.6 mm, 1 oz, any color). Zip-tie slots flank each can — lash the can bodies,
not just the leads, before the washboard drive in.

## ⚠ Before ordering

- **Pin order is an assumption**: `PIN_ORDER = 1:VDC, 2:D7, 3:GND` on both J1
  and J2 (pin 1 = leftmost, silkscreen-labeled). Verify against the actual
  Y-cable / driver harness. Fix is a one-line edit + re-run.
- Cap footprint: 18 mm can, oblong drills accept 7.5–8.3 mm lead pitch
  (AliExpress 22,000 uF @ 8 mm and Rubycon 16,000 uF @ 7.5 mm both fit).
- **No blocking diode anywhere** — deliberate. Shade-to-disarm depends on the
  cap back-draining through the panel.

## Manual-fire one-shot (SW1)

`VDC -SW1- R1 470R - C1 4u7 -> D7 line`, R2 330k bleed across C1, R3 10k gate
pulldown, D1 BZX55C3V3 zener clamping the D7 line to 3.3 V.

- Press = single ~20 ms gate pulse (series C1 blocks DC). A stuck or held
  button **cannot** park the coil energized: after the pulse the gate settles
  at ~0.2 V via the 330k/10k divider. Re-arms ~1.5 s after release.
- Pulse width scales with C1 (through-hole, swappable): 2u2 ≈ 10 ms soft tap,
  4u7 ≈ 20 ms, 10u ≈ 40+ ms hard knock. Not locked to any firmware value.
- Zener protects the ESP32 pin (abs max 3.6 V) from the 5.5–7 V bus; MCU
  contention through R1+C1 is transient and < 8 mA.
- Button only works with a lit panel — inherits the shade disarm.
- J3 parallels SW1 for an optional panel-mounted remote button (DNP if unused;
  keep leads short).

## BOM (per board)

| Ref | Part | Note |
|---|---|---|
| J1, J2 | JST B3B-XH-A vertical | pass-through in/out |
| C2–C4 | 22,000 uF 16 V radial, 18 mm | populate 2 or 3 |
| SW1 | 6 mm THT tactile | stiff variant (260 gf) preferred |
| J3 | JST B2B-XH-A vertical | optional aux button |
| R1 | 470 R axial | |
| R2 | 330k axial | |
| R3 | 10k axial | |
| C1 | 4.7 uF ceramic/film disc, 5 mm | sets one-shot pulse width |
| D1 | BZX55C3V3 zener DO-35 | band (K) toward D7 line |
