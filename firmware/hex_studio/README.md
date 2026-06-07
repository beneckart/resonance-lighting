# HEX Studio

Interactive aesthetic bench tool for the SK6812 **HEX** 37-pixel board, driven
**direct-GPIO** (off the I2C bus). Serves a phone-friendly web UI for dialing in
the look through the gobo/filter, and reads back the exact current settings so a
good-looking combo can be recorded precisely.

Standalone sketch — intentionally separate from `../power_bench` (which is
brownout/telemetry scaffolding). This one only drives LEDs + serves the UI.

## Wiring

- HEX **data** → `DATA_PIN` (default **GPIO10 / A0**, the validated direct-GPIO
  header on board 2). Override with `--pin N`.
- HEX **power** → 3V3 (dim, safe) or 5V (bright). GND common.

## Flash

```
./build.sh --port /dev/ttyACM0          # USB flash (first time)
./build.sh --pin 16 --port /dev/ttyACM0 # if you wired data to D6/GPIO16
```

Open the serial monitor at 115200 to see the URL it prints. It joins your WiFi
via `wifi_secrets.h` (auto-copied from `../power_bench`); if WiFi fails it starts
a SoftAP `ResonanceHEX` (pw `resonance`) at `http://192.168.4.1`.

## Controls

- **Color**: R/G/B sliders + a color picker; **brightness** slider (0–255, defaults
  low for ambient). **Gamma** toggle for perceptually-smooth low-end dimming.
- **Shape**: Center · +Inner ring · +Two rings · All (concentric hex rings).
- **Animation**:
  - **Spiral** — single lit pixel traces an outward spiral (ring-by-ring, by angle),
    with a **Trail** slider.
  - **Orbit** — single lit pixel circles a chosen **ring** (1/2/3). This is the
    gobo *moving-shadow* test: watch the cast shadow sweep.
  - **Breathe** — sine brightness on the active shape (candle vibe).
  - **Twinkle** — random sparkle within the active shape (wash vibe).
- **Freeze** + **Step +** — pause an animation and advance one pixel at a time to
  park the moving shadow where it looks good, then read off the lit-pixel index.
- **Current settings** readout — rgb/hex, brightness, shape, anim, speed, trail,
  orbit ring, and the lit pixel index. Read these off when something looks right.

## Geometry

The 37-px hex is stored as 7 rows of 4-5-6-7-6-5-4 (center pixel = index 18). The
firmware computes each pixel's ring (hex distance 0–3 from center) and angle at
boot, then derives the spiral order and per-ring angle-sorted member lists. Ring
sizes print over serial on boot (`1/6/12/18`) as a sanity check.
