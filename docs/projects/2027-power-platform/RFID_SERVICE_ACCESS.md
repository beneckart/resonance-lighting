# RFID identification and independent service wake/reset

Date: 2026-09-14

Status: feasibility exploration only. No selected circuit, reader purchase,
firmware implementation, or device action.

## User need and earlier context

Ben's earlier `Postmortem Findings` task asked for battery-free wireless
identification of nonworking fixtures, including dusty canopy lanterns whose
ESP radios were unavailable. He separately needed reset without opening lids.
On September 14 he connected these requirements: could the same long-range
RFID interface reset/wake a fixture from the ground, with an RTC coin cell
supporting the independent electronics? An external button alone still leaves
the problem of reaching a suspended fixture.

The earlier request was recovered from that task's messages. Its separate
23-page report was not retrieved, and no previously selected RFID component
was found in the repository. Do not describe the candidates below as an
earlier agreed design.

## Feasibility

Yes, with an RFID IC that has an external control interface. A normal passive
inventory label supplies identity but cannot reset another board. Selected UHF
RFID devices can decode an addressed RF command and change an output without
ESP32 firmware running. That output can drive an independent pulse/wake circuit.

Conceptual signal flow, not a direct-wiring schematic:

```text
Handheld UHF reader
  -> identify/select exact tag
  -> separate deliberate service command
       |
RFID IC + local pulse/latch circuitry <--- isolated low-power supply
       |                                  from RTC coin cell / main supply
       +-> ESP reset control
       +-> power-enable / ship-wake control
       +-> retained service-request reason

RTC alarm ------------------------------> power-enable control

Main LFP or solar supplies the ESP32 after wake.
```

An ordinary inventory scan must not reset fixtures. A command output should
trigger a bounded pulse and release independently of any second RF command.
Otherwise a lost radio link can leave the processor held in reset. A deliberate
command, exact target selection, command access controls appropriate to the
site, and retry/duplicate behavior belong in the protocol design.

## Concrete IC precedent

