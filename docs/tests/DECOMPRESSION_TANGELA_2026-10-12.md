# Tangela decompression intake

Date: 2026-10-12 PDT / UTC
Operator: Ben + this Codex laptop; sole USB operator
Target: Tangela, 9F0E5C, D8:85:AC:9F:0E:5C, COM5

## Status

Intake and original ROM state preserved; the original battery is now detected
following substitution and reconnection. Restarting the original image restored
normal charging. Flash, capacity correction and signature are pending the
sensor-cable check. Until ROM backup,
only read-only `t` telemetry requests were used. Origin was up to date at
d79d234 before intake.

## Wiring and USB

Ben reported the enclosure extension initially connected, then moved to
direct PowerFeather USB. No serial device or current Windows USB error device
was observed before the move. Direct USB identified COM5 and the exact target;
one transient port-open failure preceded successful fresh application data.
This is not yet a controlled diagnosis of this extension.

Ben additionally confirmed capboard TELE pin BOOST_EN wired to PowerFeather
D13/GPIO11. Source for the retained decompression artifact does not implement
control of this pin. Connecting it alone does not add boost shutdown behavior;
the existing D7/GPIO37 strike path remains separate.

## Captured application state

- Installed revision: fx-260912-f951ae9-b, valid app1 at 0x340000, no pending OTA.
- Profile FIELD, channel 11, downlight, TMF and MSA reading successfully,
  sensor bits 9, no class mismatch or BQ fault.
- Reported capacity 6000 mAh, charge limit 2000 mA. Physical capacity not yet
  confirmed; no setting changed.
- Initial 17 samples: battery_present=false, charging disabled, battery
  reading 1.116 V declining to 0.462 V over the first captured 20 seconds.
- Follow-up 164 samples, 18:46:01.816389 to 18:49:01.427548 UTC: battery absent
  and charging disabled throughout, no BQ fault; final reported battery 0 V,
  uptime 224334 ms. USB remained usable. The bounded capture and temporary
  host awake hold ended normally.
- Current reported tier FULL / guard stage 1, interrupted flag true.
- Historical PROTECT: 3270 mV, reset-load-armed, predecessor stage 2, reset
  reason code 4, load_armed=1, streak 1, fixture uptime 2077 ms. The historical
  event date is unknown; this is not proof of today's battery condition.
- Last operator storage request: storage-usb, source 8EB508, sequence 200.

## Physical measurements and measurement correction

