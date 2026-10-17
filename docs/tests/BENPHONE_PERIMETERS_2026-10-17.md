# BenPhone provisioning for Ben's three perimeters

Date: October 17, 2026 (PDT).
Operator: Ben + Codex, sole USB writer for F3FD28 and F40348 on this laptop.
Daisy F40308 is verification-only because its installed image already supports
BenPhone. Brian owns his separate laptop/connected squad.

Status: PASS -- all three actual BenPhone HTTP endpoints verified. This is an OTA-access bootstrap, not the final installation
behavior. Ben will choose installation behavior after the Loom site is known.

## Intended artifacts

- Skitty F3FD28: retained ordinary perimeter fx-261017-c2ddd6a-b,
  SHA-256 09a718c6da858615cbe47346bfd0ea0d37ad820304391ade368391a48438530e.
  This is Daisy's qualified BenPhone + private fallback image, with the USB
  service and bounded DIM/OFF confirmation fixes. No staged-PROTECT canary or
  decompression/event trigger flags. Skitty's natural recovery experiment
  completed before this explicitly requested update.
- Marill F40348: rebuild its exact source 1a53a2398d4e53507a5799069bce1b18d199f6e7
  plus only the shared wifi_benphone.h file, source checkpoint efdce31.
  Preserve both exact-target recovery and squad-trigger flags, field profile,
  channel 11, 300 mA precharge, 120 s sleep / 12000 ms listen, and 50/400/600 mm
  bounds. No new event-basic behavior or new roster is included. New recipe:
  fx-261017-4f2d8ef-b, SHA-256
  6581606bbfb4e53d574646215b79033bc681c35897bd32b60f2f09642bf8573f.
- Daisy F40308: expected c2ddd6a; verify fresh identity and skip writing.

## Legacy USB preparation

These two old images predate the ADR 0087 READY handshake. For this explicit
USB bootstrap, inspect their exact source and use existing USB `u` maintenance
entry: it calls allLoadsOff, including durable guard disarm, before WiFi scan
or join. Require a fresh post-disarm WiFi-join banner before ROM entry; then
read and CRC-check the NVS prefix and require load_arm=0 and the unchanged
stage before any application write or restart. Refuse on a missing banner or
failed guard check. This is a narrowly source-checked legacy bootstrap, not
permission for ordinary service tools to fall back to unprepared resets.

The initial preservation plan was to save a private full 8 MiB flash image,
verify the partition layout, write only
the active app slot, and verify that image and unchanged 64 KiB prefix. Keep
private backups and embedded private WiFi credentials out of Git. Marill's
revised preservation path is recorded below. Require
fresh expected-revision VALID telemetry beyond 35 s and unchanged battery
configuration, class, channel, power tier and historical PROTECT context.

## Intake

Skitty first reported old b3e2738, FULL, 3.388 V, healthy external input,
charger fault zero, VL53L5CX/MSA311 present, app1 VALID. Its 3023 mV original
PROTECT record is intact. This is fresh post-recovery evidence, not the
pre-intervention condition at yesterday's initial intake.

Fresh USB intake confirmed all three exact identities. Daisy reports
c2ddd6a, app0 VALID, FULL, 3.602 V, load_armed=false, healthy sensors: no
flash needed. Marill reports 220d90a, app1 VALID, FULL, 3.597 V and both
sensors present. Intake reader closed all ports after the three samples.
Raw capture: Oakland/20261017T154716Z-perimeter-benphone-intake.jsonl.

## Skitty USB transport and preservation

The first attempt encountered a transient COM-node/open race before any
command. On the second, source-reviewed maintenance preparation succeeded
and ROM identity matched, but a 921600-baud stub read failed before saving
the prefix. No flash was written. ROM was re-entered while already parked,
then its 115200-baud reader saved the prefix. A lower-baud stub read with
no-reset-stub matched that prefix byte-for-byte. Subsequent 460800-baud
operations keeping the RAM stub alive succeeded. This does not isolate
which of speed or stub lifetime caused the first failure.

The CRC-checked NVS prefix confirmed fc_stage=1 and load_arm=0 before writing.
The full 8 MiB original flash was saved privately and checked against the
prefix; SHA-256 b66e1d475b07103a7bb96173a5dfc4b5421abc413b229eaa788630a8b4c6daa5.
Only app1 at 0x340000 was written. New binary and unchanged prefix verification
passed. Logs: 154835Z (no-open attempt), 154950Z (preparation/transport),
155132Z (resume/backup/write), all F3FD28-benphone-usb.jsonl under Oakland.

At 08:53:59 PDT, Skitty passed fresh app1 VALID verification at 35.132 s
uptime: c2ddd6a, FULL/stage 1, 3.399 V, +235.1 mA, fault zero, VL53/MSA
reading, channel 11 and normal FIELD operation. Configuration and every
last_protect_entry field match preflight. The reset-interrupted flag is true,
but the disarmed reset did not escalate the stage. No chime was commanded.
New-image OTA upload has not been repeated on Skitty.

