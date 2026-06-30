# LED Studio (merged)

Interactive aesthetic bench tool that drives one of three LED options on the **same
data pin** (default GPIO10 / A0), with a UI **mode toggle** to hot-swap between them
-- no reflash:

- **HEX grid** -- SK6812 37px RGB hex.
- **RGBW point** -- single 4 W SK6812 RGBW (has a dedicated white die).
- **RGB point** -- single high-power RGB pixel (same as the RGBW minus the white die).

Supersedes the separate `hex_studio/` + `rgbw_studio/` sketches (kept for reference;
this one is the merged front-end).

## How the hot-swap works

Both modules are SK6812 (same WS2812 protocol + voltage), so a single
`Adafruit_NeoPixel` object is reconfigured at runtime via `updateType()` /
`updateLength()` -- **37px NEO_GRB** for HEX, **1px NEO_GRBW** for the RGBW. The strip
is blanked on every mode switch.

**Mismatched mode is harmless** (both SK6812): worst case is wrong colors or one LED
lighting until refreshed; current stays well under the 3V3 rail's ~1 A. Recommended
swap sequence: **All off -> physically swap the module on the JST -> flip the mode
toggle** to match.

## Wiring / flash

- LED data -> `DATA_PIN` (default **GPIO10 / A0**); power 3V3 (the RGBW runs fine undervolted
  at 3.3 V -- 5 V gives more Vf headroom / peak brightness but is *not* required); GND. The
  sketch enables the V2 switchable 3V3 rail (GPIO4).

> Note: the earlier "abnormally low" RGBW current at 3.3 V was a **measurement bug**, not a
> wiring problem -- `ina_monitor` divided by a 0.1 ohm shunt when the SEN0291 is 0.01 ohm, so it
> under-read 10x (fixed 2026-06-09; corrected full-RGBW draw ~ 290 mA). There is modest *real*
> rail sag under load (LED bus -> ~2.84 V at full RGBW), but **no evidence that lead / in-line
> resistance was ever the culprit** -- flaky DuPont jumpers can misbehave, but these tests don't
> demonstrate it.

```
./build.sh --port /dev/ttyACM1            # USB flash
./build.sh --pin 16 --port /dev/ttyACM1   # if data is on D6/GPIO16
```

Serial monitor (115200) prints the URL; SoftAP fallback `ResonanceLED`
(pw `resonance`) at `http://192.168.4.1`.

## Controls

- **Module toggle**: HEX grid * RGBW point * RGB point. Mode-specific controls
  show/hide accordingly (the W slider + white/warmth presets appear only for RGBW).
- **Shared**: color picker + R/G/B sliders, brightness, speed, gamma toggle, All off.
- **HEX mode**: W slider hidden; shape rings (center/+inner/+two/all); animations
  Spiral / Orbit / Breathe / Twinkle; Trail; Orbit ring; Freeze + Step.
  - **Split RGB** is a separate 3-state modifier (Off / Triad / Rotate) that applies
    on top of Static / Spiral / Orbit / Breathe -- it splits the moving "head" into
    pure R/G/B across three pixels:
    - **Triad** -- a local color-fringe cluster offset from the point (tune with
      Fringe **spread** + **rotate**).
    - **Rotate** -- R at the point, G/B the same point rotated 120 deg /240 deg about the
      grid center (3-fold rotational symmetry; collapses to white at the exact center).
- **RGBW mode**: W (white-die) slider; white/warmth presets + crossfade; animations
  Hue / Breathe / Candle / Fade (with Color B).
- **RGB mode**: same single-pixel color animations as RGBW (Hue / Breathe / Candle /
  Fade + Color B), but no W channel or white/warmth controls.
- **Settings readback** for recording good-looking combos.

See `../POWERFEATHER_NOTES.md` for the 3V3-rail / native-USB-reset gotchas.
