# Daisy post-burn intake, Oakland

Date: 2026-10-16 PDT. Operator: Ben + this Codex laptop, sole USB operator.
Target: Daisy, F40308, 68:EE:8F:F4:03:08, COM8.

## Arrival history and interventions

Ben reports one or two days of sunlight, otherwise untouched since storage.
During this intake he measured above 3.5 V across the battery clips and saw no
corrosion. Exact terminal voltage, board-side battery voltage and physical
cell capacity have not been established. Red and green/yellow board lights
and illuminated STEMMA sensor lights were visible. This is not a gauge reading
or confirmation of processor/USB health.

No application telemetry was captured before the following interventions:

- Original enclosure extension: no serial enumeration during the 210-second
  capture beginning 14:53:41 PDT or the start of the longer 14:57:13 capture.
- About 14:59: Ben accidentally pressed RESET while the original extension
  remained connected. No USB device was observed afterward.
- About 15:01: Ben substituted a new extension; still no observed USB device.
- By 15:02: direct USB, same laptop cable. Still no serial or failed USB device.
- About 15:04: one coordinated RESET with USER released. Both board lights
  went out for a few seconds and returned; USB remained absent.
- Ben then performed USER/BOOT plus RESET. ROM entry was not yet confirmed.
- At 15:06:16, after a requested five-second unplug/reconnect at the laptop,
  direct USB enumerated as COM8. No-reset ROM identity confirmed Daisy and
  8 MB flash. Initial port-open failure lasted only through enumeration.

Neither extension has been isolated as faulty. Direct USB also required the
laptop-end reconnect before it enumerated. A controlled ROM-mode comparison
with the original extension is pending; ROM avoids application sleep as a
variable. The recorded resets mean this is not an untouched first-wake capture.

## Preservation and installed images

The initial RAM-stub full-flash read stopped responding after approximately
24 KiB. A subsequent ROM-only read failed during SPI initialization. No full
8 MiB backup was completed. After a deliberate host USB reset into ROM,
two ROM-only 64 KiB reads completed successfully, about 37 seconds each:

| Region | Offset | Bytes | SHA-256 |
|---|---:|---:|---|
| Flash prefix (bootloader, partition table, NVS, OTA data) | 0x0 | 65536 | 3123b3fd0ce33ca0f2db9d641672d4b72de894204f45c55b40cefa61b47ff47e |
| Crash partition | 0x7f0000 | 65536 | 41b7bb34c8ce39752d3cadf48ea0942adcfa926e37ae09be77d8267643e2ae13 |

Private files: ops/bench/private-backups/20261016T220724Z-F40308-rom-read/.
They may contain credentials and must remain private. The crash region is
non-erased, but no valid crash record or cause has yet been established.

ROM verify-flash independently matched both application regions to retained
artifacts using on-device MD5 comparison. The retained binaries' sizes and
SHA-256 values were checked locally first:

| Slot | Revision | Bytes | Retained binary SHA-256 |
|---|---|---:|---|
| app0, 0x10000 | fx-260912-f951ae9-b | 1218752 | ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001 |
| app1, 0x340000 | fx-260831-dc82da7-b | 1220192 | 7230f81ff1737fa51ebdd357b9249748e5712657469de82a4b5b75799ac83299 |

OTA records have valid CRCs, sequences 7 and 6, both VALID (state 2).
Sequence 7 selects app0 under the ordinary two-slot boot rule. Application
telemetry has not yet corroborated the running slot. No host flash write,
erase, profile change or protection-clear command was issued.

## Saved protection and storage history

The prefix contains CRC-checked written NVS records in the resfx namespace:

- Stored fc_stage=1 (FULL), load_arm=1, boots=1. These follow today's recorded
  resets. They are not a pre-intake baseline; another application boot may
  react to the retained load marker. Corroborate against live telemetry.
- Historical PROTECT: 3026 mV, low-vbat origin, predecessor stage 3 (LEDS_OFF),
  reset code 255 (runtime event), no armed load, zero reset streak, fixture
  uptime 2231829 ms. Audit checksum valid.
- Last storage command: storage-usb (cause 8), untimed, source 8EB508,
  sequence 296, 3290 mV. Audit checksum valid. This matches the September
  storage ledger. It is historical context, not proof of the current mode.
- Channel 11, last known class 2 (perimeter), charge setting 2000 mA.

The September census already recorded the same 3026 mV low-vbat PROTECT entry.
The saved FULL record now shows that PROTECT was not still the stored stage
at backup time. Its release was not observed; prior sunlight and today's USB
power/reset history prevent assigning when or why recovery happened. Neither
the arrival voltage nor USB silence proves a false PROTECT trip or measures
uncharged-storage battery retention.

## Evidence

Public evidence under ops/bench/data/Oakland/:

- 20261016T215341Z-new-lantern-intake.jsonl
- 20261016T215713Z-new-lantern-intake.jsonl
- 20261016T220724Z-F40308-rom-backup.json
- 20261016T220724Z-F40308-rom-audit.json
- 20261016T220724Z-F40308-image-verification.json
- 20261016T220859Z-F40308-usb-comparison.jsonl (comparison in progress)

Prior reference: ops/bench/data/ca/20260912-155847-container/storage-final.csv
and ops/bench/data/ca/20260914-protect-recovery-analysis/fleet-cohort.json.

## Remaining checks

Complete the controlled original-extension comparison, capture the original
application's battery/charger/sensor state, and validate/decode the saved crash
region if possible. Leave the installed firmware and configuration unchanged
unless a later service request authorizes a change. Final application/exit
state is not yet established; the board was left in ROM for cable testing.
