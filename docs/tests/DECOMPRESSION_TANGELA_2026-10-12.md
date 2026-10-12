# Tangela decompression service

Date: 2026-10-12 PDT / UTC
Operator: Ben + this Codex laptop; sole USB operator
Target: Tangela, 9F0E5C, D8:85:AC:9F:0E:5C, COM5

## Status

Exact decompression artifact flashed and verified in app1, capacity corrected
to the confirmed 15 Ah cell, both sensors restored after cable reconnection,
and one seven-hit signature completed. Original battery detection returned
after reconnection; normal charging resumed after restarting with the battery
attached. Saved PROTECT history is unchanged. The 60-second post-signature
check passed, ending at 3.254 V and +405.8 mA with both sensors healthy.
The subsequent direct/extension/direct comparison isolated a USB communication
failure to the enclosure-extension path; Tangela stayed awake throughout.
Ben then visually identified heavy corrosion at the extension's outer,
non-PowerFeather end. Replace the extension and retain it as autopsy evidence.
Origin was up to date at d79d234 before intake.

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

Ben subsequently confirmed he had disconnected the sensor cable to perform
the donor-battery test and reported it reconnected. The missing sensors in that
boot therefore do not establish a hardware failure. The next firmware boot
will re-probe them; no class override is being applied to hide the mismatch.

## Decompression update and signature

With charging established, took a second pre-flash backup under
`ops/bench/private-backups/20261012T191647Z-9F0E5C-service/`.
Its prefix SHA-256 is
`c1e0462c534ce3f5811d0833679e75cc37444b920a9f384fde709ae64cd7d1f2`;
the coredump digest is identical to the first backup. The fresh power check
was 3.245 V, +418.8 mA, DIM, interrupted false, both load gates off.

Wrote only app1 at 0x340000. ROM verification confirmed the exact new binary
and unchanged pre-flash prefix at 19:21:35.855023 UTC. Then sent the explicit
target capacity command `C9F0E5C:15000`, verified 15000 mAh after its software
restart, and opened a 60-minute RAM-only event session with `!D9F0E5C:60` at
19:21:42.411685 UTC (12:21:42 PDT). The session is scheduled to expire at about
13:21:42 PDT unless restarted or renewed. FIELD/channel 11 and 2000 mA charge
limit remain unchanged. This artifact is USB-verified here, not OTA-qualified.

After over 30 seconds of healthy uptime with TMF ranging and MSA present,
sensor bits 9, no class mismatch, charging enabled and unchanged PROTECT
history, sent exactly one `!K9F0E5C:1` at 19:22:09.254298 UTC.
Completion at 19:22:12.927085 UTC reported:

- Seven strikes / seven signature hits, state COMPLETE (2).
- Maximum lateness 61 us; offsets 58, 500061, 750059, 1000059, 1500056,
  2500056 and 3000056 us.
- Solenoid gate off, zero blocks and zero failsafes.
- No unexpected reboot or changed historical PROTECT record.

The captured voltage minimum during this acceptance run was 3.156 V;
completion showed 3.158 V and -1727.1 mA while the strike/boost load was active.
The follow-up captured recovery to 3.252 V and +428.9 mA by uptime 43.2 seconds,
with the event LED rail on. This is a transient load response, not a new
PROTECT event. Human confirmation of the audible cadence was requested.

Post-service acceptance captured 55 samples from 19:22:22.210861 through
19:23:21.926310 UTC. All showed the expected revision, 15 Ah setting,
TMF ranging and MSA present, no class mismatch, charging enabled and battery
present, DIM with interrupted false, unchanged PROTECT history, seven total
strikes, completed signature and zero blocks/failsafes. Voltage ranged
3.248-3.258 V and charge current +299.6 to +446.0 mA. Final reading was
3.254 V, +405.8 mA, uptime 102907 ms, event session 3501 seconds remaining.
There was no transport error or unexpected reboot. Capture and temporary
host awake hold ended normally; the host commissioning checks passed.

## Solenoid saddle retention test

After the commissioning signature, Ben reported missing solenoid mounting
screws and observed that the printed saddle's friction fit did not prevent
kickback. He fitted and hand-tightened one screw, then requested several
patterns to check whether it held. This is a mechanical finding, separate
from the successful electrical/firmware acceptance.

Sent three additional `!K9F0E5C:1` commands at 19:26:06, 19:26:19 and
19:26:33 UTC, allowing 10 seconds after each completed pattern before the
next. All three completed seven hits (21 additional strikes; 28 total in
this boot). Maximum lateness was 63, 62 and 59 us respectively. No blocks,
failsafes, reboot, charger fault or changed PROTECT record occurred.

After the last pattern and 10 seconds of settling, telemetry reported
3.252 V and +412.0 mA charging at uptime 307951 ms. Ben confirmed the solenoid
"Stayed firmly in place" through the test. The short retention test therefore
passed with the single hand-tightened screw. Final installation fastening
and longer-term retention are not established by this short test.

