# LED Studio (merged)

Interactive aesthetic bench tool that drives one of three LED options on the **same
data pin** (default GPIO10 / A0), with a UI **mode toggle** to hot-swap between them
— no reflash:

- **HEX grid** — SK6812 37px RGB hex.
- **RGBW point** — single 4 W SK6812 RGBW (has a dedicated white die).
- **RGB point** — single high-power RGB pixel (same as the RGBW minus the white die).

Supersedes the separate `hex_studio/` + `rgbw_studio/` sketches (kept for reference;
this one is the merged front-end).

## How the hot-swap works

Both modules are SK6812 (same WS2812 protocol + voltage), so a single
`Adafruit_NeoPixel` object is reconfigured at runtime via `updateType()` /
`updateLength()` — **37px NEO_GRB** for HEX, **1px NEO_GRBW** for the RGBW. The strip
is blanked on every mode switch.

**Mismatched mode is harmless** (both SK6812): worst case is wrong colors or one LED
lighting until refreshed; current stays well under the 3V3 rail's ~1 A. Recommended
swap sequence: **All off → physically swap the module on the JST → flip the mode
toggle** to match.

## Wiring / flash

- LED data → `DATA_PIN` (default **GPIO10 / A0**); power 3V3 (or 5 V for the RGBW's
  full brightness); GND. The sketch enables the V2 switchable 3V3 rail (GPIO4).

```
./build.sh --port /dev/ttyACM1            # USB flash
./build.sh --pin 16 --port /dev/ttyACM1   # if data is on D6/GPIO16
```

Serial monitor (115200) prints the URL; SoftAP fallback `ResonanceLED`
(pw `resonance`) at `http://192.168.4.1`.

## Controls

- **Module toggle**: HEX grid · RGBW point · RGB point. Mode-specific controls
  show/hide accordingly (the W slider + white/warmth presets appear only for RGBW).
- **Shared**: color picker + R/G/B sliders, brightness, speed, gamma toggle, All off.
- **HEX mode**: W slider hidden; shape rings (center/+inner/+two/all); animations
  Spiral / Orbit / Breathe / Twinkle / Split-RGB (fringe spread + rotate); Trail;
  Orbit ring; Freeze + Step.
- **RGBW mode**: W (white-die) slider; white/warmth presets + crossfade; animations
  Hue / Breathe / Candle / Fade (with Color B).
- **RGB mode**: same single-pixel color animations as RGBW (Hue / Breathe / Candle /
  Fade + Color B), but no W channel or white/warmth controls.
- **Settings readback** for recording good-looking combos.

See `../POWERFEATHER_NOTES.md` for the 3V3-rail / native-USB-reset gotchas.
