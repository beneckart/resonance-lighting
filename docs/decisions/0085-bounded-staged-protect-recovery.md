# 0085 -- Bounded, staged recovery from PROTECT

Date: 2026-10-15
Status: Accepted for an exact-target Marill F40348 canary; hardware validation
and fleet promotion are separate gates. The decompression squad is excluded.
Owner: Ben
Amends: ADR 0023/0051/0068 automatic PROTECT release for the canary only.

## Problem and intent

Ben authorized replacing a durable protection state that can strand a healthy
fixture after a false or transient entry. Marill's untouched specimen supplied
a concrete example of high battery voltage with deliberately unpowered sensing;
its original entry's correctness remains unknown. A healthy fixture must have
periodic opportunities to demonstrate that it can operate again, including at
night and under solar input that is insufficient to charge while the radio runs.

The previous current proof observed awake current only. The firmware's software
integrator resets each boot and does not integrate processor sleep. A negative
awake sample cannot establish a whole-cycle energy deficit. MAX17260 hardware
continues sampling in hibernate (nominally every 5.625 seconds) and exposes a QH
coulomb counter, but this firmware/SDK does not yet collect and validate QH deltas
across sleep, rollover, gauge POR and initialization. Neither SOC nor an assumed
sleep current is an automatic recovery gate. References:

- https://ez.analog.com/power/battery-management-system/a/documents/do7111/can-the-max17260-registers-be-read-while-in-hibernate-mode
- https://ez.analog.com/power/battery-management-system/a/documents/DO7074/how-often-does-max17260-poll-raw-coulomb-count-qh-register-4dh-10ms-or-175ms

## Decision

1. Keep the immediate critical-voltage stop, load-armed reset escalation,
   default-off startup, and durable fc_stage=PROTECT. Existing voltage ladder
   thresholds and hardware charging limits are unchanged.
2. Add a versioned, checksummed retry journal. Spend and read-verify a retry
   before granting one software reboot into sensor-only operation. Consume the
   ticket before peripheral initialization. Every reset during an unfinished
   trial parks again, including a software reset. NVS failure cannot grant a
   trial. Keeping fc_stage=PROTECT through the trial also parks an older image
   if deliberately rolled back during testing.
3. Require fresh VBAT and post-wake-valid IBAT, corroboration that a real cell
   exists, and readable fault-free charger status. Current's sign, external
   supply presence, charger enable and SOC do not determine eligibility.
   The canary requires 30 continuous seconds at raw VBAT >=3.25 V before a
   trial. Raw voltage avoids using an uncertain source-resistance correction
   to justify a restart. Raised per-unit OFF thresholds raise the trial gates.
4. Stage recovery through 30 seconds of sensor operation, 30 seconds at LED
   cap 32/255, and 30 seconds at cap 128/255, using one white test pixel.
   This stays below the dense-HEX current limiter, which would otherwise make
   both all-pixel stages the same load. The ordinary frame limiter remains on.
   No autonomous or commanded chimes and no ordinary program frames may run
   during the trial. Keep raw VBAT >=3.15 V during loaded stages; sag, invalid
   power evidence, a charger fault or a failed rail start aborts. Bound the
   trial to 180 seconds. A passed trial persists DIM before clearing its ticket;
   the ordinary ladder must subsequently qualify FULL.
   Sensor-domain power must be verified, but absent/degraded ranging is its own
   diagnostic and cannot permanently veto recovery of otherwise working lights.
5. After successive failed/spent trials, wait 5 minutes, 15 minutes, 1 hour,
   then 6 hours between attempts. Saturate the frequency limit, not the number
   of attempts: a later recovery remains possible. Count survives all resets.
   Cooldown credit survives only a verified timer wake from our own recorded
   sleep; a cold or early button reset cannot erase the cooldown. Parked peers
   continue periodic radio service windows no more than 15 minutes apart.
6. Forget prior failures only after two continuous minutes of healthy lit
   service. A quiet boot or reconnect alone is not evidence of load stability.
7. Expose retry phase, count, remaining delay and reason over USB telemetry.
   Retain the last successful trial's generation, minimum voltage/current and
   whether external power was seen, so reconnecting USB does not erase evidence
   that an earlier battery-only trial succeeded. Retry-history forgiveness
   preserves that result. Independently corroborate a full, zero-current cell
   with the existing rate-limited BQ presence test.
   Keep the original burn-week entry audit intact during explicit fault tests.
   Canary-only, complete-line, exact-target USB commands inject a latch or reset
   a running trial. They cannot clear the retry count or bypass its delay.

## Deployment and acceptance

Use --protect-recovery-target F40348 with artifact variant b and the existing
field/channel-11/basic-listener defaults. The target gate also applies at boot
before identity initialization. Other fixture identities retain the old policy.
Do not combine this experiment with decompression firmware or fleet OTA.

Native tests must cover false entry, current-independent qualification, staged
limits, missing/stale evidence, real sag, reset during trial, damaged journal,
cooldown saturation, timer rollover, and framed command rejection. Hardware
acceptance requires an injected false latch on Marill, automatic staged return,
a deliberately interrupted trial with retained backoff, and a battery-only
sleep/wake recovery. Retain exact binary SHA, immutable manifest, fresh identity,
power/sensor readings and complete transition logs. Simulated faults do not
replace a later instrumented resistive-harness/weak-solar collapse experiment.

This canary does not claim every real fault is recoverable. A disconnected cell,
unreadable power hardware or unusable NVS must remain safely parked and reported.
The existing SDK can briefly enable VSQT during a true cold initialization;
this change does not modify that external library. Warm-reset/timer recovery
uses the retained off state and the consumed trial ticket. Cold-start sensor
shorts require a separate rail-sequencing qualification before fleet promotion.
