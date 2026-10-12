# Ness decompression canary: local signature

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Ben + this Codex laptop; sole USB operator
Target: Ness, 9F2738, D8:85:AC:9F:27:38, COM14

## Scope and artifact

Ben selected USER + USB first, with remote/presence triggers next. This is a
single-fixture canary under [ADR 0082](../decisions/0082-decompression-local-signature.md),
not a fleet promotion. Ness has the installed LFP and a v2 capboard fed from
VBAT instead of its disconnected former VDC feed. D7 remains the trigger.

- Revision: `fx-261012-5164506-b`
- Source: `cd9ce1ae9252a2792f760a9c77a6f2df4fea3157`
- Application: 1,206,816 bytes
- SHA-256: `a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`
- Recipe SHA-256: `5164506b6e8a3ad76bf9e8bde05f6f4e999df336ec247973117977922c06ece9`
- Toolchain: Arduino CLI 1.5.1, ESP32 3.3.7, PowerFeather SDK 2.1.0
- Flags: FIELD default, channel 11, basic listener, decompression, 300 mA
  precharge, 120-second day sleep, 12-second ordinary wake listen.
- Non-secret Wi-Fi label: `party-in-the-woods-v1`
- Retained artifact: `firmware/fixture/build/fx-261012-5164506-b/` (gitignored),
  including manifest, recipe, build options, application, ELF, map and partitions.

The complete native suite passed before final refinements. The final local
sequence tests pass 58 checks, including absolute timing, no catch-up, gate
collision, cancellation, cooldown admission, actual-hit accounting and session
expiry. New command parser tests pass 25 checks. The final clean-source ESP32
build passed: 36% flash, 21% static RAM. The earlier interrupted 3df99aa build
was abandoned and never flashed.

## Evidence preserved before update

Private directory: `ops/bench/private-backups/20261012T010803Z-9F2738/`.
This contains device data and firmware credentials and is excluded from Git.

- ROM read of the first 64 KiB preserved bootloader, partition table, NVS and
  OTA selection. SHA-256:
  `c675104a34b304df774d6dfd6b42a4a66a27bb1f62db52a76b9385f37d34ac99`.
- Retained original `fx-260912-f951ae9-b` application was verified against all
  1,218,752 bytes at 0x10000 using the ROM flash digest, then copied privately.
  SHA-256: `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.
- ROM read of the 64 KiB coredump partition at 0x7f0000 was all 0xff: no
  retained panic. SHA-256:
  `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063`.
- This is a partial flash preservation. App1 and SPIFFS were not copied and
  were left untouched. Bulk stub-assisted reads failed; ROM reads and digest
  verification succeeded. This does not isolate the bulk-read failure to the
  replacement extension, host driver or flashing tool.

Programming used `--no-stub` and wrote only app0 at 0x10000. The new partition
table matched the installed table. Post-write ROM verification matched both
the new app and the entire original 64 KiB prefix before the normal reset.
No erase-all, bootloader/partition rewrite, profile change or protection clear
was performed.

Fresh application telemetry reported the expected revision, FULL tier, good
USB supply and the original historical PROTECT record: reset-load-armed,
predecessor DIM, brownout reset reason 9, 3242 mV sampled at 2603 ms. The update
did not erase or reinterpret that evidence.

## Local phrase result

At 01:28:00 UTC the host opened a 60-minute RAM session. At 01:28:22 UTC it
sent exactly one `!K9F2738:1` line. Ness scheduled all seven 40 ms pulses.

| Hit | Planned onset (us) | Recorded GPIO onset (us) | Lateness (us) |
| --- | ---: | ---: | ---: |
| 1 | 0 | 59 | 59 |
| 2 | 500000 | 500058 | 58 |
| 3 | 750000 | 750083 | 83 |
| 4 | 1000000 | 1000059 | 59 |
| 5 | 1500000 | 1500057 | 57 |
| 6 | 2500000 | 2500056 | 56 |
| 7 | 3000000 | 3000055 | 55 |

Result: COMPLETE, strike counter 0 -> 7, gate off, zero blocked strikes and
zero failsafes. Maximum onset lateness was 83 us (0.083 ms). The largest
inter-hit interval error was 25 us. These are software timestamps immediately
after enabling the GPIO, not oscilloscope or acoustic measurements.

Sampled battery voltage across this capture ranged from 3.199 to 3.336 V;
FULL tier remained in force and the battery recovered to approximately
3.33 V afterward. This is not a measurement of the minimum transient voltage,
and it does not qualify sustained chime duty or the unmetered VBAT branch.
Ben's listening assessment is pending.

## Cancellation and service hold

A second phrase was started, then a duplicate trigger and a cancel were sent
after its first hit. The duplicate was refused. Cancellation reported one
hit, counter 7 -> 8, gate off and zero failsafes; no extra phrase was queued.
The subsequent continuous-USB hold passed: one open serial connection observed
uptime advancing from 44,855 to 195,776 ms (150.921 seconds), with no dropout or
reboot. At completion the battery was 3.337 V, power tier FULL, gate off,
failsafes zero and session time remaining 3409 seconds. This covers the former
120-second sleep interval, not a prolonged event soak.

## Use and remaining qualification

- USER or `!K9F2738:1` plays the phrase. One-second cooldown follows completion
  or cancellation. A held USER button does not repeat.
- `!K9F2738:0` cancels playback; `!D9F2738:0` ends the event session.
- `!D9F2738:60` sets a one-hour session. The allowed range is 1..120 minutes.
  A first successful USER/signature trigger opens one hour if no session is
  active; later hits do not extend it. A reset ends the RAM-only session.
- During a session the class listener light stays on and ordinary day sleep
  is suppressed. Battery protection and maintenance retain their vetoes.
- Outside a session this build listens for 15 seconds then sleeps. USER can
  wake it and request the phrase. Physical USER wake, acoustic assessment,
  prolonged operation and event energy remain to be qualified.
- Remote station/presence integration is the next stage. No new mesh packet
  contract or fleet rollout is included in this canary.

Public evidence in `ops/bench/data/Bodhi Hive/`:

- `20261012T012650Z-9F2738-canary-flash.jsonl`
- `20261012T012800Z-9F2738-local-signature.jsonl`
- `20261012T012841Z-9F2738-cancel-and-hold.jsonl`
