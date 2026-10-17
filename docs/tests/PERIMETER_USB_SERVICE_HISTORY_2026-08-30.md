# Perimeter USB service history: August 29-30, 2026

Reconstructed on 2026-10-16 from local Codex archives, retained Git history,
and an immutable firmware artifact. This is a historical audit, not a new
hardware test. No fixture commands or configuration changes were made.

## Finding

The strongest match to Ben's recollection of Shervin's battery/USB service
is an early-August-30 batch, with recorded completions from 00:14 to 04:30 PDT.
The preserved records cover 14 perimeters: ten exact USB service-image
updates to `fx-260829-b0ff5db-b`, and four fixtures already reporting
`fx-260829-af1d4ec-p`, whose firmware was retained. Both image sources contain
the August 28 full/tapered-battery PROTECT release fix and the August 29 fix
preventing the ordinary daytime sleep deadline from interrupting recovery.

Thirteen records explicitly describe successful guarded PROTECT release.
Clank's remaining record describes replacement of a 0.816 V battery and
successful post-flash commissioning. This supports Ben's recollection that
service addressed the protected state, rather than merely uploading firmware.
It does not establish an all-24-fixture service roster. The contemporary chat
calls the operator a helper; Ben identifies him as Shervin and explicitly
confirmed on October 16 that Shervin worked until 04:30 a.m. This operator
clarification resolves the earlier uncertainty about his finishing time.
See the [perimeter postmortem](../postmortem/PERIMETER_FAILURES_2026.md) for
competing causal explanations and their current evidential support.

## Evidence and its limits

1. Local Codex session `01a04b6c-ff3a-74b0-a0b0-df4009987ebc`, decompressed
   line 7603, at `2026-08-30T07:55:27.719Z` (00:55 PDT): Ben says, "I have a
   helper manually usb flashing perimeter lights" and asks to set their
   targets aside from OTA. Line 7743 records the recommended helper artifact
   as `fx-260829-af1d4ec-p`. That is a handoff, not proof it was the image
   actually flashed by the helper.
2. Git commit `2315ef69f6febf61148e1e9b79bd8a13474c8c7d`, "ops: preserve BRC
   field recovery evidence", preserves the uncommitted registry work from
   the August 30 perimeter recovery session: 13 revised existing rows and
   one new identity. Its LOG entry explicitly describes that preservation.
   This commit is on the retained daytime branch and is not an ancestor of
   the current decompression checkout. Searching only current files missed it.
3. Those registry notes record actual service results, including the exact
   `b0ff5db` binary SHA for ten uploads. Their timestamps are explicit UTC
   and are converted here to PDT (UTC-7). Commit author timezone offsets
   vary; commit display dates were not used to date the individual services.
4. For four existing `af1d4ec` images, the operator deliberately left the
   canonical firmware/hash columns blank because that checkout lacked the
   matching artifact. The reported revision is evidence, not a per-device
   binary readback. This audit found the artifact in the retained daytime
   worktree and verified its hash, which resolves source identity but does
   not retroactively supply a readback of those four physical fixtures.
5. No matching raw per-device serial/upload transcripts for these 14 services
   were found in the local August sessions, current data directory, or
   retained worktrees' data directories. The archived chat and committed
   service-result notes are the available evidence. They are stronger than
   a planned update, but should not be relabelled raw serial measurements.

Selected, non-secret evidence is preserved under
`ops/bench/data/Oakland/20261016-perimeter-usb-history-audit/`:

- `local-session-excerpts.json`: the two contemporary messages, original
  timestamps/line numbers, archive filename and SHA-256.
- `historical-service-records.json`: all 14 selected registry rows, source
  commit/blob SHA-256, callsigns and converted verification times.
- `firmware-provenance.json`: both source mappings, fix ancestry checks,
  policy/lifecycle source hashes, and the verified retained fleet manifest.

## Recorded service roster

All times below are August 30 PDT. `b0` means `fx-260829-b0ff5db-b`,
an exact service-artifact USB flash. `af` means an existing image reported
as `fx-260829-af1d4ec-p`; no flash of that image is established by this row.

