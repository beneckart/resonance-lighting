# AUTOLOCATE RSSI SIM FEASIBILITY -- 2026-07-12

Date: 2026-07-12. Status: COMPLETE (simulation study; hardware calibration
pending). Owners: Ben + Claude.

## Why

The "autoconfiguring tree" question: after ~150 fungible fixtures are hung,
can each device learn WHICH CAD fixture slot it occupies from data the fleet
already produces -- pairwise ESP-NOW RSSI plus the per-class ToF height
measurements -- or does the project need photogrammetry or manual per-device
registration? ADR 0004's standing caveat is "RSSI is a topology signal, not
distance"; this study quantifies whether 150x150 pairwise redundancy + metric
z-anchors + the CAD layout overcomes that, and where it breaks.

Tooling: `ops/locate/` (see its README). The solver library is strictly
separated from the simulator and ingests the same JSONL contract real
hardware will emit -- the math is reusable as-is when devices are in hand.

## Setup

- Ground truth: vendored CAD export (`Lighting-Controller` commit 0558a5d),
  118 fixtures + 40 synthesized perimeter ring fixtures; scale set so the
  downlight band = 7-10 ft (fleet spec is ground truth, not the export's
  unit claim; scale is a CLI knob). Devices are placed at a seeded 72-of-78
  downlight subset + all other slots with 0.15 m placement jitter (reality
  will not match CAD exactly). Scene: ~9.7 m across, z 0.1-3.1 m.
- Anchors: 72 downlights get z from the downward TMF8820 (sigma 10 mm;
  nighttime range covers the hang band per Ben), 38-40 perimeter get z from
  the VL53L5CX 8x8 zone ranges via the same robust plane fit sway_demo runs
  on hardware (ADR 0027), sigma ~5-15 mm after the fit. Uplights and
  chandelier have no ToF -> no z anchor (24 + 16 devices).
- RF noise model, calibrated to NETWORKING_FEASIBILITY_5NODE_2026-06-07:
  log-distance path loss (P0 -40 dBm @ 1 m, n 2.7) + per-packet fading
  (sigma_pkt 2 dB; measured short-window spread 2-8 dB) + STATIC per-link
  multipath bias (sigma_link, THE swept axis; measured same-placement shifts
  8-17 dB indoors, open playa estimated 2-6 dB) + directional asymmetry
  (1.5 dB; measured path-asymmetric RSSI) + per-device TX/RX offsets
  (sigma_dev 3 dB) + solar-panel directional shadow (raised-cosine, 20 dB
  depth per the measured panel attenuation; "spin" mode averages it over the
  campaign, "frozen" is the worst case) + occasional deep fades (5% of links,
  10-25 dB; doorway/oak-class dips) + bamboo trunk occlusion (10 dB on
  trunk-crossing sightlines) + 1 dB quantization + the -90 dBm receiver floor
  with a logistic reception rolloff. 50 packets per direction per link.
- Solver: see `ops/locate/README.md` (anchored 2D-MDS init, robust NLS with
  per-device offsets + P0, censored-link hinge constraints, CAD-size scale
  fix, stranded-device rescue, dual-arm init, gauge search / beacon-pinned
  registration, per-class rectangular assignment, confidence + flags).
- Verdict metric per run: **auto-correct / flagged / silent-wrong** device
  fractions. Silent-wrong (confidently misassigned) is the deployment killer.
  Assignment scored modulo the CAD's duplicate-position groups.

## What the solver build itself surfaced (findings before any sweep)

These fell out of making the pipeline survive its own simulator; each is a
real effect the deployment will meet:

1. **Floor censoring bends the whole map.** Links whose packet distribution
   straddles the -90 dBm floor lose their weak tail; survivor medians read
   high, long distances compress, and the perimeter ring visibly implodes.
   Fixed at the estimator: the beacon rate is known, so the true median is
   the (K/2)-th largest of K sent packets -- recoverable while >60% of
   packets survive; below that a link carries only "at least this far"
   (one-sided constraint in the solver). The firmware neighbor-dump must
   record expected counts (contract field `n_expected`).
