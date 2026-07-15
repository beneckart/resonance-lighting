# net_bench -- ESP-NOW networking feasibility bench

Throwaway-friendly firmware to de-risk basing ~100 fixtures (fleet now plans ~150 --
ADR 0024) on PowerFeather V2 by
validating ESP-NOW comms, range, OTA, watchdog, and battery stability on 5 boards.
Forked from `../power_bench` (reuses telemetry, OTA `/update`, autosleep guard).
See `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` for the full test plan.

## What it does

- **Roles** (`--role master|peer`): the **master** broadcasts `SHOW_FRAME` commands
  AND WiFi-STA-joins the bench AP to **bridge per-peer stats to the host** over
  UDP:54321; **peers** run pure ESP-NOW on battery and broadcast `HEARTBEAT`
  (id, seq, battery, downlink-PDR/RSSI, CA state).
- **Broadcast-only, unencrypted** ESP-NOW (`FF:FF:FF:FF:FF:FF`) -- the 100-node-
  scalable pattern (encrypted peers cap at ~17). Per-source seq tracking -> PDR.
- **Maintenance mode**: master broadcasts an ESP-NOW `ENTER_MAINT` *metadata* packet
  -> peers switch to WiFi-STA and serve `/update` + `/telemetry` for **standard WiFi
  OTA** (ADR 0010 -- never firmware over ESP-NOW). Auto-resume timeout backstops.
  Peers now report a power preflight before leaving ESP-NOW; by default this is
  advisory so the low-voltage OTA boundary can be measured instead of guessed.
- **Watchdog** (`esp_task_wdt`, net-new) + `--wdt-hangtest` to prove auto-recovery.
- **Autosleep** reboot-loop breaker (`--autosleep`) for unattended battery runs.

## CRITICAL: channel must match the AP

ESP-NOW only reaches nodes **on the same WiFi channel**, and the master's channel is
forced by the AP it joins. So **`--channel` MUST equal the bench AP's channel** on
every board. The master prints `WiFi.channel()` on boot and **warns loudly** on a
mismatch (validated: on a ch-11 AP with `--channel 6`, every send fails with
`Peer channel is not equal to the home channel`). Find the AP channel and build all
boards with that value (e.g. the home AP "BubbyNet" is on channel 11 -> `--channel 11`),
or set a dedicated bench AP/router to a fixed channel.

## Build / flash

```
# all 5 boards on the AP's channel (example: AP on ch 11)
./build.sh --role master --channel 11 --port /dev/ttyACM0   # 1 master (also the host bridge)
./build.sh --role peer   --channel 11 --port /dev/ttyACM1   # 4 peers
# OTA (node must be in maintenance mode first):
./build.sh --role peer   --channel 11 --ota 192.168.4.61

# deprecated/emergency single-board fallback only:
# peer starts its own temporary AP in maintenance mode instead of joining bench WiFi
./build.sh --role peer   --channel 11 --maint-ap --port /dev/ttyACM1
```

Recover the IP/banner via the pyserial RTS pulse (native USB-CDC; see
`../POWERFEATHER_NOTES.md`).

### build.sh flags
Default maintenance OTA requires `wifi_secrets.h`. `build.sh` reuses an existing
gitignored secrets file from `../power_bench/` or `../led_studio/` when
`firmware/net_bench/wifi_secrets.h` is absent.

`build.sh` uses a unique temporary Arduino `--build-path` per run to avoid the ESP32
Arduino cache collision seen when compiling multiple variants in parallel. If you call
`arduino-cli` directly, also provide a unique `--build-path` or build variants
sequentially.

