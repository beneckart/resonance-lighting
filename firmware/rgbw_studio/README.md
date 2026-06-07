# RGBW Studio

Interactive aesthetic bench tool for the single high-power **SK6812 RGBW** pixel
(Adafruit 5163, 4 W), driven **direct-GPIO**. Sibling of `../hex_studio`. The RGBW
is a **point source** (crisp gobo shadows) with a dedicated **white die**, so there's
no geometry here — the interesting axis is **color + temporal modulation**.

## Wiring

- RGBW **data** → `DATA_PIN` (default **GPIO10 / A0**). Override with `--pin N`.
- RGBW **power** → 3V3 (note: the 4 W RGBW is voltage-starved at 3.3 V — dimmer and
  non-linear, see ADR 0018) or **5 V** for full output. GND common.

## Flash

```
./build.sh --port /dev/ttyACM1            # USB flash
./build.sh --pin 16 --port /dev/ttyACM1   # if data is on D6/GPIO16
```

Serial monitor at 115200 prints the URL. Joins WiFi via `wifi_secrets.h`
(auto-copied from `../power_bench`); SoftAP fallback `ResonanceRGBW` (pw
`resonance`) at `http://192.168.4.1`.

## Controls

- **Channels**: R/G/B/**W** sliders + color picker; **Brightness**; **Gamma** toggle.
- **White / warmth presets**: `W only` (pure white die — low-power warm ambient),
  `RGB white`, `RGBW full`, `Warm amber`; plus a **Warmth crossfade** slider that
  blends RGB-white ↔ W to find the nicest white point.
- **Animations**:
  - **Hue cycle** — smooth rainbow (RGB only, W off), speed-settable.
  - **Breathe** — sine brightness on the current color.
  - **Candle** — smoothed random-walk flicker of *your chosen color* (set a warm
    amber first); lantern vibe.
  - **Fade** — crossfades current color ↔ **Color B** (picker).
- **Speed** slider, **All off**, and a **settings readback** (rgbw values, brightness,
  anim, color B) to record a good-looking combo.

## Notes

- Single pixel, `NEO_GRBW`. On 3V3 it's under-volted (dim, non-linear) — fine for
  judging color/shadow geometry; use 5 V for true brightness characterization.
- The W channel is a separate physical die, so W-vs-RGB-white look different through
  the gobo (and W-only is the efficient low-power warm mode, ~80 mA per earlier tests).
