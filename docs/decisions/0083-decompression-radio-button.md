# ADR 0083: Exact-roster Atom signature button

Date: 2026-10-12
Status: Accepted for implementation; hardware qualification pending.

## Context

Ben selected nine serviced lanterns for decompression and requested one Atom
Matrix press to play the local signature on all nine, day or night. ADR 0082's
images only expose the phrase through USB/USER. The existing radio strike
command plays one pulse; the old conductor chooses colors after civil dusk.

## Decision

Add a separate `firmware/atom_decompression` controller with the explicit
working roster: 9F2738, 9F0E54, 9F26B0, F40254, 9F26D8, 9F26BC, 9F0E5C,
F2BE0C and 9F2638. No unrelated fixture is a target. It uses channel 11 and
does not associate with infrastructure Wi-Fi. No clock/daylight gate applies.

Allocate types 34/35 in canonical `fixture/src/core/packet.h`: exact-target
decompression control and correlated receipt. Existing types are unchanged.
Only decompression builds act on these commands; the deployed ADR 0082 images
must be updated before they can respond. There is no fallback to seven remote
pulses and no reinterpretation of the single-strike command.

While powered and enabled, the Atom renews a five-minute RAM radio lease every
five seconds. Repeated requests catch the ordinary 120-second timer wake;
they do not physically wake a sleeping radio or exit ship mode. Initial
readiness can take a full sleep/boot cadence. Power, maintenance, pending OTA
and boot-guard vetoes remain fixture-owned. Loss of the Atom permits the radio
lease to expire within five minutes. A pre-existing independent USB/USER
session may still keep the fixture awake until its own deadline.

A debounced press sends one PLAY per exact target, with three copies in a
bounded approximately 100 ms burst. Controller boot nonce and monotonic
sequence distinguish transactions. The receiver rejects stale sequences,
consumes refusals, and never retries a refused command into a later strike.
Packets delayed in its receive queue over 500 ms are ignored. The bounded
eight-controller-boot replay table retains entries at least ten minutes;
this is duplicate suppression, not cryptographic authentication or durable
replay protection across a receiver restart.

PLAY requires an existing radio lease and starts the existing locally timed
seven-hit phrase without extending that lease. Receiver receipts distinguish
READY, STARTED, COMPLETE, REFUSED, ABORTED, STALE and ENDED; COMPLETE means the
existing GPIO sequence completed, not acoustic or mechanical proof. Receipts
include request identity, remaining lease, hit count and measured lateness.
The local five-second button interval and existing receiver cooldown prevent
overlap; boot-held and continuously held buttons do not repeat. Missed presses
are not queued to surprise visitors later.

Each of nine center LEDs represents one roster member, indicating missing,
heard, ready and recent result. Serial `t`, `play`, `pause`, and `resume` provide
bounded bench control over the same roster. Pausing releases radio leases.
There is no general-purpose packet, maintenance, NVS or arbitrary-target API.

## Limits and qualification

This preserves tight rhythm within each lantern. It does not promise sample-
accurate or simultaneous starts across lanterns; packets and receiver loops
determine the onset. Strict synchronized starts require a separate deadline
contract. Powering the Atom keeps the squad available and consumes event
energy; full event runtime and the unmetered VBAT boost branch remain to be
measured. Existing physical current/voltage and mechanism safeguards stay in
force. Radio commands remain unauthenticated like the existing mesh.

Qualify the actual Atom, one named lantern, then all nine: exact versions,
receipts and local hit timing, daylight-independent behavior, duplicate/hold
handling, idle reachability, unplug/lease expiry, and listener power behavior.
Build/flash success alone does not mark the nine-lantern installation ready.
