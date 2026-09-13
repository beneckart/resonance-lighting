# ADR 0080: Explicit fixture storage and T-Deck wake selection

Date: 2026-09-12
Status: Implemented for validation; not field-qualified or fleet-promoted

## Context

Ben returned to Nevada City after roughly a week with the installed fixtures
in a dark shipping container. They cannot presently be moved into sunlight.
The preferred storage period is until physical service, with USB wake preferred
and enclosure access for RESET acceptable. QON access is less convenient.
Ben authorized OTA at operator discretion, but separately required confirmation
before any deep-sleep or ship command. Installing this capability never selects
storage automatically, including after OTA or a failed storage attempt.

Existing type 27 transport sleep permits at most seven days, then wakes with a
dark latch and radio reception. Existing PROTECT still wakes every 900 seconds.
Neither is indefinite storage. The T-Deck's former short-sleep controls do not
span this storage use case or a complete PROTECT discovery cadence.

## Decision

Extend the canonical packet.h contract with exact-target type 32 storage and
type 33 receipts. Type 32 requires a nonzero source and target, a recognized
mode, protocol version, and explicit confirmation marker. All-zero fleet
addressing is refused. This marker prevents accidental interpretation; it is
not cryptographic authorization on the existing mesh.

- Mode 1: rails off, no timer or GPIO wake; physical RESET wakes the ESP32.
  USB alone does not wake it. No solenoid-button wake is enabled.
- Mode 2: rails off, BQ25628E ship mode. Good USB/solar supply or QON wakes it;
  ESP32 RESET and bootloader buttons alone cannot wake an unpowered MCU.
  Keep the fuel gauge enabled to retain battery history. Do not claim the
  manufacturer's gauge-disabled 1 uA specification for this implementation.

Require a working battery/power manager, ordinary mesh mode, absent good
external supply, and completed OTA verification before accepting storage.
Record the source, sequence and cause in the existing durable sleep audit
before removing loads. Verify both output rails, emit a full heartbeat and
PREPARED receipt, then attempt storage. PREPARED proves intent, not electrical
shutdown. Refusals and entry failures remain distinct; reordered radio copies
must not replace failure with PREPARED. Do not silently substitute a different
wake method if ship fails.

Append storage capability bits to full heartbeats. Old peers remain parseable;
the T-Deck offers indefinite modes only for peers advertising fresh capability.
OTA leaves the normal runtime profile and existing protection policy intact.

T-Deck Power -> Store offers USB wake, RESET wake, and a Pacific local date/time
picker for the existing seven-day transport command. Calendar mode requires
fresh GPS UTC, rejects DST gaps/repeated hours, and permits 30 minutes through
seven days. It sends a decreasing duration so all receivers target the same
selected wake time despite different reception times. ESP32 sleep-clock drift
still limits accuracy; this is not an RTC alarm guarantee.

Freeze an explicit roster at review, with exact single-target selection also
available. Confirmation defaults to Cancel and explains the wake method and
loss of radio recall. A bounded 16-minute campaign covers the 900-second
PROTECT cadence with margin. Every request is audited before first RF send.
Back/Stop halts remaining sends; it cannot recall sleeping fixtures. No host
serial command for storage is added. Cold boot does not resume a campaign.

## Tradeoffs and validation

Published V2 board figures are 24 uA deep sleep with gauge enabled and 1 uA
ship with gauge disabled: 4.032 versus 0.168 mAh per week. Across 114 fixtures,
that illustrative difference is only 0.440 Ah/week. The main savings come from
eliminating repeated radio wakeups, not choosing ship over uninterrupted deep
sleep. Attached-hardware leakage and the gauge-enabled ship current need
external measurement. Fuel-gauge awake current cannot measure sleep current.

Primary references, checked 2026-09-12:

- https://docs.powerfeather.dev/#current-consumption
- https://docs.powerfeather.dev/sdk/usage-notes/
- https://docs.powerfeather.dev/guides/reduce_power_usage/
- Installed PowerFeather SDK 2.1 Mainboard::enterShipMode implementation.

Native tests cover wire layouts and request rejection, durable audit validity,
campaign bounds, expiry/rollover, receipt ordering, and Pacific calendar edges.
Embedded builds and an accessible one-fixture sleep/USB-wake/current test are
required before claiming this feature field-qualified. Ben's sleep-command
confirmation remains required for the live test and any storage rollout.

The T-Deck registry generator also now displays newly registered unnamed
fixtures by exact ID and allows retired fixtures to retain historical callsign
assignments. It still rejects malformed, duplicate, or unknown assignments.
This resolves pre-existing roster drift without inventing human callsigns.

## 2026-09-12 field rollout result

Ben explicitly authorized fleet storage and direct laptop operation. The
retained f951ae9 fixture image and tdeck-0.3.0-storage4 controller completed
106 exact-target USB-storage PREPARED receipts with matching durable audits.
The final receipt was 18:39:20 PDT. At 19:17:57, all 106 remained radio-quiet
for at least 38m37s with continuous capture and no bridge restart; every job
was stopped. Eight unobserved installed fixtures were not commanded.
See `docs/tests/CONTAINER_STORAGE_CENSUS_2026-09-12.md` and retained job ledgers.
This promotes the radio-command path for this authorized deployment only;
physical USB wake, assembled storage current and LCD inspection remain open.
The final UI path is Home -> Rest -> Store. ADR 0081 supersedes the earlier
host-serial exclusion for the narrowly confirmed USB-storage operation.
