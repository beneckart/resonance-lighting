# Fleet OTA future speed options

These are deliberately deferred. ADR 0062 first makes the proven shared-WiFi
unicast path predictable. Revisit these only if a measured full-fleet pass still
justifies the additional recovery and protocol complexity.

## Multicast or mesh-distributed image

Send the approximately 1.2 MB image once, with chunk identity, integrity,
forward-error recovery, per-fixture missing-chunk repair, and final SHA-256
verification. This removes the current `image size x fixture count` AP traffic
and could make the roughly 120-second wake rendezvous the dominant cost.

The difficult parts are loss recovery, bounded memory, A/B rollback integration,
mixed-version compatibility, and proving that one damaged transfer cannot
strand a fixture. ADR 0010's shared-WiFi OTA remains the production choice.

## Pre-stage, then activate

Distribute and hash-verify the next image opportunistically before the show-time
deployment window. A later job-scoped activation changes boot partitions and
reboots the prepared cohort together. The visible deployment could approach one
reboot plus the 25-second validation gate.

This needs durable staged-image identity, expiry, power-loss behavior, an exact
activation roster, rollback semantics, and a safe rule for fixtures that did not
finish staging. It is the strongest long-term speed option because distribution
leaves the critical path rather than merely becoming faster.

## Decision trigger

Keep phase timings in the ADR 0062 job ledger. Consider a radical transport only
after several clean 130-fixture passes show that unicast upload remains an
operational problem after bridge scheduling, freeze, reconciliation, and cleanup
are fixed.