`--role master|peer`, `--channel N` (= AP channel), `--frame-hz N` (master rate, 0 =
pure bridge), `--hb-hz N` (peer rate), `--jitter-pct N`, `--wdt-s N`, `--wdt-hangtest`,
`--maint-timeout S`, `--start-maint`, `--autosleep`/`--budget-mah`/`--wake-s`,
`--wifi-lowpower`, `--chem 3v7|lfp`, `--cap MAH`, `--charge-ma`/`--no-charge`/`--maintain`,
`--serial-bridge`, `--maint-ap`, `--scan-report`/`--scan-s S`/`--scan-max N`,
`--field-cycle`/`--field-charge-s S`/`--field-wait-s S`/`--field-protect-s S`,
`--field-led-load`/`--field-led-spiral-rgb`/`--field-led-rgbw`/`--field-led-frame-ms MS`,
`--solenoid-d7` (targeted, fail-safe D7/GPIO37 strike control),
`--batt-ntc` (battery
thermistor on charger TS -- ONLY with the NTC physically taped to the cell, see
POWERFEATHER_NOTES), `--port`/`--ota`.

Maintenance-entry power preflight defaults to report-only (`NB_MAINT_POWER_ENFORCE=0`).
The advisory thresholds are `NB_MAINT_MIN_LFP_MV=3200`, `NB_MAINT_MIN_3V7_MV=3600`,
and `NB_MAINT_SUPPLY_OVERRIDE_MA=250`. A peer below the advisory floor sends one last
ESP-NOW heartbeat with `mt=2` before attempting maintenance, and the dashboard shows
`OTA power warn`. Only build with `-DNB_MAINT_POWER_ENFORCE=1` after the real lower
bound is measured.

The live master command `m<v10>` sets charger maintain/VINDPM in volts x10 across the
fleet and accepts the PowerFeather SDK range 4.0-16.8 V, e.g. `m46` for 4.6 V or `m71`
for a 7.1 V panel MPP.

The live master command `S` parks peers in timed deep sleep for 6 hours by default
(`S900` = 15 minutes). Peers cut the switchable 3V3/STEMMA rails before sleeping and
wake by timer, preserving no-touch recoverability for the next maintenance window.

The live master command `R<hz>` directly sets the ESP-NOW heartbeat/frame rate across
the fleet, e.g. `R1` for low-overhead charging telemetry or `R5` for faster bench
feedback. This changes transmission cadence only; the peer remains awake. For a bigger
charge-rate win on a near-empty solar peer, use targeted park.

The live master command `P<id>[:seconds]` parks one peer in timed deep sleep while other
peers keep running, e.g. `P9E5AB8:3600` gives the outdoor solar peer a one-hour charge
nap without stopping an indoor HEX drawdown. Peers cut switchable rails, wake by timer,
and rejoin ESP-NOW.

The live master command `D<id>[:mah]` starts a **targeted HEX drawdown** on one peer,
then the peer blanks the pixels and timed-sleeps when it reaches the mAh budget or the
LFP voltage guard. Example: `D9E5AF0:3500` lights the 37px HEX on GPIO10 at the
bench-default load, integrates discharge current, stops around 3500 mAh or the guarded
voltage floor, and sleeps 12 hours so the battery is still hungry for a next-day solar
run. Use the target id whenever more than one peer is online.

Battery capacity and charger current are runtime bench config. Bare commands are fleet
broadcasts; include a peer id to target one node:

- `C<mah>` stores battery capacity in all peers' NVS, then peers reboot so the next
  `Board.init()` uses the new gauge capacity. Example: `C6000` for a fleet of 32700
  LFP cells. `C<id>:<mah>` targets one peer, e.g. `C9E5AB8:6000`.
- `G<mA>` stores and live-applies the charger current cap across the fleet. Example:
  `G1500` for supervised 6 Ah solar runs. `G<id>:<mA>` targets one peer, e.g.
  `G9E5AB8:1500`. Valid range is 40-2000 mA.

The dashboard intentionally sends capacity and charge changes only to the selected peer;
All view refuses those actions so a row click cannot accidentally become a fleet command.

Chemistry is still a build-time flag (`--chem lfp|3v7`) because that controls charge
voltage and is safety-critical. The `--cap` and `--charge-ma` build flags remain useful
as first-boot defaults; NVS overrides win after a command.

