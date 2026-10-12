# Fixture storage and wake

Updated: 2026-10-11. This is the operator summary for the implemented September
12 storage firmware. It does not authorize a new device operation.

## Choose the wake contract

| T-Deck Rest -> Store option | What happens | How it wakes |
|---|---|---|
| Until USB power (ship) | Untimed BQ25628E ship mode; ESP and output rails off; fuel gauge stays enabled. | Good USB **or solar** input, or QON. RESET/BOOT alone cannot wake an unpowered ESP. |
| Until physical RESET | Untimed ESP deep sleep, output rails off, no timer/GPIO wake. | Physical RESET. USB alone is not its wake trigger. |
| Wake on date | Timed transport sleep to a selected Pacific date/time, 30 minutes to seven days ahead. | ESP timer; radio resumes with outputs latched dark. Requires fresh handheld GPS time; clock drift limits accuracy. |

The date picker rejects ambiguous/repeated or nonexistent DST times. Neither
indefinite mode can be recalled over ESP-NOW. The controller, laptop and Wi-Fi
are unnecessary after storage entry. "USB wake" is the UI shorthand, not a
promise to ignore a connected solar panel.

Storage and PROTECT are separate. Storage does not set or clear the durable
PROTECT latch. RESET, ordinary OTA and ship wake do not themselves clear it;
normal recovery policy still applies. A 12-hour radio sleep is also not a
command to enter PROTECT. See the
[recovery investigation](../tests/PROTECT_RECOVERY_INVESTIGATION_2026-09-14.md).

## Review and operate

1. Obtain a fresh census over a complete 15-minute PROTECT cadence. Name the
   exact intended fixtures; silence does not establish their battery state.
2. Verify compatible fixture firmware/capability and stable power for any
   needed OTA. Installing firmware alone never requests storage. Follow the
   [artifact handoff contract](FIRMWARE_ARTIFACT_HANDOFF.md).
3. On the T-Deck, open Home -> Rest -> Store, select the wake method and exact
   target(s), then review and explicitly confirm. Review freezes the roster,
   method and date; Cancel is initially focused. Keep the handheld and screen
   active through the bounded 16-minute catch-up campaign.
4. Review receipts and failures. Stop/Back prevents further sends but does not
   recall fixtures already asleep. Restarting the handheld never resumes a job.

The fixture refuses storage during maintenance, with good external supply,
without a working battery/power manager, or before OTA verification completes.
It verifies output rails are off and persists the request before acknowledging
PREPARED and attempting entry. PREPARED plus subsequent silence is useful
command evidence, not a measurement of electrical shutdown. Never silently
substitute another wake method after a refusal/failure.

