# Fleet OTA 120/3 Rollout Post-mortem -- 2026-08-26

## Outcome

The immutable production fixture artifact `fx-260826-51d1fe1-p` was rolled
out to 85 of the 98 fixtures that were running the prior production artifact
`fx-260826-024e508-p`.

- Source commit: `64264b2c60cbff9a9423642cb6cfee93e4272636`
- Binary size: 1,206,784 bytes
- Binary SHA-256:
  `57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`
- New field cadence: 120 s DAY_CHARGE sleep / 3000 ms timer-wake listen
- Bridge: T-Deck `8EB508`, channel 11, COM152 through the host dashboard
- Final production count: 85 new revision, 13 prior revision
- Special, prototype, recovery, dev-local, and Magic Wand revisions were not
  selected by this rollout.

All 85 promoted fixtures produced a fresh exact-revision mesh heartbeat and
survived the 25 s pending-image verification gate. The 13 remaining production
fixtures were not discovered in maintenance and therefore were not flashed.
They were left on their known-good prior image.

The final deferred IDs were:

```
9F266C  Meowth
9F26B0  Rikku
9F2714  Chunli
F2B7DC  Ponyta
F2BCF4  Gambit
F2BDD4  Gengar
F2BF7C  Rotom
F3FD28  Skitty
F40174  Pacman
F401DC  Zubat
F402A8  (no callsign in dashboard)
F40314  Dixie
F4042C  Cynder
```

Their last recorded VBAT values were 3.101-3.539 V. Most full heartbeats
reported power tier PROTECT. Short heartbeats can leave the dashboard's tier
field blank, so a blank late snapshot is not evidence that a durable PROTECT
latch cleared.

## Artifact and result ledger

The rollout used the same immutable binary for every attempt. Host result
files are:

| Result file | Attempt | Result |
| --- | --- | --- |
| `20260827-004059-fleet-ota-results.jsonl` | Three-fixture stage 1 | 3/3 upload ACK and verified |
| `20260827-010528-fleet-ota-results.jsonl` | First widening batch | 39/40 HTTP ACK; all 40 later proved exact new revision |
| `20260827-011715-fleet-ota-results.jsonl` | Smaller batch | 6/6 ACK; 5 immediately verified |
| `20260827-012126-fleet-ota-results.jsonl` | Isolated `9E5B5C` reconciliation | 1/1 ACK and verified; duplicate upload |
| `20260827-012856-fleet-ota-results.jsonl` | Smaller batch | 3/4 ACK; `9E5AE4` stayed on the old image |
| `20260827-013732-fleet-ota-results.jsonl` | Gathered batch | 5/5 ACK and verified |
| `20260827-014138-fleet-ota-results.jsonl` | Paced batch | 6/6 ACK and verified after `/resume` cleanup |
| `20260827-015127-fleet-ota-results.jsonl` | Paced batch | 15/15 ACK and verified |
| `20260827-020200-fleet-ota-results.jsonl` | Deterministic stragglers | 6/6 ACK and verified |
| `20260827-021127-fleet-ota-results.jsonl` | PROTECT cohort intersection | 1/1 ACK and verified |

The result files record HTTP upload outcomes. Final promotion counts came from
fresh mesh evidence, not by adding HTTP ACKs. `9E5954` is the important
counterexample: its upload request timed out, but it rebooted into the exact
new image and passed the pending-verify gate. Conversely, an upload ACK alone
was never treated as final success.

## Stage-1 evidence

The initial targets were `9F2638` (Lucas), `9F26BC` (Pinsir), and `F4019C`
(Treecko). All three passed power preflight, exact-target maintenance discovery,
upload, exact-revision rejoin, and pending verification.

After the 10-minute software-boot grace, all three stopped heartbeating at
about 600 s uptime. They returned after the expected roughly 120 s timer sleep
with `reset_reason=deepsleep`, low uptime, and the exact new revision. This
proved the new day cadence end to end before widening.

## What went well

1. Artifact identity stayed exact. Every upload named the immutable binary,
   expected revision, source commit, size, and SHA-256.
2. Explicit target lists prevented prototypes, the Magic Wand, Akuma, and
   one-off recovery builds from accidentally taking the fleet image.
