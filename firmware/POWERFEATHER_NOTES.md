# PowerFeather V2 -- app dev notes / gotchas

Practical best-practices for writing firmware that runs on the PowerFeather V2
(ESP32-S3) bench boards. Accumulated from the power_bench + studio work. Read this
before standing up a new sketch -- several of these have cost real bench time.

## The switchable 3V3 header rail must be enabled (GPIO4 / EN_3V3)

The V2's **3V3 header rail** (and anything hanging off it -- LEDs, STEMMA devices)
is gated by a load switch on **GPIO4** (`EN_3V3`, active **HIGH**). If you don't
turn it on, the 3V3 header measures **0 V (GND)** and nothing downstream powers up.

Two ways to enable it:

- **If you run the PowerFeather SDK:** `Board.init(...)` enables 3V3 for you (it also
  enables VSQT and starts Wire1). Nothing else needed.
- **If you DON'T run the SDK** (lightweight LED/UI app -- e.g. `hex_studio`,
  `rgbw_studio`): you are responsible for the rail. Drive it yourself in `setup()`:

  ```cpp
  #define EN_3V3_PIN 4
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // enable the switchable 3V3 header rail
  ```

GPIO4 is an RTC GPIO, so the SDK toggles it through the RTC domain (state can
survive deep sleep). For a simple active-mode app a plain `digitalWrite` is fine.

**Bonus:** because the LEDs sit on this *switchable* rail, `digitalWrite(4, LOW)` is
a **free, zero-extra-parts LED kill-switch** -- exactly the "software-cuttable 3V3"
pixel-power option in TODO. Can't accidentally drain the pack with LEDs.

