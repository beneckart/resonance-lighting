# ADR 0059: Durable operator action and sleep provenance

- Status: Accepted; source-built, hardware validation pending
- Date: 2026-08-25
- Decider: Ben Eckart
- Extends: ADR 0037, ADR 0040, ADR 0048, ADR 0051

## Context

After the August 25 fleet fixture update, the T-Deck saw only a small awake
subset. Several observed fixtures directly reported field-profile PROTECT, but
many others stayed absent longer than the normal 900-second PROTECT cadence.
The mesh and channel remained healthy because the awake subset continued CA
gossip.

The T-Deck Sleep / Dark screen selected low-power sleep for one hour by default.
A fleet sleep still required an explicit confirmation, but an accidental
confirmed Apply could plausibly create the observed synchronized radio silence.
The current bridge retained no command history across reboot, and a fixture
reported only the generic ESP32 `deepsleep` reset reason after waking. Once the
bridge rebooted, there was no durable evidence to distinguish an operator sleep
from day-charge duty cycling, low-battery PROTECT, transport sleep, or local
serial sleep.

Writing NVS on every recurring field sleep is not acceptable. A protected unit
can wake and re-sleep every 900 seconds for days, and ordinary day-charge duty
cycling is also frequent. The useful audit must therefore separate rare state
changes from routine timer repetitions.

## Decision

1. T-Deck Bridge OS keeps a self-checksummed four-entry NVS ring for
   availability-changing fleet actions: Sleep All, Dark All, Release All, and
   the Auto / Day Dark / Night Show lifecycle overrides.
2. Each entry records the action, value, exact mesh sequence, bridge uptime,
   all/target ID, and GPS-derived UTC when a fresh fix exists. The T-Deck writes
   the entry after allocating the packet header but before sending RF.
3. Sleep, Dark, and lifecycle overrides fail closed if the action audit cannot
   persist. A restorative Release may still transmit after an audit failure so
   broken storage cannot strand a reachable fleet in a lease.
4. Every fixture writes one self-checksummed record to RTC slow memory before
   timer deep sleep. On a true deep-sleep reset, the next heartbeat can identify
   power PROTECT, day charge, broadcast radio command, targeted radio command,
   transport, or local serial as the immediate cause without a flash write.
5. A fixture writes NVS only for a validated operator sleep command and once on
   entry into PROTECT. Repeated timer sleeps within the same PROTECT episode stay
   RTC-only. If the operator-command record cannot persist, the fixture refuses
   that sleep and remains reachable.
6. Append heartbeat tail 16 to the canonical `packet.h`. Its 32 bytes carry a
   compact immediate-sleep record, the last durable operator-sleep receipt, and
   the last PROTECT-entry voltage. The full heartbeat is now exactly 192 bytes,
   within the existing fixture RX buffer and the ESP-NOW limit. Old receivers
   remain compatible through the existing length gates.
7. USB telemetry exposes the fuller RTC/NVS records. T-Deck `nb-peer` output,
   the host dashboard, and the JSONL logger expose the compact tail. T-Deck
   `show` prints all four retained bridge actions; `nb-master` continuously
   exposes the newest action and retained count.
8. The Sleep / Dark screen now defaults to reversible Dark for 10 minutes.
   Low-power sleep remains selectable and confirmed, but is no longer the
   default action or duration.

## Consequences

- A future radio-silent fleet can be diagnosed by correlating the T-Deck action
  sequence with fixture command receipts after they wake, even if either side
  rebooted in the meantime.
- A fixture power-cycled during sleep loses the RTC-only immediate record, but
  an operator sleep receipt and the last PROTECT entry remain in NVS.
- Automatic day/protection cadence does not create recurring flash wear.
- The evidence is not retroactive. Both the bridge and fixtures need firmware
  containing this ADR before a new incident can be reconstructed.
- The action ring proves that a bridge attempted an RF command. It does not
  prove every fixture heard it; per-fixture receipts provide that second half.

## Validation required

1. On an isolated T-Deck and fixture, send Dark, Release, lifecycle, and a short
   Sleep; verify the exact action/sequence survives T-Deck reboot.
2. Verify the fixture reports the same source ID/sequence after timer wake and
   retains the command receipt across a subsequent power cycle.
3. Exercise day-charge and PROTECT wakes and confirm their immediate causes use
   RTC memory while only the first PROTECT entry changes its NVS record.
4. Force an action-audit NVS failure and prove Sleep/Dark/lifecycle send no RF;
   force fixture command-audit failure and prove the fixture remains awake.
5. Confirm a pre-tail fixture heartbeat still parses and a new 192-byte full
   heartbeat is accepted by T-Deck, CoreS3, host dashboard, and logger.

