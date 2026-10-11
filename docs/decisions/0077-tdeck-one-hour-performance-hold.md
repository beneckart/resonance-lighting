# 0077 -- T-Deck one-hour inspection performance hold

**Date:** 2026-08-31

**Status:** Accepted and USB-flashed on T-Deck `8EB508`.

**Owner:** Ben

**Extends:** ADR 0075 and ADR 0076. It does not change fixture authority or the
retained fixture artifact.

## Context

ADR 0076 gives one Wake Fleet press a maximum inspection-control session of
about sixteen minutes: six minutes of repeated Wake commands followed by the
fixture's ten-minute tail. That is useful for manual checks but too short for a
DJ set, and requiring an operator to re-arm every ten minutes is fragile.

The inspection fixture image already treats lifecycle mode 0 as a RAM-only
direct-control arm while preserving Auto/static white and all battery safety.
The T-Deck can therefore extend a deliberate performance without a new packet
or another fleet flash.

## Decision

1. The T-Deck Wake app has a separate confirmed **Performance Hold** action.
2. Performance Hold sends the existing lifecycle mode-0 command every two
   seconds for one hour. This is the already-proven sleep-catch cadence, not a
   new wire command or fixture behavior.
3. Each accepted copy refreshes the fixture's non-extendable ten-minute direct
   arm. The last campaign copy therefore leaves up to roughly ten minutes of
   safe tail after the T-Deck's one-hour countdown reaches zero.
4. Direct LED/audio frames still cannot extend the deadline. Auto or Night
   Show replaces the active campaign immediately and closes manual control on
   every fixture that hears it.
5. The Wake screen reports the active campaign type and remaining controller
   time. The original 16-minute Wake Fleet option remains available.
6. Fixture FULL/DIM/OFF/PROTECT, recovery gates, static-white fallback, and
   perimeter center-pixel restriction remain higher authority. The existing
   immutable fixture artifact `fx-260831-f121868-b` consumes this command
   without modification.

## Consequences

- A DJ gets at least one hour of deliberate audio control from one confirmed
  T-Deck action, plus the bounded fixture tail.
- The T-Deck must remain powered and running for the one-hour refresh campaign.
- At 0.5 small packets per second, the extra controller traffic is negligible
  beside direct audio frames and does not change fixture LED energy authority.
- Mixed older fixture images still interpret mode 0 using their older Day
  behavior. Performance Hold is intended for the ADR 0075 inspection image.

## References

- `firmware/tdeck_bridge/src/net/mesh_tx.cpp`
- `firmware/tdeck_bridge/src/ui/app_schedule.cpp`
- `docs/decisions/0075-bounded-inspection-direct-control-and-audio-gain.md`
- `docs/decisions/0076-wake-campaign-owns-inspection-control-deadline.md`
