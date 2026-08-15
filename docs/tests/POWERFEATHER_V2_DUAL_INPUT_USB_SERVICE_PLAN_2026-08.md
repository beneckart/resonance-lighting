# PowerFeather V2 VUSB + VDC hot-plug and USB-service qualification

**Date:** 2026-08-06
**Status:** P0 OPEN -- required before deployed-enclosure USB service is trusted
**Owners:** Ben + Codex

## Why this test exists

Two PowerFeather V2 boards (`F402F4` and `F402B4`) developed invalid-header boot
loops and unreadable/invalid JEDEC flash responses during the rev-1 capbank bench
setup. Both failures occurred before an energized 12 V boost was attached, so the
boosted cap rail is not an established cause. The repeated condition of concern was
the PowerFeather receiving VDC from one powered USB-hub branch while its native USB-C
VUSB/data port was connected to the same hub.

This matters in production. The enclosure intentionally exposes native USB for a
reflash or recharge rescue, while a solar panel may still be hot on VDC.

The PowerFeather design is supposed to support this condition. Its official
documentation says VUSB and VDC are Schottky-ORed and safe simultaneously; the
higher source supplies VS and similar voltages share the load. The V2 schematic
shows separate CUHS20F30 Schottky diodes from VBUS and VDC into VS. Therefore a
normal dual-input connection is not, by itself, a sufficient explanation for two
failed boards. The bench must identify an abnormal cable, connector, sequencing,
rail-transient, large-capacitor, hub, or board condition rather than labeling the
event a generic ground loop.

The official hardware notes also warn that some header holes sit very close to the
ESP32-S3 module pads, with VDC singled out as potentially destructive if header solder
bridges to a neighboring module pad. It is worth a quick Phase-0 microscope/resistance
check, but it is not a leading explanation here: `F402B4` passed sustained VDC use,
flash preflight/upload, and an unboosted pulse before failing, and the bench subsequently
ran many successful VDC-powered tests. Only an intermittent or mechanically sensitive
header defect would fit that chronology, and the same rare defect would have to explain
two boards.

Official references:

- PowerFeather simultaneous-input behavior:
  https://docs.powerfeather.dev/#miscellaneous-questions
- PowerFeather V2 schematic:
  https://docs.powerfeather.dev/assets/files/esp32-s3-powerfeather-v2-c8fb0b7f2b084b2013f65c973cfaf223.pdf

## Results to date -- 2026-08-06 static service reproduction

Known-good `F3FD7C` passed the staged static reproduction through the complete
rev-1 harness:

- battery attached; 5.0 V and 5.8 V current-limited VDC;
- VDC-first and VUSB-first insertion orders;
- same-hub VDC+VUSB and separate bench-supply VDC;
- three-pin Y harness and external boost;
- charged 59,000 uF bank at approximately 12.17 V;
- D7, swapped bench telemetry VSNS->A5 and D7S->A4;
- native USB enumeration and read-only ROM/stub reset transactions.

The boost plus capboard alone settled at 0.17 W / 29 mA from 5.8 V. A sustained
approximately 3 W reading with the PowerFeather attached was battery charging,
not capboard loss: telemetry showed approximately 489-496 mA battery charge and
410-412 mA VDC input after cap inrush ended. A 500 mA source limit made the boost
startup and charger compete and stalled the bank; a 1 A limit charged the bank in
approximately 2-3 seconds.

Fully connected idle telemetry measured VSNS 3.059 V (12.329 V calculated bank),
D7S 0 V, and gate OFF. Twenty consecutive native-USB reset/flash-ID transactions
passed 20/20 with JEDEC `20:4017`, 8 MB, and normal application rejoin. Ben observed
no strike, driver signal LED, heat, or abnormal supply behavior.

This is strong negative evidence against the documented dual-input topology,
shared USB-hub ground, capboard power, static D7/telemetry wiring, and USB reset
sequence as sufficient causes. P0 remains open because this run did not capture
high-bandwidth rail/EN transients, use an actual illuminated panel, complete 50
physical service-cable insertion cycles, or perform Phase-0 forensics on the two
failed boards.

The concurrent Cambium integration was also checked. Cambium has no fixture OTA
uploader and the review task flashed only selected perimeter `F3FD88`; no logged
OTA targets either failed board. Even if a separate application OTA had been cut,
it would normally affect the inactive app partition, not the raw JEDEC identity.
The dead-board no-stub JEDEC and preserved flash dump therefore remain decisive.

## Interim field-service rule

Until this plan passes:

1. Do not connect powered USB VBUS while a fixture has live VDC/panel input.
2. For serial, flashing, and diagnostics on a VDC- or battery-powered fixture, use a
   verified data-only USB cable or inline VBUS blocker. USB D+/D- and GND remain
   connected; only VBUS is opened.
3. If USB must power or recharge the fixture, disconnect or fully shade the panel/VDC
   input first, then connect ordinary powered USB.
4. Do not hot-swap the battery. Keep the correctly polarized production cell attached
   unless a test step explicitly powers the entire assembly down first.
5. Make the service adapter visibly switchable/labeled: `DATA ONLY` should be its
   default; `USB POWER` should require a deliberate action.

This is a conservative containment rule, not a claim that the documented
PowerFeather dual-input topology is defective.

