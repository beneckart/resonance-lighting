# presence_bench -- 4-sensor presence-sensing comparison rig

Research bench for the interactivity ask (see
`docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md` and the TODO presence
section). Four SparkFun Qwiic sensor modules, co-facing on one rigid board, one
shared Qwiic/STEMMA-QT chain, compared live on a wireless dashboard with a
baseline/delta detection view. Key question it must answer: multi-zone /
multi-object behavior -- especially whether the FoV cone self-occludes on the bamboo
splay when the sensor hangs under the solar-panel overhang pointing straight down,
and whether multi-target zones still see the floor/person PAST the splay.

## Hardware

| Addr | Module | Notes |
|------|--------|-------|
| 0x33 | MLX90640 IR array (32x24) | thermal; works through darkness, no ports needed |
| 0x29 | VL53L5CX ToF imager (8x8) | VENDORED driver in `src/vl53l5cx/` with 2 targets/zone |
| 0x41 | TMF8821 multizone ToF | 3x3/4x4 zones, native 2 objects/zone. NB 0x41 is also an INA219 alt address -- never share a chain with the INA bench |
| 0x52 | XM125 Acconeer radar | runs EITHER the I2C presence app OR the distance app; the sketch probes both (see below) |
| 0x29 | TOF400C / VL53L1X single-zone ToF | the ~$3 original production candidate; XSHUT-gated, see below |

Host: PowerFeather V2 (primary; sensors on the STEMMA-QT/VSQT rail = Wire1
GPIO47/48) or Metro ESP32-S3 (`--board metro`, plain Wire). Fuel gauge 0x36 and
charger 0x6A also live on the PowerFeather bus -- the annotated `/api/i2c_scan`
knows all of them.

## VL53L1X vs VL53L5CX: two chips at 0x29 -> TCA9548A mux

Both ST ToF parts boot at 7-bit 0x29 (the "0x52" in VL53L1X docs is the same
address in 8-bit notation -- NOT a radar conflict). Resolution: a SparkFun
TCA9548A Qwiic mux (0x70), auto-detected at boot.

