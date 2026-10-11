# 0078 -- Ben-only Atom Conductor with renewable Wake sessions

**Date:** 2026-09-02

**Status:** Accepted in source; native policy/graph tests and the Atom Matrix
embedded build pass. Hardware and installed-fleet validation remain open.

**Owner:** Ben

**Extends:** ADR 0037, ADR 0045, ADR 0065, ADR 0075, ADR 0076, and ADR 0077.

## Context

The small Atom Matrix plus Atomic Battery Base is useful as more than the
existing one-target campmate clicker. Ben wants one handheld to gather sleeping
fixtures, then originate a fleet-wide canopy chime or RGBW wave whose ordering
looks like the nearest-neighbor Color Wipe/Color Virus behavior. The immediate
constraint is no fixture-fleet flash: 70 of 114 installed fixtures have the
static inspection image and 44 remain on prior field images.

The deployed packet contract already has the required primitives:

- `NB_FORCE_LIFECYCLE` Wake/Auto;
- `NB_TIME_QUALITY` from the sparse UTC-anchor/bridge path;
- bounded `NB_LOCATE_CONTROL` and fragmented `NB_NEIGHBOR_REPORT` observations;
- exact-target `NB_TARGET_SOLENOID`;
- up to 18 exact fixture RGBW entries in each `NB_DIRECT_FRAME`.

The control and safety timers need unambiguous ownership. "Continued use" of a
one-button Atom necessarily means effect presses, but an effect-packet loop must
not silently become an unlimited Wake publisher. The current fleet is also
unauthenticated. Ben is the only authorized holder for this initial controller;
that temporary operational restriction is not a substitute for command
authentication before distribution.

## Decision

Add `firmware/atom_conductor/` as a separate firmware target. Preserve
`firmware/atom_clicker/` as the narrow, one-target proof.

### Session policy

1. A face-button wake starts a six-minute gather. Wake is transmitted every two
   seconds, covering one complete 300-second field sleep cadence with margin.
   A bounded locate request is refreshed every 30 seconds during the same
   window, with fixture reports at the existing 20-second period.
2. Every button action -- effect play or nighttime-color advance -- updates the
   Atom's local `lastInput` timestamp. Effect-send functions never send Wake.
3. The independent session scheduler sends an explicit reliable Wake renewal
   five minutes after the prior Wake only while local input is less than 15
   minutes old. There is no extra keepalive gesture.
4. Fifteen minutes without input starts cleanup. A one-hour absolute deadline
   does the same even under continuous use. Cleanup stops locate, transmits Auto
   repeatedly for ten seconds, and then deep-sleeps with GPIO39 face-button
   wake enabled.
5. Fixture expiry remains authoritative if the Atom resets or loses power. Auto
   is restorative belt-and-suspenders, not the only bound. Older images may
   extend their ordinary ten-minute receive hold when a strike/direct command is
   accepted, but that remains bounded after the final effect and never changes
   the Atom deadline.
6. The Atom does not treat the inspection light's one-hour pre-dusk start as
   nighttime. It consumes the fleet's canonical trusted UTC consensus and the
   shared Black Rock City solar calculation. Before true civil dusk the button
   selects chime; after civil dusk it selects RGBW. Missing/stale/conflicting
   UTC refuses both rather than guessing. Fixture lifecycle heartbeats are not
   a time fallback because Wake changes older lifecycle state and the current
   inspection lifecycle deliberately enters its light window early.

### Wave policy

1. The Atom stores at most 160 peers and each reporter's eight strongest
   directed observations. Either report direction is treated as a proximity
   edge. The fresh classified canopy heard most strongly by the Atom is the
   first origin; breadth-first distance assigns later layers.
2. Incomplete/disconnected reports degrade to six-dB Atom-RSSI bands for the
   next component. All fresh class-confirmed downlights remain eligible rather
   than disappearing with an incomplete graph.
3. Before civil dusk, a short press sends an exact-target type-17 chime wave in
   graph layers. After civil dusk, it sends the next RGBW wave, automatically
   advancing red -> green -> blue -> white. RGBW uses hard-cut, ten-second
   direct-frame micro-leases chunked at the existing 18-entry packet limit.
4. Power tier is deliberately not an Atom target filter. An awake, fresh
   PROTECT/low-voltage canopy receives the same deliberate best-effort strike as
   any other under ADR 0065. PROTECT still owns sleep and cannot be held awake by
   the Wake session. All local mechanism and LED power caps remain authoritative.
5. This is central replay, not new fixture-to-fixture forwarding. It is
   aesthetically analogous to Color Wipe/Color Virus while remaining compatible
   with the installed packet contract.

### Reduced surface

The conductor has no WiFi association, OTA, maintenance, NVS mutation,
fleet-sleep, lifecycle-night, or serial command parser. It can emit only the
fixed Wake/Auto/locate controls and time-gated canopy chime/RGBW effects. A
medium hold advances the next nighttime color but cannot select lights during
day or chimes during civil night. It does not claim actuator acknowledgement;
all commands are reported as attempts.

## Consequences

- No fixture flash is required. Updated reporters improve graph quality; a
  sparse report set still produces a bounded radial fallback.
- One button can mean both "I am using this" and "play this effect" without
  conflating effects with lease packets. Activity authorizes the scheduler's
  next explicit renewal; it is not itself that renewal.
- The six-minute initial wait is real. Playing a partial-fleet effect during
  gather is refused so the handheld does not present a misleading fleet-wide
  result.
- There is an intentional overlap hour: the inspection fixtures may already be
  visibly lit, but the Atom remains a chime controller until civil dusk.
- The Atom is unavailable for effects without fresh trusted UTC. This is safer
  than allowing the wrong physical action and makes time-anchor coverage an
  explicit field prerequisite.
- RGBW colors return when the ten-second direct micro-lease expires. Persisting
  a color would require deliberate re-sends and is not part of this first field
  controller.
- The current 200 mAh base remains a runtime risk. Deep sleep is implemented,
  but its real current and button-wake behavior must be measured.
- This unit must stay with Ben. Campmate distribution remains blocked on
  authenticated authorization, labels, revocation, and abuse/coexistence tests.

## Validation required

1. Flash the exact known Atom only after checking its current port, then prove
   GPIO39 press wake and measure deep-sleep/active current on the Atomic Battery
   Base.
2. With one explicitly named canopy, prove Wake, report ingestion, each RGBW
   color, one 25 ms strike attempt, manual Auto cleanup, inactivity cleanup, and
   the absolute deadline.
3. Supply trusted UTC immediately before and after civil dusk and prove the same
   short press changes from chime to RGBW; remove/stale the source and prove both
   actions fail closed.
4. Expand to a small physical cluster and compare calculated layers with actual
   adjacency before trying a fleet wave.
5. At installed scale, record fresh canopy count, directed edge count, RX queue
   drops, action duration, and visible/audible misses.
6. Include an awake PROTECT canopy and prove it may attempt a strike but still
   follows power-owned sleep rather than the Atom Wake lease.
7. Remove Atom power before cleanup and prove both current inspection and older
   field fixture behaviors recover without an Auto packet.

## References

- `firmware/atom_conductor/README.md`
- `firmware/atom_conductor/src/core/atom_session.cpp`
- `firmware/atom_conductor/src/core/fleet_graph.cpp`
- `firmware/fixture/src/core/packet.h`
- ADR 0045, ADR 0065, ADR 0075, ADR 0076, ADR 0077
