# Sakura decompression canary: local signature

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Brian + Claude Code laptop (macOS); sole USB operator
Target: Sakura, F2BE0C, 68:EE:8F:F2:BE:0C, /dev/cu.usbmodem101

## Scope and artifact

Second single-fixture canary under
[ADR 0082](../decisions/0082-decompression-local-signature.md), after
[Ness](DECOMPRESSION_NESS_2026-10-11.md). Not a fleet promotion. Sakura is the
bin-resident downlight previously used for the ADR 0070 canopy canary.

- Revision: `fx-261012-5d7ab8f-b`
- Source: `4ce6a226bf1642dba8390eebeeecfebbfe088d25` (Ness's source plus
  documentation-only commits, so a new recipe and revision)
- Application: 1,206,800 bytes
- SHA-256: `31a4942e49e5d128193041bf146ca2f7f3714dccd93cada3c50e66946c9f29c0`
- Recipe SHA-256: `5d7ab8f37a290ff85ef94ed736d14eee2c69158f8ed8a5351c7a0db7f73652f2`
- Toolchain: Arduino CLI 1.5.1 (Homebrew), ESP32 3.3.7, PowerFeather SDK 2.1.0,
  same library versions as Ness
- Flags: identical to Ness: FIELD default, channel 11, basic listener,
  decompression, 300 mA precharge, 120-second day sleep, 12-second wake listen.
- Non-secret Wi-Fi label: `party-in-the-woods-v1` (local `wifi_secrets.h` present)
- Retained artifact: `firmware/fixture/build/fx-261012-5d7ab8f-b/` on Brian's
  laptop (gitignored).

This is the first fixture artifact built on macOS. `build.sh` needs `python` on
PATH (Homebrew's `libexec/bin`); its GNU `stat -c` size echo fails cosmetically
on macOS, while the manifest records the correct byte count.

## Pre-flash state

Read-only telemetry at 02:33 UTC: `fx-260912-f951ae9-b`, app0 valid,
battery present at 3.28 V / 22 % SOC, charging about +880 mA on USB, DIM tier
(`power_tier` 1, `guard_stage` 2), BQ fault 0, solenoid enabled with zero
strikes. Retained PROTECT record: low-VBAT entry at 3032 mV. Telemetry reports
6,000 mAh gauge capacity; the registry row says 15,000 mAh. Not reconciled here.

## Evidence preserved before update

Private directory: `ops/bench/private-backups/20261012T025316Z-F2BE0C/`
(excluded from Git).

- Identity gate: ROM `read-mac` returned 68:ee:8f:f2:be:0c before any read or write.
- First 64 KiB (bootloader, partition table, NVS, otadata). SHA-256:
  `3519ed4edfc586cc1e5ed7aefab6d33d7b4054c9af108dd1f141bf4153a3a821`.
- Coredump partition at 0x7f0000. SHA-256:
  `362bb7b4308ebb435e0fd880b940b0e0c970ac5fd234e2d03762e73009c9a065`.
  **Not all 0xff:** Sakura likely holds a retained panic record. Not yet decoded.
- Installed application digest-verified against the retained
  `fx-260912-f951ae9-b` artifact (1,218,752 bytes,
  `ec03b074…5001`); a copy is in the private directory.
- Device partition table byte-identical to the new build's.
- Partial preservation: app1 and SPIFFS were not copied and were left untouched.
  Stub-assisted reads worked on macOS (Ness on Windows needed ROM reads).

Programming wrote only app0 at 0x10000. Post-write verification matched the new
app and the entire original 64 KiB prefix. No erase-all, bootloader/partition
rewrite, profile change or protection clear.

**macOS reset note:** `--after hard-reset` (RTS) left the ESP32-S3 in the stub
flasher over USB-Serial-JTAG. `--after watchdog-reset` booted the application.

Fresh application telemetry reported `fx-261012-5d7ab8f-b`, app0 valid,
`ota_pending_verify` false, decompression build, the same PROTECT record and
battery 3.241 V at DIM tier.

## Local phrase result

At 02:55:5x UTC the host opened a 60-minute session (`!DF2BE0C:60`, accepted,
3597 s left) and then sent one `!KF2BE0C:1` (accepted).

| Hit | Planned onset (us) | Recorded GPIO onset (us) | Lateness (us) |
| --- | ---: | ---: | ---: |
| 1 | 0 | 63 | 63 |
| 2 | 500000 | 500030 | 30 |
| 3 | 750000 | 750030 | 30 |
| 4 | 1000000 | 1000039 | 39 |
| 5 | 1500000 | 1500051 | 51 |
| 6 | 2500000 | 2500063 | 63 |
| 7 | 3000000 | 3000061 | 61 |

Result: COMPLETE (`signature_state` 2), counter 0 -> 7, gate off, zero blocked
strikes and zero failsafes. Maximum onset lateness 63 us. Software timestamps,
not oscilloscope or acoustic measurements. Battery sampled 3.241 V before and
3.187 V after, USB-powered, DIM tier. Brian confirmed it sounded good.

Not repeated from the Ness record: cancellation, duplicate-trigger refusal,
continuous-hold soak and physical USER trigger.

## Open items

- Decode Sakura's retained coredump from the private backup.
- ~~Reconcile 6,000 vs 15,000 mAh gauge capacity.~~ Done 03:05 UTC: Brian
  confirmed a 15,000 mAh pack; `CF2BE0C:15000` stored and verified after
  reboot. SOC is unreliable until the gauge relearns on a full charge.
- Charge fully on USB before any battery-only chime testing. Reported SOC
  (22-28 % at 6,000 mAh, 99 % after the change) is not trustworthy.
- Same remaining ADR 0082 qualification as Ness (USER wake, prolonged
  operation, event energy, remote/presence triggers).

Public evidence in `ops/bench/data/Brian bench/`:

- `20261012T025316Z-F2BE0C-canary-flash.jsonl`
- `20261012T025542Z-F2BE0C-boot-and-signature.jsonl`
- `20261012T030528Z-F2BE0C-capacity-15000.jsonl`
