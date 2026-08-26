# ADR 0056: Repeated Color Virus gestures introduce newer strains

**Date:** 2026-08-25

**Status:** Accepted

## Context

ADR 0055 made Color Virus persistent: after the first palm seed, the source and
eventual connected graph stayed infected until Stop, restart, or lease expiry.
That made a second visitor gesture appear unresponsive even when the perimeter
ToF gate had correctly cleared and re-armed. The desired interaction is for each
new gesture to release a visibly new color strain through the already-infected
graph.

Simply accepting any different neighbor hue is unsafe. Two adjacent colors can
continually overwrite one another and generate unbounded color and knock edges.
The existing `NbChoreoState` already carries a 16-bit `generation` field, so a
new packet field is unnecessary.

## Decision

1. In Color Virus, `generation` is a 16-bit serial strain sequence. A local
   seed selects one greater than the newest infected neighbor sequence it can
   currently see.
2. A re-armed local ToF rising edge creates a new strain even when the fixture
   is already infected. Random-color mode guarantees that the new transmitted
   hue bucket differs visibly from the source's current hue. A fixed palette
   selection remains fixed but still advances the strain sequence.
3. An infected fixture adopts only a serial-newer neighbor strain. If two
   independently seeded strains have the same sequence, the larger transmitted
   hue bucket is the deterministic tie-break. The losing strain cannot overwrite
   the winner on the next tick, so the graph converges instead of ping-ponging.
4. Adopting a new strain is an infection edge for output purposes. It requests
   an immediate state send and, in native knock modes, one locally gated 40 ms
   pulse on a downlight. Perimeter fixtures remain silent sensor/relay nodes.
5. Epidemic retains its infected -> immune -> susceptible timing and does not
   use cross-infected strain replacement.

No packet size, field order, or protocol version changes.

## Consequences

- A visitor can clear and re-hover over a reachable perimeter fixture to launch
  another color wave without an operator Stop/Start cycle.
- Multiple simultaneous visitor seeds converge deterministically.
- A partition with a newer unseen sequence can initially defeat a lower remote
  strain after reconnection. Another gesture after the source observes that
  sequence advances beyond it.
- Sequence comparison is modulo 16 bits; wrap remains ordered for fewer than
  32,768 unresolved strain increments, far beyond a single lease.
- In fixed-color mode the propagation is a new strain event but intentionally
  remains the selected color. Use random mode for the visible recolor easter egg.