2. **Absolute scale is nearly unobservable from RSSI.** A fleet-uniform
   attenuation (the solar panel costs ~-20 dB on every link) is exactly a P0
   shift, and the near-planar z-anchors barely pin xy scale: the solve
   settles self-consistently at whatever scale the init implied. The CAD's
   overall footprint supplies the missing scalar (correspondence-free rms
   radius match); the applied correction doubles as a P0-miscalibration
   diagnostic. Deployment alternative: measure one known-distance pair.
3. **All-links-shadowed devices strand.** A device whose every sightline is
   occluded (trunk-adjacent CAD artifacts here; a deeply-canopied lantern in
   reality) looks uniformly far away and the optimizer parks it OUTSIDE the
   cloud, median residual ~0 -- invisible to residual tests. The Hessian
   covariance proxy exposes it (sigma_pos 5-15x the fleet median) and a
   re-seed + warm restart recovers it.
4. **Rotational registration is intrinsically thin.** Under a wrong rotation
   the dense layout re-matches almost every device to SOME nearby slot, so
   the assignment cost of the correct rotation beats the best wrong one by
   only ~1-2% at ANY noise level (ambiguity ratio ~1.0 measured on the grid).
   Geometry alone picks the right rotation only by that thin margin -- it
   worked in most benign runs and flipped in others. **2-3 surveyed devices
   ("beacons") close this**: they fix mirror + coarse theta in closed form,
   and a restricted +-20 deg data search recovers the precise angle (beacon
   position error alone is one ring slot, so the hybrid matters).
5. **Chandelier slots are below the RSSI resolution floor.** 16 shafts with
   ~0.24 m spacing vs a best-case ~0.07 m median position error at near-zero
   noise, ~0.3-0.7 m realistically: within-crown assignment is essentially
   permuted. Chandelier needs manual mapping (16 devices, trivial) or can
   stay unmapped if crown fixtures are interchangeable for show purposes.

## Results -- breakage sweeps

All sweeps: 152 devices, 4 trials/point (median [min..max]), all OTHER noise
terms at realistic defaults (panel spin 20 dB, 5% deep fades, trunk occlusion,
sigma_dev 3 dB, sigma_pkt 2 dB, 50 pkt/link, -90 dBm floor), 3 beacons unless
stated. Data: `ops/locate/data/sim/2026-07-12-sweep-*.jsonl`.

### Core axis: per-link multipath bias (figure AUTOLOCATE_SWEEP_CORE)

| sigma_link | assignment acc | auto-correct | flagged | silent-wrong | pos err (median) |
|---|---|---|---|---|---|
| 0 dB | 0.91 [0.87-0.93] | 0.82 | 0.14 | 0.04 (max 0.07) | 0.30 m |
| 2 dB | 0.91 [0.79-0.92] | 0.82 | 0.16 | 0.04 (max 0.10) | 0.35 m |
| 4 dB | 0.89 [0.86-0.90] | 0.78 | 0.17 | 0.06 (max 0.08) | 0.32 m |
| 6 dB | 0.79 [0.70-0.84] | 0.67 | 0.22 | 0.13 (max 0.17) | 0.49 m |
| 8 dB | 0.67 [0.63-0.72] | 0.57 | 0.24 | 0.19 (max 0.28) | 0.55 m |
| 10 dB | 0.60 [0.50-0.64] | 0.48 | 0.28 | 0.26 (max 0.35) | 0.71 m |
| 12 dB | 0.44 [0.08-0.53] | 0.31 | 0.36 | 0.33 (max 0.65) | 0.88 m |

