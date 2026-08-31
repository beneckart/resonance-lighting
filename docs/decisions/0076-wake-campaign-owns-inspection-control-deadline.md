# 0076 -- Wake campaign owns the inspection control deadline

**Date:** 2026-08-31

**Status:** Accepted documentation clarification; the retained ADR 0075
artifact already implements this behavior.

**Owner:** Ben

**Supersedes:** ADR 0075 wording that described one ten-minute window beginning
only at first capture. It does not change the control authority or artifact.

## Context

T-Deck Wake Fleet is a six-minute campaign, not one packet. It republishes the
same lifecycle command throughout the gather interval so radios that are off
for 120 seconds are caught. Each accepted Wake Fleet packet arms the inspection
control deadline at `now + 10 minutes`.

The first ADR 0075 description correctly made direct LED/audio frames unable to
extend the deadline, but it understated the effect of the continuing Wake Fleet
campaign itself. A fixture caught early receives later campaign copies, so its
deadline is refreshed during the six-minute gather.

## Decision

1. Wake Fleet owns the inspection control deadline. Its repeated command copies
   may refresh the deadline throughout the fixed six-minute gather campaign.
2. The final campaign copies leave approximately ten minutes of fleet-wide
   manual control. The maximum bounded session is therefore approximately
   sixteen minutes from the original button press, not ten minutes.
3. Direct LED Studio and CoreS3 Audio frames still do not extend the deadline.
   An abandoned artistic publisher cannot keep the radios awake indefinitely.
4. Auto or Night Show replaces the active T-Deck lifecycle campaign and closes
   the inspection control window. Re-running Wake Fleet later is the explicit
   way to start another bounded session.
5. No fixture, T-Deck, CoreS3, packet-layout, build-recipe, or artifact change
   is required. Immutable fixture artifact `fx-260831-f121868-b` already has
   this exact behavior.

## Consequences

- Operators can wait through the gather campaign and still receive roughly ten
  minutes of full-fleet LED/audio control.
- The energy bound is six minutes of campaign gathering plus ten minutes after
  the last refresh, rather than ten minutes total.
- Static inspection fallback, three-second direct staleness, night radio duty,
  and all battery/recovery authority remain unchanged.

## References

- `firmware/tdeck_bridge/src/net/mesh_tx.cpp`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `docs/decisions/0075-bounded-inspection-direct-control-and-audio-gain.md`
