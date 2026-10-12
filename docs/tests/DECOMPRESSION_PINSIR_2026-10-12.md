# Pinsir decompression service

Date: 2026-10-12 PDT / UTC
Operator: Ben + this Codex laptop; sole USB operator
Target: Pinsir, 9F26BC, D8:85:AC:9F:26:BC, COM3

## USB intake and manual bootloader entry

Ben requested the same health check, preservation, decompression firmware,
and final local signature routine. Origin was already up to date at a7eae99.
He heard a USB sound before capture started, but the first watcher saw no
serial device. A physical RESET did not produce enumeration. He reported
that USER chimed before BOOT+RESET, then stopped chiming afterward.

A replacement extension, reported brand new, produced Windows descriptor
failure Code 43 (USB VID_0000/PID_0002), with no COM port. An elevated restart
of that exact failed device succeeded as a Windows operation but did not
clear the error. Unplugging/reconnecting at the laptop did not clear it.
Bypassing the enclosure extension with the same laptop cable and port
produced COM3 at 18:01:54 UTC. A no-reset ROM read confirmed the exact MAC.
This proves manual bootloader entry worked and the direct path communicated;
it does not by itself prove both extension cables are defective. Ben requested
a repeat of the new extension after the flash with the application held awake.

## Preserved pre-reset state and service-induced PROTECT

Before the next application boot, saved the first 64 KiB and the 64 KiB
coredump partition by ROM reads under:
`ops/bench/private-backups/20261012T180258Z-9F26BC-first-rom/`.

- Prefix SHA-256:
  `b3976c075078ff23000d38e15e171e1178869f8d4b719389748a1b7bbae398c4`.
- Coredump SHA-256:
  `5bf5e8ed8812b58f463bf796c116e5d4469c6bf2f8dba396a068fe0ad993a760`.
  Nonempty; not yet validated/decoded as a panic record.

Offline parsing of live NVS entries, with entry CRCs and the audit record's
own checksum checked, found `resfx/fc_stage=2` (DIM), `load_arm=1`, `boots=0`,
capacity 15000 mAh and charge limit 2000 mA. Its previous PROTECT audit was
reset-load-armed, predecessor DIM, reset reason 9 (brownout), 3225 mV,
streak 1, uptime 2094 ms. The date of that historical event is unknown.
This backup follows today's earlier button operations; it is not an untouched
post-burn snapshot.

After Ben's requested physical RESET out of ROM, the September firmware
`fx-260912-f951ae9-b` booted valid app0 at 0x10000, FIELD/channel 11. It
reported power-on reset, PROTECT stage 4, interrupted=true, and a NEW
reset-load-armed audit at 3247 mV, predecessor DIM, reset reason 1, streak 1,
uptime 2627 ms. Initial voltage was 3.245-3.247 V; charging then raised it
to approximately 3.274-3.275 V with no BQ fault.

This is direct evidence of a deliberate service reset consuming the guard's
load-armed path, not a new measured low-battery event. The marker alone does
not distinguish the earlier USER strike from an active LED rail. The previous
brownout audit survives in the first private backup and sanitized NVS extract.

Sent F0/L0 after preserving telemetry to hold the fixture awake with the LED
rail off. Normal sustained charging proof released PROTECT at about 3.276 V
and +341 mA, persisted OFF, and rebooted. Healthy ToF/MSA sensors returned
(sensor bits 9). The normal upward ladder reached DIM, and F1/L0 restored
FIELD with the rail off before the second backup. No protection-clear or
battery-capacity command was sent.

## Selected artifact and completion

Selected the same retained Ness-tested image, without rebuilding:

- Revision: `fx-261012-5164506-b`
- Source: `cd9ce1ae9252a2792f760a9c77a6f2df4fea3157`
- Application: 1,206,816 bytes
- SHA-256:
  `a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`
- FIELD/channel 11, basic listener, decompression, 300 mA precharge,
  120-second ordinary sleep, 12-second listening grace; individual canary.