**The breakage knee sits at ~5-6 dB -- INSIDE the upper half of the estimated
playa band (2-6 dB).** Below the knee the system delivers ~90% correct
assignment with silent-wrong pinned at 4-6%; above it silent-wrong crosses
10% and grows roughly linearly. The measured indoor placement-shift band
(8-17 dB) is clearly past breakage -- indoors this would NOT work, which
matches ADR 0004's caveat. Whether the playa sits below or above the knee is
exactly what the queued small-N real capture measures.

Note the ceiling: even at sigma_link = 0 the composite of the OTHER realistic
noise terms caps accuracy at ~0.91 overall. Per-role at the 4 dB point:
downlight 0.94 [0.90-0.97], perimeter 0.93 [0.85-1.00], uplight 1.00,
chandelier 0.31 [0.13-0.56] -- the non-chandelier fleet runs ~94-95% and the
chandelier is noise (finding 5).

### ToF anchor coverage (figure AUTOLOCATE_SWEEP_ANCHORS, sigma_link 4 dB)

| downlight ToF range | anchors | assignment acc | silent-wrong |
|---|---|---|---|
| none (perimeter only) | 40 | 0.79 [0.69-0.87] | 0.12 |
| 2.5 m | 73 | 0.81 [0.77-0.90] | 0.08 |
| 4.5 m (nominal) | 112 | 0.89 [0.86-0.90] | 0.06 |
| 6.0 m | 112 | 0.89 | 0.06 |

The downward ToF anchors are worth ~10 accuracy points and halve silent-wrong
-- Ben's "pin one DoF" intuition quantified. Degradation without them is
graceful, not catastrophic (the perimeter plane-fit anchors carry the frame).

### Beacons (gauge anchoring), sigma_link 4 and 6 dB (figure AUTOLOCATE_SWEEP_BEACONS)

| beacons | 4 dB: acc / flagged / silent-wrong | 6 dB: acc / flagged / silent-wrong |
|---|---|---|
| 0 | 0.88 / 1.00 / 0.00 | 0.79 [min 0.03!] / 1.00 / 0.00 |
| 1 | 0.88 / 1.00 / 0.00 | 0.79 / 1.00 / 0.00 |
| 2 | 0.88 / 1.00 / 0.00 | 0.79 / 1.00 / 0.00 |
| 3 | 0.89 / 0.17 / 0.06 | 0.79 / 0.22 / 0.13 |
| 4 | 0.89 / 0.17 / 0.06 | 0.79 / 0.22 / 0.13 |

The geometry is usually recovered even without beacons -- but the rotational
gauge margin is measurably thin (ambiguity ratio ~1.0: under a wrong rotation
the dense layout re-matches almost everyone to SOME slot), so the solver
flags the ENTIRE solve as ambiguous (flagged 1.00 -> silent-wrong 0 by
construction, and the 6 dB min-trial 0.03 shows the wholesale-rotation
failure the flag exists for). 1-2 beacons prevent the worst rotations
(min 0.71 vs 0.03) but cannot carve a decisive margin -- and 2 beacons can
NEVER pin the mirror (two points in the plane fit either reflection exactly;
regression-tested after this bit us). **Three surveyed devices is the magic
number**: mirror + rotation pinned in closed form, refined by a restricted
data search, flags drop to per-device levels. A fourth buys nothing.

### Model-mismatch stress arms (figure AUTOLOCATE_SWEEP_MISMATCH, 4 dB)

| arm | assignment acc | silent-wrong |
|---|---|---|
| n_true 2.2 vs assumed 2.7 | 0.81 [0.75-0.86] | 0.11 |
| n_true 3.2 vs assumed 2.7 | 0.89 [0.86-0.91] | 0.06 |
| deep-fade rate 3x (15%) | 0.86 [0.84-0.87] | 0.08 |
| two-ray ground reflection | 0.80 [0.78-0.83] | 0.10 |
| **panel FROZEN (no rotation)** | **0.18 [0.16-0.20]** | **0.63** |

