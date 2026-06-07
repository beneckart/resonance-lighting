# LOG

Append-only session journal for the Resonance Lighting workstream. Most recent first.

Format per entry:

```
## YYYY-MM-DD — author — short subject

Body. What changed, what was decided, what's next.
```

---

## 2026-06-07 — Ben + Claude — Two findings: 3V3-rail-needs-enabling (GPIO4) + 8-bit gamma low-end dead-zone

**1) PowerFeather V2 switchable 3V3 rail must be enabled (GPIO4 / EN_3V3).** The
studio sketches drove the HEX/RGBW off the 3V3 header but didn't run the SDK, so the
header read **0 V** — the rail is a load switch gated by GPIO4 (active HIGH), which
`Board.init()` normally turns on. Fix: non-SDK apps drive GPIO4 HIGH in setup()
(`pinMode(4,OUTPUT); digitalWrite(4,HIGH)`). Added to both studios, reflashed RGBW,
rail + LED came up. Bonus: since the LEDs are on the *switchable* rail,
`digitalWrite(4,LOW)` is a free LED kill-switch (the "software-cuttable 3V3"
pixel-power option). Captured this + the other recurring PowerFeather gotchas
(V2 board flag, native-USB reset/IP recovery, keep LEDs off the I2C bus) in a new
**`firmware/POWERFEATHER_NOTES.md`** best-practices doc, linked from
`firmware/README.md`.

**2) 8-bit + gamma kills the low brightness end (relevant to ambient).** With gamma
ON, the LED goes fully dark below ~brightness 24; gamma OFF lights it at very low
levels. Mechanism: gamma correction linearizes *perceived* brightness via
`out = (in/255)^2.6 * 255`, but Adafruit's gamma8 table maps **input 0..23 → 0**
(then 1 for 24..35, 2 for 36..43…) — the bottom ~9% of the range quantizes to off
because 8-bit PWM has no codes for the sub-1 values the curve demands. Tradeoff:
gamma-on = smooth perceived dimming mid/high but a dead-zone + coarse steps at the
bottom; gamma-off = usable ultra-dim but non-linear ramp. This matters because the
lantern's ambient spec ("1–3 LEDs at ~10%") sits right in the dead-zone. Noted for
later; fixes to consider when tuning the ambient look: dim-floor (`max(1,gamma8(x))`),
gentler gamma, gamma-on-color-only, or temporal dithering. No change made now.

## 2026-06-06 — Ben + Claude — RGBW Studio: interactive web app for the 4 W RGBW point source

Built `firmware/rgbw_studio/` — sibling of hex_studio for the single high-power
SK6812 RGBW pixel (Adafruit 5163, 4 W). Validated on hardware (PowerFeather ACM1,
RGBW data on GPIO10): boots, joins WiFi, serves UI; all endpoints exercised OK
(W-only, hue cycle, candle, off) and the board stayed alive through the animations.
Came up at http://192.168.4.209 (same DHCP lease as the HEX session).

