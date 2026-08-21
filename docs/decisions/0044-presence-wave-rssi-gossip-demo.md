# ADR 0044: Presence-wave RSSI gossip demo

- Status: Accepted for supervised field demonstration
- Date: 2026-08-16
- Decider: Ben Eckart
- Extends: ADR 0004, ADR 0027, ADR 0039, ADR 0040

## Context

The Nevada City hanging grid needs a quickly legible proof of presence sensing
and peer-to-peer propagation before spatial commissioning is complete. Exact
fixture coordinates and curated adjacency are not yet available. RSSI is known
to be a noisy proxy for physical distance, but the tight temporary grid makes
it useful for a deliberately non-production demonstration.

The hanging rig can appear inside some downward TMF8820 fields of view. A fixed
distance threshold would therefore fire continuously on some fixtures, while a
single closest-return summary could let an occluded zone hide changes elsewhere
in the 3x3 scene.

## Decision

Use the existing append-only `NB_EVENT` packet (type 23) for a bounded presence
wave:

- only sensor-verified canopy/downlight fixtures may originate a wave;
- each canopy learns the closest per-channel 3x3 TMF8820 background over 90
  reports (roughly the first 25-30 seconds after sensor start);
- presence requires the same channel to be at least 300 mm closer than its
  learned background on three consecutive confident reports;
- a stable close rig return is therefore background, not presence, and one
  occluded channel does not hide changes in another channel;
- the trigger latches until four clear reports and has a two-second origin
  cooldown;
- adjacent canopies that see the same person wait a randomized 120-620 ms;
  hearing the first origin cancels the other pending origins, yielding one hue;
- a new origin selects a substantially different random hue;
- every newly activated fixture forwards the event to its two strongest fresh
  RSSI neighbors that advertise wave capability and have not already appeared
  in that event's broadcast visited ledger;
- each packet is broadcast for ESP-NOW scalability but names one intended
  target, allowing all listeners to learn the visited set without replies;
- the color remains as the fixture's commission-listener posture until the next
  wave, while dashboard tags, direct/program leases, and local power vetoes keep
  their existing higher authority; and
- 37-pixel perimeter fixtures clamp the wave to linear value 14; point-source
  fixtures use value 96 before the normal local power cap.

This is a supervised proof of concept, not an assertion that strongest RSSI is
stable installation topology. Pinned or coordinate-derived adjacency remains
the production direction if spatial wave behavior matters.

## Consequences

- A person can repaint the updated reachable fleet by moving under a canopy
  sensor without involving the bridge after the initial firmware deployment.
- Old firmware is excluded from forwarding, so mixed-fleet waves can stop at
  gaps until holdbacks are updated.
- Broadcast event traffic is bounded to roughly one root announcement plus at
  most two forwarding attempts per activated fixture. There is no acknowledgement
  storm or full-heartbeat burst.
- Closest-RSSI ties, packet loss, the 24-entry neighbor cache, and simultaneous
  fanout can leave fixtures unvisited. That is acceptable evidence for this
  demo and must not be hidden as a production guarantee.
- A fully occluded sensor cannot detect a person; the per-zone differential
  prevents false firing but cannot recover missing optical information.
