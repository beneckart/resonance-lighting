# 16 · SYNC / CALIBRATION PROTOCOL — how the lights find themselves on the tree

*2026-07-05 · lighting-architect · implements Elliot's staged sync spec.*
*Code: `app/src/selfmap.ts` (solver) · `app/src/syncproto.ts` (wire frames) ·
`app/src/SelfMapPanel.tsx` (operator flow) · `app/src/calibration.ts` (persisted map v2).*
*Brainstem facts referenced: `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md`
(RSSI numbers), `BACKGROUND.md` §mesh (flash-stored position/neighbor lists),
`firmware/presence_bench` (VL53L5CX).*

---

## 1 · The problem

~118 physical lights ship with nothing but a MAC address. The 3-D model
(`fixtures.json`) says where lights are *supposed* to hang; the real install
will drift from it — and nobody wants to hand-identify 118 lanterns from a
ladder. The protocol below lets the fleet **map itself**, with people touching
only the lights the data genuinely can't pin down.

## 2 · The confidence ladder

Every fixture climbs this ladder; the cortex tracks the stage per MAC and the
twin renders it. Nothing ever silently moves DOWN the ladder.

| stage | meaning | produced by |
|---|---|---|
| `heard` | MAC announced on the mesh | boot announce |
| `ranged` | has an RSSI neighbor table | stage 1 survey |
| `placed` | embedded in the 2-D plan | stage 3 solve |
| `height` | ToF vertical fix attached | stage 2 |
| `hypothesis` | matched to a model slot + confidence | stage 3 solve |
| `confirmed` | installer verified (flash → tap) | stage 4 |
| `locked` | photogrammetry residual accepted; frozen | stage 5 |

## 3 · The five stages

### Stage 1 — mesh 2-D survey (RSSI)
Cortex broadcasts `cal_survey {session, durationS, minPings}`. Each node counts
the packets it already hears (heartbeats — the survey adds **zero radio duty**,
it only turns on bookkeeping) and rides a `cal_rssi` report on its heartbeat:
**median dBm per heard neighbor** (median, not mean — a walking body or rain
squall must not poison the survey).

Physics honesty (from `net_bench`): boards read **8–17 dB apart** and RSSI has
~±2 dB residual after medianing, so a range estimate is off by up to ~2.3×.
RSSI is a **topology** signal, not a tape measure. The solver therefore
estimates a **per-board bias jointly with the geometry** (a hot board shrinks
*all* its ranges by one factor — observable and removable) and uses ranges for
*ordering and rough shape*, not centimetres.

### Stage 2 — vertical fix (ToF)
Each lantern's downward VL53 ranger reports `cal_tof {heightM, sigmaM, clear}`.
**Hardware honesty:** the VL53L5CX ceiling is ~4 m, so `heightM` is "range to
the nearest surface below", true height-above-ground **only when `clear`**
(stable, planar return — not foliage). The solver treats non-clear returns as
no-fix and falls back to model-height band capacity. Where a clear fix exists
it is the highest-SNR signal we have (σ ≈ 8 cm vs metres of RSSI fuzz) — it
nearly sorts the fleet into the right level of the tree by itself.

### Stage 3 — solve against the model prior
The cortex fuses everything (`selfmap.ts / solveMapping`):

1. **Bias-corrected ranges** → ToF-projected onto the horizontal plane
   (`dxy = √(d² − Δh²)`) → weighted **SMACOF embedding** (near pairs trusted
   most; installer-confirmed anchors **pinned**, dragging the cloud into the
   model frame).
2. **Band-circular assignment** — the primitive that matches how a tree is
   laid out: per role, model slots cluster into height bands; ToF ranks nodes
   into bands; within a band, nodes sorted by *embedded azimuth* are matched to
   slots sorted by azimuth at every rotation × both directions, each candidate
   scored directly against the **measured ranges** (log-domain — RSSI noise is
   additive in dB).
3. **2-opt + iterated-local-search polish** on the same measured-range
   objective (swap/move assignments while the fit improves; random kicks
   escape ring-rotation local minima).
4. **Confidence per light** = how much the fit worsens under the light's best
   alternative slot, normalized by the fleet median. This is the honest
   "should a human check me?" number.

