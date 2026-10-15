# Marill post-burn intake, Oakland

Date: 2026-10-15 PDT. Operator: Ben + this Codex laptop, sole USB operator.
Target: Marill, F40348, 68:EE:8F:F4:03:48, COM7.

## As found

Ben reports no post-burn charging, resets, repairs or battery changes before
today, and the lid had never been opened. After opening, the interior looked
sound, with a little dust and no reported moisture, corrosion or loose wiring.
He measured about 3.3 V at the battery clips. Firmware is configured for a
6000 mAh LFP; physical capacity has not been separately confirmed.

Initial absent USB enumeration was partly explained by the laptop end not
being plugged in. Those earlier observations are not extension-failure
evidence. After Ben reported connecting the laptop end through the enclosure
extension, no serial or current USB error device was observed. Direct USB
then enumerated at 20:50:03 UTC without a requested button press. This alone
does not isolate extension failure: there has not yet been a controlled
direct/extension/direct test while holding the same firmware awake.

## First captured application state

- Exact retained September image: fx-260912-f951ae9-b, app1 at 0x340000,
  valid and no pending verification. FIELD profile, channel 11.
- First telemetry at 20:50:03.273 UTC, uptime 1637 ms, reset reason deepsleep.
- Battery present, 3.317 V, +0.7 mA, charging initially disabled during startup.
  Charging then enabled; by the end of the first 30-second capture the reading
  was 3.353 V and +371.2 mA. Supply good and BQ fault zero.
- Current tier PROTECT / stored guard stage 4, interrupted=false. LED rail off.
  Last ordinary sleep: PROTECT, 900 seconds, 3296 mV.
- Last operator command: untimed storage-usb, source 8EB508, sequence 759.
- Historical PROTECT entry: 3027 mV, low-vbat, predecessor stage 3 (OFF),
  runtime event (reset code 255), load marker 0, reset streak 0,
  fixture uptime 13407114 ms. This is not a recorded reset-induced entry.
- Sensor bits zero and all presence flags false while parked. The installed
  source deliberately skips sensor power/probing in a PROTECT boot; those
  flags were not evidence of failed sensors.

The September 12 census already recorded Marill in PROTECT, approximately
3.280 V, with the same 3027 mV low-vbat entry and no armed load/reset streak.
The final storage ledger records 3.283 V and the matching source/sequence 759
at 2026-09-13T01:20:21.996858Z (September 12 PDT). The entry therefore predates
this intake. These observations do not prove an uninterrupted latch, determine
the entry's wall-clock date, or quantify capacity consumed in storage.

## Preservation and interventions

At 20:50:46 UTC a fresh identity/revision check still reported PROTECT and
charging, at uptime 44805 ms. Entered the bootloader to preserve the specimen
before any host profile, guard, charging-setting or firmware mutation.
The first natural charge proof was consequently interrupted before completion.

Private full flash: ops/bench/private-backups/20261015T205046Z-F40348/.
8388608 bytes; SHA-256:
ae0566b59f82c7a8d3deb0214cd6424e06741821a30caf0099fe82e4409a196c

Both installed applications match their retained manifests:

| Slot | Revision | Application bytes | SHA-256 |
|---|---|---:|---|
| app0 | fx-260831-dc82da7-b | 1220192 | 7230f81ff1737fa51ebdd357b9249748e5712657469de82a4b5b75799ac83299 |
| app1 | fx-260912-f951ae9-b | 1218752 | ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001 |

NVS, OTA selection, both app slots, SPIFFS and coredump are preserved. The crash
partition is non-erased, but has not been validated/decoded; non-erased bytes
alone do not establish a crash or its cause. Checksum-valid audit candidates
were also extracted privately; an NVS scan alone does not distinguish live
from obsolete records. Firmware telemetry supplies the active-history evidence.

At 20:53:13 UTC requested one watchdog reset to leave the bootloader. Esptool
reported a Windows port-disappearance error during reset, but read-only capture
confirmed the original application running again, reset_reason=poweron.
Its interrupted=true flag is an observation after this deliberate intervention,
not evidence of a new field fault. The stored low-vbat entry was unchanged.

## Automatic recovery

Only read-only t queries followed the reset; no F0, X, profile change, charge
override, chime, LED command or firmware write was sent.

The original firmware accumulated its normal sustained charging proof and
explicitly logged:

```
power: tier -> 2 (bv=3.367 ma=384) [protect released: charge-current]
power: PROTECT release persisted -> clean reboot
```

The ensuing software reboot loaded stage 3 (OFF), park=0, interrupted=0,
and detected VL53L5CX + MSA311 with no class mismatch. The VL firmware upload
completed in 9640 ms. Fresh telemetry showed both sensor read flags true,
growing read counters, all 16 VL zones valid, and a nearest return around
51-52 mm in the current bench arrangement. That near return is not identified
as a person or obstruction by this test. Post-reboot charging was about
+340 to +359 mA with no charger fault. The historical 3027 mV PROTECT entry
and operator-storage audit survived recovery.

This proves automatic recovery under today's stable USB input. It does not
prove recovery under weak/intermittent solar, remaining battery capacity,
sensor performance at installation distances, or extension reliability.

The 180-second recovery observation completed normally (170 telemetry samples).
At 20:55:33.860 UTC the policy progressed from OFF to DIM without a host
command. Final sample at 20:56:25.289 UTC: 3.366 V, +370.7 mA, DIM / stage 2,
FIELD, supply good, BQ fault 0, LED rail off, guard_interrupted=false,
2426 accelerometer reads and 501 VL frames, both read flags true and all 16
zones valid. No further reboot occurred after the expected release reboot.
Serial capture closed and the temporary laptop-awake request was released.
No persistent service hold or storage command was applied. The fixture retains
its ordinary FIELD policy; continuous awake operation is not promised.

## Evidence

All public files below are in ops/bench/data/Oakland/:

- 20261015T204149Z-new-lantern-intake.jsonl and perimeter-as-found.json.
- 20261015-usb-connection-correction.json.
- 20261015T204756Z-new-lantern-intake.jsonl: first direct-USB application capture.
- 20261015T205046Z-F40348-backup.json: public backup hashes/layout, no secrets.
- 20261015T205307Z-F40348-recovery.jsonl: recorded ROM identity/reset attempt.
- 20261015T205326Z-F40348-recovery.jsonl: original-firmware recovery and sensors.

Prior references: CONTAINER_STORAGE_CENSUS_2026-09-12.md,
PROTECT_RECOVERY_INVESTIGATION_2026-09-14.md, the September storage-final.csv,
and the September 14 fleet-cohort.json analysis.