- Wiring: main chain = MLX + TMF + XM125 + mux; **VL53L5CX -> mux port 0,
  TOF400C -> mux port 1**. BOTH 0x29 residents must live behind ports (a direct
  0x29 device would collide whenever the other's channel opens).
- Firmware selects-before-use (one channel open at a time, single I2C-owner task
  makes this race-free). No address changes, ever.
- Without the mux, the fallback is `.12` behavior: L5CX direct at 0x29, L1X held
  in reset via its XSHUT jumper (A0/GPIO10) and reported as blocked.

**Do NOT enable software relocation** (`PB_VL53_RELOCATE`, default 0): changing
the VL53L5CX's I2C address reproducibly leaves it a zombie until power cycle --
it ACKs at the new address but every register reads 0 and DCI times out -- both
via SparkFun `setAddress()` post-init and via raw ST-equivalent writes at POR.
Known ST community issue; the official multi-sensor recipe needs the LPn pin,
which the SparkFun breakout does not expose to a controllable line. See LOG
2026-07-02 (cont. 2). Un-gated hot-plugging of the TOF400C onto a live bench
also froze the VL53L5CX mid-soak (the collision, live); a mid-session stall
self-heal now recovers that case.

## Bus speed: 100 kHz, NOT negotiable on this hardware

This bench originally shipped with a "measured exception" to POWERFEATHER_NOTES'
"keep the SDK's bus speed (100 kHz)" guidance, running Wire1 at 400 kHz for MLX
frame rate and VL53 blob-upload speed. **That exception was the root cause of the
2026-07-02 battery reboot epidemic**: ~60+ instantaneous poweron collapses across
TWO boards and TWO cells, USB-immune, radio-correlated. Mechanism (best-supported):
elevated-clock traffic on the bus that ALSO carries the BQ25628E charger -- i.e.
the chip your battery current flows through -- gets corrupted under WiFi TX noise,
and a bad transaction near the power path's control registers (BATFET / HIZ class)
opens the battery switch outright. No sag, no brownout detector, straight to
reset_reason=poweron; VBUS keeps the board alive on USB, which is why the bench
only died on battery. A stray EN_HIZ corruption (board silently discharging at
-290 mA WITH USB attached) was observed hours before the epidemic was recognized.

Verdict was by controlled A/B on the same board/cell/firmware with ONLY the clock
changed: 400 kHz died within seconds-to-minutes, every time; 100 kHz ran
indefinitely under the full five-sensor load (LOG 2026-07-02 cont. 5-10 and
2026-07-03 cont. 11 tell the whole elimination story -- sensors, jumper, holder,
cells, SoftAP, crossover band, NVS writes, charge-enable, and both boards were
each exonerated first).

Cost of 100 kHz: sensor cadence ~0.8 -> ~0.6 Hz, VL53 init blob 2.7 -> 9.4 s.
Acceptable. If a future bench truly needs fast sensor I2C, put the sensors on a
SEPARATE bus (second I2C controller on free GPIOs) -- never raise the clock on a
bus shared with the power-management chips. `--i2c-hz` remains only as an
experimental override.

## Charging is OFF on this bench

The sketch calls `Board.enableBatteryCharging(false)`. It is normally a cell-less
USB bench board, and enabling charge into a missing battery brownout-loops
(POWERFEATHER_NOTES "charging into a missing battery"). If you later hang a cell +
panel on it, port the solar-guard pattern from led_studio first.

## Architecture

All I2C (sensor init, reads, battery telemetry round-robin) runs in ONE FreeRTOS
task on core 0 -- the sensor libraries block (MLX `getFrame` waits out both
subpages; TMF `startMeasuring` waits a report period), and a single bus owner needs
no locking. `loop()`/HTTP on core 1 serves cached frames only, so the dashboard
never stalls behind a sensor read. Sensors initialize lazily AFTER the web server
is up, cheap-first (XM -> TMF -> MLX -> VL53-blob), each gated by a bare ACK probe;
missing/wedged sensors are re-probed every 5 s and never block the others.

Detection/baseline logic intentionally lives in the DASHBOARD (browser JS), not
firmware: thresholds tune live, the logger records the same raw frames so any rule
can be re-run offline, and the firmware stays a dumb, trustworthy instrument.

## Build / flash

```
./build.sh --port /dev/ttyACM0     # first flash over USB
./build.sh --ota <ip>              # thereafter
```

Libraries (installed 2026-07-02): `Adafruit MLX90640` (installed --no-deps; BusIO
already present), `SparkFun Qwiic TMF882X Library`, `SparkFun XM125 Arduino Library`
(+ SparkFun Toolkit), `SparkFun VL53L1X 4m Laser Distance Sensor` (NB: its
`begin()` returns 0 on SUCCESS). The VL53L5CX driver is vendored -- do NOT
lib-install it (see `src/vl53l5cx/VENDORED.md`).

## Dashboard: http://presencebench.local/

- PRESENT row: four tiles (side-by-side detection + achieved Hz + err counts).
  Amber NO BASE = capture a baseline first.
- Capture baseline (20-frame per-pixel/zone medians), delta view, occlusion
  hatching (baseline near-target < occl-mm = zone sees its own lantern), and the
  "usable zones N/64" readout -- the splay-occlusion deliverable.
- VL53 panel: tap a zone for T0/T1 target detail (`T0 412mm | T1 2810mm` = splay in
  front, floor behind -- the multi-target payoff). Detection in occluded zones keys
  on the FAR target, so a person still registers past the splay.
- Thresholds are live inputs; baseline + thresholds persist in localStorage;
  "Download baseline" saves a JSON to commit next to logged runs.

## HTTP API

| Endpoint | Purpose |
|----------|---------|
| `/api/state` | small status: per-sensor state/Hz/seq/err, xm app, knob values, battery |
| `/api/frame` | full latest frames, one combined JSON (~7-9 KB; ints: centi-degC / mm) |
| `/api/i2c_scan` | annotated bus scan (runs in the sensor task; handler waits) |
| `/api/set` | `mlx_hz` 1/2/4/8 (subpages/s), `vl_res` 4/8, `vl_hz`, `tmf_map` 1/2/6/7, `tmf_period`, `reinit=<sensor|all>`, `en_<sensor>=0/1` |
| `/update` | standard OTA (led_studio handler) |

`/api/frame` layout notes: VL53 arrays are fixed stride-64 (`d[0..63]` = target 0,
`d[64..127]` = target 1; -1 = no target; only `res*res` zones live). TMF results are
raw `[ch, sub_capture, mm, confidence]` rows; zone index = `sub*9 + ch - 1`
(relevant for the 4x4 time-multiplexed map 7). MLX values are centi-degC.

## XM125 app probe

Both Acconeer I2C apps sit at 0x52 with a shared low register map, so the sketch
(1) tries the presence app -- liveness = measure counter advancing over ~600 ms;
(2) on failure sends RESET_MODULE, waits, tries the distance app (setup +
calibrate + one read). `/api/state` reports `xm_app` 1=presence 2=distance 0=none.
Distance app is a first-class outcome: its 10-peak list separates splay / floor /
person on the radar axis. Range window 200-5000 mm (compile-time constants).
Reflashing the module with the other Acconeer app is out of scope for the bench
(needs the Acconeer exploration tool over the XM125's UART bootloader).

## Host logging

`ops/bench/presence_logger.py --host presencebench.local --label <label>` polls
`/api/frame` into `ops/bench/data/presence/<stamp>_<label>.jsonl` with
meta/frame/state/mark/summary rows. Press Enter during a walk-under to drop a
ground-truth `mark` row (offline latency stats).

## Known limits (v1)

- XM125 presence-app frame rate is the app default; no `xm_hz` knob yet.
- VL53 zone orientation vs the physical mount is unverified (add a flip toggle if
  the first rig session shows mirrored zones).
- TMF 4x4 (map 7) zone-to-geometry mapping is the datasheet's time-mux layout;
  verify with a hand-wave before trusting zone positions.
- MLX effective full-frame rate is about half the subpage rate knob.