EM4325 supports passive or battery-assisted operation and RF-controlled GPIO.
Its RF-writable I/O Word can change outputs without the host MCU. AUX can also
indicate RF events; general inventory/field detection should not directly reset
the installation. A separate GPIO command is the more useful service primitive.
Its listed 1.7 uA typical sleep draw assumes a 12.5% field detector duty cycle
with I/O and monitoring disabled, so it is not the budget for an assembled reset
interface. It is a feasibility candidate, not a selected production component.
[EM4325 datasheet](https://www.emmicroelectronic.com/sites/default/files/products/datasheets/4325-DS%20%28updated%20datasheet%20Feb%202023%29.pdf).

EM's 38 mm coin-cell reference design gives a theoretical North American read
range of 8.9 m for one antenna construction. This supports investigating the
idea, not promising installed read/write/reset range.
[EM coin-cell reference](https://www.emmicroelectronic.com/sites/default/files/products/datasheets/4325-fs2.pdf).

## What wake and reset must mean

| Operation | Independent hardware action | Limitation |
|---|---|---|
| Identify | Read stored tag identity | Does not prove ESP or battery health |
| Wake from MCU sleep | Wake pin or reset pulse | Main supply must be available |
| Reset a hung ESP | Pulse ESP reset/enable | Does not necessarily reset charger, gauge or other powered peripherals |
| Wake from ship/storage | Operate the actual power-enable/ship-wake input | ESP reset alone cannot start an unpowered MCU |
| Hard service power cycle | Interrupt/reapply chosen logic and peripheral rails | Must work with solar/USB present and not depend on the hung ESP |
| Hold for diagnosis | Retain a service request that boot firmware recognizes | Requires a bounded firmware service mode; reset alone may promptly return to sleep |

BQ25628E's QON can wake ship mode; it has no effect in shutdown mode. System
reset behavior with external input also depends on configuration. A redesign
must deliberately cover battery-only and solar/USB-present conditions rather
than assuming a wire to QON solves every case. Level shifting and isolation are
required according to the selected pins and power domains.
[TI BQ25628E datasheet, section 8.3.8](https://www.ti.com/lit/ds/symlink/bq25628e.pdf).

Hardware reset does not erase durable NVS PROTECT in current firmware. A future
service request could open a bounded diagnostic window while keeping art loads
off and retaining guard/history. It is separate from a qualified guard release.
If the LFP is empty and solar absent, the independent interface may still
identify the fixture and accept a request, but cannot make the ESP run.

## Sharing the RTC coin cell

The coin cell can supply the RTC and a very-low-power RFID/reset branch. This
is a board-level supply design, not an assumption that an RTC backup pin is a
power output. Isolate the main and backup supplies, prevent back-powering dead
rails, account for output/pull-up current, and preserve time when reset occurs.
If a primary CR cell is used, block all charge paths from solar/main power.

A Panasonic CR2032 is nominally 225 mAh. For illustrative constant total loads:

| Average backup-domain load | Charge per year | Nominal capacity / load |
|---|---:|---:|
| 2 uA | 17.5 mAh | 12.8 years |
| 5 uA | 43.8 mAh | 5.1 years |
| 10 uA | 87.6 mAh | 2.6 years |
| 20 uA | 175.2 mAh | 1.3 years |

These are arithmetic ceilings, not service-life predictions: temperature,
self-discharge, cutoff voltage, RF traffic, pulse loads, leakage and usable
capacity matter. The 0.2 mA datasheet standard drain is a rating condition,
not a universal maximum current. The coin cell powers control, not ESP Wi-Fi.
[Panasonic CR2032](https://energy.panasonic.com/eu/business/products/lithium/coin-cr-standard/models/CR2032).

An RV-3028-C7's 45 nA typical timekeeping example is about 0.39 mAh/year;
the RFID/control branch would probably dominate the shared budget. Prefer main
power when available if the backup arrangement can do so without defeating
storage or adding comparable leakage. A shared cell also couples RFID failure
or excess consumption to clock retention; qualify and monitor that consequence.
[Micro Crystal RV-3028-C7](https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf).

## Range and identification requirements

Qualify three separate ranges: passive ID with both batteries absent,
battery-assisted ID, and addressed service actuation. Passive fallback on a
battery-assisted IC may have substantially worse sensitivity; do not infer
ground-level access after coin-cell failure from a powered read result. If that
independent ID requirement cannot be met by one IC, a separate high-sensitivity
passive tag is an option.

Test on the assembled lantern with its panel, battery, orientation and sway,
including neighboring tags. Aiming a directional antenna or using RSSI may
help select a physical lantern but does not by itself guarantee uniqueness.
Maintain the tag-to-controller/callsign mapping through board and enclosure
swaps; distinguish physical assembly identity from replaceable controller ID.
Stored diagnostics, if added, must be marked with their update time and validity.

UHF service requires a compatible reader and antenna, not an ordinary phone NFC
tap. A reader accessory could be integrated with the handheld workflow; neither
its compatibility nor budget has been selected. The interface is for deliberate
service, not assumed to replace the primary show-control network.

## OTA acceleration: wake directly into bounded maintenance

Ben identified instant wake as a way to reduce OTA turnaround on 2026-09-14.
It can remove most sleep-cadence waiting and repeated capture of stragglers.
It does not speed compilation, image transfer or the post-boot verification
window. The current runbook budgets about 2.5 minutes gather/association,
7-9 minutes parallel upload, and 30-45 seconds verification/cleanup for roughly
130 healthy ordinary-cadence fixtures. These are planning targets, not a
proven result for the September 12 storage cohort. PROTECT rendezvous can
require roughly 15 minutes and sits outside that ordinary target.
[OTA runbook](../../howto/FLEET_OTA_10_MINUTE_RUNBOOK.md).

The desired feature is a retained, bounded 'wake for maintenance' request,
not merely an ESP reset. On boot, park art loads, identify the requested
maintenance session, join the known maintenance network and remain available
through upload/reboot verification, subject to qualified power and an expiry.
A plain reset preserves PROTECT and may soon return to sleep. Recovery from
unavailable power or a broken Wi-Fi join remains a separate problem.

Wake should not reset an already uploading fixture. Define duplicate requests,
job identity and cancellation, and preserve the existing fresh-revision and
pending-verify acceptance gates. For fleet OTA, measure coverage and group or
rapid roster wake: a reader that requires visiting every lantern may spend
much of the time saved. A separate trigger radio is another candidate for
this wake function; the primary firmware image can continue over shared Wi-Fi.

Longer-term pre-staging of a verified image can also remove transfer time
from the activation window; this is distinct from instant wake and remains
unimplemented. See [future OTA options](../../design/FLEET_OTA_FUTURE_SPEED_OPTIONS.md).

## Small proof before a board decision

1. Confirm one candidate's GPIO control, passive fallback, availability, reader
   command access and complete idle/RF-active energy budget.
2. With the ESP hung, asleep, and fully unpowered in separate tests, identify
   and command only that fixture while neighboring fixtures remain untouched.
3. Verify pulse release if the reader disappears mid-command; test retries,
   repeated inventory and loss of main/coin-cell power.
4. Demonstrate reset and ship wake with battery-only and solar/USB present;
   verify the retained service reason and bounded radio window on boot.
5. Repeat at actual hanging height and unfavorable orientations; measure ID
   and actuation success separately, including passive ID without either battery.
6. Verify protection/history remain intact, LFP charging stays correct through
   reset, and the coin cell never sources the ESP rail or receives charge.

Promote the concept only after it proves useful ground-level service access.
