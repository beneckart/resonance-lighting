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

Neither extension was isolated as faulty by those initial observations.
Direct USB also required the laptop-end reconnect before it enumerated. The
original extension subsequently passed the ROM and application tests below;
the replacement was not separately qualified. The recorded resets mean this
is not an untouched first-wake capture.

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
Sequence 7 selects app0 under the ordinary two-slot boot rule. Subsequent
application telemetry corroborated app0, exact revision fx-260912-f951ae9-b,
VALID with no pending verification. No host flash write, erase, profile change
or protection-clear command was issued.

## Saved protection and storage history

The prefix contains CRC-checked written NVS records in the resfx namespace:

- Stored fc_stage=1 (FULL), load_arm=1, boots=1. These follow today's recorded
  resets. They are not a pre-intake baseline. The next application boot
  explicitly reported stored=1 and moved to DIM because of that marker.
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

## Original extension and application test

The original extension re-enumerated at 15:10:06 PDT without a button press.
ROM identity succeeded, followed by a 37.16-second read of the same 64 KiB
prefix. Every byte matched the direct-USB baseline above. This establishes
working data communication in the tested orientation, not environmental or
all-orientation qualification. Because it passed, it was left connected for
the application test; no return-to-direct leg was performed.

At 15:11:02, after another exact ROM identity check, the host requested one
watchdog reset to resume the original application. Esptool reported a port
configuration error during disappearance, but subsequent application logs
confirmed the reset and boot. The read-only capture began at 15:11:06:

- Boot report: stage=2, stored=1, interrupted=1, reset=poweron. This is the
  recorded bench reset advancing FULL to DIM while a load marker was set,
  not evidence of a new field failure or PROTECT entry.
- Original FIELD firmware, channel 11, app0 VALID, 6000 mAh gauge setting,
  2000 mA charge setting. Physical cell capacity remains unconfirmed.
- First sample at uptime 12523 ms: 3.335 V, +1.5 mA. By uptime 30116 ms:
  3.499 V, +346.9 mA. Good external supply, charging enabled and BQ fault zero.
  These are awake, USB-connected readings, not sleep-current measurements.
- VL53L5CX and MSA311 were present and read successfully; all 16 ToF zones
  were valid in the last sample of the initial capture. Radio initialized.
- The historical 3026 mV low-vbat PROTECT and storage source/sequence were
  unchanged and corroborated the saved NVS.
- The next read-only captures observed natural DIM -> FULL recovery by
  uptime 75482 ms, with battery voltage about 3.49 V. No guard clear, profile
  change or firmware update was sent. The interrupted flag describes this
  boot's origin; it is not a report of another interruption after recovery.

Across 86 application samples, the captured transition was at 15:12:18.914
PDT. The final sample at 15:12:47.251 reported uptime 101064 ms, 3.491 V,
+328.1 mA, FULL/guard stage 1, healthy ToF/accelerometer reads and radio up.
The capture processes were then closed; no ongoing host service hold remains.

## Evidence

Public evidence under ops/bench/data/Oakland/:

- 20261016T215341Z-new-lantern-intake.jsonl
- 20261016T215713Z-new-lantern-intake.jsonl
- 20261016T220724Z-F40308-rom-backup.json
- 20261016T220724Z-F40308-rom-audit.json
- 20261016T220724Z-F40308-image-verification.json
- 20261016T220859Z-F40308-usb-comparison.jsonl
- 20261016T221014Z-F40308-usb-original.json
- 20261016T221102Z-F40308-app-resume.json
- 20261016T221106Z-new-lantern-intake.jsonl
- 20261016T221145Z-new-lantern-intake.jsonl
- 20261016T221217Z-new-lantern-intake.jsonl
- 20261016T221217Z-F40308-application-summary.json

Prior reference: ops/bench/data/ca/20260912-155847-container/storage-final.csv
and ops/bench/data/ca/20260914-protect-recovery-analysis/fleet-cohort.json.

## Remaining checks

Validate/decode the saved crash region if possible and confirm physical cell
capacity when convenient. No matching ELF was found in the retained September
artifact directory during intake. The original extension passes the current
setup's data test; the initial silence's precise cause remains unisolated.
The fixture remains on its original FIELD firmware and configuration, now FULL,
with its ordinary lifecycle sleep policy. No indefinite awake hold was applied.
