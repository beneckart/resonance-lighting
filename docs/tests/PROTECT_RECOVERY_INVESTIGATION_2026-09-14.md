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

## 2026-10-16 -- Shervin battery-service recollection and revised failure history

Ben recalls that Shervin replaced batteries on every perimeter on one night
during the burn, and also made sure to clear any PROTECT flag. Ben recalls
replacement cells generally being about 40-60 percent charged. A few days
of poor weather and little sun followed; the original hypothesis was that
the replacement cells were depleted. Ben now suspects renewed inappropriate
PROTECT entries and/or failed recovery prolonged the apparent outage after
sunshine improved.

This is a newly recorded operator recollection. Searches of current tracked
repo text and relevant journal/document history did not locate a named,
contemporaneous Shervin service record. The night, exact serviced roster,
charge-estimate basis, clearing method, and fresh post-service state remain
unresolved. Do not replace the reported clearing action with an assumption
that he only pressed RESET. Equally, do not mark an effective clear verified
without the procedure or a fresh non-PROTECT reading. The known firmware
preserves PROTECT across ordinary reset, power cycling and ordinary OTA;
the documented serial X clear is restricted to battery-absent service.

### What an effective clear changes

If service genuinely cleared the durable state, simple carryover of the
pre-swap latch cannot explain the subsequent protected state. The fixture
must have entered PROTECT again after that clear. That new entry could still
be a justified low-voltage stop following an energy deficit, a real transient
voltage/path collapse, or an inappropriate measurement/reset classification.
The later stuck state alone does not distinguish those entry mechanisms.

A coherent working hypothesis is therefore:

1. Battery service and an effective guard clear restore a usable starting
   state (reported history; normal post-service operation is not yet logged).
2. Insufficient harvested energy and/or load/path transients cause new
   PROTECT entries. Some entries may be inappropriate; that is not required
   for the next failure mode to occur.
3. PROTECT disables sensing and lighting and limits radio availability,
   making the fixtures appear dead.
4. Later charging restores voltage, but awake current or other release gates
   repeatedly fail the continuous 60-second proof. Functional outage can then
   outlast the original energy shortage. Bright site weather alone does not
   establish a qualifying power window at each fixture's charger.

This is a hypothesis consistent with the evidence, not an established
per-fixture chronology or proof that all perimeters shared one cause.

### Relevant retained evidence

- The September 1 LOG entry "Perimeter charging diagnosis: P126 setpoint
  mismatch plus no-input outliers" reports 20 known perimeters at 4.6 V
  VINDPM, versus the production-cabling P126 qualification at 5.8 V. The
  cited July 10 comparison found a 16.2 percent charger-input-power penalty
  and about a 19 percent charge-current penalty at 4.6 V. Those are specific
  comparison results, not a measured percentage loss for every burn day.
- That same entry reports August 28 daytime observations: five of six
  identified perimeters had good charger input but only about 0.10-0.23 W,
  with no positive awake battery current. A later group had positive awake
  current on only four of 17 fixtures. These observations support the
  awake-current mismatch mechanism but do not measure full-day energy.
- Skitty and Gengar instead had a near-zero-input signature in those retained
  records. Do not assign every fixture's behavior solely to the release rule.
- The September 12 cohort contains 15 known perimeters at >=3.25 V while in
  PROTECT, with 14 low-vbat-origin histories and one load-armed-reset history.
  These were non-charging snapshots; they do not prove failure under strong
  solar input or independently establish that initial trips were false.
- Marill's preserved intake demonstrates the visible symptom: 3.317 V and
  PROTECT with sensors uninitialized, then sensor recovery on the original
  firmware after a qualified USB charge release, without sensor repair.
- Skitty's current intake has real low voltage (about 2.7 V at the clips,
  saved prior sleep 2.604 V) and remains below the release floor in the latest
  captured wake. Its current case does not yet demonstrate failed release
  after restored battery voltage.

Keep the battery-service report as a boundary to investigate. Matching its
date and procedure to post-service telemetry and later entry provenance is
more discriminating than assuming either universal depletion or universal
false latching. The 40-60 percent estimate is not a calibrated energy budget
without knowing how the replacement cells were assessed.

### Follow-up: carried-laptop USB service, probably August 28-29

Ben clarified on October 16 that Shervin carried this laptop, replaced the
batteries, and flashed any fixtures then running older firmware, all through
USB. He recalls this being a day or two before Burning Man officially opened.
The official 2026 opening was August 30 at 00:01, making August 28-29 a useful
approximate search window, not a confirmed service date. Source:
https://survival.burningman.org/city-infrastructure/2026-city-plan/

