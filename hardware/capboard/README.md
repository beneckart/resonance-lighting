# Solarnoid cap-bank board — v2.0

Strike-drive board for the bamboo-lantern noisemakers. Sits between the solar
panel and the Adafruit 5648 MOSFET driver: takes raw panel voltage in, **boosts
it to ~12 V**, buffers it in a 3 × 22,000 µF bank, and hands that to the
solenoid driver. Force on a reluctance actuator goes as V²/R, so the boost is
worth roughly **4× the strike energy** of the original 6 V design.

100 × 40 mm, 2 layer. **Ordered 2026-08-03: 10 panels of 6×2 = 120 boards.**

## Signal chain

```
panel ──> PANEL ──> MT3608 boost (~12.07 V) ──> 3× 22,000 µF ──> DRIVER ──> 5648 ──> solenoid
                                                     │
PowerFeather D7 ────────── (passes straight through) ┘
```

- **PANEL** and **DRIVER** are *not* the same net. PANEL carries raw panel
  volts (4.6–5.9 V); DRIVER carries the boosted rail. Swapping them is not
  destructive but gives weak knocks.
- **D7** is the ESP32 gate line; it passes through untouched on the back layer.

## Ports (bottom edge, left to right)

| Ref | Connector | Pinout |
|---|---|---|
| PANEL | JST XH 4p right-angle SMT | D7 · VDC · GND · GND |
| RECVR | 1×7 0.1" SMD socket | RX480E dock — G·5V·D0·D1·D2·D3·VT (**unpopulated**, see below) |
| TELE | JST XH 2p vertical THT | VSNS → A4 · BOOST_EN → A5 |
| DRIVER | JST XH 4p right-angle SMT | D7 · VBOOST · GND · GND |

Pin 4 on PANEL/DRIVER is a **second ground** — on DRIVER that splits the ~3 A
strike return across two contacts, which matters since XH is a 3 A series.

The 2-pin TELE is physically incompatible with the 4-pin power ports, so the
two cannot be cross-plugged. Reversing the TELE cable is harmless (both lines
are ADC-capable; firmware can tell them apart).

## Subsystems

- **Boost** — MT3608, 22 µH/2.4 A shielded inductor, SS34. Feedback 130 k/6.8 k
  → 0.6 × (1 + 130/6.8) ≈ **12.07 V**. R12 (THT, DNP) parallels the lower leg to
  trim *upward*; watch the 16 V caps if you push far. **JP1** (DNP) bypasses the
  boost entirely — close it and the board reverts to v1.0 behaviour.
- **EN** — 47 k/47 k divider off VDCIN holds EN at ~VIN/2, so the boost
  **self-enables with nothing plugged in**, and a GPIO on TELE pin 2 can pull it
  low. The GPIO never sees more than ~3.0 V.
- **One-shot button (SW1)** — VDC→SW1→470R→10 µF→D7, zener-clamped. One ~40 ms
  pulse per press; the series cap blocks DC, so a stuck button *cannot* park the
  coil energised (a lit panel would otherwise sustain 2.5–4 W into the coil).
  C1B (THT, DNP) parallels C1 to lengthen the pulse.
- **5 V rail (U1)** — HT7550-1, µA-class quiescent. Replaced the AMS1117, whose
  5–10 mA idle draw was bleeding the cap bank in ~30 s and taxing the harvest.
- **Telemetry** — VSNS = VBOOST × 6.8/53.8 → A4; 47 k/6.8 k with 1 nF gives
  ~27 kHz of bandwidth, enough to actually capture the 25 ms droop transient.

## Fab package (upload these three)

| File | |
|---|---|
| `build/capbank_gerbers_v2.0_4pin.zip` | gerbers + drills |
| `capbank_cpl_4pin.csv` | 23 SMT parts |
| `capbank_bom_smt_4pin.csv` | every line carries an LCSC part number |

Settings: 2-layer FR-4, 100 × 40 mm, green/white, HASL leaded, 1.6 mm, 1 oz,
tented vias, remove mark, **SMT top side only**.

**RECVR is deliberately unpopulated** — no SMD female header has stock anywhere.
The RX dock is optional per node (production triggering is the ESP-NOW clicker),
so its pads stay bare and a socket can be hand-fitted on whichever boards ever
get a 433 MHz receiver.

**Hand-soldered in Nevada City — 8 joints/board:** the 3 electrolytics and the
2-pin TELE. C1B, R12, JP1 are DNP options.

Cap voltage ratings are deliberate and annotated in the BOM: C1/C7/C8 never see
more than ~6 V so 16 V is ample, **C9 is the only part on the 12 V rail and must
stay 25 V.** Letting the picker default to 50 V parts cost $131 on the first quote.

## Workflow — placement is Ben's, routing is scripted

Positions and silkscreen are hand-placed in KiCad and are **authoritative**.
`route_capboard.py` / `route_4pin.py` load the placement file and add *only*
tracks, vias and the ground pour, so routing is reproducible after any
re-arrangement and never fights the layout.

```bash
python3 route_4pin.py                                    # 4-pin (production)
kicad-cli pcb drc --refill-zones --save-board --severity-error \
    --exit-code-violations -o build/drc.rpt build/capbank_4pin.kicad_pcb
```

**Edit `capbank_placement_4pin.kicad_pcb`, never `build/capbank_4pin.kicad_pcb`** —
the latter is regenerated and your changes would be lost. If you do edit the
build output by accident, its positions can be synced back into the placement
file (that is how the v2.0 PANEL/TELE nudges were recovered).

| File | Role |
|---|---|
| `capbank_placement_4pin.kicad_pcb` | **source of truth** — placement + silkscreen |
| `route_4pin.py` | adds copper; regenerates `build/capbank_4pin.kicad_pcb` |
| `capbank_4pin_ben.kicad_pcb` | snapshot of the hand-edited board, for reference |
| `capbank_placement_ben.kicad_pcb` + `route_capboard.py` | 3-pin variant (S3B is out of stock; kept as fallback) |
| `generate_capboard_v10.py.bak` | the v1.0 generator, the board that was actually fabbed first |

## CAD for the plate

`cad/` holds v2.0 STEP/DXF/VRML exports — see `cad/README.md`. Mounting holes
are unchanged from v1.0 at **(3.2, 3.2), (84.8, 3.2), (84.8, 36.8), (3.2, 36.8)**,
so existing plates fit; the board simply overhangs 15 mm past the right pair.
3.2 mm holes take M2.5 or M3.

## Before the next order

- Verify the RX480E module pin order against physical parts if RECVR is ever
  populated (assumed GND · +V · D0 · D1 · D2 · D3 · VT).
- Cables are crimped from raw leads: press them **straight, never mirrored** —
  a mirrored cable swaps D7 and GND and puts the bank across the gate line.
