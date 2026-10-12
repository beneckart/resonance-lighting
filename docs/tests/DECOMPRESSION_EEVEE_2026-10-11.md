# Eevee decompression canary: local signature

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Ben + this Codex laptop; sole USB operator
Target: Eevee, 9F0E54, D8:85:AC:9F:0E:54, COM16

## Scope and artifact

Ben requested identification, a health check, decompression firmware and a
final signature chime. Brian's origin commit b2ffc4e was pulled and merged;
it records Sakura's separate canary build and acceptance, with no fixture
source changes. Both benches' journal entries were retained.

Eevee received the exact retained, Ness-tested artifact under ADR 0082:

- Revision: `fx-261012-5164506-b`
- Source: `cd9ce1ae9252a2792f760a9c77a6f2df4fea3157`
- Application: 1,206,816 bytes
- SHA-256: `a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`
- FIELD, channel 11, basic listener, decompression, 300 mA precharge,
  120-second ordinary sleep, 12-second wake listen.
- Artifact directory: `firmware/fixture/build/fx-261012-5164506-b/` (local).

This is another named canary using an existing verified binary; no rebuild
or general fleet promotion was performed. See the
[Ness record](DECOMPRESSION_NESS_2026-10-11.md) for build and native tests.

## Intake and preservation

Read-only arrival telemetry showed `fx-260912-f951ae9-b` running from valid
**app1 at 0x340000**, no pending verify, FIELD/channel 11, downlight with no
class mismatch. Battery was 3.279-3.280 V and charging about +242 to +304 mA;
USB supply good, FULL tier, BQ fault 0. TMF8820, MSA311 and BMP581 reads were
healthy. The 15,000 mAh gauge setting agrees with the existing registry.

The first captured uptime was 357,710 ms, so this is an arrival snapshot,
not an untouched first boot. It already reported one solenoid strike and
`guard_interrupted=true`; the observed reset reason was power-on. Historical
PROTECT origin was low-vbat at 3029 mV. No new PROTECT entry was observed.

Private backup: `ops/bench/private-backups/20261012T030000Z-9F0E54/`.

- First 64 KiB, including bootloader, partition table, NVS and OTA selection:
  SHA-256 `42145e21bfc66aa5a24f1d78487fefb93d762323b1e08f1212fda6cdd30645c0`.
- Coredump partition, 64 KiB at 0x7f0000:
  SHA-256 `72dcc95cdb50691345570d27b76edc44dc76158ea3d84cd35d8397bebace77c6`.
  It is nonempty. Record validity and cause have not been decoded.
- Original active application at 0x340000 was ROM digest-verified against
  all 1,218,752 bytes of the retained September image, then copied privately:
  SHA-256 `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.

This is partial flash preservation. App0 and SPIFFS were not copied and were
left untouched. ROM entry/reset were recorded interventions. No profile
change or protection clear was requested.

## Flash and acceptance

ROM MAC verification matched Eevee. The installed partition table matched
the artifact. USB programming used `--no-stub` and wrote only active app1 at
0x340000. Post-write verification matched both the new application and the
entire original 64 KiB prefix before requesting the normal reset.

Fresh application telemetry confirmed the expected revision, app1 valid,
no pending verify, FIELD/channel 11, FULL tier, good supply and no charger
fault. Every `last_protect_entry*` field matched the pre-ROM snapshot.
The new boot reported `guard_interrupted=false`.

The host opened one 60-minute RAM session with `!D9F0E54:60`, waited for
at least 30 seconds of uptime and healthy sensor/power telemetry, then sent
exactly one `!K9F0E54:1` command.

- Result: COMPLETE, strike counter 0 -> 7, all seven 40 ms hits.
- Recorded GPIO onset offsets (us):
  `67, 500061, 750068, 1000073, 1500062, 2500063, 3000062`.
- Maximum onset lateness: 73 us; largest inter-hit interval error: 11 us.
- Completion: uptime 33,894 ms, battery 3.289 V, FULL tier, gate off,
  zero blocked hits and zero failsafes, session remaining 3570 seconds.

Onset measurements are software timestamps, not acoustic or oscilloscope
measurements. The user has not yet supplied a listening result for this run.
Physical USER wake, battery-only repeated use, event energy and prolonged
operation were not tested here. The capboard wiring was not independently
inspected during this USB service.

## Evidence

Public logs in `ops/bench/data/Bodhi Hive/`:

- `20261012T025850Z-9F0E54-preflight.jsonl`
- `20261012T030020Z-9F0E54-backup.jsonl`
- `20261012T030153Z-9F0E54-flash.jsonl`

Private binary backups and credentials remain excluded from Git. Decoding
Eevee's nonempty coredump is tracked in TODO.md.
