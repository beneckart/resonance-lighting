# LOG

Append-only session journal for the Resonance Lighting workstream. Most recent first.

Format per entry:

```
## YYYY-MM-DD -- author -- short subject

Body. What changed, what was decided, what's next.
```

---

## 2026-07-02 - Ben + Claude - Bench report written: docs/tests/BOOST_AB_BENCH_REPORT_2026-07-02.html

Full narrative report of today's boost A/B campaign, written for future-us and for
Steve (plain language, no session shorthand). Every claim carries an evidence grade
(REPLICATED / MEASURED ONCE / STRONG EVIDENCE / HYPOTHESIS / OPEN) -- the session's
own corrected-mid-stream claims (the "flaky STEMMA cable", r4's "+27% from the
contact fix") are used as the worked examples of why the grading matters. Includes
five figures (HEX A/B bars, RGBW brightness ladders, the full-power topology matrix,
efficacy with measurement-plane caveats, and the raw partial-brightness
current-instability trace), regenerable via ops/bench/report_figs_boost_ab.py.
One in-session claim is explicitly downgraded in the report: "W-die 3x efficacy vs
RGB-white" was computed from unstable current readings; the defensible number is
~1.4x (bare, full brightness), and boosted RGB-white efficacy was never cleanly
measured. Open-questions table mirrors the TODO items.

## 2026-07-02 - Ben + Claude - r9 completes the matrix: boosted-VBAT-fat hits 3044 lux with NO wall; both predictions land

Final cell: TPS63802 4.2 V boost fed straight from VBAT on the larger-gauge wiring
(no INAs; lux + gauge bv). Both ladders linear end to end, no aborts:

  wonly:    129 / 259 / 513 / 766 / 1016   (prediction was ~1040-1060: hit, -3 %)
  rgbwhite: 396 / 785 / 1554 / 2305 / 3044 (prediction ~2900-3000: hit)
  Cell sag at rgbwhite-255 (~1.3-1.4 A draw): bv 3.299 -> 3.203, ~100 mV. Comfortable.

THE COMPLETED MATRIX (usual aim, bri=255, gamma 0, ~SOC 63-75 LFP):

  config                     W-only (clean white)   RGB-white (fringed)
  bare, rail-fed                    470                1310  (no wall)
  bare, VBAT + fat wire             448                1746  (no wall)
  boosted, rail-fed                1044                wall at bri=128 (rail limit)
  boosted, VBAT + thin harness    ~1060 aim-corr       wall at bri=128 (harness R)
  boosted, VBAT + fat wire         1016                3044  (NO WALL)

Campaign conclusions (RGBW 4 W point source):
- **The "wall" was never the architecture.** Rail regulator first, instrumented-
  harness resistance second; with VBAT + proper wire the module delivers its full
  ~4 W: 3044 lux, 1.74x the bare-VBAT rgbwhite and ~2.3x anything rail-fed.
- **Boost value, final form (VBAT-fed, good wiring): clean white 448 -> 1016 lux
  (2.3x); max fringed white 1746 -> 3044 (1.7x).** Efficacy tax ~25-30 % (battery
  plane, from the r7 accounting) -- boost converts efficiency into output ceiling,
  consistently, in every topology tested today.
- **Production topology, if the RGBW ships with or without boost: LED power from
  VBAT (downstream of the gauge shunt!), fat conductors, ESP rail untouched.** The
  rail-fed path gives up 33 % of bare rgbwhite and walls any boost; VBAT-direct is
  simpler AND better. EN->GPIO for the kill; connector quality is worth 25 % of
  top-end light (the day's thrice-learned lesson).
- Bare remains Ben's production GO (bare-VBAT rgbwhite 1746 lux is plenty per the
  eye test); the boost option file is complete and shelved with real numbers at
  every operating point it could be revived for.

## 2026-07-02 - Ben + Claude - Audit: gamma bug invalidates NOTHING (verified per-file); r8 blindness re-attributed to an I2C bus wedge + board reboot

Ben challenged two claims in the r8 entry; both corrections below are evidence-based.

**Gamma audit (Ben's worry: "huge repercussions -- how much does this invalidate?"):
answer NOTHING, verified against every file, not from memory.** All 31 capture files
log /state rows; scanning every row: gamma=0 in ALL runs through r7 and in r8c;
gamma=1 ONLY in r8 (optically blind anyway; its salvaged claim -- full rgbwhite at
~40 mV cell sag -- rests on the bri=255 step, where gamma8(255)=255 is identity) and
r8b (flagged at capture; its bri=255 points match r8c within 0.5 %). Every HEX suite,
every RGBW ramp r1-r7, and every verdict built on them: gamma=0 throughout, zero
impact. Render-path check: setRGBWpix applies gamma8 AFTER brightness scaling, so
gamma distorts sub-255 bri steps as ~bri^2.2 and is exactly identity at bri=255 --
matching the observed r8b curve.

**Gamma mechanism correction: not "left on from eye-testing" -- it is the BOOT
DEFAULT.** `gGamma = true` in led_studio; the PowerFeather rebooted during the
rewiring window (fingerprint in the state rows: r7 shows the all-day session state
lit=12/speed=38, r8 shows boot defaults lit=18/speed=30 -- battery feed interrupted
while working at the VBAT header). The ramp's mode-set never touched gamma, so the
default survived into r8/r8b. Ramp tool already pins gamma=0 now; the boot default
itself is a bench trap for any future /set-driven capture that assumes session state.

**r8 blindness correction: Ben is right that the STEMMA cable was likely never
flaky -- but it was not a port jump either.** Evidence: the post-r8 probe of
/dev/ttyACM2 returned live ina_monitor output with a fresh 5-hour-uptime timestamp,
so the ramp HAD been reading the correct device (the port jump to ttyACM1 happened
later, at the reseat, when the KB2040 re-enumerated and its uptime reset to ~3.5
min). During r8 the monitor was emitting almost nothing (~1 line per 2 s where ~20
expected) with 0x41 stuck present-but-ERR and the still-attached VEML undetected.
Best-fit mechanism: **wedged I2C bus** -- the INA harness was unplugged mid-session
from an actively polling monitor (classic SDA-held-low), stalling every transaction
into timeouts: slow loop, unreachable VEML, ERR spam all explained. The fix was the
KB2040 REBOOT during the reseat handling (Wire re-init cleared the wedge); the cable
reseat itself was probably incidental. Hedge: a marginal contact cannot be fully
excluded, but the sparse-output signature favors the wedge.

Hardening TODO queued: ina_monitor should clear a channel's present flag after N
consecutive ERRs and attempt I2C bus recovery (9 SCL pulses + Wire re-init) when the
whole bus errors, so a mid-session unplug cannot blind the monitor until a reboot.

## 2026-07-02 - Ben + Claude - r8 bare-VBAT fat-wire: the wall was bench wiring, and VBAT-direct beats the rail by +33% on RGB-white

Production-similar test per Ben: RGBW V+ direct from the VBAT header pin, larger-gauge
JST-XH, GND via the split cable, NO boost, NO INA instrumentation (the dupont-wired
INA harness was the suspect). Instrumentation = VEML lux + gauge bv only; the ramp
tool gained a gauge-bv abort floor for uninstrumented runs. Protocol notes: the first
run (r8) was optically blind -- the STEMMA cable to the VEML had loosened during
rewiring (reseat fixed it; KB2040 re-enumerated to ttyACM1); r8b then produced a
superlinear ladder because the UI's GAMMA toggle had been left on from eye-testing
(identity at bri=255, so full-brightness points remain valid; ramp tool now pins
gamma=0). r8c is the clean run. r8b/r8c bri=255 agreement: 0.5 %.

r8c (bare, VBAT-direct, fat wire, gamma 0, "usual" aim per the W anchor):
  wonly lux:    59 / 116 / 226 / 338 / 448   (linear; ==rail-fed 470 within mount noise)
  rgbwhite lux: 225 / 444 / 882 / 1317 / 1746 (linear; NO WALL, all steps completed;
                cell terminal sag ~40 mV at full per the r8 gauge rows)

Findings:
- **The rgbwhite collapse was the bench wiring.** On fat wire from VBAT, full
  RGB-white runs clean -- no abort, no sag worth naming. The r7 "wall" was ~0.3 ohm
  of instrumented-harness loop resistance, confirmed by its absence here.
- **VBAT-direct beats the rail path by +33 % on RGB-white** (1746 vs 1310 lux at the
  same aim): under load the 3V3 rail delivered ~2.97-3.1 V at the die while VBAT +
  fat wire holds ~3.25-3.3 V -- the starved green/blue dies convert every extra
  100 mV into light. W-only is unchanged (448 vs 470: the W die is equally starved
  either way). 1746 lux is the BRIGHTEST white of every configuration tested today,
  boosted ones included -- fringed/warm, but free.
- This quietly revalidates the old ADR 0008 topology (LED direct from VBAT) for the
  RGBW: simpler, brighter at full, and it inherits r7's proven ESP decoupling.
- **Production caveat (important): tapping VBAT at the header BYPASSES the fuel
  gauge's current shunt** (r7/r8 finding: gauge ma blind to the whole LED branch).
  A production VBAT-fed LED rail must tap downstream of the gauge sense resistor
  (trivial on a custom PCBA; needs schematic check on the PowerFeather COTS path)
  or SOC/coulomb telemetry undercounts the dominant load.

Matrix now (usual aim, bri=255): bare-rail W 470 / rgbw 1310; bare-VBAT W 448 /
rgbw 1746 no-wall; boosted-rail W 1044 / rgbw walls at 128; boosted-VBAT-thin W
~1060 aim-corr / walls at 128 (harness). Remaining cell: boosted-VBAT on fat wire
(r9) when Ben re-adds the boost -- prediction: W ~1040-1060, rgbwhite runs past the
old wall and lands ~2900-3000 if the ladder stays linear (~10.2 lux/bri at usual aim
x3.98), cell sag permitting.

## 2026-07-02 - Ben + Claude - r7 VBAT-fed boost: ~11% battery-side saving, ESP fully decoupled, wall becomes a wiring problem

Ben rewired the boost input to VBAT (VBAT header pin + GND borrowed via 2-pin JST-XH
split from the free VDC/solar port; 0x41 INA moved into the VBAT->boost branch).
Board topology surprise: the VBAT tap bypasses the 0x45 shunt, so 0x45 now reads the
BOARD's own draw only -- a dead-constant 116-118 mA at every step (cleanest ESP+radio
overhead number of the day); total system = 0x41 + 0x45.

r7 (VBAT-fed), W ladder lux: 169/335/665/992/1347; W-full stable: 225 mA @ 3.212 V =
0.723 W -> 1346.5 lux. rgbwhite: 380 @32, 759 @64; HARD ABORT at 128 (branch node
2.588 V). Board never blinked.

Predictions graded:
- **Aim moved again**: whole W ladder is a UNIFORM 1.26-1.29x vs r6 (clean geometry
  factor this time, unlike r4's kink) -- the rewiring session re-seated the module at
  the "favorable" aim, same ~+27 % magnitude as the r4 outlier; the rig plausibly has
  two quasi-stable seatings. Aim-corrected die output == r6 (~1060 vs 1044 lux):
  same 4.2 V at the die either way, as physics requires.
- **Efficiency: PASS after fixing my reference-plane sloppiness.** The 0.62-0.65 W
  prediction wrongly treated r6's 0.731 W (measured on the 3V3 RAIL, already
  once-converted, ~0.81 W at the battery) as battery-plane. Honest battery-plane
  comparison at matched die output: two-stage ~0.81 W -> single-stage 0.723 W =
  **~11 % saving**, consistent with deleting a ~90 %-efficient stage. Aim-corrected
  efficacy tax vs bare (also converted to battery plane, ~0.23 W): **~37 % -> ~28 %**.
- **"No rgbwhite wall": FAIL on this harness, but the mechanism changed.** The wall
  is no longer the 3V3 rail regulator -- it is ~0.3 ohm of harness loop resistance
  (measured: 72 mV sag at 225 mA on the W-full step; loop = VBAT pin -> dupont ->
  module -> LED -> borrowed-GND JST-XH split -> VDC port) plus cell/protection sag,
  collapsing the branch node at ~1 A demand. On a production VBAT feed (PCB traces,
  proper connectors) this wall is a wiring spec, not an architecture limit.
- **ESP decoupling: PROVEN.** Board draw stayed at 116-118 mA through every step
  INCLUDING the branch collapse; /state clean after, no reset. In this topology LED
  transients structurally cannot brown out the controller -- the radio-burst-during-
  LED-load concern is dead where it matters.

Boost option file (still shelved -- bare remains the GO): if the field test at height
ever demands the 2.2x clean white, the production shape is VBAT-fed single conversion
on the adapter PCB: ~28 % efficacy tax, ESP immune to LED transients, EN->GPIO +
pull-down for the software kill, and connector/trace quality worth ~25 % of top-end
light (today's recurring lesson, three different ways).

## 2026-07-02 - Ben + Claude - r6 GOLD STANDARD: boost verdict settles at 2.2x clean white / ~37% efficacy tax; r4's +27% was aim, not electronics

Root cause found by Ben while simplifying the boost wiring: **two blown-out female
duponts**. The RGBW's 3-pin JST cable-to-cable pins are oversized for female duponts
-- forcing them in splays the socket, and it then makes a poor friction fit on normal
header pins (the boost PCB). That is the physical mechanism behind the flaky boost
path. Rewired simply with fresh duponts, all snug; r6 run as the gold standard.

r6 boosted, default ladder:
  wonly lux: 135 / 265 / 525 / 786 / 1044   (r1: 133/263/521/777/1033 -- MATCHES r1)
  wonly @255 stable: 229 mA / 0.731 W -> 1044 lux = 1428 lux/W
  rgbwhite: 323 @32, 641 @64; HARD ABORT at 128 (2.516 V) -- wall replicated 3rd time

Interpretation (corrects the r4 entry):
- **Electrical fix confirmed and quantified**: r6 matches r4's current draw (229 vs
  227 mA at W-full) vs r1's 243 -- the bad contact wasted ~6 % input power. That is
  the WHOLE electrical story.
- **r4's +27 % light was an aim outlier**, not the contact fix: r6 has r4's
  electrical numbers with r1's optical numbers. The r4 entry's "2.7x / 20 % tax"
  claim is RETRACTED; the kinked r4 ladder (low-bri matching r1, high-bri +27 %)
  remains unexplained -- fixed geometry cannot be bri-dependent; a thermal-mechanical
  tilt of that particular mount under high drive is the surviving speculation. Logged
  as a mystery, not a finding.
- Mount-to-mount aim statistics across the day: r1/r2/r5/r6 all land within ~1-3 %
  of each other (the taped outline works); r4 was a single +26 % outlier; RGB-die aim
  separately moved -11 % once (r5). Absolute lux carries this seating uncertainty;
  within-mount ratios do not.

**GOLD STANDARD RGBW boost verdict** (r6 boosted vs r5 bare, usual seating):
  bare W-full 470 lux @ 0.208 W (2260 lux/W); boosted W-full 1044 lux @ 0.731 W
  (1428 lux/W) -> **boost = 2.2x the clean white at ~37 % efficacy tax**; boosted
  rgbwhite rail-walls at bri=128 every time; bare rgbwhite-full ~1310 lux is the
  free bright-white option (color fringe/tint tradeoff). The original r1-era numbers
  were right all along -- the day's detours bought their confirmation plus the
  dupont root cause. Bench rule going forward: NEVER mate JST cable-to-cable pins
  into female duponts; use proper JST pigtails or crimp housings.

## 2026-07-02 - Ben + Claude - Repeatability due diligence (bare r5): electrical perfect, W-die aim perfect, RGB dies -11% at identical current

Ben (rightly) disliked that a reseat moved numbers 27 %, so: bare remount, exact
ladder, compare to bare r2. Result splits into three clean findings:

1. **Bare electrical path: perfectly repeatable.** Stable-step currents identical
   across the remount (W-full 64 vs 64 mA, rgbwhite-full 263 vs 264 mA; power within
   1 %). The bare config has no reseat sensitivity.
2. **W-die optics: perfectly repeatable.** Whole W lux ladder identical to 0.1 %
   (470.5 vs 470.0 at full). The hot-swap procedure holds the W die's aim through the
   tube essentially exactly. This retroactively CLEANS UP the r1-vs-r4 boost analysis:
   W-die geometry is proven stable across mounts, so r4's kinked ladder (+5 % low,
   +27 % high, current DOWN 7 %) is pure electrical -- the lossy-contact story
   survives due diligence, and the boost-path connection is confirmed as the sole
   large variance source in the whole rig.
3. **RGB dies: -11 % lux at IDENTICAL current, uniform across the ladder** (0.89x at
   every step, r5 vs r2). Same drive, same watts, less light, W unmoved. Two candidate
   explanations, unresolved: (a) per-die aim shift -- the 4 dies sit mm apart in the
   package, and a small module rotation about the W-die axis changes the RGB dies'
   throw through the tube (the same die-offset physics as Ben's color-fringing
   observation); (b) RGB die degradation from the collapse/abort events (r1/r3/r4
   pushed high current through the RGB dies; uniform -11 % across all three from
   brief events seems less likely, but not excluded). DISCRIMINATING TEST when
   curious: nudge/rotate the module slightly and re-check rgbwhite lux at one step --
   recovery = aim; no recovery = degradation. (No RGBW per-channel singles baseline
   exists from the r2 era -- the ramps only ran wonly + rgbwhite -- so the singles
   looks in rgbw_boost_ramp.py can only characterize the current state, not compare
   backward. If degradation is suspected, capture singles NOW as the go-forward
   baseline.)

Current-seating headline numbers (r4 boosted + r5 bare, adjacent mounts):
bare W-full 470 lux @ 0.21 W; boosted W-full 1315 lux @ 0.73 W (2.8x, ~20 % efficacy
tax); bare rgbwhite-full 1310 lux @ 0.84 W (was 1475 at r2 aim -- absolute rgbwhite
lux carries the per-die aim factor, ratios within a mount do not).

## 2026-07-02 - Ben + Claude - r4 after reseat: r1 was the sick mount; healthy boost = 1315 lux clean white at only ~20% efficacy tax

Ben questioned whether the INA had settled; the within-step drift analysis that
followed found something better: partial-brightness current medians are UNRELIABLE in
ALL runs (bare included -- so not the boost module), stable only at bri=255. Mechanism:
the 3V3 rail regulator (TPS631013) burst-modes at light-mid loads and the INA219's
68 ms averaging window aliases the bursts into slow apparent wander; at full drive the
regulator runs continuous PWM and readings are rock-stable (sd ~2 mA). Lux (VEML,
100 ms integration) stays clean throughout -- trust lux everywhere, trust mA only on
stable steps. rgbw_boost_ramp.py now flags unstable steps automatically.

r4 = exact r1 replication after Ben reseated the boost connections:

  wonly lux ladder:  139 / 275 / 544 / 982 / 1315   (r1: 133 / 263 / 521 / 777 / 1033)
  wonly @255 (stable): 227 mA / 0.726 W in -> 1315 lux = 1811 lux/W
  rgbwhite: 359 @32; 716 @64 (branch 2.846 V, at the soft-floor edge);
            HARD ABORT at 128 (2.52 V) -- r1's wall REPLICATED (r3's cliff-at-72
            does not reproduce; that was the bad contact).

Decomposition: low-bri steps match r1 within 4-5 % (at light drive even a lossy
contact reaches die regulation -> bounds the optical/geometry shift at ~+5 %); high-bri
steps are +26-27 % with 7 % LESS input power. Conclusion: **r1's boost path had
contact/series resistance from the start** -- it starved the die at high drive and
burned input power in the connection. The reseat fixed it. All r1/r3 boosted absolute
numbers were depressed at high drive; bare runs unaffected (no boost path).

Updated headline (healthy mount, geometry-matched approximately):
  bare W-full ~470-490 lux @ 0.21 W (~2250-2350 lux/W)
  boosted W-full 1315 lux @ 0.73 W (~1810 lux/W)
  -> the boost buys ~2.7x the clean white at only ~20 % efficacy tax (was stated as
  2.2x / 40 % tax off the sick mount). The RGBW boost case is STRONGER than reported.
Caveats: geometry shifted ~+5 % across the re-matings, so cross-era absolute lux
carries that error; a fresh bare run at current geometry would clean up the pair.
Reliability lesson for production: the boost path connection quality is worth ~25 %
of top-end light -- connectorization/solder, not hand-wired jumpers.

## 2026-07-02 - Ben + Claude - Boost re-mount r3: electrical operating point shifted; rgbwhite cliff moved; curiosity answered

Boost re-mounted after the bare r2 run (Ben flagged a possible LED bump). The r3
alignment ladder says the change is NOT (mainly) optical seating -- the boosted
electrical operating point itself moved:

- W-only @ full: r1 243 mA / 0.776 W / 1033 lux vs r3 199 mA / 0.639 W / 968 lux.
  Current -18 % at the same commanded look; lux only -6 %; lux-per-mA UP 14 %
  (consistent with lower current density, not geometry). A pure bump cannot change
  die current.
- rgbwhite fine ladder (curiosity run): @32 120 mA / 318 lux and @64 227 mA / 633 lux
  -- HALF of r1's branch current at 64 (481 mA) for nearly the same lux -- then
  HARD ABORT at bri=72 (branch bus 2.58 V). The wall moved from somewhere in
  (64, 128] down to (64, 72], at half the current of r1's healthy 64-step.
  Nonmonotonic collapse at lower current = smells like boost-module input
  instability (IR-dip -> UVLO/foldback oscillation), not a simple rail ceiling.

Prime suspect: contact/series resistance in the re-mated boost path (input or output
connector), possibly a nudged wire/jumper; module damage from the r1 collapse event
not excluded. ACTION: reseat/meter the boost module connections (output should read
4.2 V unloaded), then re-run `--config boosted --runtag r4-align --looks wonly` and
compare to r1. Treat ALL r3 numbers as suspect for A/B purposes. Also a production
data point: a hand-wired boost path showed ~18 % current shift across one re-mating
-- connectorization/soldering quality matters if a boost ever ships.

Curiosity question (can boosted rgbwhite beat bare's 1475 lux before collapse?):
answered NO even for the healthy r1 mount by slope -- ~10.2 lux/bri projects ~1300
lux at bri=128, which is already past the wall. Bare rgbwhite wins on raw output
because the starved dies self-limit: full duty cycle, no conversion loss, no wall.
Boosted W-only (~1033 lux healthy) remains the brightest CLEAN white.

## 2026-07-02 - Ben + Claude - RGBW bare ramp r2: the boost EARNS ITS KEEP on the W die (+120% light)

Bare RGBW ladder, same harness position, same aborts (none triggered -- bare never
approaches the rail wall):

  look      bri  led_V  led_mA  led_W  batt_mA   lux
  wonly      32  3.280    11    0.036    131      60
  wonly      64  3.276    18    0.059    138     119
  wonly     128  3.268    27    0.088    153     236
  wonly     192  3.264    46    0.150    181     353
  wonly     255  3.260    64    0.209    186     470
  rgbwhite   32  3.264    39    0.127    161     185
  rgbwhite   64  3.244    70    0.227    198     369
  rgbwhite  128  3.232   127    0.410    264     737
  rgbwhite  192  3.200   192    0.614    330    1106
  rgbwhite  255  3.188   264    0.842    397    1475

Bare vs boosted, same looks (the opposite of the HEX story):
- **W-only @ full: bare 470 lux / 64 mA vs boosted 1033 lux / 243 mA = +120 % light.**
  The W die is severely current-starved at the ~3.26 V rail at EVERY brightness (it
  passes ~26 % of its boosted current; its Vf stack is the tallest on the module).
  Both ladders are linear in bri -- the starvation is a constant current ceiling, not
  compression at the top.
- rgbwhite @ 64: bare 369 vs boosted 654 lux (+77 %). Boosted rgbwhite is rail-limited
  to bri<=64-96; bare rgbwhite runs clean to FULL (264 mA, 3.19 V, no wall) because
  the starved green/blue dies cap their own draw.
- Bare rgbwhite @ full = 1475 lux = the brightest white measured through the tube --
  but presumably golden/warm (green/blue starved; VEML cannot see chromaticity).
  EYE CHECK WANTED: bare rgbwhite-255 color vs boosted wonly-255.
- Efficiency: bare W 2249 lux/W vs boosted W 1331 lux/W -- the boost still costs ~40 %
  lumens/W (same tax as HEX). The boost does not create efficiency; it buys OUTPUT
  CEILING: max clean white 470 lux bare -> 1033 boosted (2.2x).

Verdict shape for the RGBW (differs from HEX): criterion B is YES -- +120 % is
unmistakable to the eye; criterion C is still NO (~40 % efficacy tax). So the boost
decision reduces to an optics/artistic question: does the gobo role need more white
than the bare W die's ceiling? If bare W-full is bright enough at height, skip the
boost (best lumens/W on the fixture); if not, the boost is the only clean way to 2x
(bare rgbwhite is brighter still but color-compromised). Field test at projection
distance decides. Caveats: n=1 module/position; absolute lux is rig-specific;
chromaticity unmeasured.

## 2026-07-02 - Ben + Claude - RGBW boosted ramp r1: W-die is the efficiency star; RGB-white hits the rail wall

RGBW 4 W point source (led_studio mode=1, single SK6812 RGBW px on GPIO10) hot-swapped
into the tube harness, boosted (TPS63802 4.2 V fed from the 3V3 header). New
`ops/bench/rgbw_boost_ramp.py`: stepped brightness ladder per look with live
rail-droop aborts (hard floor 2.60 V on the LED branch, soft 2.80 V, /state
reachability check), one JSONL per run, LEDs blanked on any exit.

Boosted r1 (W-only then RGB-white, ladder 32/64/128/192/255):

  look      bri  led_V  led_mA  led_W  batt_mA   lux
  wonly      32  3.264    39    0.127    163     133
  wonly      64  3.248    67    0.218    178     263
  wonly     128  3.224   103    0.332    247     521
  wonly     192  3.208   194    0.622    302     777
  wonly     255  3.192   243    0.776    373    1033
  rgbwhite   32  3.176   174    0.553    346     339
  rgbwhite   64  3.104   481    1.493    684     654
  rgbwhite  128  HARD ABORT: branch bus hit 2.452 V mid-step; LEDs blanked; board
                 survived (no reset -- /state kept the script's values)

Findings (boosted config, n=1):
- **W-only is rail-safe to full**: 0.776 W input at bri=255, branch droop only
  3.26 -> 3.19 V. Lux tracks bri linearly (no compression) -- the die holds constant
  current across the ladder.
- **RGB-white hits the wall between bri 64 and 128**: 481 mA input at 64 was fine
  (3.10 V); the 128 step collapsed the branch to 2.45 V. Usable boosted RGB-white
  domain ~bri<=64-96. Rail ceiling between ~0.5 and ~1 A demand, consistent with
  Ben's ~1 A rating.
- **W-die efficacy crushes RGB-mixed white**: 1033 lux / 0.776 W = ~1330 lux/W vs
  654 / 1.49 = ~440 lux/W at rgbwhite-64. Phosphor white beats RGB mixing 3x for
  white gobo throw -- W-only should be the default white look on RGBW fixtures.
- Cross-module ballpark (same rig, different emitter geometry -- caveat): W-only
  full = 1033 lux vs HEX center-px white full = 216 lux, ~5x.

Next: swap to bare RGBW, same ladder (`--config bare --runtag r2`). The real boost
question this time is the W ladder: the W die's Vf stack may genuinely starve at the
bare ~3.2 V rail -- if bare W-only compresses/plateaus where boosted stayed linear,
the boost earns its keep on the RGBW; if not, same verdict as HEX.

## 2026-07-02 - Ben + Claude - June discharge data settles it: the pixel really saw ~2.97 V at show loads

Ben recalled the June harness metered BOTH nodes -- confirmed, the "conflation"
hypothesis from the previous entry is dead. `2026-06-10-discharge-1357.jsonl` logs
`batt_ina_bus_v` (0x45) AND `led_bus_v` (0x41, post-regulator at the LED), same
channel convention as today's boost harness. Binned medians at full-RGBW bri=255:

  batt_ina_v  led_bus_v  led_ma  batt_ma
     3.00       2.964      292     456
     2.95       2.968      292     459     <- most of the plateau lived here
     2.90       2.974      291     463
     2.85       2.859      291     468     <- LED V+ converges to VBAT: boost out of
     2.80       2.807      291     479        headroom at this load ("battery dying")

So the June "goldening at 2.8-2.95 V LED rail" was DIRECTLY MEASURED at the pixel,
provenance solid. The regulated rail delivers ~3.2 V at 42-122 mA (today's data) but
only ~2.97 V at ~290 mA show load (droop + harness IR, split not separable across the
two harnesses). Ben's slow-decline recollection is in the data too: led_ma holds ~291
constant while led_bus_v drifts down -- constant current, slowly starving voltage =
slow dimming, no cliff until VBAT < ~2.9 where LED V+ tracks the battery down.

Consistency check with today's boost verdict: intact and enriched. Single-px gobo
look (~42 mA) sits at ~3.2 V un-starved -> boost buys nothing (measured +1.6 %).
Show-class loads sit at ~2.97 V mildly starved -> boost buys a little (+6.9 % at
ring1) and would buy more at full show loads -- but the gobo role never goes there
(Ben). Ben also recalls evidence that a RADIO BURST while LEDs hog the ~1 A rail can
brown out -- consistent with the rail being the shared choke point; relevant to the
RGBW step-0 rail characterization, where the June curve already gives an anchor
(~2.97 V at ~290 mA).

Doc hygiene: "off the I2C bus" ambiguity fixed in the living docs (AGENTS.md,
POWERFEATHER_NOTES.md -- exact wording: data on a free GPIO, V+ from the regulated
3V3 header, NOT on the I2C bus) and a clarification APPENDED to ADR 0018 rather than
editing it -- the append-only rule holds, ambiguity fixed by addendum.

## 2026-07-02 - Ben + Claude - Correction: hex V+ is the regulated 3V3 rail, not VBAT; verdict rationale updated

Ben corrected a topology assumption threaded through the verdict entry below: the hex
V+ is fed from the PowerFeather's regulated 3.3 V buck-boost rail (switchable header),
NOT from VBAT. LFP at 3.1-3.2 V terminal just means the TPS631013 runs in boost mode.
What the pixel sees is the regulator setpoint minus harness/switch IR: bare runs read
~3.20 V at the branch INA at 42-122 mA (modest ~0.2 ohm apparent drop). The boosted
runs read lower (3.05-3.13 V) at the same point, but that node feeds the TPS63802's
switching input -- do not treat those as a clean impedance measurement.

What changes (the measured verdict does NOT -- it is photons vs watts):

- The "LFP plateau picks your operating point" framing was wrong in mechanism. The
  regulator picks the operating point; harness IR does the sagging. The pixel-level
  conclusion stands: ~3.2 V effective at the pixel is a mildly-undervolted sweet spot,
  blue's -5 % marks the knee edge.
- The low-SOC caveat WEAKENS: the bare config is already SOC-invariant by
  construction, because the rail regulates until deep discharge (bb_efficiency data
  showed the rail carrying much larger loads at VBAT 2.9-3.05). A low-SOC verdict
  flip is now unlikely; the residual check is rail droop under load at low VIN --
  still a 10-minute spot-check when a drained cell is around, but demoted.
- June's "goldening at 2.8-2.95 V LED rail" under show loads: PROVENANCE NOW UNCLEAR
  (correction within this session: the feed was never STEMMA -- always 3V3 + GND +
  GPIO; and per Ben, brightness back then was measured with the serial-USB PAR
  sensor). bb_efficiency notes say the LFP *terminal* sagged to ~2.9-3.05 V under
  show loads -- the June note may have conflated battery terminal with pixel V+. If
  the regulated rail actually held ~3.2-3.3 V, the goldening mechanism needs a
  re-look (rail droop near the ~1 A ceiling under ESP+LED show load is the leading
  candidate). Directly answerable now: with the bare hex mounted, ramp ring2/all at
  rising bri and log 0x41 bus_v vs branch current = the rail droop curve, ~5 min.
  Same measurement doubles as step 0 of the RGBW rail-capability check.
- The boosted config as tested was a double conversion (VBAT -> 3V3 boost -> 4.2 V
  boost); the battery-side numbers already include that tax, so the efficiency
  verdict is if anything generous to the boost.
- RGBW A/B design sharpens: today's HEX data never exceeded 0.21 A on the ~1 A rail
  (clean, uncontaminated by any limit), but 4 W white rail-direct wants ~1.2 A at
  3.3 V -- the BARE config hits the rail ceiling too. The RGBW experiment is really a
  topology question (rail-fed vs VBAT-fed boost) with a rail-capability
  characterization as step 0. TODO updated.

## 2026-07-02 - Ben + Claude - Boost A/B verdict (HEX, single-px gobo regime): boost NOT worth it at healthy SOC

Boosted r3 remount closed the loop: every r3 number matches r1 to <1 % across a
physical swap (white 217.0 vs 216.7 lux, ring1 596.8 vs 596.0). Combined bound on
seating error across the full bare/boosted/bare/boosted series: <=2 %, usually <1 %.

Consolidated results, center-anchor looks, LFP bench cell at SOC ~97 (rail 3.12-3.20 V
under load), VEML7700 at the tube exit (only ratios are portable):

  look              bare lux    boosted lux   delta     LED branch W (bare->boosted)
  white 1 px full   211.5-215.6 216.7-217.4   +1.6 %*   0.134 -> 0.216  (+60 %)
  red single        33.3        33.2-33.5     ~0        0.065 -> 0.113
  green single      128.6       129.4-129.5   +0.7 %*   0.065 -> 0.113
  blue single       60.4        63.4-63.6     +5.1 %    0.062 -> 0.113
  ring1 7px bri128  557.9       596.0-596.8   +6.9 %    0.388 -> 0.62-0.63
  (* within the +-2 % seating noise)

Verdict against Ben's three "worth it" criteria, for the HEX in its production role
(Ben's product call: >1 full-white px washes out the gobo, so single white px full --
or color-separated singles distributing the same load -- IS the operating point; the
heavy-load ladder is moot for HEX):
  A) install effort: NOT justified by B/C below.
  B) visible lumens/color gain: NO. White +1.6 % is inside the noise; blue +5 % is
     below brightness JND. No goldening on bare at this load -- the drivers are not
     meaningfully starved at plateau voltage with a single pixel.
  C) lumens/W: boost is ~40 % WORSE (white ~1594 -> ~1006 lux/W); the extra 60 %
     branch power becomes constant-current-driver heat, not photons.

