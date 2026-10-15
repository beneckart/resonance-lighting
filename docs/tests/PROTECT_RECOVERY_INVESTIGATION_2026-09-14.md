# PROTECT recovery investigation, 2026-09-14

## Perimeter apparent-death hypothesis, 2026-10-15

Ben asks whether the perimeters that became unresponsive quickly during the
burn could have latched PROTECT despite subsequently high battery readings,
with disabled sensing explaining the apparent hardware failure.

Marill F40348 now supplies a before/after example: at first direct-USB capture
it reported 3.317 V, PROTECT, and no initialized sensors. The saved entry was
low-vbat at 3027 mV, not reset-origin, and was already present in the September
12 census. After private full-flash preservation, the original firmware
qualified sustained USB charging, released PROTECT and rebooted. Both the
MSA311 and VL53L5CX then supplied fresh readings, all 16 VL zones valid, without
sensor replacement or sensor-cable reseating. See POST_BURN_MARILL_2026-10-15.md.

The late-burn dc82da7 and September f951ae9 artifact source commits have
identical power_policy.cpp, boot_guard.cpp, boot_guard_io.cpp and power_glue.cpp.
The late-burn setup also explicitly disables VSQT and skips sensor initialization
when bootGuardParked(). A protected perimeter therefore cannot respond to a
person through that sensor. LED output is vetoed too, and the usual protected
radio cadence is about 8 seconds listening per 900-second sleep. The device
can look dead while its control/power hardware remains functional.

Joined the existing September 12 high-voltage PROTECT cohort to current
registry perimeter identities: 15 known perimeters at >=3.25 V, with 14
low-vbat entry records and one load-armed-reset record. This is a partial-role
join, not an estimate of the fraction of all perimeters that failed. Inputs,
hashes and exact IDs are in
ops/bench/data/Oakland/20261015-perimeter-protect-cohort.json. August 31's LOG
already identified ten perimeters among the 21 active/probable PROTECT units.

Distinguish three mechanisms: an inappropriate reset-based entry; a transient
or erroneous low-voltage observation triggering the immediate critical-floor
path; and a justified earlier entry that remains latched after voltage recovers.
The critical floor is immediate below compensated 3050 mV with battery
corroboration required for durable persistence; it is not the 60-second
confirmed OFF threshold. The normal release requires good, valid, enabled,
fault-free external charging, >=20 mA and >=3250 mV continuously for 60 seconds.
A separate qualified full/taper proof exists. Resting voltage alone never clears
the latch. Weak/intermittent solar can therefore recover voltage while failing
the awake charging proof, as the earlier offline reproduction showed.

Conclusion: protection-driven loss of sensing is demonstrated; false entry
and the cause/timing of burn-week perimeter failures remain unproven. Next
controlled test should capture cell-terminal and board-input voltage/current
through representative perimeter load and weak/variable solar, alongside
entry provenance, recovery gates and rail state. Do not alter thresholds or
clear guards before preserving other untouched perimeter specimens.

No device commands, firmware changes or new hardware experiments were made
for this hypothesis analysis.

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

## Explicit PROTECT clear availability

The current T-Deck and ESP-NOW protocol have no un-PROTECT command. Wake,
reboot and ordinary OTA do not clear the durable guard. Physical RESET and
power cycling also preserve the NVS latch.

The fixture USB serial CLI has an explicit `X` recovery command, but only for
the historical battery-absent commissioning edge. It requires PowerFeather
ready, battery absent, charging disabled, good external input of at least
4.5 V, and no reported charger fault. It then persists IDLE and reboots. It is
not an installed-battery override and is not exposed over ESP-NOW.

A remote controlled recovery/retry could be implemented in fixture firmware
and the T-Deck; physical intervention is not inherently required by the latch.
That capability does not exist today. Preserve entry evidence before any such
service change. No device command or firmware change was made for this check.

Source: `firmware/fixture/src/esp32/serial_cli.cpp` (`case 'X'`),
`firmware/fixture/src/core/boot_guard.cpp`, `firmware/fixture/src/core/packet.h`,
and `docs/howto/FIXTURE_USB_RESCUE_HANDOFF.md`.

## Ness autopsy follow-up, 2026-10-11

Ness is 9F2738 / D8:85:AC:9F:27:38. The Bodhi Hive session restored USB by
bypassing the enclosure extension, verified ROM identity, then captured the
existing fx-260912-f951ae9-b application. No firmware update or manual guard
clear was used. The inspected boot-guard, power-policy/glue and sleep-audit
files match this artifact's fcc5ef0 source.

The retained entry says `reset-load-armed`, predecessor stage 2 (DIM), reset
reason 9 (brownout), load marker 1, streak 1. It is not a low-VBAT-origin entry.
The same record was already present in the September 12 census, when Ness
was actually in PROTECT at about 3.283 V, with supply-good false and roughly
-130 mA awake current. It predates today's soldering and button presses.

The recorded 3242 mV is a post-reboot audit sample at fixture uptime 2603 ms,
not a capture of the brownout minimum: `sleepAuditRecordProtectEntry` samples
`batteryVolts()` when the new boot writes the record. A transient rail/path
collapse therefore remains possible. Conversely, `load_armed` is a durable
recent-load marker, not proof that a load was physically on at the reset.
With no earlier `allLoadsOff` call, the current ordinary disarm path waits
until the solenoid has been quiet for 60 seconds, then starts another
60-second quiet hold; LED activity can delay it further. This is a potential
misclassification window to investigate, not proof it caused Ness's old entry.

At the first captured application boot today (15:42 PDT), the guard reported
`stage=2 stored=1 park=0 retry=1 interrupted=1 reset=poweron`. Stored stage 1
was FULL, so Ness had already left the historical PROTECT latch before this
capture; the time and cause of that release were not observed. The deliberate
physical RESET consumed a FULL-to-DIM retry. Positive USB charging followed,
and DIM returned to FULL after about 65 seconds. This was not a captured
PROTECT-release test. Two later USER strikes with USB disconnected passed,
with counter 0 -> 2, 40 ms pulses, no blocks/failsafes and continuous uptime.

Assessment: historical PROTECT is proven and its recorded reset-based trigger
matches the implemented rule. An exhausted cell, false entry, or failed release
under qualifying sunshine is not proven. The healthy two-strike test today
does not reconstruct the earlier battery/harness/rail conditions. Service
resets and the age of the load marker deserve focused follow-up, while keeping
real collapse-loop protection intact.

Evidence: `ops/bench/data/ca/20260914-protect-recovery-analysis/fleet-cohort.json`,
the September 12 container snapshots, and
`ops/bench/data/Bodhi Hive/20261011T224018Z-9F2738-physical-reset-capture-88bf9e5b.jsonl`.