## Important measurement distinction

The exposed `3V3` header is downstream of the AP22916 load switch. A board that cannot
boot firmware can legitimately show 0 V there even while its internal ESP32 rail is
alive. Failure analysis must measure internal `+3.3VP` (schematic TP7 / regulator
output), ESP `EN`/reset, and the flash response. Do not use header `3V3 = 0 V` alone as
proof of a dead TPS631013 or ESP32 rail.

Likewise, an invalid application header can result from corrupted flash contents, but
an invalid JEDEC manufacturer/device response after a stable power cycle is not an
ordinary image-corruption signature. It points toward unstable flash power/signals,
module damage, or a flash device that is not responding normally.

## Phase 0 -- preserve and examine the two quarantined boards

Do not erase or write either board until their failure signatures are recorded.

1. Photograph board revision, header soldering, connector orientation, and the exact
   cables/adapters used when each board failed.
   Inspect the VDC header joint and every adjacent WROOM pad under magnification; compare
   clearance and resistance with healthy `F3FD7C` before applying power.
2. With all power removed, compare `F402F4`, `F402B4`, and healthy `F3FD7C`:
   - resistance from VUSB, VDC, VS, `+3.3VP`, header 3V3, D7, A4, and A5 to GND;
   - diode-mode readings through each VUSB/VDC Schottky path;
   - continuity/polarity of every custom Y and USB-to-XH cable, including a check for
     one-pin-offset insertion and shield-to-GND behavior.
3. Power each failed board from one input at a time with a current-limited 5 V bench
   source. Start low and raise the limit only enough for ROM/download-mode operation.
   Record input current and locate any hot component with a thermal camera or other
   non-contact method.
4. Measure VUSB/VDC, VS, internal `+3.3VP`, header 3V3, ESP EN/reset, and USB-port
   behavior. Compare with the healthy board under the same mode.
5. Run read-only ROM diagnostics with the normal stub disabled where possible:
   `chip_id`, `flash_id`, and the exact raw JEDEC bytes. If flash ID becomes valid,
   read and hash the existing flash before any write attempt.
6. Classify each failure before repair:
   - valid JEDEC + bad image/header -> content corruption is plausible;
   - invalid JEDEC with stable `+3.3VP` and working ROM -> WROOM flash/interface fault;
   - unstable or missing `+3.3VP` -> regulator/enable/power-path fault;
   - abnormal current/hotspot -> isolate the damaged component first.

## Phase 1 -- protected dual-input test without capboard hardware

Use a known-good expendable bench PowerFeather, not a commissioned fixture. Attach a
correct production LFP cell before power-up. Use an inline USB breakout with separately
switchable/measurable VBUS and intact D+/D-/GND. Current-limit both power inputs.

Instrument at least:

- USB-side VBUS before its Schottky;
- VDC before its Schottky;
- common VS after the Schottkys;
- internal `+3.3VP`;
- input current in both branches;
- ESP EN/reset and reset reason.

Use a differential probe or two-channel subtraction for ground-bounce measurements.
Never float a protective-earth oscilloscope or clip a grounded probe to a non-GND
node.

Run these sequences first as power/heartbeat tests, not during a flash write:

1. VDC only.
2. Powered native USB only.
3. VDC first, then data-only USB.
4. VDC first, then powered USB.
5. Powered USB first, then VDC.
6. Both 5 V inputs from separate current-limited sources.
7. Both 5 V inputs from the same powered hub and the exact field cables.
8. Actual panel-like VDC above VUSB, then native powered USB.
9. Actual panel source through sun/shade transitions while USB remains attached.

Capture insertion/removal transients for every sequence. Only after the rails are
shown stable should each sequence add read-only flash ID/hash operations and then a
controlled application upload.

## Phase 2 -- reintroduce the fixture harness one variable at a time

Repeat the passing Phase-1 sequences while adding, in this order:

1. the three-pin Y harness with its unused branches open;
2. the unpowered capboard branch;
3. the direct 5 V capbank, discharged before connection;
4. the external boost input with its output disconnected;
5. the charged boosted capbank with no strike command;
6. A4/A5 telemetry;
7. the solenoid driver and bounded strike, only after static dual-input service has
   passed.

This ordering separates the documented PowerFeather input behavior from large
capacitance, converter inrush/noise, signal backfeed, and actuation transients.

## Minimum pass criteria

- No unexpected reset, EN chatter, rail collapse, damaging overvoltage, or abnormal
  hotspot in any insertion/removal sequence.
- Valid JEDEC ID and identical flash hash before/after every test block.
- At least 50 cycles of the actual hot-panel -> USB-service sequence with no failure.
- Data-only/VBUS-blocked native USB flashing works repeatedly while VDC and battery
  power the fixture.
- Powered-USB rescue works repeatedly after VDC is intentionally removed.
- The same result holds with the actual enclosure service cable and panel harness.

## Deliverables

- Scope captures and current logs for each input sequence.
- Read-only forensic record for `F402F4` and `F402B4`.
- A final root-cause statement or, if the failure cannot be reproduced, a bounded
  service-interface rule backed by the 50-cycle qualification.
- Decision on a production VBUS-blocking/switching service adapter and enclosure label.
