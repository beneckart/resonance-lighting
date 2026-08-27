# 0062 -- Job-scoped multi-target fleet OTA

**Date:** 2026-08-26

**Status:** Accepted; source/native and T-Deck synthetic-roster validated, fixture batch pending

**Owner:** Ben

**Extends:** ADR 0010, ADR 0037, ADR 0040

## Context

The first 120 s / 3 s cadence rollout showed that fixture OTA and A/B rollback
were reliable, but fleet orchestration was not. The T-Deck retained only one
sustained exact-target maintenance command. A host batch sent many `U<ID>`
commands as though every command remained active, so each new target replaced
the previous one. Discovery became probabilistic, repeated gather traffic could
leak into verification, and several raw uploader files had to be reconciled by
hand.

The production timer-wake window is 3 seconds. A complete final fleet is about
130 fixtures. Any bridge scheduler must revisit every target inside that window
and must positively prove that it stopped before upload begins.

## Decision

1. Fleet OTA is one job-scoped state machine:

   ```text
   PLAN -> PREFLIGHT -> GATHER -> DISCOVER -> FREEZE -> UPLOAD -> VERIFY -> CLEANUP
   ```

2. The T-Deck owns a fixed 160-target maintenance roster. It dispatches one
   exact-target packet every 10 ms in round-robin order. A 130-target roster has
   a 1.3-second cycle; the maximum 160-target roster has a 1.6-second cycle.
3. The bridge serial contract is job-scoped:
   - `uB<job8>:<seconds>` begins and clears a roster;
   - `uA<job8>:<ID>` adds an idempotent exact target;
   - `uF<job8>` freezes the matching job;
   - `uS<job8>` emits structured status.
4. Structured `nb-maint` status reports job ID, phase, active state, roster
   count, dispatch count, remaining time, and roster cycle time. The dashboard
   exposes the latest status plus its evidence age.
5. The host refuses to upload until it receives a fresh, matching frozen status
   with the complete roster count. There is no fixed command-tail sleep.
6. Gather duration is derived from the selected cadence. Ordinary production
   defaults to 120 seconds plus 30 seconds margin. PROTECT defaults to 900
   seconds plus 30 seconds margin. A shorter explicit deadline is refused.
7. HTTP upload outcome and final promotion state are independent evidence.
   Timeouts are reconciled against identity-matching endpoint and mesh state
   before another flash is considered.
8. Responsive maintenance endpoints receive `/resume` during cleanup. Exact new
   mesh revision, fresh revision evidence, reset identity when required, and
   survival through the 25-second host gate remain the promotion conditions.
9. Every job exclusive-creates one append-only JSONL ledger containing the
   artifact, roster, phase changes, per-target discovery/upload/verification,
   cleanup, and final verified/deferred/failed sets.
10. Legacy `U<ID>` remains a one-target 35-second command and cannot replace an
    active explicit campaign.

This changes only the T-Deck/host control contract. It does not add a second
mesh packet definition, change the fixture wire layout, weaken local power
gates, or replace shared-WiFi unicast OTA.

## Consequences

- One ordinary-cadence gather is bounded and deterministic instead of relying
  on repeated small batches.
- Gather traffic cannot re-catch a newly rebooted fixture after FREEZE.
- A complete result can be reconstructed from one ledger without adding upload
  ACK counts by hand.
- The bridge uses about 480 bytes for target IDs plus bounded scheduler state.
- At 100 maintenance packets per second, fleet validation must confirm mesh
  census and other bridge services remain healthy during a full roster gather.
- The expected healthy 130-fixture operation is about 10 minutes. Most of that
  is repeated unicast image transfer, not maintenance discovery.

## Validation required

1. Compile and flash one immutable T-Deck artifact to exact bridge `8EB508`.
2. Load a synthetic 130-ID roster and confirm `cycle=1300`, stable heap, normal
   census reception, and immediate transition to frozen status.
3. Confirm legacy `UF2B7DC` is still parsed as `U<ID>`, not the new `uF` command.
4. Run one small named fixture batch through the complete host state machine.
5. Interrupt discovery and upload separately; confirm final cleanup freezes the
   bridge and resumes every reachable endpoint.
6. Reconcile a deliberate HTTP timeout without automatically reflashing.
7. Run a full ordinary-cadence fleet pass and compare measured gather, upload,
   verification, and total time against the 10-minute target.

## Development hardware evidence -- 2026-08-26

Exact T-Deck `8EB508` on COM152 was flashed with the compiled `dev-local` image
(1,572,560 bytes, SHA-256
`96f5bad803c7af646d64c2a9d3e75c42ca1a2eaa50aaac99d79c7830985f3281`).
A 130-ID synthetic roster, checked absent from the dashboard and registry,
reported `targets=130` and `cycle=1300`. During the observation it dispatched
348 packets with 348 successful send callbacks, zero send failures, stable 86
observed peers, and continuing census reception. After `uF`, dispatch remained
exactly 348 for the following second. Legacy `UF00001` was correctly parsed as
`U<F00001>`, created a one-target legacy campaign, and was then frozen by its
reported job ID.

This validates scheduler timing, live RF coexistence, structured dashboard
status, immediate freeze, and legacy parser separation. It is not a promotable
T-Deck field artifact: the shared worktree contains unrelated in-progress
changes. A clean immutable build plus a small real-fixture interruption test and
full timed fleet pass remain required.

## References

- `firmware/tdeck_bridge/src/core/maintenance_campaign.h`
- `firmware/tdeck_bridge/src/net/mesh_tx.cpp`
- `ops/bench/fleet_dashboard_ota.py`
- `docs/howto/FLEET_OTA_10_MINUTE_RUNBOOK.md`
- `docs/tests/FLEET_OTA_120X3_ROLLOUT_POSTMORTEM_2026-08-26.md`
