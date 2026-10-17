# 0087 - Verified USB service preparation and bounded power confirmation

Date: 2026-10-16
Status: Daisy-only qualification; not fleet-promoted

## Evidence and decision

Daisy F40308's deliberate USB ROM service interrupted FULL with the load-armed
marker set. The subsequent power-on-class reset consumed the DIM retry. She
recovered after 60 s of qualifying voltage (about 73 s after boot). Ordinary
daytime wakes, however, are only 12 s and reset those RAM-only timers.

Preserve thresholds, the FULL -> DIM -> PROTECT collapse ladder, durable
PROTECT and its release rules. Make deliberate service explicit and let
power-policy confirmations own a bounded extension of ordinary daytime wakes.

## USB service

`!B<exact-id>:1` requests a 30 s USB service window; `:0` cancels via a clean
software reboot. Require external power, COMMS and no pending OTA verification.
Fence new LED/solenoid arms before pausing the radio and stopping all loads.
Verify the physical LED enable/gate LOW, NVS load-armed clear and unchanged
stored tier. Clear/read back the unexpected-reset streak only after this
explicit quiescence, so repeated planned service cannot count as a collapse
loop. Never write the stored tier or clear PROTECT. A failed check never
acknowledges READY. The host must check fresh telemetry and the matching READY
before any ROM reset. Older firmware must bootstrap through OTA first.

The fence persists for the window, including after failed preparation. Only
telemetry/cancel are actionable then; the normal output loop and radio stay
paused. Repeated prepare requests cannot extend the deadline. Expiration
performs an expected software reboot, eliminating stale queued output state.
The power policy remains live and may impose a protective shutdown sooner.
Native ROM resets outside this handshake remain conservative by design.

## Power confirmation

An active non-PROTECT voltage timer may defer ordinary daytime sleep for at
most 150 s from its first start in this boot. This covers both 60 s steps of
OFF -> DIM -> FULL without treating sleep as observed healthy voltage.
The extension also covers downward confirmation. Intermediate tier changes
carry permission in the same tick so lifecycle cannot sleep between steps.
Missing battery readings reset confirmation; a false condition ends its
streak. Chatter cannot renew the 150 s cap. A later normal wake starts a fresh
opportunity. Critical voltage still imposes PROTECT immediately. Explicit
operator sleep and OTA ownership retain their existing behavior.

## Qualification boundary

`--power-recovery-test-target F40308` requires artifact variant `t` and exposes
`!YF40308:1` (DIM) / `:2` (OFF), only from healthy FULL. It reduces the stored
tier and sleeps for three seconds so the real deep-sleep path is exercised.
It cannot clear PROTECT and is absent from the final ordinary artifact.
Test missing/refused/mismatched USB replies, repeated deliberate ROM service,
DIM and OFF real sleep recovery, timed cancellation, and unchanged history.
Skitty F3FD28 is excluded from all operations.