Path-loss-exponent error, triple deep fades, and the two-ray stressor each
cost a few points -- the solver is not brittle to the noise family. The one
catastrophic arm is a FROZEN 20 dB panel shadow: if every lantern's panel
holds one orientation for the whole capture, per-device directional bias
swamps geometry and the solve collapses with 63% silent-wrong. Hanging
lanterns rotate in wind (the spin-averaged default), but perimeter hooks and
any calm-night capture make this a real operational constraint: **capture
RSSI over a period with wind/orientation churn, or this fails.** Measuring
the panel's real angular profile is cheap insurance (the 20 dB raised-cosine
model is an assumption).

## Verdict

**RSSI + ToF auto-configuration is feasible at the optimistic-to-middle playa
noise estimate, with a specific operational recipe -- and it is NOT
hands-free-perfect.** Concretely, at sigma_link 2-4 dB with three surveyed
beacons and orientation churn during capture:

- ~89-91% of all 152 devices land on their correct CAD slot; the
  non-chandelier fleet (136 devices) runs ~94-95%.
- ~15-17% of devices are flagged for a manual check (the fix-up list, ~25
  devices, driven by low assignment margins in the dense canopy);
  silent-wrong is 4-6% (~6-9 devices found only by visual inspection).
- Median position error ~0.3 m against ~0.7-0.9 m fixture spacing.

What it takes (each measured in this study, none onerous):

1. **THREE surveyed beacon devices** (not two: a 2-point gauge cannot pin the
   mirror) -- without them the rotational gauge rests on a 1-2% cost margin
   and the solver honestly flags the ENTIRE solve as ambiguous.
   Hand-recording three devices' slots at install is enough.
2. **Capture with orientation churn** (wind) -- a frozen panel shadow is the
   one tested condition that collapses the method (0.18 accuracy).
3. **A P0/scale reference** -- the CAD footprint (built in) or one measured
   known-distance pair.
4. **Chandelier mapped manually** -- 16 devices at 0.24 m spacing are below
   the RSSI resolution floor in every regime tested.
5. **The calibration gate**: if the real playa sigma_link measures >~6 dB
   (small-N capture, queued in TODO), accuracy slides below ~80% with >10%
   silent-wrong and the method stops being worth its complexity -- fall back
   to assisted-manual (bridge blink + human confirm, which the flagged-list
   workflow already implies as UI).

So the honest answer to "auto-config vs photogrammetry vs manual": RSSI+ToF
gets the fleet ~95% configured for free IF the playa is as RF-clean as
estimated, and degrades to a labeled, flagged, human-checkable list rather
than silently lying (the confidence machinery exists precisely for this).
Photogrammetry is not needed at the optimistic band; budget the small-N
capture BEFORE committing, and budget a visual verification pass (blink each
fixture, confirm its slot) regardless -- it converts both the ~25 flagged
and the ~6-9 silent-wrong devices into fixes at ~30 s each.

## Addendum 2026-07-13 -- CAD patched (Ben's top-down inspection)

Ben eyeballed the CAD top-down and decomposed the 78 downlights: outer ring
24/24 complete, middle 22/24, inner 20/24 distinct (plus 6 fixtures stacked
at duplicate coordinates masking the counts), and 6 strays clumped at the
trunk base -- procedural-export glitches. `patch_cad_0.3.1.py` now moves the
6 strays into the 6 ring holes (slot positions inferred from the rings'
angular gaps); `fixtures-0.3.1-patched.json` is the tooling default until the
refined Blender export lands. Stacked duplicates are left in place (scoring
treats them as equivalence groups). Uplights untouched: their elevated
two-rings-of-12 placement may be intentional (uplighting the upper trunk) in
this design iteration -- earlier drafts of this report treated that as an
artifact; retracted.

