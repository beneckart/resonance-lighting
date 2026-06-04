# PowerFeather V2 battery-brownout investigation — plan, hypotheses, open questions

**Date:** 2026-06-03
**Status:** A clear, repeatable cause was isolated on a solid connection (see
**Findings** below). One board / one-to-two cells so far, and one sub-mechanism is
a strong-but-unproven candidate, so treat the production guidance as well-supported,
not final. The early observations further down are partly confounded and superseded
by the Findings.

## The question

Under exactly what conditions does the PowerFeather V2 take a **full power-on
reset** (`reset_reason=poweron`, VSYS collapses) while running **on battery**? We
want the boundary, not an impression. Motivating concern: whether routine
operation and **OTA** are safe on battery in the field.

## Findings — controlled runs (solid soldered LFP cell)

Once we had a **solid soldered LFP connection** (the spring splice had confounded
earlier runs) and instrumentation that removed prior artifacts (uptime-based phase,
no NVS flash write, `reset_reason` + battery V/I in the UDP heartbeat, low-battery
backoff), a clear and repeatable picture emerged. **Every** reset was
`reset_reason=poweron` (genuine VSYS collapse, not a crash), at healthy battery
voltage (bv 3.2–3.6 V) — so not depletion and not the connector.

| Condition (battery, solid connection) | Result |
|---|---|
| WiFi OFF, any LED state (incl. full grid) | stable |
| WiFi ON (light **or heavy** TX), **IS31 cable unplugged** | **stable** — 9 min, 0 resets, bv down to 3.24 V |
| WiFi ON, **IS31 connected** (normal, VSQT on) | brownout, **~7–17 s** (rapid) |
| WiFi ON, IS31 connected, **VSQT power shed** (`enableVSQT(false)`) | brownout, **~21 in 7 min**, modem sleep matched |