Shared-WiFi maintenance can be fleet-wide or targeted. Bare `U` remains the sustained
fleet `ENTER_MAINT` wake and is also the migration path for old peers that do not yet
understand targeted maintenance. `U<id>` sends sustained targeted maintenance to one
peer, e.g. `U9E5AB8`, so a single OTA does not pull drawdown or solar-cycle peers off
ESP-NOW. The dashboard's `Peer maint` button sends `U<id>` for the selected peer.

### Optional D7/VDC solenoid strike

`--solenoid-d7` enables manual, targeted solenoid strikes on PowerFeather D7/GPIO37
(D7 is not GPIO7). The intended wiring is an Adafruit #5648 MOSFET driver's V+ and
GND tapped from VDC/GND, with its SIGNAL input on D7. The driver board's onboard 10K
signal pulldown and flyback diode must be present; do not drive a coil from the GPIO.
Because VDC is always live, this build drives D7 LOW before board initialization and
forces it LOW for OTA, maintenance entry, and sleep.

The serial bridge command is `K<id>:<ms>`, for example `K9F2690:40`. Only the named
peer responds. Pulse width is clamped to 5-300 ms, an 80 ms rest gap rejects repeated
requests, and both an `esp_timer` one-shot and a loop deadline force the gate LOW. The
dashboard exposes the same operation as `Strike D7`; the default is 40 ms. There is no
automatic boot strike or repeat mode. `/telemetry` reports `solenoid_enabled`,
`solenoid_pin`, gate state, strike/block/failsafe counters, and the last pulse width.

The July 2026 P126 trial intentionally begins without a VDC storage capacitor to learn
whether a bright-sun strike works directly from the panel/charger input. Treat a weak
strike, input collapse, or peer reset as the result of that experiment; do not increase
the pulse limit to compensate. Add and size the capacitor from measured strike data.

July 14 qualitative follow-up: the no-capacitor panel strike was weak, while a 10,000 uF,
16 V electrolytic placed across V+/GND at the USB-C-to-XH panel adapter produced a
dramatically stronger kick. This makes VDC + local storage the leading candidate, not a
locked production circuit. Still capture voltage droop/recharge, resets/BQ faults,
hot-plug inrush, repeated strikes, residual charge at dusk, cold-Voc rating margin, and
mechanical retention before freezing the capacitor or harness.

## Field-cycle day/night lifecycle mode

`--field-cycle` is the first production-ish lifecycle test mode. It keeps the normal
ESP-NOW heartbeat, supply telemetry, shared-WiFi OTA, and serial-bridge dashboard, but
adds an autonomous peer state machine:

1. If external supply/solar is present, the peer enters `charge`, emits telemetry, cuts
   both switchable rails, and timer-sleeps in chunks while the charger works.
2. When voltage/current indicate "full enough" (`NB_FIELD_FULL_MV` plus taper current),
   it enters `wait-dark` and keeps sleeping/checking until supply disappears.
3. In dark/no-supply, it enters `draw` and stays awake at the field-cycle heartbeat rate
   (`NB_FIELD_DRAWDOWN_HZ`, default 1 Hz). This intentionally uses the always-awake radio
   load as a repeatable night drawdown. `--field-led-load` optionally adds a direct-GPIO
   SK6812 load during this draw phase; tune it with `--drawdown-lit` and
   `--drawdown-brightness`. The default is the GRB HEX. `--field-led-rgbw` selects one
   production 4 W RGBW point source in the measured RGBW wire order.
4. At `NB_FIELD_DIM_MV` it latches the draw load to the configured dim brightness. At
   `NB_FIELD_LOW_MV` it requires `NB_FIELD_LOW_CONFIRM_S` seconds of sustained low
   voltage before entering `protect`; `NB_FIELD_CRITICAL_MV` remains an immediate hard
   backstop. Before sleeping it sends final heartbeats, blanks pixels, cuts both rails,
   and enters `protect` timer sleep. By default `protect` is latched until a timer wake
   sees real battery charge current from solar/USB; it no longer retries drawdown just
   because resting voltage rebounded in the dark.

