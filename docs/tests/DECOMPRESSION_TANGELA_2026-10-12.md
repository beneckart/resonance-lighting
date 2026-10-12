# Tangela decompression intake

Date: 2026-10-12 PDT / UTC
Operator: Ben + this Codex laptop; sole USB operator
Target: Tangela, 9F0E5C, D8:85:AC:9F:0E:5C, COM5

## Status

Intake preserved; flash and signature deferred pending battery-path diagnosis.
No fixture configuration, flash, reset or chime command has been sent by the
host. Only read-only `t` telemetry requests were used. Origin was up to date
at d79d234 before intake.

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

## Retained artifact and evidence

Selected, not yet flashed: fx-261012-5164506-b, 1206816 bytes, SHA-256
`a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`.
Manifest and binary digest verified. No new build was made.

Public raw captures under `ops/bench/data/Bodhi Hive/`:

- `20261012T184256Z-new-lantern-intake.jsonl`
- `20261012T184601Z-9F0E5C-battery-check.jsonl`

No ROM/NVS or application backup has yet been made for this target.
