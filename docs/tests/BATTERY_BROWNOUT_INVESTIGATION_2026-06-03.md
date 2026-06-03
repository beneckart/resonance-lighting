# PowerFeather V2 battery-brownout investigation — plan, hypotheses, open questions

**Date:** 2026-06-03
**Status:** ACTIVE / preliminary. Observations below are partial and several are
**confounded**. Nothing here is a conclusion — this is a plan for pinning down the
precise conditions under which the board loses power on battery.

## The question

Under exactly what conditions does the PowerFeather V2 take a **full power-on
reset** (`reset_reason=poweron`, VSYS collapses) while running **on battery**? We
want the boundary, not an impression. Motivating concern: whether routine
operation and **OTA** are safe on battery in the field.

## Observations so far (each with caveats — not conclusions)

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
