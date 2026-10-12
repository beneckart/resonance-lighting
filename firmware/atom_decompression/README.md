# Decompression Atom button

An Atom Matrix with a pressable 5x5 face controls the nine-lantern decompression
squad. One press requests the existing local seven-hit signature on every
named lantern, day or night. It never sends the individual beats over radio.

This requires the receiver update in [ADR 0083](../../docs/decisions/0083-decompression-radio-button.md).
The initial `fx-261012-5164506-b` and `fx-261012-5d7ab8f-b` images ignore the
new command. A heartbeat from those images is not a READY acknowledgement.

## Operation

Keep the Atom powered during the event. It renews short listening leases on
channel 11. Allow at least one sleep/wake cycle for sleeping lanterns to join.
The initial firmware normally sleeps for two minutes between listening windows.
Ship-mode units need external power or their supported physical wake first.

The central 3x3 LEDs, left-to-right then top-to-bottom, represent:

| Ness | Eevee | Rikku |
| --- | --- | --- |
| Leeloo | Bidoof | Pinsir |
| Tangela | Sakura | Lucas |

- Blue: no fresh response.
- Amber: heard, but no fresh lease acknowledgement (including old firmware).
- Green: receiver accepted a listening lease recently.
- White after a press: phrase started.
- Gold after a press: receiver reports the seven-hit phrase complete.
- Red after a press: refused, aborted or stale command.
- Purple: controller paused. All red: radio setup failed.

Release between presses. A five-second interval prevents overlapping requests.
Holding the face never repeats. A missing/refusing lantern will not play later
as a delayed consequence of an earlier press. The Atom attempts all nine on
each press; its display and serial receipts show partial responses explicitly.

Unplugging the Atom stops renewal. Radio leases expire within five minutes;
an independent USB/USER session can keep a lantern awake longer. The receiver's
power protections can refuse chimes. All-hours behavior does not bypass them.

USB at 115200 baud accepts newline-terminated `t` (status), `play` (same as
face press), `pause` (stop renewal/release radio leases), and `resume`. There is
no arbitrary target or configuration command.

## Build and validation

Commit source, then build in a fresh directory:

```sh
bash build.sh --build-path build/<unique-reviewed-name>
```

The wrapper records the commit-derived revision, exact SHA-256, channel and
roster in `manifest.json`. Inspect it and `build.options.json` before exact-
target flashing. Preserve the original board image/configuration first.

Native tests are part of `firmware/fixture/tests/run_tests.sh`, including
packet layout, exact-target validation, replay ordering and button edge cases.
Hardware validation remains separately recorded; neither a broadcast send
callback nor an old-image heartbeat proves a phrase played.

## October 12 bench checkpoint

Atom Matrix `54B0C8` / `14:08:08:54:B0:C8`, COM6, now runs
`atom-decomp-103c91d`, built from commit
`103c91d3be3f28f743b882c32e6c739130ff201e`.
Retained directory: `build/atom-decompression-20261012-r2/`.
Application: 915728 bytes, SHA-256
`44c89c09a7428e4bcc9d154c509890acd0ba451a228b9c3d94b28edcf00fefe3`.

ROM verified app0 and the unchanged original 64 KiB prefix. The original
September conductor app0 was digest-verified and preserved with that prefix
under `ops/bench/private-backups/20261012-54B0C8-before-decompression/`.
Full-flash stub reads failed; app1, SPIFFS and coredump were not collected or
written. This is partial preservation, not a full-flash backup.

The new Atom passed 30 seconds of live serial/radio operation with the exact
revision and nine-member roster, zero presses and zero RX queue drops. It
heard the old Ness and Tangela revisions; no receiver acknowledged the new
protocol. No PLAY was sent. All native fixture tests passed, including 135
radio/roster/button checks, 81 packet-layout checks and 58 local-sequence checks.

Ben paused the work to pack the lanterns and continue in his Oakland backyard.
The Atom accepted `pause` and reported `enabled=false` before packing.
Pause is RAM-only: reconnecting power boots into lease renewal again, without
playing a phrase. No lantern receiver was updated in this session. First
end-to-end button test, nine-lantern rollout, idle/expiry and runtime tests
remain pending. There is no need to locate a particular lantern while packing.

The unfinished receiver build was stopped and its managed worktree archived.
Attempted revision `fx-261012-e8654a8-b` is incomplete and must not be used or
resumed. Start a fresh immutable receiver build before the Oakland tests.

Public evidence:
`ops/bench/data/Bodhi Hive/20261012T195443Z-54B0C8-atom-decompression-flash.jsonl`
and `20261012T195801Z-54B0C8-packing-pause.jsonl`.
