# PowerFeather V2 — app dev notes / gotchas

Practical best-practices for writing firmware that runs on the PowerFeather V2
(ESP32-S3) bench boards. Accumulated from the power_bench + studio work. Read this
before standing up a new sketch — several of these have cost real bench time.

## The switchable 3V3 header rail must be enabled (GPIO4 / EN_3V3)

The V2's **3V3 header rail** (and anything hanging off it — LEDs, STEMMA devices)
is gated by a load switch on **GPIO4** (`EN_3V3`, active **HIGH**). If you don't
turn it on, the 3V3 header measures **0 V (GND)** and nothing downstream powers up.

Two ways to enable it:

- **If you run the PowerFeather SDK:** `Board.init(...)` enables 3V3 for you (it also
  enables VSQT and starts Wire1). Nothing else needed.
- **If you DON'T run the SDK** (lightweight LED/UI app — e.g. `hex_studio`,
  `rgbw_studio`): you are responsible for the rail. Drive it yourself in `setup()`:

  ```cpp
  #define EN_3V3_PIN 4
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // enable the switchable 3V3 header rail
  ```

GPIO4 is an RTC GPIO, so the SDK toggles it through the RTC domain (state can
survive deep sleep). For a simple active-mode app a plain `digitalWrite` is fine.

**Bonus:** because the LEDs sit on this *switchable* rail, `digitalWrite(4, LOW)` is
a **free, zero-extra-parts LED kill-switch** — exactly the "software-cuttable 3V3"
pixel-power option in TODO. Can't accidentally drain the pack with LEDs.

