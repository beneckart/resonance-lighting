# PowerFeather V2 battery-brownout investigation -- plan, hypotheses, open questions

**Date:** 2026-06-03 (updated 2026-06-04; retro-analysis 2026-07-03)

**RETRO-ANALYSIS (2026-07-03) -- read this first; it re-grades every hypothesis
below.** The July presence-bench reboot epidemic (LOG 2026-07-02 cont. 5-10,
2026-07-03) was root-caused by controlled A/B to **signal integrity on the shared
power-management I2C bus**: elevated-clock (400 kHz) traffic under WiFi TX
corrupts transactions to the BQ25628E -- the chip the battery current flows
through -- and a bad transaction near its power-path control registers
(BATFET/ship/EN_HIZ class) opens the battery switch outright. That failure's
signature is IDENTICAL to this doc's: instantaneous `poweron` reset at healthy
voltage, battery-only (USB immune -- VBUS bypasses the BATFET), requires active
WiFi, stochastic. The unifying lens for BOTH investigations is therefore:
**anything that degrades the power-management bus can kill VSYS** -- in June the
degrader was the IS31 chip loading/back-powering SDA/SCL; in July it was our own
bus clock. One mechanism class, two disturbance sources.

Hypothesis re-grades under this lens (evidence-honest, June was never
instrumented at the BQ register level, so this is best-supported inference, not
re-proof):

