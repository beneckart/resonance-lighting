# Two Atom buttons: direct signature chimes

Date: October 17, 2026 (PDT).
Operator: Ben + Codex, sole USB writer for the two targets on this laptop.
Status: flashing in progress; final requested mode is direct signature chimes.

## Request and targets

Ben requested Brian's newest Atom firmware on two connected Atoms. Both are
ESP32-PICO-D4 revision 1.1, with 4 MiB flash:

| Target | Full MAC | Windows port |
| --- | --- | --- |
| 187BA4 | F8:B3:B7:18:7B:A4 | COM6 |
| 1B5D88 | F8:B3:B7:1B:5D:88 | COM12 |

The second port appeared after the initial enumeration. Ben confirmed both
were connected. Initial passive COM6 output was repeated `mode: 0`, without
a Resonance version or MAC; ROM identification established each exact MAC.
The old application headers identify IDF v4.4.7, but do not establish which
product firmware they were running. The applications differ between boards.

## Firmware and intended behavior

Fetched and fast-forwarded origin through
7ac2e0cfb1e9e2f3afb9569c668884364e5b87a9. Its latest Atom feature change is
Brian's c659841 T-Deck mode, following the double/triple-click fixes.
Built the existing atom_decompression wrapper into the fresh retained path
firmware/atom_decompression/build/atom-decomp-7ac2e0c-r2/.

- Revision: atom-decomp-7ac2e0c.
- Binary: 918,512 bytes; SHA-256
  c447311f8ca059a545e14ce99348640d03a1f390a9c19d2f727c6dc79f35a801.
- ESP32 Arduino 3.3.7; board m5stack_atom; mesh channel 11.
- Native radio/roster/button test: 160 checks, zero failures.
- Embedded build passed: 46% reported program space, 14% static RAM.
- Public manifest: ops/bench/data/Oakland/atom-decomp-7ac2e0c-manifest.json.

The first build invocation could not access the installed Arduino toolchain
from the filesystem sandbox. It stopped before compiling. A fresh r2 build
with toolchain access completed; the failed invocation's log is preserved.

Ben clarified that a press should trigger Freakout Mode and asked about
single/double/triple clicks. These are T-Deck show rules, not patterns stored
on the Atom. The saved Brian bench show currently assigns single click to
Freakout Mode twice then return to prior playback, double click to the next
Active pattern, and no rule to triple click. The initial plan was to persist
T-Deck mode, which self-addresses their click packets for T-Deck processing
instead of directly requesting each lantern's signature. Listening leases
continue in this mode. A powered T-Deck running the show rules is required.

## Preservation and transfer constraints

Both original 64 KiB boot/settings/partition prefixes were saved by ROM
reads. Their existing partition tables have one 3 MiB app0 at 0x10000 and
SPIFFS at 0x310000. The generated min_spiffs partition table is not deployed;
the new application fits the existing app0, and the old table is retained.

Full-flash stub reads were unreliable: 460800 baud failed on COM6, and
115200 baud bulk reads stopped near 200 KiB on both boards. Short reads were
intermittently successful. Subsequent 57600-baud bulk application reads also
stopped: near 807 KiB on 187BA4 and 565 KiB on 1B5D88. The tools did not save
these incomplete bulk reads. Failed reads did not write flash. Private data lives under
ops/bench/private-backups/20261017-<target>-before-decompression/.

Public backup traces are ops/bench/data/Oakland/20261017-<target>-atom-backup.jsonl.
No full-flash preservation is claimed. Deployment is app0-only, preceded by
exact ROM MAC and old-application/prefix verification. The T-Deck mode setting
is a separate intentional NVS write after new-firmware identity is confirmed.

The original applications are 1,224,880 bytes (187BA4) and 2,175,968 bytes
(1B5D88), determined from their image segment headers. Saved coverage is the
first 64 KiB of each application, plus three more consecutive 64 KiB chunks
on 1B5D88. Both complete boot/settings prefixes are saved. These are partial
backups and cannot independently restore the full old applications. Ben was
asked whether to proceed without complete old-application backups or continue
preservation. At that checkpoint, no flash or T-Deck-mode configuration had
been sent.

Ben then explicitly replied, "Proceed; old firmware isn't needed." Deployment
proceeds with the saved partial backups, verifying the old prefix and saved
application region against each exact target before writing. Complete old
application restoration is not promised.

## Final scope: single press requests a chime

Ben subsequently said mode changes can stay on the T-Deck and that single
click equals chime is sufficient. Both Atoms therefore use the existing
direct signature mode (`tdeck_mode=false`). A press requests each of the
nine named lanterns' locally timed seven-hit signature. A receiver that is
in Tone Lab relay mode can instead hand the request to its host; Atom radio
transmission alone does not prove an audible chime.

## Deferred settings persistence investigation

187BA4 accepted `tdeck 1` in RAM but returned to `tdeck_mode=false` after
reset. Readback contained the atomdecomp namespace without a tdeck key.
Erasing/reinitializing just its 20 KiB NVS partition did not fix this. Its
original NVS is saved in the original 64 KiB prefix, but current settings
are not claimed to be preserved on this board. The original application,
bootloader, and partition identities were checked before the app-only write;
the bootloader and partition table were not replaced.

A checked-save source candidate was committed as 7b61297 and built as
atom-decomp-7b61297 (919,712 bytes, SHA-256
51aeacc230d35e068eecb5f9bf17543a5cb7582539e09b90911c131753491866).
It was never flashed or hardware-qualified. After Ben simplified the scope,
f4bf147 reverted that candidate; the two deployed applications remain the
unmodified Brian-source atom-decomp-7ac2e0c artifact. The persistence cause
is unresolved, not established as corrupt old settings.

Evidence: Oakland/20261017T174351Z-187BA4-atom-flash.jsonl,
20261017-187BA4-atom-nvs-diagnostic.jsonl, and
20261017T174826Z-187BA4-nvs-repair.jsonl. The FAILED entries in the first
and third files are the persistence gate, not a failed application write.

## Application write and first functional result

Both targets passed exact-MAC verification, app0 digest verification, and
post-reset boot observation running atom-decomp-7ac2e0c in direct signature
mode. 1B5D88 also passed verification that its original 64 KiB prefix was
unchanged by the application write. Both reported radio=true and zero RX
queue drops. These boot checks do not imply that every receiver accepted
the Atom yet.

At 10:55 PDT, 187BA4 had fresh READY from all nine receivers. Exactly one
USB `play` request (the same play handler as a face press) produced COMPLETE
with seven hits from each receiver. Local strike scheduling lateness ranged
from 65 to 360 us; this is not a measurement of between-lantern start skew.
All receivers then reported fx-261017-e39594d-b. Evidence:
Oakland/20261017T175436Z-187BA4-signature-check.jsonl and
20261017T175520Z-187BA4-signature-check.jsonl.

Initially both controllers received STALE session responses. The first
later became ready; the second still showed 0/9 READY at its 30-second boot
gate. STALE does not distinguish old sequence numbers from a full receiver
replay table. The source has eight source/boot entries and can reuse retired
entries after ten minutes. This is a candidate explanation, not a captured
root cause. No receiver reset or replay-table mutation was requested here.