This clarifies the service method but does not identify an explicit guard
clear command, an erase operation, or the post-service state. Preserve Ben's
earlier recollection that Shervin made sure to clear PROTECT; do not equate
selective firmware updates with either proof of clearing or proof of failure
to clear. Relevant contemporaneous counterexamples are:

- August 29, Logan 9E5A88: the LOG explicitly records that the retained
  PROTECT latch survived both a USB upload and a capacity-configuration
  reboot. It subsequently cleared through the corrected natural recovery
  path. An ordinary upload alone did not clear this fixture.
- August 29, Eevee 9F0E54: physical RESET plus USB charging gave the old
  firmware enough continuous awake time to finish its healthy-battery
  proof and clear PROTECT before the upload. Service can therefore really
  clear a latch without the upload itself being the clearing mechanism.

The approximate service window overlaps two fixes: August 28's qualified
full-battery/taper release (commit 5865282), and August 29's correction of
the independent DAY_CHARGE sleep deadline interrupting a 60-second recovery
proof (commit 3e44ad8). Identify the exact installed revision before assigning
either bug to a serviced perimeter. The September fleet revisions already
include both fixes, but still require qualifying awake recovery evidence.

The August 27 USB-powered recovery batch is adjacent evidence, not an
identified match for Shervin's walk: its documented uploads were OTA through
the T-Deck, and the rotated fixture cables were reported as power-only.
See USB_POWERED_FIXTURE_RECOVERY_2026-08-27.md and the August 27 LOG entry
"Rotated-power cohort updated; cadence holdbacks closed".

The working history remains conditional: if the post-service state was
unprotected, later PROTECT requires a new entry. That entry can be legitimate
while the later inability to recover is still a firmware failure. Exact
service records and fresh post-service telemetry remain the missing link;
no device state was changed to investigate this recollection.

### Follow-up: historical service results located

The October 16 archive audit found a strong match after expanding the search
through early August 30 PDT. At 00:55 PDT Ben explicitly reported a helper
manually USB-flashing perimeters. Side-branch commit 2315ef6 preserves 14
perimeter service-result rows from 00:14-04:30 PDT: ten exact b0ff5db USB
updates and four existing reported af1d4ec images. Both sources contain both
the full-battery release and daytime-sleep-interruption fixes. Thirteen rows
explicitly record guarded release, and Clank records replacement-battery
commissioning. This is substantial support for a recovered post-service
starting state in this subset. Shervin is not named in the contemporary
message, and an all-24-fixture roster is not established.

See [Perimeter USB service history](PERIMETER_USB_SERVICE_HISTORY_2026-08-30.md)
for the roster, artifact/source proof and evidence limits. These committed
service notes are not raw upload transcripts; the four existing images have
reported-version evidence rather than per-device binary hashes. Skitty,
Marill and Daisy are absent from this subset. Both images retain the
awake-current-dependent release gate, so the later recovery question remains
open even where the earlier two bugs and pre-service latch carryover are
excluded by the recorded service results.

### October 16 follow-up: service boundary versus container census

An exact-ID join finds 11 of the 14 documented August 30 serviced perimeters
in the September 12 census. Ten had returned to PROTECT above 3.25 V; their
retained origins are nine `low_vbat` entries at 3.016-3.031 V and one
`load_armed_reset` entry (F402A8) at 3.261 V. Ditto was FULL at 3.305 V;
three members were unobserved. All 11 reported no good external input at
the census, so this is not direct evidence of failed recovery in sunlight.
It supports renewed entries after service but does not establish whether
individual low-voltage trips were justified or how long they persisted.

The local Codex archive also says the USB helper stopped at 01:11 PDT on
August 30, whereas service-result timestamps extend to 04:30. Do not assign
the whole subset to one continuous Shervin service walk without resolving
that discrepancy. The exact-image recovery-fix findings remain intact.
The service-history report contains the join, source hashes, entry details
and distinction from the separate downlight presence/color incident.

### October 16 operator clarification and perimeter synthesis

Ben explicitly confirms: "Shervin definitely worked until 4:30am." This
resolves the preceding uncertainty about the end of the service night.
The 01:11 chat statement should be treated as a coordination snapshot, not
evidence that Shervin stopped for the night. The retained 00:14-04:30 August
30 service results align with his confirmed hours; a complete all-perimeter
roster and independently measured starting charge are still unavailable.

The [perimeter postmortem](../postmortem/PERIMETER_FAILURES_2026.md) now
separates entry from recovery and compares five compatible causal cases:
low harvest plus recovery delay, energy shortage without an avoidable delay,
real cell/path sag, erroneous voltage samples, and reset-marker escalation.
It includes relative likelihoods, evidence strength, counterevidence and
tests that would distinguish them. The leading mixed explanation has
stronger support for low voltage/limited harvest than for its unobserved
burn-week recovery-delay sequence. No fixture change was made for this report.
