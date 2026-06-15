# LOG

Append-only session journal for the Resonance Lighting workstream. Most recent first.

Format per entry:

```
## YYYY-MM-DD — author — short subject

Body. What changed, what was decided, what's next.
```

---

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

## 2026-06-12 (cont. 3) — Ben + Claude — Interactivity/presence sensing: option space mapped (Elliot ask)

Elliot (project lead) saw the 06-11 LED demo and asked for presence detection /
interactivity — "what makes people spend quality time at the tree." Full landscape in
`docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md`; headlines: it's ART not
security (false positives are benign -> ~80 % reliability = success); the product is the
MESH choreography (PRESENCE event + ripple, the packet layer already fits); primary
candidate = downward VL53L1X ToF eye (sway-robust, ~$3, ~zero power, but needs a port
next to the gobo aperture — Steve); radar = through-enclosure (no dust exposure) but
LED-show-class power unless duty-cycled + self-sway artifacts (IMU veto); the FREE
experiment = mesh-RSSI presence (bodies attenuate 2.4 GHz ~20 dB, already in every
heartbeat). Bench kit ~$10, test plan Steve-compatible, TODOs queued.

## 2026-06-12 (cont. 2) — Ben + Claude — HEX 4.2 V boost direction (TPS63802); revised HEX budget; Steve-runnable bench TODO pushed

Remote session while Ben travels. Two outcomes, both queued as Steve-runnable TODOs
(Steve has duplicate components + Claude Code on his end; data site code `tn`):

**Revised HEX budget — the gobo looks are cheap.** Ben's verdict: HEX looks best as
1 px white or 3 px single-channel (plus trails). Measured: 1 px full = 41.8 mA
(~0.12 W rail), 3 px ~105 mA (~0.3 W) → ~0.4-0.6 W battery-side with overhead =
**all-night on the 3 W panel, in-tree**. Yesterday's 2.1 W HEX row was the all-37 case
only; in its actual role the HEX lantern runs as cheap as the RGBW one.

**4.2 V V+ boost (TPS63802) — why and how.** At the sagged ~2.9 V rail the SK6812
blue/green drivers are in dropout (Vf 3.0-3.2 + ~0.5 V headroom needed) → starved
channels = the goldening + ~25-30 % current deficit. A regulated 4.2 V V+ should give
**~+40-60 % white lumens** (blue/green recovery, V(lambda)-weighted), restore color
balance, and make looks **SOC- and fixture-invariant** (also quietly solves the
Community-Mandala brightness-normalization concern). Key constraints discovered:
- **4.2 V, NOT 5 V**: WS-data VIH = 0.7 x VDD; 5 V supply → 3.5 V threshold breaks
  3.3 V GPIO data. 4.2 V → 2.94 V = in spec, no level shifter.
- **TPS63802 module** (TI buck-boost, 1.3-5.5 Vin, 2 A, output-select solder jumpers):
  re-bridge 3V3→4V2 (fully open 3V3 first; meter unloaded). Cheap boards don't break
  out EN (tiny pad only) → bench version feeds from the **switchable 3V3** (kill-switch
  inherited via GPIO4); the **VBAT-fed + EN-on-GPIO** single-conversion variant is the
  production architecture, to live on the NeoHEX adapter PCB rev. PS pad = power-save
  mode select; leave default (PFM efficiency matters at dim ambient).
- **Count-cap required on boosted builds**: all-37 white at regulated 4.2 V ≈ 2 A out —
  far beyond module + rail. Firmware n-cap before anyone maxes "all".
- Rail-vs-pixel bottleneck clarified: at 1-3 px the limit is per-pixel undervolt (boost
  fixes); the converter/rail limit only binds at high counts.
- LM2596-class = buck, wrong direction; MT3608-class = acceptable fallback (pot-set,
  drift risk — preset jumpers preferred for fleet).

Decision data wanted from the bench (Steve): PAR + INA lumens-per-system-watt, 2.9 vs
4.2 V, 1/3 px, per-channel, at two SOCs. TODO has the full procedure.

## 2026-06-12 (cont.) — Claude — BQ25628E datasheet read: the Voc ceiling is a REGISTER BIT; "6V" panel class unblocked

Datasheet (SLUSFA4C) electrical characteristics resolve yesterday's bright-sun latch and
the panel voltage window:
- **V_VBUS_OVP is selectable**: VBUS_OVP=0 (POR default) -> 6.1/6.4/6.7 V rising;
  VBUS_OVP=1 -> 18.2/18.5/18.8 V. Our connect-time Voc 6.15 V tripped a min-spec part at
  the default setting. Chip operating range is 3.9-18 V (26 V abs max) — the ~6 V ceiling
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
  Vmp(STC) >= ~5.4 V, Voc <= ~16 V — the standard 6 V class (incl. Voltaic's 6 V ETFE
  line) is fully in-window. Current was never a constraint (2 A charge ceiling; charger
  draws only what it needs).

## 2026-06-12 — Ben + Claude — Gobo verdict: BOTH LED types, by role; full-brightness budget sketch

**Gobo session result (Ben, inverted-lantern rig, dark):** both modules are excellent for
DIFFERENT roles — the LED-axis answer is a MIXED FLEET, not a winner.
- **HEX (37x SK6812):** beautiful animations, dancing patterns, the color-channel
  separation (Split) modes shine — but it reads best within ~6 ft; at 10-15 ft the color
  washes out and patterns lose crispness. The intimate/close-range module.
- **4 W RGBW point source:** crisp and beautiful even at 15 ft; the color fringing acts
  like a Venn diagram — overlap regions mix into NEW colors, far richer than plain
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
REJECTED — which re-explains the 06-11 "4.4 V collapse": those points measured a stale
setpoint, not 4.4; LOG cont. 2/3's below-the-knee story is corrected accordingly, and the
4W-cam panel's "flat no-knee curve" below 4.6 was the same artifact). Voc ceiling: input
qualification failed at ~6.05-6.15 V and latched (the bright-sun gotcha), accepted 5.43 V
-> as-configured ceiling ~6 V-ish; datasheet ACOV verification is now PROCUREMENT-GATING
(if fixed ~6.3 V, the standard "6V" panel class (Voc 6.8-7.4) can never qualify at
open-circuit and no firmware kick saves it; viable window narrows to Voc(STC) <= ~5.8 =
the Seeed's class). Current is a non-issue (BQ = 2 A charge max; charger draws only what
it needs). Voltaic P139 (Voc 2.76) = boost-ecosystem class, unusable on a buck charger.

## 2026-06-11 (cont. 3) — Ben + Claude — 32700 VERDICT: 5726 mAh (95 % of rating) = the production cell passes; "4 W" camera panel = a 1 W panel in a trench coat

**32700 6 Ah LFP capacity (the production-cell gate): 5,726 mAh clean to a 2.473 V cutoff
over 7.16 h, ZERO resets.** Stitch: run 1 981 + ~6 (gap at ~124 mA idle after run 1's
parse crash) + run 2 4,739; **7 corrupt-but-parseable INA samples ablated** (−256 to
−343 A class; the raw script integral read 10.9 Ah — reconcile-before-believing, again).
Cross-check: the gauge's own run-2 integral came out **+8.5 % above the clean INA** —
the MAX17260 current bias replicating for the **6th consecutive session** (+8 +-1 %,
both directions, both cell types; the /1.08 software correction is now very solid).
**Verdict: PASS at $5.10 ($0.89/delivered-Ah)** — 95 % is ratings-tolerance territory on
a first cycle with a conservative cutoff. Qualify a 2nd sample from the batch before the
100-unit order (n=1), but this is the production cell unless that surprises. Notable:
under the fading HEX load the cell rode the whole tail gracefully (load self-dimmed
605 -> ~250 mA as the rail sagged) — zero brownout resets, vs the mule's 44-reset
cascade under the stiffer RGBW point-source load.

