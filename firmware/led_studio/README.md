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
`updateLength()` -- **37px NEO_GRB** for HEX, **1px NEO_RGBW** for the RGBW. The strip
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
./build.sh --sensor-triad --cap 6000 --charge-ma 500 --maintain 4.6
./build.sh --l5cx --cap 6000 --charge-ma 500 --maintain 4.6   # perimeter HEX demo
```

On shared WiFi the board registers a **per-device** mDNS name
`http://ledstudio-<last-3-MAC-bytes>.local/` (e.g. `ledstudio-9e5ae8.local`;
two boards both claiming plain `ledstudio.local` sent browser control to the
wrong unit, 2026-07-30). Serial monitor (115200) also prints the IP; SoftAP
fallback `ResonanceLED` (pw `resonance`) at `http://192.168.4.1`.

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
- **L5CX presence HEX mode** (`--l5cx`, perimeter demo enclosure): vendored
  VL53L5CX 4x4 @ 10 Hz on Wire1/100 kHz (fixture's trimmed ULD, loop-idiom like
  sway_demo -- begin() only in setup(), quick re-apply self-heal in loop).
  Boots into HEX anim 5 "Presence" at bri=255/gamma-off: all 37 px walk
  ROYGBIV until presence, then a single full-white center pixel -- the
  "dancing gobo" point -- until release. Presence = any of three tests vs a
  learned per-zone scene baseline (slow EMA, adapts only while released;
  "Re-zero scene" button): **near** (valid target <= threshold, default 95 mm,
  slider 40-300), **approach** (>=3 zones closer than baseline by
  max(200 mm, 20%) -- triggers from max range on someone walking up),
  **occlusion** (>=60% of baseline-valid zones lose their target -- a palm
  covering the whole FoV returns non-valid statuses, so "closest target" alone
  would read it as clear). 2-frame enter / 3-frame release; presence
  transitions re-render immediately instead of waiting on the speed-paced
  frame timer. The UI shows a live 4x4 zone grid (mm + trigger coloring).
- **Sensor-triad RGBW mode** (`--sensor-triad`): adds live MSA311, TMF8820, and
  BMP581 readback plus three reactive modes. ToF depth brightens the selected
  RGBW color as a target approaches (the known enclosed 20 mm window/fixture
  return is ignored); Tilt brightens with angle from a re-zeroed rest pose; and
  Elevation maps -1.5 to +1.5 m around a re-zeroed BMP581 pressure baseline from
  dim to bright. TMF8820 one-shots use a cooperative start/process/stop state
  machine instead of the driver's blocking convenience wrapper; all sensor and
  PowerFeather accesses remain single-threaded at Wire1's mandatory 100 kHz. The
  UI reports WiFi RSSI, request latency, TMF result age, and automatic recoveries.
- **Settings readback** for recording good-looking combos.

See `../POWERFEATHER_NOTES.md` for the 3V3-rail / native-USB-reset gotchas.