Why this likely holds in production: LFP spends most of the night at 3.2-3.3 V
terminal, and a single-px look (~170 mA system) barely sags it -- today's rail IS the
plateau operating point. The 2026-06-12 goldening lived at 2.8-2.95 V under multi-px
show loads the gobo role never uses. Boost gain visibly grows with load (+7 % at just
7 px half) exactly as the dropout physics predicts -- the effect is real, the
production HEX just does not operate where it pays.

Caveats / remaining: n=1 hex pair, board, cell; single SOC. The one surviving boost
case for HEX is the low-SOC end (knee, ~3.0 V open) -- worth a cheap repeat on a
run-down battery before final BOM removal. The 4 W RGBW point source is a separate
question (different LED, own Vf stack) and inherits none of this verdict.

## 2026-07-02 - Ben + Claude - Bare r2 suite: swap reproducibility ~2%; boost gain grows with load

Bare hex remounted (swap 2). Third protocol gotcha closed for good: led_studio only
redraws static frames on an actual VALUE change -- re-sending identical values does
not render, so the r2 white capture caught a dark hex despite the "poke" (artifact
file kept). boost_ab_log.py now wiggles bri by 1 count and back, forcing two real
renders; the r2b redo is the valid bare white run.

Cross-swap reproducibility (the geometry error bound Ben's back-and-forth was for):
bare white 215.6 -> 211.5 lux across two physical swaps (~2 %); red single 33.3 vs
boosted 33.5, green 128.6 vs 129.5 (<1 % where no optical gain is expected). Seating
noise ~ +-1-2 %.

Bare r2 vs boosted r1, same sliders (diffs above the noise floor in bold):
  white 1 px full: 211.5-215.6 vs 216.7-217.4 lux (+1-3 %, marginal) at +60 % LED power
  red single: 33.3 vs 33.5 (nil)   green single: 128.6 vs 129.5 (nil)
  **blue single: 60.4 vs 63.6 (+5.3 %)** -- blue is the highest-Vf channel; mild
    starvation at ~3.2 V rail is visible exactly where physics says it should be
  **ring1 7 px bri=128: 557.9 vs 596.0 (+6.8 %)** at 0.388 vs 0.631 W branch power
Trend: boost gain grows with load (1 px white ~+1-2 %, blue single +5 %, 7 px +7 %) --
consistent with the dropout/sag hypothesis; the decisive regime (ring2/all-37, low
SOC, where bare sags to ~2.8-2.95 V) is still unprobed. Efficiency so far: boost
costs ~+60 % LED-branch power for single-digit optical gains at all probed points.
Next: boosted r3 remount (boosted-side reproducibility), then design the heavy-load
ladder carefully (boosted all-37 at bri=128 projects to ~1.1 A into the 3V3 header --
brownout risk; step up with live sag watch, abort below ~2.8 V input).

## 2026-07-02 - Ben + Claude - First boosted captures: +60% LED power, +1% light at the single-px test point

Boosted hex (TPS63802 4.2 V inline on V+) swapped in, same taped position. Two protocol
gotchas caught first: (1) a run captured a BLANK hex -- led_studio only pushes static
frames on change, so a hex hot-swapped after the last render stays dark until the next
/set; boost_ab_log.py now pokes a no-op render before every capture (the artifact file
`092726_boosted-center-rgbwhite-full-r1.jsonl` is kept as a record). (2) W-channel-only
is dark: the NeoHEX is RGB-only SK6812, so W drops out of the HEX protocol (still
relevant for the 4 W RGBW point source).

Headline (60 s runs, center px RGB white full, SOC ~97, battery ~3.2-3.3 V open):
bare 215.6 lux @ 0.134 W LED branch; boosted 216.7-217.4 lux @ 0.215 W LED branch =
**+60 % electrical, +<1 % optical, lumens/W drops ~40 % (1608 -> ~1010 lux/W)**.
Battery total 0.537 -> 0.635 W. Interpretation (single test point, n=1): a SINGLE pixel
at a healthy battery is NOT in dropout -- the SK6812 constant-current drivers were
already at regulated current at ~3.19 V, so the 4.2 V headroom burns in the drivers as
heat. The boost's claimed value regime (blue/green dropout, goldening) needs the
heavy-load rail sag (~2.8-2.95 V) and/or low SOC -- not yet probed. Do NOT generalize
to "boost is worthless" from this point alone.

New `ops/bench/boost_ab_suite.sh <config> <runtag>` runs the per-mount battery:
white-full 60 s, R/G/B singles 30 s, ring1 (7 px) white bri=128 30 s, restores the
look. Boosted r1 suite: red 33.5 lux / green 129.5 / blue 63.6 (channel branch powers
nearly equal at 0.112-0.114 W; singles sum to 226.6 vs white 216.7, additive within
~5 %), ring1-half 596 lux @ 0.631 W branch, battery 1.07 W, boost input bus sagged to
3.05 V. Next: swap to bare, `boost_ab_suite.sh bare r2`, then keep alternating
(Ben's plan: multiple back-and-forth mounts to bound the seating/geometry error), then
push into the heavy-load/low-SOC regime where the boost hypothesis actually lives.

## 2026-07-02 - Ben + Claude - Boost A/B harness live; bare-hex baseline captured

Ben's harness: desk | bare HEX | upside-down 3D-printed lantern proto (cylindrical
tube) | VEML7700 taped at the tube exit, ~6 in from the hex. Look: center pixel only,
r=g=b=255, w=0, bri=255, PowerFeather battery-only (sv=0.02). New
`ops/bench/boost_ab_log.py` merges the KB2040 'ina'/'lux' serial stream with
led_studio /state into labeled JSONL (ops/bench/data/boost_ab/).

INA channel map CONFIRMED (corrects the earlier idle-based guess, which had it
backwards): **0x41 = LED power out, 0x45 = battery (charge-positive, so discharge
reads negative)**. Three independent cross-checks: (1) 0x41 = 42.1 mA at center-white
== the known 41.8 mA single-px number from 2026-06-11; (2) 0x45 = -170 mA vs the fuel
gauge's -175 mA; (3) LED-off floor: 0x41 drops to 8.3 mA (= 37-px dark quiescent),
0x45 stays ~-135 mA (ESP + WiFi overhead). Budget closes: 42 (LED) + ~128 (system) ~=
170 (battery).

Bare-hex baseline (60 s, `2026-07-02_091851_bare-center-rgbwhite-full.jsonl`):
**215.6 lux** (sd 0.22) at the tube exit; LED 42.1 mA @ 3.19 V = **0.134 W**; battery
170 mA @ 3.16 V = **0.537 W** system. Ambient-dark reference (bri=0, same position):
**2.3 lux** floor -- LED dominates the reading; look restored and verified after.

Caution flag: a quick 12 s glance ~10 min before the logged run read 167.3 lux at the
SAME LED current (42.3 mA) -- a 29 % optical difference with an unchanged electrical
operating point, almost certainly geometry (final taping happened in between), not the
LED. Protocol consequences for the A/B: (1) nothing moves once positioned -- mark the
hex outline on the desk so bare and boosted hexes seat identically under the tube;
(2) take an ambient-dark reference each session; (3) run bare vs boosted back-to-back
at similar SOC; (4) compare same-sliders AND matched-lux. n=1 harness, unshrouded room
light -- treat absolute lux as position-specific, only ratios are portable.

## 2026-07-02 - Ben + Claude - ledstudio.local live on the desk board + lux channel on the monitor

Two bench-tooling steps for the TPS63802 4.2 V boost experiment:

- `firmware/led_studio/` now sets hostname + mDNS (`http://ledstudio.local/`) and was
  USB-flashed to the desk PowerFeather `9E5B0C` (ttyACM1), replacing
  `power-bench-2026-06-11.1` -- that image predates the `/update` endpoint, so there
  was no OTA path off it; the board was already tethered. Verified live:
  `ledstudio.local` resolves (192.168.4.76), the LED Studio UI serves, and the new
  image has `/update`, so future studio tweaks go over OTA per the standing preference.

- `firmware/ina_monitor/` gains an optional photopic lux channel on the same QT chain:
  TSL2591 (0x29) and VEML7700 (0x10) are auto-detected at boot, on 'r', and by a 5 s
  background re-probe (hot-plug friendly), and stream as `lux` lines interleaved with
  the `ina` lines -- light + electrical power in one timestamped serial stream. Fixed
  low-gain / 100 ms configs sized for LED-bench levels; a `sat=1` flag marks
  saturation (move the sensor back rather than re-gaining mid-comparison). KB2040
  reflashed no-touch via the 1200-baud bootloader touch; verified INAs still stream
  and both lux probes correctly report MISSING with nothing plugged in.

Sensor rationale (Ben's question: switch from PAR?): yes, for the boost verdict use a
photopic lux sensor as the primary light metric. The decision criteria are lumens and
lumens/W as perceived, and the channels the boost should recover are blue/green -- a
photopic sensor weights them like the eye, while the PAR meter's flat 400-700 nm
quantum response over-credits blue (it counts blue photons ~1:1 that the eye weights
~0.05). Keep the PAR meter (ttyACM0) logging as a spectrum-robust cross-check and for
continuity with plot_par_vs_draw data. For A/B ratios, absolute calibration is moot;
linearity + not saturating + fixed geometry are what matter. VEML7700 is the cleaner
photopic instrument; TSL2591 has more dynamic range -- either works, both supported.

Tentative INA channel labels from the live stream after the led_studio flash: 0x41
carries the system load (~44-45 mA with the ESP awake) = battery side; 0x45 idles at
1-2 mA = LED power out with LEDs off. n=1, unlabeled wiring -- confirm by lighting
pixels and watching which channel jumps before logging real runs.

## 2026-07-02 - Ben + Claude - KB2040 flashed as the INA monitor for the boost bench

The Metro that ran `firmware/ina_monitor/` is now on noisemaker duty, so the monitor role
moves to an Adafruit KB2040 (RP2040) for the TPS63802 4.2 V boost experiment (bare vs
boosted HEX, INAs on battery and LED power out). The sketch needed zero code changes:
`Wire.begin()` default pins are the STEMMA-QT port on both boards (KB2040 = GPIO12/13),
and `Serial.printf` works on the arduino-pico core. Compiled with
`rp2040:rp2040:adafruit_kb2040` and flashed by UF2 drop onto the RPI-RP2 bootloader
drive; header comment now documents both targets and the KB2040 flash path.

Bench note: the KB2040 initially did not enumerate at all (no lsusb entry despite power).
BOOTSEL-hold + reset brought up the RPI-RP2 bootloader on the same cable, so the cable
was fine; whatever firmware was previously on the board was not exposing USB. After the
UF2 drop it enumerates as 239a:8105 with CDC serial (ttyACM2 on this host; the Apogee PAR
meter is ttyACM0 and the desk PowerFeather 9E5B0C is ttyACM1). Verified live: probe found
the two SEN0291s at 0x41 and 0x45, i2c scan clean, both streaming at 10 Hz (~3.2-3.3 V
bus, ~7-10 mA idle on each -- which INA is battery vs LED rail still needs a load test to
label). Next: wire the boost hot-swap and run the eye test + PAR/INA comparison per the
TPS63802 TODO section.

## 2026-07-02 - Ben + Claude - Retired the 120 mAh/night budget floor

Removed the old ~120 mAh/night nightly-budget number as a reference point in SYSTEM.md,
AGENTS.md, and TODO.md (ADR 0021 left as-is, append-only). It was napkin math from before
hardware testing -- low-current ESP32-C3, very dim 1-3 pixel ambient assumptions -- and
the gobo work since shows crisp projection needs far more LED power than it assumed, so
keeping it around even as a "floor" invited anchoring. The production budget will be
derived bottom-up: measured LED draw (400-500 mA at full on HEX/RGBW) x a realistic show
duty cycle, minus measured harvest at MPP. The TODO item to compute it stays open.

## 2026-07-01 - Codex - Added Modulino Buzzer and Vibro I2C controls

Extended `firmware/clacker_demo/` for Ben's Arduino Modulino Buzzer and Modulino Vibro
boards on the Metro ESP32-S3 STEMMA/Qwiic bus. The dashboard now shows Buzzer and Vibro
detection status next to the Omron relay, has `Scan I2C`, adds `Modulino buzzer` as a
selectable tone output, and adds Vibro `Pulse`, `Buzz`, `Soft buzz`, and `Vibro off`
buttons.

Implemented the Modulino protocol directly from the Arduino libraries instead of adding
another dependency: Buzzer receives 8-byte little-endian frequency/duration packets at
7-bit address `0x1E` (firmware address `0x3C`), while Vibro receives 12-byte
frequency/duration/power packets at `0x38` (firmware address `0x70`, with a fallback probe
for the `0x3A`/`0x1D` address listed in some docs). Existing beep/sweep/Moonlight playback
now routes through the Modulino Buzzer when selected.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. The live
bench detected the SparkFun relay at `0x18`, Modulino Buzzer at `0x1E`, and Modulino Vibro
at `0x38`. Verified a short Modulino Buzzer chirp and a short Vibro pulse via the API,
then restored the amp output selection and left all relays/audio/vibro off with default
`420 ms` gap / `70 ms` pulse settings.

## 2026-07-01 - Codex - Added Qwiic Omron relay dashboard controls

Extended `firmware/clacker_demo/` so the Metro ESP32-S3 dashboard can click/clack the
SparkFun Qwiic Omron relay on the STEMMA/Qwiic port. Added an `Omron Qwiic click` button,
Qwiic scan/status display, and an `Start Omron` repeat-clack mode using the same gap and
pulse-width sliders as the existing relay controls. Starting Omron repeat mode stops the
A/B relay auto mode so the audible timing stays easy to compare.

The first implementation targeted the newer TCA9555-based SparkFun Qwiic Relay Line at
`0x20`/`0x21`, but the connected board did not detect there. Added support for the older
SparkFun Qwiic Single Relay protocol at `0x18`/`0x19` as well; the bench board detected as
`single` at `0x18`. Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on
`/dev/ttyACM1`, verified the dashboard API at `http://clacker.local/`, exercised a short
Qwiic pulse plus a short repeat-clack run, then restored defaults (`420 ms` gap, `70 ms`
pulse) and left all relays/audio off.

## 2026-07-01 - Codex - Added selectable D5/D6/D7 noisemaker outputs

Updated `firmware/clacker_demo/` after Ben wired a passive piezo to Metro `D6`/GPIO6 and
a SparkFun RedBot buzzer to `D7`/GPIO7 while keeping the 8002A amp/speaker on `D5`/GPIO5.
The dashboard now has a noisemaker selector plus quick chirp buttons for amp, piezo, and
RedBot; all existing beep, sweep, and melody controls play through the selected output.
The firmware detaches the prior LEDC/PWM pin before moving playback to another output so
only one noisemaker is driven at a time. The large alarm remains intentionally unpowered.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
the board rejoined as `http://clacker.local/` / `192.168.4.57`, exercised short chirps on
all three outputs via the API, muted playback, and left the selected output back on the
8002A amp.

## 2026-07-01 - Codex - Extended Moonlight to first high melody entrance

Extended the `Moonlight` button in `firmware/clacker_demo/` so it continues past the
opening triplet setup into the first high G#4 melody entrance ("duh duh-duh" piano-line
moment Ben called out). Because the bench output is still monophonic square-wave PWM, the
G#4 entrance is exaggerated as separated longer hits rather than layered over the arpeggio
like the real piano score.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
`/api/tune?id=moonlight` starts playback and muted with `/api/tune?id=stop`; final state
reported `tune="none"`.

## 2026-07-01 - Codex - Routed sweep buttons through melody scheduler

Ben reported the three sweep buttons were still silent while the Moonlight melody worked.
Changed `firmware/clacker_demo/` so `Sweep up`, `Sweep down`, and `Laser sweep` are now
explicit stepped note sequences run through the same proven monophonic scheduler as the
working melody buttons, instead of the separate continuous-retune sweep state machine.
This should avoid the silent behavior from rapid frequency retuning.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
`/api/sweep?id=up`, `/api/sweep?id=down`, and `/api/sweep?id=laser` each report the
expected active tune state, then muted with `/api/tune?id=stop`; final state reported
`tune="none"`. Acoustic confirmation still depends on Ben's bench listen.

## 2026-07-01 - Codex - Corrected Moonlight opening from referenced MIDI

Downloaded Ben's reference MIDI (`https://bitmidi.com/uploads/16752.mid`) to inspect the
opening. It is format 1 with 8 tracks and 120 ticks/quarter; the initial tempo is about
50 BPM, making the opening triplet notes about 400 ms apart. The recognizable opening
texture repeats the G#3-C#4-E4 triplet cell eight times before moving, whereas the prior
bench melody compressed each harmony into one ascending gesture and climbed too quickly.

Updated `firmware/clacker_demo/` so `Moonlight` is now a short monophonic reduction of
the first 16 triplet groups from the MIDI-derived opening pattern. This remains square-wave
single-voice playback on Metro `D5`/GPIO5, not a piano/PCM arrangement, but it preserves the
repeated triplet texture. Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on
`/dev/ttyACM1`, triggered `/api/tune?id=moonlight`, and then muted with
`/api/tune?id=stop`; final state reported `tune="none"`.

## 2026-07-01 - Codex - Reworked clacker sweeps and Moonlight melody

Updated `firmware/clacker_demo/` after Ben reported the three sweep buttons were silent
and the Moonlight sequence was too fast / not recognizable. Replaced the sweep playback
path with direct LEDC frequency control instead of rapid queued `tone()` calls, which is a
better fit for continuously changing frequencies. Also lowered and slowed the Moonlight
sequence into a more recognizable opening-arpeggio approximation, still monophonic square
wave rather than piano/PCM audio.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Exercised
`/api/sweep?id=up`, `/api/sweep?id=laser`, `/api/tune?id=moonlight`, then muted with
`/api/tune?id=stop`; API state returned to `tune="none"`. Actual acoustic quality still
needs Ben's ears at the bench.

## 2026-07-01 - Codex - Added sweep and melody buttons to noisemaker dashboard

Extended `firmware/clacker_demo/` speaker controls with nonblocking frequency sweeps
(`Sweep up`, `Sweep down`, `Laser sweep`) plus a simple monophonic Moonlight-style
arpeggio sequence. The new controls still use the existing `tone()`/PWM path on Metro
`D5`/GPIO5; this is not PCM or polyphonic audio, just note/sweep scheduling over square
waves.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
the page contains the new buttons, exercised `/api/sweep?id=laser` and
`/api/tune?id=moonlight`, then muted with `/api/tune?id=stop`.

## 2026-07-01 - Codex - Fixed clacker dashboard slider persistence

Fixed the `firmware/clacker_demo/` dashboard sliders snapping back during the 1 Hz state
refresh. Slider changes now push timing values immediately to a new `/api/settings`
endpoint, and the browser suppresses slider rewrites while a drag/update is in flight.
Added `Cache-Control: no-store` on dashboard/API responses so the browser reloads the
new JavaScript after reflashing.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
`/api/settings?interval=760&pulse=115` persisted through `/api/state`, then restored the
bench defaults to `interval=420` and `pulse=70`.

## 2026-07-01 - Codex - Added WiFi dashboard for relay/speaker noisemaker bench

Onboarded against the repo read order and fetched `origin/main`; local `main` was current
with `origin/main` (`0 0` ahead/behind), with pre-existing local changes in `LOG.md` and
untracked `firmware/clacker_demo/` preserved.

Reworked `firmware/clacker_demo/` from an automatic two-relay pulse sketch into an Adafruit
Metro ESP32-S3 WiFi dashboard for Ben's lantern noisemaker bench. The dashboard connects to
the shared bench AP via ignored `wifi_secrets.h`, serves at `http://clacker.local/`, drives
relay modules on Metro `A0`/`A1`, supports one-shot relay clicks plus adjustable A/B
auto-clack timing, and drives the 8002A amp/speaker signal from Metro `D5`/GPIO5 with
simple tone/melody buttons. Added a local build helper that uses a dedicated Arduino
`--build-path` to avoid shared-cache collisions.

Compiled the sketch for `esp32:esp32:adafruit_metro_esp32s3`, uploaded it to the connected
Metro on `/dev/ttyACM1`, and verified the dashboard API at `http://clacker.local/api/state`
with the board reporting IP `192.168.4.57`.

## 2026-06-30 - Codex - Updated clacker sketch for two-relay comparison

Updated `firmware/clacker_demo/` for Ben's A/B relay sound comparison: the sketch now
drives relay modules on Metro `A0` and `A1`, assumes high-trigger modules, and alternates
short pulses through slow, medium, and double-tap patterns. Reflashed the connected
Adafruit Metro ESP32-S3 on `/dev/ttyACM1` after compiling with a dedicated Arduino build
path.

## 2026-06-30 - Codex - Added relay clacker bench sketch

Added `firmware/clacker_demo/`, a small Arduino sketch for Ben's relay/noisemaker
experiment using a cheap Songle-based relay module. The sketch toggles Metro D13 through
slow, medium, and double-tap patterns so active-low and active-high relay boards can be
heard without changing firmware. The README records the initial 3V3/GND/D13 wiring and
notes that common SRD-05VDC relay modules may need USB 5 V on VCC while keeping D13 as
the logic input.

## 2026-06-30 - Codex - Hungry 6 Ah cell pulled near-nominal solar power

Ben moved the nearly-depleted 6 Ah 32700 LiFePO4 cell back onto the solar `9E5AB8`
PowerFeather after briefly proving that the same cell would take high power from an
Anker USB bank on the bench rig. The next fresh solar wake showed the earlier sub-watt
behavior was not a hard panel/charger ceiling:

- `battery_v=3.404..3.458`, `battery_ma=1000..1020`, and `battery_w=3.46..3.47`.
- `supply_v=4.859`, `supply_ma=774..792`, and `supply_w=3.76..3.85`.
- Panel-side INA reported about `5.16 V` and `0.79..0.81 A`, or about 4.1 W by
  magnitude (`ina_panel_w=-4.09..-4.16`; sign is wiring direction).
- BQ telemetry still showed `bq_vindpm_mv=4800`, `bq_ichg_ma=1480`,
  `bq_vreg_mv=3600`, charge enabled, HIZ false, VBUS source detected, and no fault.

Interpretation: the P105/5 W-class panel and BQ path can source roughly 4 W in direct hot
sun with a charge-hungry LFP cell. The earlier low-watt plateau was likely a transient
combination of very-low-VBAT charger behavior, battery acceptance/surface-voltage state,
solar input qualification/VINDPM interaction, and/or simply not yet enough sun. The brief
USB charge may have lifted the cell/charger out of a low-voltage regime, making this a
good candidate for a repeatable "recover from below 3.0 V" characterization rather than
evidence of a failed solar path.

## 2026-06-30 - Codex - 7200 mAh cell swap restored multi-watt solar harvest

Ben swapped the 7200 mAh 32700 LiFePO4 cell from the disconnected `9E5AF0` setup into
the solar-powered `9E5AB8` PowerFeather while the panel was connected. This produced
the expected harvest jump:

- Before the swap, low-cell / USB-rescue samples were around `supply_v=4.887`,
  `supply_ma=94..104`, and `battery_ma=36..38`; earlier solar-only samples with the
  depleted cell were roughly 0.3-0.6 W input and near-zero battery current after the
  OTA threshold event.
- After the swap, `9E5AB8` reported `battery_v=3.571`, `battery_ma=774`,
  `supply_v=5.554`, `supply_ma=542`, and `ina_panel_mv=5832`, `ina_panel_ma=-565`.
  That is about 3.01 W at the charger telemetry and about 3.30 W by panel-side INA
  magnitude, with about 2.76 W into the battery.
- BQ telemetry showed `bq_vindpm_mv=4800`, charge enabled, HIZ false, BATFET normal,
  VBUS adapter/source detected, charge-state 2 (CV/taper bucket), and no fault.

Interpretation: the panel/charger path can harvest multi-watt power in this setup. The
earlier sub-watt behavior was not a simple panel/MPP ceiling; it was dominated by the
deeply depleted cell's charge-acceptance/precharge/power-path state and/or the source
interaction. `9E5AB8` still has `cap=6000` in NVS after the physical 7200 mAh swap; leave
it alone for a clean short harvest comparison, then set targeted capacity to 7200 mAh
before relying on gauge/SOC accounting.

During the swap the COM7 serial bridge briefly USB-disconnected/rebooted (Windows eject
sound; dashboard raw log shows a fresh `.7` boot banner at about 2026-06-30 14:14
America/Los_Angeles). It came back on COM7. The dashboard backend and logger remained
alive; the browser page may need a refresh because its event stream can stale after a
USB reconnect.

## 2026-06-30 - Codex - BQ charger telemetry OTA added during USB-rescue test

Added `net-bench-2026-06-30.7` charger telemetry while `9E5AB8` was recovering from
low VBAT on an Anker USB bank with the solar panel disconnected. The change appends a
new heartbeat tail with BQ25628E VINDPM, charge-current limit, CV limit, raw
control/status/fault registers, and dashboard/log decodes for `CHG_EN`, `EN_HIZ`,
BATFET control, VBUS state, and charge state. Updated `ops/bench/net_bench_dashboard.py`,
`ops/bench/net_bench_log.py`, and this README path's telemetry docs.

Builds/flash:

- Built peer image at
  `firmware/net_bench/build/field-cycle-peer-20260630-v7/net_bench.ino.bin`.
- Built and USB-flashed the COM7 serial bridge/master to `.7`.
- Used targeted `U9E5AB8` so older drawdown peer `9E5AF0` was not pulled into
  maintenance.
- `9E5AB8` entered shared-WiFi maintenance at `192.168.4.40` and accepted OTA to `.7`;
  `net_bench_ota.py` recorded `t_ack_s=6.21`, recovered true, no button.

First `.7` post-OTA heartbeat:

- `battery_v=2.938`, `battery_ma=36`, `supply_v=4.887`, `supply_ma=104`.
- `bq_vindpm_mv=4800`, `bq_ichg_ma=1480`, `bq_vreg_mv=3600`.
- `CHG_EN=true`, `EN_HIZ=false`, BATFET normal, VBUS adapter state, charge-state CC
  bucket, `fault0=0`.

Interpretation: the charger/power path is healthy; the low Anker wattage is not ship
mode, HIZ, or a BQ fault. It is ordinary charge regulation/source behavior with the
board near a 4.8 V VINDPM point and the cell around 2.94 V. Logger continuation now
writes to `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8-v7-bq.jsonl`.

## 2026-06-30 - Codex - USB bank masked by higher solar input during field-cycle rescue

During the `9E5AB8` low-VBAT field-cycle run, Ben connected an Anker USB battery while
the solar panel was still attached. The Anker did not detect a load, while dashboard
telemetry still showed the PowerFeather supply at about `6.2 V` from the panel. After
Ben disconnected the solar panel, then disconnected/reconnected the Anker, the board
accepted USB input. A fresh wake at 2026-06-30 13:25 America/Los_Angeles showed:

- `supply_v=4.887`, `supply_ma=92`, `supply_good=true` from the charger telemetry.
- `battery_v=2.916`, `battery_ma=38`, `ina_batt_ma=34`, so the battery was charging
  slowly rather than disconnected.
- `ina_panel_mv=4788`, `ina_panel_ma=0`, confirming the panel path was no longer the
  active source.

Interpretation: a 5 V USB bank will not necessarily source current while the solar/VDC
input is already sitting above it. Once the solar input is removed, USB works, but this
build's `--maintain 4.8` solar VINDPM setting leaves little headroom on a 5 V bank
(`4.887 V` observed at the board) and likely throttles input current. Low-VBAT
precharge/trickle behavior may also be limiting cell current around 2.9 V. Follow-up:
add direct BQ25628E charger status/fault telemetry and consider a USB-rescue policy
that lowers VINDPM toward 4.6 V when the source is a USB/power-bank input rather than
a solar panel.

## 2026-06-30 - Codex - Solar-only low-VBAT OTA succeeded at 2.901 V

The armed field-cycle watcher caught `9E5AB8` on a fresh solar wake at
2026-06-30 11:58:58 America/Los_Angeles with `battery_v=2.901`, `supply_v=6.217`,
`supply_ma=76`, and `supply_good=true`. Because the peer was still running `.4`, the
watcher intentionally sent one last bare `U`, observed maintenance telemetry at
`192.168.4.40`, and uploaded the `.6` field-cycle image:

- `ops/bench/net_bench_ota.py` wrote `t_ack_s=5.08`, `ack="Update complete. Rebooting."`,
  `recovered=true`, `button_press_required=false`, notes
  `9E5AB8 .6 solar-only low-VBAT OTA at >=2.90V`.
- The peer rejoined ESP-NOW as `net-bench-2026-06-30.6`.
- The `.6` rail-restore change worked: lux, SHT31 panel temperature/RH, and onboard INA
  telemetry returned after sleep. Live sample after the OTA showed `lux=sat`,
  `ptc=45.5`, `prh=19`, `ipv=6456`, `ipa=-71`, `ibv=2888`, `iba=0`.

This validates the "low VBAT + external solar panel" OTA stress path at about 2.90 V
loaded/charging. Future maintenance on `.6` peers can use targeted `U9E5AB8`, so parallel
drawdown tests no longer need to be disturbed by single-peer OTAs.

## 2026-06-30 - Codex - Targeted maintenance command added for single-peer OTA

Added the non-universal maintenance command Ben asked for before starting parallel
drawdown tests. `net-bench-2026-06-30.6` keeps bare `U` as the sustained fleet
maintenance wake, but also supports `U<id>` such as `U9E5AB8`:

- Firmware: added `NB_TARGET_ENTER_MAINT`; peers enter maintenance only when the
  packet's 3-byte target id matches their short id. The master serial handler sustains
  either bare fleet `U` or targeted `U<id>` for 35 s so it can catch timer-wake windows.
- Dashboard: validates `U[0-9A-Fa-f]{6}` and changes `Peer maint` to send targeted
  maintenance for the selected peer instead of broadcasting to every awake peer.
- Sensor rail restore: on boot, PowerFeather 3V3 and VSQT/STEMMA rails are explicitly
  re-enabled before env/INA probing, with a short settle delay. This should restore
  panel/battery INA telemetry after a field-cycle rail-off sleep.

Built and USB-flashed the COM7 serial bridge/master to `.6`; built the `.6` field-cycle
peer OTA artifact. Because outdoor peer `9E5AB8` is still running `.4`, the solar-only
low-VBAT migration to `.6` must use one last bare `U`; after that, targeted `U9E5AB8`
can be used without disturbing separate 6 Ah vs 7.2 Ah drawdown experiments.

At 2026-06-30 10:37 America/Los_Angeles, `9E5AB8` was alive on solar-only `.4`, charging
around 2.79 V with about 0.48 W supply input. A single `.6` watcher remains armed to
trigger the solar-only low-VBAT OTA at a fresh wake with `battery_v >= 2.90` and
`supply_good=true`; an accidentally leftover `.5` watcher was stopped so only one upload
can fire.

## 2026-06-30 - Codex - Field-cycle lifecycle mode implemented and deployed to 9E5AB8

Graduated the low-VBAT stress-test path into a first production-ish lifecycle mode inside
`firmware/net_bench` rather than starting a new sketch, preserving the proven ESP-NOW
bridge, shared-WiFi OTA, PowerFeather solar guard, and dashboard tooling.

Implemented `--field-cycle`:

- Peer state machine: `charge` on external supply/solar -> rail-cut timer sleep while
  charging -> `wait-dark` when full-ish -> always-awake `draw` in dark using the normal
  1 Hz radio load -> `protect` timer sleep at low/critical LFP voltage.
- Sleep paths blank the pixels, cut both PowerFeather switchable rails, and use timer
  wake so the board remains recoverable. Solar/USB does not have to electrically wake the
  ESP32; the charger works while the ESP32 sleeps and the next timer wake observes supply.
- Added append-only heartbeat tail: `fc`/`fcr`/`fcc`/`fce`/`fcchg`/`fcdis`/`fcmin`/`fcmax`
  for lifecycle phase, transition reason, cycle count, phase elapsed seconds, rough
  charge/discharge mAh, and cycle voltage bounds.
- Bumped the ESP-NOW receive buffer from 96 to 128 bytes and fixed append-tail length
  checks so a `.4` bridge can still parse older `.2`/`.3` peers.
- Updated `build.sh`, `firmware/net_bench/README.md`, `ops/bench/net_bench_dashboard.py`,
  and `ops/bench/net_bench_log.py` for the new mode/telemetry.

Verification/deployment:

- Compiled field-cycle peer:
  `--role peer --channel 11 --hb-hz 1 --field-cycle --chem lfp --cap 6000 --charge-ma 1500 --maintain 4.8`.
- Compiled and USB-flashed COM7 bridge/master to `net-bench-2026-06-30.4`
  (`--role master --channel 11 --serial-bridge`). Dashboard restarted on
  `http://127.0.0.1:8765/` and showed bridge `.4`.
- Shared-WiFi OTA uploaded the field-cycle peer image to `9E5AB8` at `192.168.4.40`
  from about 2.67 V VBAT, USB bank supply good. Upload acked in 4.45 s with no button.
- `9E5AB8` rejoined as `net-bench-2026-06-30.4`, emitted `fc=2` (`charge`),
  `fcr=2`, `fcc=1`, `fce=305`, `fcchg=3`, `fcmin=2675`, `fcmax=2678`, then entered
  the 5-minute charge sleep with rails cut.
- `9E5AF0` was not OTA-updated; it was resumed from maintenance and then targeted-parked
  for 21600 s to stop draining while `9E5AB8` runs the field-cycle test.
- Started long JSONL logger:
  `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8.jsonl`
  (`--duration 172800`, notes `9E5AB8 field-cycle .4 first day/night lifecycle`).

Next: let the logger run through the next wake/charge/dark/protect transitions, then
summarize charge recovery, sleep cadence, drawdown duration, cutoff reason, and whether
the full/taper heuristic needs adjustment.

## 2026-06-30 - Codex - Low-VBAT remove-from-bank behavior check

Checked the live COM7 serial-bridge dashboard and current `firmware/net_bench` code for
Ben's question about removing a very low `9E5AB8` from a USB battery bank.

Live dashboard snapshot at about 2026-06-30T14:16Z:

- `9E5AB8`: `net-bench-2026-06-30.3`, COMMS mode, 2.54 V VBAT, 0% SOC,
  +35 mA into the battery, `supply_good=true`, 4.875 V / 92 mA supply, about 0.36 W
  running load, `drawdown_active=false`.
- `9E5AF0`: `net-bench-2026-06-30.2`, 3.15 V loaded, about -165 mA, no supply.

Conclusion: the current ordinary net_bench COMMS image does **not** automatically enter
deep sleep just because VBAT is low or external supply disappears. Low-voltage sleep
exists only in specific paths: manual/broadcast `S`, targeted `P<id>[:seconds]`,
`--sleep-cycle`, `--autosleep`, and the targeted drawdown helper's soft/hard floors
(3.18 V / 3.05 V). The maintenance power check is advisory by default and protects OTA
entry reporting, not normal runtime. At 2.5 V, removing USB supply without first parking
the peer is expected to run the board at roughly always-on peer load until voltage
collapses, likely ending in shutdown/brownout behavior rather than graceful sleep.
Recommended bench action before unplugging: targeted park, e.g. `P9E5AB8:21600`, then
let it charge/recover later.

## 2026-06-30 - Ben + Codex - Low-VBAT charging OTA stress pass on 5AB8

Ran a targeted low-voltage charging OTA stress test on `9E5AB8` after the overnight
run-down rescue. No AP maintenance mode was used.

Starting bridge state at 2026-06-30T14:05Z:

- `9E5AB8`: live at 2.461 V, +37 mA into the battery, 0% SOC,
  `supply_v=4.863 V`, `supply_ma=92 mA`, `supply_good=true`,
  `net-bench-2026-06-30.1`.
- `9E5AF0`: live at 3.150 V loaded, -168 mA, `net-bench-2026-06-30.2`.

Bumped the peer test image to `net-bench-2026-06-30.3` solely to make the OTA proof
unambiguous, then built the peer image for channel 11 / LFP / 6000 mAh /
1500 mA charge limit. Binary string check before upload found `BubbyNet` and
`maintenance WiFi up`; `ResonanceMaint`, `Brandon Springs`, and old `.1`/`.2`
firmware markers were absent.

Sent the shared-WiFi maintenance command `U` through the COM7 bridge. At the maintenance
endpoint, `9E5AB8` was at about 2.496 V, +36.2 mA into the battery, 4.875 V USB supply,
and `supply_good=true` on `192.168.4.40`.

Uploaded only to `9E5AB8`:

`python ops\bench\net_bench_ota.py --bin firmware\net_bench\build\ota-20260630-lowvbat-charging-5AB8-peer-bubbynet\net_bench.ino.bin --nodes 9E5AB8=192.168.4.40 --jobs 1 --reboot comms`

Result file:
`ops/bench/data/ca/2026-06-30-low-vbat-charging-5AB8-ota-results.jsonl`.

The upload acked in 5.88 s with no button:
`Update complete. Rebooting.` `9E5AB8` rejoined ESP-NOW as
`net-bench-2026-06-30.3`, `reset_reason=software`, about 2.50 V, still charging
from USB (`supply_good=true`, about 92 mA supply current). The bounded monitor ran to
`ops/bench/data/ca/2026-06-30-5AB8-low-vbat-charging-ota-monitor.jsonl`; final sample
showed 2.507 V, +33 mA battery current, `supply_good=true`, `.3`, and fresh ESP-NOW
heartbeats.

Operational note: because `U` is still broadcast-only, `9E5AF0` also entered the
shared-WiFi maintenance window. The HTTP `/resume` request to `192.168.4.39` timed out
at the client, but the peer was verified fresh on the bridge again at 3.151 V,
`net-bench-2026-06-30.2`, no button.

## 2026-06-30 - Ben + Codex - Overnight run-down rescue and single-peer OTA pass

Ben accidentally ran the two wireless peers down overnight, which produced a useful
recovery/OTA boundary test. Baseline from the COM7 dashboard:

- `9E5AB8`: stale by about 35k seconds, last heartbeat at 2.381 V, -173 mA,
  no supply, `net-bench-2026-06-30.1`.
- `9E5AF0`: live at about 3.151 V loaded, -169 mA, SOC 8%,
  `net-bench-2026-06-30.1`.

Started a rescue monitor at
`ops/bench/data/ca/2026-06-30-5AB8-usb-revive-monitor.jsonl`, then Ben turned on the
USB battery feeding `9E5AB8`. The transition was immediate:

- 2026-06-30T13:58:20Z: still stale, 2.381 V, -173 mA, `supply_good=false`.
- 2026-06-30T13:58:21Z: fresh heartbeat, 2.388 V, +34 mA into the battery,
  `supply_v=4.855 V`, `supply_ma=88 mA`, `supply_good=true`.
- By 2026-06-30T14:04:10Z after HTTP `/resume`, it was back on ESP-NOW at 2.449 V,
  +35 mA, supply good, no button/USB data cable required.

For the other peer, bumped the net-bench version to `net-bench-2026-06-30.2` solely to
make the OTA proof unambiguous, then built a peer image for channel 11 / LFP / 7200 mAh
with `ARDUINO_BUILD_PATH` set only to keep a stable artifact path. `build.sh` already
uses a unique Arduino build path by default, so the cache-collision protection remains
inside the helper. Binary string check before upload: `BubbyNet` and `maintenance WiFi
up` present; `ResonanceMaint`, `Brandon Springs`, and old `.1` version absent.

Sent shared-WiFi maintenance command `U` through the bridge. Both awake peers joined
BubbyNet maintenance because the current command is broadcast-only:

- `9E5AF0` -> `192.168.4.39`, about 3.156 V, no supply.
- `9E5AB8` -> `192.168.4.40`, about 2.435 V, USB supply good.

Uploaded only to `9E5AF0`:

`python ops\bench\net_bench_ota.py --bin firmware\net_bench\build\ota-20260630-overrun-5AF0-peer-bubbynet\net_bench.ino.bin --nodes 9E5AF0=192.168.4.39 --jobs 1 --reboot comms`

Result file:
`ops/bench/data/ca/2026-06-30-overnight-5AF0-ota-results.jsonl`.

The upload acked in 5.06 s with no button. `9E5AF0` rejoined ESP-NOW as
`net-bench-2026-06-30.2`, `reset_reason=software`, about 3.15 V loaded. `9E5AB8` was
returned from maintenance via `http://192.168.4.40/resume` and rejoined ESP-NOW while
continuing to charge from the USB battery. No AP maintenance mode was used.

## 2026-06-29 - Ben + Codex - OTA failure interpretation softened after clean low-voltage pass

Interpretation update after the official shared-WiFi low-voltage OTA pass: the earlier
maintenance failures should not be described as proven low-VBAT instability. The clean
successful run updated both peers at about 3.10 V loaded (`9E5AB8`) and 3.27 V loaded
(`9E5AF0`) over shared WiFi with no AP and no button. That makes stale WiFi credentials
and AP-contaminated builds the more likely root causes of the confusing pre-upload
failures:

- local `wifi_secrets.h` targeted `Brandon Springs Activity Guest` while the laptop was
  actually on `BubbyNet`;
- `9E5AF0` still had a deprecated `NB_MAINT_AP` image and advertised
  `ResonanceMaint-9E5AF0`;
- old pre-`.5` images could also trip the task watchdog during maintenance entry because
  the WiFi join loop was not feeding it.

Low battery is still a stressor and a boundary variable for production policy, but the
2026-06-29 evidence does not prove that low VBAT caused the earlier OTA instability.
Treat 3.10 V loaded as a proven successful shared-WiFi OTA point, and treat the older
2.95-3.03 V failures as ambiguous / wrong-path pre-upload failures rather than voltage
cutoffs.

## 2026-06-29 - Ben + Codex - Official shared-WiFi low-voltage OTA passed on two peers

Ran the official low-voltage OTA test on the two wireless peers with the COM7 serial
bridge back on USB. This was the fleet path only: shared WiFi (`BubbyNet`) plus
`ops/bench/net_bench_ota.py` parallel uploads. No peer self-AP was used.

Pre-test live baseline from the dashboard:

- bridge `9F26F8`: COM7 serial bridge, channel 11, `net-bench-2026-06-29.4`.
- peer `9E5AB8`: `net-bench-2026-06-29.5`, about 3.098 V loaded / -156 mA, SOC 9%;
  INA battery about 3.100 V / -123 mA. This is below the `.5` advisory LFP OTA floor.
- peer `9E5AF0`: `net-bench-2026-06-29.5`, about 3.274 V loaded / -162 mA, SOC 30%.

Bumped `firmware/net_bench/net_bench.ino` version string to
`net-bench-2026-06-30.1` solely to make the OTA proof unambiguous, then built a non-AP
peer image with isolated build path:

`--role peer --channel 11 --hb-hz 1 --chem lfp --cap 6000 --charge-ma 1500`

Binary string check before upload: `ResonanceMaint` absent, `maintenance WiFi up`
present, `BubbyNet` present, stale `Brandon Springs` absent, `.1` version present, old
`.5` version absent.

Sent dashboard command `U` for sustained ESP-NOW `ENTER_MAINT`; both peers joined shared
WiFi maintenance and exposed `/telemetry` on BubbyNet:

- `9E5AF0` -> `192.168.4.30`, still `.5`, mode 1, battery 3.278 V.
- `9E5AB8` -> `192.168.4.33`, still `.5`, mode 1, battery 3.102 V.

Ran:

`python ops\bench\net_bench_ota.py --bin firmware\net_bench\build\ota-20260630-lowvoltage-official-peer-bubbynet\net_bench.ino.bin --nodes 9E5AF0=192.168.4.30,9E5AB8=192.168.4.33 --jobs 2 --reboot comms`

Results written to `ops/bench/data/ca/2026-06-30-official-low-voltage-ota-results.jsonl`:

- `9E5AB8`: upload ack `Update complete. Rebooting.`, `t_ack=4.46 s`, recovered true,
  no button.
- `9E5AF0`: upload ack `Update complete. Rebooting.`, `t_ack=4.96 s`, recovered true,
  no button.

Post-OTA ESP-NOW verification via the bridge:

- `9E5AB8`: rejoined with `firmware_rev=net-bench-2026-06-30.1`,
  `reset_reason=software`, age < 1 s, battery about 3.09-3.10 V loaded.
- `9E5AF0`: rejoined with `firmware_rev=net-bench-2026-06-30.1`,
  `reset_reason=software`, age < 1 s, battery about 3.27 V loaded.
- WiFi scan after the run showed only `BubbyNet`; no `ResonanceMaint-*` SSID.

Interpretation: current shared-WiFi OTA path is proven on two wireless peers at a lower
successful LFP voltage of about 3.10 V loaded (external INA around 3.10 V on `9E5AB8`).
This is a successful lower bound, not a final hard production threshold below which OTA
must be blocked.

## 2026-06-29 - Ben + Codex - Shared-WiFi OTA lower-bound attempt exposed AP-contaminated peer

Attempted the requested low-battery OTA pass on the two live peers using the shared-WiFi
fleet path only. Did not connect to or upload through any peer self-AP.

Pre-attempt state from the serial-bridge dashboard/log:

- bridge `9F26F8`: `net-bench-2026-06-29.4`, channel 11, serial bridge on COM7.
- peer `9E5AB8`: around 3.02-3.03 V loaded, SOC 4-5%, INA battery about 3.01-3.03 V
  and roughly -125 to -160 mA, still heartbeating.
- peer `9E5AF0`: around 3.258 V loaded, SOC 28%, still heartbeating before the command.

Built a non-AP peer image from current source (`net-bench-2026-06-29.5`) with an isolated
Arduino build path:

`--role peer --channel 11 --hb-hz 1 --chem lfp --cap 6000 --charge-ma 1500`

Sent dashboard command `U`, which is the sustained ESP-NOW `ENTER_MAINT` broadcast while
the master stays in serial-bridge comms. A shared-subnet scan found no peer `/telemetry`
endpoints, so no OTA upload occurred. The laptop was on `BubbyNet`; the checked-in local
`firmware/net_bench/wifi_secrets.h` currently targets `Brandon Springs Activity Guest`,
so the bench WiFi endpoint discovery was not on a proven same-SSID setup.

More importantly, `ResonanceMaint-9E5AF0` appeared in the OS WiFi scan after `U`. That
means peer `9E5AF0` is still running a deprecated `NB_MAINT_AP` image, despite the desired
test being shared-WiFi only. Treated that peer as AP-contaminated and did not use its AP.
It will need USB flashing or a deliberate one-off recovery decision before it can
participate in a true shared-WiFi lower-bound test.

Peer `9E5AB8` did not expose a shared-WiFi endpoint either. It rebooted during the
maintenance-entry window and came back with `reset_reason=task_watchdog` at about
3.03 V, then continued heartbeating around 3.01-3.02 V. This establishes a practical
lower-bound result for the old running image: around 3.02-3.03 V loaded is too low for
reliable shared-WiFi maintenance entry on that image. It is a pre-upload failure, not an
OTA transfer failure. After capturing the evidence, sent targeted sleep
`P9E5AB8:21600`; its heartbeat age climbed, confirming it parked instead of continuing
to drain the low LFP cell.

Follow-up USB cleanup: corrected the local, gitignored `firmware/net_bench/wifi_secrets.h`
from the stale `Brandon Springs Activity Guest` SSID to `BubbyNet`, matching the laptop's
current shared WiFi. Built and USB-flashed COM4, which enumerated as `9E5AB8`
(`D8:85:AC:9E:5A:B8`), with the same `.5` non-AP peer image. Binary string check:
`ResonanceMaint` absent, `maintenance WiFi up` present, `BubbyNet` present, stale Brandon
Springs SSID absent, `.5` version present. Serial boot banner confirmed
`net-bench-2026-06-29.5`, role peer, channel 11, node `9E5AB8`, LFP 6000 mAh / 1500 mA,
env/INA sensors present, and direct entry to `COMMS (ESP-NOW)`.

Important caveat: the visible self-AP was `ResonanceMaint-9E5AF0`; the USB board just
flashed was `9E5AB8`. Therefore `9E5AB8` is now known-clean for shared-WiFi maintenance,
but `9E5AF0` should still be treated as AP-contaminated unless it times out back to comms
and is positively identified as a non-AP build, or is USB-flashed too.

Second USB cleanup: Ben connected `9E5AF0` as COM6. USB serial HWID/MAC identified it as
`D8:85:AC:9E:5A:F0`; flashed the same verified `.5` BubbyNet non-AP peer image. Serial
boot banner confirmed `net-bench-2026-06-29.5`, role peer, channel 11, node `9E5AF0`,
LFP 6000 mAh / 1500 mA, no env/INA sensors on this board, watchdog enabled, and direct
entry to `COMMS (ESP-NOW)`. A fresh WiFi scan showed only `BubbyNet` and no
`ResonanceMaint-*` SSID. Both previously live peers (`9E5AB8` and `9E5AF0`) are now
known-clean non-AP `.5` builds for the next shared-WiFi OTA test.

Next shared-WiFi lower-bound test should start only after:

- both target peers are confirmed non-AP builds (no `ResonanceMaint-*` SSID can appear);
- `wifi_secrets.h` matches the bench SSID the laptop is actually on, or a dedicated
  portable router SSID;
- at least one peer is already on `.5` or newer so maintenance-entry watchdog feeding and
  immediate OTA-start failure resume are present;
- the test records the pre-`U` loaded voltage and INA voltage/current, then runs
  `net_bench_ota.py --reboot comms` only against discovered shared-WiFi `/telemetry` IPs.

## 2026-06-29 - Codex - OTA maintenance-entry hardening after flaky low-battery attempts

Onboarded against the current repo context and traced the recent OTA failures against the
validated 2026-06-08 path. Read: standard OTA + rollback remains validated; the recent
failures happened before firmware transfer, while entering/discovering maintenance mode
from a low or poorly powered peer.

Updated `firmware/net_bench` to `net-bench-2026-06-29.5`:

- peers now report a maintenance-entry power preflight before leaving ESP-NOW. Advisory
  floors are LFP >= 3.20 V, Generic_3V7 >= 3.60 V, or an accepted supply current >=
  250 mA, but enforcement defaults OFF (`NB_MAINT_POWER_ENFORCE=0`) so the low-voltage
  OTA lower bound can be measured instead of guessed. A below-advisory peer sends one
  heartbeat with `mt=2` before attempting maintenance.
- if WiFi/AP OTA startup fails, the peer immediately resumes ESP-NOW instead of waiting
  for the long maintenance timeout.
- the OTA upload route now feeds the task watchdog during POST/upload handling, and AP
  startup feeds the watchdog too. The earlier `.4` WiFi-association watchdog fix remains.
- heartbeat/bridge telemetry gained maintenance status (`mt=`), and the dashboard/log
  parser records it. Dashboard marks `OTA power warn` / `OTA start failed` when present.

Verification: compiled an LFP peer image and a serial-bridge master image with isolated
Arduino build paths; both compile. `python -m py_compile` passes for
`net_bench_dashboard.py`, `net_bench_log.py`, and `net_bench_ota.py`.
Promoted the Arduino parallel-compile/cache collision and deprecated `--maint-ap`
warnings into the top of `AGENTS.md` so future sessions see them during onboarding, and
changed `firmware/net_bench/build.sh` to use a unique temporary Arduino build path per
run so the safe behavior is built into the script.

Interpretation for field reliability: the old success case was real -- software-reset OTA
and rollback worked. The current task is to retire AP-mode confusion, use the shared-WiFi
parallel OTA path, and measure the real lower-voltage OTA boundary before turning any
voltage threshold into a blocking production policy.

## 2026-06-29 - Ben + Codex - Outdoor peer reflashed to parallel OTA path

Attempted a low-battery OTA setup on outdoor solar peer `9E5AB8` around 2.95 V. The
peer stopped normal ESP-NOW heartbeats after `U`, but no reachable maintenance AP or
shared-WiFi peer IP was found, so no OTA upload occurred. Treat this as a lower-bound
maintenance-entry/credentials-path failure, not a firmware-transfer failure.

Ben USB-plugged the outdoor peer as `COM4`; flashed `net-bench-2026-06-29.3` peer image
with LFP chemistry, 6000 mAh capacity, 1500 mA charger cap, channel 11, and 1 Hz
heartbeat. A first flash accidentally included the per-device maintenance AP fallback;
reflashed immediately without `NB_MAINT_AP`, using the local WiFi secrets include path so
maintenance mode remains the scalable shared-WiFi path. Boot banner confirmed node
`9E5AB8`, `net-bench-2026-06-29.3`, sensors present, and ESP-NOW comms. Binary string
check confirmed the image contains `maintenance WiFi up` and not `ResonanceMaint` or the
maintenance-AP path.

Added targeted bench config commands so dashboard row focus can become an address instead
of only a view filter: `C<id>:<mah>` targets capacity/gauge config and `G<id>:<mA>`
targets charger-current config, while bare `C<mah>`/`G<mA>` remain serial console fleet
broadcasts. Updated the dashboard to require one selected peer before sending capacity or
charge changes, and documented that `--maint-ap` is an emergency single-board recovery
mode only, not the fleet OTA path. Master compile and dashboard `py_compile` pass; the
USB bridge still needs to be flashed to `.3` before the new targeted UI commands can work
through the dashboard.

Follow-up after Ben plugged the bridge back in: reflashed COM7 (`9F26F8`) as the `.3`
serial bridge with `NB_SERIAL_BRIDGE=1`, channel 11, and 1 Hz default frame rate, then
restarted the dashboard at `http://127.0.0.1:8765/` and sent `R1`. Verified the new
targeted config path with no-op `G9E5AB8:1500`; the bridge printed
`target SET_CHARGE_MA 9E5AB8 1500 mA` and both peers stayed online. Live state then
showed `9E5AB8` around 3.03 V / 7% SOC with solid RSSI, but panel current still 0 mA and
the battery discharging roughly 0.5 W, so the current physical solar/charger condition is
not net-positive.

Promoted the bright-sun PowerFeather/BQ25628E input-latch gotcha from documented bench
knowledge to a firmware baseline. Added shared `firmware/powerfeather_solar_guard.h`:
it force-sets `REG0x17[0] VBUS_OVP=1` at charger init, watches for the stuck signature
(`supply_v` near panel Voc, `supply_good=false`, near-zero input current), and toggles
`EN_HIZ` to re-run input qualification without a physical unplug. Wired it into the
solar/charging Resonance sketches (`net_bench`, `power_bench`, `led_studio`) and updated
the firmware notes/TODO so future solar firmware treats the guard as mandatory baseline
practice. Remaining gate is bright-sun hardware validation of an automatic clear.

Added firmware-revision visibility to the net_bench dashboard. `net_bench` `.4` now
appends a fixed `fw_rev` tail to peer heartbeats, emits `fw=` on bridge master lines, and
bumps the ESP-NOW receive buffer from 64 to 96 bytes so the larger heartbeat is accepted.
`ops/bench/net_bench_dashboard.py` parses and renders firmware under each peer ID plus in
the master panel. Reflashed COM7 bridge `9F26F8` to `.4` and verified the live dashboard
shows `net-bench-2026-06-29.4` for the bridge; existing `.3` peers correctly show `fw ?`
until they are updated.

Attempted shared-WiFi maintenance entry again before peer OTA. No `/telemetry` endpoints
were reachable; outdoor peer `9E5AB8` came back with `task_watchdog`, indicating the old
`.3` peer can trip the 8 s watchdog while blocked in the 20 s WiFi-join loop. Patched
`.4` to feed the watchdog during WiFi association in both maintenance and master WiFi
joins. Peer OTA is deferred until USB flash or a known-good quick maintenance join puts
at least one peer on the watchdog-safe image.

## 2026-06-29 - Ben + Codex - Repo text normalized to ASCII

Normalized tracked text files to ASCII equivalents to avoid Windows/codepage mojibake
when agents or shell tools print project docs. Replaced Unicode punctuation and symbols
with plain forms such as `--`, `->`, `>=`, `<=`, `deg`, `ohm`, `u`, `x`, and ASCII tree
drawing. Kept binary assets and live bench data out of the mechanical rewrite.
Added an `AGENTS.md` style note asking future agents to keep Markdown/docs ASCII-only
unless Unicode is project-critical.

Verification: `rg -nP "[^\x00-\x7F]"` over tracked text now returns no hits, literal
mojibake glyph scan returns no hits, `git diff --check` is clean aside from CRLF warnings,
and all changed `ops/bench/*.py` scripts pass `python -m py_compile`.

## 2026-06-29 - Ben + Codex - Dashboard radio-rate and solar-nap controls

Added live dashboard controls for reducing ESP-NOW bench overhead while a solar peer is
trying to recover from a near-empty LFP. `ops/bench/net_bench_dashboard.py` now exposes
quick `R1`/`R2`/`R5`/`R10` buttons, a custom `R<hz>` heartbeat-rate input, and a selected
peer `Nap` control that sends `P<id>:seconds`. The dashboard command validator now accepts
bounded `R1..R100` and targeted `P<id>[:seconds]` commands.

Updated `firmware/net_bench/` to `net-bench-2026-06-29.2`: the serial bridge can now set
a direct radio/frame rate with `R<hz>`, and peers that have the matching build can accept
a targeted `NB_TARGET_SLEEP_FOR` packet and enter timed deep sleep while other peers keep
running. Documented both commands in `firmware/net_bench/README.md`.

Built both master and peer images. Flashed the COM7 bridge (`9F26F8`) with the new master
image, restarted the dashboard at `http://127.0.0.1:8765/`, and sent `R1`. Live telemetry
confirmed the outdoor solar peer `9E5AB8` still reports around 2.93 V / SOC 0% with net
positive charge, but its awake load remains roughly 0.35 W; lowering heartbeat cadence is
not the same as sleeping the MCU/radio. The bigger recovery lever is `P9E5AB8:3600`, but
that requires flashing the outdoor peer to the new peer image first, preferably over USB or
after it has enough charge for a safe maintenance window.

Bench note: after the bridge work, current telemetry showed indoor peer `9E5AF0` no longer
actively drawing down (`drawdown_active=false`, `drawdown_mah=0.0`) while still alive around
45% SOC. Treat the earlier 7200 mAh HEX drawdown run as interrupted/invalidated unless a
separate JSONL review says otherwise.

## 2026-06-29 - Ben + Codex - Multi-peer dashboard focus polish

Updated `ops/bench/net_bench_dashboard.py` so the local net_bench dashboard behaves
cleanly when multiple peers with different telemetry capabilities are online. Added an
All/peer focus selector, metric source labels, and capability-aware top-card selection:
panel and charger cards now stay sourced from the peer that actually has panel/supply
telemetry, while All view shows net battery power across fresh peers. Selecting the
indoor HEX drawdown peer now shows stable "no panel telemetry" instead of flickering
between panel data and missing fields as heartbeats alternate.

Condensed the peers table into grouped `link` / `battery` / `supply` / `panel` / `state`
cells so both the outdoor solar peer and indoor drawdown peer fit together without a
horizontal scrollbar at the normal dashboard viewport. Restarted the COM7 dashboard and
verified the live page in-browser with both peers present; the drawdown logger and peer
continued running.

## 2026-06-29 - Ben + Codex - Targeted 7200 mAh HEX drawdown started

Onboarded against the current repo context, then prepared the Amazon 7200 mAh LFP /
PowerFeather / HEX stack for tomorrow's P105 full-sun demand-limit test. Found the old
LED Studio image on the LAN at `192.168.4.30`; ARP mapped it to MAC
`d8:85:ac:9e:5a:f0`, so the net_bench node id is `9E5AF0`. Brief LED Studio probes
showed the battery around 3.30 V under the red-ring load and about `-0.75 A` with the
HEX at all-white brightness 128.

Added a targeted `net_bench` drawdown command, `D<nodeid>[:mah]`, so the serial bridge can
start a HEX load on one peer without disturbing other live peers. The peer integrates
discharge current in firmware, advertises `dd`/`ddb`/`dda` in the bridge line, stops on a
mAh budget or guarded LFP voltage floor, explicitly blanks the SK6812 frame, cuts rails,
and timed-sleeps for 12 hours. Updated the dashboard command whitelist/parser and
`net_bench_log.py` so JSONL captures `cap`/`chg` and drawdown fields.

Flashed the COM7 serial bridge (`9F26F8`) with `net-bench-2026-06-29.1`, then OTA-flashed
`9E5AF0` from LED Studio to:

`--role peer --channel 11 --chem lfp --cap 7200 --charge-ma 1500 --hb-hz 1 --maint-ap`

Started logger
`ops/bench/data/ca/2026-06-29-ca-lfp-7200-hex-drawdown-9E5AF0.jsonl` and sent
`D9E5AF0:3500`. Initial drawdown telemetry: `bv` about 3.22 V loaded, `ima` about
`-0.84 A`, `drawdown_active=1`, with existing solar peer `9E5AB8` unaffected. Expected
end condition is either about 3500 mAh delivered or the guarded loaded-voltage floor, then
12 h sleep to preserve the hungry battery for the next full-sun P105 run.

## 2026-06-29 - Ben + Codex - Voltaic ETFE outdoor MPP comparison

Ran the local power dashboard against the PowerFeather solar telemetry peer on `COM7`
with the Voltaic 5 W ETFE panel (`P105`) and the smaller Voltaic ETFE panel (`P126`),
both into a 2 Ah LFP that was hungry enough to accept real charge current. Data was logged
to `ops/bench/data/ca/2026-06-29-ca-lfp-6000-net-solar-telemetry-1hz-2118.jsonl`
(run label still says `lfp-6000`; the live peer config was changed to `C2000` for the
2 Ah pack).

Findings:

- `P105` 5 W ETFE: with about 15 deg tilt, best observed region was around `m46`/`m48`.
  At `m48`, panel-side INA was about 5.1-5.3 V and 0.73 A, roughly 3.8-3.9 W. Charger
  input was about 3.47 W and battery-side charge about 3.1-3.2 W. Raising toward `m52`
  lost power. This is less surprising against the P105 datasheet expected `Vmp` near
  4.69 V than against the storefront headline values. Remaining caveat: the 5 W run may
  still be battery-acceptance-limited; LFP near 3.55-3.6 V can enter CV/taper or hit
  terminal-voltage limits early, especially on a smaller/higher-IR cell.
- `P126` smaller ETFE: all results were with about 15 deg tilt. Best observed region was
  around `m58`; panel-side INA reached about 6.1 V and 0.31 A, roughly 1.89 W, and
  charger input was about 1.66-1.68 W. `m60`/`m62` fell off. The panel is proportionally
  close to its nominal 2 W rating in real hot/late-day conditions.
- MPP matters materially for both panels. The 5 W panel gained roughly 0.4 W charger-side
  from a poor/higher setpoint to best; the 2 W panel gained roughly 0.2 W from `m48` to
  best. As a daily-energy term, that is about 1-2 Wh/day over a 5-full-sun-hour heuristic.

Interpretation: the Voltaic ETFE panels look promising for the BOM, especially the small
panel for HEX fixtures. Use panel-side INA as panel-capability truth when available; use
charger input/battery current as system truth. For a cleaner P105 verdict, re-run with the
larger 6-7.2 Ah LFP intentionally discharged to a mid-SOC/hungry voltage region so charger
taper and cell IR are less likely to cap demand. The stair-step sweep results also make a
simple periodic software MPPT/hill-climber worth implementing and measuring.

## 2026-06-29 - Codex - Re-onboarded and reviewed OTA/stuck-device failure modes

Re-read the session-start project context (`README.md`, `LOG.md`, `TODO.md`,
`docs/block-diagram/SYSTEM.md`, the OTA/LED/PowerFeather ADRs, `POWERFEATHER_NOTES.md`,
`net_bench` docs, and the brownout/networking test notes) after context compaction.
Current state is consistent with the earlier onboarding: PowerFeather V2 remains the
validated reference, direct-GPIO LEDs remain the production LED interface, battery-only
standard OTA with rollback is feasibility-green, and the hardening work is now about
guardrails around power state, charger input qualification, rollback health, low-battery
maintenance entry, and field recovery operations.

Reliability read: the recent low-battery maintenance-AP experiment should be treated as a
boundary warning, not a refutation of the validated OTA path. It showed that an
always-awake, deeply depleted peer can brownout during AP/maintenance transition, and that
a single-WiFi laptop cannot both join the peer AP and keep Codex/backend connectivity. The
production answer should keep the default shared-router OTA path for parallel updates,
retain self-AP as a one-device recovery lane, gate maintenance on voltage/current/supply
state, and keep USB/pogo or at least external USB-power recovery as the guaranteed last
resort.

## 2026-06-29 - Ben + Codex - Recovered solar peer over USB with USB-safe VINDPM

After the low-battery maintenance-AP experiment stranded peer `9E5AB8`, unplugged the
USB serial bridge and connected the peer directly as `COM4`. Direct-flashed the updated
`net_bench` peer image (`--role peer --channel 11 --maint-ap --chem lfp --cap 6000
--charge-ma 1500 --hb-hz 1`) with `--maintain 4.6` instead of the prior 5.2 V default.
Flash succeeded and verified on MAC `d8:85:ac:9e:5a:b8`.

Rationale: a 5 V USB power bank/USB source can be blocked or heavily current-limited when
the charger's input-regulation/maintain setpoint is above the source voltage. Keep the
boot default USB-recovery-safe (about 4.6 V) and raise VINDPM live with `m<v10>` only
during panel MPP testing, or implement a persisted setting with a USB/supply-voltage clamp.

## 2026-06-29 - Ben + Codex - Low-battery maintenance-AP OTA boundary test

Tried to push the current `net_bench` peer image to solar telemetry board `9E5AB8`
while it was intentionally deep in the low-battery region. Starting point before the
maintenance command: about 2.57 V, SOC 0 %, and ~0.45-0.50 W net load. The peer heard the
bridge's sustained `ENTER_MAINT` and stopped fresh ESP-NOW telemetry. It advertised the
expected `ResonanceMaint-9E5AB8` AP briefly enough for Windows to connect when given an
explicit profile, but the Codex laptop cannot stay reachable to the backend while its only
WiFi interface is joined to the peer AP.

After returning the laptop to BubbyNet, the bridge showed the useful boundary result:
the peer had brownout-reset, emitted only two fresh post-brownout heartbeats at about
2.33 V (`rr=brownout`, uptime ~3.3 s), and then went stale. This does **not** prove that
a full OTA upload cannot ever complete at low battery, because the single-WiFi laptop
constraint prevented the upload attempt. It does show that this starting point is below a
comfortable maintenance-entry floor for the always-awake bench image: AP startup /
maintenance transition alone was enough to hit a brownout-adjacent state. Retest the full
upload with external power or a second host network interface. The temporary Windows
maintenance-AP profile was deleted and BubbyNet auto-connect was restored.

## 2026-06-29 - Codex - Onboarding pass

Read the session-start orientation path (`README.md`, `LOG.md`, `TODO.md`,
`BACKGROUND.md`, `docs/block-diagram/SYSTEM.md`, ADRs 0001-0022, root `ROADMAP.md`,
and the PowerFeather/networking/Voltaic notes) before taking on new work. Current state:
PowerFeather V2 is the validated COTS/reference architecture; ESP-NOW, battery-only OTA
with rollback, watchdog recovery, rails-off sleep, and the solar charge path are green.
The active gates remain role-specific energy sizing, BQ25628E VBUS_OVP/HIZ guard,
Voltaic P105/P126 outdoor tests, HEX/RGBW type mix and placement, HEX 4.2 V boost,
mock-hat RF, sealed-hat thermal behavior, and production firmware hardening.

Noted existing uncommitted WIP adding runtime `net_bench` battery capacity and
charge-current config (`C<mah>` / `G<mA>`) plus peer timed sleep and dashboard support;
left that work intact.

## 2026-06-28 - Ben + Codex - Runtime battery capacity and charge-current config for net_bench

Added NVS-backed bench config to `firmware/net_bench`: `C<mah>` broadcasts a battery
capacity update to peers, persists it, and reboots them so `Board.init()` applies the new
MAX17260 gauge capacity; `G<mA>` broadcasts/persists the charger current cap and applies
it live. Heartbeats now carry `cap=`/`chg=` so the serial bridge/dashboard can verify the
peer's active config after OTA. Chemistry remains build-time because the charge-voltage
profile is safety-critical.

Updated the local power dashboard with capacity/charge controls and parser support. This
is primarily for swapping the 2 Ah bench LFP and the fullbattery.com 32700 6 Ah LFP during
solar-panel tests without rebuilding firmware for a simple constant.

## 2026-06-20 - Codex - Onboarding pass and PowerFeather SDK 2.1.1 review

Read the current repo orientation path (`README.md`, `LOG.md`, `TODO.md`,
`BACKGROUND.md`, `docs/block-diagram/SYSTEM.md`, and the active PowerFeather/LED/OTA
ADRs) before reviewing the PowerFeather-SDK 2.1.1 release. Current state remains:
PowerFeather V2 is the validated COTS/reference path; ESP-NOW, battery-only OTA +
rollback, watchdog recovery, and solar charge path are feasibility-green; active gates are
bottom-up role-specific energy sizing, BQ25628E VBUS_OVP/HIZ charger guard, Voltaic panel
tests, HEX/RGBW placement, boosted-HEX characterization, mock-hat RF, thermal, and
production firmware hardening.

PowerFeather-SDK 2.1.1 is a narrow MAX17260 time-estimate fix plus version bumps. The
MAX17260 driver now preserves raw `0xFFFF` for time-to-empty/time-to-full so
`Mainboard::getBatteryTimeLeft()` returns `Result::NotReady` instead of a bogus large
estimate. Resonance impact is limited to `firmware/power_bench/` telemetry
(`time_left_min`) and the older `powerfeather_demo_port` UI; `net_bench`, `led_studio`,
charger/VINDPM behavior, OTA, sleep, LED control, and mesh feasibility are not touched.
Recommendation: update bench machines from SDK 2.1.0 to 2.1.1 when convenient, but no
architecture or firmware changes are required.

## 2026-06-17 - Codex - Reconciled stale architecture docs before commit

Cleaned up stale overview context that still pointed at the early ESP32-C3/CN3058/AP2112K
and IS31-primary direction. Added ADR 0022 to record the LED fleet decision from the gobo
session: use both HEX and 4 W RGBW point-source modules by optical role, with type mix and
placement still open. Rewrote the canonical system architecture/power-budget doc around
PowerFeather V2, BQ25628E/MAX17260/TPS631013, direct-GPIO LEDs, role-specific panel sizing,
and the still-open energy/thermal/RF gates. Updated the hardware README, BOM skeleton,
roadmap, references, glossary, README status, and TODO entries to match the current state.

## 2026-06-17 - Codex - Onboarding pass

Read the repo orientation path (`README.md`, latest `LOG.md`, `TODO.md`, `BACKGROUND.md`,
`docs/block-diagram/SYSTEM.md`, key ADRs, current test notes, and bench/tool README files)
to re-establish the live state before taking on implementation work. Current mental model:
PowerFeather V2 remains the validated COTS/reference architecture; networking, solar path,
and battery-only OTA/rollback are feasibility-green; the active gates are panel/cell sizing,
LED role split and placement, VBUS_OVP/HIZ charger guard, mock-hat RF/thermal, and production
firmware hardening. Noted existing uncommitted work on Voltaic ETFE testing, PowerFeather
SOC cautions, net_bench docs, and the new serial-bridge dashboard; left that WIP untouched.

## 2026-06-15 - Ben + Codex - Travel maintenance AP committed; Voltaic ETFE panel prep captured

Remote travel bench update. `net_bench` now has a `--maint-ap` option for client-isolated
networks: the normal/parallel path remains shared WiFi maintenance mode, but a field peer
can now enter maintenance by advertising `ResonanceMaint-<nodeid>` and serving OTA at
`192.168.4.1`. The master serial-bridge path stays useful on USB for telemetry and the
field peer can remain pure ESP-NOW until maintenance is requested. Also widened live
`SET_MAINTAIN` (`m<v10>`) to the PowerFeather SDK range, 4.0-16.8 V, so high-Vmp panels
such as Voltaic P126 can be swept without a reflash.

Captured tomorrow's Voltaic ETFE test prep in
`docs/tests/VOLTAIC_ETFE_PANEL_TEST_PREP_2026-06-15.md`: P105 5 W and P126 2 W source specs,
derived size/weight/cost comparisons, P105-vs-P126 BOM read, and a concrete outdoor run
shape for the COM7 serial bridge plus INA-instrumented peer. Key warning for the run:
both panels have Voc above the BQ25628E default low input-OVP threshold, so a no-charge
result may be the known bright-sun input qualification latch until the VBUS_OVP/HIZ-kick
firmware item is handled.

## 2026-06-12 (cont. 3) -- Ben + Claude -- Interactivity/presence sensing: option space mapped (Elliot ask)

Elliot (project lead) saw the 06-11 LED demo and asked for presence detection /
interactivity -- "what makes people spend quality time at the tree." Full landscape in
`docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md`; headlines: it's ART not
security (false positives are benign -> ~80 % reliability = success); the product is the
MESH choreography (PRESENCE event + ripple, the packet layer already fits); primary
candidate = downward VL53L1X ToF eye (sway-robust, ~$3, ~zero power, but needs a port
next to the gobo aperture -- Steve); radar = through-enclosure (no dust exposure) but
LED-show-class power unless duty-cycled + self-sway artifacts (IMU veto); the FREE
experiment = mesh-RSSI presence (bodies attenuate 2.4 GHz ~20 dB, already in every
heartbeat). Bench kit ~$10, test plan Steve-compatible, TODOs queued.

## 2026-06-12 (cont. 2) -- Ben + Claude -- HEX 4.2 V boost direction (TPS63802); revised HEX budget; Steve-runnable bench TODO pushed

Remote session while Ben travels. Two outcomes, both queued as Steve-runnable TODOs
(Steve has duplicate components + Claude Code on his end; data site code `tn`):

**Revised HEX budget -- the gobo looks are cheap.** Ben's verdict: HEX looks best as
1 px white or 3 px single-channel (plus trails). Measured: 1 px full = 41.8 mA
(~0.12 W rail), 3 px ~105 mA (~0.3 W) -> ~0.4-0.6 W battery-side with overhead =
**all-night on the 3 W panel, in-tree**. Yesterday's 2.1 W HEX row was the all-37 case
only; in its actual role the HEX lantern runs as cheap as the RGBW one.

**4.2 V V+ boost (TPS63802) -- why and how.** At the sagged ~2.9 V rail the SK6812
blue/green drivers are in dropout (Vf 3.0-3.2 + ~0.5 V headroom needed) -> starved
channels = the goldening + ~25-30 % current deficit. A regulated 4.2 V V+ should give
**~+40-60 % white lumens** (blue/green recovery, V(lambda)-weighted), restore color
balance, and make looks **SOC- and fixture-invariant** (also quietly solves the
Community-Mandala brightness-normalization concern). Key constraints discovered:
- **4.2 V, NOT 5 V**: WS-data VIH = 0.7 x VDD; 5 V supply -> 3.5 V threshold breaks
  3.3 V GPIO data. 4.2 V -> 2.94 V = in spec, no level shifter.
- **TPS63802 module** (TI buck-boost, 1.3-5.5 Vin, 2 A, output-select solder jumpers):
  re-bridge 3V3->4V2 (fully open 3V3 first; meter unloaded). Cheap boards don't break
  out EN (tiny pad only) -> bench version feeds from the **switchable 3V3** (kill-switch
  inherited via GPIO4); the **VBAT-fed + EN-on-GPIO** single-conversion variant is the
  production architecture, to live on the NeoHEX adapter PCB rev. PS pad = power-save
  mode select; leave default (PFM efficiency matters at dim ambient).
- **Count-cap required on boosted builds**: all-37 white at regulated 4.2 V ~ 2 A out --
  far beyond module + rail. Firmware n-cap before anyone maxes "all".
- Rail-vs-pixel bottleneck clarified: at 1-3 px the limit is per-pixel undervolt (boost
  fixes); the converter/rail limit only binds at high counts.
- LM2596-class = buck, wrong direction; MT3608-class = acceptable fallback (pot-set,
  drift risk -- preset jumpers preferred for fleet).

Decision data wanted from the bench (Steve): PAR + INA lumens-per-system-watt, 2.9 vs
4.2 V, 1/3 px, per-channel, at two SOCs. TODO has the full procedure.

## 2026-06-12 (cont.) -- Claude -- BQ25628E datasheet read: the Voc ceiling is a REGISTER BIT; "6V" panel class unblocked

Datasheet (SLUSFA4C) electrical characteristics resolve yesterday's bright-sun latch and
the panel voltage window:
- **V_VBUS_OVP is selectable**: VBUS_OVP=0 (POR default) -> 6.1/6.4/6.7 V rising;
  VBUS_OVP=1 -> 18.2/18.5/18.8 V. Our connect-time Voc 6.15 V tripped a min-spec part at
  the default setting. Chip operating range is 3.9-18 V (26 V abs max) -- the ~6 V ceiling
  was configuration, not silicon.
- **Input qualification is EDGE-triggered** ("power up from input source" sequence runs at
  insertion): explains why shading to 4.7 V did NOT recover but full VBUS removal did.
  EN_HIZ toggle should synthesize a fresh edge (8.3.4.3) = the firmware re-qual kick.
- Other gates are non-issues for panels: poor-source test = >=3.6-3.75 V at <=10 mA;
  sleep-exit = VBUS > VBAT + 0.115-0.34 V; UVLO rising 3.2-3.5 V.
- Bonus confirmations: chip charging defaults VREG 4.2 V / ICHG 320 mA (the exact
  led_studio-uninitialized LFP hazard, now in writing); chip-level VINDPM floor is 3.8 V
  (the 4.6 floor is the SDK clamp); VINDPM_BAT_TRACK = VBAT+400 mV dynamic floor option.
- **Supersedes the "narrow viable window" panel conclusion from earlier tonight**: with
  VBUS_OVP=1 + the requal kick (now a procurement-prerequisite TODO), the spec is
  Vmp(STC) >= ~5.4 V, Voc <= ~16 V -- the standard 6 V class (incl. Voltaic's 6 V ETFE
  line) is fully in-window. Current was never a constraint (2 A charge ceiling; charger
  draws only what it needs).

## 2026-06-12 -- Ben + Claude -- Gobo verdict: BOTH LED types, by role; full-brightness budget sketch

**Gobo session result (Ben, inverted-lantern rig, dark):** both modules are excellent for
DIFFERENT roles -- the LED-axis answer is a MIXED FLEET, not a winner.
- **HEX (37x SK6812):** beautiful animations, dancing patterns, the color-channel
  separation (Split) modes shine -- but it reads best within ~6 ft; at 10-15 ft the color
  washes out and patterns lose crispness. The intimate/close-range module.
- **4 W RGBW point source:** crisp and beautiful even at 15 ft; the color fringing acts
  like a Venn diagram -- overlap regions mix into NEW colors, far richer than plain
  R/G/B edge fringing. The long-throw/gobo module.
- Direction: lanterns of both types. Feeds ADR 0018 (update it to record both-by-role
  and the placement question: which heights/positions get which module).

**Full-brightness budget sketch** (gamma off, bri 255; measured LED-rail draws + 0.2 W
assumed production overhead, /0.85 converter; harvest = derated effective-solar-hours
estimate pending the dawn-dusk log): HEX-full ~2.1 W battery-side; 4W-module RGB-full
~1.1 W; W-only ~0.45 W. Sustainable hours/night on the 3 W panel in-tree (unshaded):
HEX 1.8-3.0 (2.4-3.6); RGB 3.6-6.0 (4.8-7.3); W-only all night. 5 W panel scales x1.67:
HEX 3-5 h, RGB ~whole-night. The 32700 (18 Wh usable) banks 3-5 nights of sustainable
show -> single nights can splurge and repay. 5 W buys storm-recovery margin more than
capability. Caveats: shading factor dominant unknown; production overhead unmeasured;
HEX "full" is rail-limited (stiffer cell = brighter AND hungrier).

**Panel-shopping spec (from the BQ25628E limits + bench):** buck-only charger ->
panel hot loaded Vmp >= 4.6 V (= the SDK's VINDPM floor; sub-4.6 setpoints are silently
REJECTED -- which re-explains the 06-11 "4.4 V collapse": those points measured a stale
setpoint, not 4.4; LOG cont. 2/3's below-the-knee story is corrected accordingly, and the
4W-cam panel's "flat no-knee curve" below 4.6 was the same artifact). Voc ceiling: input
qualification failed at ~6.05-6.15 V and latched (the bright-sun gotcha), accepted 5.43 V
-> as-configured ceiling ~6 V-ish; datasheet ACOV verification is now PROCUREMENT-GATING
(if fixed ~6.3 V, the standard "6V" panel class (Voc 6.8-7.4) can never qualify at
open-circuit and no firmware kick saves it; viable window narrows to Voc(STC) <= ~5.8 =
the Seeed's class). Current is a non-issue (BQ = 2 A charge max; charger draws only what
it needs). Voltaic P139 (Voc 2.76) = boost-ecosystem class, unusable on a buck charger.

## 2026-06-11 (cont. 3) -- Ben + Claude -- 32700 VERDICT: 5726 mAh (95 % of rating) = the production cell passes; "4 W" camera panel = a 1 W panel in a trench coat

**32700 6 Ah LFP capacity (the production-cell gate): 5,726 mAh clean to a 2.473 V cutoff
over 7.16 h, ZERO resets.** Stitch: run 1 981 + ~6 (gap at ~124 mA idle after run 1's
parse crash) + run 2 4,739; **7 corrupt-but-parseable INA samples ablated** (-256 to
-343 A class; the raw script integral read 10.9 Ah -- reconcile-before-believing, again).
Cross-check: the gauge's own run-2 integral came out **+8.5 % above the clean INA** --
the MAX17260 current bias replicating for the **6th consecutive session** (+8 +-1 %,
both directions, both cell types; the /1.08 software correction is now very solid).
**Verdict: PASS at $5.10 ($0.89/delivered-Ah)** -- 95 % is ratings-tolerance territory on
a first cycle with a conservative cutoff. Qualify a 2nd sample from the batch before the
100-unit order (n=1), but this is the production cell unless that surprises. Notable:
under the fading HEX load the cell rode the whole tail gracefully (load self-dimmed
605 -> ~250 mA as the rail sagged) -- zero brownout resets, vs the mule's 44-reset
cascade under the stiffer RGBW point-source load.

**"4 W" ring-camera panel bake-off (Ben's economy-of-scale candidate): rejected, with
numbers.** Voc only ~5.45 V hot at the connector (10-cell panel + blocking diode -- Ben
visually confirmed diode-only in the housing). Flat-mounted: a dead-flat ~0.28 W from
VINDPM 4.6 down to 4.0 (current-source-starved, ~65 mA -- no knee at all). Tilted
square to the sun at 4.6 V: 0.579 W in ~57 klux -- scaled to full sun ~1.0-1.1 W real
capability = **~4x overrated**, plus bezel self-shading when flat (tilting doubled
output, more than geometry alone explains) and the diode tax. Apples-to-apples the
Seeed 3 W delivers ~4x the real harvest. The 10-minute sweep harness is now the
panel qualifier: any candidate (incl. the ETFE panels) earns its place through it.
(Sweep-tooling fixes from the session: anchor-all-zero ZeroDivision guard; "restore
5.5 V on exit" bit us twice when the next panel's window sat below 5.5 -- the live
check is `sgood=1` + `sma=0` = setpoint above the panel's window, send `m46`.)

Misc: lux-sensor bump mid-session produced a fake 30x light drop (the 3.5 klux
"shade" reading) -- worth a mount for the TSL. Sun-angle context for today's numbers:
3:40 pm flat-mount cosine loss ~18 % + cell ~65 deg C temp derate ~16 % fully explains
"2 W from a 3 W panel" -- the Seeed performs AT rating once physics is applied.
Tomorrow: cool-AM Seeed sweep (Vmp(T) -> MPPT decision), then a dawn-to-dusk harvest
log = measured effective-solar-hours (the fixture-specific derate of Ben's 5-h
heuristic; pre-derate estimate ~2-3 h flat, ~1.5-2.5 in-tree).

## 2026-06-11 (cont. 2) -- Ben + Claude -- FIRST WIRELESS MPP SWEEP: hot-panel optimum 4.6-4.7 V = ~3x the default harvest; bright-sun input-latch gotcha; 32700 verdict pending

**The harvest question (the sizing campaign's last unmeasured term) now has its hot-panel
answer.** Full instrument cluster on the outdoor peer -- TSL2591 (lux; saturated in full
sun, IR ch1 used as the normalization channel), SHT31 taped to the panel back
(60-61 deg C; IR gun front 155-157 deg F ~ 68 deg C -> ~8 deg C front-to-back offset, cell ~64-66 deg C),
and Ben's idea of SEN0291 INAs on the peer's OWN STEMMA bus (panel + battery leads,
heartbeat tail 3, fw 2026-06-11.2) -- zero outdoor tether, all data over ESP-NOW.

- **Curve (bright sun ~3:40-4:15 pm, panel ~60 deg C back):** 5.5 V -> 0.59 W; 5.2 -> 1.18;
  5.0 -> 1.45; 4.9 -> 1.55; 4.8 -> 1.66; 4.7 -> 1.69; **4.6 -> 1.73 W (best, both sweeps)**.
  The default 5.5 V harvests **31 % of optimum (3.2x available)** -- worse than the
  06-08 cloud-confounded ~2.6x hint. Knee-bracket re-sweep reproduced 4.6 as peak
  (anchor drift 5 % with the improved 4.9 anchor).
- **Panel INA vs BQ telemetry: the BQ under-reports harvest ~10 %** (1.91 W panel-side
  vs 1.73 BQ-side at the peak) -- input-stage loss the sizing math must include. The
  self-instrumentation cross-check earned its keep on day one.
- **Below the knee:** sweep 1's 4.4 V point COLLAPSED (0.55 W, input parked near Voc --
  VINDPM below the hot panel's knee has no stable operating point when stepped to from
  near-idle); the re-sweep's 4.4 read 1.53 W but DEMAND-LIMITED (4.9/4.5/4.4 all
  identically 1.53 W late-session -- battery filling toward ~50 % and/or charger thermal
  foldback in the heat caps demand, making VINDPM moot). Conservative rule: **fixed
  setpoint >= 4.6 V hot, approach setpoint changes from above; run sweeps on a hungry
  battery.** Cool-AM session pending for Vmp(T) -> the fixed-vs-temp-comp-vs-P&O call.
- **NEW FIELD GOTCHA (production-relevant): connect/boot under bright sun latches the
  charger's input fault** -- panel sat at Voc ~6.0-6.2 V, sgood=0, zero draw; a hand-
  shade to 4.7 V did NOT clear it; only full VBUS removal (face-down/unplug) re-ran
  qualification. Captured in POWERFEATHER_NOTES + firmware-guard TODO (playa bring-up
  hazard at 100-fixture scale). Anchor methodology fix: anchor at 4.9 not 5.5 (the 5.5
  point is load-noise-dominated; first sweep's 25 % "drift" was that artifact).

**32700 capacity run (in progress at write time):** three CORRUPT-BUT-PARSEABLE INA
samples (-256 to -343 A, physically impossible) inflated the live integral -- caught by
reconciling the integral against instantaneous currents; clean re-integration =
**~4.8 Ah by the knee region, final pending the fading tail**. (An earlier in-chat
"~7 Ah, above rating" read was glitched data -- retracted within the hour. The 06-10
lesson again: reconcile integrals before believing them.) All four INA host scripts now
drop both mangled lines AND beyond-range values (the INA's +-4 A) at ingest. Notable
along the way: the HEX load FADES gracefully as the rail sags (zero resets in 5.6 h,
vs the RGBW point source's 44-reset brownout cascade on the mule) -- the two LED
architectures fail differently at end-of-charge.

Also: 9F2690 reflashed as serial-bridge master; mpp_sweep gained ir-ch1 fallback
(TSL2591 saturates in full sun even at min gain -- expected); peer INA address
convention: both-DIP-off = 0x40 = panel, both-on = 0x45 = battery (0x44 = SHT31).

## 2026-06-11 (cont.) -- Ben + Claude -- Pushed the HEX to the cliff: visual failure sequence mapped; protect latch validated live; 32700 charging for capacity test

**The aggressive ramps (Ben watching, ~20 % SOC mule cell).** After the guarded runs,
floors were dropped near hardware limits and the value ramp walked 37-px white from
141 mA up. Results, all INA ground truth:
- **Sustained ceiling ~480 mA** (val 208) at ~20 % SOC -- far above the morning's
  conservative floors; rail rode at 2.7 V (min 2.53) for whole steps without electrical
  failure. The step toward ~500 mA (val 224) ended it: **the firmware battery-floor
  protect latched mid-step** (rail CUT to 0 V, LEDs unloaded, WiFi off, no self-rejoin --
  the designed endpoint, needing a button; the brownout-reset path self-recovers). Every
  guard layer fired in design order across the night: script floors first, fw protect as
  the backstop, zero bricks.
- **Hot-step vs ramp asymmetry, quantified:** an idle->290 mA hot-step (n=10 @ full)
  brownout-reset the board instantly (rr=poweron) at the same SOC where a gradual ramp
  survived 480 mA -- a ~1.7x margin difference. "Ramp gently / no full-white hot-steps"
  is now a measured production rule, not folklore. (The danger zone also slid ~100 mA
  down as SOC fell 98 %->20 % -- the current cap must be SOC/voltage-aware or worst-case.)
- **Visual failure sequence (Ben, thick packing foam as diffuser -- too bright naked-eye):
  (1) subtle flicker (onset before any electrical flag), (2) subtle "goldening" of white
  (blue channel -- highest Vf -- starves first as the rail sags), (3) uneven lighting where
  the brightest CONTIGUOUS run of pixels jumps around every few seconds** (WS-protocol
  data corruption: pixels keep whichever frame last latched cleanly), then (4) the
  protect cut. All graceful-degradation modes -- nothing alarming below the cliff, which
  supports dim-don't-die low-battery behavior.
- First aggressive attempt (file `0549`) also caught the n=10@255 hot-step brownout live
  (board uptime reset mid-ramp; script bug fixed: failed /set now prints + retries once).

**32700 6 Ah LFP candidate -- charging overnight.** Board 9F2690 (the former bridge
master) flashed `power_bench --led none --cap 6000 --chem lfp` BEFORE cell connect (the
LFP flash-order rule), then the 32700 attached: charging at **+515 mA** (USB input-
limited), bv 3.328, supply_good. Gauge says SOC 99 -- ignore (un-learned, cycles=0,
plateau-blind); the REAL capacity test is tomorrow's full charge -> INA-coulomb
discharge (the validated 06-10 methodology). At fullbattery.com bulk ~$5.10 (~$0.85/Ah)
it's the leading production cell if it makes rating (ADR 0017 direction). Note: this
board's Wire1 scan shows an extra mystery device at 0x2A (others have only 0x36/0x6A) --
harmless so far, noted in case the board ever behaves oddly.

**Bench state for tomorrow's MPP sweep:** mule 2000 cell DISCONNECTED at ~20 % SOC
(ideal bulk-charge precondition); 9E5B0C powered off; a spare board still needs the
serial-bridge master flash (9F2690 got the new master fw tonight but was immediately
repurposed for the 32700); TSL2591 + SHT31 arrive early PM. The 32700 discharge can run
on the bench INAs in parallel with the outdoor sweep -- load/wiring decided in the
morning (HEX board swap vs RGBW on 9F2690).

## 2026-06-11 -- Ben + Claude -- Loose-ends night: RGB-3W = RGBW-4W on RGB; STEMMA cable verdict; HEX ground truth + the "all-on-max" instability explained

Bench session while waiting on the TSL2591/SHT31 delivery. Three loose ends closed.

**1. RGB-3W vs RGBW-4W: identical RGB top-end.** The new 3 W RGB module (no W channel)
at full r=g=b drew **256.5 mA** (3 cycles, +-0.3 mA) vs the 4 W RGBW's RGB-full
**257.3 mA** at the same ~2.8 V sagged rail -- 0.3 % apart. The "3 W vs 4 W" rating
difference is entirely the W channel (+66.5 mA standalone, +34 mA on top of RGB under
combined sag). W pattern on the new LED: ~0 mA (no channel, and the 4-byte RGBW frame
drives a 3-byte pixel fine -- first three bytes land). Shunt NOT backwards (Ben's worry):
both INA channels kept the original sign convention. Caveats: n=1 of each module;
tonight USB+charging vs 06-10 battery (rail sag happened to match, making it fair).
Data: `2026-06-11-afk-sweep-0028.jsonl` + `-power/gauge.png`.

**2. Metro STEMMA port: the CABLE was the whole story (port healthy).** The Metro
ESP32-S3's QT port is the same I2C bus as the headers (SDA=47/SCL=48, 10 k pullups, no
power gate; 0x36 on the bus = the Metro's own onboard MAX17048). STEMMA QT (GND,VCC,
SDA,SCL) and Gravity PH2.0 (VCC,GND,SCL,SDA) are **pairwise-inverted**, so a
straight-through adapter lands ALL FOUR pins wrong -- power reversed (the 06-09
dead-short/USB-kill incident, explained) AND SDA/SCL swapped. Ben re-matched the leads
-> all 4 INAs + the MAX17048 found and streaming **through the QT port** (which also
proves the port survived the 06-09 short). `ina_monitor` gained `s` (I2C scan) / `r`
(re-probe) serial commands for future bus debugging.

**3. HEX (37x SK6812) with INA ground truth -- and the "all-on-max instability" is a
BATTERY-SAG ceiling, not an LED/data failure.** power_bench `/set` gained `n=` (light
first n pixels; fw 2026-06-11.1, OTA'd battery-only -- another no-touch flash) and
`ops/bench/hex_ramp.py` ramps count (1->37 @ full) then value (n=37, 16->255) with
host-side abort guards (gauge-V floor primary, INA means secondary, board-reset/HTTP
the real detectors), backing off BEFORE the board browns out:
- Single pixel (INA, battery-only): **41.8 mA full white** (17.3 @ 64, 23.0 @ 128,
  34.2 @ 192). Ground-truth replacement for the gauge-based HEX numbers.
- Count ramp @ 255: safe through **n=10 (288 mA)**; n=14 (372 mA) tripped the gauge
  floor (3.008 V). Value ramp @ n=37: safe through **val 64 (261 mA)**; val 96
  (358 mA) tripped (gauge 2.980 V). Convergent: **~350-400 mA of LED draw pulls the
  bench cell's terminal to ~3.0 V even at 98 % SOC** (LED + ~150 mA WiFi system ->
  ~0.5 A battery draw; matches the 06-10 finding that brownout cascades start
  ~2.97 V under load). Per-pixel current self-limits as the rail sags (41.8 -> ~27
  mA/px at n=14), so all-37-full would NOT hit 37x41.8 -- but the cell dives first.
- Implications: (a) Ben's observed all-on-max instability = battery sag to the
  brownout zone; rail/data stayed fine to the guard floors (rail mean >= 2.79 V).
  (b) The ceiling is a CELL property (high effective IR incl. harness, ~0.7 ohm at
  0.5 A) -- the production 32700 ~6 Ah cell lifts it substantially. (c) Production
  firmware needs a **current cap** (brightness x lit-count) for burst modes --
  reinforces the existing cap-brightness TODO / ADR 0013 failsafe. We deliberately
  never drove it to an actual reset; the guards stop at early-warning floors, and the
  visual-flicker threshold (if lower) is a separate observation.
- Gauge current bias replicated again: **+7.4 %** this session (and +8.8 % on the
  CHARGE side in the morning run) -> ~+8 +-1 % across 4 sessions, both directions;
  the /1.08-ish software correction is solid.
Data: `2026-06-11-afk-sweep-0119.jsonl` (+pngs), `2026-06-11-hex-ramp-0128.jsonl`
(0126 = aborted first try whose floors were miscalibrated to transient WiFi dips --
kept for the record).

## 2026-06-10 (cont. 2) -- Ben + Claude -- MPP sweep goes fully wireless: TSL2591 lux + SHT31 panel-temp ride the heartbeat

Ben flagged the sweep's weak point: the Apogee PAR sensor is USB-tethered, so logging
light outdoors meant a laptop or a dedicated rpi at the panel. Three options weighed:

- **PowerFeather USB-C as USB-host to the Apogee: rejected.** The S3 silicon can do OTG
  host, but this stack runs TinyUSB in device mode, the Apogee is an FTDI-class device
  (needs a vendor VCP host driver under ESP-IDF, not Arduino), and the V2's USB-C is a
  charge/device input that doesn't source VBUS -- the sensor wouldn't even power up. A
  research detour, not a bench fix.
- **TSL2591 I2C lux module (arriving ~06-11): ADOPTED as the primary light channel.**
  Chained on the peer's STEMMA-QT, auto-probed at boot, lux appended to the heartbeat
  (append-only tail 2, `NB_PROTO_VER` unchanged -- same pattern as the supply fields) ->
  light data arrives over ESP-NOW with **zero outdoor tether**. Note "lux vs PAR": neither
  matches the panel's silicon spectral response -- both are *relative* normalization
  channels, which is all the sweep needs (absolute W comes from the anchors agreeing).
  The TSL2591's raw ch0/ch1 (full+IR) are logged too. **Saturation caveat:** full sun
  (~100k+ lux) can exceed its range even at min gain/integration; firmware detects and
  reports `lux=sat`; the fix is a paper/PTFE diffuser (fine for relative use). The Apogee
  remains an optional host-side cross-check for the indoor dry run (`--par-port`).
- **SHT31-D taped to the panel BACK: ADOPTED for continuous panel temp** (back-surface
  contact ~ cell temp - a couple deg C in sun; standard PV practice) -> `ptc=`/`prh=` in the
  heartbeat. The IR gun stays as the front-surface spot-check at anchors (the script
  still prompts). **Battery NTC** (the V2's 103AT thermistor on the charger TS pin) is
  exposed too (`btc=`) but **opt-in** (`--batt-ntc`): enabling TS with no thermistor
  attached makes the BQ apply JEITA to a floating pin and can SUSPEND CHARGING -- gotcha
  captured in POWERFEATHER_NOTES. With the NTC taped to the cell it doubles as hardware
  LFP charge-temp protection (a thermal-track freebie).

Implementation (compiled both roles; on-hardware validation when the sensors arrive):
`net_bench` fw 2026-06-10.1 -- env auto-probe + 1 Hz cache (TSL2591 read blocks ~120 ms,
so high-rate heartbeats reuse the cache), heartbeat tail 2, master bridge prints
`lux=/ch0=/ch1=/ptc=/prh=/btc=`; `net_bench_log.py` + `mpp_sweep.py` + `mpp_analyze.py`
parse them (host tooling re-validated end-to-end against a simulated master emitting the
new tokens). Sweep flags generalized: `light-saturated`, `light-unstable`, `no-light`.

## 2026-06-10 (cont.) -- Ben + Claude -- MPP-sweep tooling ready (next bench test); buck-boost show-load finding from existing data

**Decision: the next bench test is the clean full-sun MPP sweep** (the open TODO from
06-08 cont. 10). Rationale: with capacity, idle, and LED draw now measured, harvest is the
last unmeasured term in the battery/panel sizing equation -- and the dirty 06-08 sweep
suggests the default VINDPM 5.5 V may give up ~2x vs the hot-panel MPP (~4.85 V), i.e. a
potential ~2x panel-sizing error at 100 units, plus it settles the MPPT firmware decision.
Runner-up was the gobo session (evening-compatible, doesn't compete for sun).

**Tooling built + validated (no hardware in the loop yet):**
- net_bench master `m<v10>` -- explicit SET_MAINTAIN setpoint (e.g. `m48` -> 4.8 V) next to
  the bare-`m` cycle; range-checked to the peer's 40-58 accept window. Compiles; reflash
  the DESK master over USB -- the outdoor peer needs nothing.
- `ops/bench/mpp_sweep.py` -- guided session: anchor (5.5 V) re-visited every 3 points so
  light/temp drift is measured rather than silently corrupting the curve (the 06-08
  lesson); 3x re-send of the unacked SET_MAINTAIN broadcast; Apogee PAR sampled each
  heartbeat + IR-temp prompts; dark-panel + PAR-instability flags with a redo offer;
  restores 5.5 V on exit; relays nb-* to UDP so net_bench_log co-records. Validated
  end-to-end against a pty-simulated master (recovered a synthetic IV peak at 4.8 V).
- `ops/bench/mpp_analyze.py` -- PAR-normalized P-vs-VINDPM per session, anchor-drift
  report, best-setpoint + "what fixed 5.5 V gives up" ratio, Vmp shift cool-vs-hot.
Procedure (also in TODO): SOC <~60 % first (charger must stay in bulk/CC), indoor
window dry run, then cool-AM + hot-midday sessions on a stable-sun day.

**Buck-boost finding from EXISTING data** (`ops/bench/bb_efficiency.py` on the 06-10
full-discharge JSONL; closes part of the "efficiency vs VBAT" TODO without bench time):
at full-RGBW show load the LFP **terminal** voltage sags to ~2.9-3.05 V, so the TPS631013
ran in **boost for the entire pre-brownout discharge -- the 3.25-3.35 V buck/boost
crossover was never visited under load.** Overhead (ESP+WiFi+converter, not separable
with this instrumentation) ~0.48-0.52 W and roughly flat; P_led/P_batt lower bound
0.61-0.64. Reframes the chemistry-tax concern: no crossover/mode-hunt tax at show loads;
the residual open regime is the production **ambient** load (tens of mA), where the
plateau terminal V (~3.2-3.3 V) does sit near the crossover. Caveats: n=1 cell/board/
load; fine structure vs VBAT may be time-confounded (WiFi activity); plot
`data/ca/2026-06-10-discharge-1357-bb-eff.png`.

## 2026-06-10 -- Ben + Claude -- Full discharge: bench LFP is AT/ABOVE its 2000 mAh rating (capacity vindicated); gauge learn cycle + brownout failure mode

**Capacity, finally measured (gauge-independent).** A full charge->empty discharge on battery
(`afk_discharge.py`, full-RGBW ~467 mA load, INA 0x45 coulomb integration) delivered **~2077 mAh
to a 2.5 V cutoff over 280 min, SOC 98->0 %** -> ~ **2119 mAh** at 100 %. The bench "2000 mAh" 18650
LFP is **at/above its rating** -- every earlier under-capacity claim (the 06-09 "~760 mAh" slice,
the older "~1000 mAh / 2x overrated") is **dead.** Ben's skepticism + the reputable-dealer prior
were right; the low numbers were entirely the un-learned, plateau-fooled gauge + my slice
extrapolation. (Production targets a different cell -- LFP 32700 ~6000 mAh -- so this is methodology
validation, not a product sizing number.)
(Data note: one spurious INA-0x45 sample -- a -21 A I2C/serial glitch at 138 min -- had inflated the
logged integral to 2144 mAh; `afk_analyze` now ablates it + re-integrates -> 2077 mAh. LED & gauge
were normal at that instant, so it was a lone read glitch, not a real transient.)

- **Usable under full LED load: ~1971 mAh** before the first brownout (first reset, bv 2.97; LED
  held full to ~2045 mAh, bv 2.80). The brownout cascade is confined to the last ~100 mAh.
- **Gauge vs INA (this run IS the learn cycle):** current bias **+8.3 %** high (median, |INA|>50 mA);
  coulomb **+7.9 %** (gauge 2241 vs clean INA 2077 mAh) -- now consistent with the instantaneous
  bias (the glitch had masked it at +4.5 %). Gauge SOC hit 0 % at ~1977 mAh with ~100 mAh (~5 %)
  still left -- mildly pessimistic at the tail but respectable for an un-learned LFP gauge.
  **DesignCap 2000 is ~correct** (measured ~2119) -- the SOC flakiness was UN-LEARNED gauge, NOT a
  wrong DesignCap (retracting the 06-09 "set DesignCap ~760"). This discharge + the recharge = a
  full learn cycle; re-check SOC accuracy on the NEXT cycle.
- **Gauge SOC shape (Ben's read):** SOC held at **1 % across the whole voltage knee** (where
  dV/dQ steepens), and the **1 %->0 % step coincided almost exactly with the brownout onset** -- a
  usable "really empty now" signal even though the flat plateau hides SOC elsewhere.
- **Failure mode (intended, aggressive):** 44 brownout-reboots in the deep knee -- under the
  ~467 mA full-RGBW load, once the cell sagged below ~2.97 V the board couldn't hold ESP+LED+WiFi
  -> reboot cascade (draw fell to ~145 mA). **LEDs went unstable ~2.7 V but the board kept running
  to ~2.5 V.** Bounded by the `--batt-floor 2.3` build + the script's 2.5 V cutoff; recovered fine
  on USB (charger precharge/trickle at 2.56 V). **Production lesson: set the low-battery cutoff
  well ABOVE the heavy-load brownout point.**

Tooling: `afk_discharge.py` (fixed-load coulomb run, reset-tolerant, waits-for-unplug),
`build.sh --batt-floor`, `afk_analyze.py` (constant-load runs + robust median gauge bias + glitch
ablation/re-integration). Plot: `ops/bench/data/ca/2026-06-10-discharge-1357-gauge.png`.

## 2026-06-09 (cont.) -- Ben + Claude -- SEN0291 wattmeter read 10x low (0.1 vs 0.01 ohm shunt); fixed, cross-checked, AFK gauge-cal sweep launched

**The "400 mA (power_bench) vs 36 mA (wattmeter)" mystery was a units bug, not a measurement
conflict.** Same current, 10x apart: `ina_monitor` computed `ma = shunt_mv / INA_RSHUNT_OHMS`
with `INA_RSHUNT_OHMS = 0.1` (the INA219 *reference* shunt), but the **DFRobot SEN0291 hardware
shunt is 10 mohm (0.01)**. Every current it ever printed was **10x low**. The gauge was right.

**Evidence (convergent):**
- Datasheet: SEN0291 = "10 mohm alloy shunt", +/-8 A, **1 mA resolution** -- and 1 mA = INA219's
  10 uV LSB / 0.01 ohm. The resolution spec only closes at 0.01 ohm, not 0.1.
- Live reconcile (`ops/bench/reconcile_ina_pf.py`, W-full): INA reported 6.7 mA -> x10 = 67 mA;
  PF battery-current delta = 81 mA -> ratio **~12x** (datasheet says 10; excess = WiFi-TX bursts
  in the gauge average + sagging rail + a noisier INA on the sag). Order of magnitude confirmed.
- Battery cross-check (INA 0x45 = battery line vs gauge): off -121 vs -138 mA; RGBW-full
  -461 vs -502 mA.
- `SYSTEM.md` already had RGBW at 400-500 mA; and this day's own puzzling "wake ~ 11 mA ... far
  under the 168 mA RX" -> **x10 = ~110 mA**, resolved.

**Fix:** `ina_monitor.ino` -> `INA_RSHUNT_OHMS 0.01` (Metro reflashed). **All prior INA numbers/
plots x10** -- incl. the "11 mA/0.6 s wake" (-> ~110 mA) and the led-ina-sweep PNGs (regenerated
x10). Sub-mA sleep floor stands (still below range; relabel only). At 0.01 ohm, PG=/1 = +/-4 A range,
1 mA/LSB (the old "caps ~400 mA" comment was the 0.1 ohm artifact). Raw `shunt_mv` was always
logged, so historical JSONL is recoverable by x10 without rewriting it.

**Tooling for the AFK gauge-cal run:**
- power_bench gained `/set?r&g&b&w&bri&gamma` (arbitrary single-pixel drive, per-channel gamma)
  + an unattended **battery-floor guard** (on battery, sustained <2.90 V -> cut the 3V3 LED rail +
  WiFi; non-bricking, reset/USB recovers). Reflashed via USB. (Gotcha: the post-flash RTS reset
  left the 3V3 rail off -- needed a physical reset, per POWERFEATHER_NOTES.)
- `ops/bench/afk_sweep.py`: loops {RGB,W,RGBW}x{gamma 0,1}xlevels logging INA 0x41 (LED), INA
  0x45 (battery) and gauge telemetry per point, with a **coulomb-budget cutoff** (sag-immune; a
  voltage floor false-trips -- the cell sags to 2.99 V at 460 mA even at 33 % SOC). Launched
  battery-only, 200 mAh budget.

**Run results** (`afk_sweep.py` -> `afk_analyze.py`; 814 pts, 54.8 min, 13 cycles, battery
33%->9% SOC; stopped on the 200 mAh coulomb budget; plots `*-power.png` / `*-gauge.png`):
- Corrected LED draw: W-full ~63 mA, RGB-full ~250 mA, RGBW-full ~290 mA. Full-scale is
  RAIL-SAG-limited: the LED bus droops to ~2.84 V under load (on USB *and* battery alike) so the
  SK6812 channels lose headroom; current was flat-to-slightly-rising over the run (254->259 mA RGB)
  i.e. NOT SOC-limited here. Gamma cleanly separates the mid-range (RGBW lvl 64: 70->9 mA).
- **Gauge current bias** (n=814): gauge = 1.080*INA + 2.4 mA, mean ratio 1.094 -> reads **~+9 %
  high** vs INA ground truth -> software-correct gauge current by x0.926 (or trim the MAX17260
  sense-R). Instantaneous gauge battery_ma is noisy/laggy; INA 0x45 is steady.
- **Coulomb**: gauge integrated 200 mAh vs INA 183 mAh over the run (gauge +9 %, matching the
  current bias).
- **Capacity: NOT determined -- the earlier "~760 mAh" was an overreach (Ben pushed back, rightly).**
  Gauge SOC fell 33->9 % over 183 mAh (INA), but the **resting voltage stayed flat at 3.190->3.186 V**
  (LED-off, ~120 mA) the whole run -- we never reached the LFP knee. So we have NO read on remaining
  capacity: 183 mAh could be ~24 % of a small (~760 mAh) cell OR ~9 % of the rated 2000 mAh with a
  gauge that over-drops SOC on the flat plateau -- a mid-plateau slice can't distinguish them, and
  the un-learned LFP gauge (cycles=0) can't be trusted to either. Cell is BatterySpace (reputable,
  rated 2000); no basis to call it bad, and the user's larger cells aren't testable yet (no holder).
  Plausible too (Ben): a freshly-charged gauge may pin SOC near 100 % before dropping, so DeltaSOC over
  a slice misrepresents charge. **Resolve with a clean full-charge -> full-discharge INA-coulomb run**
  (now possible -- charging re-enabled); leave DesignCap at the 2000 rating until then. The earlier
  README/SYSTEM "~1000 mAh, 2x overrated" is likewise unverified.

Caveats: the +9 % gauge current-bias fit is tight (n=814) but single-session. Post-run the cell
idled ~120 mA; I cut it to ~66 mA via WiFi-off (`q`). Follow-up (this session) adds a recoverable
timer-wake deep-sleep-on-floor + charge-enabled recovery so an unattended low cell can't be stranded.

## 2026-06-09 -- Ben + Claude -- Rails-cut idle win; 4-channel INA219 monitor built; ground-truth shows idle is tiny (gauge over-read it)

**Rails-off A/B (the sleep-current fix).** Hypothesis from cont. 10/11: the ~20 %/night
sleep-cycle drain was the two switchable 3V3 rails left on during deep sleep, not the wakes.
Added `Board.enable3V3(false) + enableVSQT(false)` before `esp_deep_sleep_start()`
(`net-bench-2026-06-09.1`) and ran a battery-only A/B vs the rails-on overnight baseline:
**rails-on ~1.7 %/h -> rails-off ~0.5 %/h, a ~3-4x cut** (~ 20 %/night -> ~ 5 %/night). The
rails were the dominant idle draw, as hypothesized. (Ratio is robust; the gauge only moved
~1 SOC count in 2 h, so the absolute is coarse -- see below.) **Captured as a gotcha in
POWERFEATHER_NOTES** so we don't relearn it. V2 keeps the gauge alive with VSQT off (separate
power-mgmt I2C), so telemetry survives the rail-cut.

**Built a 4-channel ground-truth power monitor** (`firmware/ina_monitor/`): Adafruit Metro
ESP32-S3 reading 4x DFRobot SEN0291 (INA219) at 0x40/41/44/45, separate-monitor topology
(reads a board-under-test's current through its deep sleep -- the thing the on-board gauge
can't). Direct register reads (bus V + raw shunt mV -> current; calibration-independent),
streams `ina ...` lines. Saga worth noting: (1) a STEMMA<->Gravity cable that **swapped
VCC/GND** dead-shorted + briefly killed USB on the Metro -- *all four INA boards survived* the
reversal; (2) Metro defaults to USB-OTG/TinyUSB which re-enumerates on sketch start (no
serial) -- flash with `USBMode=hwcdc,CDCOnBoot=cdc` like the PowerFeathers; (3) the hub works
direct-wired to the Metro's SDA=47/SCL=48 headers (bypass the bad cable). A reverse-polarity
JST also scared us but the PowerFeather + INA both survived.

**First ground-truth measurement (INA in the peer's battery lead).** Caught the ~30 s wake
as a current bump: **wake ~ 11 mA for ~0.6 s, sleep ~ 0** (below the ~0.2 mA PGA floor). So
the duty-cycled **battery** drain is **sub-mA** -- far below the gauge A/B's ~0.5 %/h (~4-5
mA). Reconciliation (vindicates Ben's gauge-distrust): the gauge's ~0.5 %/h was within its
own 1-count noise on the flat LFP plateau; the real drain was simply too small for it to
resolve, and the INA finally does. **Idle is negligible -- now ground-truth, not inferred.**
Caveats: 10 Hz may undersample a brief (<100 ms) radio-init spike (40 mV bus sag hints at
one) -> a fast-sample capture is the next step to nail per-wake energy; sub-0.2 mA sleep is
below this PGA range (sharpen by dropping the range); the ~11 mA wake being far under the
~168 mA always-on RX wants understanding (likely boot/init-dominated, not full RX).

**Walked back the LFP capacity claim** (Ben was right): cont. 11's "~1000 mAh / overrated 2x"
was too strong. The 06-03 drain delivered >=617 mAh but stopped *mid-plateau* (not empty), on
an un-learned gauge -> true capacity is unknown, likely a normal ~1-1.5 Ah 18650 LFP. Needs a
clean full->empty coulomb run on a learned gauge or external meter. Softened in README /
SYSTEM.md.

## 2026-06-08 (cont. 11) -- Ben + Claude -- Drawdown aborted (redundant); LFP capacity looks ~half rated; sleep-cycle idle budget negligible

Ben flagged the running always-on LFP drawdown as redundant -- correct. The 2026-06-03
overnight reboot-loop drain (`...is31-loadgen-overnight.jsonl`) already has the LFP
discharge curve at a similar load (mean -145 mA): SOC 92->30 %, **flat ~3.25 V throughout**
(min 3.234), 4.25 h -- the "LFP plateau -> V-SOC useless -> coulomb-count" lesson. Aborted the
new run; switched the board to the sleep-cycle test instead.

**Capacity finding (from the existing 06-03 data):** it integrates to **~617 mAh delivered
for a 62 % SOC drop -> real usable capacity ~ ~1000 mAh, not the 2000 mAh rating.** The
"2000 mAh" 18650 LFP looks **overrated ~2x** (physically, 18650 LFP are ~1000-1500 mAh;
2000+ is Li-ion-class). This **~halves the assumed battery budget.** Caveat: LFP gauge SOC is
shaky on the plateau -- **confirm with a clean full->empty coulomb-counted run** (USB top-up
first). Partly answers the "compare LFP sample vs rated capacity" TODO.

**Sleep-cycle duty-cycled average (computed):** sleep-cycle validated on hw (lean wake
~250 ms to HB + ~400 ms maint-listen ~ **0.65 s radio-on per 30 s cycle**). The MAX17260
can't catch the sub-second wake spike (reads ~0 mA), so computed from trusted pieces:
avg ~ (0.65 s / 30 s) x 168 mA (the always-on radio draw) + sleep floor ~ **~4 mA at a 30 s
wake interval** (~2 mA @ 60 s, ~1 mA @ 300 s). **Takeaway: the idle/sleep budget is
negligible** (~48 mAh/night @30 s on a ~1 Ah cell ~ a few %); **sizing is LED-show- and
harvest-bound, not idle-bound.** Caveats: active current is the separately-measured always-on
figure, sleep floor is estimated -- a precise per-wake/sleep number needs an **external
ammeter (SEN0291 / multimeter)**; the gauge fundamentally under-samples brief-pulse loads.
**Field concern:** that under-sampling means a sleeping fixture's gauge SOC can read
optimistically high -> low-battery logic must cross-check **voltage** (reinforces existing
TODO). Sleep-cycle left running overnight battery-only as a gauge-vs-pulse cross-check.

## 2026-06-08 (cont. 10) -- Ben + Claude -- Solar/sizing session: sleep-cycle + OTA-wake, idle floor, MPP sweep (cloud-caveated), drawdown started

Long bench session toward battery/panel sizing. New firmware `net-bench-2026-06-08.9`
(all validated on hardware via OTA) + several findings.

**Firmware:**
- **Sleep-cycle** (`--sleep-cycle --sleep-s N`): deep-sleep duty cycle (wake -> telemetry
  heartbeat -> brief maint-listen -> deep-sleep). Validated: `rr=deepsleep`, ~32 s cycle.
  Trimmed the USB-CDC `delay(1500)` on deep-sleep wakes so the wake is lean.
- **`U` sustained ENTER_MAINT** (~35 s): no-touch OTA-recovery of a **sleeping** board --
  the normal `u` burst misses a board awake only ~400 ms/30 s. Validated: a deep-sleeping
  peer caught it on a wake window and joined WiFi for OTA. The field **fleet wake-for-
  maintenance** primitive.
- **`SET_MAINTAIN`** (master `m`): runtime VINDPM/charger-maintain set over ESP-NOW (no
  reflash) -- the MPP-sweep actuator + future P&O MPPT primitive.

**Idle-load floor (battery-only, clean):** an always-on ESP-NOW peer draws **~168 mA /
~0.55 W**, and killing the WiFi scanning barely moved it -- the load is **radio-RX-
dominated**, not scanning. 168 mA flattens a 2 Ah cell in ~12 h, so **always-on is
unsustainable on battery -> deep-sleep duty-cycling is mandatory** (quantifies the "be
quiet during sunshine" instinct).

**Harvest (full sun):** Seeed 3 W panel, flat at ~2 pm, **lux 127 k (~1000 W/m^2 = full
sun)**, panel **150 deg F / ~65 deg C** (IR; glass epsilon~0.9, so true temp ~equal or a hair higher).
Measured **~1.0-1.2 W** at the default VINDPM 5.5 V (SOC 34-58 %, bulk-charging). 3 W is
STC; heat (-15-18 % + Vmp droop) + flat angle explain <3 W -- but see MPP.

**MPP sweep -- finding + caveat:** swept VINDPM 5.5->4.4 V. **Peak power at ~4.85 V -- matches
the hot-panel Vmp prediction; 5.5 V is well past the IV knee** (power craters above ~5.0 V).
BUT a **cloud rolled in mid-sweep (127 k->37 k lux)** with the panel temp drifting, so the
absolute watts (0.14-0.37 W) and the apparent 2.6x are **NOT a clean full-sun number** (the
start/end 5.5 V points disagreed, 0.138 vs 0.215 W = intra-sweep drift). **Robust:** MPP
~ 4.85 V hot, fixed 5.5 V is wrong when hot. **TBD:** the actual full-sun gain (no full-sun
MPP point captured). **MPPT verdict: green-light to MEASURE properly (clean full-sun sweep
+ simultaneous lux/IR-temp at 2 panel temps), not yet to commit it's worth ~2x.**

**Drawdown (started, cloudy evening):** brought inside, panel disconnected, always-on
~157 mA battery-only discharge from ~SOC 60-76 % (gauge jumpy on the LFP plateau -- trust
the coulomb count). `--autosleep` deep-sleeps at brownout to protect the cell. Logging
overnight -> LFP discharge curve, gauge accuracy, delivered capacity, cutoff voltage
(`ops/bench/data/ca/` + `/tmp/nb_drawdown_raw.log`; results next session). NOTE: this used
the always-on load; the **sleep-cycle duty-cycled average** (the low overnight budget
number) is still un-measured.

## 2026-06-08 (cont. 9) -- Ben + Claude -- Conclusions: WiFi hypothesis settled (moving-board artifact) + stress-test framing

Wrap-up of the day's two device tests.

**WiFi drop -- hypothesis settled (high confidence).** The board latches to one Eero BSSID
at association and **does not auto-roam** (ESP32 has no 802.11k/v/r); carried from indoors
to the yard, it clings to the now-weak indoor node instead of hopping to the strong (-46
dBm) nearer one -> the link collapses while a good AP sits right there (the scan is the
smoking gun). Fix is cheap and already partly in place: **a reset, a software reset, or a
firmware "re-associate on link loss" guard** forces a fresh scan-and-associate, which
picks the strongest beacon (our maintenance-OTA path already does a fresh `WiFi.begin()`,
which is why OTA worked from the bad spot). **Framing (Ben):** this is a **bench artifact
of a *moving* board** -- deployed fixtures are stationary and won't walk away from their
Eero, so we're unlikely to hit this in the field. Logged as a **gotcha** (see
POWERFEATHER_NOTES) + a firmware-guard TODO, not a blocker.

**Panel 0.12 V -> 5.55 V swing -- explained:** Ben **reseated the solar connector** mid-check;
that's the swing, not a mystery intermittent. Takeaway for production: **mechanically
secure/strain-relieve the panel pigtail** (a loose connector = silent zero-harvest), and
item (a) now makes a dark panel obvious live (`supply_good=0`, `supply_v~0`).

**Stress-test framing (important for reading the numbers):** this run **highly activated the
radio (continuous ESP-NOW + 15 s all-channel WiFi scans) WHILE harvesting** -- a deliberate
worst case. Even so the cell net-charged in decent light. **In the field the fixture will
be asleep / quiet during sunshine**, so real harvest-vs-load is *more favorable* than these
bench numbers -- i.e. the bench load figures are conservative, not representative. Next
focus: a **sizing-oriented** solar run (realistic sleep/duty-cycle load, harvest across
sun/cloud/shade) to actually spec the cell + panel.

## 2026-06-08 (cont. 8) -- Ben + Claude -- Item (a): supply/panel telemetry over ESP-NOW -- built + VALIDATED on hardware

Built the solar-telemetry half of the plan (item (a)): carry the **supply (panel) side**
over ESP-NOW so it logs from anywhere without WiFi-STA. Threaded `supply_mv`/`supply_ma`/
`supply_good` end-to-end -- peer reads `Board.getSupplyVoltage/Current/checkSupplyGood`
(cached ~1 Hz in `readBattery`), **appended** to `NbHeartbeat` (kept `NB_PROTO_VER=1`;
append-only + length-checks -> no flag-day, a pre-supply master still reads the battery
fields of a supply-capable peer; new master reads old peer via `offsetof` guard), stored
in `NbPeerStat`, emitted as `sv=/sma=/sgood=` on the `nb-peer` bridge line. Host
`net_bench_log.py` parses them (optional regex group) and derives `supply_w` (panel
harvest), `battery_w`, and `load_w = supply_w - battery_w` into the JSONL. fw
`net-bench-2026-06-08.7`.

Deployed via the maintenance round-trip (master `u` -> peer rejoined WiFi -> OTA `.7`
supply build -> reflash master over USB). **Works end-to-end:** `sv=5.56 sma=160 sgood=1`
-> **panel ~0.89 W**, battery flips to **net-charging +140 mA** under the (heavy) scan
load; harvest swings 0.5-0.9 W with the clouds, all logged.

**Solved the earlier "net-discharge at noon" puzzle:** while the peer was briefly in
maintenance mode its `/telemetry` showed **`supply_v=0.123` -- the panel was essentially
dark** (shaded/mis-oriented in-hand, or a loose connector). So the discharge was simply
**zero harvest**, not a battery/load problem. Once the panel saw light again, `supply_v`
jumped to 5.55 V and it charged. Lesson: **harvest is very orientation-sensitive** -- a
real sizing finding, and exactly the thing item (a) now makes continuously visible.

**Caveat for sizing:** the derived `load_w ~ 0.39 W` here is the *diagnostic firmware's*
load (radio always on + 15 s WiFi scans), NOT a fixture budget -- don't size the cell to
it. The **panel-harvest V/I is the directly-useful output**; the load side still needs
the bottom-up fixture duty-cycle budget (existing TODO). Boards left running on ch 11,
logging to `ops/bench/data/ca/2026-06-08-ca-lfp-2000-net-master-multicast-rNA-1946.jsonl`.

## 2026-06-08 (cont. 7) -- Ben + Claude -- WiFi coverage diagnostic VALIDATED on hardware (2 boards, OTA) + PDR seq-bug fixed

Took (cont. 6)'s firmware to hardware. Flashed the **serial-bridge master** (`9F2690`)
over USB on ACM1 (`--serial-bridge --no-charge`, ch 11) -- boots into "SERIAL BRIDGE (no
WiFi)" and streams `nb-*` to USB as designed. Then **OTA'd the scan-report peer onto the
live solar board** `9E5B0C` (the only wireless Resonance board -- found by sweeping the
LAN for `/telemetry`; note `192.168.4.73` is an unrelated "Grow Light", NOT ours, left
untouched). Built the OTA with **`--chem lfp --cap 2000 --maintain 5.5`** to match the
board's LFP cell + solar panel (Li-ion profile would overcharge the LFP -- the
POWERFEATHER_NOTES gotcha).

**Worked end-to-end, first try.** Post-OTA the peer left WiFi, rejoined as an ESP-NOW
peer (`rr=software`, LFP 3.33 V, still solar-charging ~40 mA), and streamed the **2.4 GHz
coverage map to the desk with zero WiFi-STA on the field board** -- resolving the **3
BubbyNet Eero nodes separately by RSSI** (`...a3:06`/`...9c:06` @ -44, `...40:c6` @ -62, all ch
11) plus neighbors on chs 1/6/11. The two things flagged as load-bearing-but-unverified
in (cont. 6) -- async `WiFi.scanNetworks()` coexisting with ESP-NOW, and the post-scan
channel re-pin -- **both hold**.

**Found + fixed a real bug:** heartbeats and scan-AP packets shared one tx sequence
counter, so each scan batch's N sends read as N phantom heartbeat *gaps* at the master
(uplink PDR showed a bogus 0.65). Gave heartbeats their **own contiguous seq** (`hbSeq`
in `sendHeartbeat`). Re-OTA'd the fix via the **maintenance round-trip** (master serial
`u` -> peer rejoined BubbyNet -> OTA -> both back to comms, no touch -- also validates that
path). After: `gaps=0` through scans, `pdr=1.0` with an honest occasional `gaps=1`.
(Downlink `dlpdr~0.8` is expected: the peer is deaf to the master's 10 Hz frames during
its own ~2.5 s scan window -- informative, not a fault.)

Net: **item (b) is validated on hardware.** Still TODO: the actual **yard walk** (carry
`9E5B0C` out, watch the per-Eero-node RSSI fall off -> the coverage-at-distance map +
where to place a field maintenance AP) and write that note. Tooling to capture it
(`net_bench_serial_bridge.py` -> `net_bench_log.py` `nb-scanap` rows) is ready but a
background log wasn't started this session. Boards left running on ch 11.

## 2026-06-08 (cont. 6) -- Ben + Claude -- WiFi coverage diagnostic, reworked as a wireless ESP-NOW bridge (firmware done, untested on hw)

Picked up the solar-telemetry/range handoff plan, item (b) -- the WiFi range diagnostic.
Started on the standalone tethered sketch (`firmware/wifi_diag/`: associates, streams
RSSI/BSSID/channel + a 2.4 GHz scan, flags a *missed-roam* when a stronger same-SSID Eero
node wasn't chosen). Then Ben pushed back on the laptop tether and proposed a better
setup: an **ESP-NOW "wireless serial" bridge** to his desktop. That's the right call --
it's the *same* architecture item (a) needs, so building it once serves both.

**Reworked (b) as scan-only over an ESP-NOW bridge** (extends `firmware/net_bench/`):
- **`--serial-bridge`** (a master): does NOT join WiFi; stays pinned to `--channel` and
  relays everything it hears (`nb-master`/`nb-peer`/`nb-scanap`) to **USB serial**, so a
  desk-tethered board logs the whole field fleet -- no laptop in the yard.
- **`--scan-report`** (a field peer): async-scans 2.4 GHz (**never associates**), then
  broadcasts the strongest `--scan-max` APs (BSSID/RSSI/ch/SSID) as a new `NB_SCANAP`
  packet. Because it never associates, the radio is **ours to pin to `--channel`** (no
  Eero-channel coupling -- the key insight; an *associated* board is locked to the Eero's
  channel and ESP-NOW rides that). Radio is re-pinned to `--channel` after each scan;
  ESP-NOW TX is suppressed while the scan hops.
- Host: `ops/bench/net_bench_serial_bridge.py` relays the bridge's serial -> UDP:54321 so
  the **existing** `net_bench_log.py`/`net_bench_monitor.py` work unchanged; `net_bench_log.py`
  gained an `nb-scanap` row (per-AP coverage -> JSONL).

Why this answers (b): the plan's own stated smoking gun is "a scan showing a closer node
with better RSSI it didn't pick" -- a **scan needs no association**, so scan-only delivers
the per-Eero-node RSSI coverage map from anywhere in the yard (and tells us where to put
the field maintenance AP). The empirical roaming-*decision* test stays in the tethered
`wifi_diag` probe.

**Status: all 4 net_bench variants compile clean (28% flash); NOT yet run on hardware**
(no board on USB this session -- ACM0 is the PAR sensor). Cautions before trusting any
map: async `WiFi.scanNetworks()` + ESP-NOW coexistence on the S3 is assumed-fine but
unverified, and the post-scan channel re-pin is the load-bearing line. Next (Ben): flash
2 boards on a shared `--channel`, walk the field peer, confirm `nb-scanap` updates from
the yard, then write the RSSI map + AP-placement note here. Details: updated
`SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md` (end) + `firmware/net_bench/README.md`.

## 2026-06-08 (cont. 5) -- Ben + Claude -- A/B rollback VALIDATED (bad image auto-reverts) + the recipe

Tested A/B rollback with a bad image (battery-only LFP). **PASS:** pushed a power_bench
build whose self-test hook reports unhealthy (`extern "C" bool verifyOta(){return false;}`,
gated by `-DRES_OTA_FAIL_SELFTEST`); on first boot the Arduino core (`initArduino`, before
`setup()`) saw the image `PENDING_VERIFY`, called `verifyOta()`->false ->
`esp_ota_mark_app_invalid_rollback_and_reboot()` -> bootloader **reverted to the last-good
image automatically, no touch** (board came back on `ota1`; the bad image never reached
setup/WiFi). `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is in the arduino-esp32 3.3.7 build.

**Gotcha (caught the first try):** `verifyOta()` is a **C-linkage** weak hook (defined in a
.c core file). A plain C++ override is name-mangled, silently does NOT override, the default
(returns true) runs, and the bad image **sticks** (no rollback). Must use `extern "C"`.

**Production recipe (the safety net):** implement `extern "C" bool verifyOta()` with a real
self-test (power chip init + radio + fuel-gauge reachable) -> return false on failure for an
auto-revert. **Limitation:** this only catches self-test FAILURE; an image that passes
verifyOta then crashes/hangs LATER in setup()/loop() is already marked valid -> could brick.
Robust pattern: `verifyRollbackLater()=true` to DEFER the mark-valid, run extended checks +
the watchdog, and mark valid only after proving stable for N s -- so a late crash/hang trips
the watchdog while still PENDING_VERIFY and rolls back next boot. power_bench keeps the
gated `RES_OTA_FAIL_SELFTEST` fixture as a reusable rollback test.

## 2026-06-08 (cont. 4) -- Ben + Claude -- Battery-only OTA validated on worst-case LFP (the field-reset requirement)

Per Ben (correctly): battery-only OTA with NO physical access is a hard requirement (can't
take lanterns off the tree), so battle-test it now. Did 3 consecutive OTAs to the LFP board
**battery-only (no USB), at ~3.2 V (the buck-boost-crossover, hardest regime), over WiFi**:
**3/3 recovered cleanly, no button**, each via software reset (`rr=software`), and the new
image confirmed running (`fw` flipped to `power-bench-2026-06-08.ota1` after OTA#1 -- a real
update, not a rollback). With the ~14 battery-only peer OTAs earlier this session that's
**~17/17, zero failures.** Conclusion: **battery-only field OTA is trustworthy** -- the
"never touch a deployed lantern" requirement is met.

Key clarification (resolves the earlier confusion): the flaky/stranding resets were the
**USB-JTAG hardware reset** (esptool's RTS path during *USB* flashing) + the no-battery
brownout -- neither exists in field OTA, which uses `ESP.restart()` (software reset), reliable
every time. "Use USB" was bench-iteration convenience, not a trust statement.

Caveats (refinements, NOT blockers; a failed OTA is safe -- stays on / A-B rolls back to the
known-good image, never bricks): (1) tested over GOOD WiFi (the field model = a local AP near
the tree for a maintenance window); OTA over a MARGINAL link is untested (TCP retries, but a
bad link could fail the upload -> no update). (2) A/B rollback not yet explicitly tested (push
a deliberately-broken image -> confirm auto-revert) -- worth doing as the ultimate safety net
alongside the watchdog + autosleep recovery.

## 2026-06-08 (cont. 3) -- Ben + Claude -- Solar path validated (net-positive in weak light) + LFP bring-up + a brownout root-cause

Moved to solar feasibility (power_bench, not net_bench). Switched to the LFP 2000 mAh cell --
flashed `power_bench --chem lfp --cap 2000 --maintain 5.5` (Seeed 3W panel: Vmp 5.5 / Voc
8.2 / Imp 540 mA) BEFORE connecting the cell (LFP charges to ~3.6 V, not Li-ion's 4.2 V --
flashing the LFP profile first keeps the charger safe). `Board.init(2000, Generic_LFP) Ok`.

**Brownout root-caused (clean):** on USB with NO battery, the board crash-looped (USB-CDC up
~1 s then reset). Cause = `--maintain` (VINDPM) 5.5 V > USB 4.92 V -> the charger *rejects*
USB (won't pull its input below the 5.5 V setpoint) -> with no battery to source VSYS, it
brownouts; and enabling charging into a missing battery is the trigger point. Connecting the
cell fixed it instantly (battery sources VSYS). Unifies the earlier brownout work: it was
`maintain > supply voltage` + no buffer, not "no battery" per se. (Firmware guard TODO: don't
enable charging if no battery detected; and `maintain` must be <= the supply you're on.)

**Solar result (partly cloudy, ~10:18 am Oakland, through a window):** panel **5.56 V x
~66 mA ~ 0.37 W**, VINDPM holding the panel steady at 5.5 V, **battery_ma +~10 mA -- net
POSITIVE charge** into the LFP (3.31 V / 33%, safe) *even with WiFi running* (the radio eats
~56 of the 66 mA; ~10 mA banks). Path validated end-to-end. Extrapolations: asleep, ~all
66 mA would bank; full sun -> ~540 mA (~8x) -> the ~120 mAh/night budget closes with margin.
Solar essentially de-risked (it's what the board is built for).

Also: ESP-NOW reached the back fence but WiFi-STA couldn't hold the yard -- expected, not a
bug (different destination = router vs office-master, and WiFi assoc+TCP needs far more
margin than ESP-NOW's loss-tolerant broadcast). It WiFi-reconnected fine once close -- no
instability. Next (do on USB so reflash/tune is safe): full-sun board-asleep harvest number
+ `--maintain` sweep (5.5/5.0/4.6) for the shaded canopy.

## 2026-06-08 (cont. 2) -- Ben + Claude -- T3 range walk: clean V, link held through house+yard+oak

Walked the cup board (`9F2690`) out the back door, across the yard to the fence (behind a
big oak), and back, slowly, with the 3 stationary boards as controls. New tooling:
`ops/bench/net_bench_walk.py` (continuous per-peer RSSI/PDR logger, run in background) +
`net_bench_walk_plot.py` (Pillow V plot) + live landmark markers. Result: a clean V/bathtub
(-19 dBm office -> -80..-87 floor at fence/oak with a few brief dropouts -> -30 back), 152
samples / 328 s. **Findings:** (1) the **house doorway dominated** (~50 dB in the first ~30
steps); open-yard distance added little; (2) the **oak trunk caused the deepest dips**,
recovering at the fence past it; (3) RSSI is **path-asymmetric** (door -69 out / -47 back --
multipath); (4) the **3 reference boards stayed flat** -> the swing is real, environment
stable (good control). The link **held ~100 steps through a house door + full backyard +
behind an oak** -- far harsher than the tree (open air + bamboo, no doorway), so a strong
deployment result. Data/graph: `ops/bench/data/ca/2026-06-08-rangewalk.{jsonl,png,-markers}`.
Live RSSI also viewable via `net_bench_monitor.py`. (Still un-measured: pure open-field
clean-LoS cliff distance -- the house doorway masked the distance falloff here.)

## 2026-06-08 (cont.) -- Ben + Claude -- Obstruction mapping: enclosure ~RF-transparent, solar panel is the attenuator

Used the identify/locate blink to label peers placed in different obstructions (10 Hz, all
held ~99-100% PDR at bench range): 3D-printed lantern cylinder (board inside) -15 dBm;
ceramic cup -29; metal laptop in a metal+glass cabinet -31; **glass+metal solar panel on a
box -52** (~25-35 dB hit). **Two build-relevant findings:** (1) the **lantern enclosure is
~RF-transparent** -- the printed/plastic housing won't detune or block the mesh; (2) the
**solar panel is the one real attenuator (~25-35 dB)** and it sits over the antenna in the
hat -- the antenna-keepout concern made concrete (still 100% PDR / ~38 dB margin at bench
range). Caveats: placement+obstruction combined (not pure material deltas), RSSI approximate,
short range. Worst case = panel attenuation + full tree distance stacked -> the mock-hat RF
test (Steve). Also flagged: identify's 8 s blink is too short for human-in-the-loop / field
use (Ben missed a single blink waiting on chat latency) -- make it ~30 s or toggle-until-stop.

## 2026-06-08 -- Ben + Claude -- Fuel-gauge false-low after charge (SOC needs voltage cross-check)

Morning: one peer (`9E5AF0`, 10050 mAh) was blinking 4 Hz (LED "<10%"), but **bv=4.188 V
= fully charged** -- the cell charged fine; the gauge is misreading 1%. Extends yesterday's
cap-reseed finding: after the `DesignCap` change the MAX17260 re-seeded per-board to
*different wrong* values (`9F26F8`->~100%, `9E5AF0`->~1%), and the overnight charge didn't
fix it because the board ran the whole time (~100-200 mA) so the charger likely never hit
the clean **termination** event the gauge uses to anchor 100%. Lessons for production: (1)
gauge SOC is untrustworthy after a cap change / without a real learn cycle; (2) an
always-awake fixture solar-charging may never anchor its gauge (the duty-cycled CA design
helps -- low load during charge); (3) **low-battery logic must cross-check voltage** -- a
false 1% could trip a needless shutdown, a false 100% could over-discharge. Action: add a
voltage sanity-check to the battery LED (bv>4.0 V => never show "critical").

**Done (v07.5, OTA'd to all 5):** the battery LED now floors the displayed level by a
loaded-Li-ion voltage estimate, so a false-low gauge can't show "critical" -- `9E5AF0` now
shows SOLID (gauge still reads 1% but bv 4.19 V vetoes it). Ben's field-vs-bench insight:
this false-low is likely a **bench artifact** -- deployed fixtures sleep + trickle-charge
from solar under near-zero load, so the charger reaches termination and the gauge anchors
(and gets a real cycle daily); the always-pinging bench run is the pathological case.
Friction noted: each firmware OTA needs per-board cap bins (cap is a build flag) -- a
follow-up could store cap in NVS / make it runtime-settable so one bin serves all.

## 2026-06-07 (cont. 6) -- Ben + Claude -- Rate sweep PASS: ESP-NOW scales to ~100 nodes

Ran the broadcast-rate sweep (new `ops/bench/net_bench_ratesweep.py`, drives the master's
`+`/`-` over serial + measures per-rate PDR from the bridge), 1->50 Hz, master + 4 peers,
co-located, Li-ion. **Aggregate uplink PDR >=97% across the whole range, no collapse:**
1Hz 100%, 10Hz 99.5%, 20Hz(100 pkt/s) 99.1%, 50Hz(250 pkt/s) 97.2%. Clean airtime fit
`loss ~ 1.05e-4 x pkt/s` -> **100 nodes @ 1-2 Hz/node ~ 98-99% PDR**. Strong GREEN for the
"can we base 100 fixtures on this" question. (Tooling fix: the naive worst-peer knee was a
small-sample artifact -- one lost packet of ~60 reads as 98%; switched the verdict to
aggregate loss.) Caveats: 5-node small-N (no hidden-node at scale), co-located (range is
T3/T4 next), Li-ion (re-verify on LFP). T5 parallel-OTA already passed; T3/T4/T6/T7 remain.

## 2026-06-07 (cont. 5) -- Ben + Claude -- Identify/locate command; per-board cap; MAX17260 re-seed finding

Added an on-demand **identify/locate** command (master `i`/`I` -> target board blinks a
distinct `..-` on the onboard LED for 8 s; the data-center chassis-ID pattern) and used it
to map board<->battery without plugging in: master 2200, `9F2690`/`9E5AB8` 4400,
`9E5AF0`/`9F26F8` 10050 mAh. OTA'd each board with its correct `--cap` (fw v07.4; all 5
recovered no-button -- cumulative OTA reliability still 100%).

**Fuel-gauge finding:** changing the MAX17260 `DesignCap` re-inits the gauge and **resets
learned SOC** -> a transient bad reading (`9F26F8` 10050 mAh: 27% @3.73 V with cap=2000 ->
**100% @3.72 V** after re-seeding to 10050; true ~50%). So: **set DesignCap once at first
boot, charge to full to anchor 100%, let the gauge learn over a cycle; don't change cap in
the field.** More critical on LFP (flat OCV). Folds into T6 prep (fully charge cells
first). Also shipped a `/resume` re-init fix (v07.3) in the same firmware.

## 2026-06-07 (cont. 4) -- Ben + Claude -- net_bench first light: ESP-NOW works, OTA validated, battery-LED deployed

Flashed the fleet (1 master USB + peers on Li-ion). **First light, ch 11:** master +
**3 peers** up, uplink/downlink **PDR ~99.5%** at 10 Hz co-located, RSSI -25 to -33 dBm,
**0 send-fail** -- ESP-NOW works. (One flashed peer never booted -- a silent no-boot the
watchdog can't catch since it never reached loop(); post-flash boot flakiness or flat
cell.) Added a **battery-level onboard LED** (GPIO46: >50% solid, 25-50% 1 Hz, 10-24%
2 Hz, <10% 4 Hz) and **OTA-deployed it** (v07.2) to master + 2 reachable peers via the
maintenance-mode cycle. **T5 effectively PASS** -- all recovered via *software reset, no
button* (master via /telemetry, peers via ESP-NOW rejoin with rr=software).

Two findings: (1) `net_bench_ota.py` false-FAILED the peers -- they reboot OFF WiFi into
comms, so /telemetry polling can't see them; fixed with `--reboot comms` (the OTA
"complete/Rebooting" ack + software reset IS the success signal; confirm rejoin via the
bridge). (2) **Brownout de-risk:** the ~4%-SOC peer dropped out entering maintenance --
the WiFi-association inrush on a near-empty Li-ion cell is the brownout failure mode; at
100x we must gate OTA/maintenance on SOC (or lean on the autosleep guard). Next: charged
cells on all boards, then the rate sweep + range/obstruction matrix.

## 2026-06-07 (cont. 3) -- Ben + Claude -- net_bench: first ESP-NOW firmware + 5-node feasibility harness

Built the project's **first ESP-NOW firmware** to de-risk basing ~100 fixtures on the
PowerFeather V2 (networking/radio/stability axis). New `firmware/net_bench/` (forked from
power_bench): broadcast-only ESP-NOW (unencrypted FF:FF -- the 100-node-scalable pattern;
encrypted peers cap at ~17), **master** role (broadcasts SHOW_FRAME + WiFi-STA-bridges
per-peer stats to the host over UDP:54321) and **peer** role (pure ESP-NOW on battery,
HEARTBEAT with seq/battery/downlink-PDR). Per-source seq-gap PDR. **Maintenance-mode
switch** (ESP-NOW metadata -> peers join AP -> standard WiFi OTA, ADR-0010 compliant; no
firmware over ESP-NOW). **Watchdog** added (esp_task_wdt -- net-new, closes the open
field-reliability TODO) + `--wdt-hangtest`. Autosleep guard ported.

Host harness: `ops/bench/net_bench_log.py` (master bridge -> JSONL), `net_bench_ota.py`
(parallel OTA + auto-recovery/no-button assertion), `net_bench_summary.py` (per-peer
PDR/RSSI + scale-extrapolation loss knee). Test plan + acceptance targets:
`docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md`.

**Bench-validated on 1 board (9E5B0C):** boots, Board.init Ok, ESP-NOW up, heartbeats
broadcasting (0 send-fail); **watchdog recovery PASS** (induced hang -> task-WDT reset ->
reboot, post-reset reason `task_watchdog`, no human); master WiFi-join + host JSONL
capture PASS. **Channel-lock confirmed real:** home AP "BubbyNet" is ch 11, so building
with `--channel 6` made the master warn and every send fail (`Peer channel != home
channel`). **Action for Ben: build all 5 boards with `--channel 11`** (= the AP channel)
to run the multi-node matrix. All battery results will be Li-ion (JST-PH) -- asterisked to
re-verify on LFP (LFP plateau sits on the buck-boost crossover, the harder regime).
Multi-node T0-T7 pending Ben's 5 boards on a matched channel. Plan approved; this is the
implementation of that plan.

## 2026-06-07 (cont. 2) -- Ben + Claude -- Second Split style (rotate-about-center) + ping-pong spiral

Two small LED Studio refinements:
- **Split RGB is now 3-state (Off / Triad / Rotate).** Triad = the original local R/G/B
  offset cluster (spread/rotate). **Rotate** = R at the point, G/B the same point
  rotated 120 deg /240 deg about the grid center -> a 3-fold rotationally-symmetric color
  split (collapses to white at the exact center; shines with a moving spiral/orbit
  head). Both validated on hardware.
- **Spiral now ping-pongs** (out to the edge, then retraces inward) instead of jumping
  from the outer tip back to the center -- no per-frame discontinuity. Orbit still wraps
  seamlessly (closed ring). Verified: spiral order-index steps by <=1 the whole cycle.

## 2026-06-07 (cont.) -- Ben + Claude -- Merged LED Studio (HEX + RGBW + RGB), Split-as-toggle

Merged `hex_studio` + `rgbw_studio` into one **`firmware/led_studio/`** with a UI mode
toggle that hot-swaps between three LED options on the same A0/GPIO10 data pin -- no
reflash -- by reconfiguring the NeoPixel type/length at runtime
(`updateType`/`updateLength`): **HEX grid (37px RGB)**, **RGBW point (1px)**, and a
new **RGB point (1px)** for the high-power RGB LED (same as the RGBW minus the white
die -- same render path, 3-byte strip, W ignored). Removed the two now-superseded
single sketches. Confirmed harmless to mismatch mode vs physical module (both SK6812):
worst case is wrong colors / one LED until refreshed; strip is blanked on each switch.

Per Ben's request, **Split-RGB is now a toggle modifier, not its own animation** -- so
the separated R/G/B triad follows the selected path: Static (parked at the anchor,
Step+ to move it), Spiral, Orbit (sweeps the triad along the path with trail), and
Breathe (pulses the triad). Spread/rotate tune the fringe width. Validated on hardware
across all three modes + the split paths.

Process note (field-reliability data): the **USB-JTAG flash flakiness recurred twice**
this session -- the port dropped after one upload (needed a replug) and a write failed
with "Error during build" before succeeding on retry. Reinforces the TODO that the
deployed lantern must never depend on the USB/RTS reset path (software reset + watchdog
+ the autosleep recovery instead). Recovering the IP after a reset still needs the
pyserial RTS pulse (native USB-CDC) -- see `firmware/POWERFEATHER_NOTES.md`.

## 2026-06-07 -- Ben + Claude -- Two findings: 3V3-rail-needs-enabling (GPIO4) + 8-bit gamma low-end dead-zone

**1) PowerFeather V2 switchable 3V3 rail must be enabled (GPIO4 / EN_3V3).** The
studio sketches drove the HEX/RGBW off the 3V3 header but didn't run the SDK, so the
header read **0 V** -- the rail is a load switch gated by GPIO4 (active HIGH), which
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
`out = (in/255)^2.6 * 255`, but Adafruit's gamma8 table maps **input 0..23 -> 0**
(then 1 for 24..35, 2 for 36..43...) -- the bottom ~9% of the range quantizes to off
because 8-bit PWM has no codes for the sub-1 values the curve demands. Tradeoff:
gamma-on = smooth perceived dimming mid/high but a dead-zone + coarse steps at the
bottom; gamma-off = usable ultra-dim but non-linear ramp. This matters because the
lantern's ambient spec ("1-3 LEDs at ~10%") sits right in the dead-zone. Noted for
later; fixes to consider when tuning the ambient look: dim-floor (`max(1,gamma8(x))`),
gentler gamma, gamma-on-color-only, or temporal dithering. No change made now.

## 2026-06-06 -- Ben + Claude -- RGBW Studio: interactive web app for the 4 W RGBW point source

Built `firmware/rgbw_studio/` -- sibling of hex_studio for the single high-power
SK6812 RGBW pixel (Adafruit 5163, 4 W). Validated on hardware (PowerFeather ACM1,
RGBW data on GPIO10): boots, joins WiFi, serves UI; all endpoints exercised OK
(W-only, hue cycle, candle, off) and the board stayed alive through the animations.
Came up at http://192.168.4.209 (same DHCP lease as the HEX session).

The RGBW is a point source (crisp gobo) with a dedicated W die, so this studio is
all about color + temporal modulation (no geometry): R/G/B/**W** sliders + color
picker, gamma toggle; white/warmth presets (W-only, RGB-white, RGBW-full, warm amber)
+ a warmth crossfade slider (RGB-white <-> W); and color animations -- **Hue cycle**,
**Breathe**, **Candle** (smoothed random-walk flicker of the chosen color), **Fade**
(crossfade to a Color-B picker). Settings readback for recording good combos.

Reminder from the LED findings: at 3.3 V the RGBW is voltage-starved (dim, non-linear
mid-range) -- fine for judging color/shadow geometry on the bench, but use 5 V for true
brightness characterization. Next: run it through the inverted-lantern gobo rig
alongside hex_studio to settle point-vs-area (and W-vs-RGB-white) by eye.

## 2026-06-04 (cont. 11) -- Ben + Claude -- HEX Studio: interactive web app for HEX aesthetics + gobo dial-in

Built `firmware/hex_studio/` -- a standalone WiFi web app to dial in the SK6812 HEX
look through the gobo, separate from `power_bench` (which is brownout/telemetry
scaffolding). Validated on hardware: flashed to the PowerFeather (ACM1, HEX data on
**GPIO10**, 3V3 + GND), boots, joins WiFi, serves the UI. Boot prints confirm the
HEX37 geometry (`ring sizes 1/6/12/18`); all HTTP endpoints exercised OK (`/state`,
`/set`, `/off`). Drove it red/center, then split-mode -- the R channel pixel computed
onto index 19, confirming the triad geometry.

Features: brightness + R/G/B sliders (+ color picker), gamma toggle for smooth
low-end dimming; shape selector (center / +inner ring / +two rings / all, computed
from the real hex rings, center = px 18); animations -- **Spiral** (single pixel
outward, trail slider), **Orbit** (single pixel around a chosen ring = the gobo
*moving-shadow* test), **Breathe**, **Twinkle**; **Freeze + Step+** to park a moving
pixel and read off its index; and **Split RGB** (Ben's ask) -- pure R/G/B on three
pixels in a triad around an anchor, with **spread** (fringe width) + **rotate**
sliders, anchor walked by Step+ -- to deliberately throw *wide separated color
fringes* through the gobo (vs the tight fringe of co-located channels). The page reads
back the exact current settings (rgb/hex, bri, shape, anim, lit pixel, split anchor/
spread) so a good-looking combo can be recorded precisely.

Bench wiring confirmed this session: **ACM1 = PowerFeather MCU, ACM0 = Apogee PAR
meter**, HEX on **pin 10**. Flash: `./build.sh --pin 10 --port /dev/ttyACM1`. The S3
is native-USB-CDC, so the boot banner (with the IP) only appears on a reset -- pulse
RTS via pyserial (or just re-flash) to recover the IP; this session it came up at
192.168.4.209 (DHCP, may change). Next: Ben drives it through the inverted-lantern +
flat-filter rig (source on desk, shadow on ceiling) to compare point vs area vs
split-fringe looks and record what reads well.

## 2026-06-04 (cont. 10) -- Ben + Claude -- AMENDMENT: LED axis NOT resolved; RGBW undervolting is viable; gobo testing queued

Walking back two overstatements from the cont. 8/9 entries below. Those entries
stand as the record of what was measured, but their *conclusions* were too strong:

1. **"LED axis resolved / SK6812 HEX direct-GPIO is the BOM front-runner" -- overstated.**
   The LED module is **not decided**. IS31-out is firm, but the HEX-direct and the
   4 W RGBW are **roughly tied in viability** and serve **different, complementary
   roles**, not the same one:
   - **SK6812 HEX direct-GPIO** = distributed / area source -> **washes out the gobo**
     (good for general ambient glow), or animate by moving a single lit pixel around
     the hex (the cast-shadow-in-motion idea -- untested, want to try it).
   - **4 W RGBW** = single **point source** -> the only candidate that throws **crisp
     mandala shadows** through the gobo. A multi-LED array can't do that geometry.
   Because the gobo wants a point source and the ambient mode wants an area source,
   the "winner" may be **application-dependent** rather than one part. No frontrunner
   until gobo testing says so.

2. **"4 W RGBW needs 5 V" -- overstated.** It is **voltage-starved at 3.3 V in this
   bench run** (non-monotonic mid-range current near its Vf), but Ben is fairly
   convinced from prior experience that **undervolting it is viable -- 5 V is NOT
   required**, with caveats. What we actually have is a poorly-characterized low-V
   curve, not a hard 5 V requirement. **Open work:** properly map the RGBW's 3.3 V
   behavior -- usable dimming range, color balance, max brightness -- before deciding
   whether any boost is warranted.

Also flagging that the **PAR/mA efficiency ranking is muddied** by testbeds run at
different SOC/load (each LED run sat at a different buck-boost operating point -- see
the Field-reliability "buck-boost efficiency vs VBAT" item), so the HEX-vs-NeoHEX
~1.6x and HEX-vs-RGBW comparisons are *system* efficiency at as-measured conditions,
not a clean intrinsic ranking. Re-rank at a fixed VBAT before trusting the slopes.

**Next:** basic gobo testing (point vs area source, crisp-shadow vs wash, the
single-moving-pixel animation idea) + a clean RGBW low-voltage characterization.
TODO + ADR 0018 amended to drop the single-winner framing. ADR 0018 rewrite should
record "IS31 out; HEX-direct and RGBW both live" -- not a decided module.

## 2026-06-04 (cont. 9) -- Ben + Claude -- 4W RGBW characterized + full efficiency ranking (LED axis resolved)

Tested Adafruit 5163 (4 W addressable RGBW NeoPixel) direct-GPIO. At 3.3 V it's
**voltage-starved** -- Vf ~3.0-3.2 V, and the rail sags into that band under load
(bv->3.11 V at full), so current is non-linear and it only reaches ~half its rated
output (~430 mA vs ~800 mA at 5 V). Diagnostic: `rgbw-undervolt.png`. **It needs 5 V**
(unlike the hex, which under-volts gracefully). Cleaner re-run via `--wifi-lowpower`.

Final PAR-vs-draw efficiency ranking (`led-par-vs-draw.png`, slope = PAR/mA):
- **RGBW 4 W: steepest + highest PAR (~38)** -- brightest and most efficient *at high
  brightness*; but poor/non-linear dimming at 3.3 V and a single point source; wants 5 V.
- **HEX-direct ~0.07**, **HEX/NeoDriver ~0.055**, **NeoHEX ~0.04** (least efficient, out).

**Warm-white-only (RGBW W channel only, `--rgbw-white`):** the ultra-low-power "vibes"
mode -- **~78 mA at full but dim (PAR 8)** at 3.3 V (W channel under-driven; brighter at
5 V). Efficient (~0.09 PAR/mA) but low absolute output. Cleaner data this run (45 s
dwell + 100% cell) confirmed the earlier low-brightness "PAR>0, mA~0" was the measurement
floor (small LED current swamped by WiFi-baseline jitter), not real zero current. A clean
all-channel re-run (longer dwell) **agrees with the noisy one at the endpoints** (full
white ~430 mA / PAR 40, reproducible) and fixed the br=60 under-read (14->190 mA), **but the
mid-range stayed non-monotonic** (br=160 drew less current than br=100 yet more light) --
i.e. the messiness is the 4 W RGBW operating unstably *at its Vf on 3.3 V*, NOT measurement
noise. PAR (light) is monotonic; current is erratic. Confirms: the 4 W RGBW **needs 5 V**
for a clean/characterizable curve; at 3.3 V only the full-white point is trustworthy. So **LED
draw is a knob ~80 mA (dim warm) -> ~430 mA (full RGBW); the artistic brightness target
picks the point.** Added flags `--rgbw-white`, `--step-ms`.

**LED axis resolves to a use-case choice:** distributed dimmable glow -> **SK6812 HEX,
direct-GPIO @ 3.3 V** (no boost); single ultra-bright beacon -> **4 W RGBW, needs 5 V
boost**; ultra-low-power warm ambient -> **RGBW warm-white-only ~80 mA**. IS31 ruled out
(shared-bus brownout). Tooling added today: `--bright-sweep`,
`--sweep-max`, `--brightness`, `--pixel-pin`, `--wifi-lowpower`; `led_efficiency_sweep.py`
(+reboot-abort), `plot_led_eff.py`, `plot_par_vs_draw.py`, `plot_rgbw_diag.py`; Apogee
SQ-420 PAR reader.

## 2026-06-04 (cont. 8) -- Ben + Claude -- Direct-GPIO HEX validated; 3-way efficiency: direct-GPIO SK6812 wins

Soldered a 4-pin header on board 2 (3V3 * QON-NC * GND * A0=GPIO10) and drove the HEX
(SK6812) **direct from GPIO10** -- no NeoDriver, off the I2C bus. Validated working
(`--led neohex --pixel-pin 10`). Then a capped efficiency sweep (`--sweep-max`, new flag)
overlaid on the NeoDriver curves (`led-eff-3way.png`):
- **Efficiency order: hex-direct >= hex(NeoDriver) > neohex.** Direct-GPIO HEX is ~10% more
  light/mA than HEX-via-NeoDriver (no passthrough/overhead loss), and both SK6812 beat the
  WS2812C NeoHEX (~1.6x).
- **Direct draws ~1.7-1.8x current+PAR per brightness setting** vs NeoDriver (br=60: 362 mA/
  PAR27 vs 215 mA/PAR15) -- because the NeoDriver's Vin->pixel **passthrough drops voltage**
  and direct gives the LEDs the full 3.3 V (current is very VCC-sensitive near the WS2812/
  SK6812 low-V knee). Gap widens with current.
- **Confirmed by the 4-way 2x2** (`led-eff-4way.png`): NeoHEX shows direct~NeoDriver (low
  current -> negligible passthrough drop), while the high-current HEX shows the 1.7x gap -- so
  efficiency is a chip property (HEX 1.6x), and the path-difference is current-dependent.
- **BOM front-runner: SK6812 HEX, direct-GPIO** -- most efficient, fewest parts, brownout-safe
  by construction. Caveats: WS2812 latch their last frame (must send an explicit all-off to
  blank); connect/bring-up gently (full-white inrush browns the rail); higher VCC = browns a
  marginal cell sooner (run on a healthy pack / cap brightness).

Process findings logged: (1) board 2's USB-JTAG **auto-reset is flaky** -- after flashing, tap
the physical reset if the green LED doesn't come up (chip is healthy; verified via esptool
flash_id). (2) **SOC is trustworthy while the cell stays connected** (held 91->92% across a
USB->battery unplug, only bv relaxed ~0.3 V) -- the big SOC jumps earlier were from **cell
hot-swaps** resetting the gauge's coulomb state, not from USB power. New tooling: `--sweep-max`,
reboot-abort in `led_efficiency_sweep.py`, `ops/bench/plot_led_eff.py`.

## 2026-06-04 (cont. 7) -- Ben + Claude -- CORRECTION: NeoDriver does NOT boost pixel power (only the data signal)

Per Adafruit (product 5766): the NeoDriver's 5 V charge-pump is **only for the data
signal** ("clean 5 V signal even on 3 V boards") -- it does **NOT** power/boost the
NeoPixels. *"No way the STEMMA QT port can provide that much current... need external 5 V
on the terminal blocks."* Pixel power = whatever feeds Vin (3-5 V), passed through.
- **Corrects** the earlier (cont. 3/5) claim that the NeoDriver "boosts Vin->5 V,
  self-contained." It does not.
- Explains the "dimmer on 3V3": pixels run at **3.3 V (under their 3.7-5 V spec)** ->
  under-driven, not a boost current cap (the draw-vs-brightness curve doesn't plateau,
  confirming under-voltage scaling, not a current limit). On board 2's USB-hub 5 V the
  pixels got full 5 V -> "blindingly bright."
- **BOM consequences:** (1) full brightness needs a real ~5 V pixel supply -- battery
  (3.2-4.2 V) and 3V3 are below 5 V, so add a **5 V boost** for max brightness, or accept
  reduced brightness under-volted; (2) for dim/<=1 A operation under-volted is fine (matches
  the budget); (3) VBAT (<=4.2 V Li-ion) > 3V3 (3.3 V) for brightness without a boost;
  (4) the NeoHEX-vs-HEX efficiency was measured at 3.3 V (under-volt) -- SK6812 tolerates
  low V better, so re-check the 1.6x edge at the actual ship voltage.
- Plot of the comparison: `ops/bench/data/ca/led-eff-compare.png` (via new
  `ops/bench/plot_led_eff.py`).

## 2026-06-04 (cont. 6) -- Ben + Claude -- NeoHEX vs HEX efficiency: HEX (SK6812) ~1.6x more light/mA

Built brightness-sweep tooling: fw `--bright-sweep` (steps brightness {0,5,15,30,60,100,
160,255}, 30s each, light-WiFi held constant, reports `br=` in heartbeat; br=0 = LEDs off
for a clean baseline) + `--brightness` flag + `ops/bench/led_efficiency_sweep.py` (reads
Apogee SQ-420 PAR on USB + board `ima` over WiFi, groups by br, prints PAR-per-LED-mA).
Setup: 6" tube, PAR sensor at top pointing down, module at base, NeoDriver Vin from 3V3.

- **Result: HEX (SK6812) ~ 1.6x more light-efficient than NeoHEX (WS2812C-2020)** --
  PAR/LED-mA: NeoHEX ~0.040-0.045 (flat), HEX ~0.062-0.072, consistent across all
  brightness steps. At matched ~384 mA draw: NeoHEX PAR 15 vs HEX PAR 26 (~1.7x). HEX
  reaches higher max (PAR 30 @ 491 mA vs 16 @ 384 mA). **For the power budget, HEX wins.**
  Data: `ops/bench/data/ca/led-eff-{neohex,hex}.json`.
- Both SK6812/WS2812C are 37-px RGB (GRB), Grove->NeoDriver, no reflash to swap.
- **Caveats:** PAR is photon flux, not lumens (spectra differ, so perceived-brightness
  ratio may shift -- but 1.6x is consistent across 6 levels); 6" low-SNR geometry (dim
  steps noisy, mid-high solid); color/dimming-smoothness not measured (visual call, also
  tends to favor SK6812). Full-white NeoHEX/HEX off 3V3 = 384/491 mA LED -- within 1 A.
- Found + fixed a baseline bug: `setBrightness(0)` doesn't blank NeoPixels, so br=0 must
  set ledOn=false (color 0) for a true LED-off baseline.

## 2026-06-04 (cont. 5) -- Ben + Claude -- LED decision: IS31 ruled out, NeoHEX (via NeoDriver) leading; NeoHEX-vs-HEX + RGBW queued

- **3V3-powered NeoDriver works on battery:** board 1 (the brownout-prone unit) + NeoDriver
  fed from the **3V3 header** (dim, brightness 30 -> ~0.5 A from 3V3, under the 1 A limit),
  STEMMA for I2C, on battery + WiFi -> **no brownout** (Ben observed). Dim-30 is still
  "pretty bright." Added `--brightness` build-flag.
- **DECISION: IS31FL3741 13x9 ruled out for the V2 battery product.** Cause: its presence
  on the V2's shared charger/gauge I2C bus + WiFi reliably browns out on battery
  (well-proven, IS31-specific). Caveats noted: (a) untested mitigations -- VSYS bulk cap, or
  moving it to the *second* I2C bus (GPIO35/36, not the shared bus) -- might rescue it; (b)
  it's a 13x9 grid vs the hex form. **Revisit only if the grid aesthetic is a hard
  requirement.** Supersedes ADR 0018 (IS31 as primary module) for the battery build --
  flag ADR 0018 for an update.
- **Leading LED path: NeoHEX (WS2812C-2020) via Adafruit NeoDriver** -- no brownout, no
  solder on the I2C side, self-contained (NeoDriver boosts 3-5 V Vin -> 5 V + level-shifts
  data). Continue stability testing.
- **Queued tests:** (1) **NeoHEX (WS2812C-2020) vs HEX (SK6812)** head-to-head -- color
  quality, dimming smoothness (low-end PWM), power efficiency vs brightness, low-V behavior
  (SK6812 generally better at low V / finer PWM; WS2812C-2020 smaller/denser). (2) **single
  high-power RGBW LED.** (3) LED-current measurement at field brightness (folds into #1).

Fixed the brick-risk that ate ~1 h today (no-wake deep sleep stranded board 2, needed
BOOT+RESET download-mode + `esptool erase_flash`). fw `power-bench-2026-06-04.2`:
- **Never deep-sleep while external supply present** (USB/VDC) -- root cause of the
  stranding; on supply the board stays flashable/recoverable and there's no brownout
  risk anyway. `lgSupplyPresent()` = `getSupplyVoltage > 4.0 V`.
- **Timer wake** (15 min) instead of indefinite, via `esp_sleep_enable_timer_wakeup`.
- On a timer wake **still on battery -> re-sleep** (protect cell); **on supply -> run/
  charge**. So plugging USB self-recovers within one interval; can't brick.
- Unified `lgEnterDeepSleep()` (loop-break, coulomb-budget, lowbatt-knee, maxrun all
  route through it; LED-clear guarded for IS31/NeoPixel/NeoDriver). Compiles clean for
  all LED variants.
- **VALIDATED LIVE** (3 mAh budget / 60 s wake, `--budget-mah`/`--wake-s` flags): on USB
  ran continuously w/o sleeping (charging, mah=0); on battery hit the 3 mAh budget ->
  SLEEPING announce -> deep sleep; 124 s of timer-wake/re-sleep silence on battery; then
  USB plug -> recovered on the next wake (fresh boot, ima=+438 charging) with **no
  BOOT+RESET download-mode needed**. Brick-risk resolved.

## 2026-06-04 (cont. 3) -- Ben + Claude -- NeoDriver (I2C) is STABLE: brownout is IS31-SPECIFIC, not the bus

Built a `--led neodriver` variant (Adafruit NeoDriver 5766, SeeSaw I2C -> WS2812, on the
STEMMA bus; added Adafruit_seesaw lib + seesaw_NeoPixel in lgApplyLed). Drove a NeoHEX
full-white, **LED 5 V from an external USB hub** (LED current off the battery; the
NeoDriver boosts 3-5 V Vin -> 5 V and level-shifts data, per its silkscreen).

- **Result: STABLE** -- board 2, NeoDriver on the same shared I2C bus, battery + WiFi,
  full-white -> **371 s+, 0 reboots, through the heavy-WiFi phase**, bv steady 3.25. Same
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
  that needs the **auto-sleep wake-source fix first** (brick-risk; on TODO) -- today the
  no-wake deep sleep + download-mode recovery cost ~1 h and corrupted board 2's WiFi
  (fixed via `esptool erase_flash`).

## 2026-06-04 (cont. 2) -- Ben + Claude -- IS31 presence on the I2C bus is NECESSARY for the brownout (clean A/B)

Decisive test: board 2, same deep-cycled cell, on battery, **IS31 physically unplugged**
-> **stable 365 s+, 0 reboots, through light AND heavy WiFi** (bv 3.27, soc 93). Versus
the same board+cell **with** the IS31 -> brownout loop. Only variable changed = the IS31
on the STEMMA/I2C bus.

- **The IS31's presence on the shared I2C bus is necessary.** Rules out cell+WiFi alone
  (stable) and WiFi-association-inrush alone (stable). Loops occurred in phase 0 with
  **LEDs off**, so it's **not LED current** -- it's the chip on the bus. Matches Ben's
  back-current / I2C-disturbance hypothesis.
- **Still open:** (a) IS31 *actively* misbehaving (spikes/back-current on SDA/SCL) vs
  (b) *any* I2C device loading the shared charger/gauge bus tips VSYS under WiFi.
  Next test: Adafruit NeoDriver (5766, I2C SeeSaw) on the same bus, NeoPixels powered
  externally -> also brownouts => (b); clean => (a). Needs a SeeSaw NeoPixel driver in fw.
- **Procurement note:** an I2C LED module on the V2's shared power-management bus is a
  real risk for the battery product; nudges toward a non-shared-bus (GPIO/SeeSaw-with-
  external-power) LED path, or bus isolation / bulk cap mitigation.
- Aside: board 2's WiFi wedged after the brownout/deep-sleep/download-mode gauntlet;
  recovered only via full `esptool erase_flash` + reflash + clean reboot (corrupted
  PHY/NVS). The loop-breaker's no-wake-source deep sleep also needed manual BOOT+RESET
  download-mode to reflash -- both reinforce the wake-on-USB fix already on the TODO.

## 2026-06-04 -- Ben + Claude -- Brownout CAME BACK overnight (794-reboot loop); guard flaw fixed; SOC/voltage thesis confirmed

Left board 1 on the loadgen on battery overnight (coulomb-budget auto-sleep at 91%
SOC). Morning: a **794-reboot loop over 4.25 h** -- every reset `poweron` (VSYS
collapse), at **healthy bv 3.24-3.46 across SOC 98%->30%**, in the **lightest** phase
(LEDs off, light WiFi), boots dying ~5-9 s in (around WiFi association). The first
boot ran 112 s, then a steady ~100 reboots / 30 min.

- **The brownout is real + intermittent on board 1.** Yesterday's "non-reproduction"
  (n=3 boards stable, capstone, wiggle) was the fluke; it drifts marginal over
  hours/temperature. Strengthens **H2 (marginal connection on board 1)**; per-boot
  trigger looks like the **WiFi-association current spike**, not load-stacking
  (lightest load) and not depletion (healthy V at every SOC).
- **Guard flaw (Ben called it):** coulomb-budget + max-runtime + low-V auto-sleep are
  all RAM state that resets each reboot, so a tight loop defeats them (`mah_used`
  never passed 1.4 of the 1000 mAh budget). It only bled slowly (92%->30%) because
  each short boot draws little. **Fix:** NVS-persisted boot counter (`--autosleep`) --
  clean start (USB/SW reset) zeroes it, `poweron` boots increment, >=25 sub-survival
  boots => deep sleep before WiFi.begin; a boot surviving 120 s clears it. fw
  `power-bench-2026-06-04.1`. Heartbeat now also carries `soc=` and `mah=`.
- **SOC/voltage thesis confirmed hard:** bv pinned at ~3.24 V for 4 h while gauge SOC
  drained 92%->30% -- LFP voltage is useless for SOC, but the gauge's coulomb count
  tracked the drain (it's the *voltage* that's untrustworthy, not the gauge number).
  Plots via new `ops/bench/plot_soc_v.py`:
  `2026-06-02-ca-liion-4400-soc_v.png` (Li-ion, usable slope) vs
  `2026-06-03-ca-lfp-overnight-soc_v.png` (LFP, near-vertical plateau). Logger:
  `ops/bench/loadgen_log.py` (JSONL + inline reboot flags + LED-current A/B).
- **Now running (2026-06-04):** same cell+grid on **pristine board 2**, multi-hour
  with the fixed guard -- board-specificity test (loop like board 1, or run clean?),
  and if stable it finally captures the LED-current A/B + LFP V-SOC discharge curve.

### 2026-06-04 (cont.) -- board 2 ALSO loops (NOT board-specific); loop-breaker validated

- **Board 2 (pristine) brownout-looped too** -- first boot 356 s (reached phase 1,
  grid lit), then collapsed on the USB->battery unplug (Ben watched the grid cut out at
  the instant of unplug = the first brownout), then looped (poweron, healthy bv ~3.23,
  soc ~72). So the brownout is **NOT board-1-specific** -- overturns the "board 1 solder
  joint" read. Common factors across all looping cases: the **cell** (deep-cycled
  overnight), the **IS31 grid + cable**, firmware.
- **Loop-breaker FIRED (fix validated in the wild):** board 2 deep-slept itself out of
  the loop. Logger saw only 8 reboots but the firmware NVS counter counts every boot --
  including the sub-association boots that die before sending any UDP -- so it hit 25 and
  slept while staying silent to the logger. Cell protected at ~72%/3.23 V.
- **Temperature ruled out** (Ben: office 72.5 deg F now, ~74 when it worked, 79 max -- too
  narrow to matter).
- **Leading hypotheses now:** (Ben) the **IS31 driver latching into a bad state** ->
  back-current/spikes on SDA/SCL (fits: IS31-unplugged always stable; `enableVSQT(false)`
  never helped = I2C back-power); vs the **deep-cycled cell's raised ESR** exposing the
  IS31+WiFi load. Next: (1) unplug IS31 + rerun same cell (presence necessary?), (2) GPIO
  WS2812 vs IS31 (I2C-specific vs load), (3) fresh cell + IS31 (cell-ESR).

## 2026-06-03 (cont. 2) -- Ben + Claude -- Brownout does NOT reproduce on n=3 boards; supersedes the "load-stacking" conclusion

**Walk-back of the entry below.** We lifted n=1->n=3 by moving the **same LFP cell,
same IS31 grid, same STEMMA cable** across three boards (only the board changed), then
re-tested the original board. Result: the brownout reproduces on **none** of them.

- **Board 2** (pristine): stable, light + heavy WiFi, 0 resets, bv to 3.19 V.
- **Board 3** (pristine): stable, light + into heavy, 0 resets, bv to 3.20 V.
- **Board 1** (the one that browned out earlier, capstone re-test, identical setup):
  **stable**, 4 min, 0 resets, bv 3.24 V.
- **Wiggle test** on board 1: 30 s of hard mechanical stress on the leads/connector
  **plus STEMMA hot-replugs** (the action that caused an instant reset earlier) ->
  **0 resets / 0 dropouts over 200 s**. Could not re-induce the collapse by any means.

**So both earlier conclusions are wrong/superseded:** not a platform "load-stacking"
property (boards 2/3 fine), not "board 1 anomalous" (board 1 now fine too). With board,
cell, grid, and cable all held constant, the only thing that changed across the
afternoon is **repeated unplug/re-seat of connectors** -> leading explanation is now
**H2: a marginal physical connection** (soldered battery joint and/or STEMMA seat) that
re-seated. **Inferred, not confirmed** -- we showed the brownout *stopped*, not *why*,
and could not reproduce it even deliberately. Also notable: stable while in **active
boost** at 3.24 V (the *harder* regime) argues against H3 (low-LFP/boost instability).

**Bottom line for procurement (unblocked):** three V2 boards run IS31 + continuous WiFi
on battery with zero brownouts down to ~3.2 V, so we **cannot** call V2 + IS31 unsafe on
battery. We also **cannot** claim full root-cause understanding (non-reproducible). Carry
a **VSYS bulk cap as cheap insurance** and watch for recurrence in the field. Full
write-up (Status, board-swap table, superseded sections) in
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`. Lesson logged: we wrote a firm
conclusion twice today and were wrong both times -- n=1 + a single connection was not
enough.

## 2026-06-03 (cont.) -- Ben + Claude -- Brownout cause isolated: IS31-on-bus + WiFi (load-stacking) [SUPERSEDED by the entry above]

On a SOLID soldered LFP connection (the spring splice had confounded earlier runs)
and with cleaned-up instrumentation (uptime-based phase, no NVS write, `reset_reason`
+ battery V/I in the UDP heartbeat), the brownout reproduced cleanly and we isolated
it. Full write-up + open questions in
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`.

- WiFi off (any LED): stable. WiFi on + IS31 **unplugged** (light or heavy TX):
  stable (9 min, 0 resets, bv to 3.24 V). WiFi on + IS31 **connected**: `poweron`
  brownout ~7-17 s.
- **Cause:** load-stacking -- needs BOTH WiFi active AND the IS31 module physically on
  the STEMMA/I2C bus; neither alone does it. `reset_reason=poweron` (VSYS collapse) at
  healthy bv -> not depletion / connector / chemistry. Modem sleep did not fix it.
- **Sub-result:** firmware VSQT power-shed (`enableVSQT(false)`) did NOT fix it (~21
  resets / 7 min) -- only physically unplugging the module stops it. Candidate
  mechanism: I2C back-powering (IS31 stays on SDA/SCL off the main 3V3). Unproven.

Implications (firming, not final; n=1 board): **VSYS bulk capacitance** is the
mechanism-independent fix (bench-validate next); an **I2C LED module can't be
software-shed** (back-power) whereas a **GPIO WS2812** could; OTA-on-battery shouldn't
rely on VSQT-shed for the IS31 (use bulk cap / daytime solar / a GPIO module).

Also: ported demo gained an **Input Current Limit (IINDPM) slider** -- confirmed the
~500 mA USB charge cap is the **BC1.2/USB-C source-detection default** (not a port
bug; the SDK sets IINDPM=3200 but USB-C advertises current via CC, not D+/D-).
Doesn't affect solar/VDC charging. Tooling: loadgen heartbeat now carries
phase+uptime+bv+reset_reason+lb+sqt, low-batt backoff, and a `--loadgen-shed` mode.

## 2026-06-03 -- Ben + Claude -- Battery-brownout investigation: tooling, plan, ported demo (ONGOING, no conclusions yet)

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
  full grid) -- radio-off baselines.
- `--wifi-lowpower` (modem sleep + 8.5 dBm), `--charge-ma`, `--ota` (wireless flash).

Ported PowerFeather's official ESPUI web-telemetry demo to V2 / SDK 2.x / core 3.x
(`firmware/powerfeather_demo_port`): SDK 1.x->2.x API (mV->V floats, maintain-voltage
units), `Generic_LFP`, and the ESP32Async core-3.x library stack. Compiles, boots,
and brings up the `PowerFeather_Demo` AP on V2 (verified on USB); web UI + on-battery
behavior still to exercise with a phone + a solid battery connection.

Next: re-run the matrix on a solid (soldered) LFP connection at known SOC.

## 2026-06-02 -- Ben + Claude -- PowerFeather V2.R2 power-bench bring-up (Phase A)

PowerFeather V2.R2 arrived. Stood up an Arduino-based power-telemetry bench
harness on it. New firmware `firmware/power_bench/` forked from `smoke_test`,
adding PowerFeather-SDK telemetry and a JSON `/telemetry` endpoint for WiFi data
collection across the three test axes (battery, LED option, solar panel).

Toolchain confirmed: FQBN `esp32:esp32:esp32s3_powerfeather`, board macro
`ARDUINO_ESP32S3_POWERFEATHER`, ESP32 core 3.3.7, PowerFeather-SDK 2.1.0
(namespace `PowerFeather`, singleton `Board`, `<PowerFeather.h>`). LED libs already
installed.

Battery chemistry is firmware-only (no jumpers): `Board.init(capacity_mAh,
BatteryType)` -- `Generic_3V7` for Li-ion (current), one-line swap to `Generic_LFP`
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
- Phase 3: `/telemetry` JSON serves correct values over WiFi -- `battery_v` 3.60 V,
  `battery_ma` +204 mA (charging at the cap), `supply_v` 4.665 V, `supply_ma`
  ~236 mA, `supply_good` true. Power balances: ~1.1 W in, ~0.73 W into the cell.

Two findings:
1. BUG (fixed): the float telemetry fields were one-position shifted due to C++
   unspecified argument-evaluation order -- the SDK getter was inlined as a function
   argument alongside the out-param it writes. Sequenced the getter before the JSON
   append (matching the integer-field pattern). Confirmed against the SDK's stock
   `SupplyAndBatteryInfo` example, which read correctly the whole time.
2. ROOT CAUSE FOUND + FIXED: `soc_pct/health_pct/cycles/time_left_min` returned
   `InvalidState` because the SDK selects the fuel-gauge IC at COMPILE TIME --
   MAX17260 (V2) only if `POWERFEATHER_BOARD_V2`/`CONFIG_ESP32S3_POWERFEATHER_V2`
   is defined, else the V1 `LC709204F`. In an Arduino build neither is set, so it
   defaulted to the V1 gauge and `probe()` failed on the wrong IC (the stock SDK
   example fails the same way for the same reason). A power-cycle did not help -- it
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

## 2026-05-20 -- Ben + Codex -- PCBWay assembly quote revised toward J5-only

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

## 2026-05-18 -- Ben + Codex -- PCBWay packet prepared for NeoHEX adapter

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

## 2026-05-18 -- Ben + Codex -- NeoHEX adapter gained JST-SH fallback output

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

## 2026-05-18 -- Ben + Codex -- NeoHEX adapter moved toward SMT PCBA

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

## 2026-05-18 -- Ben + Codex -- Smoke mode 1 changed to max center

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

## 2026-05-18 -- Ben + Codex -- KiCad 10 starter PCB for NeoHEX adapter

Ben upgraded KiCad from the Ubuntu 22.04 package to KiCad 10 via the KiCad PPA.
Verified `kicad-cli` is now available and reports `10.0.3`; the `pcbnew`
Python module also reports `10.0.3`.

Added a KiCad starter project at
`hardware/led-adapter/neohex-passive-rev-a/kicad/`:

- `neohex-passive-rev-a.kicad_pro` -- KiCad 10 project file.
- `neohex-passive-rev-a.kicad_pcb` -- routed 60 mm x 35 mm starter layout.
- `generate_starter_pcb.py` -- reproducible generator for the starter PCB.
- `README.md` -- KiCad-specific caveats and validation commands.

The starter layout keeps Rev A passive: external `VLED` injection, shared
ground, selectable STEMMA/GPIO data input, 330 ohm data resistor, local
decoupling, optional `SJ4` STEMMA_V+ bridge marked for low-current testing only,
and test pads. `kicad-cli pcb drc` reports zero violations and zero unconnected
items, and Gerber/drill export succeeds into `/tmp/res-neohex-kicad/`.

Important caveat: J1 is still a placeholder JST-PH 1x04 2.0 mm footprint standing
in for the exact M5Stack Grove/HY2.0 socket, and no schematic has been captured
yet. Do not order this board until J1 is replaced with the exact connector
footprint, cable pin order is verified, and the schematic/PCB are back-checked.

## 2026-05-18 -- Ben + Codex -- NeoHEX passive adapter Rev A design packet

Started a small PCB workstream for a no-solder-ish HEX/NeoHEX adapter board as both an educational PCB exercise and a possible 100-unit assembly aid.

Added `hardware/led-adapter/neohex-passive-rev-a/`:

- `README.md` -- design intent, schematic, connector pinouts, layout guidance, assembly variants, bring-up checklist, and open questions.
- `bom.csv` -- first-pass BOM for Grove/HY2.0 output, external LED power input, STEMMA/QT data input, optional generic GPIO input, data resistor, decoupling, jumpers, and test pads.
- `netlist.csv` -- explicit nets for KiCad capture.

Rev A is intentionally passive: connectors, shared ground, power injection, one data-source solder jumper, 330 ohm data resistor, and optional bulk capacitance. It does not include a boost regulator or constant-current driver. Added TODO items to capture the board in KiCad and order quick-turn boards.

## 2026-05-18 -- Ben + Codex -- Planned iso-current LED brightness test

Added `docs/tests/ISO_CURRENT_LED_BRIGHTNESS_TEST_2026-05-18.md` after visual smoke testing showed large brightness differences between full-low modes: roughly `FeatherS2 Neo >> NeoHEX ~= IS31FL3741 > Atom Matrix`, with the Atom Matrix diffuser likely contributing.

The new test plan separates electrical normalization from optical/gobo evaluation. It defines current targets, pattern classes, measurement setup with SEN0291 wattmeters, fixed-camera optical procedure, and result tables. Added a TODO item to run the test once the SEN0291 wattmeters are available.

## 2026-05-18 -- Ben + Codex -- Standalone Atom recovered on new subnet

The standalone Atom Matrix + DFRobot DFR0559 stack appeared unreachable from the dashboard at its old address `192.168.4.250`. After Ben moved it from the DFR0559 output to direct USB, serial confirmed it was healthy and connected to `BubbyNet`, but DHCP had assigned `192.168.5.32`.

Serial report:

- Board: `m5stack_atom`
- MAC: `F8:B3:B7:1B:51:08`
- Fixture ID: `1B5108`
- Reset reason: `poweron`
- Previous firmware: `smoke-2026-05-15.7`
- WiFi IP: `192.168.5.32`

OTA-updated the Atom to `smoke-2026-05-18.2` at `192.168.5.32` and updated the local COTS mode dashboard from the stale `192.168.4.250` address. The board was warm while powered from the DFR0559 even with LEDs off; no firmware fault was visible over USB. Follow up with SEN0291 current measurements on the DFR0559 5 V output before leaving that stack powered unattended.

## 2026-05-18 -- Ben + Codex -- NeoHEX center-cluster mapping adjustment

Ben observed that Atom + NeoHEX mode `3` appeared as a single seven-LED column. The placeholder NeoHEX crop used contiguous indices `15..21`, which confirms the NeoHEX chain appears to be indexed by hex columns rather than by a rectangular 3x3 layout.

Updated the Atom + NeoHEX crop for `smoke-2026-05-18.2` to use a first-pass center hex cluster around center index `18`: `11, 12, 17, 18, 19, 24, 25`. Built the Atom + NeoHEX variant and OTA-flashed `192.168.4.27`; the board came back as `smoke-2026-05-18.2`, and `/mode?m=3` succeeded.

Network scan found the reachable smoke boards at `192.168.4.27`, `192.168.4.248`, and `192.168.4.249`. The standalone Atom + DFRobot DFR0559 stack at prior address `192.168.4.250` remains unreachable; likely next checks are DFR0559 ON jumper position, battery/output recovery via BOOT, supply stability, and then USB serial recovery if needed.

## 2026-05-18 -- Ben + Codex -- Atom + NeoHEX smoke-test variant

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

## 2026-05-15 -- Ben + Codex -- Brightness calibration fix for smoke-test modes

Ben observed that several LED measurement modes were effectively invisible, especially on the Atom Matrix: `4` full-low was invisible, `5` capped full-array was extremely faint, and `1` center was too dim. Root cause was double dimming on NeoPixel boards: low RGB component values were also being multiplied by low `Adafruit_NeoPixel::setBrightness()` values, causing integer scaling to round many channels down to 0 or 1. The IS31FL3741 full-low mode also used RGB values below RGB565's low-end quantization threshold.

Updated `firmware/smoke_test/` to `smoke-2026-05-15.7`:

- NeoPixel measurement modes now use `setBrightness(255)` and control current with explicit low raw RGB values.
- IS31FL3741 modes now avoid RGB565 values that quantize to black.
- Mode `1`, `3`, `4`, and `5` brightness levels were raised while keeping capped full-array modes conservative.

Built and OTA-flashed `.7` to all three unplugged boards over WiFi. All three returned to mode `0`, and `/mode?m=5` then `/mode?m=0` succeeded on C6 + IS31FL3741, FeatherS2 Neo, and Atom Matrix.

## 2026-05-15 -- Ben + Codex -- Static COTS mode dashboard

Added `ops/bench/cots-mode-dashboard.html`, a local static dashboard for the three active smoke-test boards:

- C6 + IS31FL3741 at `192.168.4.248`
- FeatherS2 Neo at `192.168.4.249`
- Atom Matrix at `192.168.4.250`

The page sends `/mode?m=<mode>` commands by iframe navigation rather than `fetch()`, so it works from a local `file://` page without requiring CORS headers from the ESP web server. It includes per-board and all-board controls for modes `0`, `1`, `2`, `3`, `4`, `5`, and `q`, plus embedded board status iframes.

## 2026-05-15 -- Ben + Codex -- OTA and USB flash timing benchmarks

Ben ordered 12 DFRobot SEN0291 I2C digital wattmeters, so manual USB power-meter experiments are on hold until they arrive. Added a TODO item to integrate the wattmeters into the power-test harness/worksheets.

Ran first flash timing benchmarks on `smoke-2026-05-15.6`; details are in `docs/tests/OTA_FLASH_BENCHMARKS_2026-05-15.md`.

Results:

- Strict sequential OTA, waiting for each board to be reachable again: 44.123 s for 3 boards.
- Parallel OTA batch: 18.291 s for all 3 boards to upload and become reachable again.
- USB upload, excluding compile time: C6 7.109 s upload / 10.188 s ready; FeatherS2 Neo 13.047 s upload / 16.218 s ready; Atom Matrix 14.287 s upload / 17.515 s ready.

FeatherS2 had one failed USB reset/upload attempt (`Errno 71`) that left it in the ESP32-S2 bootloader; a recovery USB upload succeeded, and a subsequent normal USB upload also succeeded. All three boards are back online at `smoke-2026-05-15.6`, mode `0`.

## 2026-05-15 -- Ben + Codex -- LED measurement firmware loaded on COTS smoke boards

Extended `firmware/smoke_test/` into a deterministic LED measurement harness and bumped it to `smoke-2026-05-15.6`.

New serial/HTTP measurement modes:

- `q` -- quiet baseline: stop OTA/WiFi and clear LEDs.
- `0` -- LEDs off, current WiFi/OTA state unchanged.
- `1` -- center dim warm white.
- `2` -- 3-pixel RGB fringe.
- `3` -- center 3x3 dim warm white.
- `4` -- full-array very-low white.
- `5` -- full-array capped white, brief measurements only.

The OTA status page now shows the active mode and exposes `/mode?m=<mode>` links, so the USB current meter workflow can use either serial commands or `curl` while WiFi OTA is active. Added `docs/tests/COTS_LED_MEASUREMENTS_2026-05-15.md` as the worksheet for current and optics readings.

Built and uploaded `smoke-2026-05-15.6` over HTTP OTA to all three connected boards:

- C6 + IS31FL3741: `192.168.4.248`
- FeatherS2 Neo: `192.168.4.249`
- M5Stack Atom Matrix: `192.168.4.250`

All three served `Version: smoke-2026-05-15.6`, accepted `/mode?m=1`, and were left in mode `0` with LEDs off and OTA still available. LED-current readings are still open; record them in the new worksheet.

## 2026-05-15 -- Ben + Codex -- Home-WiFi web OTA validated on all three COTS smoke boards

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

## 2026-05-15 -- Ben + Codex -- COTS smoke firmware built, flashed, and serial-verified

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

## 2026-05-15 -- Ben + Codex -- First COTS prototype USB inventory and interim C6 matrix path

Three COTS prototype boards arrived and were connected over USB for first bench bring-up:

- Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 RGB LED matrix over STEMMA-QT. This is an interim substitute for the delayed PowerFeather matrix stack, useful for IS31FL3741 I2C, LED-current, OTA, and gobo/optics testing, but not a substitute for PowerFeather `VSQT`, LiFePO4 charging, fuel-gauge, sleep-current, or solar telemetry validation.
- M5Stack Atom Matrix with built-in 5x5 LEDs, USB-powered for now.
- UnexpectedMaker FeatherS2 Neo with built-in 5x5 LEDs, USB-powered for now.

USB/serial inventory on Ben's Linux bench:

- `/dev/ttyACM0` -- UnexpectedMaker FeatherS2 Neo, USB VID:PID `303a:80b5`, serial `84722E75D023`, Arduino FQBN `esp32:esp32:um_feathers2neo`.
- `/dev/ttyACM1` -- Adafruit Feather ESP32-C6 via Espressif USB JTAG/serial, USB VID:PID `303a:1001`, serial `58:E6:C5:E4:1B:2C`, Arduino FQBN `esp32:esp32:adafruit_feather_esp32c6`.
- `/dev/ttyUSB0` -- M5Stack Atom Matrix via FT232, USB VID:PID `0403:6001`, serial `8D529F3938`, Arduino FQBN `esp32:esp32:m5stack_atom`.

Local tool state: Arduino CLI is installed with `esp32:esp32` core 3.3.7. No repo firmware exists yet beyond architecture docs. No firmware was flashed during this inventory pass.

Immediate test direction: create a small USB smoke/OTA bring-up firmware before broader firmware architecture work. It should print board ID, MAC-derived fixture ID, reset reason, build version, LED driver status, I2C scan results where applicable, and OTA status. Use LiPo-only DFRobot DFR0559 tests for now and do not connect LiFePO4 to LiPo-only boards.

## 2026-05-11 -- Ben + GPT -- PowerFeather SDK 2.0.0 release confirms V2 support path

PowerFeather-SDK 2.0.0 was released shortly after the PowerFeather V2 hardware/schematic review. This is a strong positive signal that the PowerFeather developer is active and that V2 is far enough along to have first-class software support.

Key release-note items relevant to Resonance Lighting:

- Adds PowerFeather V2 board support selectable through ESP-IDF Kconfig or `POWERFEATHER_BOARD_V2`.
- Adds MAX17260 fuel-gauge support, including battery current, health, cycles, time estimates, alarms, learned-state restore, LiFePO4 mode, and custom MAX17260 battery profiles.
- Adds a shared fuel-gauge abstraction for LC709204F and MAX17260, which should let Resonance firmware support V1/LiPo fallback and V2/LiFePO4 paths behind one interface.
- Adds `BatteryType::Generic_LFP`, directly matching the project's preferred LiFePO4 chemistry.
- Adds `Board.init()` for no-battery operation and `Board.init(const MAX17260::Model&)` for custom battery profiles.
- Adds `updateBatteryFuelGaugeTemp()` overload that reads the board thermistor and updates the fuel gauge.
- V2 keeps the power-management I2C bus available while `VSQT` is disabled. This matters because Resonance wants to turn off external LED modules / STEMMA-QT loads while preserving housekeeping telemetry.
- Charger settings can be retained across RTC-preserving warm boots when battery/profile configuration still matches.
- Custom profiles now apply profile charge voltage and termination current to the charger.
- Initialization safety was improved: charger part validation, POR/watchdog recovery, profile-change detection, and full policy reapplication.
- MAX17260 LFP configuration, profile loading, learned-parameter handling, voltage alarms, and fuel-gauge reinitialization were fixed.
- Missing/open/shorted battery temperature sensors now get sanity checks.
- I2C fault latency was reduced with bounded transfer timeouts and the newer ESP-IDF I2C master driver.
- ESP-IDF requirement is now >=5.2, <=5.5.

Interpretation:

PowerFeather V2 is no longer just an attractive schematic. It now has explicit SDK support for the exact features Resonance cares about: LiFePO4 fuel-gauge mode, MAX17260 telemetry, thermistor integration, custom profiles, power-domain behavior with `VSQT` off, and improved recovery from charger/gauge initialization edge cases.

Action:

- Treat PowerFeather V2 + PowerFeather-SDK 2.x as the primary COTS LiFePO4 prototype path.
- On first hardware arrival, verify the boards are truly V2 by visual chip ID and I2C scan.
- Build first firmware with ESP-IDF >=5.2 and PowerFeather-SDK 2.x, not the older 1.x docs/examples.
- Add a small compatibility layer in Resonance firmware so PowerFeather telemetry can be consumed by the normal battery/power telemetry interface.
- Capture telemetry from BM 2026 fixtures if this platform or a PowerFeather-derived custom board is used; this data should inform BM 2027 solar/battery sizing.

Open questions:

- Does the Elecrow stock currently shipping as "ESP32-S3 PowerFeather V2" contain V2 hardware, or could it be V1 stock/listing ambiguity?
- Will the developer share V2 KiCad layout files, or only schematic/3D model?
- How well has V2 been tested with actual LiFePO4 cells under solar/VDC input?
- Does the SDK expose enough raw charger/fuel-gauge telemetry for long-term logging without significant custom driver work?

## 2026-05-10 -- Ben + ChatGPT -- PowerFeather V2 / COTS R&D update

Second-pass architecture update after COTS search, purchases, and schematic review.

### What changed

- **PowerFeather V2 is now the leading COTS/reference architecture.** It appears to match the project unusually well: ESP32-S3-WROOM-1, onboard PCB antenna, BQ25628E charger/power-path, LiFePO4 support in V2, MAX17260 fuel gauge, TPS631013 buck-boost 3.3 V rail, switchable VSQT/STEMMA-QT rail, solar/DC input, and rich power telemetry. V2 status is still preliminary until hardware arrives and is verified.
- **PowerFeather V1 remains LiPo-only as a board-level system.** V1 uses BQ25628E, but the board-level fuel gauge and regulator choices make it unsuitable for LiFePO4 production use. It may still be a strong LiPo fallback.
- **PowerFeather V1/V2 schematic diff completed.** V1 and V2 both use BQ25628E. V2 swaps the 3.3 V regulator from XC6220 LDO to TPS631013 buck-boost, swaps the fuel gauge from LC709204F to MAX17260, adds a 20 mohm current-sense resistor, and adds I2C power-domain isolation around the STEMMA-QT rail.
- **COTS purchases made.** Ben bought the R&D candidates discussed in the COTS survey except USB power meters, which are already on hand. Elecrow PowerFeather boards were ordered despite possible ambiguity about whether the listing is V2 or V1. Ben also contacted the PowerFeather creator about V2 availability and KiCad files.
- **LED module plan narrowed.** The Adafruit IS31FL3741 13x9 RGB matrix is the leading plug-and-play STEMMA-QT LED module for PowerFeather. M5Stack NeoHEX is promising optically but is WS2812/Grove, not STEMMA-QT/I2C, and likely needs a GPIO data line plus a 5 V or otherwise suitable LED rail. M5Stack Atom Matrix is a compelling all-in-one fallback with ESP32 + 5x5 LEDs + USB-C.
- **Battery sourcing narrowed.** Prefer one larger LiFePO4 cell per fixture, ideally 18650 1500-2000 mAh, instead of multiple 14430 cells in parallel. 14430 cells are easy to find and cheap, but packs of many small cells add contacts, matching, wiring, assembly, and QA risk.
- **Solar-panel plan clarified.** Square/rectangular 1-5 W panels are fine for R&D. Round panels remain aesthetically attractive for production but are harder to source quickly and should not block testing.

### Current COTS prototype tracks

1. **PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 13x9 matrix.** Primary design-aligned candidate.
2. **PowerFeather V2 + LiFePO4 + solar panel + M5Stack NeoHEX.** Alternative LED geometry test; not STEMMA-QT plug-and-play.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback: DFR0559 owns battery/solar, FeatherS2 Neo battery JST stays empty, Feather is powered over USB.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback: small ESP32 + 5x5 LEDs powered by USB from the solar manager.

### Immediate tests once parts arrive

- Confirm whether Elecrow PowerFeather boards are V2 or V1 by chip markings and I2C scan.
- Verify LiFePO4 configuration and charging behavior on actual V2 hardware before trusting it.
- Measure sleep current with VSQT off and LED modules attached.
- Measure solar harvest and charge behavior for each 1-5 W panel under sun, shade, and heat.
- Compare IS31FL3741, NeoHEX, FeatherS2 Neo, and Atom Matrix for gobo projection, brightness, color fringing, PWM artifacts, current draw, and mechanical fit.
- RF-test each candidate inside a mock hat with panel, battery, screws, and wiring in realistic locations.
- Validate fail-safe behavior: LEDs stuck on, MCU hang, watchdog reset, low-battery cutoff, and recovery from depleted battery when solar input returns.

### Follow-up docs added

- `docs/research/COTS_SURVEY_2026-05-10.md`
- `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md`
- `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`
- ADR 0015 -- PowerFeather V2 as leading COTS/reference architecture
- ADR 0016 -- Purchased COTS prototype shortlist
- ADR 0017 -- Battery cell format and sourcing
- ADR 0018 -- LED module/interface plan

## 2026-05-06 -- Ben + Claude (Cowork) -- Pre-share cleanup pass

Final cleanup before pushing the repo to GitHub and sharing with Steve and the wider team:

- **Bamboo "cone" -> "lantern" / "cylinder".** The bamboo piece is geometrically a cylinder with a steam-bent flared skirt at the bottom, not a cone. The only cone-shaped object in the project is the experimental projective-geometry filter / gobo. Scrubbed every "bamboo cone" reference across BACKGROUND, ROADMAP, README, AGENTS, glossary, ADR 0007, hardware/references, ops/bom, enclosure README. Gobo "cone" references preserved.
- **Agent-neutral voice.** Rewrote BACKGROUND.md from a Ben-addressed narrative into a third-person project-context document. Replaced "Ben (you)" with "Ben Eckart" throughout. Replaced "Dad" with "Steve Eckart" outside this LOG file.
- **Scrubbed historical / distracting context** from active docs. Removed "Critical dates" stale-deadline table from BACKGROUND. Removed crossed-out resolved items from TODO and ROADMAP. The narrative of "we initially thought X, then learned Y" now lives only in this LOG; active docs present the current state cleanly.
- **New ADR 0009 -- Minimize per-fixture operations at scale (O(1), not O(N)).** Captured Ben's strong constraint that anything done per-fixture is multiplied by 100. Specifies: no soldering on receipt; same firmware for every fixture; per-unit identity from MAC; investigate JLCPCB pre-flash service; design pogo-pin flashing jig as fallback. Reinforced in `README.md`, `hardware/README.md`, `TODO.md`. This is now the ninth and (so far) final ADR.

After this pass, the active docs (`README`, `AGENTS`, `BACKGROUND`, `TODO`, `ROADMAP`, `SYSTEM`, ADRs, glossary) read as a clean shared documentation set for Ben + Steve + future AI agents + the wider Resonance team. The journey from "what is this project" through "let's design solar lights" to "modular hat with LiFePO4 carrier board with O(1) ops" lives in this LOG.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Logistics flow confirmed: air-ship to TN, integrate at Grass Valley

Big risk-register item resolved: **Bamboo Pure is air-shipping a small batch of prototype bamboo lanterns to Steve in Tennessee.** Electronics workstream is fully decoupled from the May 10 Bali sea container. The end-to-end logistics flow:

1. Bali -> TN: prototype lanterns by air for early mechanical prototyping (Phase 2).
2. Bali -> Grass Valley, CA: tree structure + remaining bamboo by sea container.
3. Ben (CA): designs PCB, ships to Steve.
4. Steve (TN): finalizes hat enclosure with both bamboo and PCB in hand.
5. Steve -> Ben (TN -> CA): ships 100 hats.
6. Ben -> Grass Valley: drives hats + electronics to meet the bamboo container at the staging area.
7. Grass Valley: final integration. Truck to BRC.

**Updated docs:**

- `docs/ROADMAP.md` -- Phase 2 dependencies, Phase 6 rewritten as cross-country logistics + Grass Valley integration, risk register marked resolved, open dependencies list updated.
- `TODO.md` -- removed urgency on "catch Elliot before Bali," removed ship-path decision (resolved), added air-ship-timing confirmation.

**What this changes practically:**

- Phase 2 (mechanical prototyping) can start as soon as bamboo arrives in TN, not when Elliot returns from Bali.
- Phase 5 production fab no longer races a container deadline.
- Phase 6 is a cross-country logistics piece with TN -> CA -> Grass Valley flow rather than US -> BRC direct.
- Grass Valley pre-build staging area is now the canonical "integration site" terminology.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Roadmap, power-budget correction, prototyping strategy

Three additions:

**`docs/ROADMAP.md`** -- phases 0-10, working backward from BM 2026 (late August). Phase 1 (TTGO bench prototype) starts 2026-05-07 and runs ~3 weeks. Phase 3 (custom carrier board v1) lands ~2026-07-01. Phase 5 (production fab) ~2026-08-01. Risk register and open dependencies on team included.

**Prototyping strategy clarification.** The "validate the architecture before committing to LiFePO4 silicon" risk is fully mitigated by Phase 1 -- using the **TTGO T-Beam (with its built-in TP4056 LiPo charger)** as the LiPo prototype platform. No intermediate "LiPo carrier board" needed -- that would add a board spin without de-risking anything Phase 1 doesn't already cover. The CN3058 LiFePO4 charger circuit is the only chemistry-specific portion; we lift its reference circuit from datasheet, AI-review, and validate on Phase 3 v1 board with MCP73123 as designed-in fallback. (Captured in `docs/ROADMAP.md`, not yet a separate ADR -- promote to ADR if revisited.)

**Power budget correction.** Earlier estimate assumed "4 WS2812B all on at once" yielding ~10 mA LED average. Actual usage model is **1-9 LEDs per fixture, typically 1-3 lit at a time** (default ambient = 1 LED at 10%, showy = 3 LEDs at 30%, wand-burst = 9 LEDs full but rare and brief). Per-LED current scales linearly per WS2812B datasheet -- confirmed against 2018 Talisman v2 measurements on the 16-LED ring (500 mA / 16 = 31 mA per LED at full white, matching). Updated `docs/block-diagram/SYSTEM.md`:

- Per-LED reference table replaces "4-LED ring" table.
- Time-weighted nightly LED current ~5 mA (vs. 10 mA estimated earlier).
- Total daily drain ~120 mAh (vs. 170 mAh).
- Panel sizing recommendation now 1-2 W (vs. 2 W); 1 W is sufficient.
- Battery: 18650 still preferred for 12-night autonomy and 2-year life; 14430 (~3 nights) now reasonable if cell sourcing forces it.
- BOM updated for 1-9 LED count per fixture.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Handoff documents

Before switching to Claude Code for daily iteration, dumped context to handoff-friendly artifacts so future agents (Ben's Claude Code, Steve's Claude Code, Elliot's Co-Work, future Cowork sessions) can pick up cold:

- `AGENTS.md` at root -- explicit preamble for any agent picking up this repo. Read order, who's working, what's known vs assumed, what the repo does NOT cover, when to ask Ben.
- `docs/block-diagram/SYSTEM.md` -- the canonical system architecture. ASCII block diagram, voltage rails, current draw table grounded in 2018 Talisman v2 measurements + ESP32-C3 datasheet, single-fixture daily power budget (~170 mAh/night, well covered by 2 W panel + 1500 mAh 18650), back-of-envelope max-stress check for wand-interaction events. Cost-comparison sketch vs `INV_2026_00401`.
- `docs/decisions/` -- eight ADRs: ESP32-C3-MINI-1 (0001), LiFePO4 chemistry (0002), CN3058 charger (0003), ESP-NOW mesh (0004), FreeRTOS task architecture (0005), custom PCB not dev-board-on-carrier (0006), modular hat enclosure (0007), WS2812B from Vbat with no level shifter (0008).
- `firmware/ARCHITECTURE.md` -- RTOS task decomposition (`led_render_task`, `ca_tick_task`, `mesh_tx_task`, `mesh_rx callback`, `housekeeping_task`), inter-task communication via FreeRTOS queues + atomic shared state, sleep behavior, boot sequence, OTA strategy.
- `hardware/atopile/EXAMPLE.md` -- sample atopile module (`voltage_regulator.ato` for the AP2112K-3.3 LDO) so the schematic-as-code pattern is concrete. List of modules to build.
- `ops/bom.md` -- first-pass BOM grouped by carrier-board electronics, non-PCB electronics, and mechanical. Per-fixture target ~$23. 100-fixture total ~$2,310.
- `docs/glossary.md` -- proper nouns and acronyms for new agents dropping in cold.

These files are now the canonical project context outside this conversation. The earlier `BACKGROUND.md` remains the long-form narrative.

Switching to Claude Code from here. Cowork retains read access to this repo via GitHub (when pushed) for review and project management.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Repo bootstrap

Stood up this repo. Ported `BACKGROUND.md` from earlier Cowork session -- captures full project context, team, decisions to date, prior-art lessons from 2018 Talisman v2 build, code reusable from `beneckart/future-robotics`, and the design space for this year (electronics architecture, mandala filter program, mesh creative possibilities).

Decisions baked in so far (subject to team review):

- **MCU:** ESP32-C3-MINI-1 for production. Prototype on TTGO T-Beam and T-Ice modules already in Steve's workshop.
- **Battery chemistry:** LiFePO4. Chosen for thermal tolerance in desert deployment.
- **Charger IC:** CN3058 (LiFePO4-tuned, JLCPCB basic part, ~$0.30). Rejected TP4056, bq24074, CN3791 -- all LiPo-tuned, wrong charge profile.
- **3.3 V LDO:** AP2112K-3.3 (450 mV dropout, JLCPCB basic part, fits LiFePO4's 2.5-3.6 V range).
- **LEDs:** 1-4 WS2812B per fixture, powered direct from battery rail (3.3 V GPIO satisfies WS2812B's 0.7 x Vcc threshold per Talisman v2 verification).
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
3. Bench validation on existing TTGO modules -- solar charging path first.

Switching to Claude Code for daily firmware/hardware iteration. Cowork retains read access to this repo via GitHub for project management and review.
