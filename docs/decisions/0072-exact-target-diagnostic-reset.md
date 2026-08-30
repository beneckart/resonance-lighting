# 0072 -- Exact-target diagnostic reset from T-Deck serial

**Date:** 2026-08-30

**Status:** Accepted for named diagnostics

**Owner:** Ben

**Supersedes:** ADR 0048 only for the fixed, exact-target serial reset below

## Context

After the installed canopy fixtures were lowered, 34 downlights reported that
local presence interaction was replacing a received green DIRECT frame with
dedicated warm white. A clean boot at final height is the bounded diagnostic:
the 90-read sensor warmup should learn the new stationary ground background.

The wire protocol already has `NB_TARGET_SLEEP_FOR`, and fixtures already
handle it as a rails-off timed deep sleep followed by a clean boot. CoreS3
exposes targeted timed sleep, but ADR 0048 deliberately left every sleep and
reboot operation out of T-Deck serial and agent control. T-Deck therefore had
no safe way to reset one named installed canary without sleeping the fleet or
using an unrelated OTA/NVS mutation.

## Decision

1. Add serial quick command `R<6-hex-ID>` to T-Deck Bridge OS.
2. Require exactly six hexadecimal ID digits and reject `000000`. There is no
   broadcast spelling, class selection, roster expansion, or implicit target.
3. Always send `NB_TARGET_SLEEP_FOR` with value 1 second. The caller cannot
   choose a longer sleep duration.
4. Repeat the same addressed packet six times at 8 ms gaps, matching the
   established exact-target RF convention.
5. Durably record `reset-target`, the exact target, value, bridge sequence,
   uptime, and UTC quality before RF. Refuse the command if that T-Deck audit
   write fails.
   The fixture's existing operator-sleep path separately persists the received
   sleep provenance before entering deep sleep and refuses sleep if that write
   fails.
6. Keep reset absent from the Claude tool schema. Fleet sleep remains available
   only through ADR 0048's physically confirmed local UI.
7. Operation still follows ADR 0040: declare one bridge/operator, name targets,
   check fresh power/firmware state, use one canary first, and require fresh
   reset/rejoin evidence before widening.

## Consequences

- A named fixture can be clean-booted without OTA, unrelated fixture
  configuration changes, or a fleet-wide availability event. The operation
  does update the existing fixture operator-sleep provenance record in NVS.
- The reset cuts its LED rail and radio for about one second, then normal boot,
  sensor warmup, and fleet behavior resume.
- The serial operator remains capable of temporarily interrupting one fixture,
  so exact identity, power preflight, audit persistence, and post-action proof
  are mandatory.
- This diagnostic does not fix DIRECT/local-interaction precedence or safe
  post-install re-baselining. Those remain separate P0 firmware work.
