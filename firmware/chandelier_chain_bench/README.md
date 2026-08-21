# Chandelier chain bench

PowerFeather V2 diagnostic for a homogeneous daisy chain of the production
addressable point modules on GPIO10/A0 and the switchable 3V3 header rail.

RGBW pixels consume 32 data bits and RGB pixels consume 24. Do not mix the two
types in one chain with this standard NeoPixel driver; downstream framing and
colors will be wrong. Build separately for `--type rgbw` or `--type rgb`.

The diagnostic mode's automatic 34-second sequence repeats:

1. Spatial moving rainbow -- confirms every address and data continuity.
2. Pure red, green, blue, and white fills -- exposes channel order and color
   shift at the far end.
3. Repeating R/G/B/W bars -- makes index/order faults obvious.
4. One-pixel white chase -- locates the first bad link.
5. Full RGBW stress -- maximizes modeled aggregate current for rail-droop and
   glitch testing.

Brightness is automatically capped from the measured module draw so the selected
active count stays at or below the requested LED budget: 290 mA for full RGBW and
257 mA for full RGB. Dedicated RGBW white-die fill uses a conservative
70 mA/module model (measured about 63 mA), allowing white-only chain tests to
reach full scale without weakening the cap on all-channel stress. The default
800 mA LED budget leaves about 200 mA below PowerFeather's 1 A shared board +
3V3 + VSQT rating. A supervised, radio-free bench run may request at most
900 mA; rail saturation is not a production current limiter.
The reboot default is deliberately only 64/255: raise it after observing a full
stable cycle so any brownout returns to a safe baseline instead of looping back
into the stress level.

For a simple stand-alone handoff, `--mode demo` replaces the diagnostic ladder
with a continuous slow rainbow glow and moving wipe. The onboard USER/BOOT
button toggles the pixels and the physical 3V3 LED rail off/on. The animation
resumes after the next press; no host, WiFi, mesh, or configuration is required.
Separate controllers free-run from their own boot time and are not synchronized.

```sh
./build.sh --type rgbw --pixels 4 --brightness 64 --budget-ma 800 --cap 6000 --port COM71
./build.sh --mode demo --type rgb --pixels 9 --brightness 88 --budget-ma 800 --cap 6000
```

Serial at 115200 prints JSON power/pattern telemetry. Commands:

```text
t        telemetry
n<N>     active pixel count, 1..24
b<N>     requested brightness, 0..255 (the current cap remains authoritative)
a        toggle automatic sequence
m<0..7>  manual: rainbow R G B W bars chase stress
+ / -    requested brightness by 16
o        all-off frame then switch the 3V3 LED rail off before chain changes
p        switch the LED rail on and resume after chain changes
```

The firmware avoids reconfiguring GPIO10 after NeoPixel RMT has claimed it.
Calling `pinMode()` on that pin after the first `show()` detaches Arduino-ESP32
3.x RMT and can make a later rail-on cycle remain dark.

For seven lensed RGB modules at static RGB white, the 800 mA default applies
brightness 113/255 (`7 * 257 * 113 / 255` is about 797 mA). The earlier
220 mA/module provisional model was retired after the June 11 measurement was
rechecked; do not use its 149/255 result as a safe long-duration setting.

The sketch configures the LFP charger profile but holds charging off until the
gauge reports a plausible installed cell. This makes the same image safe on a
bare USB bench and with the eventual chandelier battery installed.
