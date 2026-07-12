# led_sol_bench

Combined RGBW + solenoid bench (2026-07-11): solenoid_demo's strike machinery +
led_studio's RGBW render path, born as the ADR 0029 VBAT-direct feed test and
grown into the instrument that closed it (rail won -- see the ADR amendment).

- LED data D13/GPIO11 (NOT GPIO13 -- that's EN0, SDK-owned), solenoid gate
  D12/GPIO12. Runtime **feed A/B** toggle: A = 3V3 rail + A0, B = VBAT + D13
  (blanks the outgoing module -- pixels latch).
- Runtime wire-order switch (production 4 W RGBW is **RGBW**, not GRBW) and
  `/raw` wire-slot injection for order/dead-die diagnosis.
- `/gndprobe` -- firmware-only floating-ground detector (INPUT_PULLDOWN on the
  data pin; ~100 % high = module return current on the data line). See
  POWERFEATHER_NOTES.
- `/lux` -- VEML7700 on STEMMA-QT (gain 1/8, IT 100 ms), used by
  `ops/bench/ab_lux.py` for the rail-vs-VBAT lux campaign.
- `/probe_strike?ms=N` -- one pulse + supply-node sampling + MSA311 impact peak
  (strike-energy meter), used by `ops/bench/solenoid_vdc_sweep.py` for the
  VDC-tap (solar percussion) sweep.

Build/flash: `./build.sh --port /dev/ttyACMx` (USB) or `./build.sh --ota <ip>`.
Web UI: http://ledsol.local/ -- strike controls, RGBW anims, flash-sync,
feed/order/pin toggles, probes. Coil safety unchanged from solenoid_demo:
esp_timer one-shot + loop failsafe + width clamp + rest gap; gate pinned low
before anything at boot (VBAT/VDC coil supplies have no rail kill).
