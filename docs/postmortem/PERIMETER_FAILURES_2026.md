# Perimeter failures during Burning Man 2026

Assessment date: October 16, 2026. Evidence cutoff: 17:07 PDT.
Prepared for Ben and the lighting team. Working causal assessment; likelihoods
below are judgments from the retained evidence, not measured failure rates.

## Current assessment

The leading explanation is a combination of limited harvested energy,
new low-voltage trips after battery service, and a recovery policy that could
keep otherwise usable perimeters unavailable. The evidence strongly supports
low measured voltage and PROTECT-driven loss of functionality. It moderately
supports the more specific claim that the awake-current recovery requirement
prolonged outages after the energy situation improved. We have not captured
that complete sequence on a perimeter under burn-week sunlight.

It is not necessary for the original trip to have been false for the recovery
design to have caused an avoidably long outage. Conversely, a high voltage
measured later does not establish that the original trip was false. Genuine
depletion, a brief power-path sag, an erroneous low sample, and a reset-based
trip can all leave a later high-voltage PROTECT observation.

Ben confirms that Shervin worked until about 04:30 a.m. The documented service
window is early August 30 PDT, with 14 retained perimeter service results from
00:14 to 04:30. The earlier 01:11 chat message saying the helper was no longer
servicing is a coordination snapshot, not evidence that Shervin finished for
the night. Ben's report of all-perimeter battery swaps and flag clearing is
accepted as operator history; the independently retained subset is 14 units,
not a complete 24-unit roster. The recalled 40-60 percent starting charge was
not independently calibrated. [Service history](../tests/PERIMETER_USB_SERVICE_HISTORY_2026-08-30.md)

## Evidence that the explanation must account for

| ID | Observation | What it establishes |
| --- | --- | --- |
| E1 | Shervin serviced perimeters through about 04:30 a.m.; Ben recalls battery swaps and clearing PROTECT. Thirteen of 14 retained rows explicitly record guarded release; Clank records replacement of a 0.816 V cell and successful commissioning. | A real recovery boundary exists for the documented subset. A later protected state cannot simply be assumed to be an uncleared pre-service latch. |
| E2 | Ten service rows identify `fx-260829-b0ff5db-b`; four retain a reported `fx-260829-af1d4ec-p`. Both source versions contain the full/taper release fix and the correction preventing ordinary daytime sleep from interrupting the release timer. | Missing those two fixes is not a supported explanation for this documented subset after service. Four reported revisions are not four physical binary readbacks. |
| E3 | A September 1 nighttime snapshot of 20 known perimeters had median VBAT 3.190 V, 19 below 3.250 V, and 16 in PROTECT. | The group did experience broadly low measured voltage. This favors an energy or loaded-voltage problem over a theory in which all cells stayed at high voltage throughout. |
| E4 | August 28 afternoon observations showed five of six identified perimeters with good input but about 0.10-0.23 W and no positive awake battery current. In a later group, only four of 17 ever had positive awake current. | Available input often did not cover the awake load. These observations precede the service boundary and demonstrate the operating condition, not the later fate of each serviced unit. |
| E5 | All 20 September 1 perimeters reported 4.6 V input regulation. The July production-cabling P126 comparison favored 5.8 V; 4.6 V delivered 16.2 percent less charger-input power and about 19 percent less battery charge current in that comparison. | A measured configuration mismatch could reduce already-limited harvest. Those percentages are not measured losses for every burn day. |
| E6 | Skitty and Gengar had approximately 0.01 V / 0 mA charger input in retained daylight observations, unlike peers with weak but present input. | There were near-zero-input outliers. Panel orientation, shade, connection and hardware causes are unresolved. |
| E7 | Eleven of the 14 serviced perimeters appear in September 12's census: ten in PROTECT above 3.25 V and Ditto FULL at 3.305 V. Three were unobserved. | Renewed protection after recorded recovery is strongly supported. It is a selected service/census intersection, not a 10/11 failure-rate estimate for the whole installation. |
| E8 | Those ten protected members retain nine low-VBAT origins at 3.016-3.031 V and one load-armed-reset origin, F402A8, at 3.261 V. All 11 observed members reported no good external input at census time. | Low-voltage provenance dominates this subset. The snapshot cannot show whether a qualifying recovery window occurred earlier in sunlight. |
| E9 | The broader role-matched September high-voltage PROTECT group contains 15 known perimeters: 14 low-VBAT origins and one load-armed reset. | The service subset is consistent with the broader retained pattern. E7-E9 overlap and must not be counted as independent samples. |
| E10 | Marill arrived in PROTECT at 3.317 V with sensors uninitialized. On its original firmware, sustained USB charging released PROTECT and a clean reboot restored both sensors without sensor repair. | Protection alone can explain apparent death and lack of presence response. The original release path can work with adequate input. This does not prove its earlier entry was false. |
| E11 | Daisy was already out of its historical PROTECT state by backup time, despite initial USB silence. Its original extension later passed a byte-identical transfer test. | USB silence is not a reliable diagnosis of PROTECT or a failed extension. Daisy's release time and cause were not observed. |
| E12 | Skitty arrived around 2.7 V at its clips; saved prior sleep voltage was 2.604 V. It autonomously enumerated, reported PROTECT and later reached 3.031 V on USB. | At least this specimen had a real low-voltage condition at intake. Its latest captured voltage remains below the release floor; it has not demonstrated a recovered-battery release failure. |