| Completed | Fixture | Short MAC | Resulting image | Recorded result |
| --- | --- | --- | --- | --- |
| 00:14 | Clank | F2BF60 | b0 | Replaced 0.816 V cell; commissioning and sensors passed |
| 00:35 | Gengar | F2BDD4 | b0 | ROM recovery; guarded PROTECT release; sensors passed |
| 01:49 | Spyro | F2BCF0 | b0 | Release after charging above 3.250 V; passed at 3.266 V |
| 02:26 | Unassigned | F402A8 | b0 | Low-voltage and later load-armed-reset entries both released; 100 mA charge limit corrected to 2 A; sensorless operation explicitly accepted |
| 02:51 | Batman | F2BDC4 | af | Continuous 60-second release and sensors passed |
| 02:56 | Coco | F2BFE0 | af | Replaced cell that remained near 3.210 V; 3.366 V replacement passed guarded release |
| 03:06 | Rocket (named later) | 9F268C | b0 | Old commission profile corrected to field; retained PROTECT released; sensors passed |
| 03:16 | Ditto | F3FCAC | b0 | Uninterrupted recovery released; sensors passed |
| 03:36 | Magmar | F2BDFC | af | Charged from reported approximately 2 percent SOC; released above floor; sensors passed |
| 03:46 | Torkoal | F3FD88 | b0 | Recovery before update and second 60-second release after update; sensors passed |
| 03:59 | Cammy | F2B900 | b0 | Recovery before update and second 60-second release after update; sensors passed |
| 04:11 | Sneasel | F403DC | b0 | Charged cell already above floor; two release proofs passed; sensors passed |
| 04:20 | Joltik | F4043C | af | Continuous 60-second release and sensors passed |
| 04:30 | Gambit | F2BCF4 | b0 | Installed cell reported approximately 87 percent SOC; two release proofs passed; sensors passed |

The service notes explicitly say ordinary flashes preserved NVS. The records
describe charging-qualified releases, not an unconditional erase of PROTECT.
The reported SOC values are preserved observations, not calibrated capacity
measurements. The notes do not establish that every serviced battery was new.

## Firmware membership

| Image | Exact source | Full/taper release fix | Day-sleep interruption fix |
| --- | --- | --- | --- |
| fx-260829-b0ff5db-b | 280598802a306c7d3c2b901b480f2dac864f4ca5 | Included | Included |
| fx-260829-af1d4ec-p | 0d8a2443325e710ec7671e7751074908aec45aea | Included | Included |

Fix commits are `5865282960c401f10e596df5876ec0475ef68f2e` and
`3e44ad86892b4a18a7e1dff3285ca9c40194f5aa`. Both are ancestors of both image
sources. Direct source inspection also confirms the full-battery proof and
the lifecycle `powerOwnsSleep` gate; the two images have identical
`power_policy.cpp` and `power_policy.h` contents.

The `b0` source mapping and build identity are in the Toad build entry added
by commit `5ef7c5980416bbee16b042822e1a1a9200c453be`; its binary SHA-256 is
`276d6558116a40da15f32eccf6bc7a940ef6d827ba965ddb81a8a8f5b0e27ae0`.
It has one maintenance WiFi profile, which is why service notes call for a
later dual-site production update. Variant `b` did not omit the recovery fixes.

The retained `af` manifest and binary in the daytime worktree agree at SHA-256
`669e765e9a9b2b5c618d495f49b7ccb876711baa16056973dc8495665c0398d8`.

## Consequence for the postmortem

For this documented service cohort, absence of those two recovery fixes is
not supported as the explanation for a later outage. The recorded releases
also mean later PROTECT episodes require new entries after those releases.
This narrows the investigation without proving that the later entries were
false: actual depletion, transient collapses, and reset/load classification
remain distinct possibilities.

Both images still require either >=3.25 V with >=+20 mA net awake battery
current, or the qualified >=3.45 V full-battery path, continuously for 60
awake seconds with good external input and an enabled fault-free charger.
Charging during sleep still does not count toward that proof. The remaining
weak-input/awake-current recovery trap therefore remains possible after this
service, even though the two earlier bugs were fixed.