> Note: **VSQT** (the STEMMA-QT connector's 3V3) is a *separate* switch
> (`Board.enableVSQT()` in the SDK). Enabling the 3V3 header (GPIO4) is not the same
> as enabling VSQT. Know which rail your device is actually on.

## Those two 3V3 rails dominate deep-sleep current -- cut BOTH before sleeping

There are **two** SDK-switchable 3.3 V rails, and **leaving them on through deep sleep
dominates the sleep/idle current** (learned the hard way, 2026-06-09):

- **3V3 header rail** -- `Board.enable3V3(false)` (raw equivalent: `digitalWrite(4, LOW)`).
- **VSQT** (STEMMA-QT 3.3 V) -- `Board.enableVSQT(false)`.

A `--sleep-cycle` build that deep-slept with both rails left ON drained the LFP cell at
~**1.7 %/h** (gauge); cutting both right before `esp_deep_sleep_start()` dropped it to
~**0.5 %/h** -- a **~3-4x reduction** (~ 20 %/night -> ~ 5 %/night). An external INA219 in
the battery lead then showed the true rails-off duty-cycled drain is **sub-mA** -- so small
the LFP fuel gauge couldn't even resolve it on its flat plateau. So in any sleep path:

```cpp
if (pfReady) { Board.enable3V3(false); Board.enableVSQT(false); } // ambient-draw killer
esp_sleep_enable_timer_wakeup(...);
esp_deep_sleep_start();
```

- The switches are **I2C-latched in the power-management domain**, so they **persist
  through ESP32 deep sleep** with no `gpio_hold` gymnastics; `Board.init()` re-enables them
  on the next wake.
- **On V2 the charger/gauge stay alive with VSQT off** (separate power-management I2C) -- so
  you keep telemetry while the rails are down. (On V1, VSQT-off also kills the gauge.)
- For deeper storage the SDK has `enterShutdownMode()` (~1.4 uA, only charger+gauge powered,
  wakes only on a good supply) and `enterShipMode()` (battery-FET disconnect, wakes via the
  QON pin / good supply). Neither is timer-wakeable, so they're for shipping / dead-battery
  modes, not a duty-cycled fixture -- use deep sleep + rail-cut for that.

## If you use the SDK, you MUST set the V2 board flag

The PowerFeather SDK selects the fuel gauge **at compile time**. Build with:

```
-DPOWERFEATHER_BOARD_V2=1
```

Without it the SDK silently falls back to the V1 **LC709204F** gauge; on a V2 (which
has the **MAX17260**) you'll get `InvalidState` for SOC/health/cycles. `power_bench`
has an `#error` guard + sets this in its `build.sh`. The V2 charger/gauge/STEMMA live
on **Wire1 (GPIO47/48)** at 100 kHz -- keep the SDK's bus speed.

## Wire1 at >100 kHz can OPEN YOUR BATTERY SWITCH (2026-07-02/03, hard-won)

The line above ("keep the SDK's bus speed") is not a style preference -- it is
**load-bearing**. Raising Wire1 to 400 kHz (a "measured exception" for sensor
throughput on the presence bench) caused an epidemic of instantaneous
`reset_reason=poweron` collapses on battery: ~60+ across TWO boards and TWO cells,
radio-correlated, USB-immune. Root-caused by controlled A/B (identical firmware,
only the clock changed): 400 kHz died in seconds-to-minutes, 100 kHz ran
indefinitely under a heavier bus load. Best-supported mechanism: Wire1 also
carries the **BQ25628E -- the chip the battery current flows THROUGH** -- and
corrupted transactions under WiFi TX noise can flip power-path register bits
(BATFET / ship / EN_HIZ class), opening the battery path outright. No sag, no
brownout detector, straight to poweron; USB survives because VBUS bypasses the
BATFET. A stray EN_HIZ corruption (board discharging at -290 mA WITH USB
attached) was observed in the same sessions. Full story: LOG 2026-07-02 cont.
5-10, 2026-07-03 cont. 11.

Rules:
- **Never raise the clock on any bus shared with the charger/gauge.** If sensors
  need fast I2C, give them a separate controller on free GPIOs.
- Treat unexplained `poweron` resets on battery (but not USB) as possible
  power-path register upsets, not just "brownout" -- check what shares the bus
  and what clocks it runs at, before probing connectors and cells (we executed
  five hardware suspects first; the bus clock was the killer).
- Related history: the June IS31 brownout was also a shared-power-bus disturbance
  (ADR 0018). The pattern is general: anything that degrades signal integrity on
  the power-management bus can kill VSYS. The custom-PCBA track should give the
  charger/gauge a DEDICATED bus.

## Native USB-CDC: the boot banner (and WiFi IP) only prints on reset

The S3 uses its **built-in USB** as the serial port. Consequences:

- **Opening the serial monitor does NOT reset the chip.** If the app only prints its
  info (e.g. the WiFi IP) at boot, you'll see *nothing* by opening the monitor after
  the fact -- the banner already scrolled past before the port was open.
- To recover the IP **without reflashing**, pulse a reset over the USB-serial lines
  (esptool-style). A quick pyserial one-liner:

  ```python
  import serial, time
  s = serial.Serial('/dev/ttyACM1', 115200, timeout=0.3)
  s.setDTR(False); s.setRTS(True); time.sleep(0.15); s.setRTS(False)
  # then read lines for ~12 s to catch the banner
  ```

- The post-flash **"Hard reset via RTS pin" is sometimes flaky** -- the app may not
  start (no liveness) until a *physical* reset or a serial-open nudge. The chip is
  usually healthy (verify with `esptool flash_id`). This is a known field-reliability
  concern (see TODO "Field reliability"): production reset paths must not depend on
  the JTAG-RTS reset -- prefer software reset (`esp_restart`) + watchdog so a deployed
  lantern never needs a button press.

## Keep LED driver chips OFF the shared charger/gauge I2C bus

The IS31FL3741 on the V2's shared I2C bus browns out the board on battery under WiFi
(IS31-specific; see ADR 0018 + the brownout investigation). **Drive addressable LEDs
direct-GPIO** -- data on a free GPIO (the studios use **GPIO10 / A0**), V+ from the
regulated switchable 3V3 header rail, NOT on the I2C bus. Brownout-safe
by construction. If you must use an I2C LED device, a NeoDriver (SeeSaw) was stable
where the IS31 wasn't, but direct-GPIO is the preferred path.

## Misc pin notes

- **GPIO4** -- EN_3V3 (switchable 3V3 header rail enable, active HIGH).
- **GPIO46** -- onboard user LED.
- **GPIO47 / GPIO48** -- Wire1 (charger / gauge / STEMMA-QT), SDK-owned.
- **GPIO10 / A0** -- free IO; used as direct-GPIO LED data in the studios.

## 8-bit LED dimming + gamma: the low-end dead-zone

Addressable LEDs (WS2812/SK6812) are **8-bit per channel**, which fights gamma
correction at very low brightness -- relevant because the lantern's ambient spec lives
there. With Adafruit's gamma 2.6 curve, `gamma8(input)` is **0 for input 0..23**,
then 1 (24..35), 2 (36..43)... so the bottom ~9% of the range quantizes to **off**, and
gamma-on gives only a few coarse steps low down. Gamma-off exposes PWM 1,2,3... (usable
ultra-dim) but the perceived ramp is non-linear. 8-bit just isn't enough resolution
for *smooth* ultra-dim. Possible fixes when we get to ambient tuning: a dim-floor
(`max(1, gamma8(x))`), a gentler gamma, gamma-on-color-only, or temporal dithering.
See the LOG 2026-06-07 entry; revisit for the ambient look.

## Charging into a missing battery / `maintain` > supply voltage = brownout

Two ways the charger leaves VSYS unpowered and the board crash-loops (USB-CDC up ~1 s then
reset), both seen 2026-06-08:

- **`setSupplyMaintainVoltage` (VINDPM/`--maintain`) set ABOVE the supply voltage.** The
  charger refuses to draw from a source it would have to pull below the maintain setpoint --
  so e.g. `--maintain 5.5` (a solar-panel MPP) on **USB at 4.9 V** makes the charger ignore
  USB entirely (`supply_ma = 0`). With a battery present it just runs/discharges off the
  cell; with **no battery**, VSYS has no source -> brownout loop. Rule: **`maintain` must be
  <= the supply you're powering from** (use ~4.6 V for USB; the panel MPP only for solar).
- **Enabling charging with no battery connected** is the trigger point of that crash. The
  fix is just to connect the cell (or, for firmware robustness, don't `enableBatteryCharging`
  if no battery is detected). Diagnose a 1-s crash-loop by busy-waiting for the port and
  dumping the boot serial -- the last printed SDK line points at the offending step.

Also: changing battery **chemistry** (Li-ion <-> LFP) means flashing the matching
`--chem` FIRST (board on USB, cell *unplugged*), THEN connecting the cell -- `Board.init`
sets the charger termination voltage (LFP ~3.6 V vs Li-ion 4.2 V); charging an LFP under a
Li-ion profile overcharges it.

## MAX17260 won't cold-POR from a deeply discharged cell — it self-recovers on charge

Observed 2026-07-06 (32700 shootout): after a deep discharge to ~2.45 V and an OTA
reboot with the cell at ~2.78 V, the gauge went mute — battery_ma/soc/health/cycles all
error (the same signature as no-cell-attached), while battery_v (charger-side) still
read. The gauge had operated fine down to 2.49 V when *continuously powered*; a cold
boot at ~2.8 V is below its wake threshold. No intervention needed: once the charger's
precharge lifted the cell to ~2.81 V the gauge came back on its own (sibling board woke
at ~2.93 V). If one stays mute after the cell is charged, a 10 s cell re-seat hard-PORs
it. Don't debug "broken gauge telemetry" on a board that just came off a deep drawdown.

## Treat LFP SOC as advisory, not control truth

The MAX17260 current telemetry is useful after calibration, but its percentage SOC is not a
hard state variable during LFP solar tests. LFP's voltage curve is flat through the middle
and steep near the top/bottom, and the gauge can remain badly wrong after capacity/profile
changes or before a full learn cycle.

Observed 2026-06-17 on the Voltaic 5 W ETFE test: the gauge reported ~58% while the cell was
around 3.57-3.58 V and the charger would not pull the panel down to the requested VINDPM.
That is a charge-acceptance / taper signature, not a panel limit. In solar qualification
and production power logic:

- Use battery voltage and current as guardrails, especially near the LFP top and bottom.
- Use corrected battery-current integration for Wh/mAh accounting.
- Use panel-side INA only as a bench/sentinel truth source for panel capability and faults.
- Avoid SOC-only decisions; require voltage/current cross-checks for low-battery and
  "hungry enough for MPP test" decisions.

## Panel-specific MPP and battery-acceptance artifacts

The charger VINDPM/maintain setpoint is a useful crude MPPT actuator, but the curve is
panel-specific and can be distorted by the battery's ability to accept charge.

Voltaic ETFE follow-up (2026-06-29, Oakland late sun, about 15 deg panel tilt):

- P105 5 W ETFE: best observed around `m46`/`m48`; panel-side INA about 3.8-3.9 W,
  charger input about 3.47 W. Raising toward `m52` lost power. This is plausible
  against the P105 datasheet expected Vmp, but the absolute power may still be
  limited by the 2 Ah LFP charge acceptance or CV/taper behavior.
- P126 smaller ETFE: best observed around `m58`; panel-side INA about 1.89 W and
  charger input about 1.66-1.68 W. `m60`/`m62` fell off. This is proportionally
  close to the nominal 2 W rating.

Rules of thumb from these runs:

- Do not assume one fixed VINDPM works for every panel SKU. The P105 preferred a lower
  setpoint than the P126.
- A near-full, warm, high-IR, or small LFP cell can make a panel look weak because the
  charger hits the battery regulation/taper behavior before the panel is actually at
  its limit.
- For fair panel qualification, use a hungry battery that is not in precharge and not
  near the LFP top knee. For the larger production LFP, re-test while the resting cell
  voltage is roughly mid-SOC rather than around 3.55-3.6 V while charging.
- A simple software hill-climber is probably worth the firmware complexity: first-sun
  sweep, then periodic 3-point checks around the last best setpoint, skipping sweeps
  when voltage/current suggest CV/taper or poor charge-acceptance truth.

## OTA A/B rollback: the `verifyOta()` hook is C-linkage

`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in arduino-esp32 3.3.7. Rollback runs via a weak
hook the core calls in `initArduino()` **before `setup()`**: if the freshly-OTA'd image is
`PENDING_VERIFY`, it calls `verifyOta()` -> true => `mark_app_valid` (keep), false =>
`mark_app_invalid_rollback_and_reboot` (revert to last-good). Default returns true, which is
why ordinary OTAs stick. **Validated 2026-06-08:** a `verifyOta()->false` image auto-reverts,
battery-only, no touch.

- **Gotcha:** the hook is **C-linkage** (defined in a `.c` core file). A plain C++
  `bool verifyOta(){...}` is name-mangled, silently does NOT override, and the bad image
  **sticks** (no rollback). Use **`extern "C" bool verifyOta()`**.
- **Limit:** this catches self-test *failure* only. An image that passes `verifyOta()` then
  crashes/hangs LATER (in `setup()`/`loop()`) is already marked valid -> can brick. Robust
  pattern: `verifyRollbackLater()=true` to defer the mark-valid, run extended checks + a
  watchdog, and mark valid only after the image proves stable for N s.
- OTA does NOT update the bootloader (only the app partition); the rollback-capable bootloader
  is written during a full USB flash.

## ESP32 WiFi does not roam between APs (latches one BSSID)

The ESP32 WiFi-STA stack has **no 802.11k/v/r roaming**. It associates to the strongest
BSSID for the SSID **at connect time**, then **stays latched to it** even as that AP's
RSSI collapses and a stronger same-SSID AP (another mesh node) becomes available. Observed
2026-06-08: a board associated indoors, carried to the yard, clung to the weak indoor Eero
and dropped its link while a -46 dBm nearer node sat right there (a scan proved it was
available). 5/6 GHz client devices roamed fine; the S3 (2.4 GHz only) did not.

- **Fix:** force a fresh associate -- a **reset, `esp_restart()`, or `WiFi.disconnect()` +
  `WiFi.begin()`** re-scans and picks the **strongest** beacon. A production
  "re-associate on link-loss / low-RSSI" guard gives cheap roaming. (The net_bench
  maintenance-OTA path already does a fresh `WiFi.begin()`, so it self-selects the best AP.)
- **Scope:** this bit us with a **moving** board. Deployed fixtures are **stationary**, so
  they won't walk away from the AP they associated to -- low field risk, but the
  maintenance AP should still be the **strongest** thing near the tree during an OTA window.

## Don't enable the charger's battery temp-sense without a thermistor attached

`Board.enableBatteryTempSense(true)` flips the BQ25628E's **TS input on** -- the charger
then applies JEITA temperature limits to whatever the TS pin reads. With **no NTC
physically attached** the pin floats out of the plausible-bias window and the charger can
**suspend or derate charging** (the SDK's `getBatteryTemperature()` itself rejects bias
outside [0.1, 0.8] as open/short). So:

- Battery NTC telemetry is **opt-in** in net_bench (`./build.sh --batt-ntc`) -- only build
  with it when the PowerFeather's 103AT thermistor is actually taped to the cell.
- With the NTC attached it's doubly useful: real battery temp in the heartbeat (`btc=`)
  AND hardware charge-temperature protection (LFP charge limits -- the sealed-hat thermal
  question).

## Solar connect/boot under bright sun can latch the charger's input fault

Observed 2026-06-11 (bright hot afternoon, Seeed 3 W panel): with the panel at
open-circuit (~6.0-6.2 V on a hot panel; HIGHER on a cold bright morning), the
BQ25628E rejected the input (`supply_good=false`, zero draw) and stayed at Voc
indefinitely -- and a hand-shade that sagged the panel to ~4.7 V did NOT clear it.
Only a full VBUS removal (panel face-down/unplugged until V collapsed under ~3.9 V)
re-ran input qualification; the charger then engaged and pulled the panel down to
VINDPM, where Voc is never seen again. A connect-order/weather-dependent deadlock:
06-08's weak-light bring-up never hit it.

- **Field implication (100 fixtures, playa):** connecting or resetting a fixture in
  full sun can leave it silently not charging. Mitigations: connect panels shaded /
  face-down; spec the production panel so its COLD-morning Voc clears the charger's
  input window; and keep the firmware guard enabled in every solar/charging image.
- Diagnosis signature: `sv=` ~Voc with `sma=0 sgood=0` (the onboard INA confirming
  zero panel current).

Baseline firmware practice:

- Any Resonance sketch that enables PowerFeather solar/battery charging must include
  `firmware/powerfeather_solar_guard.h` and call `pfSolarGuardInit(...)` after
  `Board.init()` / charger setup.
- Its normal telemetry loop must call `pfSolarGuardTick(...)` with supply voltage,
  supply current, `checkSupplyGood()`, the active maintain/VINDPM setpoint, and whether
  charging is enabled.
- The guard force-sets BQ25628E `REG0x17[0] VBUS_OVP=1` at boot and watches for the
  stuck signature before toggling `EN_HIZ` in `REG0x16` to re-run input qualification.
  This is a project baseline, not an optional test harness feature.
