# Ben's T-Deck Tone Lab update

Date: October 17, 2026 (PDT).
Operator: Ben + Codex, sole USB writer for 8EB508 on this laptop.
Target: 44:1B:F6:8E:B5:08, COM10, LilyGO T-Deck Plus, 16 MiB flash / 8 MiB PSRAM.
Status: PASS. Flashed, stable boot and mesh verified, Tone Lab menu confirmed by Ben.

## Source and scope

Ben requested Brian's newest T-Deck firmware, already running on TSwift
979604. The first fetch through 0139e3d contained only the lantern-side
changes. The later push through b2778e4 includes the merged brian/tdeck-tonelab
work: show playback, Atom rules, and the event-mode selector. Brian's logged
TSwift mode-switch flash used a development build of 2c2ff2f; the later source
also clarifies the view control and adds the show sender/documentation.

Merged that source, retaining both benches' journal entries. Local source
332e7cb131b5f31685636d83c9512d4e3d9f20ac changes only the retained release name
to tdeck-0.3.1-tonelab1; the new Tone Lab behavior is Brian's implementation.
This is a new Windows build, not a claim of byte identity with TSwift's
development binary. No fixture, Atom, or TSwift firmware is changed here.

The retained build directory is
firmware/tdeck_bridge/build/tdeck-tonelab-20261017-332e7cb/.
The toolchain matches Brian's documented versions: ESP32 Arduino 3.3.7,
LVGL 9.5.0 and LovyanGFX 1.2.24. Build options pin USB hardware CDC,
16 MiB QIO flash, 8 MiB OPI PSRAM, and app3M_fat9M_16MB partitions.

## Intake and preservation

Passive telemetry confirmed tdeck-0.3.0-storage4, channel 11, successful
radio sends with zero reported failures. ROM independently confirmed the
full MAC and memory sizes. The selected app is app0 at 0x10000, size
0x300000. The existing bootloader and partition table are retained.

Bulk RAM-stub reads stalled, first at 4 KiB and later at 32 KiB. Neither
baud-rate reduction nor a fresh stub produced a complete bulk backup.
Direct ROM reads did save the boot/NVS/OTA prefix and crash region. No
complete new 16 MiB backup is claimed. The existing app0 digest exactly
matches the separately retained 1,578,224-byte storage4 binary, SHA-256
11b98b33ca46babf21ad27cb1f2edd7cccdf6319a5e130df3e5442fd6260c1b1.

Private preservation directory:
ops/bench/private-backups/20261017T163639Z-8EB508-tdeck/.

- prefix-rom.bin: 65,536 bytes, SHA-256
  cbb81df00df1ba797ac3b510c5a48b788054245905b24eadecd81d09814fe2ab.
- coredump-rom.bin: 65,536 bytes, SHA-256
  71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063.

After the read-only work, the watchdog reset returned a host-side serial
exception as USB detached. Fresh old-firmware telemetry confirmed that the
application did restart. The final PASS in the backup log records this;
the preceding helper exception is not evidence of a failed backup read.
Settings: channel 11, day display, night backlight 115, unset WiFi profile;
API key output is redacted. No fleet commands were sent.

## Verification

The native show parser / playback-model / Atom-click test passed. The shared
packet layout passed all 94 checks. The full native runner was initially
blocked by its environment/preflight: non-login Bash lacked core utilities,
and its cache-maintenance test then refused while the legitimate ROM reader
was active. These were host invocation/preflight issues, not failing model
assertions. Build and hardware results are recorded below.

The firmware contains the Tone Lab player, not TSwift's user-loaded show.
No show or interaction rules are copied as part of this firmware request.
On a previously unconfigured handheld the service starts without a show,
with keep-awake off and no mode command sent. A show can later be exported
from the laptop Tone Lab and loaded with ops/bench/tdeck_show_send.py.

## Retained build

The cold build completed successfully. Application size is 1,599,792 bytes,
SHA-256 8b365d2260a9acb690fd824fa6eb1fa1d8d90e07296274bc8d54f8a222c0cbe7.
Reported flash usage is 50%; static RAM is 212,248 bytes (64%). The linker
emitted its toolchain .note.GNU-stack warning; the final compile/link returned
success. Actual build options, partition bytes, embedded revision and binary
hash were checked before flashing. The immutable manifest is
Oakland/tdeck-0.3.1-tonelab1-manifest.json; the build log is
Oakland/20261017-8EB508-tonelab-build.log.

After the compiler and ROM reader exited, the build-wrapper contract test
passed on its normal login-shell invocation. The targeted model test and
94 packet-layout checks also passed. No source repair was needed beyond
assigning the new retained release name.

## Hardware result

On October 17 at about 09:51 PDT, flashed only app0 through the ROM loader
at 115200 baud. The written application digest matched the retained binary;
the saved 64 KiB boot/NVS/OTA prefix remained unchanged. App1 and the FAT
filesystem were not written. Fresh serial evidence then confirmed:

- tdeck-0.3.1-tonelab1, target 8EB508, channel 11.
- Stable operation through 60,021 ms uptime, 29 successful radio sends and
  zero send failures; fresh traffic from 11 peer IDs.
- LVGL shell initialized, 8 MiB PSRAM detected, keyboard and touch detected,
  audio codec at 0x40 and GPS detected at 38400 baud.
- Settings matched preflight, including channel, display, backlight, model
  and unset WiFi profile. Key output remained redacted.
- Tone Lab had zero loaded patterns/rules, playing=-1 and awake=0. No show
  playback or fleet mode change was requested during verification.

Ben opened Tone Lab from the launcher and confirmed its menu appears.
The postflash check exited successfully. Evidence:
ops/bench/data/Oakland/20261017T164928Z-8EB508-tonelab-flash.jsonl.
This confirms firmware and menu bring-up, not an end-to-end performance of
a loaded show. Brian's later flash-record commit 43dda55 was also merged;
it contained no further T-Deck source changes.


## Follow-up: saved show loaded

At Ben's subsequent request, loaded the repository's Brian bench show onto
8EB508 using ops/bench/tdeck_show_send.py with the explicit target ID.
The source is ops/bench/data/Brian bench/tonelab-show.tls, introduced in
54e32e1: 2,383 bytes, CRC32 4650112D, SHA-256
747294ab98dd06154f45d5a7aa75818bf46ccf3a6926c994c6f2d9a77dd02531.
Worktree bytes matched the committed Git blob exactly before transfer.
Its embedded export timestamp is 2026-10-17T00:16:18.448Z.

Fresh preflight verified 8EB508 on tdeck-0.3.1-tonelab1 with no loaded show.
The device acknowledged all 7 patterns, 2 rules and 11 roster entries with
no storage warning. A T-Deck-only software reboot then confirmed all seven
patterns still Active and both rules enabled. Fresh master telemetry at
20003 ms showed channel 11 and zero send failures. Playback was idle
(playing=-1), and keep-awake remained off (awake=0). No play or fleet-mode
command was sent, and no other device was written.

Patterns: Beat 1; Cha Cha Cha; Drowning in Soup; Freakout Mode; Jingle Balls;
Shave and a Haircut; Thriller?. Enabled rules accept any Atom: single click
runs Freakout Mode twice and returns to prior playback; double click advances
to the next Active pattern. These are the saved source's own settings.

Evidence: ops/bench/data/Oakland/20261017T165618Z-8EB508-brian-show-load.jsonl.
This follow-up supersedes the earlier empty-show state; the earlier firmware
flash observations remain unchanged.