Hypotheses are pushed to nodes as `cal_assign {stage:"hypothesis"}` — each node
stores its slot + position in flash (the storage Ben's architecture already
reserves for position/neighbor lists), so patterns needing position work even
if the cortex dies (the cortex is optional; the mesh is not).

### Stage 4 — install-time confirm (the ACTIVE LOOP)
This is the workflow that actually saves install days:

```
while accuracy not accepted:
    solve → sort lights by confidence
    installer flash-confirms ONLY the ~10 shakiest   (identify-flash → tap slot)
    each confirm becomes an ANCHOR → re-solve reorients everything else
```

Every confirm is worth far more than one light: anchors pin the embedding and
force band rotations. Measured on the real 118-fixture export in the worst
connectivity case simulated (45–60 m radio on the ~100 m canopy):
**32% → 70% → 86% → 100% accuracy at ~40 confirms instead of 118 manual IDs**
(live run, `rtl-selfmap-*.jpeg`). With good connectivity the solver starts at
~63% with *zero* anchors and needs far fewer.

Special cases the solver knows about:
- **Coincident/near-coincident fixtures** (the export contains some, and the
  chandelier cluster sits at ~1.9 m spacing): no amount of ranging separates
  them — they surface at the bottom of the confidence queue by construction
  and are resolved by flash-confirm or photogrammetry.
- **Symmetric layouts** (rings): rotations are near-equivalent hypotheses;
  without an anchor the orientation is genuinely unknowable. One confirm per
  ring breaks it.

### Stage 5 — photogrammetry lock
The AutoCal solo-step (each light alone, R/G/B/W) already provides the capture
windows: cameras see exactly one light per step; the log ties frame-time →
fixture id. Triangulated positions replace estimates, residuals vs the map are
checked, then `cal_lock {mapVersion, mapHash}` freezes the fleet. The hash
(order-independent FNV over the mac→slot table) is recomputed by both sides on
every reconnect — **a mismatch means the map drifted** (a swapped repair unit,
a re-hung lantern) and the twin flags it instead of lying.

## 4 · Wire summary (Protocol v1.1 — the seam Ben implements)

| frame | dir | when | payload |
|---|---|---|---|
| `cal_survey` | ↓ bcast | open session | `session, durationS, minPings` |
| `cal_rssi` | ↑ heartbeat | during session | `mac, role, rows[{mac, med, n}]` |
| `cal_tof` | ↑ heartbeat | during session | `mac, heightM, sigmaM, clear` |
| `cal_assign` | ↓ unicast | after solve / confirm / lock | `mac, fixtureId, stage, pos, confidence` |
| `cal_ack` | ↑ | after assign | `mac, fixtureId, stage` |
| `cal_lock` | ↓ bcast | map accepted | `session, mapVersion, mapHash, force` |
| `identify` | ↓ | stage 4 flash | (exists — `calibration.ts`) |

All frames are validated as untrusted input (`isCalFrame`). Control-plane only,
low-rate, no pixel streaming — ADR 0004/0010 honored.

## 5 · What swaps when hardware arrives (no rework)

| sim today | hardware later | unchanged |
|---|---|---|
| `simulateSurvey()` (model + drift + net_bench noise) | real heartbeat riders | `foldSession()`, solver, panel, map |
| `clear:true` everywhere | firmware ground-plane heuristic on the 8×8 zones | ToF gating logic |
| installer "confirm" = sim truth tap | identify-flash → tap in Commissioning panel | queue ordering, anchor mechanics |
| accuracy vs sim truth | photogrammetry residuals | lock flow + hash |

## 6 · Open questions (for Ben / next sessions)

1. **Heartbeat budget**: a 118-row RSSI table won't fit one ESP-NOW frame
   (250 B). Proposal: nodes report **top-16 neighbors by median RSSI** — the
   solver only trusts near pairs anyway. Needs a firmware-side cap constant.
2. **Survey TX cadence**: is ~1 Hz heartbeat enough ping density for
   `minPings=12` within a 120 s session, or should survey mode raise the
   announce rate temporarily (still parameter-frames, still low duty)?
3. **ToF ground heuristic**: which 8×8-zone statistic marks a return "clear"
   (planar fit? variance gate?) — presence_bench data can answer this.
4. **Anchor recipe**: cheapest install-day plan is confirming ~1 light per
   ring/level *first* (breaks every rotation ambiguity up front), then letting
   the active loop mop up. Worth writing into the install runbook.
