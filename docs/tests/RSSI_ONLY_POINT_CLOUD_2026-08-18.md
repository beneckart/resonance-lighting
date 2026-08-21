# RSSI-only point cloud from the Nevada City rig capture -- 2026-08-18

Date: 2026-08-18. Status: COMPLETE (exploratory offline analysis; not installation
coordinates). Owner: Ben + Codex.

## Question

Can the pre-pack Nevada City RSSI capture produce a 3D point cloud with no other
a priori knowledge?

The source is JSONL, despite the initial shorthand "CSV":
`c322562:ops/locate/data/field/20260818-0300-nevada-city-rig-rssi.jsonl` in the
`basic-listener` worktree. The solver deliberately reads only `tx`, `rx`, and
`rssi_dbm`. It ignores the capture note, roster, fixture class, ToF, CAD, rig
dimensions, known positions, and path-loss calibration.

## Method

`ops/locate/locate_rssi_cloud.py` is an unanchored sibling of `locate_run.py`:

1. Median-aggregate repeated neighbor-table EWMA snapshots per directed pair.
2. For each reporter, turn its local RSSI ranking into ordinal constraints:
   `distance(stronger, reporter) < distance(weaker, reporter)`.
3. Fit a dimensionless embedding plus a regularized per-transmitter radio bias.
4. Hold out 20 percent of each reporter's directed links before building the
   training triplets. Fit additive transmitter/receiver terms on the remaining
   links and score prediction of the hidden RSSI.
5. Repeat the fit in 1D through 5D. Separately compare early- and late-half 3D
   embeddings after Procrustes gauge alignment.

No metric RSSI-to-distance conversion is made. Coordinates are necessarily
arbitrary up to translation, rotation/reflection, and scale.

## Capture quality

- 25,154 observation rows.
- 96 transmitters/devices; only 48 are reporters.
- 4,558 unique directed pairs and 3,430 undirected pairs; the graph is connected.
- 1,128 undirected pairs are reciprocal. Their median absolute directional
  asymmetry is 1 dB (95th percentile 3 dB).
- A directed pair has a median six snapshots, but these are repeated EWMAs rather
  than independent packet windows. Fifty-six percent never change; 90 percent
  span at most 2 dB over the run. The repeated row count therefore overstates the
  independent RF evidence.

## Link-holdout result

| Embedding | Hidden-link RMSE | Gain over radio-bias-only baseline | Median within-reporter rank rho |
|---|---:|---:|---:|
| 1D | 9.27 dB | 0.72 dB | 0.479 |
| 2D | 7.30 dB | 2.69 dB | 0.642 |
| 3D | 6.77 dB | 3.22 dB | 0.650 |
| 4D | 6.70 dB | 3.29 dB | 0.691 |
| 5D | 6.37 dB | 3.62 dB | 0.718 |

RSSI clearly contains useful relative topology: 2D and 3D predict held-out links
far better than transmitter/receiver bias alone. It does not select a physical
3D Euclidean model. The 2D -> 3D rank improvement is only 0.008, while 4D and 5D
continue improving. Extra latent dimensions are still absorbing real structure,
consistent with antenna orientation, panel shadow, multipath, enclosure/device
bias, and missing reporter directions rather than literal spatial dimensions.

The 3D fit is visually a reproducible latent cloud, not the known rectangular
rigs. Unsupervised k-means chooses four weak/moderate clusters (silhouette 0.428;
sizes 25, 17, 30, 24), with no defensible mapping to the physical 6x12, 2x12, and
1x8 arrangements. Early and late halves have high pair-distance correlation to
the full fit (rho 0.983 and 0.985), but this is weak evidence because they are
successive reads of nearly unchanged on-device EWMAs, not independent surveys.

## Verdict

**Yes, an RSSI-only relative 3D latent point cloud can be extracted. No, this
capture alone does not identify a physical 3D point cloud.** The output is useful
for neighbor selection, topology visualization, outlier detection, and as an
initialization for the existing anchored solver. It must not be labeled meters,
vertical, north, or installation position.

This is partly a fundamental limit, not only a solver limitation. RSSI alone
cannot determine origin, orientation, handedness, or absolute scale. With only
half the devices reporting, tx-only nodes also never provide their own radial
ordering. A 3D optimizer can always spend its extra freedom explaining RF bias.

## Best next capture

For the strongest possible RSSI-only attempt before adding geometry:

1. Make all devices reporters.
2. Record true per-packet/window medians plus sent/received counts, not repeated
   neighbor-table EWMAs.
3. Capture long enough for lantern/panel orientation churn and repeat the whole
   survey independently.
4. Demand a 3D cross-validation elbow and stable pairwise distances across the
   independent surveys before calling the latent cloud geometric.

Even a perfect RSSI-only relative cloud still needs one external scale fact and
a gauge reference before it can become physical coordinates. The existing
`locate_run.py` CAD/ToF/beacon path supplies those facts when the task changes
from no-prior exploration to installation registration.

## Reproduce

```
python ops/locate/locate_rssi_cloud.py \
  C:\Users\benec\Documents\resonance-tree-worktrees\basic-listener\ops\locate\data\field\20260818-0300-nevada-city-rig-rssi.jsonl \
  --out ops/locate/data/field/20260818-0300-nevada-city-rig-rssi-cloud.json
```

Derived report:
`ops/locate/data/field/20260818-0300-nevada-city-rig-rssi-cloud.json`.
