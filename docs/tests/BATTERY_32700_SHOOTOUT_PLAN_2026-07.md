# 32700 LFP shootout: fullbattery 6 Ah vs Palowextra "7.2 Ah" (2026-07)

**Question:** does the Amazon Palowextra 32700 ("7.2 Ah") actually out-deliver the
fullbattery.com 6 Ah production cell? Decision at stake: a ~75-unit buy of the
Palowextra cell. Both specimens are **virgin** (uncycled).

**Prior art this must stay comparable to:** LOG 2026-06-11 (cont. 3) — fullbattery
32700 #1 delivered **5,726 mAh clean to 2.473 V** (INA-coulomb, HEX37 val224 load,
~745→533 mA battery-side, 7.16 h, zero resets). $0.89/delivered-Ah.

## Method (one line)

Full Nitecore charge → fixed HEX37 val224 load → `afk_discharge.py` to 2.5 V cutoff;
truth = external INA219 battery-lead coulomb integral; onboard gauge integral logged in
parallel (known **+8 ± 1 % high**, correct by /1.08). Crossover: two rigs run
simultaneously off ONE 4-channel ina_monitor (2 INA channels per rig), cells swap rigs
between cycles — after 2 cycles each cell has an INA-truth run on each rig, so rig
effects cancel in the comparison and the gauge bias gets checked per-board.

## Hardware

Both rigs fully instrumented from the single KB2040/Metro ina_monitor (one QT chain —
note this commons the two rigs' grounds; fine on battery, and no USB touches either
board mid-run anyway):

| | rig 1 = Ben's "board 1" | rig 2 = Ben's "board 2" |
|---|---|---|
| board | **9E5B0C** (192.168.4.39 on 2026-07-06; fixture_id in /telemetry is ground truth, DHCP may drift) | **9E5AF0** (192.168.4.40) |
| load | NeoHEX 37px on **GPIO10/A0** | NeoHEX 37px on **GPIO10/A0** |
| battery INA (capacity truth) | **0x45** (hub slot 1, a1=1 a0=1), high-side in the VBAT+ lead | **0x40** (slot 3, a1=0 a0=0), high-side in the VBAT+ lead |
| LED-line INA (rail sag / load split) | **0x41** (slot 2, a1=0 a0=1) | **0x44** (slot 4, a1=1 a0=0) |

MAC↔physical mapping verified 2026-07-06 two ways (green-pulse moved the right LED
channel; a mule cell on board 1 appeared at 0x45 bus_v and on 9E5B0C's gauge at once).

The battery-lead channel is the one that measures capacity (it sees LED + ESP/WiFi +
regulator loss — June 11: 745 mA battery-side vs 574 mA LED-side at start). The
LED-line channel is diagnostics: bus_v = rail sag (flicker/goldening onset), and
battery − LED = system draw. Keep shunt insertions short; identical lead dress per rig.

Note on INA `bus_v` in this topology: with rigs battery-only, their grounds don't
reference the monitor's ground, so bus_v reads ~0.16 V low (same artifact visible in the
June 11 data). Harmless — capacity uses shunt current (differential), cutoff uses gauge
battery_v; treat INA bus_v as relative-trend-only.

Cells: label them **F** (fullbattery 6 Ah) and **P** (Palowextra 7.2 Ah) on the wrap.

Bench measurements 2026-07-06 (UMS4 IR readout — seating/contact-sensitive, re-check on
a re-seat before treating as real; kitchen-scale weights):

| cell | claimed | weight | UMS4 IR |
|---|---|---|---|
| F (fullbattery) | 6.0 Ah | 138 g | 60 mΩ |
| P (Palowextra) | 7.2 Ah | 136 g | 136 mΩ |

External datum (Ben, 2026-07-06): YouTube channel **Off-Grid Garage** tested this exact
"Palo LiFePO4 7200mAh" cell ("Palo LiFePO4 Cells with 7200mAh. Any good?") and measured
**~5,450 mAh** — independent support for the overrated-label hypothesis, methodology
similar to ours (cycler discharge to a 2.5 V-class cutoff).

