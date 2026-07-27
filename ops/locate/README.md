# ops/locate -- RSSI + ToF fixture auto-localization

Feasibility tooling for the "autoconfiguring tree": simulate ~150 ESP-NOW
devices with playa-realistic RSSI noise, solve the pairwise-RSSI + ToF-anchor
localization problem into a 3D point cloud, register it onto the CAD fixture
layout, and measure whether each physical device can learn which fixture slot
it occupies -- or whether the project needs photogrammetry / manual entry.
Feasibility report: `docs/tests/AUTOLOCATE_RSSI_SIM_FEASIBILITY_2026-07-12.md`.

## Layering (the reuse contract)

```
locate/    SOLVER LIBRARY -- never imports sim/. Input surface:
           (devices, links, z-anchors, CAD model, path-loss prior, config).
           Feed it real device data and it does not know the difference.
sim/       SIMULATOR -- produces exactly those inputs from the vendored CAD
           plus a parameterized RF/ToF noise model. Imports locate.model only.
tests/     plain-assert test modules (pytest-compatible), run by locate_selftest.py
```

CLIs: `locate_run.py` (one run: sim or real, figures + HTML viewer),
`locate_sweep.py` (parameter sweeps -> breakage curves),
`locate_ingest.py` (capture logs -> contract JSONL),
`locate_selftest.py` (test runner).

## Dependency policy

This directory is the repo's analysis-code area (per Ben, 2026-07-12): numpy +
scipy in the library, matplotlib in the CLI/plot layer only. `ops/bench`
stays stdlib-only. Machine note: the system scipy 1.8 is broken against the
user-site numpy 2.x -- `pip install --user --upgrade scipy` (installed
2026-07-12, scipy 1.15.3). The system mpl_toolkits shadows the user-site one,
so figures use 2D projections (the HTML viewer provides free 3D).

## Data contracts (what real hardware must emit)

Pairwise RSSI JSONL -- one row per direction per aggregation window:

```
{"ts_utc": "...", "tx": "9E5AF0", "rx": "9F2690", "rssi_dbm": -61,
 "n": 12, "n_expected": 50, "censored": false}
```

`rssi_dbm` is the CENSORING-CORRECTED window median: packets below the ~-90 dBm
receiver floor are never received, so a plain survivor median is biased high
(pairs look closer than they are -- this measurably warps the whole solve). The
sender beacons at a fixed rate, so the sent count is known and the true median
is the (n_expected/2)-th largest received sample; when that rank was lost,
`censored: true` and the solver treats the link as "at least this far"
(one-sided constraint). Firmware should aggregate on-device the same way
(`locate/rssi.py:_directional_median` is the reference; `io_jsonl.py`
`rows_from_directed()` / `rows_to_links()` are the codec).

Roster JSON -- device identity, role (known from the hardware complement),
and the ToF-derived height where the class has one:

```
{"devices": [{"dev_id": "9F2690", "role": "downlight",
              "z_tof_m": 2.41, "z_sigma_m": 0.01}, ...]}
```

## CAD ground truth

`data/fixtures-0.3.1.json` is vendored from `app/public/fixtures.json` on
`origin/Lighting-Controller` (commit 0558a5d, blob af0892c). 118 fixtures
(78 downlight / 24 uplight / 16 chandelier); the 38-40 perimeter fixtures are
absent from the export and synthesized as a parametric ring (5 ft, radius
auto = 1.15x canopy). Units: the export's scale is NOT trusted (`meta.units`
says m, spans say otherwise); per Ben the fleet spec's "downlights hang at
7-10 ft" IS ground truth, so the default `--cad-scale auto:downlights` maps
the highest downlight to 10 ft (~0.075 m/unit; plain float overrides it --
the "slider").

**Default CAD = `data/fixtures-0.3.1-patched.json`** (2026-07-13): the raw
export left 6 downlight ring holes (2 middle + 4 inner) and 6 strays at the
trunk base; `patch_cad_0.3.1.py` deterministically moves the strays into the
holes (all three rings now 24 distinct positions). Regenerate with
`./patch_cad_0.3.1.py`; supersede both files when the refined Blender export
lands. Remaining quirks handled in code: 6 groups of stacked duplicate
positions (assignment scores within-group swaps as correct) and 78 downlight
slots vs 72 production devices (rectangular assignment). Uplights are
elevated in this export -- possibly intentional (uplighting the upper trunk),
NOT "corrected".

## Quickstart

```
./locate_selftest.py                      # 29 tests, ~70 s
./locate_run.py --sim --seed 7 --sigma-link 4 --beacons 3 --plots --html
./locate_sweep.py --suite core --trials 5 --workers 4 --plots
./locate_run.py --pairwise pw.jsonl --roster roster.json \
    --beacons-map 9F2690=F010,9E5AB8=P004     # the real-data path
```

Solver pipeline (locate/pipeline.py): censor-guard -> anchored 2D-MDS init
(z measured for ~112 of 152 devices, so xy is solved where the signal variance
is) -> robust NLS in dB space (positions + per-device offsets + P0, Huber,
one-sided residuals for censored links) -> CAD-size scale correction (absolute
scale is near-unidentifiable from RSSI + near-planar anchors; the CAD's overall
radius supplies the one missing scalar, correspondence-free) -> stranded-device
rescue (covariance-proxy detection) -> gauge search / beacon-pinned restricted
search + per-class rectangular assignment -> confidence (exact LAP margins,
registration-ambiguity ratio, optional bootstrap) -> flags.

Two init arms (plain / SMACOF) run by default and the lower registration cost
wins (`--init plain|smacof` to force one): each arm is a different
local-minimum basin and neither dominates across noise draws.

## Verdict metric

`auto_correct / flagged / silent_wrong` fractions: silent-wrong (confidently
misassigned) is the deployment killer. Registration carries a global
ambiguity ratio -- the dense layout re-matches almost everyone to SOME slot
under a wrong rotation, so without beacons the rotational gauge rests on a
~2% cost margin and the whole solve is flagged ambiguous. THREE surveyed
devices ("beacons", `--beacons` / `--beacons-map`) pin the gauge; two cannot
(a 2-point planar gauge fits either mirror exactly).