`--field-led-spiral-rgb` selects the production-HEX draw profile copied from LED
Studio: one anchor spirals center-to-edge-to-center while pure red, green, and blue
pixels stay at symmetric 120-degree offsets. Use `--drawdown-brightness 255` for
three full-brightness single-channel LEDs. Field-cycle battery current, mAh, and Wh
are corrected at acquisition by `/1.08` for the replicated MAX17260 +8% bias;
`/telemetry` exposes corrected `battery_ma`, `battery_ma_raw`, and
`battery_current_divisor` for auditability. Supply-side telemetry remains the
charger-input measurement and is not given the MAX17260 correction.

`--field-led-rgbw` fixes the configured strip to one pixel, selects `NEO_RGBW`, and
drives `R=G=B=255, W=0` at the requested global brightness. This is the production
downlight full-RGB ceiling, not all four dies full. The normal rail-off startup,
four-step ramp, DIM retry, and PROTECT behavior remain unchanged.

Solar does not need to electrically wake the ESP32 in the normal case: the charger keeps
charging while the ESP32 is in timer deep sleep, and the next timer wake observes the
supply. This is more recoverable than deliberately running the cell into protection.

Example peer build for the current BubbyNet/channel-11 field rig:

```
./build.sh --role peer --channel 11 --field-cycle \
  --field-charge-s 300 --field-wait-s 300 --field-protect-s 900 \
  --field-wake-ms 8000 --field-cold-ms 30000 \
  --field-dim-mv 3000 --field-dim-brightness 64 \
  --field-low-mv 2950 --field-critical-mv 2900 --field-low-confirm-s 60 \
  --field-led-load --drawdown-lit 18 --drawdown-brightness 128 \
  --chem lfp --cap 6000 --charge-ma 1500 --maintain 4.6
```

For the July 14 P105/RGBW ceiling run, replace the HEX load line with:

```
  --field-led-load --field-led-rgbw --drawdown-lit 1 --drawdown-brightness 255
```

The heartbeat tail adds `fc=` (phase), `fcr=` (reason), `fcc=` (cycle), `fce=` (phase
elapsed seconds), `fcchg=`/`fcdis=` (rough charge/discharge mAh integrated from sampled
battery current), and `fcmin=`/`fcmax=` (cycle voltage bounds). The dashboard and
`net_bench_log.py` parse these fields. `fce` resets at each phase transition, while
`fcchg`/`fcdis` and the Wh counters are cycle-total. For a night-only result, subtract
the counters at the DRAW boundary; do not divide an absolute cycle counter by `fce`.

`net-bench-2026-07-01.1` adds field-cycle v2 summaries while keeping the heartbeat at
128 bytes for old-bridge compatibility:

- `fcwhc=` / `fcwhd=`: cycle charge/discharge Wh x10 from sampled battery power.
- `fcpw=`, `fcbw=`, `fcdw=`: peak panel, battery-charge, and battery-draw W x100.
- `fclow=`: current soft-low debounce seconds, capped at 255.
- `fcmchg=`, `fcmwait=`, `fcmdraw=`, `fcmprot=`: time spent in each field-cycle phase,
  in minutes, capped at 255. The `/telemetry` JSON exposes the same phase totals in
  seconds for an OTA-maintenance check.

`net-bench-2026-07-08.1` aligns the field-cycle low-voltage behavior with ADR 0023's
measured 32700 LFP curve for the current HEX-load bench:

- `NB_FIELD_DIM_MV` defaults to 3000 mV loaded and halves the field-cycle HEX load by
  default (`--field-dim-brightness` can override).
- `NB_FIELD_LOW_MV` defaults to 2950 mV loaded and is confirmed for 60 s before protect.
- `NB_FIELD_CRITICAL_MV` defaults to 2900 mV loaded and enters protect immediately.
- Protect is latched until `NB_FIELD_RECOVER_CHARGE_MA` of battery charge current is
  seen on a timer wake. `--field-protect-retry-dark` restores the older stress-test
  behavior where resting-voltage rebound can restart drawdown in the dark.