> Note: **VSQT** (the STEMMA-QT connector's 3V3) is a *separate* switch
> (`Board.enableVSQT()` in the SDK). Enabling the 3V3 header (GPIO4) is not the same
> as enabling VSQT. Know which rail your device is actually on.

## Those two 3V3 rails dominate deep-sleep current — cut BOTH before sleeping

There are **two** SDK-switchable 3.3 V rails, and **leaving them on through deep sleep
dominates the sleep/idle current** (learned the hard way, 2026-06-09):

- **3V3 header rail** — `Board.enable3V3(false)` (raw equivalent: `digitalWrite(4, LOW)`).
- **VSQT** (STEMMA-QT 3.3 V) — `Board.enableVSQT(false)`.

A `--sleep-cycle` build that deep-slept with both rails left ON drained the LFP cell at
~**1.7 %/h** (gauge); cutting both right before `esp_deep_sleep_start()` dropped it to
~**0.5 %/h** — a **~3–4× reduction** (≈ 20 %/night → ≈ 5 %/night). An external INA219 in
the battery lead then showed the true rails-off duty-cycled drain is **sub-mA** — so small
the LFP fuel gauge couldn't even resolve it on its flat plateau. So in any sleep path:

```cpp
if (pfReady) { Board.enable3V3(false); Board.enableVSQT(false); } // ambient-draw killer
esp_sleep_enable_timer_wakeup(...);
esp_deep_sleep_start();
```

- The switches are **I2C-latched in the power-management domain**, so they **persist
  through ESP32 deep sleep** with no `gpio_hold` gymnastics; `Board.init()` re-enables them
  on the next wake.
- **On V2 the charger/gauge stay alive with VSQT off** (separate power-management I2C) — so
  you keep telemetry while the rails are down. (On V1, VSQT-off also kills the gauge.)
- For deeper storage the SDK has `enterShutdownMode()` (~1.4 µA, only charger+gauge powered,
  wakes only on a good supply) and `enterShipMode()` (battery-FET disconnect, wakes via the
  QON pin / good supply). Neither is timer-wakeable, so they're for shipping / dead-battery
  modes, not a duty-cycled fixture — use deep sleep + rail-cut for that.

## If you use the SDK, you MUST set the V2 board flag

The PowerFeather SDK selects the fuel gauge **at compile time**. Build with:

```
-DPOWERFEATHER_BOARD_V2=1
```

Without it the SDK silently falls back to the V1 **LC709204F** gauge; on a V2 (which
has the **MAX17260**) you'll get `InvalidState` for SOC/health/cycles. `power_bench`
has an `#error` guard + sets this in its `build.sh`. The V2 charger/gauge/STEMMA live
on **Wire1 (GPIO47/48)** at 100 kHz — keep the SDK's bus speed.

## Native USB-CDC: the boot banner (and WiFi IP) only prints on reset

The S3 uses its **built-in USB** as the serial port. Consequences:

- **Opening the serial monitor does NOT reset the chip.** If the app only prints its
  info (e.g. the WiFi IP) at boot, you'll see *nothing* by opening the monitor after
  the fact — the banner already scrolled past before the port was open.
- To recover the IP **without reflashing**, pulse a reset over the USB-serial lines
  (esptool-style). A quick pyserial one-liner:

  ```python
  import serial, time
  s = serial.Serial('/dev/ttyACM1', 115200, timeout=0.3)
  s.setDTR(False); s.setRTS(True); time.sleep(0.15); s.setRTS(False)
  # then read lines for ~12 s to catch the banner
  ```

- The post-flash **"Hard reset via RTS pin" is sometimes flaky** — the app may not
  start (no liveness) until a *physical* reset or a serial-open nudge. The chip is
  usually healthy (verify with `esptool flash_id`). This is a known field-reliability
  concern (see TODO "Field reliability"): production reset paths must not depend on
  the JTAG-RTS reset — prefer software reset (`esp_restart`) + watchdog so a deployed
  lantern never needs a button press.

## Keep LED driver chips OFF the shared charger/gauge I2C bus

The IS31FL3741 on the V2's shared I2C bus browns out the board on battery under WiFi
(IS31-specific; see ADR 0018 + the brownout investigation). **Drive addressable LEDs
direct-GPIO** (off the I2C bus) — e.g. the studios use **GPIO10 / A0**. Brownout-safe
by construction. If you must use an I2C LED device, a NeoDriver (SeeSaw) was stable
where the IS31 wasn't, but direct-GPIO is the preferred path.

## Misc pin notes

- **GPIO4** — EN_3V3 (switchable 3V3 header rail enable, active HIGH).
- **GPIO46** — onboard user LED.
- **GPIO47 / GPIO48** — Wire1 (charger / gauge / STEMMA-QT), SDK-owned.
- **GPIO10 / A0** — free IO; used as direct-GPIO LED data in the studios.

## 8-bit LED dimming + gamma: the low-end dead-zone

Addressable LEDs (WS2812/SK6812) are **8-bit per channel**, which fights gamma
correction at very low brightness — relevant because the lantern's ambient spec lives
there. With Adafruit's gamma 2.6 curve, `gamma8(input)` is **0 for input 0..23**,
then 1 (24..35), 2 (36..43)… so the bottom ~9% of the range quantizes to **off**, and
gamma-on gives only a few coarse steps low down. Gamma-off exposes PWM 1,2,3… (usable
ultra-dim) but the perceived ramp is non-linear. 8-bit just isn't enough resolution
for *smooth* ultra-dim. Possible fixes when we get to ambient tuning: a dim-floor
(`max(1, gamma8(x))`), a gentler gamma, gamma-on-color-only, or temporal dithering.
See the LOG 2026-06-07 entry; revisit for the ambient look.

## Charging into a missing battery / `maintain` > supply voltage = brownout

Two ways the charger leaves VSYS unpowered and the board crash-loops (USB-CDC up ~1 s then
reset), both seen 2026-06-08:

- **`setSupplyMaintainVoltage` (VINDPM/`--maintain`) set ABOVE the supply voltage.** The
  charger refuses to draw from a source it would have to pull below the maintain setpoint —
  so e.g. `--maintain 5.5` (a solar-panel MPP) on **USB at 4.9 V** makes the charger ignore
  USB entirely (`supply_ma = 0`). With a battery present it just runs/discharges off the
  cell; with **no battery**, VSYS has no source → brownout loop. Rule: **`maintain` must be
  ≤ the supply you're powering from** (use ~4.6 V for USB; the panel MPP only for solar).
- **Enabling charging with no battery connected** is the trigger point of that crash. The
  fix is just to connect the cell (or, for firmware robustness, don't `enableBatteryCharging`
  if no battery is detected). Diagnose a 1-s crash-loop by busy-waiting for the port and
  dumping the boot serial — the last printed SDK line points at the offending step.

Also: changing battery **chemistry** (Li-ion ↔ LFP) means flashing the matching
`--chem` FIRST (board on USB, cell *unplugged*), THEN connecting the cell — `Board.init`
sets the charger termination voltage (LFP ~3.6 V vs Li-ion 4.2 V); charging an LFP under a
Li-ion profile overcharges it.

## OTA A/B rollback: the `verifyOta()` hook is C-linkage

`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in arduino-esp32 3.3.7. Rollback runs via a weak
hook the core calls in `initArduino()` **before `setup()`**: if the freshly-OTA'd image is
`PENDING_VERIFY`, it calls `verifyOta()` → true ⇒ `mark_app_valid` (keep), false ⇒
`mark_app_invalid_rollback_and_reboot` (revert to last-good). Default returns true, which is
why ordinary OTAs stick. **Validated 2026-06-08:** a `verifyOta()→false` image auto-reverts,
battery-only, no touch.

- **Gotcha:** the hook is **C-linkage** (defined in a `.c` core file). A plain C++
  `bool verifyOta(){...}` is name-mangled, silently does NOT override, and the bad image
  **sticks** (no rollback). Use **`extern "C" bool verifyOta()`**.
- **Limit:** this catches self-test *failure* only. An image that passes `verifyOta()` then
  crashes/hangs LATER (in `setup()`/`loop()`) is already marked valid → can brick. Robust
  pattern: `verifyRollbackLater()=true` to defer the mark-valid, run extended checks + a
  watchdog, and mark valid only after the image proves stable for N s.
- OTA does NOT update the bootloader (only the app partition); the rollback-capable bootloader
  is written during a full USB flash.

## ESP32 WiFi does not roam between APs (latches one BSSID)

The ESP32 WiFi-STA stack has **no 802.11k/v/r roaming**. It associates to the strongest
BSSID for the SSID **at connect time**, then **stays latched to it** even as that AP's
RSSI collapses and a stronger same-SSID AP (another mesh node) becomes available. Observed
2026-06-08: a board associated indoors, carried to the yard, clung to the weak indoor Eero
and dropped its link while a −46 dBm nearer node sat right there (a scan proved it was
available). 5/6 GHz client devices roamed fine; the S3 (2.4 GHz only) did not.

- **Fix:** force a fresh associate — a **reset, `esp_restart()`, or `WiFi.disconnect()` +
  `WiFi.begin()`** re-scans and picks the **strongest** beacon. A production
  "re-associate on link-loss / low-RSSI" guard gives cheap roaming. (The net_bench
  maintenance-OTA path already does a fresh `WiFi.begin()`, so it self-selects the best AP.)
- **Scope:** this bit us with a **moving** board. Deployed fixtures are **stationary**, so
  they won't walk away from the AP they associated to — low field risk, but the
  maintenance AP should still be the **strongest** thing near the tree during an OTA window.

## Don't enable the charger's battery temp-sense without a thermistor attached

`Board.enableBatteryTempSense(true)` flips the BQ25628E's **TS input on** — the charger
then applies JEITA temperature limits to whatever the TS pin reads. With **no NTC
physically attached** the pin floats out of the plausible-bias window and the charger can
**suspend or derate charging** (the SDK's `getBatteryTemperature()` itself rejects bias
outside [0.1, 0.8] as open/short). So:

- Battery NTC telemetry is **opt-in** in net_bench (`./build.sh --batt-ntc`) — only build
  with it when the PowerFeather's 103AT thermistor is actually taped to the cell.
- With the NTC attached it's doubly useful: real battery temp in the heartbeat (`btc=`)
  AND hardware charge-temperature protection (LFP charge limits — the sealed-hat thermal
  question).