Skitty, Marill and Daisy are not among these 14 service-result records.
Do not infer their service history, or an all-perimeter result, from this
subset. Match subsequent entry provenance and revision-specific telemetry
to each fixture's own service boundary.

## Follow-up audit: later state and timing caveat, October 16

An exact-ID join to the September 12 container census finds 11 of these 14
serviced perimeters. Ten were in PROTECT with reported battery voltage above
3.25 V; Ditto was FULL at 3.305 V. Gambit, Gengar and Magmar were not observed
in that census. Absence is not evidence that those three had failed.

| September 12 fixture | Battery V | Retained entry mV | Retained origin |
| --- | ---: | ---: | --- |
| Rocket 9F268C | 3.317 | 3022 | low_vbat |
| Cammy F2B900 | 3.280 | 3027 | low_vbat |
| Spyro F2BCF0 | 3.258 | 3027 | low_vbat |
| Batman F2BDC4 | 3.284 | 3016 | low_vbat |
| Clank F2BF60 | 3.267 | 3023 | low_vbat |
| Coco F2BFE0 | 3.284 | 3026 | low_vbat |
| Torkoal F3FD88 | 3.280 | 3027 | low_vbat |
| Unassigned F402A8 | 3.278 | 3261 | load_armed_reset |
| Sneasel F403DC | 3.267 | 3031 | low_vbat |
| Joltik F4043C | 3.264 | 3019 | low_vbat |

The retained evidence is
`ops/bench/data/Oakland/20261016-perimeter-usb-history-audit/service-to-september12.json`.
It preserves source hashes, exact IDs, field ages, later firmware revisions
and the September provenance fields. Battery samples were roughly 10-896 s
old at census end; these are not simultaneous meter measurements. All 11
observed members reported `supply_good=false`. The snapshot therefore does
not show a failed release while qualifying solar or USB power was present.

This supports renewed PROTECT entries after the recorded service recoveries,
not uninterrupted pre-service latches. Nine retained low-voltage origins
cluster around 3.02 V. That is evidence of low measured voltage at entry,
not proof that all nine cells were empty, that all trips were false, or that
the subsequent higher voltage resulted from solar charging rather than
unloading. A useful working hypothesis is low-voltage entry followed by a
too-demanding recovery gate; neither the date of each entry nor continuous
PROTECT residence through the sunny period is established here.

F402A8 is the distinct reset case. Its service notes already record both
low-voltage and load-armed-reset entries followed by release. September's
retained reset origin at 3.261 V deserves separate examination. In the b0
source, the durable load marker and unexpected-reset classification can
produce PROTECT from prior DIM/OFF state. The marker is not an instantaneous
physical measurement of a collapsing load. Normal software and deep-sleep
resets are excluded from that unexpected-reset set. Do not generalize this
one origin to the nine recorded low-voltage cases.

The service notes also do not establish a uniform fresh-battery energy
baseline. Clank and Coco explicitly record replacement cells; other entries
describe charging an installed cell or validating its recovery. This does
not disprove unlogged swaps, but the records cannot calibrate Ben's recalled
40-60 percent charge state across the whole cohort.

### Archive timing and operator clarification

The same local archive that places a USB helper at 00:55 PDT on August 30
contains an explicit 01:11:25 PDT user message: "yes, the helper is not
servicing lights anymore". It answers a request to pause USB service before
seven selected OTA updates. The excerpt, source line numbers and archive
hash are preserved in `helper-stop-excerpts.json` in the audit directory.
Most service-result timestamps are later, extending to 04:30 PDT. Initially
this made the helper's finishing time uncertain. Ben subsequently clarified
on October 16: "Shervin definitely worked until 4:30am." Accept that direct
operator history; the 01:11 message is a coordination snapshot, not a reliable
end-of-night timestamp. An exact pause/resumption timeline is not reconstructed,
and the retained roster still contains only 14 results. The artifact and
release findings are unchanged.

A separate local session, "Debug LED color commands", describes an August 30
warm-white problem after downlights were lowered: local ToF presence was
overriding commanded colors. Its initial transmitter-stall explanation was
revised after Ben reported that blink worked. Those downlight observations
are not evidence of dead perimeters. The subsequent one-second fleet sleep
commands requested normal timer wakes, not unexpected power-on resets.