- The bridge line appends `fcdim=` and `fclat=`, also exposed in `/telemetry` as
  `field_load_dimmed` and `field_protect_latched`.

`net-bench-2026-07-12.1` makes the field LED load fail-safe across resets that erase
RTC state without abandoning the remaining battery after the first full-load collapse.
This guard is automatic for `--field-cycle --field-led-load` builds:

- Before energizing the LED rail, firmware persists an NVS session stage: idle, full,
  dim, or protect. Verified solar recovery clears it.
- A power-on, brownout, panic, or watchdog from the full stage consumes exactly one
  retry before rail-on and restarts through the staged ramp at dim brightness. A reset
  from dim, or any reset from protect, keeps 3V3 off and hard-parks until verified
  charge. Thus POR cannot recreate the old full-power loop, while the first event does
  not automatically strand the remaining Ah.
- Boot drives the pixel data and EN_3V3 low before PowerFeather initialization, parks
  the SDK's cold-init rail enable immediately, and initializes ESP-NOW before the field
  load. A deliberate start waits 1 s, clears the newly enabled rail, and ramps brightness
  in four steps over 400 ms. A <=2.95 V sample during that ramp parks immediately; a
  sample below the configured dim threshold selects the dim target. The P105 deployment
  uses 3.10 V with a 10 s steady-state confirmation; protect remains 2.95 V / 60 s and
  critical remains 2.90 V immediate.
- `/telemetry` adds `field_session_stage`, `field_session_marker`,
  `field_interrupted_boot`, `field_interrupted_retry`, and `field_interrupted_park`.
  Existing bridge logs identify the parked state through `field_phase=5`,
  `field_reason=8`, and `field_protect_latched=true`, so the heartbeat protocol did not
  need another tail.

The same revision qualifies visual dusk instead of treating one low-current charger
sample as darkness. A TSL2591-equipped peer requires five minutes at <=200 lux and uses
>=500 lux as the separate dawn threshold. This prevents a full battery from creating
false afternoon night/sunrise cycles. A peer without the light sensor falls back to
30 minutes without useful charger input. Build overrides are
`--field-dusk-lux-x10`, `--field-dawn-lux-x10`, `--field-dusk-confirm-s`, and
`--field-dusk-no-sensor-confirm-s`; `/telemetry` exposes `field_dusk_s`.

The old optional `--autosleep` counter remains useful for non-field bench modes, but is
not the field LED safety mechanism: it runs later in setup and waits many resets. The
NVS field-session stage is armed before the first LED load and catches the first
unexpected reset.

`net-bench-2026-07-12.2` repairs the day/night regression found on the first P105
deployment of the reset guard. Darkness is latched once drawdown begins, so a missing
TSL2591 sample is unknown rather than synthetic daylight. The firmware also remembers
that a lux sensor has produced a valid sample across deep-sleep wakes, keeping its
5-minute dusk qualification stable instead of switching to the 30-minute bare-peer
fallback on an intermittent read. `/telemetry` exposes that capability latch as
`field_lux_sensor_seen`.

The first `.2` rail repair removed an erroneous GPIO4 deinitialization, but live P105
current proved that was insufficient: the PowerFeather SDK's held-RTC-pin setter can
return success even when its unchecked level write did not energize the physical rail.
`net-bench-2026-07-12.3` explicitly reinitializes EN_3V3/GPIO4 as an RTC input/output,
drives it high, reads the level back, and re-enables hold after the SDK call. Any failed
step now takes the existing interrupted-session park path instead of reporting a false
LED-on state.

Live `.3` validation on P105 proved the rail repair and exposed the real startup
transient: the former 400 ms ramp reached about 500 mA / 2.93 V before a power-on reset;
the one-time dim retry held about 300 mA / 3.07 V for roughly 9 s before a second POR,
then correctly hard-parked. `net-bench-2026-07-13.1` stretches the four-step ramp to
3.2 s so delayed harness/cell sag is visible before full brightness, and waits 10 s
after an interrupted full-load boot before applying the one allowed dim retry. The P105
profile should continue to deploy with `--field-dim-mv 3100 --maintain 4.6`; the
generic/bare-peer dim default remains 3000 mV. The P126 2 W profile uses
`--maintain 5.8`.