E1-E2 and E7-E8: [service roster and exact-ID join](../tests/PERIMETER_USB_SERVICE_HISTORY_2026-08-30.md),
including retained source hashes and field ages. E3-E6: September 1 LOG entry
"Perimeter charging diagnosis: P126 setpoint mismatch plus no-input outliers"
in [LOG](../../LOG.md), and [July solar measurements](../tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md).
E9: [perimeter cohort evidence](../../ops/bench/data/Oakland/20261015-perimeter-protect-cohort.json).
E10-E12: [Marill](../tests/POST_BURN_MARILL_2026-10-15.md),
[Daisy](../tests/POST_BURN_DAISY_2026-10-16.md), and
[Skitty](../tests/POST_BURN_SKITTY_2026-10-16.md) intake records.

The entry audit is retained history, not a waveform or a complete event log.
It survives successful recovery. Runtime entry records sample battery voltage
near the transition; reset-origin records sample after reboot and do not
capture the reset's voltage minimum. Neither type supplies a reliable calendar
date for the original fault. Current tier, post-service state, and entry history
must therefore be interpreted together. No continuous burn-week electrical
trace links all three for any member of this subset.

## Why a protected perimeter looked dead

The relevant late-burn firmware parks the LED rail and the sensor rail,
skips sensor initialization, and ordinarily listens for only about 8-9 seconds
between 900-second sleeps. A hand wave cannot wake a sensor that has no power.
A short radio campaign can miss the listening window, and ordinary Wake or
Performance Hold does not override the independent PROTECT policy. This
behavior is implemented and was directly demonstrated by Marill's sensor
recovery. [Policy investigation](../tests/PROTECT_RECOVERY_INVESTIGATION_2026-09-14.md)

The immediate critical floor is below 3.05 V after the configured load
compensation; it is not the separate 60-second low-voltage OFF timer.
Durable low-voltage persistence requires corroboration that a battery exists,
but that does not prove every individual voltage reading is accurate or that
the cell itself, rather than its wiring, sagged. Unexpected resets can also
escalate a previously armed DIM/OFF state to PROTECT. The retained load marker
means a load was armed, not that it was physically drawing current at reset.
[ADR 0051](../decisions/0051-corroborated-protect-and-load-armed-escalation.md)

With both August fixes installed, ordinary release still requires good valid
external input, readable enabled fault-free charging, VBAT at least 3.25 V,
and at least +20 mA net battery current for 60 continuous awake seconds.
A separate qualified full-battery path starts at 3.45 V and also requires
60 seconds plus corroborating battery/charger state. A qualifying streak
keeps the processor awake long enough to finish. Nonqualifying samples reset
the streak; asleep time does not accumulate it. Successful release persists
OFF and reboots before the normal ladder earns DIM/FULL.
[ADR 0068](../decisions/0068-full-battery-protect-release-and-entry-provenance.md)

