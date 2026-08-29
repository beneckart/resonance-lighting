# 0073 -- Solarnoid reservoir voltage is independent of charger supply-good

**Date:** 2026-08-29

**Status:** Accepted in source; exact-target hardware canary pending. No fleet
promotion is authorized.

**Owner:** Ben

**Supersedes:** ADR 0072 only where it requires the BQ25628E `supply_good` bit
on the charged-VDC-reservoir path.

## Context

Hawkeye `9F2664` accepted the first one-target ADR 0072 canary well before its
hour. The exact image, pending-verify survival, field/downlight identity,
FULL tier, recovery state 0, and roughly 6.57-6.75 V VDC all passed. During the
following four-minute pre-window observation, lifecycle nevertheless remained
in `DAY_CHARGE`.

The reason was visible without waiting for an actuation. The BQ `supply_good`
bit briefly reported false while measured VDC remained charged, then returned
true. Each false sample reset the 60-second qualification timer. This is
consistent with the fleet snapshot: charge-done downlights can retain high VDC
while the charger changes its input acceptance state.

`supply_good` qualifies the charger's use of VDC. It does not qualify the
upstream capacitor voltage that directly powers the solarnoid. Conjoining the
two recreated the same category error ADR 0072 intended to remove.

## Decision

1. Live-harvest qualification remains `supply_good` plus at least 150 mA for
   entry/strike or 100 mA to remain active.
2. Charged-reservoir qualification is the independent measured VDC input:
   at least 5.8 V for entry/strike or 5.4 V to remain active. It does not also
   require `supply_good`.
3. Battery voltage cannot produce these VDC readings: the LFP is roughly
   2.5-3.6 V, while the accepted reservoir floor is 5.8 V. A high battery with
   absent/low VDC therefore remains ineligible.
4. FULL power tier, scheduled day, valid time, authority-free operation,
   exact target/hour, one-minute confirmation, hard ritual window, and all
   mechanism gates remain unchanged.
5. The first artifact `fx-260829-2876d89-t` is retained as pre-window evidence
   but is superseded before its eligible hour. It has not actuated and is not a
   rollback target after the canary campaign; Hawkeye's exact production prior
   remains `fx-260829-af1d4ec-p`.

## Consequences

- Charger taper, termination, or input-state flicker cannot erase a visibly
  charged solarnoid reservoir.
- Live current with low VDC still qualifies through the stricter BQ-valid path.
- High open-circuit VDC must persist for one minute and is rechecked at every
  act; if the reservoir is spent and does not recharge, later events abstain.
- The correction remains one-target only until telemetry and sound prove it.

## Validation required

Follow ADR 0072's exact Hawkeye procedure with a newly generated artifact and
the same future hour. The audit must distinguish an energy/schedule attempt
from a mechanism result, and Hawkeye must be restored to the retained exact
production binary after the fixed window.

## References

- ADR 0072
- `firmware/fixture/src/core/lifecycle.*`
- OTA job `F6088220`
