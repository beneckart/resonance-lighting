# Iso-Current LED Brightness Test - 2026-05-18

> **Status 2026-07-08: SUPERSEDED, results tables never filled.** Overtaken by the
> PowerFeather V2 bench: the IS31 was ruled out (ADR 0018), the fleet went mixed
> HEX + RGBW by role (ADR 0022), and the equal-conditions comparison eventually
> happened as the 2026-07-02 LED drive matrix (LOG,
> `docs/tests/BOOST_AB_BENCH_REPORT_2026-07-02.html`, ADR 0029). Kept as a record
> of the planned protocol.

Purpose: compare LED candidates at equal measured current, then compare gobo
projection quality at equal exposure/geometry. Current smoke-test modes are
useful for bring-up, but they are not equal-lumen or equal-current states.

Motivation: in current visual smoke tests, full-low brightness appears roughly:

`FeatherS2 Neo >> NeoHEX ~= IS31FL3741 > Atom Matrix`

The Atom Matrix result may be strongly affected by its diffuser/window. This
test is intended to separate electrical efficiency, optical output, diffuser
loss, and gobo washout.

## Boards Under Test

- FeatherS2 Neo built-in 5x5 matrix.
- Atom Matrix built-in 5x5 matrix.
- Atom + M5Stack Unit NeoHEX over Grove.
- Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 matrix over STEMMA-QT.
- Optional: M5Stack Unit HEX/SK6812 if wired later.

## Required Equipment

- SEN0291 I2C wattmeter or equivalent inline voltage/current sensor.
- Fixed power source for the first pass, preferably USB or bench 5 V.
- Repeatable gobo/filter setup with fixed LED-to-gobo distance.
- Fixed camera position and manual exposure/white balance.
- Dark or controlled-light workspace.

## Measurement Setup

Use one board at a time. Put the wattmeter in the LED/load path that best
isolates LED current:

- External LED modules: measure the LED module power rail directly if possible.
- Integrated dev boards: measure whole-board input current, then subtract mode
  `0` baseline current from each LED mode.
- IS31FL3741 over STEMMA-QT: measure board input first, and measure STEMMA/QT
  rail current separately if a breakout cable is available.

For every row, record:

- Supply voltage.
- Baseline current in mode `0`.
- Current in the active LED mode.
- Delta LED/load current.
- Visual brightness notes.
- Gobo notes: crispness, washout, color fringing, diffuser artifacts.

## Electrical Normalization Targets

For each board, tune firmware brightness values to hit these approximate delta
current targets:

| Target | Reason |
| --- | --- |
| 5 mA | Minimum viable ambient / lowest visible state |
| 10 mA | Likely default night ambient budget |
| 25 mA | Brighter sustained candidate |
| 50 mA | Short showy state candidate |
| 100 mA | Upper stress point for LED module comparison, brief only |

Do not force every board to every target if the mode is optically useless or
thermally questionable. Stop early if a module or regulator becomes warm.

## Pattern Set

Measure each target current for these pattern classes where possible:

| Pattern | Description |
| --- | --- |
| Center | One LED closest to optical axis |
| RGB fringe | Three adjacent red/green/blue LEDs |
| Small cluster | 3x3 rectangular crop or 7-LED hex center cluster |
| Full low | All LEDs on at low white |

NeoHEX note: its small-cluster mode should use a 7-LED hex center cluster, not
contiguous LED indices. Current first-pass mapping is `11, 12, 17, 18, 19, 24,
25` around center index `18`.

## Optical Procedure

1. Fix LED-to-gobo distance.
2. Fix gobo-to-projection-surface distance.
3. Disable automatic camera exposure, gain, white balance, and HDR.
4. Photograph each board/pattern/current target.
5. Use the same exposure for all boards at a given current target.
6. Record whether the pattern is crisp, blown out, dim, fringed, or washed.

Acceptance targets:

- Default mode should produce a recognizable, non-washed gobo pattern at the
  intended lantern height.
- Showy modes can be less crisp, but should not erase the patterned aperture
  unless intentionally used as a wash.
- A candidate that is bright but optically unusable should not win on lumens
  alone.

## Results Table

| Board | Pattern | Supply V | Baseline mA | Active mA | Delta mA | Brightness setting | Optical notes |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| FeatherS2 Neo | Center | | | | | | |
| FeatherS2 Neo | RGB fringe | | | | | | |
| FeatherS2 Neo | Small cluster | | | | | | |
| FeatherS2 Neo | Full low | | | | | | |
| Atom Matrix | Center | | | | | | |
| Atom Matrix | RGB fringe | | | | | | |
| Atom Matrix | Small cluster | | | | | | |
| Atom Matrix | Full low | | | | | | |
| Atom + NeoHEX | Center | | | | | | |
| Atom + NeoHEX | RGB fringe | | | | | | |
| Atom + NeoHEX | Small cluster | | | | | | |
| Atom + NeoHEX | Full low | | | | | | |
| C6 + IS31FL3741 | Center | | | | | | |
| C6 + IS31FL3741 | RGB fringe | | | | | | |
| C6 + IS31FL3741 | Small cluster | | | | | | |
| C6 + IS31FL3741 | Full low | | | | | | |

## Follow-Up Firmware Work

- Add runtime brightness/current presets instead of fixed hard-coded smoke modes.
- Add per-board brightness tables once current targets are measured.
- Add a host-side test script that sets mode/brightness, reads SEN0291 values,
  and writes CSV rows.
- Consider a dashboard control for target-current presets after SEN0291 support
  exists.