The charger can put energy into the cell while the processor sleeps, yet the
radio and processor can make net battery current negative at each wake. The
policy then sees no release proof even if the cell is gaining energy over
the full cycle. In the retained synthetic test, assumed +50 mA during a
900-second sleep and -80 mA during a 9-second wake average +48.7 mA, but the
actual policy never starts its release timer at 3.30 V. This proves the
mechanism is reachable in code; the currents and fixed voltage are synthetic,
not a measured Skitty or burn-week charging trace. Intermittent otherwise-good
input also prevents release when no continuous 60-second interval qualifies.
[Reproduction](../../ops/bench/data/ca/20260914-protect-recovery-analysis/reproduction-result.json)

## Relative likelihood of the competing cases

These ratings describe contribution to the observed perimeter outages, not
the chance that every perimeter shared one cause. "Leading" is the best
current explanation; "plausible" fits but lacks a decisive link; "weak" needs
an additional unobserved fault. Confidence describes the strength of the
causal evidence, separately from whether the mechanism exists. The cases can
coexist, so their likelihoods do not sum to 100 percent. Numeric posterior
percentages would imply precision that the selected snapshots cannot supply.

| Case | Relative likelihood and scope | Strength of evidence |
| --- | --- | --- |
| A. Low harvest causes genuine trips; recovery gates prolong some outages | Leading combined explanation; moderately likely to explain a substantial part of the pattern | Strong support for low voltage and limited input; moderate support for recovery amplification in the field |
| B. Genuine depletion and/or repeated trips account for outages without a usable-battery recovery trap | Plausible major contributor and the strongest alternative to A | Strong support for an energy problem; weak ability to reconstruct later release/retrip timing |
| C. Cell or wiring sag trips protection despite useful remaining capacity; rebound and restrictive recovery keep it unavailable | Plausible for some units; less supported as the dominant explanation | Mechanism fits the voltage history; no perimeter fault waveform or resistance measurement proves it |
| D. Erroneous voltage samples trip healthy power systems | Possible but weak as a dominant explanation | Immediate entry permits the mechanism; no paired independent measurement demonstrates a false sample |
| E. Reset/load-marker classification trips a healthy or transiently disturbed unit | Established entry category for one member; false classification itself remains plausible but unproven | Strong evidence for F402A8's recorded category; evidence points away from this explaining most retained perimeter entries |

### Case A: shortage followed by a recovery barrier

Shervin restores operation. Partially charged batteries then support the
lights through poor harvest, eventually reaching the critical floor. Loads
are parked. Later, unloading and some charging raise voltage, but at wake
the available input cannot supply the processor/radio and +20 mA into the
battery continuously. Input variation or invalid charger samples can further
interrupt proof. Presence sensing remains off even though limited operation
might now be possible.

E3-E5 strongly favor the first half; E4, the offline reproduction, and E7-E10
favor the second half. Marill's recovery on stronger USB power is consistent
with this account. It does not tell us whether it had previously accumulated
useful charge in the sun. Daisy can have recovered when its individual input
became adequate, while Skitty can remain depleted because its harvest was
worse. Thus this case allows the observed exceptions without claiming a
universal permanent deadlock.

The missing decisive measurement is positive full-cycle charge together
with failed awake proof and a battery that can sustain useful staged load.
If that is measured on the old firmware, this explanation becomes much
stronger. If every non-release interval instead has negative full-cycle
energy and an unsustainable battery, Case B becomes stronger. If all old
release gates remain valid for a full minute without release, an additional
implementation fault must be investigated; the documented gate alone would
not explain that observation.

### Case B: insufficient energy with normal release and possible retrips

Service restores operation, but the replacement cells are not full and
subsequent harvest does not cover use. Some units remain below the release
floor; some rise above it only after loads are removed. Others might release
under sunshine, run again, and return to PROTECT before the later census.
The retained history does not count all such cycles. A dark-storage snapshot
above 3.25 V cannot exclude this case because voltage alone is insufficient
to establish recovered capacity or a qualified daytime release interval.

E3 and Skitty's independently measured low voltage strongly support actual
energy shortage somewhere in the fleet. The July prototype provides a scale
check: 157.7 mA whole-fixture draw for a 9-10 hour show is about 1.42-1.58 Ah.
If a nominal 6 Ah cell really started at 40-60 percent, its nominal stored
charge would be 2.4-3.6 Ah, or about 1.5-2.5 such shows with no recharge.
Protection reserve would reduce usable charge. This is illustrative arithmetic,
not measured burn-week runtime: actual patterns, duty cycle, starting SOC,
cell capacity and daily harvest are not established for the serviced units.