3. Partial discovery failed safely. Missing fixtures were reported as deferred
   and were never invented from IP scans.
4. A/B OTA and the 25 s validation rule kept reboot outcomes recoverable.
5. Maintenance HTTP telemetry provided an independent truth source when mesh
   reporting was delayed or absent.
6. The fixture `/resume` endpoint cleanly recovered new-image fixtures that
   were accidentally pulled back into maintenance after reboot.
7. Local power gates remained authoritative. No low-power fixture was forced
   to light or strike as part of OTA.

## What made the rollout laborious

### 1. The T-Deck has one maintenance campaign slot

`meshEnterMaintenance()` stores one `NbTargetCmd`. Every new `U<ID>` replaces
the prior campaign. `fleet_dashboard_ota.py` sends many targets rapidly and
then resends the missing list in the same order. This means the final target
gets the sustained 35 s campaign while earlier targets may get only an
opportunistic packet between serial/UI ticks.

The host tool behaved as though every named target had an independent campaign,
but the bridge implemented only one. The first 80-target widening pass found
40/80 after 420 s. Smaller rapid batches still commonly found only 40-60
percent of their requested fixtures.

### 2. There are two materially different sleep cadences

The prior ordinary field cadence was 300 s sleep / 15 s listen. PROTECT uses
900 s sleep and only an 8 s wake grace. A discovery deadline longer than 300 s
does not solve target scheduling, and a five- or six-minute campaign cannot
cover one complete PROTECT cadence at all.

For `N` targets with one bridge campaign slot, deterministic pacing must obey:

```
N * target_dwell < minimum wake window
campaign duration >= maximum sleep interval + wake margin
```

The seven-target straggler pass used 2 s per target, producing a roughly 14 s
cycle that fit inside the old 15 s ordinary listen window. It found 6/7 across
one ordinary cadence. The same shape was not sufficient for the 8 s PROTECT
window, and its five-minute duration was far below the 900 s PROTECT sleep.

### 3. Fresh does not mean gathered

A dashboard age under 10 s proves only that a heartbeat was recently heard.
The fixture may be at the end of its short wake grace and about to sleep. It
does not prove that a control command was accepted or that the 10-minute
operator hold is active.

The repeated `B600` dark lease raised the observed live count from about 25 to
74 and was useful, but the host had no per-fixture proof of which peers accepted
the lease. A fresh heartbeat and a confirmed awake hold must be distinct states
in future tooling.

### 4. Mesh revision display can lag the maintenance endpoint

After upload, the dashboard sometimes showed an old revision for tens of
seconds before a newer full heartbeat arrived. The apparent rollback count in
the first widening batch fell from six to three to two to one and then zero.
Maintenance HTTP telemetry already reported the exact new image during part of
that disagreement.

The verifier was correct to wait for fresh exact mesh evidence, but operator UI
must show revision evidence age and source. A current short heartbeat must not
make an older cached full-heartbeat revision look current.

### 5. Gather traffic leaked into verification once

A host pacing helper continued issuing `U<ID>` while a six-fixture upload was
rebooting. Three fixtures successfully booted the new image and were then
re-caught by the still-running helper, returning to maintenance and disappearing
from mesh verification. Their HTTP endpoints proved the new revision; explicit
`GET /resume` calls restored comms and all three passed verification.

This was recoverable, but it is exactly the race a production OTA state machine
must make impossible.

### 6. HTTP ACK and actual result are separate evidence

`9E5954` timed out at the uploader but was already running the new image. A
blind retry would have added risk without adding information. The correct
response was to reconcile maintenance telemetry, reset/uptime, exact revision,
and pending verification first.

## Recommended OTA state machine

Future fleet OTA should be one explicit job with these non-overlapping phases:

```
PLAN -> PREFLIGHT -> GATHER -> DISCOVER -> FREEZE -> UPLOAD -> VERIFY -> CLEANUP
```

### PLAN

- Name an immutable artifact revision and SHA-256.
- Resolve an explicit target roster from the registry.
- Exclude protected one-off roles unless separately acknowledged.
- Record the operator, bridge ID, channel, job ID, and start timestamp.

