# Marill staged PROTECT recovery canary -- 2026-10-15

Owner/operator: Ben + Codex laptop, sole USB writer. Target: Marill,
68:EE:8F:F4:03:48 (F40348), perimeter, direct PowerFeather USB, LFP connected.
The nine decompression fixtures are outside this experiment. See ADR 0085.

## Artifact and preserved evidence

- Source: `1d86cb8020b4a7347c577ee0db4842a3079feda6` (clean checkout).
- Revision: `fx-261015-d953f80-b`, bench/canary only.
- Binary: 1,228,688 bytes.
- SHA-256: `c9ed50c5696609724ab563b7b461357dc4270b3324858faf75744822be5cd326`.
- Flags: PowerFeather V2; 300 mA precharge; field/basic listener; channel 11;
  120 s day sleep; 12000 ms listen; PROTECT recovery target F40348 only.
- Non-secret WiFi profile label: `party-in-the-woods-v1`.
- Exact artifact retained at `firmware/fixture/build/fx-261015-d953f80-b/`.
- Original full flash backup and autopsy: `POST_BURN_MARILL_2026-10-15.md`.

At 21:51:58 UTC, USB writing and readback verification completed for the active
app1 partition at 0x340000. The partition layout matched before writing. A fresh
64 KiB prefix backup was byte-verified unchanged afterward; SHA-256:
`c1692fe3a97cab8a2083a9b0a111898d4d7e1acc5b41c95126e0b65551ae7442`.
No NVS erase, partition-table write, alternate-slot write, or OTA selection
change was performed. The requested watchdog reset disconnected the Windows
port; subsequent application telemetry, rather than its esptool exit status,
confirmed a successful boot.

Fresh telemetry at 29.799 s uptime confirmed the expected revision, field /
channel 11, both VL53L5CX and MSA311 reads healthy, 3.358 V, +363.1 mA, BQ fault
0, and no pending OTA verification. The historical low-vbat PROTECT entry at
3027 mV remained intact. The 6000 mAh gauge setting was unchanged; physical
capacity has not been re-confirmed.

## Tests and observations

The full native suite passed; the final expanded retry/serial test passed 343
checks. The final immutable embedded build completed normally with a memory
summary and manifest/hash validation. An earlier artifact build was deliberately
abandoned, never flashed, after review found that all-pixel white would hit the
dense-HEX current limiter at both stage caps. The final image uses one white
pixel so cap 32 and cap 128 exercise distinct loads.

At 21:52:34.732 UTC, `!RF40348:1` injected a persistent latch. No retry count or
historical audit was cleared. The unit qualified 30 s of healthy power and
spent/persisted attempt 1 before rebooting into sensor-only operation. The
trial boot showed stored stage 4 (PROTECT), running stage 3 (LEDs off), retry=1,
and verified VSQT off/on. VL firmware initialization completed in 9648 ms with
the early watchdog active.

At 21:54:05.394 UTC, after reaching low LED stage 2, `!RF40348:2` interrupted the
trial. The following boot re-parked with attempt=1, pending=0, last_reason=8
(reset), and a full 300000 ms cooldown. Sensors and LEDs were off. It then
slept for the remaining cooldown instead of replaying the consumed ticket.

The timed wake restored attempt=1 and zero remaining cooldown, then required
a fresh 30 s qualification before spending attempt 2. With no additional host
command, its observed stages were sensor-only at 21:59:58.763 UTC, low LED at
22:00:33.911, DIM LED at 22:01:04.463, and PASS at 22:01:35.502. Loaded-stage
telemetry stayed between 3.350 and 3.358 V. The successful transition cleared
the pending ticket, retained generation=2, and returned to DIM / guard stage 2.
USB was connected for this trial, so this is not battery-only evidence.

After PASS, the host sent RAM-only N1 and L1 to request continuous lit service
and test the two-minute retry-history forgiveness rule. At 22:03:39.944 UTC,
the counter cleared from 2 to 0 after 124.4 s elapsed since those commands.
The fixture had progressed normally to FULL / stage 1, LED rail on, both
sensors healthy, 3.353 V, +334.5 mA. It retained success generation 2, minimum
3350 mV / +272 mA and external_seen=1. N2 returned lifecycle control to AUTO;
the subsequent test reboot clears the temporary LED override.

At 22:03:49 UTC, a new exact-target test injected the latch and requested a
20 s timer sleep. Ben was prompted to remove USB, leave the LFP connected,
keep the panel out of sunlight, wait four minutes, then reconnect. USB remained
connected throughout the observed trial. It passed again as generation 3, with
minimum 3350 mV / +298 mA and external_seen=1. This is a second USB success,
not battery-only acceptance. A coordinated unplugged retry remains pending.
The host requested another ordinary N1/L1 stable-light period to prepare the
fixture without bypassing the retry-history rule.

At 22:09:11.924 UTC, retry count returned to zero through that normal rule.
The host then sent `!SF40348:1` for a one-second timed sleep to clear all RAM
bench overrides. Fresh post-wake telemetry confirmed field / channel 11,
FULL / stage 1, count=0, pending=0, smoke_render=false,
bench_rail_forced_off=false, LED rail off, both sensor reads healthy, no charger
fault, and about 3.355 V / +379 mA on USB. Success generation 3 and the original
3027 mV PROTECT audit both survived the sleep. No forced awake hold remains.

USB canary acceptance passed. Battery-only acceptance remains open; do not
promote to the squad on the strength of these USB trials alone. For the next
run, coordinate operator readiness before sending `!RF40348:3`, then unplug
USB before the recovery trial, keep the solar panel shaded, wait four minutes,
reconnect, and require a new success generation with external_seen=0.

## Evidence files

Public traces under `ops/bench/data/Oakland/`:

- `fx-261015-d953f80-b-manifest.json`: exact deployed artifact identity.
- `20261015T212419Z-F40348-recovery.jsonl`: bounded preliminary capture with no
  serial observations; not used as recovery evidence.
- `20261015T212827Z-F40348-canary.jsonl`: original-image baseline, completed.
- `20261015T214950Z-F40348-protect-flash.jsonl`: ownership, preflight, flash,
  hash/prefix verification and reset request.
- `20261015T215218Z-F40348-observe-trial.jsonl`: fresh canary boot and sensors.
- `20261015T215234Z-F40348-interrupt-trial.jsonl`: injected latch, interrupted
  trial and subsequent recovery observation.
- `20261015T220349Z-F40348-battery-trial.jsonl`: unplugged recovery experiment.
- `20261015T220706Z-F40348-prepare-trial.jsonl`: stable-service preparation for
  a coordinated unplugged retry.
- `20261015T220924Z-F40348-canary.jsonl`: final timed wake / bench-override cleanup.

This experiment does not qualify a real resistive harness, intermittent solar,
cold-start sensor short, or whole-cycle coulomb integration. Existing SDK cold
initialization can briefly enable VSQT; see the explicit limitation in ADR 0085.