- **H2 (connection/battery-path impedance) -- RETIRED as the leading explanation.**
  It was always "leading-but-unconfirmed" (this doc's own words); the July
  elimination round reproduced the exact signature with soldered welded-tab
  leads, no holder, two boards, two healthy cells -- and the June observations
  H2 leaned on (stability after re-seating) are equally explained by re-seating
  the IS31/STEMMA connection changing how that chip loaded the bus. No confirmed
  connector kill exists in either dataset. Connector hygiene remains good
  practice; it is no longer a suspect of record.
- **H5 (load-stacking) / "low droopy battery" framings -- DEAD.** Already
  superseded in June (loops at lightest load, healthy V); July adds stability
  at 3.33 V under 100 kB/s TX. Load raises the dice-roll rate at most.
- **H3 (LFP low-voltage / boost mode) -- DEAD** (June: stable in boost at
  3.18-3.24 V; July: stable in the crossover band under heavy TX).
- **H4 (VSYS bulk capacitance) -- RETIRED as a remedy.** Capacitance answers a
  sag mechanism; the observed kills are switch-openings, which no cap prevents.
  Bulk capacitance stays ordinary good design, not a brownout fix.
- **H1 (TX di/dt transients) -- PARTIALLY SUBSUMED:** TX matters, but as the
  NOISE source coupling into the bus during transactions, not (on the evidence)
  as a VSYS-collapsing load by itself: heavy TX alone was stable in June
  (IS31 unplugged) and in July (loadgen heavy phase; 100 kHz full bench).
- **The June sub-result that aged best:** "VSQT power-shed did NOT fix it; only
  physical disconnection did" + "back-powering through I/O clamps keeps the IS31
  on the bus" -- i.e. the kill tracked the chip's PRESENCE ON THE BUS, not its
  power. That was the bus-integrity mechanism announcing itself.

Standing rules from the unified story now live in
`firmware/POWERFEATHER_NOTES.md` ("Wire1 at >100 kHz can OPEN YOUR BATTERY
SWITCH"): never raise the clock on, or attach bus-degrading devices to, any bus
shared with the charger/gauge; custom PCBA gets a dedicated power-management
bus; treat battery-only `poweron` resets as possible power-path register upsets
and check the bus before probing connectors and cells.
**Status (2026-06-04 -- the brownout CAME BACK):** Overnight, board 1 running the
loadgen on battery went into a **794-reboot loop over 4.25 h** -- every one a
`poweron` VSYS collapse, at **healthy voltage (bv 3.24-3.46) across the whole SOC
range (98%->30%)**, in the **lightest** load phase (LEDs off, light WiFi), boots dying
at ~5-9 s right around **WiFi association**. So the afternoon "non-reproduction"
(below) was the fluke; board 1's brownout is **real and intermittent** -- solid for
hours when freshly re-seated/warm, then it drifts marginal. This **strengthens H2
(marginal connection on board 1)** and the per-boot trigger looks like the
**WiFi-association current spike** on a marginal VSYS, *not* load-stacking (lightest
load) and *not* depletion (healthy V, all SOC). Two consequences:
(1) **Guard flaw found + fixed:** the coulomb-budget / max-runtime / low-V auto-sleep
were all RAM state that **reset every reboot**, so a tight loop defeated them
(mah_used never exceeded 1.4 of the 1000 budget). Fix = an **NVS-persisted boot
counter** (`--autosleep`): clean start zeroes it, brownouts increment, >=25
sub-survival boots => deep sleep before WiFi. (2) **SOC/voltage thesis confirmed
hard:** bv sat pinned at ~3.24 V for 4 h while the gauge SOC (coulomb-based) drained
92%->30% -- voltage is useless for LFP SOC, but the gauge's coulomb count tracked the
drain. Plot: `ops/bench/data/ca/2026-06-03-ca-lfp-overnight-soc_v.png`.
**Result (2026-06-04): board 2 ALSO loops -- NOT board-specific.** Pristine board 2,
same cell+grid, brownout-looped too (first boot 356 s reaching the lit-grid phase, then
collapsed at the USB-unplug and looped at healthy bv ~3.23, soc ~72). So it is **not**
board 1's solder joint. The **NVS loop-breaker fired and deep-slept the board** (fix
validated). Temperature ruled out (office 72.5-79 deg F, ~74 when it last worked). Common
factors across every looping case: the **deep-cycled cell**, the **IS31 grid+cable**,
firmware. Leading hypotheses: **IS31 driver latching into a bad state** (back-current /
spikes on SDA/SCL -- fits IS31-unplugged-always-stable and VSQT-shed-never-helped) vs the
**cell's raised ESR** (post deep-cycle) exposing the IS31+WiFi load. Discriminating tests
queued: (1) unplug IS31 + rerun same cell; (2) GPIO WS2812 vs IS31; (3) fresh cell + IS31.

**Result -- test (1), 2026-06-04: IS31 presence on the I2C bus is NECESSARY.** Board 2,
same deep-cycled cell, on battery, **IS31 unplugged -> stable 365 s+, 0 reboots, through
light AND heavy WiFi** (vs the loop *with* the IS31, only variable changed). So it's
**not** cell+WiFi alone and **not** WiFi-inrush alone (both stable); and since the loops
were in phase 0 with **LEDs off**, **not LED current** either -- it's the IS31 *chip on the
shared charger/gauge I2C bus*. Backs the I2C-disturbance hypothesis. Remaining split:
**(a)** IS31 actively misbehaving (SDA/SCL back-current/spikes) vs **(b)** any I2C device
loading that shared bus tips VSYS under WiFi -> distinguished by test (2)': **Adafruit
NeoDriver (5766) on the same bus, NeoPixels externally powered** (also brownouts => b;
clean => a).

**Result -- test (2)', 2026-06-04: NeoDriver STABLE => the brownout is IS31-SPECIFIC (a).**
Board 2, NeoDriver (SeeSaw I2C) on the same shared bus, full-white NeoHEX (LED 5 V
external, off the battery), battery + WiFi -> **371 s+, 0 reboots, through heavy WiFi**,
bv 3.25 -- vs the IS31 looping within ~1 min on the same board/cell/bus. So it's **not**
"any I2C device on the power-mgmt bus" -- it's the **IS31FL3741 chip's own behavior** on
SDA/SCL (back-current/loading during WiFi spikes; recall it browns out even LEDs-off).
**LED-axis takeaway:** I2C LEDs are not categorically out; **NeoDriver + WS2812 (NeoHEX)
is a viable no-solder, self-contained LED path** (onboard 5 V boost + data level-shift)
that does NOT brown out on battery. **Caveats:** n=1/~6 min and the IS31 was intermittent
(stable for minutes before failing) -> confirm the NeoDriver with an **hours/overnight**
run, which first needs the **auto-sleep wake-source fix** (deep sleep has no wake source ->
strands the board; cost ~1 h + a WiFi-PHY corruption recovered via `esptool erase_flash`).

**Status (2026-06-03):** **The brownout no longer reproduces -- leading suspect is a marginal
physical connection, not the board/module/chemistry.** An n=3 board-swap, holding the
**same cell, IS31 grid, and STEMMA cable** constant and changing only the board,
found **all three boards stable** under the exact IS31+WiFi+battery condition that
previously collapsed board 1 in 7-17 s -- including **board 1 itself** on the capstone
re-test (4 min, 0 resets, bv 3.24 V). So it is **not** a platform property, **not**
board-1-specific, **not** the cell/grid/cable (all held constant). The one thing that
changed across the afternoon is **repeated unplugging/re-seating of connectors**,
which points at **H2 -- battery-path / connection impedance** (a marginal soldered
battery joint and/or STEMMA seat that re-seated): under WiFi current spikes a
high-impedance contact collapses VSYS, a good contact rides through. **This is a
hypothesis, not yet confirmed** -- we've shown the brownout *stopped*, not *why*. Next
step is a deliberate reproduction attempt (stress/wiggle connections under load).
Both earlier conclusions in this doc (load-stacking platform property; board-1
anomaly) are **superseded** by this section. The early observations further down are
likewise superseded.

## The question

Under exactly what conditions does the PowerFeather V2 take a **full power-on
reset** (`reset_reason=poweron`, VSYS collapses) while running **on battery**? We
want the boundary, not an impression. Motivating concern: whether routine
operation and **OTA** are safe on battery in the field.

## Findings -- controlled runs (solid soldered LFP cell)

Once we had a **solid soldered LFP connection** (the spring splice had confounded
earlier runs) and instrumentation that removed prior artifacts (uptime-based phase,
no NVS flash write, `reset_reason` + battery V/I in the UDP heartbeat, low-battery
backoff), a clear and repeatable picture emerged. **Every** reset was
`reset_reason=poweron` (genuine VSYS collapse, not a crash), at healthy battery
voltage (bv 3.2-3.6 V) -- so not depletion and not the connector.

| Condition (battery, solid connection) | Result |
|---|---|
| WiFi OFF, any LED state (incl. full grid) | stable |
| WiFi ON (light **or heavy** TX), **IS31 cable unplugged** | **stable** -- 9 min, 0 resets, bv down to 3.24 V |
| WiFi ON, heavy TX, `setSleep(false)` (radio full-on), **IS31 unplugged** | **stable** -- re-confirm run, 0 resets, bv down to **3.20 V** |
| WiFi ON, **IS31 connected** (normal, VSQT on) | brownout, **~7-17 s** (rapid) |
| WiFi ON, IS31 connected, **VSQT power shed** (`enableVSQT(false)`) | brownout, **~21 in 7 min**, modem sleep matched |

Things that turned out **not** to be the cause: the battery connector (now solid
solder), cell depletion (healthy bv at reset), the battery chemistry switch, the
NVS-write artifact, and the WiFi radio power mode (`--wifi-lowpower` = modem sleep +
8.5 dBm did **not** fix it, and didn't differ materially from `setSleep(false)`).

### Board-swap test (n=3) -- the brownout stopped reproducing on every board

All board-1 findings above were collected on a **single board**. To lift n=1, we
moved the **same LFP cell, the same IS31 grid, and the same STEMMA cable** across
three boards in turn (same `--loadgen` IS31 firmware on each), changing **only the
board**:

| Board | Physical state | Result (same cell+grid+cable, IS31 on bus, on battery) |
|---|---|---|
| **1** (earlier runs) | hand-soldered JST-XH on VDC/GND | brownout, **7-17 s**, repeatable (the Findings above) |
| **2** | pristine, untouched | **stable** -- light *and* heavy WiFi, ~9+ min, 0 resets, bv to **3.19 V** |
| **3** | pristine, untouched | **stable** -- light + into heavy, 0 resets, bv to **3.20 V** |
| **1** (capstone re-test) | same board 1, after the swapping | **stable** -- 4 min, 0 resets, bv 3.24 V |

The capstone (last row) is the key result: board 1, which *did* collapse earlier, is
now **stable with the identical setup** that boards 2 & 3 survived. So the brownout
reproduces on **none** of the three boards anymore. It is therefore **not** the board,
**not** the cell, **not** the grid/cable (all held constant across every run). The only
variable that changed across the afternoon is the **repeated unplug/re-seat of
connectors** during the swap -- which makes **H2 (connection / battery-path impedance)**
the leading explanation: a marginal soldered battery joint and/or STEMMA seat making
intermittent high-impedance contact, re-seated by the handling. A bad contact collapses
VSYS under a WiFi current spike; a good one rides through.

This **de-risks the ~100-unit procurement** (the failure looks like a connector/solder
quality issue, not a flaw in the V2, the IS31, or the chemistry) -- *if* we can confirm
it. We have only shown the brownout **stopped**, not the mechanism. We were too quick,
twice today, to write a firm conclusion ("load-stacking platform property", then
"board-1 anomaly"); both are superseded. Treat H2 as leading-but-unconfirmed.

**Now-open, in priority order:**
1. **Deliberately reproduce it** -- re-run IS31+WiFi on board 1 and physically *stress
   the connections* (wiggle/flex the battery leads and the STEMMA-QT connector). If the
   collapse returns on a bad contact and clears on a good one, H2 is confirmed directly.
2. **Inspect/reflow board 1's battery + VDC joints** under magnification (cold joint,
   flux, hairline bridge); re-test.
3. If it cannot be re-induced, log it as a non-reproducible connection transient, keep a
   VSYS bulk cap as cheap insurance, and move on.

### The original board-1 pattern (now superseded -- kept for the record)

When board 1 *was* browning out, the pattern was: it required **both** WiFi active
**and** the IS31 module physically on the bus -- heavy WiFi alone (module unplugged)
was stable, the LED grid alone (WiFi off) was stable, together they collapsed VSYS.
At the time we read this as **load-stacking on a marginal VSYS** (H5). **The n=3
board-swap supersedes that reading:** board 1 no longer reproduces it, so the "WiFi +
IS31 together" signature is now better explained as *a marginal contact that only
drops out when total current (IS31 baseline + WiFi spike) is high enough* -- i.e. H2,
not H5. The "needs both" observation fits a connection-impedance cause just as well as
a load-stacking one; we'd previously only entertained the load reading.

### A specific sub-result (candidate mechanism, not proven)

Cutting the module's **power** rail in firmware (`enableVSQT(false)`) **did not fix
it** -- still ~the same brownout rate. Only **physically disconnecting** the module
stopped it. Most likely **I2C back-powering**: with VSQT (VCC) off, the IS31 stays on
SDA1/SCL1 (pulled to the *main* 3V3), so current flows into the chip through its I/O
clamp diodes and it keeps loading the rail; unplugging removes that path. Fits the
data but not proven (would want a scope / a bus-isolation test).

### Implications for production (re-scoped by the n=3 board-swap)

**Read this section in light of the board-swap result:** two pristine boards did
*not* brown out, so the items below describe the **board-1 failure mode** and the
defensive options *if* the thin margin turns out to exist on production units too --
not a confirmed platform-wide requirement. Pending the board-1 capstone re-confirm,
the baseline expectation is now that a stock V2 + IS31 runs on battery fine.

1. **VSYS bulk capacitance is the mechanism-independent fix** *if* a margin problem
   shows up on stock units -- enough local energy to ride through the sub-ms WiFi
   spike regardless of module load. **Not yet bench-validated, and possibly
   unnecessary** given boards 2 & 3 were stable without it. Demote from "leading
   requirement" to "cheap insurance to characterize."
2. **LED-module choice affects software load-shed.** An **I2C** module (IS31) can't be
   cleanly shed in software (back-power); a **GPIO WS2812** module (NeoHEX / single
   RGBW, one data line) could be fully shed by rail-off + data-low.
3. **OTA-on-battery:** the simple "drop VSQT during the OTA window" mitigation does
   **not** work for the I2C IS31. Robust paths: bulk cap; OTA in daylight (solar
   supply present buffers spikes, as USB does); or a sheddable GPIO LED module.
4. **Production radio profile helps anyway:** ESP-NOW + light sleep (radio mostly off)
   is far gentler than the bench's continuous WiFi -- the acute brownout is partly a
   worst-case bench artifact. But the thin VSYS margin is real.

### Open questions / next data

- **CAPSTONE: does board 1 still collapse?** Move the same cell + grid + cable back
  onto board 1 and re-run. Confirms (or breaks) the board-swap conclusion via a pure
  A/B. -- now the single highest-value test.
- If board 1 still fails: **is it the soldering or the unit?** Inspect/reflow the
  VDC/GND joints (reflow, clean flux, check for bridges); re-test. Distinguishes a
  fixable solder defect from an intrinsically bad board.
- **n=3 done:** two pristine boards (2, 3) stable under the board-1 brownout
  condition; brownout is board-specific. (Was: "repeat on a second board, n=1.")
- Does a **GPIO WS2812** module (NeoHEX / RGBW) behave the same, and can it be
  software-shed where the IS31 can't? (Still worth knowing for the LED-module axis.)
- *(Lower priority now)* VSYS bulk cap effect; I2C-back-power mechanism; VSQT
  rail-restore inrush -- only matter if the thin margin shows on stock units.

## Early observations (partly confounded -- superseded by the Findings above)

| Condition | Battery | Result | Confounds / notes |
|---|---|---|---|
| USB, any load (incl. WiFi 15 h) | -- | stable | clean |
| Battery, radio OFF, no LED | PKCell Li-ion | stable | -- |
| Battery, radio OFF, full LED grid | LFP (bare) | stable | LED rail sagged (pink) but no reset |
| Battery, light WiFi (UDP), no LED | LFP, higher V | stable ~4 min | no SDK init, no IS31 attached |
| Battery, heavy HTTP polling + IS31 | PKCell Li-ion | repeated `poweron` resets | heavy poll **+** IS31-on-VSQT **+** PKCell **+** `setSleep(false)` all stacked |
| Battery, light WiFi + SDK + IS31 | LFP ~3.17 V, **spring-splice** | repeated `poweron` resets | **heavily confounded**: marginal splice + low LFP voltage (boost mode) + SDK/IS31 |

Notable: in the reset cases, `reset_reason=poweron` (full VSYS loss, not the ESP
brown-out *detector*, not a crash) and battery voltage read healthy right up to
the reset -- so it does not look like simple cell depletion.

## Hypotheses (candidates to test -- listed, not ranked as truth)

- **H1 -- transient / di-dt:** fast WiFi-TX current spikes (~0.3-0.5 A, sub-ms)
  momentarily collapse VSYS when no supply is present to buffer them. (The
  BQ25628E power path supplements spikes from the battery **only when USB/VDC is
  connected**, per PowerFeather docs.)
- **H2 -- battery-path impedance:** connector/splice resistance, protection-PCB
  impedance, or cell ESR drop voltage under spikes. Our **spring-splice** test
  connection is a strong suspected confounder; a marginal connector mimics a
  brownout.
- **H3 -- LFP low-voltage / boost mode:** on the LFP plateau (~3.2 V) and below,
  the buck-boost runs in *boost*, amplifying input current on spikes and worsening
  transient response -> less headroom at low SOC. Prediction: worse at low LFP SOC,
  fine when full.
- **H4 -- insufficient VSYS bulk capacitance:** not enough local energy storage to
  ride through sub-ms spikes; chemistry-independent. (Would be fixed by adding a cap.)
- **H5 -- load stacking:** LED module on VSQT **plus** WiFi together exceed headroom
  where either alone is fine.
- **H6 -- radio config / duty:** `setSleep(false)` + heavy continuous TX (our
  polling) vs. modem-sleep / light traffic changes spike frequency and size.

Counter-evidence to keep honest: the official demo (V1) ran WiFi telemetry on
battery, and a lightly-loaded UDP test on our LFP ran stable for 4 min -- so "WiFi
on battery" is clearly not *always* a problem. The boundary is what we're after.

## Controls to eliminate confounds (do these before trusting any result)

- **Solid battery connection** -- soldered/tabbed leads to a JST, **not** spring
  splices. (In progress.) The splice has confounded multiple runs.
- **Known SOC**, with battery voltage logged in-band, so depletion is separable
  from transient brownout.
- **Hold the battery constant** across a load sweep (don't switch PKCell <-> LFP
  mid-matrix, as we accidentally did).
- Vary **one** of {IS31 attached, SDK init, radio mode, supply present} at a time.

## Test matrix (run once the connection is solid)

Axes: **battery** {LFP full, LFP low, Li-ion full} x **radio** {off, light UDP,
heavy TX, demo AP@10 Hz} x **LED** {off, full grid} x **supply** {none, USB}.

Priority cells (battery-only) -- the ones still open:
1. light WiFi **+ full LED grid** (does the LED load tip light WiFi over?)
2. heavy WiFi **+/-** LED (map the load boundary on a *good* connection)
3. **LFP full-SOC vs low-SOC** under identical load (tests H3)
4. **ported demo on battery** (AP, ~10 Hz) +/- LED -- does the reference app reset?
5. (if any of the above reset) add a **VSYS bulk cap** and repeat (tests H4)

## Tooling for the matrix

- **`firmware/power_bench` loadgen** (`build.sh --loadgen`): WiFi station, no HTTP
  server, emits a UDP heartbeat carrying **phase + uptime + battery voltage** so a
  host listener (`bind :54321`) detects outages/reboots remotely and separates
  low-battery from transient. Auto-sweeps light/heavy x LED on/off. *(Phase logic
  being changed to persist across reboots via RTC memory so a reboot in one phase
  doesn't restart the sweep.)*
- **`build.sh --batt-stress` / `--batt-stress-full`**: radio OFF, blinks the LED
  panel (center or full grid) -- radio-off baselines.
- **`firmware/powerfeather_demo_port`**: the ported reference web-telemetry app
  (AP + ~10 Hz) for the demo-load cell of the matrix.
- **`build.sh --wifi-lowpower`**: modem sleep + 8.5 dBm TX (tests H6).
- **`ops/bench/power_logger.py`**: supply/charge telemetry over WiFi (needs a supply).

## What would move our understanding

- Solid-connection LFP repeating (or not) the light-WiFi resets -> H2 vs H3.
- Full-SOC vs low-SOC LFP under identical load -> H3.
- Heavy WiFi on a good connection +/- LED -> load boundary (H1/H5).
- Demo on battery -> does the reference app brown out?
- VSYS bulk cap added -> does it stop regardless of the above (H4)?
