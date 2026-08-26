# ADR 0052: Runtime-selectable commission default

**Date:** 2026-08-25

**Status:** Accepted; source-built, hardware validation pending

**Owners:** Ben + Codex

**Extends:** ADR 0039. The ready beacon remains the default for an unset value,
but it is no longer the only possible no-command commission posture.

## Context

The commission profile stays continuously reachable and currently returns to a
class-aware ready beacon when a bridge lease or direct stream ends. The field
profile instead runs the autonomous Greenberg-Hastings wildfire at scheduled
night. Bench and artistic testing needs the reachable commission control plane
without forcing every no-command interval back to the ready beacon.

Strict rails-off commission dark also remains necessary for electrical and
rail-cycle diagnostics. Rebuilding different artifacts to select among these
postures would violate the one-image fleet doctrine and make artifact identity
harder to reason about.

## Decision

1. Add a wire/NVS-stable `CommissionDefaultMode` with three meaningful local
   fallbacks:
   - `listener`: the existing class-aware ready beacon;
   - `ca`: the autonomous light-only GH wildfire with its built-in defaults;
   - `dark`: strict electrically dark commission fallback.
2. A missing or invalid stored value resolves to `listener`, preserving the
   accepted build-week behavior on upgraded fixtures.
3. The setting applies only while the fixture profile is `commission`. The
   field profile continues to use its UTC/solar lifecycle and autonomous night
   CA regardless of this value.
4. Active direct streams and program leases retain higher authority. Changing
   the default during a lease changes the fallback used on release/expiry; it
   does not interrupt the active lease.
5. Autonomous commission CA is light-only. The daytime knock output remains an
   explicit bounded lease and is never available as a persisted no-command
   default.
6. Allocate packet type 30 for `NB_COMMISSION_DEFAULT`; type 29 remains reserved
   for the queued bounded sensor-report packet. Receivers reject the all-zero
   target for type 30.
7. Bridge OS exposes an exact-target Default app. `ALL` is implemented as a
   deterministic walk over fresh short IDs, not a broadcast. The operator may
   apply a setting until reboot or explicitly persist it after confirmation.
   Repeated RF copies share one sender/sequence/uptime identity and cause at
   most one successful NVS write at the receiver.
8. Local power, boot, transport, OTA, LED-rail, and actuator safety remain above
   every selected default.

## Consequences

- Commissioned fixtures can run autonomous CA after the T-Deck is turned off,
  while retaining commission reachability and heartbeat cadence.
- A persisted selection survives fixture reboot; an until-reboot selection does
  not.
- Field deployment behavior cannot be accidentally redefined by changing this
  setting.
- A fixture on older firmware ignores the new packet. Hardware validation must
  therefore use named updated fixtures and verify the observed fallback rather
  than treating a T-Deck send status as acknowledgement.

## Validation required

1. On one named downlight, one perimeter, and one RGB uplight, select each mode
   until reboot and confirm lease override plus release/expiry fallback.
2. Persist `ca`, reboot, and confirm it survives; then restore and persist
   `listener`.
3. Select `dark` and verify an actual LED-rail cut, not merely a zero frame.
4. Switch the same canaries to field profile and prove the commission-default
   value does not change scheduled day/night behavior.
5. Exercise `ALL: targeted fresh` on a small named cohort and verify that no
   non-target or older fixture changes state.