**"4 W" ring-camera panel bake-off (Ben's economy-of-scale candidate): rejected, with
numbers.** Voc only ~5.45 V hot at the connector (10-cell panel + blocking diode — Ben
visually confirmed diode-only in the housing). Flat-mounted: a dead-flat ~0.28 W from
VINDPM 4.6 down to 4.0 (current-source-starved, ~65 mA — no knee at all). Tilted
square to the sun at 4.6 V: 0.579 W in ~57 klux — scaled to full sun ~1.0-1.1 W real
capability = **~4x overrated**, plus bezel self-shading when flat (tilting doubled
output, more than geometry alone explains) and the diode tax. Apples-to-apples the
Seeed 3 W delivers ~4x the real harvest. The 10-minute sweep harness is now the
panel qualifier: any candidate (incl. the ETFE panels) earns its place through it.
(Sweep-tooling fixes from the session: anchor-all-zero ZeroDivision guard; "restore
5.5 V on exit" bit us twice when the next panel's window sat below 5.5 — the live
check is `sgood=1` + `sma=0` = setpoint above the panel's window, send `m46`.)

Misc: lux-sensor bump mid-session produced a fake 30x light drop (the 3.5 klux
"shade" reading) — worth a mount for the TSL. Sun-angle context for today's numbers:
3:40 pm flat-mount cosine loss ~18 % + cell ~65 °C temp derate ~16 % fully explains
"2 W from a 3 W panel" — the Seeed performs AT rating once physics is applied.
Tomorrow: cool-AM Seeed sweep (Vmp(T) -> MPPT decision), then a dawn-to-dusk harvest
log = measured effective-solar-hours (the fixture-specific derate of Ben's 5-h
heuristic; pre-derate estimate ~2-3 h flat, ~1.5-2.5 in-tree).

## 2026-06-11 (cont. 2) — Ben + Claude — FIRST WIRELESS MPP SWEEP: hot-panel optimum 4.6-4.7 V = ~3x the default harvest; bright-sun input-latch gotcha; 32700 verdict pending

**The harvest question (the sizing campaign's last unmeasured term) now has its hot-panel
answer.** Full instrument cluster on the outdoor peer — TSL2591 (lux; saturated in full
sun, IR ch1 used as the normalization channel), SHT31 taped to the panel back
(60-61 °C; IR gun front 155-157 °F ≈ 68 °C → ~8 °C front-to-back offset, cell ~64-66 °C),
and Ben's idea of SEN0291 INAs on the peer's OWN STEMMA bus (panel + battery leads,
heartbeat tail 3, fw 2026-06-11.2) — zero outdoor tether, all data over ESP-NOW.

- **Curve (bright sun ~3:40-4:15 pm, panel ~60 °C back):** 5.5 V → 0.59 W; 5.2 → 1.18;
  5.0 → 1.45; 4.9 → 1.55; 4.8 → 1.66; 4.7 → 1.69; **4.6 → 1.73 W (best, both sweeps)**.
  The default 5.5 V harvests **31 % of optimum (3.2x available)** — worse than the
  06-08 cloud-confounded ~2.6x hint. Knee-bracket re-sweep reproduced 4.6 as peak
  (anchor drift 5 % with the improved 4.9 anchor).
- **Panel INA vs BQ telemetry: the BQ under-reports harvest ~10 %** (1.91 W panel-side
  vs 1.73 BQ-side at the peak) — input-stage loss the sizing math must include. The
  self-instrumentation cross-check earned its keep on day one.
- **Below the knee:** sweep 1's 4.4 V point COLLAPSED (0.55 W, input parked near Voc —
  VINDPM below the hot panel's knee has no stable operating point when stepped to from
  near-idle); the re-sweep's 4.4 read 1.53 W but DEMAND-LIMITED (4.9/4.5/4.4 all
  identically 1.53 W late-session — battery filling toward ~50 % and/or charger thermal
  foldback in the heat caps demand, making VINDPM moot). Conservative rule: **fixed
  setpoint >= 4.6 V hot, approach setpoint changes from above; run sweeps on a hungry
  battery.** Cool-AM session pending for Vmp(T) → the fixed-vs-temp-comp-vs-P&O call.
- **NEW FIELD GOTCHA (production-relevant): connect/boot under bright sun latches the
  charger's input fault** — panel sat at Voc ~6.0-6.2 V, sgood=0, zero draw; a hand-
  shade to 4.7 V did NOT clear it; only full VBUS removal (face-down/unplug) re-ran
  qualification. Captured in POWERFEATHER_NOTES + firmware-guard TODO (playa bring-up
  hazard at 100-fixture scale). Anchor methodology fix: anchor at 4.9 not 5.5 (the 5.5
  point is load-noise-dominated; first sweep's 25 % "drift" was that artifact).

**32700 capacity run (in progress at write time):** three CORRUPT-BUT-PARSEABLE INA
samples (-256 to -343 A, physically impossible) inflated the live integral — caught by
reconciling the integral against instantaneous currents; clean re-integration =
**~4.8 Ah by the knee region, final pending the fading tail**. (An earlier in-chat
"~7 Ah, above rating" read was glitched data — retracted within the hour. The 06-10
lesson again: reconcile integrals before believing them.) All four INA host scripts now
drop both mangled lines AND beyond-range values (the INA's +-4 A) at ingest. Notable
along the way: the HEX load FADES gracefully as the rail sags (zero resets in 5.6 h,
vs the RGBW point source's 44-reset brownout cascade on the mule) — the two LED
architectures fail differently at end-of-charge.

Also: 9F2690 reflashed as serial-bridge master; mpp_sweep gained ir-ch1 fallback
(TSL2591 saturates in full sun even at min gain — expected); peer INA address
convention: both-DIP-off = 0x40 = panel, both-on = 0x45 = battery (0x44 = SHT31).

## 2026-06-11 (cont.) — Ben + Claude — Pushed the HEX to the cliff: visual failure sequence mapped; protect latch validated live; 32700 charging for capacity test

**The aggressive ramps (Ben watching, ~20 % SOC mule cell).** After the guarded runs,
floors were dropped near hardware limits and the value ramp walked 37-px white from
141 mA up. Results, all INA ground truth:
- **Sustained ceiling ~480 mA** (val 208) at ~20 % SOC — far above the morning's
  conservative floors; rail rode at 2.7 V (min 2.53) for whole steps without electrical
  failure. The step toward ~500 mA (val 224) ended it: **the firmware battery-floor
  protect latched mid-step** (rail CUT to 0 V, LEDs unloaded, WiFi off, no self-rejoin —
  the designed endpoint, needing a button; the brownout-reset path self-recovers). Every
  guard layer fired in design order across the night: script floors first, fw protect as
  the backstop, zero bricks.
- **Hot-step vs ramp asymmetry, quantified:** an idle->290 mA hot-step (n=10 @ full)
  brownout-reset the board instantly (rr=poweron) at the same SOC where a gradual ramp
  survived 480 mA — a ~1.7x margin difference. "Ramp gently / no full-white hot-steps"
  is now a measured production rule, not folklore. (The danger zone also slid ~100 mA
  down as SOC fell 98 %->20 % — the current cap must be SOC/voltage-aware or worst-case.)
- **Visual failure sequence (Ben, thick packing foam as diffuser — too bright naked-eye):
  (1) subtle flicker (onset before any electrical flag), (2) subtle "goldening" of white
  (blue channel — highest Vf — starves first as the rail sags), (3) uneven lighting where
  the brightest CONTIGUOUS run of pixels jumps around every few seconds** (WS-protocol
  data corruption: pixels keep whichever frame last latched cleanly), then (4) the
  protect cut. All graceful-degradation modes — nothing alarming below the cliff, which
  supports dim-don't-die low-battery behavior.
- First aggressive attempt (file `0549`) also caught the n=10@255 hot-step brownout live
  (board uptime reset mid-ramp; script bug fixed: failed /set now prints + retries once).

**32700 6 Ah LFP candidate — charging overnight.** Board 9F2690 (the former bridge
master) flashed `power_bench --led none --cap 6000 --chem lfp` BEFORE cell connect (the
LFP flash-order rule), then the 32700 attached: charging at **+515 mA** (USB input-
limited), bv 3.328, supply_good. Gauge says SOC 99 — ignore (un-learned, cycles=0,
plateau-blind); the REAL capacity test is tomorrow's full charge -> INA-coulomb
discharge (the validated 06-10 methodology). At fullbattery.com bulk ~$5.10 (~$0.85/Ah)
it's the leading production cell if it makes rating (ADR 0017 direction). Note: this
board's Wire1 scan shows an extra mystery device at 0x2A (others have only 0x36/0x6A) —
harmless so far, noted in case the board ever behaves oddly.

**Bench state for tomorrow's MPP sweep:** mule 2000 cell DISCONNECTED at ~20 % SOC
(ideal bulk-charge precondition); 9E5B0C powered off; a spare board still needs the
serial-bridge master flash (9F2690 got the new master fw tonight but was immediately
repurposed for the 32700); TSL2591 + SHT31 arrive early PM. The 32700 discharge can run
on the bench INAs in parallel with the outdoor sweep — load/wiring decided in the
morning (HEX board swap vs RGBW on 9F2690).

## 2026-06-11 — Ben + Claude — Loose-ends night: RGB-3W = RGBW-4W on RGB; STEMMA cable verdict; HEX ground truth + the "all-on-max" instability explained

Bench session while waiting on the TSL2591/SHT31 delivery. Three loose ends closed.

**1. RGB-3W vs RGBW-4W: identical RGB top-end.** The new 3 W RGB module (no W channel)
at full r=g=b drew **256.5 mA** (3 cycles, +-0.3 mA) vs the 4 W RGBW's RGB-full
**257.3 mA** at the same ~2.8 V sagged rail — 0.3 % apart. The "3 W vs 4 W" rating
difference is entirely the W channel (+66.5 mA standalone, +34 mA on top of RGB under
combined sag). W pattern on the new LED: ~0 mA (no channel, and the 4-byte RGBW frame
drives a 3-byte pixel fine — first three bytes land). Shunt NOT backwards (Ben's worry):
both INA channels kept the original sign convention. Caveats: n=1 of each module;
tonight USB+charging vs 06-10 battery (rail sag happened to match, making it fair).
Data: `2026-06-11-afk-sweep-0028.jsonl` + `-power/gauge.png`.

**2. Metro STEMMA port: the CABLE was the whole story (port healthy).** The Metro
ESP32-S3's QT port is the same I2C bus as the headers (SDA=47/SCL=48, 10 k pullups, no
power gate; 0x36 on the bus = the Metro's own onboard MAX17048). STEMMA QT (GND,VCC,
SDA,SCL) and Gravity PH2.0 (VCC,GND,SCL,SDA) are **pairwise-inverted**, so a
straight-through adapter lands ALL FOUR pins wrong — power reversed (the 06-09
dead-short/USB-kill incident, explained) AND SDA/SCL swapped. Ben re-matched the leads
→ all 4 INAs + the MAX17048 found and streaming **through the QT port** (which also
proves the port survived the 06-09 short). `ina_monitor` gained `s` (I2C scan) / `r`
(re-probe) serial commands for future bus debugging.

**3. HEX (37x SK6812) with INA ground truth — and the "all-on-max instability" is a
BATTERY-SAG ceiling, not an LED/data failure.** power_bench `/set` gained `n=` (light
first n pixels; fw 2026-06-11.1, OTA'd battery-only — another no-touch flash) and
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
  mA/px at n=14), so all-37-full would NOT hit 37x41.8 — but the cell dives first.
- Implications: (a) Ben's observed all-on-max instability = battery sag to the
  brownout zone; rail/data stayed fine to the guard floors (rail mean >= 2.79 V).
  (b) The ceiling is a CELL property (high effective IR incl. harness, ~0.7 ohm at
  0.5 A) — the production 32700 ~6 Ah cell lifts it substantially. (c) Production
  firmware needs a **current cap** (brightness x lit-count) for burst modes —
  reinforces the existing cap-brightness TODO / ADR 0013 failsafe. We deliberately
  never drove it to an actual reset; the guards stop at early-warning floors, and the
  visual-flicker threshold (if lower) is a separate observation.
- Gauge current bias replicated again: **+7.4 %** this session (and +8.8 % on the
  CHARGE side in the morning run) → ~+8 +-1 % across 4 sessions, both directions;
  the /1.08-ish software correction is solid.
Data: `2026-06-11-afk-sweep-0119.jsonl` (+pngs), `2026-06-11-hex-ramp-0128.jsonl`
(0126 = aborted first try whose floors were miscalibrated to transient WiFi dips —
kept for the record).

## 2026-06-10 (cont. 2) — Ben + Claude — MPP sweep goes fully wireless: TSL2591 lux + SHT31 panel-temp ride the heartbeat

Ben flagged the sweep's weak point: the Apogee PAR sensor is USB-tethered, so logging
light outdoors meant a laptop or a dedicated rpi at the panel. Three options weighed:

- **PowerFeather USB-C as USB-host to the Apogee: rejected.** The S3 silicon can do OTG
  host, but this stack runs TinyUSB in device mode, the Apogee is an FTDI-class device
  (needs a vendor VCP host driver under ESP-IDF, not Arduino), and the V2's USB-C is a
  charge/device input that doesn't source VBUS — the sensor wouldn't even power up. A
  research detour, not a bench fix.
- **TSL2591 I2C lux module (arriving ~06-11): ADOPTED as the primary light channel.**
  Chained on the peer's STEMMA-QT, auto-probed at boot, lux appended to the heartbeat
  (append-only tail 2, `NB_PROTO_VER` unchanged — same pattern as the supply fields) →
  light data arrives over ESP-NOW with **zero outdoor tether**. Note "lux vs PAR": neither
  matches the panel's silicon spectral response — both are *relative* normalization
  channels, which is all the sweep needs (absolute W comes from the anchors agreeing).
  The TSL2591's raw ch0/ch1 (full+IR) are logged too. **Saturation caveat:** full sun
  (~100k+ lux) can exceed its range even at min gain/integration; firmware detects and
  reports `lux=sat`; the fix is a paper/PTFE diffuser (fine for relative use). The Apogee
  remains an optional host-side cross-check for the indoor dry run (`--par-port`).
- **SHT31-D taped to the panel BACK: ADOPTED for continuous panel temp** (back-surface
  contact ≈ cell temp − a couple °C in sun; standard PV practice) → `ptc=`/`prh=` in the
  heartbeat. The IR gun stays as the front-surface spot-check at anchors (the script
  still prompts). **Battery NTC** (the V2's 103AT thermistor on the charger TS pin) is
  exposed too (`btc=`) but **opt-in** (`--batt-ntc`): enabling TS with no thermistor
  attached makes the BQ apply JEITA to a floating pin and can SUSPEND CHARGING — gotcha
  captured in POWERFEATHER_NOTES. With the NTC taped to the cell it doubles as hardware
  LFP charge-temp protection (a thermal-track freebie).

Implementation (compiled both roles; on-hardware validation when the sensors arrive):
`net_bench` fw 2026-06-10.1 — env auto-probe + 1 Hz cache (TSL2591 read blocks ~120 ms,
so high-rate heartbeats reuse the cache), heartbeat tail 2, master bridge prints
`lux=/ch0=/ch1=/ptc=/prh=/btc=`; `net_bench_log.py` + `mpp_sweep.py` + `mpp_analyze.py`
parse them (host tooling re-validated end-to-end against a simulated master emitting the
new tokens). Sweep flags generalized: `light-saturated`, `light-unstable`, `no-light`.

## 2026-06-10 (cont.) — Ben + Claude — MPP-sweep tooling ready (next bench test); buck-boost show-load finding from existing data

**Decision: the next bench test is the clean full-sun MPP sweep** (the open TODO from
06-08 cont. 10). Rationale: with capacity, idle, and LED draw now measured, harvest is the
last unmeasured term in the battery/panel sizing equation — and the dirty 06-08 sweep
suggests the default VINDPM 5.5 V may give up ~2x vs the hot-panel MPP (~4.85 V), i.e. a
potential ~2x panel-sizing error at 100 units, plus it settles the MPPT firmware decision.
Runner-up was the gobo session (evening-compatible, doesn't compete for sun).

**Tooling built + validated (no hardware in the loop yet):**
- net_bench master `m<v10>` — explicit SET_MAINTAIN setpoint (e.g. `m48` -> 4.8 V) next to
  the bare-`m` cycle; range-checked to the peer's 40–58 accept window. Compiles; reflash
  the DESK master over USB — the outdoor peer needs nothing.
- `ops/bench/mpp_sweep.py` — guided session: anchor (5.5 V) re-visited every 3 points so
  light/temp drift is measured rather than silently corrupting the curve (the 06-08
  lesson); 3x re-send of the unacked SET_MAINTAIN broadcast; Apogee PAR sampled each
  heartbeat + IR-temp prompts; dark-panel + PAR-instability flags with a redo offer;
  restores 5.5 V on exit; relays nb-* to UDP so net_bench_log co-records. Validated
  end-to-end against a pty-simulated master (recovered a synthetic IV peak at 4.8 V).
- `ops/bench/mpp_analyze.py` — PAR-normalized P-vs-VINDPM per session, anchor-drift
  report, best-setpoint + "what fixed 5.5 V gives up" ratio, Vmp shift cool-vs-hot.
Procedure (also in TODO): SOC <~60 % first (charger must stay in bulk/CC), indoor
window dry run, then cool-AM + hot-midday sessions on a stable-sun day.

**Buck-boost finding from EXISTING data** (`ops/bench/bb_efficiency.py` on the 06-10
full-discharge JSONL; closes part of the "efficiency vs VBAT" TODO without bench time):
at full-RGBW show load the LFP **terminal** voltage sags to ~2.9–3.05 V, so the TPS631013
ran in **boost for the entire pre-brownout discharge — the 3.25–3.35 V buck/boost
crossover was never visited under load.** Overhead (ESP+WiFi+converter, not separable
with this instrumentation) ~0.48–0.52 W and roughly flat; P_led/P_batt lower bound
0.61–0.64. Reframes the chemistry-tax concern: no crossover/mode-hunt tax at show loads;
the residual open regime is the production **ambient** load (tens of mA), where the
plateau terminal V (~3.2–3.3 V) does sit near the crossover. Caveats: n=1 cell/board/
load; fine structure vs VBAT may be time-confounded (WiFi activity); plot
`data/ca/2026-06-10-discharge-1357-bb-eff.png`.

## 2026-06-10 — Ben + Claude — Full discharge: bench LFP is AT/ABOVE its 2000 mAh rating (capacity vindicated); gauge learn cycle + brownout failure mode

**Capacity, finally measured (gauge-independent).** A full charge→empty discharge on battery
(`afk_discharge.py`, full-RGBW ~467 mA load, INA 0x45 coulomb integration) delivered **~2077 mAh
to a 2.5 V cutoff over 280 min, SOC 98→0 %** → ≈ **2119 mAh** at 100 %. The bench "2000 mAh" 18650
LFP is **at/above its rating** — every earlier under-capacity claim (the 06-09 "~760 mAh" slice,
the older "~1000 mAh / 2× overrated") is **dead.** Ben's skepticism + the reputable-dealer prior
were right; the low numbers were entirely the un-learned, plateau-fooled gauge + my slice
extrapolation. (Production targets a different cell — LFP 32700 ~6000 mAh — so this is methodology
validation, not a product sizing number.)
(Data note: one spurious INA-0x45 sample — a −21 A I2C/serial glitch at 138 min — had inflated the
logged integral to 2144 mAh; `afk_analyze` now ablates it + re-integrates → 2077 mAh. LED & gauge
were normal at that instant, so it was a lone read glitch, not a real transient.)

- **Usable under full LED load: ~1971 mAh** before the first brownout (first reset, bv 2.97; LED
  held full to ~2045 mAh, bv 2.80). The brownout cascade is confined to the last ~100 mAh.
- **Gauge vs INA (this run IS the learn cycle):** current bias **+8.3 %** high (median, |INA|>50 mA);
  coulomb **+7.9 %** (gauge 2241 vs clean INA 2077 mAh) — now consistent with the instantaneous
  bias (the glitch had masked it at +4.5 %). Gauge SOC hit 0 % at ~1977 mAh with ~100 mAh (~5 %)
  still left — mildly pessimistic at the tail but respectable for an un-learned LFP gauge.
  **DesignCap 2000 is ~correct** (measured ~2119) — the SOC flakiness was UN-LEARNED gauge, NOT a
  wrong DesignCap (retracting the 06-09 "set DesignCap ~760"). This discharge + the recharge = a
  full learn cycle; re-check SOC accuracy on the NEXT cycle.
- **Gauge SOC shape (Ben's read):** SOC held at **1 % across the whole voltage knee** (where
  dV/dQ steepens), and the **1 %→0 % step coincided almost exactly with the brownout onset** — a
  usable "really empty now" signal even though the flat plateau hides SOC elsewhere.
- **Failure mode (intended, aggressive):** 44 brownout-reboots in the deep knee — under the
  ~467 mA full-RGBW load, once the cell sagged below ~2.97 V the board couldn't hold ESP+LED+WiFi
  → reboot cascade (draw fell to ~145 mA). **LEDs went unstable ~2.7 V but the board kept running
  to ~2.5 V.** Bounded by the `--batt-floor 2.3` build + the script's 2.5 V cutoff; recovered fine
  on USB (charger precharge/trickle at 2.56 V). **Production lesson: set the low-battery cutoff
  well ABOVE the heavy-load brownout point.**

Tooling: `afk_discharge.py` (fixed-load coulomb run, reset-tolerant, waits-for-unplug),
`build.sh --batt-floor`, `afk_analyze.py` (constant-load runs + robust median gauge bias + glitch
ablation/re-integration). Plot: `ops/bench/data/ca/2026-06-10-discharge-1357-gauge.png`.

## 2026-06-09 (cont.) — Ben + Claude — SEN0291 wattmeter read 10× low (0.1 vs 0.01 Ω shunt); fixed, cross-checked, AFK gauge-cal sweep launched

**The "400 mA (power_bench) vs 36 mA (wattmeter)" mystery was a units bug, not a measurement
conflict.** Same current, 10× apart: `ina_monitor` computed `ma = shunt_mv / INA_RSHUNT_OHMS`
with `INA_RSHUNT_OHMS = 0.1` (the INA219 *reference* shunt), but the **DFRobot SEN0291 hardware
shunt is 10 mΩ (0.01)**. Every current it ever printed was **10× low**. The gauge was right.

**Evidence (convergent):**
- Datasheet: SEN0291 = "10 mΩ alloy shunt", ±8 A, **1 mA resolution** — and 1 mA = INA219's
  10 µV LSB ÷ 0.01 Ω. The resolution spec only closes at 0.01 Ω, not 0.1.
- Live reconcile (`ops/bench/reconcile_ina_pf.py`, W-full): INA reported 6.7 mA → ×10 = 67 mA;
  PF battery-current delta = 81 mA → ratio **~12×** (datasheet says 10; excess = WiFi-TX bursts
  in the gauge average + sagging rail + a noisier INA on the sag). Order of magnitude confirmed.
- Battery cross-check (INA 0x45 = battery line vs gauge): off −121 vs −138 mA; RGBW-full
  −461 vs −502 mA.
- `SYSTEM.md` already had RGBW at 400–500 mA; and this day's own puzzling "wake ≈ 11 mA … far
  under the 168 mA RX" → **×10 = ~110 mA**, resolved.

**Fix:** `ina_monitor.ino` → `INA_RSHUNT_OHMS 0.01` (Metro reflashed). **All prior INA numbers/
plots ×10** — incl. the "11 mA/0.6 s wake" (→ ~110 mA) and the led-ina-sweep PNGs (regenerated
×10). Sub-mA sleep floor stands (still below range; relabel only). At 0.01 Ω, PG=/1 = ±4 A range,
1 mA/LSB (the old "caps ~400 mA" comment was the 0.1 Ω artifact). Raw `shunt_mv` was always
logged, so historical JSONL is recoverable by ×10 without rewriting it.

**Tooling for the AFK gauge-cal run:**
- power_bench gained `/set?r&g&b&w&bri&gamma` (arbitrary single-pixel drive, per-channel gamma)
  + an unattended **battery-floor guard** (on battery, sustained <2.90 V → cut the 3V3 LED rail +
  WiFi; non-bricking, reset/USB recovers). Reflashed via USB. (Gotcha: the post-flash RTS reset
  left the 3V3 rail off — needed a physical reset, per POWERFEATHER_NOTES.)
- `ops/bench/afk_sweep.py`: loops {RGB,W,RGBW}×{gamma 0,1}×levels logging INA 0x41 (LED), INA
  0x45 (battery) and gauge telemetry per point, with a **coulomb-budget cutoff** (sag-immune; a
  voltage floor false-trips — the cell sags to 2.99 V at 460 mA even at 33 % SOC). Launched
  battery-only, 200 mAh budget.

**Run results** (`afk_sweep.py` → `afk_analyze.py`; 814 pts, 54.8 min, 13 cycles, battery
33%→9% SOC; stopped on the 200 mAh coulomb budget; plots `*-power.png` / `*-gauge.png`):
- Corrected LED draw: W-full ~63 mA, RGB-full ~250 mA, RGBW-full ~290 mA. Full-scale is
  RAIL-SAG-limited: the LED bus droops to ~2.84 V under load (on USB *and* battery alike) so the
  SK6812 channels lose headroom; current was flat-to-slightly-rising over the run (254→259 mA RGB)
  i.e. NOT SOC-limited here. Gamma cleanly separates the mid-range (RGBW lvl 64: 70→9 mA).
- **Gauge current bias** (n=814): gauge = 1.080·INA + 2.4 mA, mean ratio 1.094 → reads **~+9 %
  high** vs INA ground truth → software-correct gauge current by ×0.926 (or trim the MAX17260
  sense-R). Instantaneous gauge battery_ma is noisy/laggy; INA 0x45 is steady.
- **Coulomb**: gauge integrated 200 mAh vs INA 183 mAh over the run (gauge +9 %, matching the
  current bias).
- **Capacity: NOT determined — the earlier "~760 mAh" was an overreach (Ben pushed back, rightly).**
  Gauge SOC fell 33→9 % over 183 mAh (INA), but the **resting voltage stayed flat at 3.190→3.186 V**
  (LED-off, ~120 mA) the whole run — we never reached the LFP knee. So we have NO read on remaining
  capacity: 183 mAh could be ~24 % of a small (~760 mAh) cell OR ~9 % of the rated 2000 mAh with a
  gauge that over-drops SOC on the flat plateau — a mid-plateau slice can't distinguish them, and
  the un-learned LFP gauge (cycles=0) can't be trusted to either. Cell is BatterySpace (reputable,
  rated 2000); no basis to call it bad, and the user's larger cells aren't testable yet (no holder).
  Plausible too (Ben): a freshly-charged gauge may pin SOC near 100 % before dropping, so ΔSOC over
  a slice misrepresents charge. **Resolve with a clean full-charge → full-discharge INA-coulomb run**
  (now possible — charging re-enabled); leave DesignCap at the 2000 rating until then. The earlier
  README/SYSTEM "~1000 mAh, 2× overrated" is likewise unverified.

Caveats: the +9 % gauge current-bias fit is tight (n=814) but single-session. Post-run the cell
idled ~120 mA; I cut it to ~66 mA via WiFi-off (`q`). Follow-up (this session) adds a recoverable
timer-wake deep-sleep-on-floor + charge-enabled recovery so an unattended low cell can't be stranded.

## 2026-06-09 — Ben + Claude — Rails-cut idle win; 4-channel INA219 monitor built; ground-truth shows idle is tiny (gauge over-read it)

**Rails-off A/B (the sleep-current fix).** Hypothesis from cont. 10/11: the ~20 %/night
sleep-cycle drain was the two switchable 3V3 rails left on during deep sleep, not the wakes.
Added `Board.enable3V3(false) + enableVSQT(false)` before `esp_deep_sleep_start()`
(`net-bench-2026-06-09.1`) and ran a battery-only A/B vs the rails-on overnight baseline:
**rails-on ~1.7 %/h → rails-off ~0.5 %/h, a ~3–4× cut** (≈ 20 %/night → ≈ 5 %/night). The
rails were the dominant idle draw, as hypothesized. (Ratio is robust; the gauge only moved
~1 SOC count in 2 h, so the absolute is coarse — see below.) **Captured as a gotcha in
POWERFEATHER_NOTES** so we don't relearn it. V2 keeps the gauge alive with VSQT off (separate
power-mgmt I2C), so telemetry survives the rail-cut.

**Built a 4-channel ground-truth power monitor** (`firmware/ina_monitor/`): Adafruit Metro
ESP32-S3 reading 4× DFRobot SEN0291 (INA219) at 0x40/41/44/45, separate-monitor topology
(reads a board-under-test's current through its deep sleep — the thing the on-board gauge
can't). Direct register reads (bus V + raw shunt mV → current; calibration-independent),
streams `ina ...` lines. Saga worth noting: (1) a STEMMA↔Gravity cable that **swapped
VCC/GND** dead-shorted + briefly killed USB on the Metro — *all four INA boards survived* the
reversal; (2) Metro defaults to USB-OTG/TinyUSB which re-enumerates on sketch start (no
serial) — flash with `USBMode=hwcdc,CDCOnBoot=cdc` like the PowerFeathers; (3) the hub works
direct-wired to the Metro's SDA=47/SCL=48 headers (bypass the bad cable). A reverse-polarity
JST also scared us but the PowerFeather + INA both survived.

**First ground-truth measurement (INA in the peer's battery lead).** Caught the ~30 s wake
as a current bump: **wake ≈ 11 mA for ~0.6 s, sleep ≈ 0** (below the ~0.2 mA PGA floor). So
the duty-cycled **battery** drain is **sub-mA** — far below the gauge A/B's ~0.5 %/h (~4–5
mA). Reconciliation (vindicates Ben's gauge-distrust): the gauge's ~0.5 %/h was within its
own 1-count noise on the flat LFP plateau; the real drain was simply too small for it to
resolve, and the INA finally does. **Idle is negligible — now ground-truth, not inferred.**
Caveats: 10 Hz may undersample a brief (<100 ms) radio-init spike (40 mV bus sag hints at
one) → a fast-sample capture is the next step to nail per-wake energy; sub-0.2 mA sleep is
below this PGA range (sharpen by dropping the range); the ~11 mA wake being far under the
~168 mA always-on RX wants understanding (likely boot/init-dominated, not full RX).

**Walked back the LFP capacity claim** (Ben was right): cont. 11's "~1000 mAh / overrated 2×"
was too strong. The 06-03 drain delivered ≥617 mAh but stopped *mid-plateau* (not empty), on
an un-learned gauge → true capacity is unknown, likely a normal ~1–1.5 Ah 18650 LFP. Needs a
clean full→empty coulomb run on a learned gauge or external meter. Softened in README /
SYSTEM.md.

## 2026-06-08 (cont. 11) — Ben + Claude — Drawdown aborted (redundant); LFP capacity looks ~half rated; sleep-cycle idle budget negligible

Ben flagged the running always-on LFP drawdown as redundant — correct. The 2026-06-03
overnight reboot-loop drain (`...is31-loadgen-overnight.jsonl`) already has the LFP
discharge curve at a similar load (mean −145 mA): SOC 92→30 %, **flat ~3.25 V throughout**
(min 3.234), 4.25 h — the "LFP plateau → V-SOC useless → coulomb-count" lesson. Aborted the
new run; switched the board to the sleep-cycle test instead.

**Capacity finding (from the existing 06-03 data):** it integrates to **~617 mAh delivered
for a 62 % SOC drop → real usable capacity ≈ ~1000 mAh, not the 2000 mAh rating.** The
"2000 mAh" 18650 LFP looks **overrated ~2×** (physically, 18650 LFP are ~1000–1500 mAh;
2000+ is Li-ion-class). This **~halves the assumed battery budget.** Caveat: LFP gauge SOC is
shaky on the plateau — **confirm with a clean full→empty coulomb-counted run** (USB top-up
first). Partly answers the "compare LFP sample vs rated capacity" TODO.

**Sleep-cycle duty-cycled average (computed):** sleep-cycle validated on hw (lean wake
~250 ms to HB + ~400 ms maint-listen ≈ **0.65 s radio-on per 30 s cycle**). The MAX17260
can't catch the sub-second wake spike (reads ~0 mA), so computed from trusted pieces:
avg ≈ (0.65 s / 30 s) × 168 mA (the always-on radio draw) + sleep floor ≈ **~4 mA at a 30 s
wake interval** (~2 mA @ 60 s, ~1 mA @ 300 s). **Takeaway: the idle/sleep budget is
negligible** (~48 mAh/night @30 s on a ~1 Ah cell ≈ a few %); **sizing is LED-show- and
harvest-bound, not idle-bound.** Caveats: active current is the separately-measured always-on
figure, sleep floor is estimated — a precise per-wake/sleep number needs an **external
ammeter (SEN0291 / multimeter)**; the gauge fundamentally under-samples brief-pulse loads.
**Field concern:** that under-sampling means a sleeping fixture's gauge SOC can read
optimistically high → low-battery logic must cross-check **voltage** (reinforces existing
TODO). Sleep-cycle left running overnight battery-only as a gauge-vs-pulse cross-check.

## 2026-06-08 (cont. 10) — Ben + Claude — Solar/sizing session: sleep-cycle + OTA-wake, idle floor, MPP sweep (cloud-caveated), drawdown started

Long bench session toward battery/panel sizing. New firmware `net-bench-2026-06-08.9`
(all validated on hardware via OTA) + several findings.

**Firmware:**
- **Sleep-cycle** (`--sleep-cycle --sleep-s N`): deep-sleep duty cycle (wake → telemetry
  heartbeat → brief maint-listen → deep-sleep). Validated: `rr=deepsleep`, ~32 s cycle.
  Trimmed the USB-CDC `delay(1500)` on deep-sleep wakes so the wake is lean.
- **`U` sustained ENTER_MAINT** (~35 s): no-touch OTA-recovery of a **sleeping** board —
  the normal `u` burst misses a board awake only ~400 ms/30 s. Validated: a deep-sleeping
  peer caught it on a wake window and joined WiFi for OTA. The field **fleet wake-for-
  maintenance** primitive.
- **`SET_MAINTAIN`** (master `m`): runtime VINDPM/charger-maintain set over ESP-NOW (no
  reflash) — the MPP-sweep actuator + future P&O MPPT primitive.

**Idle-load floor (battery-only, clean):** an always-on ESP-NOW peer draws **~168 mA /
~0.55 W**, and killing the WiFi scanning barely moved it — the load is **radio-RX-
dominated**, not scanning. 168 mA flattens a 2 Ah cell in ~12 h, so **always-on is
unsustainable on battery → deep-sleep duty-cycling is mandatory** (quantifies the "be
quiet during sunshine" instinct).

**Harvest (full sun):** Seeed 3 W panel, flat at ~2 pm, **lux 127 k (~1000 W/m² = full
sun)**, panel **150 °F / ~65 °C** (IR; glass ε≈0.9, so true temp ~equal or a hair higher).
Measured **~1.0–1.2 W** at the default VINDPM 5.5 V (SOC 34–58 %, bulk-charging). 3 W is
STC; heat (−15–18 % + Vmp droop) + flat angle explain <3 W — but see MPP.

**MPP sweep — finding + caveat:** swept VINDPM 5.5→4.4 V. **Peak power at ~4.85 V — matches
the hot-panel Vmp prediction; 5.5 V is well past the IV knee** (power craters above ~5.0 V).
BUT a **cloud rolled in mid-sweep (127 k→37 k lux)** with the panel temp drifting, so the
absolute watts (0.14–0.37 W) and the apparent 2.6× are **NOT a clean full-sun number** (the
start/end 5.5 V points disagreed, 0.138 vs 0.215 W = intra-sweep drift). **Robust:** MPP
≈ 4.85 V hot, fixed 5.5 V is wrong when hot. **TBD:** the actual full-sun gain (no full-sun
MPP point captured). **MPPT verdict: green-light to MEASURE properly (clean full-sun sweep
+ simultaneous lux/IR-temp at 2 panel temps), not yet to commit it's worth ~2×.**

**Drawdown (started, cloudy evening):** brought inside, panel disconnected, always-on
~157 mA battery-only discharge from ~SOC 60–76 % (gauge jumpy on the LFP plateau — trust
the coulomb count). `--autosleep` deep-sleeps at brownout to protect the cell. Logging
overnight → LFP discharge curve, gauge accuracy, delivered capacity, cutoff voltage
(`ops/bench/data/ca/` + `/tmp/nb_drawdown_raw.log`; results next session). NOTE: this used
the always-on load; the **sleep-cycle duty-cycled average** (the low overnight budget
number) is still un-measured.

## 2026-06-08 (cont. 9) — Ben + Claude — Conclusions: WiFi hypothesis settled (moving-board artifact) + stress-test framing

Wrap-up of the day's two device tests.

**WiFi drop — hypothesis settled (high confidence).** The board latches to one Eero BSSID
at association and **does not auto-roam** (ESP32 has no 802.11k/v/r); carried from indoors
to the yard, it clings to the now-weak indoor node instead of hopping to the strong (−46
dBm) nearer one → the link collapses while a good AP sits right there (the scan is the
smoking gun). Fix is cheap and already partly in place: **a reset, a software reset, or a
firmware "re-associate on link loss" guard** forces a fresh scan-and-associate, which
picks the strongest beacon (our maintenance-OTA path already does a fresh `WiFi.begin()`,
which is why OTA worked from the bad spot). **Framing (Ben):** this is a **bench artifact
of a *moving* board** — deployed fixtures are stationary and won't walk away from their
Eero, so we're unlikely to hit this in the field. Logged as a **gotcha** (see
POWERFEATHER_NOTES) + a firmware-guard TODO, not a blocker.

**Panel 0.12 V → 5.55 V swing — explained:** Ben **reseated the solar connector** mid-check;
that's the swing, not a mystery intermittent. Takeaway for production: **mechanically
secure/strain-relieve the panel pigtail** (a loose connector = silent zero-harvest), and
item (a) now makes a dark panel obvious live (`supply_good=0`, `supply_v≈0`).

**Stress-test framing (important for reading the numbers):** this run **highly activated the
radio (continuous ESP-NOW + 15 s all-channel WiFi scans) WHILE harvesting** — a deliberate
worst case. Even so the cell net-charged in decent light. **In the field the fixture will
be asleep / quiet during sunshine**, so real harvest-vs-load is *more favorable* than these
bench numbers — i.e. the bench load figures are conservative, not representative. Next
focus: a **sizing-oriented** solar run (realistic sleep/duty-cycle load, harvest across
sun/cloud/shade) to actually spec the cell + panel.

## 2026-06-08 (cont. 8) — Ben + Claude — Item (a): supply/panel telemetry over ESP-NOW — built + VALIDATED on hardware

Built the solar-telemetry half of the plan (item (a)): carry the **supply (panel) side**
over ESP-NOW so it logs from anywhere without WiFi-STA. Threaded `supply_mv`/`supply_ma`/
`supply_good` end-to-end — peer reads `Board.getSupplyVoltage/Current/checkSupplyGood`
(cached ~1 Hz in `readBattery`), **appended** to `NbHeartbeat` (kept `NB_PROTO_VER=1`;
append-only + length-checks → no flag-day, a pre-supply master still reads the battery
fields of a supply-capable peer; new master reads old peer via `offsetof` guard), stored
in `NbPeerStat`, emitted as `sv=/sma=/sgood=` on the `nb-peer` bridge line. Host
`net_bench_log.py` parses them (optional regex group) and derives `supply_w` (panel
harvest), `battery_w`, and `load_w = supply_w − battery_w` into the JSONL. fw
`net-bench-2026-06-08.7`.

Deployed via the maintenance round-trip (master `u` → peer rejoined WiFi → OTA `.7`
supply build → reflash master over USB). **Works end-to-end:** `sv=5.56 sma=160 sgood=1`
→ **panel ~0.89 W**, battery flips to **net-charging +140 mA** under the (heavy) scan
load; harvest swings 0.5–0.9 W with the clouds, all logged.

**Solved the earlier "net-discharge at noon" puzzle:** while the peer was briefly in
maintenance mode its `/telemetry` showed **`supply_v=0.123` — the panel was essentially
dark** (shaded/mis-oriented in-hand, or a loose connector). So the discharge was simply
**zero harvest**, not a battery/load problem. Once the panel saw light again, `supply_v`
jumped to 5.55 V and it charged. Lesson: **harvest is very orientation-sensitive** — a
real sizing finding, and exactly the thing item (a) now makes continuously visible.

**Caveat for sizing:** the derived `load_w ≈ 0.39 W` here is the *diagnostic firmware's*
load (radio always on + 15 s WiFi scans), NOT a fixture budget — don't size the cell to
it. The **panel-harvest V/I is the directly-useful output**; the load side still needs
the bottom-up fixture duty-cycle budget (existing TODO). Boards left running on ch 11,
logging to `ops/bench/data/ca/2026-06-08-ca-lfp-2000-net-master-multicast-rNA-1946.jsonl`.

## 2026-06-08 (cont. 7) — Ben + Claude — WiFi coverage diagnostic VALIDATED on hardware (2 boards, OTA) + PDR seq-bug fixed

Took (cont. 6)'s firmware to hardware. Flashed the **serial-bridge master** (`9F2690`)
over USB on ACM1 (`--serial-bridge --no-charge`, ch 11) — boots into "SERIAL BRIDGE (no
WiFi)" and streams `nb-*` to USB as designed. Then **OTA'd the scan-report peer onto the
live solar board** `9E5B0C` (the only wireless Resonance board — found by sweeping the
LAN for `/telemetry`; note `192.168.4.73` is an unrelated "Grow Light", NOT ours, left
untouched). Built the OTA with **`--chem lfp --cap 2000 --maintain 5.5`** to match the
board's LFP cell + solar panel (Li-ion profile would overcharge the LFP — the
POWERFEATHER_NOTES gotcha).

**Worked end-to-end, first try.** Post-OTA the peer left WiFi, rejoined as an ESP-NOW
peer (`rr=software`, LFP 3.33 V, still solar-charging ~40 mA), and streamed the **2.4 GHz
coverage map to the desk with zero WiFi-STA on the field board** — resolving the **3
BubbyNet Eero nodes separately by RSSI** (`…a3:06`/`…9c:06` @ −44, `…40:c6` @ −62, all ch
11) plus neighbors on chs 1/6/11. The two things flagged as load-bearing-but-unverified
in (cont. 6) — async `WiFi.scanNetworks()` coexisting with ESP-NOW, and the post-scan
channel re-pin — **both hold**.

**Found + fixed a real bug:** heartbeats and scan-AP packets shared one tx sequence
counter, so each scan batch's N sends read as N phantom heartbeat *gaps* at the master
(uplink PDR showed a bogus 0.65). Gave heartbeats their **own contiguous seq** (`hbSeq`
in `sendHeartbeat`). Re-OTA'd the fix via the **maintenance round-trip** (master serial
`u` → peer rejoined BubbyNet → OTA → both back to comms, no touch — also validates that
path). After: `gaps=0` through scans, `pdr=1.0` with an honest occasional `gaps=1`.
(Downlink `dlpdr≈0.8` is expected: the peer is deaf to the master's 10 Hz frames during
its own ~2.5 s scan window — informative, not a fault.)

Net: **item (b) is validated on hardware.** Still TODO: the actual **yard walk** (carry
`9E5B0C` out, watch the per-Eero-node RSSI fall off → the coverage-at-distance map +
where to place a field maintenance AP) and write that note. Tooling to capture it
(`net_bench_serial_bridge.py` → `net_bench_log.py` `nb-scanap` rows) is ready but a
background log wasn't started this session. Boards left running on ch 11.

## 2026-06-08 (cont. 6) — Ben + Claude — WiFi coverage diagnostic, reworked as a wireless ESP-NOW bridge (firmware done, untested on hw)

Picked up the solar-telemetry/range handoff plan, item (b) — the WiFi range diagnostic.
Started on the standalone tethered sketch (`firmware/wifi_diag/`: associates, streams
RSSI/BSSID/channel + a 2.4 GHz scan, flags a *missed-roam* when a stronger same-SSID Eero
node wasn't chosen). Then Ben pushed back on the laptop tether and proposed a better
setup: an **ESP-NOW "wireless serial" bridge** to his desktop. That's the right call —
it's the *same* architecture item (a) needs, so building it once serves both.

**Reworked (b) as scan-only over an ESP-NOW bridge** (extends `firmware/net_bench/`):
- **`--serial-bridge`** (a master): does NOT join WiFi; stays pinned to `--channel` and
  relays everything it hears (`nb-master`/`nb-peer`/`nb-scanap`) to **USB serial**, so a
  desk-tethered board logs the whole field fleet — no laptop in the yard.
- **`--scan-report`** (a field peer): async-scans 2.4 GHz (**never associates**), then
  broadcasts the strongest `--scan-max` APs (BSSID/RSSI/ch/SSID) as a new `NB_SCANAP`
  packet. Because it never associates, the radio is **ours to pin to `--channel`** (no
  Eero-channel coupling — the key insight; an *associated* board is locked to the Eero's
  channel and ESP-NOW rides that). Radio is re-pinned to `--channel` after each scan;
  ESP-NOW TX is suppressed while the scan hops.
- Host: `ops/bench/net_bench_serial_bridge.py` relays the bridge's serial → UDP:54321 so
  the **existing** `net_bench_log.py`/`net_bench_monitor.py` work unchanged; `net_bench_log.py`
  gained an `nb-scanap` row (per-AP coverage → JSONL).

Why this answers (b): the plan's own stated smoking gun is "a scan showing a closer node
with better RSSI it didn't pick" — a **scan needs no association**, so scan-only delivers
the per-Eero-node RSSI coverage map from anywhere in the yard (and tells us where to put
the field maintenance AP). The empirical roaming-*decision* test stays in the tethered
`wifi_diag` probe.

**Status: all 4 net_bench variants compile clean (28% flash); NOT yet run on hardware**
(no board on USB this session — ACM0 is the PAR sensor). Cautions before trusting any
map: async `WiFi.scanNetworks()` + ESP-NOW coexistence on the S3 is assumed-fine but
unverified, and the post-scan channel re-pin is the load-bearing line. Next (Ben): flash
2 boards on a shared `--channel`, walk the field peer, confirm `nb-scanap` updates from
the yard, then write the RSSI map + AP-placement note here. Details: updated
`SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md` (end) + `firmware/net_bench/README.md`.

## 2026-06-08 (cont. 5) — Ben + Claude — A/B rollback VALIDATED (bad image auto-reverts) + the recipe

Tested A/B rollback with a bad image (battery-only LFP). **PASS:** pushed a power_bench
build whose self-test hook reports unhealthy (`extern "C" bool verifyOta(){return false;}`,
gated by `-DRES_OTA_FAIL_SELFTEST`); on first boot the Arduino core (`initArduino`, before
`setup()`) saw the image `PENDING_VERIFY`, called `verifyOta()`→false →
`esp_ota_mark_app_invalid_rollback_and_reboot()` → bootloader **reverted to the last-good
image automatically, no touch** (board came back on `ota1`; the bad image never reached
setup/WiFi). `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is in the arduino-esp32 3.3.7 build.

**Gotcha (caught the first try):** `verifyOta()` is a **C-linkage** weak hook (defined in a
.c core file). A plain C++ override is name-mangled, silently does NOT override, the default
(returns true) runs, and the bad image **sticks** (no rollback). Must use `extern "C"`.

**Production recipe (the safety net):** implement `extern "C" bool verifyOta()` with a real
self-test (power chip init + radio + fuel-gauge reachable) → return false on failure for an
auto-revert. **Limitation:** this only catches self-test FAILURE; an image that passes
verifyOta then crashes/hangs LATER in setup()/loop() is already marked valid → could brick.
Robust pattern: `verifyRollbackLater()=true` to DEFER the mark-valid, run extended checks +
the watchdog, and mark valid only after proving stable for N s — so a late crash/hang trips
the watchdog while still PENDING_VERIFY and rolls back next boot. power_bench keeps the
gated `RES_OTA_FAIL_SELFTEST` fixture as a reusable rollback test.

## 2026-06-08 (cont. 4) — Ben + Claude — Battery-only OTA validated on worst-case LFP (the field-reset requirement)

Per Ben (correctly): battery-only OTA with NO physical access is a hard requirement (can't
take lanterns off the tree), so battle-test it now. Did 3 consecutive OTAs to the LFP board
**battery-only (no USB), at ~3.2 V (the buck-boost-crossover, hardest regime), over WiFi**:
**3/3 recovered cleanly, no button**, each via software reset (`rr=software`), and the new
image confirmed running (`fw` flipped to `power-bench-2026-06-08.ota1` after OTA#1 — a real
update, not a rollback). With the ~14 battery-only peer OTAs earlier this session that's
**~17/17, zero failures.** Conclusion: **battery-only field OTA is trustworthy** — the
"never touch a deployed lantern" requirement is met.

Key clarification (resolves the earlier confusion): the flaky/stranding resets were the
**USB-JTAG hardware reset** (esptool's RTS path during *USB* flashing) + the no-battery
brownout — neither exists in field OTA, which uses `ESP.restart()` (software reset), reliable
every time. "Use USB" was bench-iteration convenience, not a trust statement.

Caveats (refinements, NOT blockers; a failed OTA is safe — stays on / A-B rolls back to the
known-good image, never bricks): (1) tested over GOOD WiFi (the field model = a local AP near
the tree for a maintenance window); OTA over a MARGINAL link is untested (TCP retries, but a
bad link could fail the upload → no update). (2) A/B rollback not yet explicitly tested (push
a deliberately-broken image → confirm auto-revert) — worth doing as the ultimate safety net
alongside the watchdog + autosleep recovery.

## 2026-06-08 (cont. 3) — Ben + Claude — Solar path validated (net-positive in weak light) + LFP bring-up + a brownout root-cause

Moved to solar feasibility (power_bench, not net_bench). Switched to the LFP 2000 mAh cell —
flashed `power_bench --chem lfp --cap 2000 --maintain 5.5` (Seeed 3W panel: Vmp 5.5 / Voc
8.2 / Imp 540 mA) BEFORE connecting the cell (LFP charges to ~3.6 V, not Li-ion's 4.2 V —
flashing the LFP profile first keeps the charger safe). `Board.init(2000, Generic_LFP) Ok`.

**Brownout root-caused (clean):** on USB with NO battery, the board crash-looped (USB-CDC up
~1 s then reset). Cause = `--maintain` (VINDPM) 5.5 V > USB 4.92 V → the charger *rejects*
USB (won't pull its input below the 5.5 V setpoint) → with no battery to source VSYS, it
brownouts; and enabling charging into a missing battery is the trigger point. Connecting the
cell fixed it instantly (battery sources VSYS). Unifies the earlier brownout work: it was
`maintain > supply voltage` + no buffer, not "no battery" per se. (Firmware guard TODO: don't
enable charging if no battery detected; and `maintain` must be ≤ the supply you're on.)

**Solar result (partly cloudy, ~10:18 am Oakland, through a window):** panel **5.56 V ×
~66 mA ≈ 0.37 W**, VINDPM holding the panel steady at 5.5 V, **battery_ma +~10 mA — net
POSITIVE charge** into the LFP (3.31 V / 33%, safe) *even with WiFi running* (the radio eats
~56 of the 66 mA; ~10 mA banks). Path validated end-to-end. Extrapolations: asleep, ~all
66 mA would bank; full sun → ~540 mA (≈8×) → the ~120 mAh/night budget closes with margin.
Solar essentially de-risked (it's what the board is built for).

Also: ESP-NOW reached the back fence but WiFi-STA couldn't hold the yard — expected, not a
bug (different destination = router vs office-master, and WiFi assoc+TCP needs far more
margin than ESP-NOW's loss-tolerant broadcast). It WiFi-reconnected fine once close — no
instability. Next (do on USB so reflash/tune is safe): full-sun board-asleep harvest number
+ `--maintain` sweep (5.5/5.0/4.6) for the shaded canopy.

## 2026-06-08 (cont. 2) — Ben + Claude — T3 range walk: clean V, link held through house+yard+oak

Walked the cup board (`9F2690`) out the back door, across the yard to the fence (behind a
big oak), and back, slowly, with the 3 stationary boards as controls. New tooling:
`ops/bench/net_bench_walk.py` (continuous per-peer RSSI/PDR logger, run in background) +
`net_bench_walk_plot.py` (Pillow V plot) + live landmark markers. Result: a clean V/bathtub
(−19 dBm office → −80..−87 floor at fence/oak with a few brief dropouts → −30 back), 152
samples / 328 s. **Findings:** (1) the **house doorway dominated** (~50 dB in the first ~30
steps); open-yard distance added little; (2) the **oak trunk caused the deepest dips**,
recovering at the fence past it; (3) RSSI is **path-asymmetric** (door −69 out / −47 back —
multipath); (4) the **3 reference boards stayed flat** → the swing is real, environment
stable (good control). The link **held ~100 steps through a house door + full backyard +
behind an oak** — far harsher than the tree (open air + bamboo, no doorway), so a strong
deployment result. Data/graph: `ops/bench/data/ca/2026-06-08-rangewalk.{jsonl,png,-markers}`.
Live RSSI also viewable via `net_bench_monitor.py`. (Still un-measured: pure open-field
clean-LoS cliff distance — the house doorway masked the distance falloff here.)

## 2026-06-08 (cont.) — Ben + Claude — Obstruction mapping: enclosure ~RF-transparent, solar panel is the attenuator

Used the identify/locate blink to label peers placed in different obstructions (10 Hz, all
held ~99-100% PDR at bench range): 3D-printed lantern cylinder (board inside) −15 dBm;
ceramic cup −29; metal laptop in a metal+glass cabinet −31; **glass+metal solar panel on a
box −52** (~25-35 dB hit). **Two build-relevant findings:** (1) the **lantern enclosure is
~RF-transparent** — the printed/plastic housing won't detune or block the mesh; (2) the
**solar panel is the one real attenuator (~25-35 dB)** and it sits over the antenna in the
hat — the antenna-keepout concern made concrete (still 100% PDR / ~38 dB margin at bench
range). Caveats: placement+obstruction combined (not pure material deltas), RSSI approximate,
short range. Worst case = panel attenuation + full tree distance stacked → the mock-hat RF
test (Steve). Also flagged: identify's 8 s blink is too short for human-in-the-loop / field
use (Ben missed a single blink waiting on chat latency) — make it ~30 s or toggle-until-stop.

## 2026-06-08 — Ben + Claude — Fuel-gauge false-low after charge (SOC needs voltage cross-check)

Morning: one peer (`9E5AF0`, 10050 mAh) was blinking 4 Hz (LED "<10%"), but **bv=4.188 V
= fully charged** — the cell charged fine; the gauge is misreading 1%. Extends yesterday's
cap-reseed finding: after the `DesignCap` change the MAX17260 re-seeded per-board to
*different wrong* values (`9F26F8`→~100%, `9E5AF0`→~1%), and the overnight charge didn't
fix it because the board ran the whole time (~100-200 mA) so the charger likely never hit
the clean **termination** event the gauge uses to anchor 100%. Lessons for production: (1)
gauge SOC is untrustworthy after a cap change / without a real learn cycle; (2) an
always-awake fixture solar-charging may never anchor its gauge (the duty-cycled CA design
helps — low load during charge); (3) **low-battery logic must cross-check voltage** — a
false 1% could trip a needless shutdown, a false 100% could over-discharge. Action: add a
voltage sanity-check to the battery LED (bv>4.0 V => never show "critical").

**Done (v07.5, OTA'd to all 5):** the battery LED now floors the displayed level by a
loaded-Li-ion voltage estimate, so a false-low gauge can't show "critical" — `9E5AF0` now
shows SOLID (gauge still reads 1% but bv 4.19 V vetoes it). Ben's field-vs-bench insight:
this false-low is likely a **bench artifact** — deployed fixtures sleep + trickle-charge
from solar under near-zero load, so the charger reaches termination and the gauge anchors
(and gets a real cycle daily); the always-pinging bench run is the pathological case.
Friction noted: each firmware OTA needs per-board cap bins (cap is a build flag) — a
follow-up could store cap in NVS / make it runtime-settable so one bin serves all.

## 2026-06-07 (cont. 6) — Ben + Claude — Rate sweep PASS: ESP-NOW scales to ~100 nodes

Ran the broadcast-rate sweep (new `ops/bench/net_bench_ratesweep.py`, drives the master's
`+`/`-` over serial + measures per-rate PDR from the bridge), 1→50 Hz, master + 4 peers,
co-located, Li-ion. **Aggregate uplink PDR ≥97% across the whole range, no collapse:**
1Hz 100%, 10Hz 99.5%, 20Hz(100 pkt/s) 99.1%, 50Hz(250 pkt/s) 97.2%. Clean airtime fit
`loss ≈ 1.05e-4 × pkt/s` → **100 nodes @ 1–2 Hz/node ≈ 98–99% PDR**. Strong GREEN for the
"can we base 100 fixtures on this" question. (Tooling fix: the naive worst-peer knee was a
small-sample artifact — one lost packet of ~60 reads as 98%; switched the verdict to
aggregate loss.) Caveats: 5-node small-N (no hidden-node at scale), co-located (range is
T3/T4 next), Li-ion (re-verify on LFP). T5 parallel-OTA already passed; T3/T4/T6/T7 remain.

## 2026-06-07 (cont. 5) — Ben + Claude — Identify/locate command; per-board cap; MAX17260 re-seed finding

Added an on-demand **identify/locate** command (master `i`/`I` → target board blinks a
distinct `..-` on the onboard LED for 8 s; the data-center chassis-ID pattern) and used it
to map board↔battery without plugging in: master 2200, `9F2690`/`9E5AB8` 4400,
`9E5AF0`/`9F26F8` 10050 mAh. OTA'd each board with its correct `--cap` (fw v07.4; all 5
recovered no-button — cumulative OTA reliability still 100%).

**Fuel-gauge finding:** changing the MAX17260 `DesignCap` re-inits the gauge and **resets
learned SOC** → a transient bad reading (`9F26F8` 10050 mAh: 27% @3.73 V with cap=2000 →
**100% @3.72 V** after re-seeding to 10050; true ~50%). So: **set DesignCap once at first
boot, charge to full to anchor 100%, let the gauge learn over a cycle; don't change cap in
the field.** More critical on LFP (flat OCV). Folds into T6 prep (fully charge cells
first). Also shipped a `/resume` re-init fix (v07.3) in the same firmware.

## 2026-06-07 (cont. 4) — Ben + Claude — net_bench first light: ESP-NOW works, OTA validated, battery-LED deployed

Flashed the fleet (1 master USB + peers on Li-ion). **First light, ch 11:** master +
**3 peers** up, uplink/downlink **PDR ~99.5%** at 10 Hz co-located, RSSI −25 to −33 dBm,
**0 send-fail** — ESP-NOW works. (One flashed peer never booted — a silent no-boot the
watchdog can't catch since it never reached loop(); post-flash boot flakiness or flat
cell.) Added a **battery-level onboard LED** (GPIO46: >50% solid, 25-50% 1 Hz, 10-24%
2 Hz, <10% 4 Hz) and **OTA-deployed it** (v07.2) to master + 2 reachable peers via the
maintenance-mode cycle. **T5 effectively PASS** — all recovered via *software reset, no
button* (master via /telemetry, peers via ESP-NOW rejoin with rr=software).

Two findings: (1) `net_bench_ota.py` false-FAILED the peers — they reboot OFF WiFi into
comms, so /telemetry polling can't see them; fixed with `--reboot comms` (the OTA
"complete/Rebooting" ack + software reset IS the success signal; confirm rejoin via the
bridge). (2) **Brownout de-risk:** the ~4%-SOC peer dropped out entering maintenance —
the WiFi-association inrush on a near-empty Li-ion cell is the brownout failure mode; at
100× we must gate OTA/maintenance on SOC (or lean on the autosleep guard). Next: charged
cells on all boards, then the rate sweep + range/obstruction matrix.

## 2026-06-07 (cont. 3) — Ben + Claude — net_bench: first ESP-NOW firmware + 5-node feasibility harness

Built the project's **first ESP-NOW firmware** to de-risk basing ~100 fixtures on the
PowerFeather V2 (networking/radio/stability axis). New `firmware/net_bench/` (forked from
power_bench): broadcast-only ESP-NOW (unencrypted FF:FF — the 100-node-scalable pattern;
encrypted peers cap at ~17), **master** role (broadcasts SHOW_FRAME + WiFi-STA-bridges
per-peer stats to the host over UDP:54321) and **peer** role (pure ESP-NOW on battery,
HEARTBEAT with seq/battery/downlink-PDR). Per-source seq-gap PDR. **Maintenance-mode
switch** (ESP-NOW metadata → peers join AP → standard WiFi OTA, ADR-0010 compliant; no
firmware over ESP-NOW). **Watchdog** added (esp_task_wdt — net-new, closes the open
field-reliability TODO) + `--wdt-hangtest`. Autosleep guard ported.

Host harness: `ops/bench/net_bench_log.py` (master bridge → JSONL), `net_bench_ota.py`
(parallel OTA + auto-recovery/no-button assertion), `net_bench_summary.py` (per-peer
PDR/RSSI + scale-extrapolation loss knee). Test plan + acceptance targets:
`docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md`.

**Bench-validated on 1 board (9E5B0C):** boots, Board.init Ok, ESP-NOW up, heartbeats
broadcasting (0 send-fail); **watchdog recovery PASS** (induced hang → task-WDT reset →
reboot, post-reset reason `task_watchdog`, no human); master WiFi-join + host JSONL
capture PASS. **Channel-lock confirmed real:** home AP "BubbyNet" is ch 11, so building
with `--channel 6` made the master warn and every send fail (`Peer channel != home
channel`). **Action for Ben: build all 5 boards with `--channel 11`** (= the AP channel)
to run the multi-node matrix. All battery results will be Li-ion (JST-PH) — asterisked to
re-verify on LFP (LFP plateau sits on the buck-boost crossover, the harder regime).
Multi-node T0–T7 pending Ben's 5 boards on a matched channel. Plan approved; this is the
implementation of that plan.

## 2026-06-07 (cont. 2) — Ben + Claude — Second Split style (rotate-about-center) + ping-pong spiral

Two small LED Studio refinements:
- **Split RGB is now 3-state (Off / Triad / Rotate).** Triad = the original local R/G/B
  offset cluster (spread/rotate). **Rotate** = R at the point, G/B the same point
  rotated 120°/240° about the grid center → a 3-fold rotationally-symmetric color
  split (collapses to white at the exact center; shines with a moving spiral/orbit
  head). Both validated on hardware.
- **Spiral now ping-pongs** (out to the edge, then retraces inward) instead of jumping
  from the outer tip back to the center — no per-frame discontinuity. Orbit still wraps
  seamlessly (closed ring). Verified: spiral order-index steps by ≤1 the whole cycle.

## 2026-06-07 (cont.) — Ben + Claude — Merged LED Studio (HEX + RGBW + RGB), Split-as-toggle

Merged `hex_studio` + `rgbw_studio` into one **`firmware/led_studio/`** with a UI mode
toggle that hot-swaps between three LED options on the same A0/GPIO10 data pin — no
reflash — by reconfiguring the NeoPixel type/length at runtime
(`updateType`/`updateLength`): **HEX grid (37px RGB)**, **RGBW point (1px)**, and a
new **RGB point (1px)** for the high-power RGB LED (same as the RGBW minus the white
die — same render path, 3-byte strip, W ignored). Removed the two now-superseded
single sketches. Confirmed harmless to mismatch mode vs physical module (both SK6812):
worst case is wrong colors / one LED until refreshed; strip is blanked on each switch.

Per Ben's request, **Split-RGB is now a toggle modifier, not its own animation** — so
the separated R/G/B triad follows the selected path: Static (parked at the anchor,
Step+ to move it), Spiral, Orbit (sweeps the triad along the path with trail), and
Breathe (pulses the triad). Spread/rotate tune the fringe width. Validated on hardware
across all three modes + the split paths.

Process note (field-reliability data): the **USB-JTAG flash flakiness recurred twice**
this session — the port dropped after one upload (needed a replug) and a write failed
with "Error during build" before succeeding on retry. Reinforces the TODO that the
deployed lantern must never depend on the USB/RTS reset path (software reset + watchdog
+ the autosleep recovery instead). Recovering the IP after a reset still needs the
pyserial RTS pulse (native USB-CDC) — see `firmware/POWERFEATHER_NOTES.md`.

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

## 2026-05-11 — Ben + GPT — PowerFeather SDK 2.0.0 release confirms V2 support path

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

## 2026-05-10 — Ben + ChatGPT — PowerFeather V2 / COTS R&D update

Second-pass architecture update after COTS search, purchases, and schematic review.

### What changed

- **PowerFeather V2 is now the leading COTS/reference architecture.** It appears to match the project unusually well: ESP32-S3-WROOM-1, onboard PCB antenna, BQ25628E charger/power-path, LiFePO4 support in V2, MAX17260 fuel gauge, TPS631013 buck-boost 3.3 V rail, switchable VSQT/STEMMA-QT rail, solar/DC input, and rich power telemetry. V2 status is still preliminary until hardware arrives and is verified.
- **PowerFeather V1 remains LiPo-only as a board-level system.** V1 uses BQ25628E, but the board-level fuel gauge and regulator choices make it unsuitable for LiFePO4 production use. It may still be a strong LiPo fallback.
- **PowerFeather V1/V2 schematic diff completed.** V1 and V2 both use BQ25628E. V2 swaps the 3.3 V regulator from XC6220 LDO to TPS631013 buck-boost, swaps the fuel gauge from LC709204F to MAX17260, adds a 20 mΩ current-sense resistor, and adds I2C power-domain isolation around the STEMMA-QT rail.
- **COTS purchases made.** Ben bought the R&D candidates discussed in the COTS survey except USB power meters, which are already on hand. Elecrow PowerFeather boards were ordered despite possible ambiguity about whether the listing is V2 or V1. Ben also contacted the PowerFeather creator about V2 availability and KiCad files.
- **LED module plan narrowed.** The Adafruit IS31FL3741 13x9 RGB matrix is the leading plug-and-play STEMMA-QT LED module for PowerFeather. M5Stack NeoHEX is promising optically but is WS2812/Grove, not STEMMA-QT/I2C, and likely needs a GPIO data line plus a 5 V or otherwise suitable LED rail. M5Stack Atom Matrix is a compelling all-in-one fallback with ESP32 + 5x5 LEDs + USB-C.
- **Battery sourcing narrowed.** Prefer one larger LiFePO4 cell per fixture, ideally 18650 1500–2000 mAh, instead of multiple 14430 cells in parallel. 14430 cells are easy to find and cheap, but packs of many small cells add contacts, matching, wiring, assembly, and QA risk.
- **Solar-panel plan clarified.** Square/rectangular 1–5 W panels are fine for R&D. Round panels remain aesthetically attractive for production but are harder to source quickly and should not block testing.

### Current COTS prototype tracks

1. **PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 13x9 matrix.** Primary design-aligned candidate.
2. **PowerFeather V2 + LiFePO4 + solar panel + M5Stack NeoHEX.** Alternative LED geometry test; not STEMMA-QT plug-and-play.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback: DFR0559 owns battery/solar, FeatherS2 Neo battery JST stays empty, Feather is powered over USB.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback: small ESP32 + 5x5 LEDs powered by USB from the solar manager.

### Immediate tests once parts arrive

- Confirm whether Elecrow PowerFeather boards are V2 or V1 by chip markings and I2C scan.
- Verify LiFePO4 configuration and charging behavior on actual V2 hardware before trusting it.
- Measure sleep current with VSQT off and LED modules attached.
- Measure solar harvest and charge behavior for each 1–5 W panel under sun, shade, and heat.
- Compare IS31FL3741, NeoHEX, FeatherS2 Neo, and Atom Matrix for gobo projection, brightness, color fringing, PWM artifacts, current draw, and mechanical fit.
- RF-test each candidate inside a mock hat with panel, battery, screws, and wiring in realistic locations.
- Validate fail-safe behavior: LEDs stuck on, MCU hang, watchdog reset, low-battery cutoff, and recovery from depleted battery when solar input returns.

### Follow-up docs added

- `docs/research/COTS_SURVEY_2026-05-10.md`
- `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md`
- `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`
- ADR 0015 — PowerFeather V2 as leading COTS/reference architecture
- ADR 0016 — Purchased COTS prototype shortlist
- ADR 0017 — Battery cell format and sourcing
- ADR 0018 — LED module/interface plan

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