The RGBW is a point source (crisp gobo) with a dedicated W die, so this studio is
all about color + temporal modulation (no geometry): R/G/B/**W** sliders + color
picker, gamma toggle; white/warmth presets (W-only, RGB-white, RGBW-full, warm amber)
+ a warmth crossfade slider (RGB-white ↔ W); and color animations — **Hue cycle**,
**Breathe**, **Candle** (smoothed random-walk flicker of the chosen color), **Fade**
(crossfade to a Color-B picker). Settings readback for recording good combos.

Reminder from the LED findings: at 3.3 V the RGBW is voltage-starved (dim, non-linear
mid-range) — fine for judging color/shadow geometry on the bench, but use 5 V for true
brightness characterization. Next: run it through the inverted-lantern gobo rig
alongside hex_studio to settle point-vs-area (and W-vs-RGB-white) by eye.

## 2026-06-04 (cont. 11) — Ben + Claude — HEX Studio: interactive web app for HEX aesthetics + gobo dial-in

Built `firmware/hex_studio/` — a standalone WiFi web app to dial in the SK6812 HEX
look through the gobo, separate from `power_bench` (which is brownout/telemetry
scaffolding). Validated on hardware: flashed to the PowerFeather (ACM1, HEX data on
**GPIO10**, 3V3 + GND), boots, joins WiFi, serves the UI. Boot prints confirm the
HEX37 geometry (`ring sizes 1/6/12/18`); all HTTP endpoints exercised OK (`/state`,
`/set`, `/off`). Drove it red/center, then split-mode — the R channel pixel computed
onto index 19, confirming the triad geometry.

Features: brightness + R/G/B sliders (+ color picker), gamma toggle for smooth
low-end dimming; shape selector (center / +inner ring / +two rings / all, computed
from the real hex rings, center = px 18); animations — **Spiral** (single pixel
outward, trail slider), **Orbit** (single pixel around a chosen ring = the gobo
*moving-shadow* test), **Breathe**, **Twinkle**; **Freeze + Step+** to park a moving
pixel and read off its index; and **Split RGB** (Ben's ask) — pure R/G/B on three
pixels in a triad around an anchor, with **spread** (fringe width) + **rotate**
sliders, anchor walked by Step+ — to deliberately throw *wide separated color
fringes* through the gobo (vs the tight fringe of co-located channels). The page reads
back the exact current settings (rgb/hex, bri, shape, anim, lit pixel, split anchor/
spread) so a good-looking combo can be recorded precisely.

Bench wiring confirmed this session: **ACM1 = PowerFeather MCU, ACM0 = Apogee PAR
meter**, HEX on **pin 10**. Flash: `./build.sh --pin 10 --port /dev/ttyACM1`. The S3
is native-USB-CDC, so the boot banner (with the IP) only appears on a reset — pulse
RTS via pyserial (or just re-flash) to recover the IP; this session it came up at
192.168.4.209 (DHCP, may change). Next: Ben drives it through the inverted-lantern +
flat-filter rig (source on desk, shadow on ceiling) to compare point vs area vs
split-fringe looks and record what reads well.

## 2026-06-04 (cont. 10) — Ben + Claude — AMENDMENT: LED axis NOT resolved; RGBW undervolting is viable; gobo testing queued

Walking back two overstatements from the cont. 8/9 entries below. Those entries
stand as the record of what was measured, but their *conclusions* were too strong:

1. **"LED axis resolved / SK6812 HEX direct-GPIO is the BOM front-runner" — overstated.**
   The LED module is **not decided**. IS31-out is firm, but the HEX-direct and the
   4 W RGBW are **roughly tied in viability** and serve **different, complementary
   roles**, not the same one:
   - **SK6812 HEX direct-GPIO** = distributed / area source → **washes out the gobo**
     (good for general ambient glow), or animate by moving a single lit pixel around
     the hex (the cast-shadow-in-motion idea — untested, want to try it).
   - **4 W RGBW** = single **point source** → the only candidate that throws **crisp
     mandala shadows** through the gobo. A multi-LED array can't do that geometry.
   Because the gobo wants a point source and the ambient mode wants an area source,
   the "winner" may be **application-dependent** rather than one part. No frontrunner
   until gobo testing says so.

2. **"4 W RGBW needs 5 V" — overstated.** It is **voltage-starved at 3.3 V in this
   bench run** (non-monotonic mid-range current near its Vf), but Ben is fairly
   convinced from prior experience that **undervolting it is viable — 5 V is NOT
   required**, with caveats. What we actually have is a poorly-characterized low-V
   curve, not a hard 5 V requirement. **Open work:** properly map the RGBW's 3.3 V
   behavior — usable dimming range, color balance, max brightness — before deciding
   whether any boost is warranted.

Also flagging that the **PAR/mA efficiency ranking is muddied** by testbeds run at
different SOC/load (each LED run sat at a different buck-boost operating point — see
the Field-reliability "buck-boost efficiency vs VBAT" item), so the HEX-vs-NeoHEX
~1.6× and HEX-vs-RGBW comparisons are *system* efficiency at as-measured conditions,
not a clean intrinsic ranking. Re-rank at a fixed VBAT before trusting the slopes.

**Next:** basic gobo testing (point vs area source, crisp-shadow vs wash, the
single-moving-pixel animation idea) + a clean RGBW low-voltage characterization.
TODO + ADR 0018 amended to drop the single-winner framing. ADR 0018 rewrite should
record "IS31 out; HEX-direct and RGBW both live" — not a decided module.

## 2026-06-04 (cont. 9) — Ben + Claude — 4W RGBW characterized + full efficiency ranking (LED axis resolved)

Tested Adafruit 5163 (4 W addressable RGBW NeoPixel) direct-GPIO. At 3.3 V it's
**voltage-starved** — Vf ~3.0–3.2 V, and the rail sags into that band under load
(bv→3.11 V at full), so current is non-linear and it only reaches ~half its rated
output (~430 mA vs ~800 mA at 5 V). Diagnostic: `rgbw-undervolt.png`. **It needs 5 V**
(unlike the hex, which under-volts gracefully). Cleaner re-run via `--wifi-lowpower`.

Final PAR-vs-draw efficiency ranking (`led-par-vs-draw.png`, slope = PAR/mA):
- **RGBW 4 W: steepest + highest PAR (~38)** — brightest and most efficient *at high
  brightness*; but poor/non-linear dimming at 3.3 V and a single point source; wants 5 V.
- **HEX-direct ~0.07**, **HEX/NeoDriver ~0.055**, **NeoHEX ~0.04** (least efficient, out).

**Warm-white-only (RGBW W channel only, `--rgbw-white`):** the ultra-low-power "vibes"
mode — **~78 mA at full but dim (PAR 8)** at 3.3 V (W channel under-driven; brighter at
5 V). Efficient (~0.09 PAR/mA) but low absolute output. Cleaner data this run (45 s
dwell + 100% cell) confirmed the earlier low-brightness "PAR>0, mA≈0" was the measurement
floor (small LED current swamped by WiFi-baseline jitter), not real zero current. A clean
all-channel re-run (longer dwell) **agrees with the noisy one at the endpoints** (full
white ~430 mA / PAR 40, reproducible) and fixed the br=60 under-read (14→190 mA), **but the
mid-range stayed non-monotonic** (br=160 drew less current than br=100 yet more light) —
i.e. the messiness is the 4 W RGBW operating unstably *at its Vf on 3.3 V*, NOT measurement
noise. PAR (light) is monotonic; current is erratic. Confirms: the 4 W RGBW **needs 5 V**
for a clean/characterizable curve; at 3.3 V only the full-white point is trustworthy. So **LED
draw is a knob ~80 mA (dim warm) → ~430 mA (full RGBW); the artistic brightness target
picks the point.** Added flags `--rgbw-white`, `--step-ms`.

**LED axis resolves to a use-case choice:** distributed dimmable glow → **SK6812 HEX,
direct-GPIO @ 3.3 V** (no boost); single ultra-bright beacon → **4 W RGBW, needs 5 V
boost**; ultra-low-power warm ambient → **RGBW warm-white-only ~80 mA**. IS31 ruled out
(shared-bus brownout). Tooling added today: `--bright-sweep`,
`--sweep-max`, `--brightness`, `--pixel-pin`, `--wifi-lowpower`; `led_efficiency_sweep.py`
(+reboot-abort), `plot_led_eff.py`, `plot_par_vs_draw.py`, `plot_rgbw_diag.py`; Apogee
SQ-420 PAR reader.

## 2026-06-04 (cont. 8) — Ben + Claude — Direct-GPIO HEX validated; 3-way efficiency: direct-GPIO SK6812 wins

Soldered a 4-pin header on board 2 (3V3 · QON-NC · GND · A0=GPIO10) and drove the HEX
(SK6812) **direct from GPIO10** — no NeoDriver, off the I2C bus. Validated working
(`--led neohex --pixel-pin 10`). Then a capped efficiency sweep (`--sweep-max`, new flag)
overlaid on the NeoDriver curves (`led-eff-3way.png`):
- **Efficiency order: hex-direct ≥ hex(NeoDriver) > neohex.** Direct-GPIO HEX is ~10% more
  light/mA than HEX-via-NeoDriver (no passthrough/overhead loss), and both SK6812 beat the
  WS2812C NeoHEX (~1.6x).
- **Direct draws ~1.7-1.8x current+PAR per brightness setting** vs NeoDriver (br=60: 362 mA/
  PAR27 vs 215 mA/PAR15) — because the NeoDriver's Vin→pixel **passthrough drops voltage**
  and direct gives the LEDs the full 3.3 V (current is very VCC-sensitive near the WS2812/
  SK6812 low-V knee). Gap widens with current.
- **Confirmed by the 4-way 2x2** (`led-eff-4way.png`): NeoHEX shows direct≈NeoDriver (low
  current → negligible passthrough drop), while the high-current HEX shows the 1.7x gap — so
  efficiency is a chip property (HEX 1.6x), and the path-difference is current-dependent.
- **BOM front-runner: SK6812 HEX, direct-GPIO** — most efficient, fewest parts, brownout-safe
  by construction. Caveats: WS2812 latch their last frame (must send an explicit all-off to
  blank); connect/bring-up gently (full-white inrush browns the rail); higher VCC = browns a
  marginal cell sooner (run on a healthy pack / cap brightness).

Process findings logged: (1) board 2's USB-JTAG **auto-reset is flaky** — after flashing, tap
the physical reset if the green LED doesn't come up (chip is healthy; verified via esptool
flash_id). (2) **SOC is trustworthy while the cell stays connected** (held 91→92% across a
USB→battery unplug, only bv relaxed ~0.3 V) — the big SOC jumps earlier were from **cell
hot-swaps** resetting the gauge's coulomb state, not from USB power. New tooling: `--sweep-max`,
reboot-abort in `led_efficiency_sweep.py`, `ops/bench/plot_led_eff.py`.

## 2026-06-04 (cont. 7) — Ben + Claude — CORRECTION: NeoDriver does NOT boost pixel power (only the data signal)

Per Adafruit (product 5766): the NeoDriver's 5 V charge-pump is **only for the data
signal** ("clean 5 V signal even on 3 V boards") — it does **NOT** power/boost the
NeoPixels. *"No way the STEMMA QT port can provide that much current… need external 5 V
on the terminal blocks."* Pixel power = whatever feeds Vin (3–5 V), passed through.
- **Corrects** the earlier (cont. 3/5) claim that the NeoDriver "boosts Vin→5 V,
  self-contained." It does not.
- Explains the "dimmer on 3V3": pixels run at **3.3 V (under their 3.7–5 V spec)** →
  under-driven, not a boost current cap (the draw-vs-brightness curve doesn't plateau,
  confirming under-voltage scaling, not a current limit). On board 2's USB-hub 5 V the
  pixels got full 5 V → "blindingly bright."
- **BOM consequences:** (1) full brightness needs a real ~5 V pixel supply — battery
  (3.2–4.2 V) and 3V3 are below 5 V, so add a **5 V boost** for max brightness, or accept
  reduced brightness under-volted; (2) for dim/≤1 A operation under-volted is fine (matches
  the budget); (3) VBAT (≤4.2 V Li-ion) > 3V3 (3.3 V) for brightness without a boost;
  (4) the NeoHEX-vs-HEX efficiency was measured at 3.3 V (under-volt) — SK6812 tolerates
  low V better, so re-check the 1.6x edge at the actual ship voltage.
- Plot of the comparison: `ops/bench/data/ca/led-eff-compare.png` (via new
  `ops/bench/plot_led_eff.py`).

## 2026-06-04 (cont. 6) — Ben + Claude — NeoHEX vs HEX efficiency: HEX (SK6812) ~1.6x more light/mA

Built brightness-sweep tooling: fw `--bright-sweep` (steps brightness {0,5,15,30,60,100,
160,255}, 30s each, light-WiFi held constant, reports `br=` in heartbeat; br=0 = LEDs off
for a clean baseline) + `--brightness` flag + `ops/bench/led_efficiency_sweep.py` (reads
Apogee SQ-420 PAR on USB + board `ima` over WiFi, groups by br, prints PAR-per-LED-mA).
Setup: 6" tube, PAR sensor at top pointing down, module at base, NeoDriver Vin from 3V3.

- **Result: HEX (SK6812) ≈ 1.6x more light-efficient than NeoHEX (WS2812C-2020)** —
  PAR/LED-mA: NeoHEX ~0.040-0.045 (flat), HEX ~0.062-0.072, consistent across all
  brightness steps. At matched ~384 mA draw: NeoHEX PAR 15 vs HEX PAR 26 (~1.7x). HEX
  reaches higher max (PAR 30 @ 491 mA vs 16 @ 384 mA). **For the power budget, HEX wins.**
  Data: `ops/bench/data/ca/led-eff-{neohex,hex}.json`.
- Both SK6812/WS2812C are 37-px RGB (GRB), Grove→NeoDriver, no reflash to swap.
- **Caveats:** PAR is photon flux, not lumens (spectra differ, so perceived-brightness
  ratio may shift — but 1.6x is consistent across 6 levels); 6" low-SNR geometry (dim
  steps noisy, mid-high solid); color/dimming-smoothness not measured (visual call, also
  tends to favor SK6812). Full-white NeoHEX/HEX off 3V3 = 384/491 mA LED — within 1 A.
- Found + fixed a baseline bug: `setBrightness(0)` doesn't blank NeoPixels, so br=0 must
  set ledOn=false (color 0) for a true LED-off baseline.

## 2026-06-04 (cont. 5) — Ben + Claude — LED decision: IS31 ruled out, NeoHEX (via NeoDriver) leading; NeoHEX-vs-HEX + RGBW queued

- **3V3-powered NeoDriver works on battery:** board 1 (the brownout-prone unit) + NeoDriver
  fed from the **3V3 header** (dim, brightness 30 → ~0.5 A from 3V3, under the 1 A limit),
  STEMMA for I2C, on battery + WiFi → **no brownout** (Ben observed). Dim-30 is still
  "pretty bright." Added `--brightness` build-flag.
- **DECISION: IS31FL3741 13×9 ruled out for the V2 battery product.** Cause: its presence
  on the V2's shared charger/gauge I2C bus + WiFi reliably browns out on battery
  (well-proven, IS31-specific). Caveats noted: (a) untested mitigations — VSYS bulk cap, or
  moving it to the *second* I2C bus (GPIO35/36, not the shared bus) — might rescue it; (b)
  it's a 13×9 grid vs the hex form. **Revisit only if the grid aesthetic is a hard
  requirement.** Supersedes ADR 0018 (IS31 as primary module) for the battery build —
  flag ADR 0018 for an update.
- **Leading LED path: NeoHEX (WS2812C-2020) via Adafruit NeoDriver** — no brownout, no
  solder on the I2C side, self-contained (NeoDriver boosts 3–5 V Vin → 5 V + level-shifts
  data). Continue stability testing.
- **Queued tests:** (1) **NeoHEX (WS2812C-2020) vs HEX (SK6812)** head-to-head — color
  quality, dimming smoothness (low-end PWM), power efficiency vs brightness, low-V behavior
  (SK6812 generally better at low V / finer PWM; WS2812C-2020 smaller/denser). (2) **single
  high-power RGBW LED.** (3) LED-current measurement at field brightness (folds into #1).

Fixed the brick-risk that ate ~1 h today (no-wake deep sleep stranded board 2, needed
BOOT+RESET download-mode + `esptool erase_flash`). fw `power-bench-2026-06-04.2`:
- **Never deep-sleep while external supply present** (USB/VDC) — root cause of the
  stranding; on supply the board stays flashable/recoverable and there's no brownout
  risk anyway. `lgSupplyPresent()` = `getSupplyVoltage > 4.0 V`.
- **Timer wake** (15 min) instead of indefinite, via `esp_sleep_enable_timer_wakeup`.
- On a timer wake **still on battery → re-sleep** (protect cell); **on supply → run/
  charge**. So plugging USB self-recovers within one interval; can't brick.
- Unified `lgEnterDeepSleep()` (loop-break, coulomb-budget, lowbatt-knee, maxrun all
  route through it; LED-clear guarded for IS31/NeoPixel/NeoDriver). Compiles clean for
  all LED variants.
- **VALIDATED LIVE** (3 mAh budget / 60 s wake, `--budget-mah`/`--wake-s` flags): on USB
  ran continuously w/o sleeping (charging, mah=0); on battery hit the 3 mAh budget →
  SLEEPING announce → deep sleep; 124 s of timer-wake/re-sleep silence on battery; then
  USB plug → recovered on the next wake (fresh boot, ima=+438 charging) with **no
  BOOT+RESET download-mode needed**. Brick-risk resolved.

## 2026-06-04 (cont. 3) — Ben + Claude — NeoDriver (I2C) is STABLE: brownout is IS31-SPECIFIC, not the bus

Built a `--led neodriver` variant (Adafruit NeoDriver 5766, SeeSaw I2C → WS2812, on the
STEMMA bus; added Adafruit_seesaw lib + seesaw_NeoPixel in lgApplyLed). Drove a NeoHEX
full-white, **LED 5 V from an external USB hub** (LED current off the battery; the
NeoDriver boosts 3–5 V Vin → 5 V and level-shifts data, per its silkscreen).

- **Result: STABLE** — board 2, NeoDriver on the same shared I2C bus, battery + WiFi,
  full-white → **371 s+, 0 reboots, through the heavy-WiFi phase**, bv steady 3.25. Same
  board/cell/bus/WiFi that **looped the IS31 within ~1 min**.
- **Verdict: the brownout is IS31-SPECIFIC**, not "any I2C device on the power-mgmt bus."
  Since the IS31 browns out even LEDs-off (presence alone), it's the IS31FL3741 chip's
  electrical behavior on SDA/SCL (back-current/loading during WiFi spikes), not LED
  current and not a general bus property. Matches Ben's hypothesis, isolated to the part.
- **LED-axis implication:** I2C LEDs are NOT categorically out. **NeoDriver + WS2812
  (NeoHEX) is a strong no-solder, self-contained LED path** (bright, onboard 5 V boost +
  data level-shift, no extra parts) that does NOT brown out the V2 on battery.
- **Caveats:** n=1, ~6 min; the IS31 was *intermittent* (stable for minutes before
  failing overnight), so the NeoDriver needs an **hours/overnight** run to trust. And
  that needs the **auto-sleep wake-source fix first** (brick-risk; on TODO) — today the
  no-wake deep sleep + download-mode recovery cost ~1 h and corrupted board 2's WiFi
  (fixed via `esptool erase_flash`).

## 2026-06-04 (cont. 2) — Ben + Claude — IS31 presence on the I2C bus is NECESSARY for the brownout (clean A/B)

Decisive test: board 2, same deep-cycled cell, on battery, **IS31 physically unplugged**
→ **stable 365 s+, 0 reboots, through light AND heavy WiFi** (bv 3.27, soc 93). Versus
the same board+cell **with** the IS31 → brownout loop. Only variable changed = the IS31
on the STEMMA/I2C bus.

- **The IS31's presence on the shared I2C bus is necessary.** Rules out cell+WiFi alone
  (stable) and WiFi-association-inrush alone (stable). Loops occurred in phase 0 with
  **LEDs off**, so it's **not LED current** — it's the chip on the bus. Matches Ben's
  back-current / I2C-disturbance hypothesis.
- **Still open:** (a) IS31 *actively* misbehaving (spikes/back-current on SDA/SCL) vs
  (b) *any* I2C device loading the shared charger/gauge bus tips VSYS under WiFi.
  Next test: Adafruit NeoDriver (5766, I2C SeeSaw) on the same bus, NeoPixels powered
  externally → also brownouts ⇒ (b); clean ⇒ (a). Needs a SeeSaw NeoPixel driver in fw.
- **Procurement note:** an I2C LED module on the V2's shared power-management bus is a
  real risk for the battery product; nudges toward a non-shared-bus (GPIO/SeeSaw-with-
  external-power) LED path, or bus isolation / bulk cap mitigation.
- Aside: board 2's WiFi wedged after the brownout/deep-sleep/download-mode gauntlet;
  recovered only via full `esptool erase_flash` + reflash + clean reboot (corrupted
  PHY/NVS). The loop-breaker's no-wake-source deep sleep also needed manual BOOT+RESET
  download-mode to reflash — both reinforce the wake-on-USB fix already on the TODO.

## 2026-06-04 — Ben + Claude — Brownout CAME BACK overnight (794-reboot loop); guard flaw fixed; SOC/voltage thesis confirmed

Left board 1 on the loadgen on battery overnight (coulomb-budget auto-sleep at 91%
SOC). Morning: a **794-reboot loop over 4.25 h** — every reset `poweron` (VSYS
collapse), at **healthy bv 3.24–3.46 across SOC 98%→30%**, in the **lightest** phase
(LEDs off, light WiFi), boots dying ~5–9 s in (around WiFi association). The first
boot ran 112 s, then a steady ~100 reboots / 30 min.

- **The brownout is real + intermittent on board 1.** Yesterday's "non-reproduction"
  (n=3 boards stable, capstone, wiggle) was the fluke; it drifts marginal over
  hours/temperature. Strengthens **H2 (marginal connection on board 1)**; per-boot
  trigger looks like the **WiFi-association current spike**, not load-stacking
  (lightest load) and not depletion (healthy V at every SOC).
- **Guard flaw (Ben called it):** coulomb-budget + max-runtime + low-V auto-sleep are
  all RAM state that resets each reboot, so a tight loop defeats them (`mah_used`
  never passed 1.4 of the 1000 mAh budget). It only bled slowly (92%→30%) because
  each short boot draws little. **Fix:** NVS-persisted boot counter (`--autosleep`) —
  clean start (USB/SW reset) zeroes it, `poweron` boots increment, ≥25 sub-survival
  boots ⇒ deep sleep before WiFi.begin; a boot surviving 120 s clears it. fw
  `power-bench-2026-06-04.1`. Heartbeat now also carries `soc=` and `mah=`.
- **SOC/voltage thesis confirmed hard:** bv pinned at ~3.24 V for 4 h while gauge SOC
  drained 92%→30% — LFP voltage is useless for SOC, but the gauge's coulomb count
  tracked the drain (it's the *voltage* that's untrustworthy, not the gauge number).
  Plots via new `ops/bench/plot_soc_v.py`:
  `2026-06-02-ca-liion-4400-soc_v.png` (Li-ion, usable slope) vs
  `2026-06-03-ca-lfp-overnight-soc_v.png` (LFP, near-vertical plateau). Logger:
  `ops/bench/loadgen_log.py` (JSONL + inline reboot flags + LED-current A/B).
- **Now running (2026-06-04):** same cell+grid on **pristine board 2**, multi-hour
  with the fixed guard — board-specificity test (loop like board 1, or run clean?),
  and if stable it finally captures the LED-current A/B + LFP V-SOC discharge curve.

### 2026-06-04 (cont.) — board 2 ALSO loops (NOT board-specific); loop-breaker validated

- **Board 2 (pristine) brownout-looped too** — first boot 356 s (reached phase 1,
  grid lit), then collapsed on the USB→battery unplug (Ben watched the grid cut out at
  the instant of unplug = the first brownout), then looped (poweron, healthy bv ~3.23,
  soc ~72). So the brownout is **NOT board-1-specific** — overturns the "board 1 solder
  joint" read. Common factors across all looping cases: the **cell** (deep-cycled
  overnight), the **IS31 grid + cable**, firmware.
- **Loop-breaker FIRED (fix validated in the wild):** board 2 deep-slept itself out of
  the loop. Logger saw only 8 reboots but the firmware NVS counter counts every boot —
  including the sub-association boots that die before sending any UDP — so it hit 25 and
  slept while staying silent to the logger. Cell protected at ~72%/3.23 V.
- **Temperature ruled out** (Ben: office 72.5 °F now, ~74 when it worked, 79 max — too
  narrow to matter).
- **Leading hypotheses now:** (Ben) the **IS31 driver latching into a bad state** →
  back-current/spikes on SDA/SCL (fits: IS31-unplugged always stable; `enableVSQT(false)`
  never helped = I2C back-power); vs the **deep-cycled cell's raised ESR** exposing the
  IS31+WiFi load. Next: (1) unplug IS31 + rerun same cell (presence necessary?), (2) GPIO
  WS2812 vs IS31 (I2C-specific vs load), (3) fresh cell + IS31 (cell-ESR).

## 2026-06-03 (cont. 2) — Ben + Claude — Brownout does NOT reproduce on n=3 boards; supersedes the "load-stacking" conclusion

**Walk-back of the entry below.** We lifted n=1→n=3 by moving the **same LFP cell,
same IS31 grid, same STEMMA cable** across three boards (only the board changed), then
re-tested the original board. Result: the brownout reproduces on **none** of them.

- **Board 2** (pristine): stable, light + heavy WiFi, 0 resets, bv to 3.19 V.
- **Board 3** (pristine): stable, light + into heavy, 0 resets, bv to 3.20 V.
- **Board 1** (the one that browned out earlier, capstone re-test, identical setup):
  **stable**, 4 min, 0 resets, bv 3.24 V.
- **Wiggle test** on board 1: 30 s of hard mechanical stress on the leads/connector
  **plus STEMMA hot-replugs** (the action that caused an instant reset earlier) →
  **0 resets / 0 dropouts over 200 s**. Could not re-induce the collapse by any means.

**So both earlier conclusions are wrong/superseded:** not a platform "load-stacking"
property (boards 2/3 fine), not "board 1 anomalous" (board 1 now fine too). With board,
cell, grid, and cable all held constant, the only thing that changed across the
afternoon is **repeated unplug/re-seat of connectors** → leading explanation is now
**H2: a marginal physical connection** (soldered battery joint and/or STEMMA seat) that
re-seated. **Inferred, not confirmed** — we showed the brownout *stopped*, not *why*,
and could not reproduce it even deliberately. Also notable: stable while in **active
boost** at 3.24 V (the *harder* regime) argues against H3 (low-LFP/boost instability).

**Bottom line for procurement (unblocked):** three V2 boards run IS31 + continuous WiFi
on battery with zero brownouts down to ~3.2 V, so we **cannot** call V2 + IS31 unsafe on
battery. We also **cannot** claim full root-cause understanding (non-reproducible). Carry
a **VSYS bulk cap as cheap insurance** and watch for recurrence in the field. Full
write-up (Status, board-swap table, superseded sections) in
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`. Lesson logged: we wrote a firm
conclusion twice today and were wrong both times — n=1 + a single connection was not
enough.

## 2026-06-03 (cont.) — Ben + Claude — Brownout cause isolated: IS31-on-bus + WiFi (load-stacking) [SUPERSEDED by the entry above]

On a SOLID soldered LFP connection (the spring splice had confounded earlier runs)
and with cleaned-up instrumentation (uptime-based phase, no NVS write, `reset_reason`
+ battery V/I in the UDP heartbeat), the brownout reproduced cleanly and we isolated
it. Full write-up + open questions in
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`.

- WiFi off (any LED): stable. WiFi on + IS31 **unplugged** (light or heavy TX):
  stable (9 min, 0 resets, bv to 3.24 V). WiFi on + IS31 **connected**: `poweron`
  brownout ~7–17 s.
- **Cause:** load-stacking — needs BOTH WiFi active AND the IS31 module physically on
  the STEMMA/I2C bus; neither alone does it. `reset_reason=poweron` (VSYS collapse) at
  healthy bv → not depletion / connector / chemistry. Modem sleep did not fix it.
- **Sub-result:** firmware VSQT power-shed (`enableVSQT(false)`) did NOT fix it (~21
  resets / 7 min) — only physically unplugging the module stops it. Candidate
  mechanism: I2C back-powering (IS31 stays on SDA/SCL off the main 3V3). Unproven.

Implications (firming, not final; n=1 board): **VSYS bulk capacitance** is the
mechanism-independent fix (bench-validate next); an **I2C LED module can't be
software-shed** (back-power) whereas a **GPIO WS2812** could; OTA-on-battery shouldn't
rely on VSQT-shed for the IS31 (use bulk cap / daytime solar / a GPIO module).

Also: ported demo gained an **Input Current Limit (IINDPM) slider** — confirmed the
~500 mA USB charge cap is the **BC1.2/USB-C source-detection default** (not a port
bug; the SDK sets IINDPM=3200 but USB-C advertises current via CC, not D+/D-).
Doesn't affect solar/VDC charging. Tooling: loadgen heartbeat now carries
phase+uptime+bv+reset_reason+lb+sqt, low-batt backoff, and a `--loadgen-shed` mode.

## 2026-06-03 — Ben + Claude — Battery-brownout investigation: tooling, plan, ported demo (ONGOING, no conclusions yet)

Investigating the precise conditions under which the PowerFeather V2 takes a full
power-on reset on battery while running fine on USB. Observations so far are
partial and several are **confounded** (a marginal spring-splice test connection
on the bare LFP, battery type switched mid-investigation, stacked loads), so this
entry records **tooling and a plan, not findings**. Plan, hypotheses, and the open
test matrix are in `docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`.

Added bench tooling to `firmware/power_bench` (via `build.sh` flags):
- `--loadgen`: WiFi load generator (no HTTP server) emitting a UDP heartbeat with
  phase + uptime + battery voltage for remote outage/reset detection; auto-sweeps
  {light/heavy WiFi} x {LED off / full grid}. Phase persisted in NVS so it advances
  past (not retries) a phase that reboots the board.
- `--batt-stress` / `--batt-stress-full`: radio OFF, LED-panel heartbeat (center or
  full grid) — radio-off baselines.
- `--wifi-lowpower` (modem sleep + 8.5 dBm), `--charge-ma`, `--ota` (wireless flash).

Ported PowerFeather's official ESPUI web-telemetry demo to V2 / SDK 2.x / core 3.x
(`firmware/powerfeather_demo_port`): SDK 1.x->2.x API (mV->V floats, maintain-voltage
units), `Generic_LFP`, and the ESP32Async core-3.x library stack. Compiles, boots,
and brings up the `PowerFeather_Demo` AP on V2 (verified on USB); web UI + on-battery
behavior still to exercise with a phone + a solid battery connection.

Next: re-run the matrix on a solid (soldered) LFP connection at known SOC.

## 2026-06-02 — Ben + Claude — PowerFeather V2.R2 power-bench bring-up (Phase A)

PowerFeather V2.R2 arrived. Stood up an Arduino-based power-telemetry bench
harness on it. New firmware `firmware/power_bench/` forked from `smoke_test`,
adding PowerFeather-SDK telemetry and a JSON `/telemetry` endpoint for WiFi data
collection across the three test axes (battery, LED option, solar panel).

Toolchain confirmed: FQBN `esp32:esp32:esp32s3_powerfeather`, board macro
`ARDUINO_ESP32S3_POWERFEATHER`, ESP32 core 3.3.7, PowerFeather-SDK 2.1.0
(namespace `PowerFeather`, singleton `Board`, `<PowerFeather.h>`). LED libs already
installed.

Battery chemistry is firmware-only (no jumpers): `Board.init(capacity_mAh,
BatteryType)` — `Generic_3V7` for Li-ion (current), one-line swap to `Generic_LFP`
for LiFePO4. Note the SDK leaves charging DISABLED by default; the firmware now
calls `enableBatteryCharging(true)` with a conservative 200 mA cap (configurable).

Flashed and validated against the SDK validation plan (board `9E5AB8`, fixture on
WiFi at `192.168.4.185`), with a 4400 mAh PKCell Li-ion (2x18650), a 1 W panel on
VDC, and the IS31FL3741 13x9 on STEMMA-QT:
- Phase 1: I2C scan of Wire1 (STEMMA-QT, GPIO47/48) shows MAX17260 gauge (0x36),
  BQ25628E charger (0x6A), and IS31 (0x30) -> confirmed V2 hardware. The STEMMA-QT
  bus is shared by the power ICs and the LED module; the IS31 driver uses `Wire1`.
- Phase 2: `Board.init(4400, Generic_3V7)` returns `Result::Ok`; charging enabled
  at 200 mA cap; no SDK errors.
- Phase 3: `/telemetry` JSON serves correct values over WiFi — `battery_v` 3.60 V,
  `battery_ma` +204 mA (charging at the cap), `supply_v` 4.665 V, `supply_ma`
  ~236 mA, `supply_good` true. Power balances: ~1.1 W in, ~0.73 W into the cell.

Two findings:
1. BUG (fixed): the float telemetry fields were one-position shifted due to C++
   unspecified argument-evaluation order — the SDK getter was inlined as a function
   argument alongside the out-param it writes. Sequenced the getter before the JSON
   append (matching the integer-field pattern). Confirmed against the SDK's stock
   `SupplyAndBatteryInfo` example, which read correctly the whole time.
2. ROOT CAUSE FOUND + FIXED: `soc_pct/health_pct/cycles/time_left_min` returned
   `InvalidState` because the SDK selects the fuel-gauge IC at COMPILE TIME —
   MAX17260 (V2) only if `POWERFEATHER_BOARD_V2`/`CONFIG_ESP32S3_POWERFEATHER_V2`
   is defined, else the V1 `LC709204F`. In an Arduino build neither is set, so it
   defaulted to the V1 gauge and `probe()` failed on the wrong IC (the stock SDK
   example fails the same way for the same reason). A power-cycle did not help — it
   was never a learning issue. Fix: build with `-DPOWERFEATHER_BOARD_V2=1` (now in
   `firmware/power_bench/build.sh`, with a `#error` guard in the sketch). With the
   flag: gauge = MAX17260, probe ok, `soc 7%`, health 100%, cycles 0, time_left,
   `telemetry_errors []`. Also added an init retry for the post-flash boot transient.

Also noted: mode `q` (quiet baseline) stops WiFi, so the WiFi logger must use mode
`0` (LEDs off, radio on) as its baseline. And the 200 mA charge current dominates
LED-current deltas, so clean LED measurement wants `-DRES_PF_ENABLE_CHARGING=0`.

Phase B done: `ops/bench/power_logger.py` (WiFi poller -> site-partitioned JSONL),
`power_summary.py`, `ops/bench/data/{ca,tn}/`, ADR 0020, and
`docs/tests/POWER_BENCH_HARNESS_2026-06-02.md`. Logger + summary validated against
the live board. Firmware variant builds (IS31/NeoHEX/RGBW) all compile.

## 2026-05-20 — Ben + Codex — PCBWay assembly quote revised toward J5-only

PCBWay's first assembly quote identified J1 / M5Stack A118 as the expensive and
slow part: about $32.82 for five assembled boards and 7-10 working days of
component lead time. Revised the PCBWay packet to match the practical prototype
path:

- Keep J1 pads in the Gerbers for later hand-solder/fit testing.
- Mark J1 DNP for assembly so PCBWay does not source the A118 connector.
- Use J5 as the assembled LED output through the Grove-to-STEMMA-QT cable.
- Keep C2 DNP.
- Update PCBWay notes and BOM to six placed SMD parts: J2, J3, J4, J5, R1, C1.

PCB fabrication counts remain 46 SMT pads and 14 drill holes. Assembly counts
are now six SMD components, zero through-hole components, and DNP parts J1/C2.

## 2026-05-18 — Ben + Codex — PCBWay packet prepared for NeoHEX adapter

Created `hardware/led-adapter/neohex-passive-rev-a/manufacturing/pcbway/` with
a self-contained quick-turn PCBA upload packet:

- `neohex-passive-rev-a-gerbers.zip` with Gerbers plus drill file.
- `bom-pcbway.csv` with only populated parts: J1, J2, J3, J4, J5, R1, C1.
- `neohex-passive-rev-a-pos-pcbway.csv` with C2 filtered out as DNP.
- `neohex-passive-rev-a-pos-all.csv` as a full centroid reference.
- `ORDER_NOTES.txt` and `README.md` with PCBWay settings, DNP notes, solder
  jumper notes, and pad/hole counts.
- `drc.rpt` showing zero violations and zero unconnected items.

For the PCBWay enquiry, use 46 SMT pads and 14 drill holes if they mean board
fabrication counts; use 7 SMD components and 0 through-hole components if they
mean assembly placement counts.

## 2026-05-18 — Ben + Codex — NeoHEX adapter gained JST-SH fallback output

Added a second LED-output receptacle to the NeoHEX passive adapter starter PCB:

- Kept J1 as the local M5Stack A118 HY2.0-4P SMD candidate.
- Added J5 as a stock JST-SH 4-pin SMT receptacle intended for an Adafruit
  4528-style Grove-to-STEMMA-QT cable.
- Wired J5 in parallel with J1 so Rev A can use either output without solder
  rework; the unused output should be left unplugged.
- Mapped J5 as `1 GND`, `2 VLED`, `3 NC`, `4 DATA_OUT`, matching the NeoHEX
  signal on the Grove yellow/SCL-position conductor.
- Updated the design packet, BOM, netlist, KiCad README, and TODOs.

`kicad-cli pcb drc` reports zero violations and zero unconnected items after
adding J5. Remaining risks are physical cable/footprint verification, J2 power
harness verification, and schematic capture/back-check.

## 2026-05-18 — Ben + Codex — NeoHEX adapter moved toward SMT PCBA

Ben preferred a PCBA-friendly adapter because the board will sit inside the
enclosure and should not see meaningful cable forces. Reworked the NeoHEX
adapter starter PCB away from through-hole populated connectors:

- Added local footprint library `hardware/led-adapter/neohex-passive-rev-a/kicad/resonance.pretty/`.
- Added local `M5Stack_HY2.0-4P_SMD_A118` candidate footprint for J1, based on
  the M5Stack A118 HY2.0-4P SMD connector dimensions.
- Replaced J2 with stock SMT
  `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal`.
- Grew the starter board to 72 mm x 35 mm so the larger SMT connector bodies,
  routing, and labels remain easy to inspect.
- Updated the J1 silkscreen label to `J1 HY2.0 SMD` next to the connector.

`kicad-cli pcb drc` reports zero violations and zero unconnected items after the
SMT conversion. The design is still not order-ready: physically verify J1
against the actual M5Stack Grove/HY2.0 cable, verify J2 against the chosen power
lead, and capture/back-check the schematic before sending to assembly.

## 2026-05-18 — Ben + Codex — Smoke mode 1 changed to max center

Changed COTS smoke firmware mode `1` from dim warm-white center to max-white
center for each board class:

- IS31FL3741: `LEDscaling=0xFF`, `globalCurrent=0xFF`, center pixel white.
- NeoPixel-backed boards: global brightness remains `255/255`, center pixel is
  now `(255, 255, 255)`.

Bumped firmware to `smoke-2026-05-19.1`, updated the smoke README and COTS mode
dashboard label to `1 Center Max`, built all four variants, and OTA-flashed:

- `192.168.4.248` / fixture `E41B2C` / C6 + IS31FL3741.
- `192.168.4.249` / fixture `570D32` / FeatherS2 Neo.
- `192.168.5.32` / fixture `1B5108` / Atom Matrix.
- `192.168.4.27` / fixture `55BA78` / Atom + NeoHEX.

All four boards reported `smoke-2026-05-19.1` and mode `1 center_max_white`
after flashing. Atom + NeoHEX needed a throttled OTA retry
(`curl -H 'Expect:' --limit-rate 40k ...`) after normal multipart upload attempts
failed.

## 2026-05-18 — Ben + Codex — KiCad 10 starter PCB for NeoHEX adapter

Ben upgraded KiCad from the Ubuntu 22.04 package to KiCad 10 via the KiCad PPA.
Verified `kicad-cli` is now available and reports `10.0.3`; the `pcbnew`
Python module also reports `10.0.3`.

Added a KiCad starter project at
`hardware/led-adapter/neohex-passive-rev-a/kicad/`:

- `neohex-passive-rev-a.kicad_pro` — KiCad 10 project file.
- `neohex-passive-rev-a.kicad_pcb` — routed 60 mm x 35 mm starter layout.
- `generate_starter_pcb.py` — reproducible generator for the starter PCB.
- `README.md` — KiCad-specific caveats and validation commands.

The starter layout keeps Rev A passive: external `VLED` injection, shared
ground, selectable STEMMA/GPIO data input, 330 ohm data resistor, local
decoupling, optional `SJ4` STEMMA_V+ bridge marked for low-current testing only,
and test pads. `kicad-cli pcb drc` reports zero violations and zero unconnected
items, and Gerber/drill export succeeds into `/tmp/res-neohex-kicad/`.

Important caveat: J1 is still a placeholder JST-PH 1x04 2.0 mm footprint standing
in for the exact M5Stack Grove/HY2.0 socket, and no schematic has been captured
yet. Do not order this board until J1 is replaced with the exact connector
footprint, cable pin order is verified, and the schematic/PCB are back-checked.

## 2026-05-18 — Ben + Codex — NeoHEX passive adapter Rev A design packet

Started a small PCB workstream for a no-solder-ish HEX/NeoHEX adapter board as both an educational PCB exercise and a possible 100-unit assembly aid.

Added `hardware/led-adapter/neohex-passive-rev-a/`:

- `README.md` — design intent, schematic, connector pinouts, layout guidance, assembly variants, bring-up checklist, and open questions.
- `bom.csv` — first-pass BOM for Grove/HY2.0 output, external LED power input, STEMMA/QT data input, optional generic GPIO input, data resistor, decoupling, jumpers, and test pads.
- `netlist.csv` — explicit nets for KiCad capture.

Rev A is intentionally passive: connectors, shared ground, power injection, one data-source solder jumper, 330 ohm data resistor, and optional bulk capacitance. It does not include a boost regulator or constant-current driver. Added TODO items to capture the board in KiCad and order quick-turn boards.

## 2026-05-18 — Ben + Codex — Planned iso-current LED brightness test

Added `docs/tests/ISO_CURRENT_LED_BRIGHTNESS_TEST_2026-05-18.md` after visual smoke testing showed large brightness differences between full-low modes: roughly `FeatherS2 Neo >> NeoHEX ~= IS31FL3741 > Atom Matrix`, with the Atom Matrix diffuser likely contributing.

The new test plan separates electrical normalization from optical/gobo evaluation. It defines current targets, pattern classes, measurement setup with SEN0291 wattmeters, fixed-camera optical procedure, and result tables. Added a TODO item to run the test once the SEN0291 wattmeters are available.

## 2026-05-18 — Ben + Codex — Standalone Atom recovered on new subnet

The standalone Atom Matrix + DFRobot DFR0559 stack appeared unreachable from the dashboard at its old address `192.168.4.250`. After Ben moved it from the DFR0559 output to direct USB, serial confirmed it was healthy and connected to `BubbyNet`, but DHCP had assigned `192.168.5.32`.

Serial report:

- Board: `m5stack_atom`
- MAC: `F8:B3:B7:1B:51:08`
- Fixture ID: `1B5108`
- Reset reason: `poweron`
- Previous firmware: `smoke-2026-05-15.7`
- WiFi IP: `192.168.5.32`

OTA-updated the Atom to `smoke-2026-05-18.2` at `192.168.5.32` and updated the local COTS mode dashboard from the stale `192.168.4.250` address. The board was warm while powered from the DFR0559 even with LEDs off; no firmware fault was visible over USB. Follow up with SEN0291 current measurements on the DFR0559 5 V output before leaving that stack powered unattended.

## 2026-05-18 — Ben + Codex — NeoHEX center-cluster mapping adjustment

Ben observed that Atom + NeoHEX mode `3` appeared as a single seven-LED column. The placeholder NeoHEX crop used contiguous indices `15..21`, which confirms the NeoHEX chain appears to be indexed by hex columns rather than by a rectangular 3x3 layout.

Updated the Atom + NeoHEX crop for `smoke-2026-05-18.2` to use a first-pass center hex cluster around center index `18`: `11, 12, 17, 18, 19, 24, 25`. Built the Atom + NeoHEX variant and OTA-flashed `192.168.4.27`; the board came back as `smoke-2026-05-18.2`, and `/mode?m=3` succeeded.

Network scan found the reachable smoke boards at `192.168.4.27`, `192.168.4.248`, and `192.168.4.249`. The standalone Atom + DFRobot DFR0559 stack at prior address `192.168.4.250` remains unreachable; likely next checks are DFR0559 ON jumper position, battery/output recovery via BOOT, supply stability, and then USB serial recovery if needed.

## 2026-05-18 — Ben + Codex — Atom + NeoHEX smoke-test variant

Fourth COTS prototype connected over USB: M5Stack Atom Matrix v1.1 on an Atomic Battery Base, connected to M5Stack Unit NeoHEX over Grove.

Added a compile-time smoke-test variant for Atom + NeoHEX:

- Build flag: `--build-property compiler.cpp.extra_flags=-DRES_ATOM_GROVE_NEOHEX=1`
- Board name: `m5stack_atom_neohex`
- NeoPixel data pin: GPIO26, matching the Atom Grove yellow signal wire.
- Pixel count: 37.
- Initial center index assumption: 18.

USB-flashed the new Atom over `/dev/ttyUSB0`. It reported MAC `14:08:08:55:BA:78`, fixture ID `55BA78`, and joined home WiFi at `192.168.4.27`. The OTA web page reports `smoke-2026-05-18.1`, board `m5stack_atom_neohex`, and mode `0`. Verified `/mode?m=2` then `/mode?m=0` over HTTP.

Also OTA-updated the reachable C6 + IS31FL3741 board and FeatherS2 Neo board to `smoke-2026-05-18.1`. The original standalone Atom Matrix at `192.168.4.250` was not reachable during this pass and remains to be updated when powered/reconnected.

Updated the local COTS mode dashboard to include Atom + NeoHEX, and added the new stack to the LED measurement worksheet. The existing C6, FeatherS2, and regular Atom smoke-test builds still compile.

## 2026-05-15 — Ben + Codex — Brightness calibration fix for smoke-test modes

Ben observed that several LED measurement modes were effectively invisible, especially on the Atom Matrix: `4` full-low was invisible, `5` capped full-array was extremely faint, and `1` center was too dim. Root cause was double dimming on NeoPixel boards: low RGB component values were also being multiplied by low `Adafruit_NeoPixel::setBrightness()` values, causing integer scaling to round many channels down to 0 or 1. The IS31FL3741 full-low mode also used RGB values below RGB565's low-end quantization threshold.

Updated `firmware/smoke_test/` to `smoke-2026-05-15.7`:

- NeoPixel measurement modes now use `setBrightness(255)` and control current with explicit low raw RGB values.
- IS31FL3741 modes now avoid RGB565 values that quantize to black.
- Mode `1`, `3`, `4`, and `5` brightness levels were raised while keeping capped full-array modes conservative.

Built and OTA-flashed `.7` to all three unplugged boards over WiFi. All three returned to mode `0`, and `/mode?m=5` then `/mode?m=0` succeeded on C6 + IS31FL3741, FeatherS2 Neo, and Atom Matrix.

## 2026-05-15 — Ben + Codex — Static COTS mode dashboard

Added `ops/bench/cots-mode-dashboard.html`, a local static dashboard for the three active smoke-test boards:

- C6 + IS31FL3741 at `192.168.4.248`
- FeatherS2 Neo at `192.168.4.249`
- Atom Matrix at `192.168.4.250`

The page sends `/mode?m=<mode>` commands by iframe navigation rather than `fetch()`, so it works from a local `file://` page without requiring CORS headers from the ESP web server. It includes per-board and all-board controls for modes `0`, `1`, `2`, `3`, `4`, `5`, and `q`, plus embedded board status iframes.

## 2026-05-15 — Ben + Codex — OTA and USB flash timing benchmarks

Ben ordered 12 DFRobot SEN0291 I2C digital wattmeters, so manual USB power-meter experiments are on hold until they arrive. Added a TODO item to integrate the wattmeters into the power-test harness/worksheets.

Ran first flash timing benchmarks on `smoke-2026-05-15.6`; details are in `docs/tests/OTA_FLASH_BENCHMARKS_2026-05-15.md`.

Results:

- Strict sequential OTA, waiting for each board to be reachable again: 44.123 s for 3 boards.
- Parallel OTA batch: 18.291 s for all 3 boards to upload and become reachable again.
- USB upload, excluding compile time: C6 7.109 s upload / 10.188 s ready; FeatherS2 Neo 13.047 s upload / 16.218 s ready; Atom Matrix 14.287 s upload / 17.515 s ready.

FeatherS2 had one failed USB reset/upload attempt (`Errno 71`) that left it in the ESP32-S2 bootloader; a recovery USB upload succeeded, and a subsequent normal USB upload also succeeded. All three boards are back online at `smoke-2026-05-15.6`, mode `0`.

## 2026-05-15 — Ben + Codex — LED measurement firmware loaded on COTS smoke boards

Extended `firmware/smoke_test/` into a deterministic LED measurement harness and bumped it to `smoke-2026-05-15.6`.

New serial/HTTP measurement modes:

- `q` — quiet baseline: stop OTA/WiFi and clear LEDs.
- `0` — LEDs off, current WiFi/OTA state unchanged.
- `1` — center dim warm white.
- `2` — 3-pixel RGB fringe.
- `3` — center 3x3 dim warm white.
- `4` — full-array very-low white.
- `5` — full-array capped white, brief measurements only.

The OTA status page now shows the active mode and exposes `/mode?m=<mode>` links, so the USB current meter workflow can use either serial commands or `curl` while WiFi OTA is active. Added `docs/tests/COTS_LED_MEASUREMENTS_2026-05-15.md` as the worksheet for current and optics readings.

Built and uploaded `smoke-2026-05-15.6` over HTTP OTA to all three connected boards:

- C6 + IS31FL3741: `192.168.4.248`
- FeatherS2 Neo: `192.168.4.249`
- M5Stack Atom Matrix: `192.168.4.250`

All three served `Version: smoke-2026-05-15.6`, accepted `/mode?m=1`, and were left in mode `0` with LEDs off and OTA still available. LED-current readings are still open; record them in the new worksheet.

## 2026-05-15 — Ben + Codex — Home-WiFi web OTA validated on all three COTS smoke boards

Committed and pushed the initial smoke-test baseline as `f36595e Add COTS smoke test firmware`.

Added station-mode web OTA support to `firmware/smoke_test/`:

- `wifi_secrets.h` is now ignored by git.
- `wifi_secrets.h.example` documents the local secrets format.
- Serial command `w` connects to configured WiFi and starts the same web updater.
- Serial command `o` still starts temporary AP OTA mode.
- `RES_WIFI_AUTO_CONNECT` allows bench firmware to enter WiFi OTA maintenance mode on boot.
- The web updater page now reports board, fixture ID, and firmware version.

Created a local ignored `wifi_secrets.h` for Ben's home WiFi and USB-flashed `smoke-2026-05-15.3` to all three boards as the WiFi-enabled OTA baseline. All three connected to the home WiFi and started web OTA:

- C6 + IS31FL3741: `192.168.4.248`
- FeatherS2 Neo: `192.168.4.249`
- M5Stack Atom Matrix: `192.168.4.250`

Then built `smoke-2026-05-15.4` and uploaded the app binaries over HTTP OTA to all three boards:

- `curl -F firmware=@/tmp/res-c6-ota/smoke_test.ino.bin http://192.168.4.248/update`
- `curl -F firmware=@/tmp/res-feathers2neo-ota/smoke_test.ino.bin http://192.168.4.249/update`
- `curl -F firmware=@/tmp/res-atom-ota/smoke_test.ino.bin http://192.168.4.250/update`

All three returned `Update complete. Rebooting.` and reconnected, serving `Version: smoke-2026-05-15.4` from their OTA web pages.

Open follow-up: `RES_WIFI_AUTO_CONNECT` is convenient for bench testing but should stay off in committed examples and production-like firmware. Production should enter OTA only in explicit maintenance mode.

## 2026-05-15 — Ben + Codex — COTS smoke firmware built, flashed, and serial-verified

Added `firmware/smoke_test/`, an Arduino CLI smoke-test sketch for the first three COTS prototypes. It builds for:

- `esp32:esp32:adafruit_feather_esp32c6:CDCOnBoot=cdc,PartitionScheme=min_spiffs`
- `esp32:esp32:um_feathers2neo:PartitionScheme=min_spiffs`
- `esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs`

The sketch prints a serial boot report, MAC-derived fixture ID, reset reason, heap, OTA partition labels, board pin summary, I2C scan results, and a conservative LED test. It also includes a serial-command-triggered temporary AP web updater (`o` command) for future OTA smoke testing without hard-coded WiFi credentials.

Installed Arduino libraries needed for the smoke pass: Adafruit IS31FL3741 Library 1.2.3, Adafruit BusIO 1.17.4, Adafruit GFX Library 1.12.6. Existing Adafruit NeoPixel 1.15.4 is used for the built-in 5x5 matrices.

All three boards were flashed and serial-verified:

- Adafruit Feather ESP32-C6 + IS31FL3741: firmware `smoke-2026-05-15.2`, MAC `58:E6:C5:E4:1B:2C`, fixture ID `E41B2C`, I2C devices `0x30` (IS31FL3741) and `0x36` (likely onboard battery monitor), IS31 initialized, OTA partition `app0`.
- FeatherS2 Neo: firmware `smoke-2026-05-15.2`, MAC `48:27:E2:57:0D:32`, fixture ID `570D32`, built-in 25-pixel matrix on GPIO21, no I2C devices found, OTA partition `app0`.
- M5Stack Atom Matrix: firmware `smoke-2026-05-15.2`, MAC `F8:B3:B7:1B:51:08`, fixture ID `1B5108`, built-in 25-pixel matrix on GPIO27, no I2C devices found, OTA partition `app0`.

Notes:

- Arduino builds should not be run in parallel against the same sketch/cache; mixed RISC-V/Xtensa objects corrupted the Arduino cache. Sequential builds with explicit `--build-path` work.
- The smoke LED test intentionally limits both total lit pixels and PWM/global brightness. This matches the gobo/patterned-aperture direction and avoids M5Stack Atom Matrix full-brightness stress.
- End-to-end OTA upload through the temporary AP is implemented but not yet tested from a browser/client.

## 2026-05-15 — Ben + Codex — First COTS prototype USB inventory and interim C6 matrix path

Three COTS prototype boards arrived and were connected over USB for first bench bring-up:

- Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 RGB LED matrix over STEMMA-QT. This is an interim substitute for the delayed PowerFeather matrix stack, useful for IS31FL3741 I2C, LED-current, OTA, and gobo/optics testing, but not a substitute for PowerFeather `VSQT`, LiFePO4 charging, fuel-gauge, sleep-current, or solar telemetry validation.
- M5Stack Atom Matrix with built-in 5x5 LEDs, USB-powered for now.
- UnexpectedMaker FeatherS2 Neo with built-in 5x5 LEDs, USB-powered for now.

USB/serial inventory on Ben's Linux bench:

- `/dev/ttyACM0` — UnexpectedMaker FeatherS2 Neo, USB VID:PID `303a:80b5`, serial `84722E75D023`, Arduino FQBN `esp32:esp32:um_feathers2neo`.
- `/dev/ttyACM1` — Adafruit Feather ESP32-C6 via Espressif USB JTAG/serial, USB VID:PID `303a:1001`, serial `58:E6:C5:E4:1B:2C`, Arduino FQBN `esp32:esp32:adafruit_feather_esp32c6`.
- `/dev/ttyUSB0` — M5Stack Atom Matrix via FT232, USB VID:PID `0403:6001`, serial `8D529F3938`, Arduino FQBN `esp32:esp32:m5stack_atom`.

Local tool state: Arduino CLI is installed with `esp32:esp32` core 3.3.7. No repo firmware exists yet beyond architecture docs. No firmware was flashed during this inventory pass.

Immediate test direction: create a small USB smoke/OTA bring-up firmware before broader firmware architecture work. It should print board ID, MAC-derived fixture ID, reset reason, build version, LED driver status, I2C scan results where applicable, and OTA status. Use LiPo-only DFRobot DFR0559 tests for now and do not connect LiFePO4 to LiPo-only boards.

## 2026-05-06 — Ben + Claude (Cowork) — Pre-share cleanup pass

Final cleanup before pushing the repo to GitHub and sharing with Steve and the wider team:

- **Bamboo "cone" → "lantern" / "cylinder".** The bamboo piece is geometrically a cylinder with a steam-bent flared skirt at the bottom, not a cone. The only cone-shaped object in the project is the experimental projective-geometry filter / gobo. Scrubbed every "bamboo cone" reference across BACKGROUND, ROADMAP, README, AGENTS, glossary, ADR 0007, hardware/references, ops/bom, enclosure README. Gobo "cone" references preserved.
- **Agent-neutral voice.** Rewrote BACKGROUND.md from a Ben-addressed narrative into a third-person project-context document. Replaced "Ben (you)" with "Ben Eckart" throughout. Replaced "Dad" with "Steve Eckart" outside this LOG file.
- **Scrubbed historical / distracting context** from active docs. Removed "Critical dates" stale-deadline table from BACKGROUND. Removed crossed-out resolved items from TODO and ROADMAP. The narrative of "we initially thought X, then learned Y" now lives only in this LOG; active docs present the current state cleanly.
- **New ADR 0009 — Minimize per-fixture operations at scale (O(1), not O(N)).** Captured Ben's strong constraint that anything done per-fixture is multiplied by 100. Specifies: no soldering on receipt; same firmware for every fixture; per-unit identity from MAC; investigate JLCPCB pre-flash service; design pogo-pin flashing jig as fallback. Reinforced in `README.md`, `hardware/README.md`, `TODO.md`. This is now the ninth and (so far) final ADR.

After this pass, the active docs (`README`, `AGENTS`, `BACKGROUND`, `TODO`, `ROADMAP`, `SYSTEM`, ADRs, glossary) read as a clean shared documentation set for Ben + Steve + future AI agents + the wider Resonance team. The journey from "what is this project" through "let's design solar lights" to "modular hat with LiFePO4 carrier board with O(1) ops" lives in this LOG.

---

## 2026-05-06 — Ben + Claude (Cowork) — Logistics flow confirmed: air-ship to TN, integrate at Grass Valley

Big risk-register item resolved: **Bamboo Pure is air-shipping a small batch of prototype bamboo lanterns to Steve in Tennessee.** Electronics workstream is fully decoupled from the May 10 Bali sea container. The end-to-end logistics flow:

1. Bali → TN: prototype lanterns by air for early mechanical prototyping (Phase 2).
2. Bali → Grass Valley, CA: tree structure + remaining bamboo by sea container.
3. Ben (CA): designs PCB, ships to Steve.
4. Steve (TN): finalizes hat enclosure with both bamboo and PCB in hand.
5. Steve → Ben (TN → CA): ships 100 hats.
6. Ben → Grass Valley: drives hats + electronics to meet the bamboo container at the staging area.
7. Grass Valley: final integration. Truck to BRC.

**Updated docs:**

- `docs/ROADMAP.md` — Phase 2 dependencies, Phase 6 rewritten as cross-country logistics + Grass Valley integration, risk register marked resolved, open dependencies list updated.
- `TODO.md` — removed urgency on "catch Elliot before Bali," removed ship-path decision (resolved), added air-ship-timing confirmation.

**What this changes practically:**

- Phase 2 (mechanical prototyping) can start as soon as bamboo arrives in TN, not when Elliot returns from Bali.
- Phase 5 production fab no longer races a container deadline.
- Phase 6 is a cross-country logistics piece with TN → CA → Grass Valley flow rather than US → BRC direct.
- Grass Valley pre-build staging area is now the canonical "integration site" terminology.

---

## 2026-05-06 — Ben + Claude (Cowork) — Roadmap, power-budget correction, prototyping strategy

Three additions:

**`docs/ROADMAP.md`** — phases 0–10, working backward from BM 2026 (late August). Phase 1 (TTGO bench prototype) starts 2026-05-07 and runs ~3 weeks. Phase 3 (custom carrier board v1) lands ~2026-07-01. Phase 5 (production fab) ~2026-08-01. Risk register and open dependencies on team included.

**Prototyping strategy clarification.** The "validate the architecture before committing to LiFePO4 silicon" risk is fully mitigated by Phase 1 — using the **TTGO T-Beam (with its built-in TP4056 LiPo charger)** as the LiPo prototype platform. No intermediate "LiPo carrier board" needed — that would add a board spin without de-risking anything Phase 1 doesn't already cover. The CN3058 LiFePO4 charger circuit is the only chemistry-specific portion; we lift its reference circuit from datasheet, AI-review, and validate on Phase 3 v1 board with MCP73123 as designed-in fallback. (Captured in `docs/ROADMAP.md`, not yet a separate ADR — promote to ADR if revisited.)

**Power budget correction.** Earlier estimate assumed "4 WS2812B all on at once" yielding ~10 mA LED average. Actual usage model is **1–9 LEDs per fixture, typically 1–3 lit at a time** (default ambient = 1 LED at 10%, showy = 3 LEDs at 30%, wand-burst = 9 LEDs full but rare and brief). Per-LED current scales linearly per WS2812B datasheet — confirmed against 2018 Talisman v2 measurements on the 16-LED ring (500 mA / 16 = 31 mA per LED at full white, matching). Updated `docs/block-diagram/SYSTEM.md`:

- Per-LED reference table replaces "4-LED ring" table.
- Time-weighted nightly LED current ~5 mA (vs. 10 mA estimated earlier).
- Total daily drain ~120 mAh (vs. 170 mAh).
- Panel sizing recommendation now 1–2 W (vs. 2 W); 1 W is sufficient.
- Battery: 18650 still preferred for 12-night autonomy and 2-year life; 14430 (~3 nights) now reasonable if cell sourcing forces it.
- BOM updated for 1–9 LED count per fixture.

---

## 2026-05-06 — Ben + Claude (Cowork) — Handoff documents

Before switching to Claude Code for daily iteration, dumped context to handoff-friendly artifacts so future agents (Ben's Claude Code, Steve's Claude Code, Elliot's Co-Work, future Cowork sessions) can pick up cold:

- `AGENTS.md` at root — explicit preamble for any agent picking up this repo. Read order, who's working, what's known vs assumed, what the repo does NOT cover, when to ask Ben.
- `docs/block-diagram/SYSTEM.md` — the canonical system architecture. ASCII block diagram, voltage rails, current draw table grounded in 2018 Talisman v2 measurements + ESP32-C3 datasheet, single-fixture daily power budget (~170 mAh/night, well covered by 2 W panel + 1500 mAh 18650), back-of-envelope max-stress check for wand-interaction events. Cost-comparison sketch vs `INV_2026_00401`.
- `docs/decisions/` — eight ADRs: ESP32-C3-MINI-1 (0001), LiFePO4 chemistry (0002), CN3058 charger (0003), ESP-NOW mesh (0004), FreeRTOS task architecture (0005), custom PCB not dev-board-on-carrier (0006), modular hat enclosure (0007), WS2812B from Vbat with no level shifter (0008).
- `firmware/ARCHITECTURE.md` — RTOS task decomposition (`led_render_task`, `ca_tick_task`, `mesh_tx_task`, `mesh_rx callback`, `housekeeping_task`), inter-task communication via FreeRTOS queues + atomic shared state, sleep behavior, boot sequence, OTA strategy.
- `hardware/atopile/EXAMPLE.md` — sample atopile module (`voltage_regulator.ato` for the AP2112K-3.3 LDO) so the schematic-as-code pattern is concrete. List of modules to build.
- `ops/bom.md` — first-pass BOM grouped by carrier-board electronics, non-PCB electronics, and mechanical. Per-fixture target ~$23. 100-fixture total ~$2,310.
- `docs/glossary.md` — proper nouns and acronyms for new agents dropping in cold.

These files are now the canonical project context outside this conversation. The earlier `BACKGROUND.md` remains the long-form narrative.

Switching to Claude Code from here. Cowork retains read access to this repo via GitHub (when pushed) for review and project management.

---

## 2026-05-06 — Ben + Claude (Cowork) — Repo bootstrap

Stood up this repo. Ported `BACKGROUND.md` from earlier Cowork session — captures full project context, team, decisions to date, prior-art lessons from 2018 Talisman v2 build, code reusable from `beneckart/future-robotics`, and the design space for this year (electronics architecture, mandala filter program, mesh creative possibilities).

Decisions baked in so far (subject to team review):

- **MCU:** ESP32-C3-MINI-1 for production. Prototype on TTGO T-Beam and T-Ice modules already in Steve's workshop.
- **Battery chemistry:** LiFePO4. Chosen for thermal tolerance in desert deployment.
- **Charger IC:** CN3058 (LiFePO4-tuned, JLCPCB basic part, ~$0.30). Rejected TP4056, bq24074, CN3791 — all LiPo-tuned, wrong charge profile.
- **3.3 V LDO:** AP2112K-3.3 (450 mV dropout, JLCPCB basic part, fits LiFePO4's 2.5–3.6 V range).
- **LEDs:** 1–4 WS2812B per fixture, powered direct from battery rail (3.3 V GPIO satisfies WS2812B's 0.7 × Vcc threshold per Talisman v2 verification).
- **Mesh:** ESP-NOW. No infrastructure required at BRC.
- **OTA:** required from day one. One USB-C flash per device, then over-the-air forever.
- **Enclosure:** sealed 3D-printed solar "hat" that sits partially inside / partially over the bamboo cone top. Set screws absorb bamboo dimensional variability.

Open team-side questions (see `BACKGROUND.md` and `TODO.md` for full list):

- Rope attachment point: hat, bamboo, or hybrid. Pending Vishnu / Ed / Elliot.
- Container vs separate ship for electronics. Bamboo ships from Bali 2026-05-10.
- Hat dimensions confirmation to Vishnu so he can finalize renders.
- INV_2026_00401 cost decomposition.

Next concrete steps for Ben + Steve:

1. System block diagram + power budget (highest-leverage upstream artifact).
2. atopile module library: `solar_input`, `lifepo4_charger`, `power_path`, `voltage_regulator`, `esp32_module`, `led_output`. Build each from reference schematics.
3. Bench validation on existing TTGO modules — solar charging path first.

Switching to Claude Code for daily firmware/hardware iteration. Cowork retains read access to this repo via GitHub for project management and review.