`net-bench-2026-07-13.2` also makes the NVS `protect` stage authoritative after every
reset type. Live OTA validation found that a software reset could reinitialize the RTC
phase to charge while NVS still said protect, which would have bypassed the
charge-release latch after maintenance. A persisted protect stage now parks before
sensor/LED startup until verified charge clears it; a persisted dim stage survives a
deliberate software reset without silently returning to full brightness.

The targeted field-cycle OTA helper defaults to 4.6 V VINDPM, which is both the
USB-rescue-safe setting and the qualified P105 5 W operating point. The P126 2 W peer
uses its separately qualified 5.8 V point and must pass `--maintain 5.8` explicitly.

The full P105/P126 field reconstruction, production show-duration math, daily-harvest
ledger, POR regression chain, and reusable telemetry gotchas are recorded in
`docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md`.

`net-bench-2026-07-06.1` adds an opt-in safe VINDPM perturb helper for the field-cycle
bench:

```
./build.sh --role peer --channel 11 --field-cycle --field-mppt ... --maintain 4.6
```

With `--field-mppt`, a charge wake preserves the OTA listen window, then samples fixed
P105 candidates 4.6/4.8/5.0 V when the battery/input gates are healthy enough. It logs
`mppts=` (status), `mpptr=` (reason), `mpptn=` (run count), `mpptv=`/`mpptbest=`/
`mpptlast=` (volts x10), and `mppt46=`/`mppt48=`/`mppt50=` (W x100). By default it
clamps back to 4.6 V before sleeping or entering maintenance so USB/power-bank rescue
stays safe. `--field-mppt-hold` is available for a later harvest-optimization test, but
do not use it for the first safety/learning deployment. This tail extends the heartbeat
past the old 128 B bridge buffer, so flash/build the matching serial bridge before an
MPPT peer.

`net-bench-2026-06-30.7` also appends BQ25628E charger telemetry for low-VBAT
solar/USB rescue debugging:

- `bqv=` VINDPM in mV, `bqichg=` charge-current limit in mA, `bqvreg=` CV limit in mV.
- Raw BQ bytes: `bq16`, `bq18`, `bq1d`, `bq1e`, `bq1f`, `bq20`, `bq21`, `bq22`,
  and `bq38`.
- The dashboard/log derive the most useful bits: `bq_chg_en`, `bq_en_hiz`,
  `bq_batfet_ctrl`, `bq_vbus_stat`, and `bq_chg_stat`, while preserving raw bytes for
  later decode.

### Env sensors (MPP sweep: light + panel temp over the air)
A TSL2591 (lux, 0x29) and/or SHT31-D (temp/RH, 0x44) chained on the peer's STEMMA-QT
are **auto-probed at boot** (no build flag; one image serves sensored and bare boards)
and appended to the heartbeat: `lux=` (`sat` = saturated -- full sun can exceed the
TSL2591's range even at min gain; a paper/PTFE diffuser fixes it, and the relative
normalization use survives the unknown attenuation), `ch0=`/`ch1=` raw counts,
`ptc=`/`prh=` (tape the SHT31 to the panel BACK ~ cell temp), `btc=` (battery NTC,
needs `--batt-ntc`). Master `m<v10>` (e.g. `m48`) sets an explicit VINDPM for scripted
sweeps; bare `m` cycles presets. Host: `ops/bench/mpp_sweep.py` + `mpp_analyze.py`.

## Serial bridge + field scan-report (no laptop in the field)

Two modes that let you log the field fleet **from your desk over USB**, instead of
tethering a laptop in the yard (the WiFi range diagnostic, item (b) of
`docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md`). Why ESP-NOW and not WiFi-STA:
the ESP32-S3 is 2.4 GHz-only and won't hold an Eero association from the yard, but
ESP-NOW reached the back fence -- so carry the data on ESP-NOW.

