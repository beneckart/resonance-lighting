# Bidoof decompression service

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Ben + this Codex laptop; sole USB operator
Target: Bidoof, 9F26D8, D8:85:AC:9F:26:D8, COM4

## Intake

Ben requested the same health check, preservation, decompression firmware
and final local signature routine. Initial capture began at 04:43:22 UTC,
at about 592 seconds uptime. It reported fx-260912-f951ae9-b, valid app1
at 0x340000, FIELD/channel 11, FULL tier, power-on reset, guard stage 1
and `guard_interrupted=true`. No retained PROTECT entry was present.
The previous operator command was USB-wake storage from bridge 8EB508.

Nine initial samples showed 3.208-3.213 V and positive charging current,
about +192 to +275 mA. USB supply was good, BQ fault status was zero,
and downlight ToF/MSA sensors were healthy without a class mismatch.
Stored capacity was 6,000 mAh; Ben confirmed a physical 15 Ah cell.

The selected service gate uses the existing 3.20 V maintenance advisory
floor from ADR 0046, plus a present battery, good input, charging enabled,
no charger fault and FULL/DIM allowance. This is not a PROTECT release:
the 3.25 V recovery floor applies to a previously parked fixture. No
firmware protection threshold is changed for this service.

A bounded host awake hold was started before servicing. The USB port
disappeared before any flash/configuration operation; a read-only watcher
was started to allow a normal sleep/wake cycle before intervention.

Pulled Brian's 6a07760 Lucas service/capacity records. No firmware source
changed, so the selected binary remains the exact Ness-tested image.

## Preservation and selected image

Bidoof reappeared at 04:45:42 UTC without a host reset. Fresh pre-ROM
telemetry confirmed a deep-sleep wake, 3.216 V, FULL tier, healthy sensors,
charging enabled and `guard_interrupted=false`. No profile change or
protection-clear command was needed.

Private backup: `ops/bench/private-backups/20261012T044400Z-9F26D8/`.
The original 6,000 mAh setting was preserved before correction.

- First 64 KiB, including bootloader, partition table, NVS and OTA selection:
  SHA-256 `a148b2e9cd92ed4840cc3f7fd9a492b0252029d31e3c941b8f75d502f6c5dfb2`.
- Coredump partition at 0x7f0000: nonempty, preserved privately and not yet
  decoded. Nonempty bytes alone do not prove a valid panic or a current fault.
  SHA-256 `a184d1f0d8b62b6de93047654844c1dd753c4527f3b3ae5f34efc3c034906349`.
- Original app1: ROM digest matched the retained September binary, which
  was copied privately, 1,218,752 bytes, SHA-256
  `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.

The partition table matched the new image. This is partial flash
preservation: app0 and SPIFFS were not copied. The exact retained
[Ness-tested](DECOMPRESSION_NESS_2026-10-11.md) binary was selected without
rebuilding:

- Revision: `fx-261012-5164506-b`
- Source: `cd9ce1ae9252a2792f760a9c77a6f2df4fea3157`
- Application: 1,206,816 bytes
- SHA-256: `a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`
- FIELD, channel 11, basic listener, decompression, 300 mA precharge,
  120-second ordinary sleep, 12-second listen; individual-fixture canary.

## Flash, capacity correction and acceptance

Only app1 at 0x340000 was written. At 04:48:37 UTC ROM verification matched
the new application and the entire original 64 KiB prefix. App0, SPIFFS and
the crash-record partition were left untouched. The upload completed without
an interruption or retry.

Fresh telemetry confirmed the new revision, valid app1, no pending verify,
FIELD/channel 11 and no retained PROTECT entry. At 04:48:39 the host sent
`C9F26D8:15000`; after the expected software reboot, telemetry confirmed
15,000 mAh. This intentionally updates NVS after the unchanged-prefix check;
the original capacity is preserved in the private backup. The charge limit
remained 2,000 mA. The gauge's reported SOC is not a reliable estimate after
resetting its capacity model.

The host opened a 60-minute RAM session at 04:48:43 UTC, waited at least
30 seconds of stable uptime, then sent one `!K9F26D8:1` at 04:49:11. The
pre-strike gate confirmed battery >=3.20 V, charging, good input, FULL tier,
healthy downlight ToF, no class mismatch or charger fault, and no interrupted
boot flag.

- COMPLETE: seven 40 ms hits, counter 0 -> 7, gate off, no blocks/failsafes.
- GPIO onset offsets (us): `59, 500057, 750057, 1000088, 1500059, 2500057, 3000057`.
- Maximum onset lateness 88 us; maximum inter-hit interval error 31 us.
- Completion at uptime 34417 ms: battery sampled 3.116 V, FULL tier.
- Follow-up at uptime 39741-48323 ms: recovered to 3.234-3.239 V, charging
  +234 to +264 mA, FULL tier, no charger fault, seven total strikes, no
  retained PROTECT entry and continuous uptime.

These are sampled voltage and firmware GPIO timestamps, not a minimum
transient capture or acoustic/oscilloscope measurement. Listening assessment,
physical USER wake, battery-only repeated use and overnight energy remain
unverified on this fixture. Session expiry is around 05:48:44 UTC / 22:48:44
PDT. The active session uses battery after USB removal, and the VBAT capboard
remains connected during sleep. The temporary host awake hold was released
after the final power capture.

## Public evidence

Under `ops/bench/data/Bodhi Hive/`:

- `20261012T044322Z-9F26D8-preflight.jsonl`
- `20261012T044408Z-9F26D8-usb-watch.jsonl`
- `20261012T044545Z-9F26D8-backup.jsonl`
- `20261012T044721Z-9F26D8-flash.jsonl`
- `20261012T044920Z-9F26D8-settled-power.jsonl`
