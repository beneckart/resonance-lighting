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

## Expected solar/USB wake versus the observed sequence

Code review on October 16 used Daisy's verified September image's source,
fcc5ef07ccf65f0f1d034bfe523e9f1d7f68bf10, and retained manifest. This is not a
claim about the newer Marill recovery canary. No additional device operation
was performed for this review.

- The recorded storage-usb request selects BQ ship mode, with the ESP powered
  off. Good external input from either USB or the solar panel ends that mode.
  Assuming functioning panel wiring and sufficient input, the first sunshine
  should therefore have powered Daisy up before this USB session. Darkness
  afterward does not automatically issue another ship-mode command.
- After ship wake, the original FIELD application resumes, including retained
  protection state and ordinary timer sleep. The ordinary daytime interval is
  120 seconds, followed by a 12-second listening grace; other lifecycle gates
  can extend awake time. Unqualified PROTECT recovery instead sleeps for
  900 seconds, with an 8-second wake grace. A qualifying recovery proof holds
  the device awake to complete its continuous 60-second check.
- Ordinary timer sleep arms the timer and, if released, USER-button EXT0 wake.
  It does not arm a USB-power wake source. Consequently, plugging USB into an
  already powered but deeply sleeping ESP is not the same event as supplying
  power to a ship-mode board. Charging can proceed while USB data is absent;
  the processor may wait for its next timer wake.
- Physical RESET on a powered board should start the processor and the
  application initializes Serial even when PROTECT parks the sensor rail.
  Ordinary cold/reset boot has a ten-minute lifecycle grace; PROTECT has its
  own 30-second cold/reset grace instead. These are policy windows after
  initialization, not guarantees against a separate boot or USB fault.
  The observed absence after the coordinated reset cannot be explained just
  by the sensor rail being off or by a 15-minute timer still counting down.
- Correct ROM boot bypasses the application, including its PROTECT and sleep
  policy. ROM communication was confirmed only after the laptop-end replug.
  This points to an additional USB connection/enumeration or physical-path
  issue, but does not isolate Windows, the USB peripheral, cable contacts or
  another cause. The successful original-extension comparison removes the
  basis for calling that extension failed from the initial silence alone.

The saved FULL stage proves recovery had occurred by backup time, not whether
sunshine or this USB session produced it. The historical 3026 mV PROTECT audit
survives recovery and is not evidence that Daisy was still parked on arrival.
Likewise, days in sunlight and a high terminal voltage are not a log of the
processor's intervening wake cycles.

Source paths at that commit: fixture.ino (unconditional Serial initialization),
src/esp32/board_power.cpp (ship and timer entry), solenoid.cpp (EXT0 wake),
behavior_glue.cpp (ordinary grace), power_glue.cpp (PROTECT grace), and
src/core/power_policy.cpp (release proof). Paths are relative to firmware/fixture/.

Platform references: [TI on BQ25628 solar/adapter ship wake](https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1275790/bq25628-bq25628-ship-mode)
and [Espressif USB sleep behavior](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/usb-serial-jtag-console.html#sleep-mode-considerations).
Espressif describes deep-sleep USB disconnection and expected reconnection on
wake. Its explicit persistent-pullup failure discussion is for light sleep;
that is not proof of Daisy's failure mechanism. The manual-sleep guidance also
documents physical reconnect as a way to reestablish the connection.

## Additional chained-extension check

After that review Ben inserted two additional extensions in series, one new
and one previously deemed faulty. The capture beginning 22:21:49 UTC initially
saw no serial port. COM8 then returned without a requested reset or cable
change, and 20 telemetry records identified Daisy F40308 on the original
September firmware, FULL/guard 1. The reset reason was deepsleep and the last
sleep was day-charge for 120 seconds. Thus this initial silence was followed
by a verified natural timer wake with working USB through the assembled chain.
No awake hold or configuration change was issued. This is a functional data
check of the current assembled path and orientation, not an all-orientation
or environmental cable qualification. The capture completed and closed.

Evidence: ops/bench/data/Oakland/20261016T222149Z-F40308-extension-chain.jsonl.

## Windows event baseline across three wake windows

At Ben's request, kept the assembled cable chain and original FIELD firmware
unchanged. A Windows USB ETW capture ran from 15:30:18 to 15:35:28 PDT,
alongside no-reset serial discovery and read-only t queries. No physical
reconnect, device reset, PnP restart, profile change or device awake hold was
requested. A host-only sleep inhibitor kept the laptop awake during capture.
An initial ETW start was denied because sandbox elevation did not grant a
Windows administrator token; the subsequent approved Windows UAC launch
started the trace successfully. Both captures stopped automatically.

Providers: USBHUB3, UCX and USBXHCI, Default + StateMachine (0x201), level 5.
No transfer-payload keywords were enabled. Raw ETL and Windows topology are
retained privately because the capture is host-wide. Daisy mapped to the
USB composite parent with her exact MAC, root-hub port 1, with COM8 as MI_00.

| Wake | Windows enumeration start (PDT) | Enumeration duration | Result |
|---|---|---|---|
| 1 | 15:30:22.651 | 197.421 ms | NTSTATUS 0, COM8, valid Daisy telemetry |
| 2 | 15:32:44.393 | 197.674 ms | NTSTATUS 0, COM8, valid Daisy telemetry |
| 3 | 15:35:05.957 | 197.192 ms | NTSTATUS 0, COM8, valid Daisy telemetry |

The trace contains three enumeration starts and three successful completions,
with zero recorded enumeration retries, enumeration failures, port-reset
timeouts or descriptor-validation failures. Its 7,010 records include the
trace header and the three selected providers. This is not a claim about
unrecorded traffic-level errors or rare failures beyond this short run.

All 29 telemetry samples reported FULL/guard 1 and BQ fault zero. Each wake
reported deepsleep after day-charge/120 seconds; ToF and accelerometer were
present, with valid ToF reads and radio up. Two fully observed COM-absent
intervals were 120.035 and 119.956 seconds. The first two sleep transitions
closed the serial handle with a transport error; these coincide with normal
USB disappearance and subsequent successful timer wakes. The third transition
explicitly printed `deep sleep (day-charge), timer wake 120s` before closure.

This supports Ben's observation that an already established connection can
survive ordinary sleep/reconnect cycles. It did not reproduce the earlier
first-attachment failure, exercise a software USB recovery, or prove that
such a failure cannot recur. The next more informative comparison is a fresh
perimeter's first USB attachment with ETW already armed and prior solar
exposure recorded, before reset or connector changes.

Evidence under ops/bench/data/Oakland/:

- 20261016T223018Z-F40308-usb-cycles.jsonl
- 20261016T223018Z-F40308-usb-cycles-summary.json
- 20261016T223018Z-F40308-usb-etw-summary.json

Private raw trace: ops/bench/private-backups/20261016T223018Z-F40308-usb-cycles/usb.etl.
SHA-256: ef48e6f0cc32d209c6cd7ce86e3e4c1b729fb3afd4ad7fdfbcc921bb98623d3c.