After normal recovery, a second private backup was saved under
`ops/bench/private-backups/20261012T180627Z-9F26BC-service/`. Its prefix
SHA-256 is `b24de627c2c5ea6d0f1cef8acf4008286cccf21f12e75e9626ceddc4e39d8b30`.
The crash record was unchanged. The original app0 matched all 1,218,752 bytes
of the retained September binary by ROM digest and was copied privately:
`ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.
This is partial preservation; app1 and SPIFFS were not copied or written.

Only app0 at 0x10000 was flashed. At 18:10:04 UTC, ROM verification matched
the new application and unchanged second-backup prefix. Fresh telemetry
confirmed the expected revision, valid app0, FIELD/channel 11, DIM allowance,
healthy downlight ToF/MSA and no pending verification or charger fault.
Capacity remained 15000 mAh, charge limit 2000 mA, and the new service-reset
PROTECT audit remained intact. No manual protection clear was used.

The host opened a 60-minute RAM session at 18:10:07 UTC, waited over 30 seconds
of stable uptime, and sent one `!K9F26BC:1` at 18:10:35.

- COMPLETE: seven 40 ms hits, strike counter 0 -> 7, gate off, no blocks or
  failsafes.
- GPIO onset offsets (us): `60, 500054, 750057, 1000058, 1500060, 2500055, 3000057`.
- Maximum onset lateness 60 us; maximum inter-hit interval error 6 us.
- Completion sample: 3.121 V, DIM. At uptime 39446 ms it had recovered to
  3.271 V, +334 mA charging, DIM, no interrupted flag or new reset.
- Expected RAM-session expiry: about 19:10 UTC / 12:10 PDT, unless reset.

These are sampled voltages and firmware GPIO timestamps, not a transient
minimum or acoustic measurement. Physical USER wake, battery-only repeat use,
and whole-system energy remain unqualified on this fixture. This image is
not OTA-qualified.

## Awake extension comparison

Ben clarified that the extension used for the postflash retest was the
original dusty one, not the brand-new replacement requested in the prompt.
The first watcher's metadata describes the planned new-extension test; the
actual reported cable identity takes precedence. The new extension was tested
separately afterward, as recorded below.

Direct USB remained healthy for roughly nine minutes after the signature.
During the swap, a brief reconnect at 18:20:09 UTC reported uptime 604978 ms,
2999 seconds of session time remaining, DIM and no fault; it then disconnected.
The watcher reached its 600-second deadline before completing a continuous
extension capture. A fresh, explicitly dusty-labeled capture beginning
18:21:02 saw no serial port. Windows also showed no current USB error device.
That capture was stopped to make a separately recorded direct control test.

Ben returned the same cable directly to Pinsir, with the battery connected
and no button presses. At 18:21:52 UTC, telemetry resumed at uptime 708191 ms.
The host interval was 103.262 seconds and the uptime increase 103.213 seconds:
the MCU did not reboot or enter ordinary deep sleep during this gap. The same
awake session remained active, and every retained PROTECT field was unchanged.
The final direct sample at 18:22:10 reported uptime 725670 ms, battery 3.272 V,
+331 mA charging, DIM, no charger fault, seven total strikes, zero blocks and
failsafes, with 2878 seconds left in the session.

Conclusion: the dusty-extension USB data path failed in this setup while the
direct path worked before and afterward. Ordinary firmware deep sleep and a
new PROTECT event do not explain this particular failure. This does not locate
the fault within the extension, contacts, orientation or combined cable path,
and lack of USB data does not establish absence of charging power. No additional
chime, reset, configuration or session command was issued during this test.
Pinsir was left on direct USB at that stage; captures and temporary host awake
holds ended.

### Clean extension follow-up: passed while awake

Ben then connected the clean extension. COM3 enumerated normally. A read-only
capture from 18:23:11 to 18:24:11 UTC collected 55 telemetry samples over
60.100 seconds without transport errors. Compared with the preceding direct
sample, host time advanced 61.444 seconds and MCU uptime 61.419 seconds. The
same session continued, with no reboot or change to any PROTECT history field.

All samples had good supply, charging enabled, no BQ fault, DIM allowance and
no interrupted guard flag. Battery voltage ranged 3.257-3.268 V and gauge
charge current +127.4 to +288.4 mA. The final sample was 3.264 V, +249.1 mA,
supply 4.609 V, uptime 847219 ms, with 2756 seconds left in the session.
The strike count remained seven with zero blocks/failsafes; no extra chime,
reset or configuration command was sent. The capture and host awake hold ended,
leaving Pinsir connected through the clean extension.

The clean extension passes this awake data/charging check. Its earlier ROM
enumeration failure remains unexplained; these observations do not establish
that it is defective or identify contact/orientation/host-state effects. The
dusty extension remains the path that failed the direct/extension/direct check.

### Clean extension: PowerFeather-end plug flipped

Ben rotated only the extension's male plug at the PowerFeather 180 degrees,
leaving the host-side connection unchanged. The capture began after the flip;
it does not measure enumeration latency during the physical reconnection.
COM3 supplied 56 telemetry samples over 60.175 seconds, from 18:26:12 to
18:27:12 UTC, without a transport error. Since the previous clean-extension
sample, host time advanced 120.878 seconds and MCU uptime 120.859 seconds.
The MCU stayed in the same awake session, with unchanged PROTECT fields.

Final sample: uptime 1028260 ms, battery 3.265 V, +238.6 mA charging, supply
4.625 V, good supply, charging enabled, DIM, no interrupted guard or BQ fault,
seven strikes and 2575 seconds remaining in the original event session.
Battery range was 3.260-3.269 V, with gauge charge current +167.6 to +286.2 mA.
Only read-only telemetry requests were sent; the capture and host awake hold
ended normally.

Both tested orientations at the PowerFeather end work with the current
host-side orientation and awake firmware. This test did not reproduce an
orientation fault. It does not qualify the other connector's orientations or
explain the earlier bootloader enumeration failure.

### Clean extension: outer connection flipped

Ben next reported "flipped at end," interpreted as rotating the laptop cable
plug at the extension's female socket while retaining the prior flipped
PowerFeather-end orientation. This interpretation was stated to Ben before
capture. COM3 delivered 55 samples over 60.038 seconds, from 18:28:01 to
18:29:01 UTC, without a transport error. Host time since the preceding sample
advanced 48.695 seconds and MCU uptime 48.741 seconds; the same awake session
continued and PROTECT fields were unchanged.

All samples had good supply, charging enabled and zero BQ fault. Final sample:
uptime 1136984 ms, battery 3.268 V, +289.1 mA charging, supply 4.609 V, DIM,
no interrupted guard, seven total strikes and 2467 seconds remaining in the
original session. Battery range was 3.259-3.270 V and gauge charge current
+148.5 to +301.4 mA. Only telemetry queries were sent. The capture and host
awake hold ended normally.

Using A for each connection's initial working orientation and B for its
180-degree rotation, the awake checks (including the final test below) cover:

| PowerFeather end | Extension female socket | Result |
| --- | --- | --- |
| A | A | Passed 60-second capture |
| B | A | Passed 60-second capture |
| B | B | Passed 60-second capture |
| A | B | Passed 60-second capture |

No orientation-dependent failure has been reproduced in these checks. The
earlier ROM enumeration failure remains unexplained.

### Clean extension: fourth orientation combination passed

Ben returned only the PowerFeather-end plug to its initial orientation A,
leaving the outer connection in B. COM3 delivered 56 samples over 60.929
seconds, from 18:29:54 to 18:30:55 UTC, without a transport error. Host time
since the prior capture advanced 52.484 seconds and MCU uptime 52.538 seconds.
The same awake session continued with unchanged PROTECT fields.

Every sample had good supply, charging enabled and zero BQ fault. Final
sample: uptime 1250416 ms, battery 3.270 V, +282.5 mA charging, supply 4.609 V,
DIM, no interrupted guard, seven strikes, zero solenoid failsafes and 2353
seconds left in the original session. Battery range was 3.259-3.270 V and
gauge charge current +152.6 to +302.4 mA. Only read-only telemetry requests
were sent. Capture and temporary host awake hold ended normally.

All four clean-extension orientation combinations pass these awake checks.
This argues against a consistently missing orientation connection in this
extension. It does not rule out intermittent contact or enumeration issues,
qualify bootloader behavior, or explain the dusty-extension failure.

## Evidence

Public captures are under `ops/bench/data/Bodhi Hive/`:

- `20261012T174828Z-new-lantern-intake.jsonl`
- `20261012T175201Z-new-lantern-intake.jsonl`
- `20261012T180258Z-9F26BC-first-rom.jsonl`
- `20261012T180258Z-9F26BC-first-rom-nvs-audit.json`
- `20261012T180426Z-new-lantern-intake.jsonl`
- `20261012T180521Z-9F26BC-recovery.jsonl`
- `20261012T180715Z-9F26BC-backup.jsonl`
- `20261012T180850Z-9F26BC-flash.jsonl`
- `20261012T181043Z-9F26BC-direct-postflash-snapshot.json`
- `20261012T181043Z-9F26BC-extension-retest.jsonl`
- `20261012T182102Z-9F26BC-dusty-extension.jsonl`
- `20261012T182137Z-new-lantern-intake.jsonl`
- `20261012T182311Z-9F26BC-clean-extension.jsonl`
- `20261012T182612Z-9F26BC-clean-extension-board-flipped.jsonl`
- `20261012T182801Z-9F26BC-clean-extension-host-flipped.jsonl`
- `20261012T182953Z-9F26BC-clean-extension-fourth-orientation.jsonl`