For an explicitly authorized laptop campaign, storage4 also exposes the bounded
USB host flow in the [T-Deck guide](../../firmware/tdeck_bridge/README.md#explicit-usb-host-storage-adr-0081)
and [ADR 0081](../decisions/0081-explicit-host-usb-storage.md).
`ops/bench/fleet_storage_usb.py` requires explicit confirmation, the expected
bridge/revision/channel, exact targets and a new exclusive ledger. It matches
receipt source and sequence, checks late failures and stops the job. A queued
USB write is not proof of fixture storage. Do not replay historical job plans
as a current roster.

## Retained firmware and completed rollout

These are the exact historical artifacts, not a claim that a rebuild of current
main has identical bytes. Preserve their revision, recipe and binary identity.

| Target | Revision | Application bytes | Source commit | Manifest |
|---|---|---:|---|---|
| Fixture | `fx-260912-f951ae9-b` | 1,218,752 | `fcc5ef07ccf65f0f1d034bfe523e9f1d7f68bf10` | [Fixture manifest](../../firmware/fixture/build/fx-260912-f951ae9-b/manifest.json) |
| T-Deck Plus | `tdeck-0.3.0-storage4` | 1,578,224 | `556c2f6cdbefd152a1f79437b9e7f859b7b7fd44` | [Controller manifest](../../firmware/tdeck_bridge/build/storage-host-20260912-r4/manifest.json) |

Fixture application SHA-256:
`ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.
T-Deck application SHA-256:
`11b98b33ca46babf21ad27cb1f2edd7cccdf6319a5e130df3e5442fd6260c1b1`.

The fixture image uses FIELD, channel 11, LFP, 300 mA precharge, a basic
listener, 120-second ordinary day sleep and 12-second listening grace. Its
existing `b` suffix is preserved: Ben explicitly approved this exact artifact
for the September storage operation; this is not a general new fleet release.
The controller was flashed to `8EB508` / `44:1B:F6:8E:B5:08`. The September
14 Oakland snapshot found the other controller, `979604`, on older firmware.

On September 12, all 106 observed fixtures passed fresh exact-revision OTA
verification beyond the pending window, then returned matching storage receipts
and durable audits. The final live check recorded at least 38m37s of quiet,
continuous capture and stopped jobs. Eight unobserved baseline fixtures were
not commanded; Kairi and Wooper were later heard in Oakland, leaving six
unlocated as of September 14. Radio reach did not prove container location.
See the [census and rollout](../tests/CONTAINER_STORAGE_CENSUS_2026-09-12.md)
and [retained evidence](../../ops/bench/data/ca/20260912-155847-container/README.md).

## Post-burn intake before servicing

Use two passes: preserve the as-found evidence, then repair and qualify the
fixture. A healthy result after repair cannot establish that its field state
was healthy. Ness's October 11 investigation illustrates this distinction:
its historical PROTECT entry was real, but today's first captured app boot
already had a stored FULL stage. The release itself was not observed.

1. **Record before powering or changing anything.** Label the fixture with its
   short/full MAC and callsign; record the time, enclosure/cell identity,
   existing harness and capboard source, visible moisture/corrosion/damage,
   and cell-terminal voltage. Photograph the original connections. Distinguish
   measured terminal voltage from gauge voltage and SOC. Record any earlier
   resets, charging, battery swaps or soldering; do not call these units untouched.
2. **Capture the first wake.** Start an exclusive new USB enumeration/serial
   trace before connecting directly to the PowerFeather. Keep the installed
   LFP connected for normal service. Use a no-reset serial opener and read-only
   `t` telemetry; do not press USER (it can strike), repeatedly press RESET,
   flash, erase NVS or clear a guard to obtain a clean-looking baseline.
   A USB-ship fixture should wake from good external input; an explicitly
   RESET-only stored fixture needs a recorded physical RESET instead. Match
   the exact MAC before any later command.
3. **Preserve identity and history.** Save the complete boot text and telemetry,
   including revision, profile/channel, cell capacity/charge settings, class,
   reset reason, guard stage/interruption, current power tier, last ordinary
   sleep, last operator storage request, last PROTECT entry/context, and all
   charger/gauge validity/fault fields. Keep original JSONL, not just a table.
   Capture the current state and its subsequent charging/recovery as separate
   observations. A normal app boot can change NVS; its first log is not a
   byte-for-byte pre-boot snapshot.
4. **Preserve a deeper specimen when useful.** Prioritize anomalous units and
   a few comparable healthy units for full flash/NVS backup before firmware
   or configuration changes. If genuinely pre-app NVS matters, arrange ROM
   entry before its first application boot, instead of doing step 2 first.
   Read without erase/write, retain a SHA-256 and exact identity, and keep
   backups private because they can contain Wi-Fi credentials. ROM entry and
   later reset are interventions and must be recorded too. Do not delay all
   service work waiting for long traces from every lantern.
5. **Then establish a stable bench session.** On verified compatible firmware
   such as fx-260912-f951ae9-b, record the original profile and send `F0`
   (persisted commission profile), followed by `L0` (RAM-only LED rail-off
   override). Verify fresh telemetry shows commission and the rail off.
   Commission suppresses ordinary lifecycle sleep; with verified good external
   supply it also keeps PROTECT serviceable without clearing the latch.
   `L0` does not disconnect the VBAT-fed capboard. Neither command is a
   substitute for recording the original state first. Name one operator and
   exact target; these serial commands address the attached unit, not a roster.
6. **Observe recovery before load testing.** Keep stable USB and the installed
   cell connected. For this firmware's normal PROTECT release, verify good
   supply, valid/enabled/fault-free charger data, at least +20 mA gauge charging
   and at least 3.25 V compensated battery voltage continuously for 60 seconds.
   A separate corroborated >=3.45 V full/taper proof also exists. Capture the
   explicit release message and ensuing reboot; do not infer release from a
   single 3.3 V reading. If it does not qualify, retain the failed gate evidence
   and diagnose input/cell/charger behavior. `X` is a battery-absent recovery
   command, not an installed-battery override.
7. **Service and qualify.** After preserving the baseline, isolate USB, solar,
   battery and stored capbank energy before soldering. Follow the
   [VBAT capboard notes](../../hardware/capboard/README.md#2026-10-11----bodhi-hive-vbat-fed-chime-experiment).
   Test on battery alone: individual strikes, intended repeated-use cadence,
   capbank voltage/refill, no resets, and radio operation after an idle period.
   Measure branch standby/refill current externally: the new VBAT branch
   bypasses the gauge shunt and remains powered when the LED rail is off.
   Preserve a clearly labeled after-service trace and hardware changes.
8. **Choose the exit state explicitly.** Commission persists across reboot;
   `L0` does not. Do not leave a battery fixture awake accidentally. Record the
   intended event profile/session, or restore its prior FIELD profile (`F1`
   when that was the original setting) and verify fresh status. For storage,
   use the explicit reviewed storage operation and receipts above. The old
   August rescue guide's flash-first recipe is not the intake workflow.

Known live-profile detail: the current profile-change path reinitializes
`daySleepS` to 300 seconds without reapplying the artifact's 120-second
override. Record fresh `day_sleep_s` after returning to FIELD; a subsequent
boot reapplies the artifact setting. This does not change the no-sleep
commission behavior, but it changes the expected next FIELD wake interval.

Physical RESET is not an un-PROTECT operation. On current firmware an
unexpected reset with a recent load marker can advance FULL -> DIM or
DIM/OFF -> PROTECT. The ordinary solenoid-only quiet path can retain that
marker for about two minutes, and LED activity can extend it. Avoid repeated
resets, especially just after chimes; waiting a fixed interval is not proof
that every load marker is clear. A deliberate bounded service mode that
records intent and handles this transition remains future work, not a
guarantee supplied by this procedure.

Suggested intake record (one per fixture):

```
UTC / short MAC / full MAC / callsign:
As-found cell and enclosure / terminal volts / wiring photos:
Prior interventions, including today's first external power and reset:
Firmware / original profile / channel / raw trace path:
Current tier + guard / reset cause / last storage / historical PROTECT:
Charger/gauge validity, supply, current, faults / observed recovery:
Private backup path + SHA-256, if taken:
Service changes / battery-only test + refill/standby measurements:
Final profile or storage receipt / operator:
```

## USB wake is not an indefinite USB service hold

On October 11, direct-USB Ness (9F2738) reappeared at 23:00:38 UTC and reported
`reset_reason=deepsleep`, `last_sleep_reason=day-charge`, `last_sleep_s=120`,
`power_tier=0`, and `guard_stage=1`. USB supply was good and BQ fault was zero;
charging resumed after startup qualification. It disappeared again around
23:00:50 UTC, then returned at 23:02:50 after 120.09 seconds absent. The second
wake again reported FULL and day-charge/120; after about 12 seconds it printed
`deep sleep (day-charge), timer wake 120s` and disconnected. The measured wake
interval was about 132 seconds. This is ordinary FIELD sleep, distinct from
the earlier enclosure extension Code 43 failure and a PROTECT 900-second sleep.

"Until USB power" specifies how storage ends. Once awake, FIELD lifecycle can
sleep again even with the cable connected. The ESP32-S3's native USB device
disappears during deep sleep and reconnects on wake, causing Windows sounds.
A USB disconnect sound alone says nothing about charging current while asleep.
Use supply/charger measurements, not the enumeration sound, to assess charging.

Evidence: `ops/bench/data/Bodhi Hive/20261011T225943Z-9F2738-usb-cycle-d276c45e.jsonl`.
Platform reference: [Espressif USB Serial/JTAG sleep behavior](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/usb-serial-jtag-console.html#sleep-mode-considerations).

## Small decompression installation: canary and remaining scope

Ness now has the USER/USB-only `fx-261012-5164506-b` decompression canary.
One `!K9F2738:1` line or USER press plays the locally timed seven-hit signature.
`!D9F2738:60` opens a RAM-only one-hour awake session; `!D9F2738:0` ends it.
The first successful signature trigger also opens one hour, without extending
an already active session. These commands require this canary, not the older
ordinary FIELD image. Power protection remains active and history is retained.
See [the acceptance record](../tests/DECOMPRESSION_NESS_2026-10-11.md) for exact
artifact identity, preserved evidence and measured timing. Remote/presence
integration and event energy qualification remain open.

### Working decompression squad, 2026-10-12

Ben selected the nine serviced lanterns below as the working decompression
squad. The registry and individual records confirm nine unique fixtures with
decompression firmware and a completed local seven-hit signature: seven on
Ben's bench and two on Brian's. The records do not establish a separate
battery-only acceptance test or independent wiring inspection for every unit.

| Lantern | Short MAC | Service bench | Acceptance record |
| --- | --- | --- | --- |
| Ness | `9F2738` | Ben | [Ness](../tests/DECOMPRESSION_NESS_2026-10-11.md) |
| Eevee | `9F0E54` | Ben | [Eevee](../tests/DECOMPRESSION_EEVEE_2026-10-11.md) |
| Rikku | `9F26B0` | Ben | [Rikku](../tests/DECOMPRESSION_RIKKU_2026-10-11.md) |
| Leeloo | `F40254` | Ben | [Leeloo](../tests/DECOMPRESSION_LEELOO_2026-10-11.md) |
| Bidoof | `9F26D8` | Ben | [Bidoof](../tests/DECOMPRESSION_BIDOOF_2026-10-11.md) |
| Pinsir | `9F26BC` | Ben | [Pinsir](../tests/DECOMPRESSION_PINSIR_2026-10-12.md) |
| Tangela | `9F0E5C` | Ben | [Tangela](../tests/DECOMPRESSION_TANGELA_2026-10-12.md) |
| Sakura | `F2BE0C` | Brian | [Sakura](../tests/DECOMPRESSION_SAKURA_2026-10-11.md) |
| Lucas | `9F2638` | Brian | [Lucas](../tests/DECOMPRESSION_LUCAS_2026-10-11.md) |

Ben's seven run `fx-261012-5164506-b`; Brian's two run
`fx-261012-5d7ab8f-b`, built from the same firmware source plus documentation
changes with matching feature flags. These remain distinct immutable
artifacts; exact digests and provenance are in the linked acceptance records.
No reflash or device command was performed for this roster reconciliation.

Ness's physical cell is confirmed 15 Ah, but its last observed gauge setting
was still 6 Ah; the registry's 15 Ah value does not prove the on-device setting
was corrected. That correction remains queued. Tangela's final saddle
fastening and the squad's battery-only event runtime also remain follow-ups.

### Shared interaction still to implement

For the Loom, start with one accessible button station and an explicit roster
of serviced, VBAT-retrofitted chime lanterns. A press should produce a short
ordered chime sequence at any hour. A later presence station can feed that
same trigger, with clear/rearm and cooldown behavior so one stationary visitor
does not repeatedly fire the chimes. Use the working roster above; placement
remains to be settled with the art liaison.

Existing pieces are useful but the remote station behavior is not yet implemented:

- The [Atom clicker](../../firmware/atom_clicker/README.md) sends a bounded
  exact-target strike on a press, but currently has one build-time target.
- The [Atom conductor](../../firmware/atom_conductor/README.md) gathers a fleet
  and selects daytime chimes versus nighttime color using Black Rock City
  time. It is not the all-hours small-roster public button described here.
- Current fixture presence propagation produces light/color behavior, not
  this chime trigger. The inspected path originates only on downlights;
  a perimeter presence station needs explicit integration and qualification.
- Deliberate operator strikes already bypass the solar/day gate, but a
  sleeping receiver cannot hear them. Establish a deliberate awake event
  session before admitting visitors; do not rely on a visitor's first press
  to discover sleeping units. An explicit session end, idle-reachability test,
  energy measurement and retained mechanism safeguards are required.

These station requirements remain follow-up work beyond the Ness canary.

## Qualification still open

At service, measure assembled gauge-enabled ship and timerless deep-sleep
current, physically test USB/solar/QON and RESET wake, and inspect the T-Deck
storage/calendar screens. The shipped command path has evidence; these
electrical and physical checks do not yet. Do not apply the vendor's
gauge-disabled 1 uA ship number to this gauge-enabled implementation.

Most modeled savings come from removing recurring radio windows; choosing
ship over uninterrupted deep sleep buys the practical external-power wake.
Cell-terminal measurements are needed to interpret historical very-low
telemetry. Remove external power before battery swaps. Preserve reset,
protection, charger and sleep history before changing a diagnostic specimen.

Implementation contracts: [ADR 0080](../decisions/0080-explicit-container-storage.md)
and [ADR 0081](../decisions/0081-explicit-host-usb-storage.md).
