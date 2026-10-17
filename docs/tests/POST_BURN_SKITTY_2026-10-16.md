# Skitty post-burn intake and first USB attachment -- 2026-10-16

## Specimen and method

- Skitty, F3FD28, full MAC 68:EE:8F:F3:FD:28, identified from USB serial
  and application telemetry. Callsign cross-checked against the fleet registry.
- Reported firmware: fx-260831-b3e2738-b; FIELD profile, perimeter class,
  Generic_LFP, configured capacity 6000 mAh. Physical capacity was not checked
  during this intake. This is an August image, not Daisy's September image.
- Ben reports sunlight exposure before this test. Exposure duration and net
  harvested energy are not measured.
- Windows USB ETW and read-only serial capture were armed at 15:40:53 PDT
  before Ben connected the existing enclosure extension. No reset, cable
  substitution, protection clear, firmware change or service hold was issued.
- Serial capture uses no-reset port opening and only `t` telemetry queries.
  ETW captures USBHUB3/UCX/USBXHCI Default + StateMachine events, without
  transfer-payload keywords. Capture has a 20-minute bound.

## Timeline (PDT)

| Time | Observation |
| --- | --- |
| After connection; report recorded 15:44 | Ben measures about 2.7 V at battery clips; only red charger LED lit; no Windows connection sound. Subsequently confirms red LED steady. |
| 15:51:02.592 | Windows begins USB enumeration on port 1. |
| 15:51:02.788 | Enumeration succeeds, status 0, about 196 ms after start. |
| 15:51:03.646 | COM9 appears with Skitty's USB serial identity; capture opens without reset. |
| 15:51:04.695 | First application sample: deepsleep reset, uptime 2165 ms, battery 2.885 V, PROTECT, prior sleep 900 s. |
| 15:51:08.679 | Application verifies 300 mA precharge readback and logs battery present, charging ON. |
| 15:51:10.815 | Charging enabled, battery 2.885 V, supply 4.617 V / 324 mA, supply good, charger fault register 0. |
| 15:51:11.735 | Application announces `deep sleep (protect), timer wake 900s`. |
| 15:51:12.005 | COM port absent again. |
| 15:58 (Ben's reported measurement time) | Battery clips measure 2.957 V with USB connection left in place. |

## Preserved state

- All four telemetry samples report PROTECT, guard stage 4, no interrupted
  guard, LED rail off and sensor bits 0. Sensor absence in this protective
  posture is not a sensor-failure diagnosis.
- Saved last sleep voltage is 2604 mV. This predates the captured wake;
  the record does not provide a wall-clock timestamp for that sleep.
- Saved PROTECT entry is 3023 mV, origin `low-vbat`, predecessor stage 3,
  load armed 0, reset streak 0. Entry uptime is 28397260 ms; its calendar
  date is not established. This is not evidence of a high-voltage false trip.
- Saved last operator sleep is `radio-all`, 43200 s, source 8EB508,
  sequence 205. It is a separate historical record, not the current sleep.
- Precharge register readback is 300 mA, normal charge limit 2000 mA,
  regulation voltage 3600 mV and input maintain voltage 4600 mV.
- Early startup samples have charging disabled. The application then enables
  charging before sleep. Reported battery current remains around 1.5 mA in
  the last two samples while supply current changes from 70 to 324 mA;
  these brief startup readings do not establish steady battery current.
  A configured 300 mA limit must not be reported as measured charging current.

## Interpretation and follow-up

Waiting through the existing sleep interval was sufficient for USB access
through this extension. The captured wake reports PROTECT and a real low
battery voltage; no forced recovery or connector change was necessary.
The first-attachment trace therefore provides a successful delayed wake,
not a reproduction of Daisy's earlier enumeration problem.

The rise from about 2.7 V to 2.957 V, together with charger enable/status,
is consistent with charging. It does not measure recovered capacity,
prove eventual PROTECT release, or explain why prior sunlight did not leave
the battery charged. Preserve this specimen's firmware/state while observing
further recovery. Do not infer an exact state of charge from these voltages.

The September 12 container census lists Skitty as unobserved, and the
September 14 Oakland inventory still lists it among the six unlocated IDs.
This October 16 USB identification establishes its current whereabouts.
There is no confirmation here that this specimen received the September
storage firmware or successfully entered USB/solar-wake ship mode.

Evidence:

- `ops/bench/data/Oakland/20261016T224053Z-fresh-perimeter-usb.jsonl`
- `ops/bench/data/Oakland/20261016T224053Z-fresh-perimeter-observations.jsonl`
- Raw ETW: `ops/bench/private-backups/20261016T224053Z-fresh-perimeter-usb/usb.etl`
  (private; preserve locally).
- Closed ETW summary:
  `ops/bench/data/Oakland/20261016T224053Z-F3FD28-usb-etw-summary.json`.
  The original capture finished at 16:00:53.984 PDT with four telemetry
  samples in one wake window. ETW closed successfully; SHA-256:
  `48c78f8b3f21ce1898a904e8f4055357f3e8c6b4fcd98fba5373460aa27bad57`.
  One enumeration start/completion, zero recorded retries, failures or
  descriptor-validation failures.

## Natural recovery follow-up

Ben requested continued observation without intervention. A dedicated
four-hour, fixed-MAC serial capture was armed at 16:00 PDT to take over only
after the original capture writes its finished record. It uses `t` only,
does not toggle reset lines, and does not request a service hold. It prevents
laptop sleep while running. A thread heartbeat checks every ten minutes and
reports confirmed recovery, material problems or expiration; routine wakes
and voltage rises do not generate notifications. It stops the logger and
pauses itself after confirmed recovery or expiration.

- Continuation prefix:
  `ops/bench/data/Oakland/20261016T230004Z-F3FD28-natural-recovery`
  (`.jsonl` raw evidence and `.status.json` progress).
- Heartbeat ID: `watch-skitty-natural-protect-recovery`.
- Continuation confirmed running at 16:00:54.709 PDT, after the original
  capture finished; deadline 20:00:54.709 PDT. No competing serial owner.
- The logger's `recovered_observed` flag is only a candidate: verify the raw
  release/reboot timeline and fresh stable non-PROTECT samples before calling
  recovery successful.

### Verified release policy for this revision

The historical artifact entry in commit `3a8ae3f` maps this revision to source
`316470a8c393cc44ad5ba60c90c7d84c8a0003b6`, binary SHA-256
`1c31b265a1dba8544a7b02670e42382314d20fc0820cb67ed312ad43b25c349d`.
This session has identified the reported revision, not read back and hashed
Skitty's flash. Read `power_policy.cpp` and `power_glue.cpp` from that source
commit, rather than applying current Marill recovery behavior to this image.

- Normal path: battery >=3.25 V and net battery current >=+20 mA, continuously
  for 60 awake seconds, with valid/good external power, valid charger status,
  charging enabled and no charger fault.
- Full/taper alternative: battery >=3.45 V, independently corroborated cell,
  charger phase 0 (not charging/done), 2 (CV) or 3 (top-off), plus the same
  good-supply and enabled/fault-free charger requirements for 60 seconds.
- A missing or disqualifying sample resets the proof timer. Switching proof
  type also resets it. Time asleep does not accumulate toward the minute.
- Once a proof qualifies, the policy suppresses PROTECT sleep so the minute
  can complete. Otherwise a normal protected timer wake has an eight-second
  grace and returns to a 900-second sleep.
- Successful release persists OFF and reboots to initialize the previously
  parked hardware. It is not an immediate grant of full lighting power.

At the reported 2.957 V, neither release path meets its voltage requirement.
The charger's roughly 3.0 V precharge transition is separate from the firmware's
3.25 V release floor. Awake radio/processor draw is included in net battery
current: positive charge during sleep does not guarantee the awake +20 mA
criterion. This experiment follows that distinction without clearing guards.

### 16:06 PDT wake and rapid LED observation

The continuation captured four samples at 16:06:06 through 16:06:12. Voltage
was 3.031-3.033 V, up from the prior wake's 2.884-2.885 V. PROTECT/stage 4
remained in place; charger fault was 0. Startup initially disabled charging,
then logged charging ON at 16:06:09.991. Final supply reading was 4.629 V /
324 mA. Battery-current samples ranged from -1.4 to +3.5 mA and were gathered
during startup; they do not establish steady charging current or a
current-blocked release, since voltage was still below 3.25 V. The unit
announced another 900-second PROTECT sleep at 16:06:13.057.

Ben noticed a wake and rapid blinking on a Feather light (color unspecified).
The exact archived `status_led.cpp` implements a 4 Hz USER-LED low-battery
pattern: these samples reported SOC 3, and voltage below 3.15 V does not raise
the display's low-SOC floor. This makes the observation consistent with that
pattern if it was the green/yellow USER LED. The red charger STAT LED is a
separate indicator. The display value is not a verified remaining-capacity
measurement. Next timer wake is expected around 16:21 PDT.

### Charger transition interpretation

The BQ25628E datasheet Rev C gives the rising precharge-to-fast-charge
threshold as 3.0 V typical, 2.9-3.1 V limits. A 3.031 V gauge reading alone
does not establish which side of this particular charger's threshold it is
on. CHG_STAT=1 groups trickle, precharge and fast CC together; the persistent
300 mA precharge register is a setting, not a live phase indication.

The final two 16:06 samples have REG0x1D=0x54: bit 2 VINDPM_STAT is set,
indicating input-voltage regulation is active. The final input reading is
4.629 V against a 4.600 V configured input floor. Thus available input power
is another constraint; leaving precharge does not guarantee the configured
2000 mA battery charge current. These samples do not isolate whether any
limitation originates in the USB source, cable, connector or another part
of the supply path. No supply or charger settings were changed.

Source: https://www.ti.com/lit/ds/symlink/bq25628e.pdf, electrical
characteristics and REG0x1D/REG0x1E definitions. The observed low-end voltage
rise is not proportional to restored capacity; the LFP knee and charging
voltage lift prevent converting this short voltage series directly to mAh.

### Panel orientation and dual-input interpretation

At approximately 16:17 PDT, Ben clarified that the connected panel is upside
down and receiving no light; the current location is shady. He asked about
turning it face up but has not reported doing so. Record this as current
illumination context, separate from the sunlight exposure before intake.
No panel or charger setting was changed by the host.

PowerFeather's documented VDC and VUSB inputs are Schottky-diode ORed. The
higher effective source voltage supplies the common input; sources can share
current when their loaded voltages converge. A shaded panel's open-circuit
voltage alone therefore does not establish that it can take over the load.
If its voltage falls under load, the connected USB source can contribute
automatically. The selection is hardware behavior and does not wait for a
processor wake. Turning the panel face up would introduce a second energy
source into the current USB recovery observation; mark its time if done.

Reference: https://docs.powerfeather.dev/#miscellaneous-questions

### 16:31 PDT observation gap

The logger is alive (PID 20628), with status updated at 16:31:45 PDT and no
stderr output, but its raw evidence still ends with the 16:06:13 declared
900-second sleep. The expected approximately 16:21 USB wake was not captured.
At the check, about 25.6 minutes had elapsed since the sleep announcement.
Independent read-only port enumeration found no COM ports; pnputil reported
no problem devices in the USB class. A CIM inventory query was access-denied,
so it supplies no further evidence. No serial port was opened by these checks.

This is one missed expected USB observation, not proof that the MCU failed
to wake, charging stopped, voltage collapsed or PROTECT released. Leave the
bounded logger running and check the next window before attributing a cause.
No reset, replug, configuration change or experiment extension was performed.

### 16:32 PDT logger failure; detected at 16:41 PDT

The next heartbeat found a terminal `phase=error` status, updated at
2026-10-16T23:32:46.093123Z (16:32:46 PDT). The raw JSONL contains a matching
`finished` record at 23:32:46.095124Z. PID 20628 was no longer present.
The stderr traceback identifies a host file operation, not a serial command:
`checkpoint()` called `temp.replace(status)`, which failed with
`PermissionError: [WinError 5] Access is denied` while replacing the
`.status.tmp` file over `.status.json`. The finalizer subsequently wrote the
terminal status and finished record. The underlying file-access conflict is
not established; do not assign it to OneDrive, another reader, or USB hardware
without further evidence.

No new device sample was captured after 16:06:12.141 PDT: 3.031 V, PROTECT
tier 3 / guard stage 4, enabled charger, good input and zero charger fault.
Neither natural release nor a fresh non-PROTECT boot was observed. At 16:41
the host again enumerated no COM ports. The absent 16:21 observation predates
the logger failure and remains unexplained; there is no evidence here that
charging stopped or the cell collapsed.

The capture ended early and is not a completed four-hour recovery experiment.
The heartbeat `watch-skitty-natural-protect-recovery` was set to PAUSED at
16:41 PDT after verifying the finished/error record. It was not restarted or
extended. No competing serial reader, reset, hold, flash, configuration write
or chime was performed. The actual battery voltage and PROTECT state after
the last captured wake remain unknown. A later authorized run should use a
fresh evidence file and tolerate bounded transient status-checkpoint errors
without sacrificing the raw serial log.

### 16:49 PDT authorized logger repair and resume

Ben explicitly requested a logger repair and resumed observation, with no
reset or changes to Skitty. Added `ops/bench/usb_recovery_watch.py` with
bounded retries for status-file replacement and independent raw JSONL health
records. Exhausting the snapshot retries now records the checkpoint failure
and continues observation. The raw terminal record is written before the
optional final status snapshot. This repairs the observed host failure
without claiming its underlying file-access cause is known.

Six focused tests passed in `test_usb_recovery_watch.py`: transient and
persistent replacement failures, preservation of the prior status file,
exclusive evidence creation, exact-target discovery, and validation of a
finished same-target predecessor. The observer acquires a per-target process
lock, opens serial without DTR/RTS reset, sends only `t` telemetry requests,
and keeps the laptop awake for the bounded run.

Verified the old logger's finished/error record before starting PID 6628 at
16:49:02 PDT. New prefix:
`ops/bench/data/Oakland/20261016T234902Z-F3FD28-natural-recovery-r2`.
The pointer in `tmp/skitty-recovery-current.txt` now selects this run.
The deadline remains 20:00:54 PDT (03:00:54.709410Z October 17); this is a
resume within the original observation window, not four more hours.
Reactivated the existing ten-minute heartbeat with raw health records as a
fallback when the status snapshot is stale.

At 16:50:54 PDT the replacement observer had fresh successful health and
status writes, no stderr, zero snapshot failures and no new device contact.
The last actual Skitty sample remains 16:06, 3.031 V in PROTECT. No lantern
reset, hold-awake command, configuration change, flash or chime was sent.
Any recovery during the observation gap must be identified as such; a later
non-PROTECT sample alone will not establish the missing release transition.

### 17:09 PDT requested status check

The replacement observer remains healthy: status and raw health records were
fresh at 17:09:38 PDT, with zero checkpoint failures, empty stderr and no
serial openings or samples since its 16:49 restart. Independent read-only
Windows/.NET and pyserial port inventories also found no serial ports.
The original raw log confirms the last telemetry at 16:06:12 PDT: 3.031 V,
PROTECT, charging enabled and charger fault zero, followed by a declared
900-second sleep. More than an hour without another observed USB wake is
longer than that declared cadence. This does not establish that the processor
has not woken or that charging has stopped; present voltage and tier remain
unknown. The passive watch continues under the original deadline, with no
reset, replug, fixture command or configuration change during this check.

### 17:10-17:11 PDT passive Windows USB investigation

Ben asked whether Windows enumeration could explain the observation gap and
authorized laptop-side checks without disturbing the natural-recovery test.
Read-only PnP inventories found no connected serial ports and no problem
devices. Skitty's exact USB composite instance remained registered but
Disconnected, IsPresent=false, HasProblem=false. Its parent is
`USB\ROOT_HUB30\5&2b1548e6&0&0`, physical port 1, location path
`PCIROOT(0)#PCI(0803)#PCI(0000)#USBROOT(0)#USB(1)`.
The saved last-arrival/removal values correspond to 16:06:04 / 16:06:13 PDT.

Kernel-PnP Device Management event 1010 identifies this exact Skitty instance
as missing from the bus at 16:06:13.061 PDT, about 4.5 ms after the raw serial
`deep sleep (protect), timer wake 900s` message. The earlier 15:51 removal
similarly aligns with the first recorded sleep. The term "surprise removed"
is Windows' event wording; here it is consistent with the observed transition
into processor sleep, not independent evidence of a faulty cable.

Since 15:35, the enabled USBXHCI Operational and Kernel-PnP Driver Watchdog
logs had no events. Device Management/Configuration contained the known
arrival/removal history, with no later target arrival or retained failure
found. A 15:51 "requires further installation" configuration message predates
successful serial telemetry and is not proof of a continuing driver failure.
The selected system sleep/resume/crash event IDs were also absent. Existing
controllers/hubs reported Started. These findings do not exclude a host-side
detect/enumeration fault: the deeper USB event trace had ended at 16:00 and
cannot reconstruct the untraced gap retrospectively.

Started a separate administrator ETW trace at 17:11:14 PDT, with a script-owned
20-minute deadline of 17:31:14 PDT. It enables only Default + StateMachine
events from USBHUB3, UCX and USBXHCI (0x201), not transfer-payload keywords.
It performs no serial access, rescan, reset, USB-port cycle, driver change,
power-setting change or fixture command. The existing exact-target serial
observer remains the sole serial owner under its original 20:00:54 deadline.
The first launch was rejected before tracing because `-rf` cannot accompany
`-ets`; that failed attempt was preserved and the corrected helper owns the
duration. Successful startup returned exit 0, a fresh running status and a
nonempty ETL. A subsequent non-admin `logman query` was access-denied; it did
not change the active trace.

Private trace directory selected by `tmp/skitty-usb-passive-current.txt`:
`ops/bench/private-backups/20261017T001103Z-F3FD28-usb-passive-r2/`.
It contains the retained event/PnP baseline, request, status, provider filter,
console and raw ETL; the helper writes `etw-summary.json` when it closes.
The existing heartbeat will inspect the closed trace and report its result
once. Event attribution must use the target identity or mapped hub/path, not
port number alone. No future attach events would mean no observed USB
presentation, not proof that the processor never woke or charging stopped.

Reference: Microsoft's [USB ETW overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-event-tracing-for-windows)
describes hub enumeration and state-machine events and controller activity.

### 17:31 PDT passive USB trace completed

The separate 20-minute trace stopped normally (logman exit 0). It contains
2,023 records: 630 USBHUB3, 1,198 UCX, 194 USBXHCI and one trace header.
There are zero recorded enumeration starts, completions, retries, failures
or descriptor-validation failures. The exact known parent hub
`USB\ROOT_HUB30\5&2b1548e6&0&0` appears in the initial UCX rundown, mapping
it to USB device handle `0x7cf71aa465f8`. No Skitty serial-identity attach was
recorded. Controller/hub state-machine traffic exists, so the trace is not
an empty capture; a bare port number was not used as fixture identity.

This does not show Windows attempting and rejecting Skitty during the
capture. It also does not prove the processor never woke, or that charging
stopped: neither a missing electrical connection nor an unpresented USB
interface can be distinguished here. Natural-recovery serial observation
remains unchanged, healthy, with no new samples at 17:34 PDT; its original
20:00:54 deadline remains. The trace was not restarted or extended.

Raw ETL SHA-256:
`7dc06c37e75f400a889a4d5d6b5a89ca0ee8c93c911ff681c08c35341219ec47`.
The private trace directory retains its baseline, summary and review marker.

### About 17:52 PDT visual wake without a USB sample

Ben reports seeing Skitty wake, with the Feather indicator blinking more
slowly than before, approximately 1-2 Hz. This is a user-observed visual
interval, not a measured blink trace or a voltage/current sample. At 17:52:07
the replacement logger was healthy with zero samples and zero opens; the
observation gap therefore remains. No intervention was made on Skitty.

The exact installed source's status_led.cpp uses 1/2/4 Hz from the larger of
reported SOC and its coarse LFP voltage floor: >=3.15 V supplies the 2 Hz
floor, >=3.20 V the 1 Hz floor, and >=3.35 V a solid-light floor. A higher
reported SOC may slow it earlier. These are indicator heuristics, not measured
remaining capacity or PROTECT release criteria. The slower observed blink is
consistent with recovering battery indication but cannot prove a voltage
threshold or natural release. Raw note: 20261017T005200Z-F3FD28-visual-wake.json.

### 17:51:26 PDT fresh passive radio observation recovered from canary log

Review of the T-Deck receiver log from Daisy's separate OTA session recovered
a fresh Skitty heartbeat at 17:51:26.255564 PDT, with reported age 7 ms:
3.250 V, integer battery current 0 mA, PROTECT (`ptier=3`), deep-sleep reset,
uptime 9170 ms, supply 4.625 V / 306 mA, supply-good true and charger fault
zero. Firmware remained `fx-260831-b3e2738-b`; the historical protection entry
remained 3023 mV. The previous sleep record says PROTECT for 900 seconds at
3243 mV. This independently corroborates activity near Ben's visual wake.

An earlier row in the same file is cached: its 611422 ms age at 17:46:36
places its receipt around 17:36:24, with 3.243 V. The new row's age reset and
changed voltage establish freshness even though sequence 5 and uptime 9170
repeat across sleep boots. Later copies with increasing ages are not new
wakes. Do not deduplicate these records by sequence/uptime alone.

The new voltage reaches the ordinary release floor, but the reported awake
current does not meet +20 mA. One early-wake integer sample cannot establish
the current throughout that wake, charging during sleep, or a continuous
60-second release streak. It proves PROTECT at this sampled wake, not that
recovery is impossible. USB remains unobserved since 16:06; that USB gap no
longer means all telemetry is absent. The USB observer was healthy at 17:58
with zero samples, unchanged original deadline, and no restart.

No Skitty-directed command, serial open, reset or configuration change was
made. The temporary T-Deck capture has ended. Extracted raw records, source
line numbers, receipt estimates and source SHA-256 are retained in
`ops/bench/data/Oakland/20261017T005126Z-F3FD28-passive-radio-evidence.json`;
source is `20261017T004700Z-8EB508-daisy-benphone-ota-radio.jsonl`.

### 18:13 PDT: another user-observed wake without USB evidence

Ben reported "skitty awake NOW" at about 18:13 PDT. A read-only check at
18:13:51 found the existing logger healthy (status updated 18:13:49), still
with zero replacement-run samples or port opens. No additional serial port
was opened and no command, reset, or configuration change was sent to Skitty.
This supports processor wake activity but does not establish PROTECT release.

### 18:14 PDT: user reports sustained awake indicator

Ben reported that Skitty remained on with a continuous approximately 1 Hz
Feather blink rather than returning promptly to sleep. At 18:14:57 the
existing logger was still healthy (18:14:55 checkpoint), with no USB samples.
The exact installed source status_led.cpp encodes battery indication, not
lifecycle: max(gauge SOC, LFP voltage floor), with a 30-point floor at >=3.20 V
and 1 Hz for effective values 25..50. This is not a calibrated SOC estimate or
proof of release/night show. Qualified PROTECT recovery itself suppresses
sleep. Requested a non-invasive visual distinction between sensor/lantern
lights and the Feather indicator; no commands were sent to Skitty.

### 18:16 PDT: natural recovery confirmed by passive radio

Ben then reported all sensor LEDs lit, the HEX dark, and the Feather blinking.
A 50-second read-only capture of the already-running T-Deck 8EB508's serial
output (zero writes; no Skitty serial access or commands) obtained five
advancing Skitty reports. Skitty was DIM (ptier=1), DAY_CHARGE (life=1), on
unchanged fx-260831-b3e2738-b, with software-reset uptime 524756..568385 ms,
VBAT 3.277..3.279 V, +267..276 mA, good 4.625 V supply, and charger fault zero.
The historical 3023 mV PROTECT entry is unchanged. The five independent
receipt-minus-uptime estimates place the software boot at about 18:07:27.9
PDT, with transport/timestamp uncertainty. This is stable non-PROTECT operation
more than eight minutes after that boot, not a cached online flag.

Natural recovery is confirmed. The actual release and its 60-second evidence
window occurred during the observation gap; neither the release line nor the
exact transition time was captured. The last directly observed PROTECT radio
sample remains 17:51:26. This outcome proves this unit recovered naturally
under today's USB charging conditions; it does not disprove recovery starvation
under weaker solar input or establish which historical field trips were false.
DIM remains reasonable at this sample's voltage: the default FULL-clear
threshold is 3.30 V for 60 seconds. The dark HEX does not indicate night show;
its reported lifecycle is DAY_CHARGE.

Evidence: 20261017T011607Z-F3FD28-passive-tdeck.jsonl and the corresponding
natural-recovery-confirmed.json contain raw lines, ages, receipt/boot estimates
and source hash. An earlier empty 011602Z capture file resulted from a missing
Python keyword argument before any serial open. The successful capture closed
normally. Created the replacement USB logger's stop file and verified its
finished record at 18:16:42.501774 PDT; it still truthfully reports zero USB
samples and recovered_observed=false. Radio evidence, not that flag, establishes
recovery. The separate USB ETW trace was already finished/reviewed. The recovery
heartbeat was paused after completion. No changes were made to Skitty.
