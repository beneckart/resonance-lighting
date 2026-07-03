# Vendored: SparkFun VL53L5CX Arduino Library

- Source: https://github.com/sparkfun/SparkFun_VL53L5CX_Arduino_Library
- Commit: 248607060e49d18bdf064c6aa411ffe515993d16 (main, cloned 2026-07-02)
- License: see LICENSE.md in this directory (ST Ultralite Driver license terms
  apply to the vl53l5cx_* files; SparkFun's wrapper is MIT-style -- headers intact).

## Why vendored instead of arduino-cli lib install

`VL53L5CX_NB_TARGET_PER_ZONE` is a ULD **compile-time** macro in `platform.h`. The
presence bench needs 2 targets per zone (bamboo splay near + floor/person far in the
SAME zone -- the self-occlusion question). A `-D` flag cannot reach the library's
translation units reliably and sed-patching the installed library is invisible state,
so the library lives in the sketch's `src/` (Arduino compiles it recursively).

## Local edits (marked with RESONANCE comments)

1. `platform.h`: `VL53L5CX_NB_TARGET_PER_ZONE` 1U -> 2U.
2. `platform.h` output slimming: DISABLE ambient_per_spad, nb_spads_enabled,
   range_sigma_mm, reflectance_percent, motion_indicator. KEEP distance_mm,
   target_status, nb_target_detected, signal_per_spad.
3. `vl53l5cx_api.cpp` `_vl53l5cx_poll_for_answer`: break out with
   VL53L5CX_STATUS_ERROR after the 2 s timeout. ST's loop exits only on the
   expected register value, so a device that goes mute mid-init (NACKs reads)
   spins forever.
4. `vl53l5cx_api.cpp` `vl53l5cx_stop_ranging`: break after the 5 s timeout.
   ST sets `status = ERROR` on timeout but never exits the poll loop, so
   stop_ranging on a device that is NOT ranging (the MCU-stop bit never
   asserts) spins forever -- this was the actual first-bring-up hang
   (2026-07-02): `begin()` succeeded in 2.7 s, then a stop-before-start in the
   sketch's config path wedged the sensor task permanently. The sketch also
   avoids stop-before-start now; the break is defense in depth.

Everything else is byte-identical to the upstream commit.
