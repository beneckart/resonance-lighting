# Bench flash log — Mario & Luigi (2026-08-15)

For Ben and Justin, written by lighting-architect at Elliot's direction, BEFORE
the flash so the record exists regardless of outcome. Plain account of what was
done to two fixtures today, why, and exactly what to inspect.

## The two units

| nickname | fixture_id | state when found |
|---|---|---|
| Mario | F40384 | 3.30 V, PROTECT park, 900 s wake cycle, charging 2 A on Mac USB |
| Luigi | F2BDB0 | 2.92 V, PROTECT park, 900 s wake cycle, charging 2 A on Mac USB |

Both hard-plugged into the Mac mini by Elliot as a designated bench-control
pair ("the Mario Brothers OG connections").

## What happened today (timeline of findings)

1. Fleet silent all day to radio commands. Diagnosis chain ran through: stale
   roster knocks -> channel-6 theory (disproved by 3-min ch6 census: zero
   packets) -> day-gate -> targeted-vs-broadcast -> and finally, from the
   lanterns' own serial consoles: **PROTECT park** (power_policy.cpp), entered
   during the deep-discharge days, persisted in NVS, waking 9 s / 900 s.
2. Serial console capture (1 s poll trap on the transient USB windows) showed:
   `battery 2.92V present -> charging ON (2000 mA)` and
   `deep sleep (protect), timer wake 900s`.
3. Same captures PROVED the radio path works: one wake window printed
   `profile -> dev (until reboot)` — acceptance of our NB_PROFILE packet.
   The fleet was never deaf; it is asleep 99% of the time.
4. Elliot's 433 MHz garage remote strikes solenoids fine (hardware path,
   RX480E, bypasses firmware entirely); radio knocks additionally require
   dev profile + day lifecycle + tier FULL (behavior_glue.cpp:67), which
   PROTECT prevents.

## What we flashed and how

- **Artifact:** `fixture.ino.merged.bin`, built locally on the Mac mini with
  YOUR script and YOUR current source: `firmware/fixture/build.sh --channel 11
  --profile prod`, tree = `upstream/main` @ d52d249 (2026-08-14).
  Sketch 1,140,473 bytes (34%). app sha256
  `822ac608547cd59f6e65c373a11f42703026ee6d15bdd54d011d03fec71fd252`.
  Toolchain: arduino-cli 1.5.1 (brew) + esp32:esp32 3.3.11 + PowerFeather-SDK
  2.1.4 + SparkFun TMF882X 1.0.2 + MSA301 1.1.4 + BMP581 1.0.1.
  NOTE FOR BEN: this is NOT your blessed `.2` artifact — it is a fresh build
  of newer source (post-08-11 hardening) on a different bench. If you want
  the fleet uniform, OTA your artifact over it; A/B + rescue USB both remain.
- **Method:** the ESP32-S3's USB only exists during the 9 s wake windows, so a
  1 s-poll trap runs esptool the instant a port enumerates; the ROM bootloader
  cannot sleep, so the session survives past the window. Single session:
  `write_flash 0x0 merged.bin` then `erase_region 0x9000 0x5000` (NVS).
- **Why the NVS erase:** nvs_store.h — the parked fc_led_stage deliberately
  survives reflashes ("an OTA'd production image must not un-park a protected
  unit"). Without the erase the fresh image boots straight back into the park.
  We understand and respect why that guard exists for the FLEET; these two are
  a bench pair on wall power at Elliot's explicit direction, and the erase also
  discards per-unit calibration NVS — re-commission these two before they
  rejoin the production count (registry rows F40384 / F2BDB0 need updating;
  their firmware_rev/sha will no longer match).

## Open questions we could not answer from here

1. Are solarnoid strikers physically FITTED in the current hats? Radio + serial
   command paths are fully verified to the gate; zero audible strikes all day.
2. PROTECT compound-release hold period — observed lanterns re-tier to FULL
   after hours of sun; exact hold parameters would let us predict wake times.
3. Your `.2` artifact for uniform OTA once maintenance WiFi is reachable.

Daemon-side additions (Justin): branch `lighting/dev-mode-endpoints` in the
local cambium clone — /debug/channel, /debug/knock (NB_TARGET_SOLENOID builder,
refuses broadcast per firmware contract), /debug/maint, /debug/rate,
/debug/profile. 341/341 tests green. Yours to review/merge/reject.