Effect at the 4 dB / 3-beacon point (3 seeds): overall 0.82-0.93 (median
~0.91 vs ~0.885 unpatched), with the downlight class at 0.86-0.99 (median
0.97 vs 0.94) -- the trunk strays were exactly the devices the rescue
machinery kept fishing back, and the main-body sweep numbers are accordingly
a touch conservative. Chandelier and the breakage-knee location are
unchanged.

## Addendum 2026-07-15 -- sensor complement = class identity; the missing
## uplight-vs-chandelier distinction is free

The class labels the solver relies on come from hardware: downlights carry
TMF8820, perimeter carries VL53L5CX, so both self-identify on I2C. The
sensorless pair (uplight, chandelier) cannot self-identify -- the sim's 4-way
label assumption was optimistic there. Measured cost of dropping it (merge
the two into one "no-sensor" assignment class, 4 dB / 3 beacons, seeds
7/11/13): ZERO -- identical overall and per-role accuracy, and within the
merged class geometry sorts the two sub-fleets perfectly (uplight rings at
r ~2.4 m vs crown clump at r < 0.5 m, far apart relative to ~0.3 m position
errors; 100% class recovery on all seeds). Deployment implication: no
provisioning step is needed to tell an uplight board from a chandelier board.

## Addendum 2026-07-15b -- single-node replacement is the easy case

Swap scenario (ADR 0009 fungibility): fleet configured, one node dies, a
fresh device is hung. Measured by re-solving with all 151 survivors pinned
as known assignments (the existing pipeline, zero new math): the replacement
was assigned CORRECTLY in 6/6 trials -- downlight / perimeter / chandelier
victims at BOTH 4 dB and 8 dB (past the full-solve knee), position error
0.2-0.6 m, unflagged. The survivors act as ~150 known-position anchors (no
gauge ambiguity, no beacons needed) and the candidate set collapses to the
vacancy list -- even the chandelier becomes unambiguous for a single swap
(one vacant crown slot). Workflow: unconfigured node beacons "whoami" ->
bridge roll-call diffs live MACs vs the fleet map -> neighbors accumulate
RSSI for ~1 min -> pinned solve -> NVS written OTA. Operator effort: hang
the box.

## Caveats / untested model assumptions

- The noise family is Gaussian-per-link + structured terms; real playa
  multipath may be neither. The 2-6 dB "playa band" is an ESTIMATE from
  indoor measurements -- the study's realism gate is a small-N (10-20 board)
  backyard pairwise capture to measure sigma_link before trusting the maps
  (TODO). The two-ray and heavy-tail stress arms bracket, not prove.
- The perimeter ring is synthesized evenly spaced (CAD export lacks the
  class), which is the WORST case for rotational aliasing; real hand-placed
  hooks are less symmetric.
- The CAD export is a procedural first pass (0.3.1): its vertical layout
  disagrees with the fleet spec (downlights near the crown), it contains
  duplicate and near-ground fixtures, and its scale claim is wrong. All are
  parameterized around, but the anchors/geometry conclusions should be
  re-checked when a refined export lands.
- Solar panel shadow is modeled as a raised-cosine azimuthal lobe with full
  20 dB depth on both ends of a link; no measured angular profile exists yet.
- Devices are assumed static over the capture; wind sway is not modeled
  (medians over a long window should absorb it, unverified).

## Reproduce

```
cd ops/locate
./locate_selftest.py                                  # 30 tests
./locate_run.py --sim --seed 7 --sigma-link 4 --beacons 3 --plots --html
./locate_sweep.py --suite core,beacons,anchors,mismatch --trials 4 \
    --workers 8 --plots                               # ~1 h on the workstation
# a sigma_link x sigma_dev heatmap suite ("grid") exists but was not part of
# this campaign; run it with --suite grid if wanted
```

Sweep data + figures: `ops/locate/data/sim/2026-07-12-sweep-*.{jsonl,png}`;
single-run figures + interactive viewer: `ops/locate/data/sim/2026-07-12-sim-*`.