Things that turned out **not** to be the cause: the battery connector (now solid
solder), cell depletion (healthy bv at reset), the battery chemistry switch, the
NVS-write artifact, and the WiFi radio power mode (`--wifi-lowpower` = modem sleep +
8.5 dBm did **not** fix it, and didn't differ materially from `setSleep(false)`).

### What it points to (well-supported)

The brownout requires **both** WiFi active **and** the IS31 LED module physically
present on the STEMMA-QT / I2C bus. **Neither alone does it:** heavy WiFi with the
module unplugged is stable; the full LED grid with WiFi off is stable; together they
collapse VSYS. This is **load-stacking on a marginal VSYS** (hypothesis H5), with a
**razor-thin transient margin** — the module tips it even with its LEDs *off*.

### A specific sub-result (candidate mechanism, not proven)

Cutting the module's **power** rail in firmware (`enableVSQT(false)`) **did not fix
it** — still ~the same brownout rate. Only **physically disconnecting** the module
stopped it. Most likely **I2C back-powering**: with VSQT (VCC) off, the IS31 stays on
SDA1/SCL1 (pulled to the *main* 3V3), so current flows into the chip through its I/O
clamp diodes and it keeps loading the rail; unplugging removes that path. Fits the
data but not proven (would want a scope / a bus-isolation test).

### Implications for production (firming up)

1. **VSYS bulk capacitance is the mechanism-independent fix** — enough local energy
   to ride through the sub-ms WiFi spike regardless of module load, so LEDs and radio
   coexist on battery. Leading production requirement. **Not yet bench-validated** —
   the next concrete step is to add a cap and re-test.
2. **LED-module choice affects software load-shed.** An **I2C** module (IS31) can't be
   cleanly shed in software (back-power); a **GPIO WS2812** module (NeoHEX / single
   RGBW, one data line) could be fully shed by rail-off + data-low.
3. **OTA-on-battery:** the simple "drop VSQT during the OTA window" mitigation does
   **not** work for the I2C IS31. Robust paths: bulk cap; OTA in daylight (solar
   supply present buffers spikes, as USB does); or a sheddable GPIO LED module.
4. **Production radio profile helps anyway:** ESP-NOW + light sleep (radio mostly off)
   is far gentler than the bench's continuous WiFi — the acute brownout is partly a
   worst-case bench artifact. But the thin VSYS margin is real.

### Open questions / next data

- **Does a VSYS bulk cap actually stop it?** (Add a cap, re-run.) — the key unproven fix.
- Confirm the I2C-back-power mechanism (scope, or tristate/isolate the bus).
- The VSQT rail-**restore** inrush (hot-plugging the IS31 caused an instant reboot;
  the firmware pulse test was inconclusive — too few clean pulses).
- Does a **GPIO WS2812** module (NeoHEX / RGBW) brown out the same way, and can it be
  software-shed where the IS31 can't?
- Repeat on a known-good cell and a **second board** (n=1 board so far).

## Early observations (partly confounded — superseded by the Findings above)

| Condition | Battery | Result | Confounds / notes |
|---|---|---|---|
| USB, any load (incl. WiFi 15 h) | — | stable | clean |
| Battery, radio OFF, no LED | PKCell Li-ion | stable | — |
| Battery, radio OFF, full LED grid | LFP (bare) | stable | LED rail sagged (pink) but no reset |
| Battery, light WiFi (UDP), no LED | LFP, higher V | stable ~4 min | no SDK init, no IS31 attached |
| Battery, heavy HTTP polling + IS31 | PKCell Li-ion | repeated `poweron` resets | heavy poll **+** IS31-on-VSQT **+** PKCell **+** `setSleep(false)` all stacked |
| Battery, light WiFi + SDK + IS31 | LFP ~3.17 V, **spring-splice** | repeated `poweron` resets | **heavily confounded**: marginal splice + low LFP voltage (boost mode) + SDK/IS31 |

Notable: in the reset cases, `reset_reason=poweron` (full VSYS loss, not the ESP
brown-out *detector*, not a crash) and battery voltage read healthy right up to
the reset — so it does not look like simple cell depletion.

## Hypotheses (candidates to test — listed, not ranked as truth)

- **H1 — transient / di-dt:** fast WiFi-TX current spikes (~0.3–0.5 A, sub-ms)
  momentarily collapse VSYS when no supply is present to buffer them. (The
  BQ25628E power path supplements spikes from the battery **only when USB/VDC is
  connected**, per PowerFeather docs.)
- **H2 — battery-path impedance:** connector/splice resistance, protection-PCB
  impedance, or cell ESR drop voltage under spikes. Our **spring-splice** test
  connection is a strong suspected confounder; a marginal connector mimics a
  brownout.
- **H3 — LFP low-voltage / boost mode:** on the LFP plateau (~3.2 V) and below,
  the buck-boost runs in *boost*, amplifying input current on spikes and worsening
  transient response → less headroom at low SOC. Prediction: worse at low LFP SOC,
  fine when full.
- **H4 — insufficient VSYS bulk capacitance:** not enough local energy storage to
  ride through sub-ms spikes; chemistry-independent. (Would be fixed by adding a cap.)
- **H5 — load stacking:** LED module on VSQT **plus** WiFi together exceed headroom
  where either alone is fine.
- **H6 — radio config / duty:** `setSleep(false)` + heavy continuous TX (our
  polling) vs. modem-sleep / light traffic changes spike frequency and size.

Counter-evidence to keep honest: the official demo (V1) ran WiFi telemetry on
battery, and a lightly-loaded UDP test on our LFP ran stable for 4 min — so "WiFi
on battery" is clearly not *always* a problem. The boundary is what we're after.

## Controls to eliminate confounds (do these before trusting any result)

- **Solid battery connection** — soldered/tabbed leads to a JST, **not** spring
  splices. (In progress.) The splice has confounded multiple runs.
- **Known SOC**, with battery voltage logged in-band, so depletion is separable
  from transient brownout.
- **Hold the battery constant** across a load sweep (don't switch PKCell ↔ LFP
  mid-matrix, as we accidentally did).
- Vary **one** of {IS31 attached, SDK init, radio mode, supply present} at a time.

## Test matrix (run once the connection is solid)

Axes: **battery** {LFP full, LFP low, Li-ion full} × **radio** {off, light UDP,
heavy TX, demo AP@10 Hz} × **LED** {off, full grid} × **supply** {none, USB}.

Priority cells (battery-only) — the ones still open:
1. light WiFi **+ full LED grid** (does the LED load tip light WiFi over?)
2. heavy WiFi **±** LED (map the load boundary on a *good* connection)
3. **LFP full-SOC vs low-SOC** under identical load (tests H3)
4. **ported demo on battery** (AP, ~10 Hz) ± LED — does the reference app reset?
5. (if any of the above reset) add a **VSYS bulk cap** and repeat (tests H4)

## Tooling for the matrix

- **`firmware/power_bench` loadgen** (`build.sh --loadgen`): WiFi station, no HTTP
  server, emits a UDP heartbeat carrying **phase + uptime + battery voltage** so a
  host listener (`bind :54321`) detects outages/reboots remotely and separates
  low-battery from transient. Auto-sweeps light/heavy × LED on/off. *(Phase logic
  being changed to persist across reboots via RTC memory so a reboot in one phase
  doesn't restart the sweep.)*
- **`build.sh --batt-stress` / `--batt-stress-full`**: radio OFF, blinks the LED
  panel (center or full grid) — radio-off baselines.
- **`firmware/powerfeather_demo_port`**: the ported reference web-telemetry app
  (AP + ~10 Hz) for the demo-load cell of the matrix.
- **`build.sh --wifi-lowpower`**: modem sleep + 8.5 dBm TX (tests H6).
- **`ops/bench/power_logger.py`**: supply/charge telemetry over WiFi (needs a supply).

## What would move our understanding

- Solid-connection LFP repeating (or not) the light-WiFi resets → H2 vs H3.
- Full-SOC vs low-SOC LFP under identical load → H3.
- Heavy WiFi on a good connection ± LED → load boundary (H1/H5).
- Demo on battery → does the reference app brown out?
- VSYS bulk cap added → does it stop regardless of the above (H4)?