Cycle 1 conditions: started 2026-07-06 ~21:16Z, ambient **79.9 °F / 26.6 °C**, F on
rig 1 / P on rig 2, both runs clean at start (0 resets, gauge/INA ratios 1.077 / 1.038).
Note: F's run doubles as the **second-sample qualification** of the fullbattery batch
(June 11 verdict was n=1; 75 units since purchased).

Priors these set (hypotheses, discharge decides): equal weight at +20 % claimed capacity
would require ~20 % better gravimetric density than a cell already verified at 95 % of
rating — implausible for same-chemistry 32700; and 2.3× IR, if it survives a re-seat, is
a lower-grade-cell tell (~+57 mV extra sag at our 0.75 A → slightly earlier knee and a
weaker capacity-to-3.0 V number).

## Firmware (built & verified 2026-07-06, not yet flashed)

```
./firmware/power_bench/build.sh --led neohex --pixel-pin 10 --cap 6000 --chem lfp \
    --no-charge --batt-floor 2.3 --ota <board-ip>      # (or --port /dev/ttyACMx first time)
```

- `--no-charge`: charging happens ONLY on the Nitecore — hard-excludes the USB-charging
  confound that ruined the 46 h soak integral.
- `--batt-floor 2.3`: firmware default self-protects at 2.90 V; we need the 2.5 V tail.
- `--cap 6000` on both boards: gauge SOC is advisory anyway (LFP plateau); don't reflash
  per cell.
- **LFP flash-order rule:** flash BEFORE connecting the cell. USB flash needs the
  physical-reset tap afterward (RTS/3V3 gotcha); OTA self-reboots.

## Charge protocol (identical every cycle)

1. Nitecore UMS4, **LiFePO4 mode confirmed** (terminates 3.65 V; the display's "3.7" is
   rounding). Record the slot; keep each cell on its same slot all week.
2. Let it terminate fully; **rest ≥ 1 h off the charger**.
3. DMM the rested cell: expect ~3.40–3.45 V. Anything near 4.1 V = Li-ion profile —
   abort and flag. Record rested V each cycle.
4. First session only: record each cell's **weight** and the UMS4 **IR** readout
   (a "7.2 Ah" 32700 notably lighter than the 6 Ah cell is a red flag on its own).

## Run commands

One process owns the monitor's serial port; both rigs tail its file
(`ina_logger.py` + `--ina-file`, added 2026-07-06, offline smoke-tested):
```
./ina_logger.py --port /dev/ttyACM<N> --out data/ca/<date>-ina.log
```
Rig 1:
```
./afk_discharge.py --led-ip <RIG1_IP> --ina-file data/ca/<date>-ina.log \
    --load custom --r 224 --g 224 --b 224 --w 0 --bri 255 --n 37 \
    --cutoff-v 2.5 --site ca \
    --notes "32700 shootout: cell <F|P> cycle <k>, rig1, HEX37 val224, slot <s>, ambient <T> C"
```
Rig 2 (simultaneous, same file, its own channels):
```
./afk_discharge.py --led-ip <RIG2_IP> --ina-file data/ca/<date>-ina.log \
    --batt-ch 0x40 --led-ch 0x44 \
    --load custom --r 224 --g 224 --b 224 --w 0 --bri 255 --n 37 \
    --cutoff-v 2.5 --site ca \
    --notes "32700 shootout: cell <F|P> cycle <k>, rig2, HEX37 val224, slot <s>, ambient <T> C"
```
Start-of-run sanity (2 min in, BOTH rigs): `batt_ina_ma` ≈ `gauge_battery_ma`/1.08
within a few %. A ~10× mismatch = the INA shunt-config bug; a sign flip = leads
reversed. Fix before letting it ride. Runs are ~7–10 h; battery only (no USB).

