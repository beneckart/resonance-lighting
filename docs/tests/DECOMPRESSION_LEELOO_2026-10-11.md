# Leeloo: powered recovery and decompression canary

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Ben + this Codex laptop; sole USB operator
Target: Leeloo, F40254, 68:EE:8F:F4:02:54, COM17

## Intake and recovery

Ben requested the same intake, decompression flash and final local signature
workflow used for the preceding lanterns. The first captured boot at 04:27 UTC
reported software reset, fx-260912-f951ae9-b, valid app1 at 0x340000,
FIELD/channel 11, OFF tier and stored guard stage 3. Battery was present at
3.274 V. Charging became enabled during startup, with good USB supply and no
BQ fault. The configured capacity was 6,000 mAh; Ben confirmed that the
installed physical cell is 15 Ah.

Retained history records a low-vbat PROTECT entry at 3042 mV, predecessor
stage 2 (DIM), load armed, reset reason 255 and reset streak 0, at historical
uptime 18804223 ms. The previous operator command was USB-wake storage from
bridge 8EB508. The first live snapshot was already OFF, so this capture does
not establish when or how an earlier PROTECT latch released. It does not
establish a false PROTECT entry.

After saving arrival telemetry, the host sent F0/L0 at 04:27:33 UTC for a
powered commission service hold with the LED rail off. Normal recovery
reached DIM at 04:28:10.694, with the log reporting 3.289 V and +255 mA.
F1/L0 restored the original FIELD profile at 04:28:15; FIELD and rail-off
were confirmed at 04:28:16. No manual protection clear was sent.

## Preservation and artifact

Private backup: `ops/bench/private-backups/20261012T042800Z-F40254/`.
This is a post-recovery backup, before the capacity correction, rather than
an untouched pre-boot image. Public serial traces retain the arrival and
recovery states.

- First 64 KiB, including bootloader, partition table, NVS and OTA selection:
  SHA-256 `5ae8e28b9be463b468dacda22c9654bac112c356f92d1a9360697a34b4d0e517`.
- Coredump at 0x7f0000: all 0xff; no retained crash record in that partition.
  SHA-256 `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063`.
- Original app1: ROM digest matched the retained September image, which was
  copied privately, 1,218,752 bytes, SHA-256
  `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.

This is partial flash preservation. App0 and SPIFFS were not copied. The
partition table matched the selected artifact. The selected image is the
exact [Ness-tested](DECOMPRESSION_NESS_2026-10-11.md) binary, without a rebuild:

- Revision: `fx-261012-5164506-b`
- Source: `cd9ce1ae9252a2792f760a9c77a6f2df4fea3157`
- Application: 1,206,816 bytes
- SHA-256: `a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`
- FIELD, channel 11, basic listener, decompression, 300 mA precharge,
  120-second ordinary sleep, 12-second listen; individual-fixture canary.

## Host standby interrupted the first upload

The first write started at 04:30 UTC. Windows recorded entry to Modern
Standby at 04:30:50.123 (idle timeout) and exit at 04:34:05.347 (mouse input).
The flash tool subsequently reported a stopped serial stream at 86.6% and
exited with code 2. No capacity or chime command had been sent. Windows
still enumerated the same COM17/MAC; a no-reset reconnect timed out, then
an explicit ROM reset/read-MAC restored communication. No flasher remained
running before retry. A temporary host awake hold was started, and Ben also
disabled the Windows three-minute sleep setting.

The next pre-write guard correctly stopped because the 64 KiB prefix no
longer matched its original backup. A separate private read preserved it as
`post-interruption-prefix-64KiB.bin`, SHA-256
`b94b503a0b1787ecafe059098484310e7236d3d56d0522d725ec2486d7c3d606`.
The only change was 29 bytes in NVS sector 0xa000: the `boots` counter
advanced from 0 to 1, with the corresponding NVS entry/bitmap changes.
Bootloader, partition table, OTA selection and every other NVS entry were
unchanged. This does not establish which intervening reset incremented it.

The resumed write used this verified post-interruption prefix for its
unchanged-configuration check, preserving the counter and both private
snapshots. It rewrote the same complete app1 image from the beginning.

## Acceptance and capacity correction

At 04:38:10 UTC ROM verification matched the new app1 and the complete
post-interruption prefix. App0 and SPIFFS were left untouched. Live telemetry
confirmed the expected revision, valid app1, no pending verification,
FIELD/channel 11 and unchanged retained PROTECT history.

At 04:38:13 the host sent exact-target `CF40254:15000`. After the expected
software reboot, fresh telemetry confirmed 15,000 mAh. This intentionally
changes NVS after the flash/prefix verification; the original 6,000 mAh
configuration remains in both private prefix backups. The charge limit
remained 2,000 mA. Reported SOC is not a reliable capacity estimate after
resetting the gauge model.

A 60-minute RAM session started at 04:38:17 UTC. After at least 30 seconds
of stable uptime, healthy downlight ToF, DIM tier, good supply and no charger
fault, the host sent exactly one `!KF40254:1` at 04:38:44.

- COMPLETE: seven 40 ms hits, counter 0 -> 7, gate off, no blocks/failsafes.
- GPIO onset offsets (us): `58, 500060, 750060, 1000052, 1500060, 2500066, 3000061`.
- Maximum onset lateness 66 us; maximum inter-hit interval error 8 us.
- Completion at uptime 34129 ms: 3.301 V, DIM, 3570 session seconds left.
- Follow-up at uptime 39450-45828 ms: 3.305-3.311 V, charging +148 to
  +273 mA, DIM, no charger fault, continuous uptime and seven total strikes.
- All retained `last_protect_entry*` fields matched the pre-ROM snapshot.

The session expires around 05:38 UTC / 22:38 PDT unless reset or explicitly
changed. It consumes battery after USB removal; the VBAT capboard remains
connected when firmware sleeps. Physical listening, USER wake, battery-only
repeated use and overnight energy remain unverified on this fixture.
GPIO timing is firmware evidence, not acoustic or oscilloscope measurement.
The temporary host awake hold was released after acceptance.

## Public evidence

Under `ops/bench/data/Bodhi Hive/`:

- `20261012T042707Z-F40254-preflight.jsonl`
- `20261012T042733Z-F40254-recovery.jsonl`
- `20261012T042819Z-F40254-backup.jsonl`
- `20261012T042953Z-F40254-flash.jsonl` (interrupted write)
- `20261012T043500Z-F40254-host-standby.json`
- `20261012T043506Z-F40254-flash.jsonl` (reconnect timeout; no write)
- `20261012T043533Z-F40254-rom-reconnect.jsonl`
- `20261012T043536Z-F40254-flash.jsonl` (prefix guard stopped; no write)
- `20261012T043633Z-F40254-prefix-investigation.jsonl`
- `20261012T043654Z-F40254-flash.jsonl` (verified write and acceptance)
- `20261012T043853Z-F40254-settled-power.jsonl`

Origin was checked before publishing; no new Brian commits were present.