### PREFLIGHT

- Snapshot each target's battery, input, power tier, revision, reset reason,
  uptime, and evidence age.
- Classify expected cadence: ordinary, PROTECT, commission, or unknown.
- Refuse a campaign whose deadline does not span the slowest selected cadence.
- Split low/protected targets into a strong-sun pass rather than mixing them
  into an ordinary batch.

### GATHER

- Submit the complete target roster to the bridge once.
- The bridge, not the laptop, owns target scheduling and repeats exact-target
  maintenance packets for every unresolved ID.
- Retain multiple active targets in a bounded table instead of one slot.
- Expose per-target campaign state and total remaining count.
- Derive dwell/cycle from the shortest target wake window.

### DISCOVER

- Accept only maintenance endpoints whose self-reported fixture ID matches the
  requested ID.
- Record IP, maintenance revision, battery, and discovery timestamp.
- Keep undiscovered targets explicitly deferred.

### FREEZE

- Stop the bridge gather campaign and receive an explicit stopped ACK.
- Wait one command tail before any upload.
- From this point until verification completes, no `U` traffic may be emitted.

### UPLOAD

- Upload the one immutable binary to the frozen endpoint set in parallel.
- Record HTTP result independently from final promotion state.
- On timeout, query endpoint and mesh evidence before retrying.

### VERIFY

- Require a heartbeat newer than the job start.
- Require exact expected revision and a reset transition/job epoch.
- Require survival beyond the 20 s fixture pending-image window (host gate 25 s).
- If a target is found in maintenance on the new image, call `/resume` and
  continue verification instead of uploading again.

### CLEANUP

- Resume any leftover maintenance endpoints.
- Release temporary leases.
- Emit a final roster with `verified`, `deferred`, and `failed` states and
  evidence for each.
- Never summarize a momentary live count as the number of healthy fixtures.

## Concrete implementation work

1. Replace the T-Deck's single `gMaintCampaign` with a bounded multi-target
   campaign table or queue. A host command should add a roster without erasing
   earlier targets.
2. Add a campaign ID and explicit start/stop/status protocol to the T-Deck
   serial bridge. The host must be able to prove FREEZE before upload.
3. Teach `fleet_dashboard_ota.py` to select a gather duration and scheduler
   from ordinary versus PROTECT cadence, and to refuse an insufficient window.
4. Add endpoint reconciliation: after upload timeout or missing mesh rejoin,
   probe the known IP for exact revision and mode before retrying.
5. Add automatic `/resume` cleanup for exact-new fixtures left in maintenance.
6. Preserve separate evidence ages for short heartbeat, full heartbeat fields,
   and firmware revision in the dashboard and verifier.
7. Telemetry should expose compiled `day_sleep_s`, `wake_listen_ms`,
   `protect_sleep_s`, protect wake grace, and age of the last accepted operator
   control. This distinguishes recently heard from positively gathered.
8. Write one append-only job ledger containing every target transition. The
   scattered JSONL upload files remain useful raw evidence, but should not be
   the only way to reconstruct one logical rollout.
9. Make the T-Deck UI report `verified / deferred / protected` instead of only
   a volatile `live / seen` count during maintenance operations.

Source follow-up on 2026-08-26 implemented items 1-8 under ADR 0062, including
the bounded bridge roster, job-scoped status/freeze contract, cadence-derived
host state machine, endpoint reconciliation, `/resume` cleanup, separate
firmware evidence age, and unified job ledger. Embedded compilation passes;
exact T-Deck and fleet hardware validation remain open. The richer on-device
T-Deck OTA result UI in item 9 remains deferred; the laptop ledger is currently
the authoritative verified/deferred/failed surface.

## Follow-up for the 13 deferred fixtures

Do not run the remaining pass late in the day. Use a strong-sun window and
first confirm whether each durable PROTECT latch releases under the compound
voltage/current rule. If any stay protected, run a dedicated roster campaign
for at least one complete 900 s sleep interval plus margin, with a scheduling
cycle shorter than 8 s. Then freeze all maintenance traffic, upload the exact
same immutable artifact, verify each through the pending-image gate, and record
the final 98/98 production result.