- **`--serial-bridge`** (a master): does **not** join WiFi -- stays pinned to
  `--channel` and **relays everything it hears to USB serial** (the same `nb-master` /
  `nb-peer` / `nb-scanap` lines). A desk-tethered board thus logs the whole fleet.
- **`--scan-report`** (a field peer): periodically WiFi-**scans** 2.4 GHz (never
  associates -> radio stays on `--channel` for ESP-NOW), and broadcasts the strongest
  `--scan-max` APs (BSSID/RSSI/channel/SSID) as `NB_SCANAP` packets. This maps which
  Eero node is reachable at what RSSI from anywhere in the yard -- the coverage map +
  the "a stronger node was available" half of the missed-roam story, streamed wirelessly.

```
# desk bridge (USB to your PC):
./build.sh --role master --channel 11 --serial-bridge --port /dev/ttyACM0
# field board (battery, walked); scans every 15 s:
./build.sh --role peer --channel 11 --scan-report --scan-s 15 --hb-hz 1 --port /dev/ttyACM1
# on the PC: relay serial -> UDP so the usual loggers work, then log:
ops/bench/net_bench_serial_bridge.py --port /dev/ttyACM0   &
ops/bench/net_bench_log.py --site ca --notes "yard 2.4GHz coverage"
```

Channel note: because the field board never associates, `--channel` is **yours to
pick** (a clean 2.4 GHz channel) -- no coupling to the Eero's channel. Both boards must
share it. The **association/roaming** empirical test (does it actually cling to the far
node?) is deferred -- `firmware/wifi_diag/` is the tethered probe for that.

## Emergency single-board maintenance AP

Default maintenance mode is still the fleet path: peers join a normal bench/router WiFi
with client isolation disabled, so `net_bench_ota.py` can update many nodes in parallel.

For one-off recovery where the only available WiFi is client-isolated, build a single
field peer with `--maint-ap`. Treat this as deprecated unless Ben explicitly asks for it.
It is intentionally **not** the normal bench or fleet OTA mode: each peer creates its own
AP, so updates are serialized and the laptop has to switch networks. Normal telemetry
still rides ESP-NOW to the USB serial-bridge master. The fleet-scalable path is
shared-WiFi/portable-router maintenance plus `net_bench_ota.py` parallel uploads.
When the master sends `U` (sustained `ENTER_MAINT`), the peer leaves ESP-NOW and starts a
temporary AP named `ResonanceMaint-<nodeid>` with password `resonance`; the peer serves
`/`, `/telemetry`, `/resume`, and `/update` at `http://192.168.4.1/`.

```
# USB desk bridge (keeps relaying telemetry; no WiFi dependency):
./build.sh --role master --channel 11 --serial-bridge --port /dev/ttyACM0

# Field peer: solar/INA/env telemetry over ESP-NOW, self-AP only during maintenance:
./build.sh --role peer --channel 11 --maint-ap --chem lfp --cap 6000 --port /dev/ttyACM1

# Later, on the bridge serial console: press `U`.
# Then connect the laptop to WiFi SSID ResonanceMaint-<nodeid> / password resonance.
# OTA from that temporary AP:
./build.sh --role peer --channel 11 --maint-ap --chem lfp --cap 6000 --ota 192.168.4.1
```

This AP mode is intentionally one-peer-at-a-time. Use the default shared-WiFi maintenance
mode whenever you want parallel OTA.

## Onboard LED = battery level (GPIO46)

Every board shows its battery state-of-charge on the onboard user LED, so the wireless
fleet is readable at a glance: **>50% solid * 25-50% blink 1 Hz * 10-24% 2 Hz * <10%
4 Hz * no reading = off**. A **voltage cross-check** floors the level so a healthy cell
never shows "critical" -- the MAX17260 SOC can read wildly wrong after a `DesignCap`
change / before a learn cycle (we saw a full 4.19 V cell report 1%); a loaded-Li-ion
voltage floor vetoes a false-low gauge reading. (Production low-battery logic should do
the same -- never trust gauge SOC blindly.)