## Retained artifact and evidence

### Subsequent enclosure-extension check

Ben confirmed all preceding service and mounting tests used direct USB,
suspected this enclosure extension too, and reported switching to it. Started
a three-minute read-only USB watcher at 19:28:34 UTC. No serial port appeared
and no telemetry sample was obtained. Two contemporaneous Windows PnP checks
also showed neither a present PowerFeather device nor a USB error device;
there was no observed Code 43 in these checks.

Confirmation that the extension was fully connected had been requested but
had not arrived when the watch ended. This is therefore a no-enumeration
observation, not yet a controlled failure diagnosis. A return-to-direct check
without pressing buttons remains to be done once connection state is clear.
Last direct sample was 19:26:46.965874 UTC, uptime 307951 ms, 3296 seconds
remaining in the event session, 28 strikes, unchanged PROTECT and 3.252 V.
Those values are retained for the next uptime/history comparison. No reset,
reflash, power/profile change or further strike was sent during this check.

Ben then reported trying all four orientation combinations without a
successful Windows USB connection sound. Individual orientation transitions
were not timestamped in the host trace. Started the return-to-direct watcher
and requested the same laptop cable/port, battery left connected and no button
presses, to separate an extension-path failure from a lantern reboot/sleep.
Ben confirmed the return to direct USB. COM5 then supplied 56 samples over
61.07 seconds, from 19:33:18.511977 through 19:34:19.584902 UTC, with no
transport error, telemetry gap or reboot. The first returned sample was
391.546 seconds after the last direct baseline; MCU uptime advanced 391.528
seconds (18 ms difference). The original event session remained active and
all historical PROTECT fields were unchanged. Thus a reset or deep-sleep
cycle does not explain the missing USB connection during the extension test.

The direct/extension/direct comparison isolates the failure to the extension
assembly or its mating contacts. Treat this extension as failed for service
and retain it for inspection; the electrical comparison alone did not
distinguish corrosion, internal wiring damage or contact seating. The four unsuccessful
orientations were reported by Ben, not separately instrumented host phases.

Final direct sample: 3.256 V, +354.6 mA charging, DIM, interrupted false,
TMF/MSA healthy, no class mismatch/BQ fault, 2844 seconds left in the event
session. The strike counter was 35 on return, versus 28 in the older direct
baseline; the source/timing of those seven intervening strikes was not
observed. This USB test sent only telemetry queries, with no strike or reset
command. Capture and temporary host awake hold ended normally.

### Visual corrosion finding

Ben subsequently reported clear, heavy corrosion on visual inspection of
the non-PowerFeather end of this extension, corresponding to the outer
connector. This is a direct user-reported physical observation, rather than
the earlier corrosion hypothesis; no inspection photograph has been reviewed
by the assistant. Together with the failed extension test and successful
direct return, the observation strongly supports connector corrosion as a
cause of the USB failure. The exact affected contacts and any additional
internal damage have not been isolated.

Replace the extension for service. Retain/label the failed assembly and
photograph the outer connector, internal plug and cap/seal before cleaning.
No replacement, cleaning or post-cleaning retest is yet recorded. The route
by which moisture reached or remained at the connector, including the
earlier cap-trapped-moisture hypothesis, is still unproven. This observation
applies to Tangela's extension; it does not establish corrosion on the other
suspect extensions without inspection.

### Files

Flashed and verified: fx-261012-5164506-b, 1206816 bytes, SHA-256
`a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`.
Manifest and binary digest verified. No new build was made.

Public raw captures under `ops/bench/data/Bodhi Hive/`:

- `20261012T184256Z-new-lantern-intake.jsonl`
- `20261012T184601Z-9F0E5C-battery-check.jsonl`
- `20261012T191049Z-9F0E5C-substitute-battery.jsonl`
- `20261012T191153Z-9F0E5C-original-reconnect-battery.jsonl`
- `20261012T191456Z-9F0E5C-backup.jsonl`
- `20261012T191628Z-9F0E5C-charge-restart-battery.jsonl`
- `20261012T191849Z-9F0E5C-backup.jsonl`
- `20261012T192021Z-9F0E5C-flash.jsonl`
- `20261012T192222Z-9F0E5C-post-service-battery.jsonl`
- `20261012T192606Z-9F0E5C-mount-test.jsonl`
- `20261012T192834Z-9F0E5C-extension-check.jsonl`
- `20261012T193228Z-9F0E5C-direct-return-check.jsonl`

Both private original-state backups are retained. The original battery
connection now supports charging and the signature load, but the earlier
absence does not identify a failed component. If it recurs, inspect seating,
retention and crimps before replacing parts. The enclosure extension was not
usable in the subsequent comparison and should be retained for inspection.
D13 boost control and offline crash decoding remain separate follow-up work.
