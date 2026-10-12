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