**Locate / identify:** the master can tell a specific board (or all) to blink a distinct
`..-` pattern for 8 s (overriding the battery display) so you can find it physically --
the data-center "chassis ID LED" pattern. Master serial `i` cycles through peers one at a
time (prints the ID it's pinging); `I` lights all peers at once. Forward-looking: a field
build would assign each fixture an install-time index (NVS) for a readable per-fixture
beacon; on-demand locate is the primitive that matters now.

## Serial commands (115200)
`t` telemetry * `r` report (role/mode/rate/txseq/sendok/fail/peers) * `+`/`-` step the
broadcast rate for a sweep (master broadcasts `SET_RATE` to peers) * `i` identify next
peer (locate, blinks `..-` 8 s) * `I` identify all peers * `U[peerid]` master:
sustained maintenance wake, bare = fleet and `U9E5AB8` = one peer * `c` resume *
`m<v10>` set VINDPM * `S[seconds]` timed deep-sleep peers (bare `S` = 6 h) *
`D<id>[:mah]` targeted HEX drawdown + sleep * `C<mah>` set capacity and reboot peers *
`G<mA>` set charge-current cap * `x` watchdog hang test (needs
`--wdt-hangtest`).

Additional solar-charge helpers: `R<hz>` sets the heartbeat/frame rate directly, and
`P<id>[:seconds]` parks one selected peer without sleeping the rest of the bench.

## Host tooling
- `ops/bench/net_bench_log.py` -- capture the master's UDP bridge -> JSONL (per-peer
  PDR/RSSI/reboots, firmware revision, and maintenance status when present).
- `ops/bench/net_bench_ota.py` -- parallel OTA to N IPs + auto-recovery verification.
  Use `--reboot comms` when OTA'ing peers (they reboot OFF WiFi into ESP-NOW, so the
  OTA "Update complete. Rebooting." ack + software reset is the success signal; confirm
  rejoin via the master bridge). Default `--reboot maint` verifies via `/telemetry`.
- `ops/bench/field_cycle_ota.py` -- pit-crew wrapper for the field-cycle peer path:
  persistent named build dir, targeted `U<id>` through the dashboard, automatic
  shared-WiFi `/telemetry` IP discovery by `fixture_id`, wait-out of the 35 s targeted
  maintenance tail, OTA via `net_bench_ota.py --reboot comms`, and dashboard rejoin
  verification. Its 360 s discovery default covers one 300 s field-sleep cadence.
  Build once with `--build-only`, then deploy the verified artifact with `--bin`.
  Examples: `python ops/bench/field_cycle_ota.py 9F26F8 --hex-lit 18 --brightness 128`
  or `python ops/bench/field_cycle_ota.py 9F26F8 --rgbw --brightness 255`.
- `ops/bench/net_bench_summary.py` -- per-peer + aggregate stats + scale extrapolation.
- `ops/bench/net_bench_serial_bridge.py` -- relay a `--serial-bridge` board's USB serial
  to UDP:54321, so all the UDP tooling above works from a desk-tethered bridge (no
  laptop in the field). `net_bench_log.py` also logs `nb-scanap` coverage rows.

- `ops/bench/net_bench_dashboard.py` -- local browser dashboard for a USB
  `--serial-bridge` master. It owns the serial port, serves `http://127.0.0.1:8765/`,
  shows live solar/battery/RF telemetry, writes safe controls back to the bridge
  (`m<v10>`, targeted `U<id>`, `c`, identify, refresh), and can still forward `nb-*` lines to
  UDP:54321 for the older tools. Travel example:
  `python ops/bench/net_bench_dashboard.py --port COM7`.

## Caveats
Battery runs are **Li-ion (`Generic_3V7`)** for now -- *re-verify every stability finding
on LFP* (LFP's plateau parks on the buck-boost crossover, a harder regime). USB/pogo
stays the guaranteed recovery path. Bare-board RF only; hat antenna detuning is deferred
to Steve's mock-hat.
