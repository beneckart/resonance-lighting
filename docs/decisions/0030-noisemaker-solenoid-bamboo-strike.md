# 0030 -- Noisemaker: solenoid bamboo-strike selected; speaker synth abandoned

**Date:** 2026-07-15
**Status:** Accepted. The candidate question is closed; sub-decisions remain open
(solenoid voltage variant, strike power source, mounting, per-class scope --
listed below). Annotation 2026-07-16: the solenoid PART question widened -- bench
work (data pending commit from the bench laptop) shows 22,000 uF of strike storage
buys headroom for STRONGER solenoids, and a part bake-off is in progress with the
**0730B 6 V/1 A as primary candidate**; the in-transit 3 V/5 V units may be
returned. The strike-transient concern is CLOSED-benign: strikes appear as VDC
droops indistinguishable from a passing cloud/shadow -- the BQ charger is not
confused. Scope is trending fleet-wide on downlights + perimeter: the driver
count was topped up to 160 on 2026-07-16 ("the solenoids are cool enough we may
promote them to a feature on all the downlights and perimeter lights").
Annotation 2026-07-24: **the design is FINALIZED and named -- the "solarnoid"**
(VDC-tap solar supply + storage cap + solenoid + craft-store bulk mallet, order
TBC). Scope SETTLED narrower than the 07-16 trend: solarnoids pair with the
LARGE enclosures only (they need the extra space) -> downlights, <=110. The
perimeter's small hats sit this one out; expect a healthy MOSFET-driver surplus
(160 bought vs <=110 needed).
**Owners:** Ben + Claude

## Context

Elliot's interactivity ask spawned a noisemaker exploration (June-July): can the
fixtures make sound as well as light? Candidates benched and crowd-sampled:

- **Relay clicks** (Songle; SparkFun Qwiic Omron): a lovely mechanical tick, but
  the Omron's $18/unit killed it at fleet scale and crowd reactions were mixed.
- **Buzzers / piezo / vibration motor**: too soft, or read as a cell phone.
- **8002A amp square-wave tones**: loud but disliked -- square waves felt harsh
  against the bamboo-tree aesthetic (consistent across listeners).
- **Candidate A -- STEMMA speaker #3885 + custom percussion synth**
  (`firmware/speaker_demo/`): a 16 kHz fixed-point synth of organic percussion
  (bamboo knock, marimba, chime, water drip). Proven clean on hardware (fw .9,
  2026-07-07) after a run of PWM-carrier/amp-oscillator debugging; loudness was
  the remaining wish, and the exposed trim pot was a fleet liability.
- **Candidate B -- MOSFET driver + push-pull solenoid mallet** physically
  striking the bamboo -- the authentic knock the synth imitates. First bench
  2026-07-10 (`firmware/solenoid_demo/`): **815 strikes, zero resets, no
  failsafes tripped**. Fleet parts ordered the same day (150 solenoids in 3 V /
  5 V variants + 110 MOSFET drivers).

Design intent recorded 2026-07-12: strikes are a **daytime** feature --
solar-surplus percussion while the LEDs are irrelevant; night belongs to the
light show.

## Decision

**The solenoid mallet striking the bamboo itself is the fleet noisemaker.** The
strikes work so well physically that the #3885 speaker path is ABANDONED
(2026-07-15): the spare-speaker buy is cancelled, and no fleet fixture carries a
speaker/amp. The percussion synth and `speaker_demo` survive as bench/preview
instruments (useful for demoing ripple concepts on a desk), not production
hardware. Relay clicks and synthesized beeps are no longer pursued -- the
solenoid is the better physical click, natively.

The lantern becomes the instrument: the bamboo tube is the resonator, and the
sound is real percussion, not a recording of one.

## Remaining sub-decisions (open, tracked in TODO)

- **3 V vs 5 V solenoid variant** -- 75 of each on hand for the A/B.
- **Strike power source** -- the 3V3 header rail sags (~290 mA ceiling; strikes
  pulse 0.7-1+ A). Options: battery/VS pin, or the VDC-tap (XH 2-pin Y-cable off
  the panel input + storage cap, "daytime-only striker by construction") --
  VDC-tap sweep tooling landed 2026-07-11; verify strike transients don't
  confuse the BQ charger input.
- **Mallet/mounting design** vs the O(1)-ops rule (ADR 0009) -- per-fixture
  mounting labor is the cost to watch.
- **Per-class scope** -- all fixtures vs a subset; 150 solenoids keep fleet-wide
  possible.
- **Strike scheduler / daytime gating** in firmware (solar-surplus policy), and
  the strike current/loudness numbers for the daytime energy budget.

## Consequences

- To-buy queue: spare #3885 speakers CANCELLED; solenoid strike-power + wiring
  residuals remain the only noisemaker buys.
- The daytime-percussion intent gives the solar surplus a job: fixtures that are
  full by mid-day can spend harvest on sound without touching the night budget.
- Enclosure/mounting: the solenoid mallet needs a mount that couples the strike
  into the bamboo (Steve's integration track).
- Choreography surface grows: daytime knock-ripples through the mesh become a
  first-class mode alongside the night light show (BACKGROUND creative list).
- `speaker_demo`'s PWM-audio lessons (integer-locked clocks, PAM oscillator
  beats) are archived in the LOG/README for any future audio work.

## References

- LOG 2026-07-07 (noisemaker shootout status + crowd feedback), 2026-07-10
  (solenoid_demo first bench, 815 strikes), 2026-07-10 (cont.) (strike bench
  session), 2026-07-12 (cont. 2) (daytime-only design intent), 2026-07-15
  (verdict: speaker abandoned).
- `firmware/solenoid_demo/`, `firmware/speaker_demo/`, `firmware/clacker_demo/`.
- ADR 0009 (O(1) ops), 0024 (fleet), `ops/PROCUREMENT.md` (parts).

## 2026-08-06 amendment -- rev-1 boards require the 12 V boost retrofit

The rev-1.0 three-pin capbank was tested with the candidate HS-0730B and 59,000 uF
using identical 8/12/20/35/50 ms pulses at direct 5.086 V and boosted 12.284 V.
Both electrical sweeps were stable with a 32700 LFP attached, but Ben judged every
direct-5-V strike very weak and the boosted strikes hard and useful. At 50 ms, the
direct bank contributed about 0.083 J and the boosted bank about 1.193 J, a 14.3x
difference in net capacitor contribution. That is not total coil/mechanical energy,
but the physical A/B resolves the functional question.

**Decision:** any rev-1 capboard deployed with the HS-0730B must receive the external
approximately 12 V boost retrofit. Direct 5 V operation is a diagnostic/fallback
mode, not an acceptable production strike. The ordered v2 board's native boost
remains the production direction; do not close its bypass jumper for normal use.

The boosted drive remains bounded-pulse service on a nominal 6 V coil. Final fleet
pulse width, low-SOC/hot-panel qualification, and thermal/endurance limits remain
open. The separate PowerFeather VUSB+VDC service-input failure investigation is not
closed by this mechanical verdict.
