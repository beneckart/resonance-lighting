# PROTECT recovery investigation, 2026-09-14

Ben identified persistent high-VBAT PROTECT as a possible explanation for much
less chiming during the last days at Burning Man. Treat this as a priority
recovery/reliability investigation. High voltage alone does not prove a healthy
cell or an erroneous guard, and the available September 12 dark-storage census
does not establish whether a particular unit failed to recover under sunshine.
The Oakland units stay on their existing operating policy, with no new storage,
OTA, guard clear, or other device change during this investigation.

## Fleet evidence

The retained September 12 census contains 106 fixtures, 74 in PROTECT:

- 40 PROTECT fixtures were at or above 3.25 V; seven were at or above 3.30 V.
- All 40 reported supply-good false. This snapshot contains no qualifying
  charging window with which to prove an automatic release failure.
- Their retained entry origins are 35 low-VBAT, four load-armed reset, and
  one legacy/unknown. These are firmware-observed histories, not independent
  cell-terminal measurements or proof that every current latch was erroneous.
- The high-voltage group contains 18 f121868 inspection fixtures, 15 dc82da7
  visibility fixtures and seven b3e2738 final-burn fixtures. Exact IDs/callsigns,
  voltage ages, entry records and manifest source ancestry are retained in
  `ops/bench/data/ca/20260914-protect-recovery-analysis/fleet-cohort.json`.

## Known recovery bugs versus current code

Two older failures were already reproduced on hardware:

1. Rikku: a near-full battery accepted only 0-2 mA, so the old +20 mA release
   proof never completed. ADR 0068 / commit 5865282 added the qualified
   full/tapered-battery alternative.
2. Toad: independent 12-second DAY_CHARGE sleep repeatedly erased the RAM-only
   60-second proof even with valid input and about +506 mA charging. Commit
   3e44ad8 gave power_policy sole ownership of PROTECT sleep. Logan subsequently
   proved recovery through a genuine timer wake on the repaired image.

The immutable manifests and git ancestry establish that all three September 12
fleet revisions already included BOTH fixes before that day's storage OTA.
Groot's b0ff5db service image also contains both. Kiki's older 658b7d2 image,
from documented source 91663fd, contains neither. Kairi/Wooper's exact source
mapping was not established in this analysis; do not classify their fixes from
the date-looking revision alone.

## Remaining policy limitation reproduced offline

Compiled a small injected-sample harness against the unchanged current
`power_policy.cpp`. The test touches no device. Result and exact compiled source
hash are in `reproduction-result.json`; executable was temporary and not retained.

| Synthetic input | Observed policy result |
|---|---|
| 3.30 V, valid good input, no fault, +50 mA steady charging | Releases at 61 s in the one-second tick harness |
| 3.55 V, same gates, CV phase, +2 mA | Full-battery path releases at 61 s |
| 3.30 V, valid good input, -80 mA during listening | No release over 600 simulated awake seconds |
| 3.30 V, +50 mA interrupted by 5 s at zero every 50 s | No release over 600 simulated awake seconds |

A separate 20-cycle run resets RAM on each simulated 9-second wake followed by
900 seconds asleep. With an illustrative source charging at +50 mA during sleep,
and 130 mA additional awake draw giving -80 mA during listening, mean battery
current is still +48.713 mA. Nevertheless, the actual policy never starts its
positive-current proof during those wake samples at 3.30 V. The >=3.45 V full
battery path does not apply. Sleep charge is an external model assumption;
this harness is not a simulated charger/cell or physical evidence of this fault.

This is a reproducible recovery limitation to assess against field requirements:
a battery can gain usable charge while asleep without satisfying the continuous
awake recovery proof. Input flicker, failed status reads or proof-type changes
can also repeatedly reset the timer. This does not justify simply clearing
PROTECT above 3.25 V; legitimate load/path collapse remains possible.

## Chiming has a separate deployment explanation

The Aug 31 final-burn b3e2738 and later dc82da7 source permit ordinary wake chimes
and hourly rituals in FULL/DIM, with the normal class, time and authority gates.
OFF/PROTECT veto those autonomous chimes. By contrast, f121868 enables static
inspection mode, explicitly disabling both autonomous wake chimes and hourly
rituals independent of battery tier. The September 1 rollout verified this
inspection image on 70 of 114 fixtures. Not all 70 are chime-equipped canopies.
This is a concrete additional contributor to quiet during the last Burning Man
days; reduced sound is not an independent proof of a recovery failure.

References: `docs/tests/ADR_0074_PARTIAL_FLEET_ROLLOUT_2026-09-01.md`, ADR 0074,
ADR 0068, and the August 29 Toad/Logan/Groot entries in LOG.md.

## Bench work needed before a repair

- Preserve Oakland revision/configuration/NVS and entry/sleep/charger evidence
  before resetting guards or updating. Map the physical battery/harness first.
- Compare Kiki (known older release bugs), Kairi (3.303 V with a recorded
  2.993 V low-VBAT/unarmed entry), and Groot (both fixes, now 2.880 V).
- On an installed known-good cell and stable supply, log every recovery gate
  over at least one complete proof window and verify return through FULL/DIM
  and actual chime eligibility. Isolate cell/path failure from software state.
- Then test weak/variable input while measuring current across sleep and wake,
  rather than treating awake gauge current as whole-cycle charge history.
- If a new recovery policy is warranted, keep rail-off checks, qualified battery
  evidence, bounded retry and collapse-loop protection. Reproduce its positive
  and negative cases before any fleet deployment.
- Expose why recovery is waiting and why autonomous chiming is ineligible in
  future diagnostics; a voltage plus PROTECT badge conceals the needed evidence.

## Wake-app interaction

PROTECT exception: Wake Fleet and Performance Hold do not override the power
policy's sleep decision or clear its durable latch. A protected fixture normally
listens for about 8-9 seconds between 900-second sleeps; receiving Wake refreshes
ordinary lifecycle/control state, but not the independent PROTECT sleep grace.
The six-minute Wake campaign cannot guarantee catching a 15-minute PROTECT
cycle. Performance Hold repeats long enough to catch that cycle, but does not
keep a still-protected fixture continuously reachable. Qualified recovery or
OTA/maintenance can separately change its awake behavior. Continuous radio
claims apply only while the fixture's power policy permits them.
