# Lucas decompression canary: local signature

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Brian + Claude Code laptop (macOS); sole USB operator
Target: Lucas, 9F2638, D8:85:AC:9F:26:38, /dev/cu.usbmodem101

## Scope and artifact

Single-fixture canary under
[ADR 0082](../decisions/0082-decompression-local-signature.md). Not a fleet
promotion. Same artifact and procedure as
[Sakura](DECOMPRESSION_SAKURA_2026-10-11.md).

- Revision: `fx-261012-5d7ab8f-b` (source `4ce6a22`, built on Brian's macOS bench)
- SHA-256: `31a4942e49e5d128193041bf146ca2f7f3714dccd93cada3c50e66946c9f29c0`
  (re-checked before writing)
- Application: 1,206,800 bytes

## Pre-flash state

Read-only telemetry at 04:34 UTC: `fx-260912-f951ae9-b` running from **app1**
(valid), FIELD, channel 11, downlight, battery present at 3.281 V, charging
about +790 mA on USB, DIM tier (`power_tier` 1, `guard_stage` 2), BQ fault 0,
solenoid enabled with zero strikes, gauge capacity 6,000 mAh. Retained PROTECT
record: `reset-load-armed` at 3146 mV (historical, same pattern as Rikku).

## Evidence preserved before update

Private directory: `ops/bench/private-backups/20261012T043546Z-9F2638/`
(excluded from Git).

- Identity gate: ROM `read-mac` returned d8:85:ac:9f:26:38 before any read or write.
- First 64 KiB SHA-256:
  `8bbbd092e803d74f890b56294388c8bb066906f88c8e00d4f09b4de5162aac4a`.
- Coredump partition at 0x7f0000: all 0xff, no retained panic. SHA-256:
  `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063`.
- Installed app1 at 0x340000 digest-verified against the retained
  `fx-260912-f951ae9-b` artifact (`ec03b074…5001`); copy kept privately.
- Device partition table byte-identical to the new build's.
- Partial preservation: app0 and SPIFFS were not copied and were left untouched.

Programming wrote only app1 at 0x340000, the running slot, so otadata was not
changed. Post-write verification matched the new app and the original 64 KiB
prefix. `--after watchdog-reset` booted the application (see the Sakura
record's macOS reset note). No erase-all, bootloader/partition rewrite, profile
change or protection clear.

Fresh telemetry: `fx-261012-5d7ab8f-b`, app1 valid, `ota_pending_verify` false,
decompression build, battery 3.264 V at DIM tier.

## Local phrase result

At 04:36 UTC the host opened a 60-minute session (`!D9F2638:60`, accepted) and
sent one `!K9F2638:1` (accepted).

| Hit | Planned onset (us) | Recorded GPIO onset (us) | Lateness (us) |
| --- | ---: | ---: | ---: |
| 1 | 0 | 57 | 57 |
| 2 | 500000 | 500056 | 56 |
| 3 | 750000 | 750053 | 53 |
| 4 | 1000000 | 1000057 | 57 |
| 5 | 1500000 | 1500058 | 58 |
| 6 | 2500000 | 2500058 | 58 |
| 7 | 3000000 | 3000060 | 60 |

Result: COMPLETE (`signature_state` 2), counter 0 -> 7, gate off, zero blocked
strikes and zero failsafes. Maximum onset lateness 60 us. Software timestamps,
not acoustic measurements. USB-powered, DIM tier, battery 3.264 V before and
3.293 V after. Brian confirmed it sounded good.

## Gauge capacity

Brian confirmed a 15,000 mAh pack. Exact-target `C9F2638:15000` at 04:36:49 UTC
stored the value and rebooted (software reset). Later telemetry reported
`battery_capacity_mah` 15000, about 3.30 V, +745 to +805 mA charging, DIM
tier, no BQ fault. SOC is not recorded as meaningful: it is unreliable on these
LFP lanterns and resets with the gauge model. A 5-minute session held the
fixture awake for the readings; `!D9F2638:0` then ended it and Lucas
immediately slept (USB dropped at about 04:37:19 UTC), as expected.

Not repeated here: cancellation, duplicate-trigger refusal, continuous-hold
soak and physical USER trigger.

## Open items

- Charge fully on USB before any battery-only chime testing.
- Same remaining ADR 0082 qualification as Ness (USER wake, prolonged
  operation, event energy, remote/presence triggers).

Public evidence in `ops/bench/data/Brian bench/`:

- `20261012T043436Z-9F2638-intake.jsonl`
- `20261012T043546Z-9F2638-canary-flash.jsonl`
- `20261012T043619Z-9F2638-boot-and-signature.jsonl`
- `20261012T043649Z-9F2638-capacity-15000.jsonl`
