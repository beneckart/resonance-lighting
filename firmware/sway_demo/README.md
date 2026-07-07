# Sway Demo

Motion-reactive lighting bench demo: an **MSA311 accelerometer** plus a
**VL53L5CX multizone ToF** (both on the STEMMA-QT / Wire1 bus) drive the single
**4 W SK6812 RGBW point source**. Color follows the **sway** (high-passed accel
delta -- the signal a hanging lantern gets from wind), the **tilt** direction,
or both. A built-in web app draws the live sensor state -- including accel tilt
and ToF ground-plane tilt side by side -- so the mapping can be verified by eye
next to the physical light.

## Signal path

```
accel sample (MSA311, +/-4 g, 125 Hz ODR, 50 Hz loop)
  |-- low-pass (~0.4 s)  -> gravity vector -> pitch/roll/tilt vs calibrated rest
  `-- sample - gravity   -> |sway| -> fast-attack / slow-decay envelope

ToF frame (VL53L5CX, 4x4 @ 10 Hz, farthest valid target per zone)
  `-- robust least-squares plane fit -> GEOMETRIC tilt vs ground + height
```

The two tilt estimates answer different questions. Accel tilt is inertial: on a
swinging pendulum the apparent gravity stays aligned with the rope, so it is
mostly blind to swing angle (the pendulum degeneracy). The ToF plane fit is
geometric -- the fixture's orientation relative to the actual ground -- so it
sees swing directly. Comparing the filled dot (accel) and cyan ring (ToF) on
the bubble display is the point of the exercise. Note the two breakouts' axes
are aligned only as well as they are physically squared to each other.

Color mapping (web-selectable):

- **Sway** (default): hue sweeps warm amber -> violet with the sway envelope;
  brightness rises from the base with it. Strong spikes (top 20% of the sway
  scale) flash the RGBW's white die.
- **Tilt**: hue = azimuth of the lean (full color wheel), brightness = how far
  it leans (45 deg = full). Hue holds below 3 deg of tilt (azimuth is noise there).
- **Both**: tilt steers the hue, sway pumps the brightness.

(LED color runs off the accel; the ToF is a verification instrument for now.)

"Sensitivity" sets the sway full scale exponentially, 1.5 g (slider 1) down to
0.03 g (slider 100). "Re-zero tilt" captures the current pose as the rest
reference for BOTH sensors: the accel rest vector (also auto-captured ~1.5 s
after boot) and the ToF's fitted ground plane (auto-captured on the first good
fit). ToF tilt is reported relative to that zeroed mount -- a jury-rigged,
non-level sensor placement reads 0 after a re-zero; the absolute mount angle
stays visible in the ToF info line. The relative tilt is the exact 3D angle
between the current and zeroed ground normals (acos(n . n0)), which is
SPIN-INVARIANT for any mount tilt -- verified synthetically (10 deg swing reads
10.000 deg at every spin phase; the earlier component-subtraction version
fluttered ~0.5 deg with spin at a 15 deg mount). The reported tilt DIRECTION is
in the spinning body frame (no yaw reference on board), so expect the cyan ring
to orbit while the rig twists; its radius is the trustworthy signal. Heavy spin
also puts centripetal acceleration on the accel (~1 g at 3 rev/s a few cm off
axis), so during hard spin the accel bubble and part of the sway envelope are
spin artifacts -- production should mount the accel near the spin axis.

## Wiring

- MSA311 (0x62) and VL53L5CX (0x29) on the **STEMMA-QT connector** = Wire1
  (GPIO47/48). That is the shared charger/gauge bus: it runs at **100 kHz,
  never faster** (POWERFEATHER_NOTES "Wire1 at >100 kHz can OPEN YOUR BATTERY
  SWITCH"). The ToF runs 4x4 (not 8x8) and drops the signal-per-spad output so
  its per-frame read stays short on the shared bus -- this sketch reads sensors
  from loop(), not a dedicated task (see src/vl53l5cx/VENDORED.md).
- RGBW data -> **GPIO10 / A0** (override with `./build.sh --pin N`); V+ from the
  switchable 3V3 header rail (GPIO4, enabled by the sketch); GND.
- **Charging** (since .3): off at boot, then a one-shot guard enables a gentle
  500 mA LFP-profile charge once the gauge reports a plausible cell voltage
  (2.5-4.4 V) -- charging into a missing battery brownout-loops. The solar
  guard (`powerfeather_solar_guard.h`) runs whenever charging is on.

## Build / flash

```
arduino-cli lib install "Adafruit MSA301"   # one-time; also provides MSA311
./build.sh --port /dev/ttyACM1              # USB flash
./build.sh --ota 192.168.4.xx               # WiFi OTA thereafter
```

The VL53L5CX driver is vendored in `src/vl53l5cx/` (copied from
presence_bench; local platform.h edits documented in VENDORED.md).

## Web app

On shared WiFi: **http://swaydemo.local/** (mDNS), or the IP from the serial
banner (115200). SoftAP fallback `ResonanceSway` (pw `resonance`) at
`http://192.168.4.1`. Endpoints: `/` viz + controls, `/state` JSON at ~10 Hz,
`/set` params, `/update` standard OTA.

The viz shows: bubble level (filled dot = accel tilt in the exact LED color,
cyan ring = ToF ground-plane tilt, rings at 10/20/30/45 deg, trail = recent
motion), a pulse ring whose radius is the sway envelope, the ToF zone heatmap
(gray = range, green box = used by the plane fit) with height above ground, a
30 s sway strip chart, and raw numbers. Missing MSA311 -> LED breathes red +
page warning (re-probed every 5 s); missing ToF -> heatmap says so (re-probed
every 30 s; ranging stalls self-heal).
