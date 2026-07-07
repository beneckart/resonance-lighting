# Sway Demo

Motion-reactive lighting bench demo: an **MSA311 accelerometer** (Adafruit
STEMMA-QT) drives the single **4 W SK6812 RGBW point source**. Color follows the
**sway** (high-passed accel delta -- the signal a hanging lantern would get from
wind), the **tilt** direction, or both. A built-in web app draws a live
bubble-level + sway-pulse graphic of the sensor state so the mapping can be
verified by eye next to the physical light.

## Signal path (50 Hz)

```
accel sample (MSA311, +/-4 g, 125 Hz ODR, 62.5 Hz BW)
  |-- low-pass (~0.4 s)  -> gravity vector -> pitch/roll/tilt vs calibrated rest
  `-- sample - gravity   -> |sway| -> fast-attack / slow-decay envelope
```

Color mapping (web-selectable):

- **Sway** (default): hue sweeps warm amber -> violet with the sway envelope;
  brightness rises from the base with it. Strong spikes (top 20% of the sway
  scale) flash the RGBW's white die.
- **Tilt**: hue = azimuth of the lean (full color wheel), brightness = how far
  it leans (45 deg = full). Hue holds below 3 deg of tilt (azimuth is noise there).
- **Both**: tilt steers the hue, sway pumps the brightness.

"Sensitivity" sets the sway full scale exponentially, 1.5 g (slider 1) down to
0.03 g (slider 100). "Re-zero tilt" captures the current pose as the rest
reference (also done automatically ~1.5 s after boot).

## Wiring

- MSA311 on the **STEMMA-QT connector** = Wire1 (GPIO47/48). That is the shared
  charger/gauge bus: it runs at **100 kHz, never faster** (POWERFEATHER_NOTES
  "Wire1 at >100 kHz can OPEN YOUR BATTERY SWITCH").
- RGBW data -> **GPIO10 / A0** (override with `./build.sh --pin N`); V+ from the
  switchable 3V3 header rail (GPIO4, enabled by the sketch); GND.
- Charging is **OFF** in this sketch (bench demo, often cell-less on USB). Port
  the led_studio charger config + solar guard before enabling it.

## Build / flash

```
arduino-cli lib install "Adafruit MSA301"   # one-time; also provides MSA311
./build.sh --port /dev/ttyACM1              # USB flash
./build.sh --ota 192.168.4.xx               # WiFi OTA thereafter
```

## Web app

On shared WiFi: **http://swaydemo.local/** (mDNS), or the IP from the serial
banner (115200). SoftAP fallback `ResonanceSway` (pw `resonance`) at
`http://192.168.4.1`. Endpoints: `/` viz + controls, `/state` JSON at ~10 Hz,
`/set` params, `/update` standard OTA.

The viz shows: bubble level (dot = pitch/roll vs rest, rings at 10/20/30/45 deg,
trail = recent motion), a pulse ring whose radius is the sway envelope, a 30 s
sway strip chart, raw accel / tilt / sway numbers, and the exact RGBW values the
LED is showing (the dot is painted that color; a white core = W-die flash). If
the MSA311 is missing the LED breathes red and the page shows a warning
(re-probed every 5 s -- STEMMA hot-plug friendly).
