# 0086 -- Perimeter squad interaction canary

Date: 2026-10-15
Status: Accepted for the Marill F40348 first interaction test; hardware
qualification pending. No receiver rollout or pattern-mode changes.
Owner: Ben

## Decision

Ben requested presence plus USER on Marill to play one bell at a time through
the nine decompression lanterns, then all nine on interaction ten. Repeat in
the established roster order: Ness, Eevee, Rikku, Leeloo, Bidoof, Pinsir,
Tangela, Sakura, Lucas, all. Rejected/not-ready presses do not advance or queue.
An accepted step is consumed before radio transmission: partial delivery is
reported, never automatically replayed later. Minimum interval is 2.5 seconds.

The exact-target `--squad-trigger-target F40348` bench build requires the
matching `--protect-recovery-target F40348`. It preserves ADR 0085 protection
and recovery and cannot be combined with the decompression receiver flag.
Marill is a controller, with its own solenoid requests disabled. No change to
the nine receiver images or their stored parts/settings is needed.

Sessions are explicit, RAM-only, 1..120 minutes. `!TF40348:60` starts one hour;
`:0` ends it. USER when idle opens one hour; a USER deep-sleep wake opens it
after power checks settle. Subsequent USER presses, `!JF40348:1`, or presence
advance the cycle. There is no queued bell for the waking press, since the
receivers may still be asleep. Session expiry returns to 120-second sleep.
Presence cannot wake a sleeping sensor; USER or USB opens the next session.
Day/night scheduling does not gate an active session. Power, OTA, maintenance,
and staged recovery veto it. No current-sign requirement is added.

The ADR 0083 exact-target five-minute radio leases repeat approximately every
three seconds, gathering sleeping peers at their existing wake cycle. A fresh
READY receipt (under 12 seconds) is required for every intended recipient.
Session stop sends lease release; lost releases expire within five minutes.
No resets, profile writes, pattern writes, or persistent wake settings occur.

## Compatibility and timing

Use canonical `NbEvent` kind 2 for single strikes. Its existing payload has
no target: it MUST be sent by ESP-NOW unicast to the roster MAC learned from
a matching source-ID heartbeat or lease receipt, NEVER broadcast. Failure
to add/send a unicast peer has no broadcast fallback. Unknown fixtures never
receive these strike frames. The shared Atom/perimeter roster also contains
Brian's 2026-10-14 per-lantern pulse widths (13..18 ms).

One random nonzero event ID per interaction and up to three copies per target
use the existing receiver dedupe. Every copy's relative delay refers to one
deadline one second after admission. Stop sending with under 250 ms remaining.
This tolerates lost copies and avoids serial/USB onset pacing. The legacy
receivers fire in their main loops, with a 250 ms maximum lateness allowance;
RF queue delay, loop jitter, and mechanisms still affect acoustic alignment.
This is a first scheduled group-strike test, NOT ADR 0084's 1 ms pattern timer
or a measured sub-millisecond synchronization claim. Delivery counters prove
queueing only; this legacy message has no execution/acoustic receipt.

## Presence and feedback

Use fresh VL53L5CX 4x4 frames: any valid return at 150..700 mm on two
consecutive frames triggers after 800 ms of clear space (no 150..800 mm return).
Boot-held presence, stale readings over one second, sensor failures, and a
stationary hand cannot repeatedly fire. Invalid data never counts as clear.
The 150 mm lower bound excludes Marill's observed fixed ~103 mm return in
zone 14. This is a hand-wave setup; mounting and distance tuning remain a
bench qualification item, not a general crowd detector.

One dim pixel shows amber while the next target is unavailable, green when
eligible, and blue after an accepted interaction. Existing output power caps
and the staged-recovery renderer remain authoritative. USB telemetry reports
session remaining, next step, READY mask, each observed revision, send errors,
rejections, and detector rearm state.

## Validation

Native tests exercise all ten steps, cooldown and wraparound, missing peers,
held/stale/failed presence, framed exact-target commands, and legacy receiver
deadline/dedupe compatibility. Build-wrapper tests enforce bench variant,
matching recovery identity, and receiver/controller separation. Hardware
results are recorded separately after the immutable canary is flashed.

## 2026-10-15 canary amendment -- bounded radio recovery

The first hardware run heard all nine expected receiver revisions, but after
the first unicast strike its TX queue stopped making useful progress; immediate
send failures followed and READY receipts expired. Battery, main loop, sensor
sampling and heap remained healthy. The exact driver error was not exposed by
that image, so the root cause is not established.

For the controller only, serialize all sends (including its heartbeats) until
the preceding send-completion callback. Record immediate driver errors and
callback successes/failures. This follows Espressif's recommendation to wait
for the preceding callback rather than stack ESP-NOW sends:
https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/network/esp_now.html

A missing completion or persistent immediate error for 750 ms aborts any
pending strike, invalidates READY receipts and detector rearm, and cycles the
radio off/on with a cooperative 200 ms pause. Keep the consumed cycle step;
never replay its strike. At most three radio repairs occur per boot, then end
the session and return to ordinary sleep. A failed radio restart likewise
ends the session. No NVS mutation or processor reset is added. This is bounded
fault handling, not proof of the original failure's cause or a substitute for
repeating the hardware test.