## Schedule (crossover, both rigs INA-truth)

| cycle | rig 1 | rig 2 |
|---|---|---|
| 1 | **F** | **P** |
| 2 | **P** | **F** |
| 3–4 (optional) | repeat 1–2 | |

After cycle 2 each cell has one INA run per rig: compare per-rig (F@rig1 vs P@rig1,
F@rig2 vs P@rig2) — the two ratios should agree; their spread bounds the rig effect.
Break-in asymmetry is symmetric here (each cell's rig-1 and rig-2 runs are its cycles
1 and 2), so mostly cancels in the cross-rig average; cycles 3–4 only if the gap is
inside the noise. Same ambient temp for paired runs (note it — LFP capacity is
temperature-sensitive).

## Analysis

```
./afk_analyze.py data/ca/<file>.jsonl
```
- **Reconcile before believing**: glitch-ablate corrupt-but-parseable INA samples
  (June 11's raw integral read 10.9 Ah before ablation → 5.7 clean). afk_analyze +
  ingest guards handle the known modes; still eyeball integral vs instantaneous.
- Report per cell: **mAh to 2.5 V** (headline) and **mAh to 3.0 V** (product-relevant —
  lantern firmware floors live at 2.9–3.18 V; a mushy knee can win at 2.5 and lose at 3.0).
- Gauge-only runs: divide gauge integral by the run's own INA-derived bias if available,
  else /1.08.
- Decision metric: $/delivered-Ah (F baseline: $0.89). Note n=1 per supplier — a good P
  specimen justifies a small second order + 2–3-cell qualification, not the 75-unit buy.

## Pre-run wiring check (`ina_mapcheck.py`, added 2026-07-06)

Lights each rig green in turn and verifies the right INA channels move with the right
sign. Run TWICE: once any time (LED channels), once **on battery at cell-connect**
(battery-shunt polarity — a battery channel can't respond while USB-powered/battery-less):
```
./ina_mapcheck.py --ina-port /dev/ttyACM<kb2040> \
    --rig 9E5B0C=192.168.4.39:0x45:0x41 --rig 9E5AF0=192.168.4.40:0x40:0x44
```

## Status (2026-07-06)

- [x] Firmware image builds clean (neohex pin10, LFP, no-charge, floor 2.3)
- [x] `afk_discharge.py`: `--no-ina` + `--ina-file` modes added; `ina_logger.py` tee
      written; two-rig shared-file flow smoke-tested offline (mock board + mock INA
      stream: channel isolation, glitch ablation, cutoff all verified)
- [x] Both boards USB-flashed + verified on WiFi: 9E5AF0 → 192.168.4.40,
      9E5B0C → 192.168.4.39 (`led_option: neohex37`, `battery_type: Generic_LFP`)
- [x] Hex loads confirmed lighting on both (green pulse: supply_ma +70/+82 mA)
- [x] KB2040 up (was a red/black-swapped adapter shorting the QT rail); 4/4 INA219s
      present after the fix, i2c scan clean
- [x] `ina_mapcheck.py` pass 1: DIP map exactly as designed; MAC↔physical resolved
      (board 1 = 9E5B0C, board 2 = 9E5AF0); found board 2's LED shunt REVERSED
- [x] ALL WIRING VERIFIED (2026-07-06): three reversed shunts found & fixed by wire
      swap (board 2 LED 0x44, board 1 batt 0x45, board 2 batt 0x40); final state — both
      batt channels negative on discharge, both LED channels positive under load,
      map confirmed vs green-pulse + gauges
- [x] Cells weighed/IR'd (see table above); re-seat the P cell and re-read IR once
- [ ] F and P on the Nitecore (record slots), rest ≥1 h, DMM rested-V (~3.40–3.45 V)
- [ ] Mule cells (4 Ah / 2 Ah) off the rigs, F → board 1, P → board 2
- [ ] Optional 30 s insurance: `ina_mapcheck.py` on battery with real cells, then cycle 1