## Marill build

Clean build completed at 08:55:11 PDT. Artifact fx-261017-4f2d8ef-b,
1,236,256 bytes, SHA-256
6581606bbfb4e53d574646215b79033bc681c35897bd32b60f2f09642bf8573f.
Recipe SHA-256 4f2d8ef8158bff71c0b6d1c791edb90245e819cb2f9b8458d38ab27f41f0a9a7.
Source efdce31bb78cbf33fedb8067b1faab0c0fbdbf0b, clean. Verified identical
feature flags, libraries and toolchain to 220d90a, actual build options,
embedded identity and BenPhone credentials, and byte-exact retained outputs.
Flash usage 36%, static RAM 21%. Manifest is retained with the Oakland data.

The session-specific legacy bootstrap driver is retained as non-executable
20261017-perimeter-benphone-bootstrap-driver.txt. It is restricted to these
two old revisions/targets and is not a general replacement for ADR 0087
service preparation. Its first pre-open race was made retryable; the final
copy uses 115200 baud and retains the RAM stub between ROM operations.

## Live BenPhone verification

After Ben enabled the hotspot, an explicit Windows scan found BenPhone on
2.4 GHz channel 6 (WPA2-Personal); the laptop joined it. Skitty's local USB
maintenance request joined at 10.53.247.253. The initial host HTTP request
was sandbox-blocked; an authorized retry read the matching c2ddd6a telemetry
successfully, then `c` restored healthy COMMS on channel 11. No second image
upload was required. Evidence: 20261017T155657Z-F3FD28-benphone-join.jsonl.

Daisy also passed a fresh BenPhone maintenance endpoint check at
10.53.247.228/channel 6, then returned to COMMS/channel 11. No image was
written. Evidence: 20261017T155730Z-F40308-benphone-join.jsonl.

Marill's initial high-speed prefix/full reads stalled before any write.
Its ROM and 115200-baud stub reads matched; NVS confirmed stage 1 and
load_arm=0. Rather than require a redundant full read at the slow rate,
verified its current app1 byte-for-byte against retained 220d90a and saved
a fresh prefix and crash region. Its earlier full autopsy image remains
retained. No new successful full 8 MiB Marill capture is claimed. New
prefix SHA-256: 824d198ffcd373a9c187699c35c94c2f30372bdc3d065dd85e2bdf0410de77b8.
Crash-region SHA-256: 308ae151a618ef479eeb8b124231e13344b34fc417a42fab484a02aa28c82765.
The final retained driver records this Marill-specific preservation path
and uses 115200 baud. Logs retain the earlier failed read attempts too.

## Marill completion

The final 115200-baud operation wrote only app1 and verified both the new
image and unchanged prefix. Its first boot reported the correct revision,
VALID, FULL/stage 1 and healthy sensors through 28.963 s before USB detached.
The next fresh boot explicitly reported deepsleep, last sleep day-charge,
120 s: the earlier detach was normal sleep, not a failed application boot.

On that wake, USB maintenance joined BenPhone at 10.53.247.37/channel 6.
HTTP telemetry verified the exact revision and VALID state at 36.191 s,
FULL/stage 1, healthy sensors, unchanged settings and every historical
last_protect_entry field. Sending c produced the COMMS banner followed by
immediate sleep/USB detach; the host's serial exception is retained in the
log. A separate read-only continuation caught the following natural wake
and confirmed COMMS/channel 11, healthy ESP-NOW, unchanged FULL/stage 1,
50/400/600 mm bounds and original 3027 mV history. No reset was used for
either wake check. The final PASS follows the earlier host exception in
20261017T160044Z-F40348-benphone-join.jsonl.

All three have now used the real hotspot and exposed matching HTTP
maintenance telemetry. This run did not upload a second copy over WiFi;
Daisy's prior actual OTA qualification remains the upload evidence. The
new Skitty/Marill registry rows therefore keep ota_verified=false while
recording their successful access checks. They are back in normal COMMS
with their ordinary sleep policy; no event session or chime was started.

## Shared handoff

Merged Brian's 52058ed (including his ten-device 876a1c8 installation records
and macOS service-helper fixes). This did not replace these three exact
perimeter images. His record still lists canopy hotspot/OTA qualification
as open. The Marill source checkpoint efdce31 is in shared history; its
managed build worktree was archived after exact outputs were copied and
verified. Private backups remain local.

Merged-source checks: seven USB-service host tests pass, packet-layout
tests pass 92 checks, and squad-trigger tests pass 570 checks. The first
isolated squad test link omitted its two core .cpp dependencies; the rerun
with serial_mutation.cpp and strike_event.cpp passed without source changes.
No full rebuild of Brian's merged installation firmware was needed for
these retained perimeter artifacts.