Ben measured 3.213 V directly at the cell, then 3.73 V between the PowerFeather
VBAT header and GND while USB was connected. The host initially requested the
header measurement as a battery-path check, then explicitly corrected that
interpretation: this header can be charger-powered even with no battery.
The 3.73 V header reading is not evidence of a 3.73 V cell or proof that the
cell is connected to the charger. See the manufacturer's
[power-path explanation](https://docs.powerfeather.dev/#miscellaneous-questions)
and [V2 schematic](https://docs.powerfeather.dev/assets/files/esp32-s3-powerfeather-v2-c8fb0b7f2b084b2013f65c973cfaf223.pdf).

Ben then reported 0 V across BATP/BATN at the board-side battery connector
with the plug seated, and asked whether ship mode could explain it. Ship mode
does not remove an attached cell's voltage from those input contacts; good
external power also exits ship mode. The recorded application was already
running on USB. See [ship-mode behavior](https://docs.powerfeather.dev/sdk/2.x/api/mainboard/).

Together with the 3.213 V cell measurement, this points toward an interruption
between the cell and board input, or a probe-contact/measurement issue. It
does not yet identify a bad crimp, wire, splice, mating contact or socket joint.
Requested next measurement: disconnect the battery lead from the board and
measure the two metal contacts of the free battery-side plug in DC-voltage
mode, keeping the lead attached to the cell. A reading near cell voltage would
focus inspection on the mating/socket path; zero would focus it upstream in
the battery lead/terminations, after confirming good probe contact. No flash,
chime or charging override has been sent.

Ben also obtained 0 V at the unplugged battery-side JST-PH plug, but could not
confirm that the Fluke probe tips reached its recessed contacts. Treat this
as an inconclusive measurement, not proof of an open lead. With that plug
removed, the USB-powered VBAT output still read approximately 3.7 V, consistent
with the documented power path. Do not count a further hardware failure on
this evidence alone.

Next test should avoid probing inside the small contact cavities: use a
matching JST-PH socket/pigtail with separated accessible test leads, or substitute
a known-good LFP battery together with its complete lead. Establish which is
available before coordinating the test. No substitution has yet occurred.

Ben subsequently reported 3.209 V at the accessible battery clips. Clarification
is pending whether the probes touched the clip metal/wire attachments or the
cell terminals underneath. This confirms voltage at the measured points, but
does not yet establish continuity through the full lead and JST connection.

Ben confirmed the probes were on the clips themselves. Cell voltage therefore
reaches the metal clips; wire terminations, wires, JST crimps, mating contacts
and the board-side path remain unverified. Asked whether a known-working
lantern's complete battery-and-lead assembly is available for substitution.

## Substitute battery/lead comparison

Ben connected the battery JST-PH lead from another lantern awaiting service,
with approximately 3.3 V measured at that assembly's clips. This is a comparison
assembly, not a previously qualified known-good battery/lead.

Read-only capture from 19:10:49.818404 through 19:11:48.917324 UTC contains 55
samples on Tangela 9F0E5C. All report battery present, 3.292-3.294 V, no BQ fault,
and charging disabled. Current ranged from -2.0 to +3.2 mA; this is not evidence
of useful charging. There were no transport errors or reboots during capture.
Historical `last_protect_entry*` fields match the earlier intake exactly.

This demonstrates that Tangela can report plausible battery voltage through
its connector with the substitute assembly. It narrows investigation toward
the original assembly or its connection/seating, without yet locating a faulty
part. A controlled return to the original lead is the next test.

The installed September artifact's source at commit
`fcc5ef07ccf65f0f1d034bfe523e9f1d7f68bf10` contains a one-shot charging guard
that can leave charging off for the session after no battery reading for
60 seconds. This is consistent with the observed disabled charging following
battery-absent operation; the guard's triggering message was not captured in
this test. No reset or charge override was sent to investigate it yet.

First sample uptime was 248867 ms, last 307925 ms. Relative to the much earlier
capture, the new boot timing indicates an intervening restart outside capture;
its cause was not observed. The current boot reports `poweron`, DIM, guard
interrupted true and one previous 40 ms strike, with zero solenoid blocks or
failsafes. The host sent no strike command. No new PROTECT record was observed.

A separate read-only watcher was started before asking Ben to reconnect the
original battery/lead while leaving USB connected and pressing neither button.
The capture contains 162 samples, including 18 absent-battery samples during
the swap, then 121 original-battery samples at 3.208-3.211 V from
19:12:39.132800 through 19:14:53.167650 UTC. No transport error or reboot was
observed. The original connection now works at steady low load; the earlier
absence could involve seating or an intermittent connection, but no component
failure is confirmed. Charging remains disabled pending restart.

Ben confirmed Tangela's original cell is 15 Ah. The existing 6000 mAh setting
has not yet been changed; correction will follow original-state preservation.

## Original preservation and charging restart

ROM readback preserved the first 64 KiB (bootloader, partition table, NVS and
OTA selection), the 64 KiB coredump partition, and a retained copy of the
original app1 binary after ROM digest verification against the installed
image. App0 and SPIFFS were not copied and were not written.

Private backup directory:
`ops/bench/private-backups/20261012T191339Z-9F0E5C-pre-charge-restart/`.

- Prefix SHA-256: `507e8cccdab04e81f41937bf00d33f0d8a19f16bcfa566e740d237e7249cf359`.
- Coredump SHA-256: `135d24802af0dcb444507326375f45e6d633bc34212e25f40bc4c9c43b821aa7`.
  It is non-erased; decoding and provenance remain pending.
- Original app1 SHA-256: `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.

Offline whitelisted NVS inspection verified entry CRCs and the PROTECT audit
checksum. Current stage was 2 (DIM), `load_arm=0`, `boots=0`. The historical
3270 mV reset-load-armed record matches telemetry. This inspection did not
expose saved network credentials.

After backup, the host requested a ROM hard reset into the unchanged September
application. Boot reported `reset=unknown`, stage 2, not parked, interrupted
false, and verified VSQT off/on sequencing. With the original battery already
connected, the normal guard enabled charging at about 6 seconds. At 17.7 seconds,
telemetry showed 3.228 V and +382.3 mA with no BQ fault. No charging override or
PROTECT clear was sent. Historical PROTECT fields remained unchanged.

The completed restart capture contains 83 samples from 19:16:33.255364 through
19:18:02.426760 UTC. All 74 samples after 10 seconds of uptime report charging
enabled; voltage was 3.226-3.239 V and current +352.4 to +404.6 mA. Final sample:
3.238 V, +393.8 mA, uptime 91347 ms, DIM, interrupted false. Battery remained
present, no BQ fault or new PROTECT record was observed. The bounded capture
and temporary host awake hold ended normally.

This boot also reported no TMF or MSA sensor, sensor bits 0 and downlight class
mismatch, although both sensors appeared in the first intake. Asked Ben whether
the sensor cable was unplugged or loosened during access. Sensor readiness and
the decompression flash remain pending.

## Retained artifact and evidence

Selected, not yet flashed: fx-261012-5164506-b, 1206816 bytes, SHA-256
`a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`.
Manifest and binary digest verified. No new build was made.

Public raw captures under `ops/bench/data/Bodhi Hive/`:

- `20261012T184256Z-new-lantern-intake.jsonl`
- `20261012T184601Z-9F0E5C-battery-check.jsonl`
- `20261012T191049Z-9F0E5C-substitute-battery.jsonl`
- `20261012T191153Z-9F0E5C-original-reconnect-battery.jsonl`
- `20261012T191456Z-9F0E5C-backup.jsonl`
- `20261012T191628Z-9F0E5C-charge-restart-battery.jsonl`

The first original ROM/NVS/application preservation is complete; no flash yet.
