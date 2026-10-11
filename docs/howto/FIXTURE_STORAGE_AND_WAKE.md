# Fixture storage and wake

Updated: 2026-10-06. This is the operator summary for the implemented September
12 storage firmware. It does not authorize a new device operation.

## Choose the wake contract

| T-Deck Rest -> Store option | What happens | How it wakes |
|---|---|---|
| Until USB power (ship) | Untimed BQ25628E ship mode; ESP and output rails off; fuel gauge stays enabled. | Good USB **or solar** input, or QON. RESET/BOOT alone cannot wake an unpowered ESP. |
| Until physical RESET | Untimed ESP deep sleep, output rails off, no timer/GPIO wake. | Physical RESET. USB alone is not its wake trigger. |
| Wake on date | Timed transport sleep to a selected Pacific date/time, 30 minutes to seven days ahead. | ESP timer; radio resumes with outputs latched dark. Requires fresh handheld GPS time; clock drift limits accuracy. |

The date picker rejects ambiguous/repeated or nonexistent DST times. Neither
indefinite mode can be recalled over ESP-NOW. The controller, laptop and Wi-Fi
are unnecessary after storage entry. "USB wake" is the UI shorthand, not a
promise to ignore a connected solar panel.

Storage and PROTECT are separate. Storage does not set or clear the durable
PROTECT latch. RESET, ordinary OTA and ship wake do not themselves clear it;
normal recovery policy still applies. A 12-hour radio sleep is also not a
command to enter PROTECT. See the
[recovery investigation](../tests/PROTECT_RECOVERY_INVESTIGATION_2026-09-14.md).

## Review and operate

1. Obtain a fresh census over a complete 15-minute PROTECT cadence. Name the
   exact intended fixtures; silence does not establish their battery state.
2. Verify compatible fixture firmware/capability and stable power for any
   needed OTA. Installing firmware alone never requests storage. Follow the
   [artifact handoff contract](FIRMWARE_ARTIFACT_HANDOFF.md).
3. On the T-Deck, open Home -> Rest -> Store, select the wake method and exact
   target(s), then review and explicitly confirm. Review freezes the roster,
   method and date; Cancel is initially focused. Keep the handheld and screen
   active through the bounded 16-minute catch-up campaign.
4. Review receipts and failures. Stop/Back prevents further sends but does not
   recall fixtures already asleep. Restarting the handheld never resumes a job.

The fixture refuses storage during maintenance, with good external supply,
without a working battery/power manager, or before OTA verification completes.
It verifies output rails are off and persists the request before acknowledging
PREPARED and attempting entry. PREPARED plus subsequent silence is useful
command evidence, not a measurement of electrical shutdown. Never silently
substitute another wake method after a refusal/failure.

For an explicitly authorized laptop campaign, storage4 also exposes the bounded
USB host flow in the [T-Deck guide](../../firmware/tdeck_bridge/README.md#explicit-usb-host-storage-adr-0081)
and [ADR 0081](../decisions/0081-explicit-host-usb-storage.md).
`ops/bench/fleet_storage_usb.py` requires explicit confirmation, the expected
bridge/revision/channel, exact targets and a new exclusive ledger. It matches
receipt source and sequence, checks late failures and stops the job. A queued
USB write is not proof of fixture storage. Do not replay historical job plans
as a current roster.

## Retained firmware and completed rollout

These are the exact historical artifacts, not a claim that a rebuild of current
main has identical bytes. Preserve their revision, recipe and binary identity.

| Target | Revision | Application bytes | Source commit | Manifest |
|---|---|---:|---|---|
| Fixture | `fx-260912-f951ae9-b` | 1,218,752 | `fcc5ef07ccf65f0f1d034bfe523e9f1d7f68bf10` | [Fixture manifest](../../firmware/fixture/build/fx-260912-f951ae9-b/manifest.json) |
| T-Deck Plus | `tdeck-0.3.0-storage4` | 1,578,224 | `556c2f6cdbefd152a1f79437b9e7f859b7b7fd44` | [Controller manifest](../../firmware/tdeck_bridge/build/storage-host-20260912-r4/manifest.json) |

Fixture application SHA-256:
`ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.
T-Deck application SHA-256:
`11b98b33ca46babf21ad27cb1f2edd7cccdf6319a5e130df3e5442fd6260c1b1`.

The fixture image uses FIELD, channel 11, LFP, 300 mA precharge, a basic
listener, 120-second ordinary day sleep and 12-second listening grace. Its
existing `b` suffix is preserved: Ben explicitly approved this exact artifact
for the September storage operation; this is not a general new fleet release.
The controller was flashed to `8EB508` / `44:1B:F6:8E:B5:08`. The September
14 Oakland snapshot found the other controller, `979604`, on older firmware.

On September 12, all 106 observed fixtures passed fresh exact-revision OTA
verification beyond the pending window, then returned matching storage receipts
and durable audits. The final live check recorded at least 38m37s of quiet,
continuous capture and stopped jobs. Eight unobserved baseline fixtures were
not commanded; Kairi and Wooper were later heard in Oakland, leaving six
unlocated as of September 14. Radio reach did not prove container location.
See the [census and rollout](../tests/CONTAINER_STORAGE_CENSUS_2026-09-12.md)
and [retained evidence](../../ops/bench/data/ca/20260912-155847-container/README.md).

## Qualification still open

At service, measure assembled gauge-enabled ship and timerless deep-sleep
current, physically test USB/solar/QON and RESET wake, and inspect the T-Deck
storage/calendar screens. The shipped command path has evidence; these
electrical and physical checks do not yet. Do not apply the vendor's
gauge-disabled 1 uA ship number to this gauge-enabled implementation.

Most modeled savings come from removing recurring radio windows; choosing
ship over uninterrupted deep sleep buys the practical external-power wake.
Cell-terminal measurements are needed to interpret historical very-low
telemetry. Remove external power before battery swaps. Preserve reset,
protection, charger and sleep history before changing a diagnostic specimen.

Implementation contracts: [ADR 0080](../decisions/0080-explicit-container-storage.md)
and [ADR 0081](../decisions/0081-explicit-host-usb-storage.md).
