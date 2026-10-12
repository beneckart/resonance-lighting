# Rikku: observed PROTECT recovery and decompression canary

Date: 2026-10-11 PDT (2026-10-12 UTC)
Operator: Ben + this Codex laptop; sole USB operator
Target: Rikku, 9F26B0, D8:85:AC:9F:26:B0, COM11

## Intake and normal recovery

Ben requested the same health-check, decompression flash and final signature
workflow used for Eevee. Windows initially enumerated no serial port and no
failed USB device. The watcher started at 04:12:06 UTC. Rikku appeared at
04:14:01 with reset reason `deepsleep`, without a host-requested reset.

Initial telemetry: fx-260912-f951ae9-b, valid app0 at 0x10000, FIELD/channel 11,
15,000 mAh battery setting, battery present at 3.261 V, PROTECT tier and
stored stage 4. Charging was initially disabled during startup, then enabled;
at uptime 8777 ms it was charging at +168.5 mA with good USB supply and no
BQ fault. The parked boot had skipped the ToF initialization.

The retained PROTECT record identifies `reset-load-armed`, predecessor DIM
(stage 2), power-on reset (1), load armed, streak 1. The logged battery was
3146 mV at 2668 ms into that historical boot. This identifies the firmware
trigger, not whether the initiating event was a depleted cell, a load-related
power collapse or a deliberate power interruption. It does not independently
establish a false PROTECT entry.

After preserving the arrival telemetry, at 04:14:08 UTC the host sent `F0`
and `L0`: persisted commission service hold and RAM-only LED rail-off. This
kept the fixture available on USB while preserving its protection policy.
Charging stayed around +170 to +210 mA at roughly 3.27 V.

At 04:15:07 UTC the firmware reported:

```
power: tier -> 2 (bv=3.271 ma=205) [protect released: charge-current]
power: PROTECT release persisted -> clean reboot
```

The next boot reported software reset and OFF tier (stored stage 3). The host
reasserted `L0` after that reboot. The normal upward ladder reached DIM at
uptime 63660 ms. At 04:16:14 UTC `F1` restored the original FIELD profile and
`L0` kept the rail off. Fresh FIELD/rail-off telemetry was confirmed before
ROM entry. No manual protection-clear command was issued.

This is a directly observed automatic recovery, unlike a snapshot of an
already recovered fixture. The original fault history remained available.

## Preserved state and installed artifact

Private backup: `ops/bench/private-backups/20261012T041600Z-9F26B0/`.
It was captured **after normal recovery and restoration of FIELD**, not as
an untouched pre-boot NVS image. Arrival and recovery states are in the public
serial traces below.

- First 64 KiB, including bootloader, partition table, NVS and OTA selection:
  SHA-256 `c1df776bd5c2331c89e1c5d4adee72b947ae9eaf0cb4ddb0fa3758e83bd8fb0f`.
- Coredump partition at 0x7f0000: all 0xff, no retained panic;
  SHA-256 `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063`.
- Active app0 matched the retained 1,218,752-byte September image by ROM
  digest verification and was copied privately:
  SHA-256 `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.

This is partial flash preservation; app1 and SPIFFS were not copied and were
left untouched. Only app0 at 0x10000 was written, using ROM `--no-stub`.
The installed partition table matched the artifact. Post-write verification
matched the new application and the entire original 64 KiB prefix.

The exact retained [Ness-tested](DECOMPRESSION_NESS_2026-10-11.md) image was
used, without rebuilding:

- Revision: `fx-261012-5164506-b`
- Source: `cd9ce1ae9252a2792f760a9c77a6f2df4fea3157`
- Application: 1,206,816 bytes
- SHA-256: `a15f81d9f1eb7fa8c79903de26b494b029f195ec5a78e165753b963f90ec292e`
- FIELD, channel 11, basic listener, decompression, 300 mA precharge,
  120-second ordinary sleep, 12-second listen; single-fixture canary.

## Acceptance

Fresh boot telemetry confirmed the expected revision, valid app0, no pending
verify, FIELD/channel 11, DIM tier, healthy downlight ToF and no class mismatch
or charger fault. The interrupted-boot flag was false. Every retained
`last_protect_entry*` field matched the pre-ROM snapshot.

The host opened a 60-minute RAM session with `!D9F26B0:60`, waited for at least
30 seconds of stable uptime, then sent one `!K9F26B0:1` command.

- COMPLETE: seven 40 ms hits, counter 0 -> 7, gate off, no blocks/failsafes.
- GPIO onset offsets (us): `62, 500057, 750065, 1000065, 1500056, 2500058, 3000066`.
- Maximum onset lateness 66 us; maximum inter-hit interval error 9 us.
- Completion at uptime 34141 ms: 3.187 V, DIM, session remaining 3569 seconds.
- Follow-up at uptime 42265-44470 ms: 3.273 -> 3.278 V, charging +125 to
  +238 mA, DIM, no charger fault or solenoid failsafe, continuous uptime.

These are firmware GPIO timestamps and sampled voltages, not acoustic,
oscilloscope or minimum-transient measurements. Listening assessment, physical
USER wake, battery-only repeated use and overnight energy remain unverified
for this fixture. Its capboard wiring was not independently inspected during
the USB operation. The active session uses battery power after USB removal;
the VBAT capboard remains connected even after the session expires.

## Public evidence

Under `ops/bench/data/Bodhi Hive/`:

- `20261012T041206Z-usb-intake.jsonl`
- `20261012T041408Z-9F26B0-recovery-service.jsonl`
- `20261012T041551Z-9F26B0-recovery-finish.jsonl`
- `20261012T041618Z-9F26B0-backup.jsonl`
- `20261012T041748Z-9F26B0-flash.jsonl`
- `20261012T041946Z-9F26B0-settled-power.jsonl`

Also pulled Brian's 2673baf record of Sakura's battery-capacity correction;
it changes no fixture source and did not change this selected artifact.