The seven-day July P126 run, normalized from its overly long bench nights to
9-10 hours, was approximately break-even with little poor-weather margin.
That supports vulnerability to bad weather but does not prove the production
panel was intrinsically undersized. E7-E10 leave open avoidable recovery delay,
so a claim that depletion alone explains every hour of unavailability would
exceed the evidence. This case predicts persistently inadequate energy or
observable release/run/retrip cycles, rather than a charged usable battery
held indefinitely by the proof requirement.

### Case C: real sag with capacity still available

A battery, clip, lead, crimp or power path drops voltage when loaded or during
a transient. The board correctly observes a low voltage and parks, even if
the cell has useful capacity at lower current. Removing load produces a
higher reading. The subsequent release rule may keep the unit parked after
the transient is gone. This can coexist with Case A: depleted batteries and
marginal wiring need not be independent problems.

The low-entry/high-later pattern fits, but by itself does not distinguish
this case from ordinary depletion. The narrow cluster near 3.02 V is also
what a threshold at 3.05 V would select; it is not a signature of a sensor
bug or a particular resistance. The configured 0.15 ohm compensation cannot
prove the actual installed path had that resistance. We lack synchronized
cell-terminal, board-input and load measurements at a perimeter trip.

This is a reasonable explanation for a subset, with low causal confidence
so far. It would gain strong support if the cell stays healthy while board
voltage falls below the floor, or if repeatable load sag clears with a lead
replacement. A real collapse is a reason to retain protection; recovery
should establish safe reduced-load operation rather than blindly clear it.

### Case D: a false low-voltage measurement

A stale, incorrectly valid, mis-scaled or otherwise erroneous low sample
reaches the immediate critical branch while the actual cell and board input
remain healthy. The resulting durable latch then persists until the normal
release conditions are met. This fits the abstract sequence of service,
low-entry record, high later voltage and absent sensing.

However, it requires an additional fault not demonstrated by the perimeter
records. A missing sample is not automatically zero: the implemented policy
checks validity and battery corroboration. Independent low-voltage evidence
from Skitty and the broad September 1 low-voltage group favor a real energy
component. No retained perimeter trace shows a low firmware reading at the
same instant that independent cell and board measurements stayed high.
Accordingly this is weak as the dominant story, while remaining testable for
individual units. A synchronized disagreement, rather than a later high
voltage alone, is the evidence needed to promote it.

### Case E: a reset that the guard interprets as load trouble

F402A8 retains a load-armed-reset origin at 3.261 V, unlike the nine low-VBAT
members of the serviced/census intersection. A real short collapse, an
otherwise benign power interruption, or an overly persistent recent-load
marker could each produce this history. The voltage is sampled after reboot,
so it cannot distinguish those alternatives. Its service notes also record
a reset-origin episode and subsequent successful recovery.

Reset-based entry is therefore a confirmed part of the perimeter story, but
its false-positive character is not confirmed. In the broader high-voltage
perimeter set, only one of 15 retained origins is reset-based. This is strong
evidence against explaining that entire set with benign reset escalation.
Ness provides related reset-marker evidence, but is a canopy and must not be
counted as a second perimeter example. A test should align reset cause,
independent voltage, actual load activity and marker age on one exact unit.

## Contributors and explanations that must remain separate

Weak or absent solar input can feed Cases A and B. E6 establishes an input
problem at the sampled moments, not whether a panel was disconnected,
shaded, dirty, oriented poorly or defective. Gengar was among the serviced
14; Skitty was not. A confirmed near-zero input reading must not be explained
away solely by the awake-current rule or by the 4.6 V versus 5.8 V mismatch.

Some damaged USB extensions are real service faults, but a USB data fault
does not explain retained low-VBAT origins or prove solar charging failed.
Daisy's original extension was vindicated after host reconnection, and
Skitty enumerated without a cable change after waiting. USB symptoms should
not be pooled with presence/lighting outages as one hardware failure count.
Marill's intact sensors after release also argue against assuming widespread
dead sensor hardware from `sensor_bits=0` in PROTECT.

