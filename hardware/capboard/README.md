# Solarnoid cap-bank board (v1.0)

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

`build/capbank_gerbers_v1.0.zip` is the fab upload; `build/capbank_cpl.csv` is
the placement file for JLC assembly (pair with a BOM csv carrying LCSC part
numbers). Zip-tie slots flank each can — lash the can bodies, not just the
leads, before the washboard drive in. Four bare 3.2 mm NPTH mounting holes
(M3, silk-ringed) take nylon standoffs; three sit in corners, the fourth at
(3.2, 28.5) because the IN connector housing owns the bottom-left corner.
v1.0 folds Ben's KiCad GUI pass back into the script (component nudges,
caption placement, connector references renamed as labels: IN/OUT x2, RECVR,
TELE-METRY); his hand-edited board is kept at build/capbank_ben_edit.kicad_pcb
for comparison. The script remains the source of truth — GUI edits to
build/capbank.kicad_pcb are overwritten on regeneration unless folded in. The Resonance shell mark (from
`marketing/brand-assets/Logo/Project/Logo7c_shell_black.png`, checked into
this dir as `logo_shell.png`) is rendered to silkscreen by the generator:
small badge on the front, large one on the back. Silkscreen art is free at
any fab — it's just ink on a layer they print regardless.

## Ports (confusion-proof: each role a different connector family)

All along the bottom edge, vertical entry, silkscreen-labeled. The ONLY two
3-pin XH on the board are the daisy-chain pair — impossible to misplug.

| Ref | Connector | Role | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|---|
| IN/OUT (J1) | XH 3p | pass-through in | D7 | VDC | GND |
| IN/OUT (J2) | XH 3p | pass-through out | D7 | VDC | GND |
| TELE-METRY (J4) | XH 2p | telemetry | VSNS -> A4 | D7S -> A5 | — |
| RECVR (J3) | 1x7 0.1" female socket | RX receiver dock / button | see below | | |

- **J4**: 2p-to-2p cable to the PowerFeather A4/A5 header. Plugging it
  reversed is harmless — both lines are ADC inputs, and firmware can
  auto-detect which is which (VSNS reads mid-scale analog; D7S reads
  rail-or-zero). Ground returns via the power harness; because the strike
  loop never crosses the board↔Feather ground path, 2-wire readings keep
  droop *shape*, inflection timing, and ΔV-per-strike intact (absolutes carry
  ~±0.15 V of charger-current offset). For bench-grade absolutes,
  double-crimp a sense-ground wire into the LED header's GND cavity at the
  Feather — a free Kelvin tap, no board pins consumed.
- **J3**: a 7-position female socket the RX480E module plugs straight into —
  no wiring at all. Socket order (silkscreen-labeled, left to right):
  **GND · 5V · D0 · D1 · D2 · D3 · VT**. Only GND/5V/D0 are connected;
  D1–D3/VT land on labeled no-connect positions (jumper-accessible later).
  5V comes from the on-board AMS1117-5.0: min(5.0 V, VDC − 1.1 V) ≈ 3.5–5.0 V,
  always inside the module's 3.3–5 V window (raw VDC floats to ~7 V and would
  cook it). R7 (1k) between D0 and the one-shot protects the receiver's
  output if SW1 is pressed while it's docked; the one-shot's series cap means
  a stuck transmitter can't park the coil. The module mounts vertically
  (right-angle pins); inserted correctly its board overhangs the open space
  above the caps and U1 — the metal RF can's 1–2 mm bump has ample clearance.
  Inserted backwards it visibly blankets the J1 connector: obvious at a
  glance. A dumb 2-wire button
  still works — dupont its leads into the 5V and D0 positions.

## Assembly split (JLCPCB economic PCBA)

Small parts are top-side SMD; the connectors (2x XH 3p, 1x XH 2p, 1x pin
header) are THT — JLC THT assembly or ~11 easy through-hole joints per board
by hand (Ben's call: hand-solder is fine). Hand-soldering beyond that: **2–3
electrolytics** (the AliExpress stock isn't in the JLC catalog). C1B is a
through-hole footprint precisely so field tuning stays iron-friendly.

## ⚠ Before ordering

- **Pin order** matches the reported PowerFeather header (1:D7, 2:VDC, 3:GND).
  Cables are assembled from raw pre-crimped leads into empty housings — press
  them in **straight (pin1->pin1), never mirrored**: a mirrored cable swaps
  D7 and GND and puts the cap bank across VDC–D7 (the gate line).
- **Connector MPNs**: B3B-XH-A (x2), B2B-XH-A (x1), any 1x03 2.54 mm male
  pin header — all ubiquitous.
- **RX480E pin order** (GND, +V, D0, D1, D2, D3, VT) verified against product
  photos; give one physical module a continuity beep on arrival (GND ties to
  the module's ground pour). If a batch ever differs, edit RX_ORDER/RX_LABELS
  and re-run.
- **Why the 480E and not a 4-pin RX470/WL101**: the 470-class part is a raw
  superheterodyne receiver — its DATA pin outputs the demodulated bitstream,
  including every burst of 433 MHz noise (TPMS, weather stations, doorbells).
  Driving the one-shot from it would fire randomly. The 480E's onboard
  EV1527 decoder (learn/pair, per-button outputs) is exactly the part that
  makes MCU-less firing possible; the 470 only makes sense feeding an MCU,
  and our MCU nodes already have ESP-NOW.
- **RF receiver notes** (RX480E-4 class, EV1527 protocol): D0–D3 are the four
  per-fob-button outputs (not channel select); VT asserts on any valid code —
  D0 fires the knock. Buy the *momentary* (M) variant, not latching/interlock.
  Each receiver has a learn button — pair every receiver to ONE fob (press
  learn, press fob button A), so 20 receivers need a single transmitter.
  Solder a ~17 cm wire antenna for range. Idle draw is a few mA from the
  panel — irrelevant by day, and the bus is dead at night anyway. On washboard
  roads a socketed module deserves a dab of hot glue.
- Cap footprint: 18 mm can, oblong drills accept 7.5–8.3 mm lead pitch
  (AliExpress 22,000 uF @ 8 mm and Rubycon 16,000 uF @ 7.5 mm both fit).
- **No blocking diode anywhere** — deliberate. Shade-to-disarm depends on the
  cap back-draining through the panel.

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

## Telemetry (J4 -> PowerFeather A4/A5)

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
| J1, J2 | JST B3B-XH-A | THT vertical XH 3p | daisy in/out |
| J4 | JST B2B-XH-A | THT vertical XH 2p | telemetry |
| J3 | female socket 1x07 2.54 mm | THT vertical | RX receiver dock |
| U1 | AMS1117-5.0 | SOT-223 | 5V* rail for receiver |
| C7 | 10 uF X7R 16 V | 1206 | LDO output cap (same MPN as C1) |
| R7 | 1 k | 0805 | BTN series protection (same MPN as R6) |
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