The two known August release bugs are historically real but already fixed
in E2's resulting images. Ordinary flashing also preserves the guard, but
that observation does not negate the recorded releases during this service.
Missing fixes or an uncleared old latch remain possible only for units whose
own service/revision history has not been established.

Reduced canopy chiming is not an independent measure of perimeter failure.
The September inspection image explicitly disabled autonomous canopy chimes.
The separate August 30 downlight warm-white incident was attributed to local
presence overriding color commands after the lights were lowered. Neither
observation establishes a perimeter PROTECT cause.

## Tests that would distinguish the cases

These are proposed tests after preservation and the current passive Skitty
watch; this report does not authorize new firmware or physical intervention.

| Priority | Measurement or comparison | Discriminating result |
| --- | --- | --- |
| 1 | Finish passive Skitty observation and retain raw release/boot evidence, or the explicit gap/deadline. | Release on adequate USB shows that specimen's path works; absence of USB observations alone diagnoses neither charging nor PROTECT. |
| 2 | On an exact old image and known installed battery, independently log signed battery current across sleep and wake, cell/board voltage, and every release gate under weak/variable input. | Net positive cycle charge with failed awake proof supports A; persistent energy deficit supports B. Include a stable-input control that should release. |
| 3 | Exercise representative perimeter sensing/lighting while measuring cell terminals and board input simultaneously, preserving the first trip and reset context. | Board-only sag supports a connection/path cause; cell sag supports cell/load limits; a false firmware low reading with both independently healthy supports D. |
| 4 | Compare panel/input behavior among weak-input peers and near-zero-input outliers; qualify P126 input setpoints with USB fallback preserved. | Separates insufficient illumination, connection failure and regulation losses. A fleet-wide 5.8 V change is not a safe substitute for this comparison. |
| 5 | Compare the old policy with Marill's bounded staged recovery under the same measured conditions, including genuine sag and interrupted trials. | Demonstrates whether availability improves without repeated collapse. Marill already passed USB and battery-only canary cases, not this complete field-condition comparison. |

Marill's October canary is relevant positive evidence for the repair direction:
an injected latch recovered through sensor and limited-LED stages on battery,
with retained minimum 3.293 V, minimum signed current -241 mA and no external
input seen during the trial. An interrupted trial retained its cooldown.
That proves positive charging current is not necessary for this tested staged
recovery. It does not prove Marill's original trip was false or validate all
resistive-harness and weak-solar conditions.
[Canary results](../tests/PROTECT_RECOVERY_MARILL_2026-10-15.md)

## Boundaries of the present conclusion

The strongest established result is functional: PROTECT can disable an intact
perimeter's sensing and light output, and service releases did not prevent
later protected states in much of the retained subset. The best working
causal model adds a real energy problem and a recovery requirement poorly
matched to intermittent low-power harvesting. The balance between those two
contributors remains unresolved. A universal false-latch story, a universal
hardware-failure story and a universal depletion-only story each claim more
than the evidence supports.

At this report's cutoff, Skitty's repaired observer is alive with fresh health
records, but no new device contact since its 16:49 restart. Last actual device
evidence remains 16:06, 3.031 V in PROTECT. Its current state must not be filled
in from expected wake times or the fact that the host logger is healthy.

## October 16, 18:16 PDT addendum: Skitty did recover naturally

Five fresh passive T-Deck reports now establish Skitty F3FD28 in DIM / DAY_CHARGE
on its unchanged August 31 firmware. Uptime and receive ages place its software
boot near 18:07:28 PDT; by observation it had remained up for over eight minutes.
VBAT was 3.277..3.279 V and awake net current +267..276 mA. The release transition
and full qualifying minute were not captured, so this is confirmation of the
recovered state, not a complete trace of the release mechanism. The observer
was stopped normally and the recovery heartbeat paused; Skitty was not reset,
flashed, or configured. See the appended Skitty intake report and 011607Z radio
records for raw evidence.

This fulfills the stable-input control proposed in test 1: old firmware can
recover this specimen under adequate USB charging. It reduces support for an
unconditional, permanently stuck release path in Skitty. It does not rule out
the weaker-input/awake-current mechanism in case A, explain every perimeter,
or convert historical high-voltage latches into proven false trips. The
original low-voltage intake remains evidence of genuine depletion. Earlier
cutoff statements above are retained as the observations available then.
