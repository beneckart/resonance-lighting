# USB-powered fixture recovery - 2026-08-27

## Scope and artifact

Ben connected an expected 24 depleted/old fixtures to USB power. The mesh
census first found 23, later 24, and finally 25 USB-powered fixtures as sleeping
units woke. T-Deck `8EB508` on channel 11 was the only command/OTA writer.

All fixture uploads used the existing immutable production artifact:

- revision: `fx-260826-51d1fe1-p`
- source commit: `64264b2c60cbff9a9423642cb6cfee93e4272636`
- binary bytes: `1,206,784`
- SHA-256: `57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`
- field profile default, channel 11, 300 mA BQ precharge, 120 s day sleep,
  3 s listen window

Every successful target had a fresh identity/power preflight, exact-target
maintenance entry, upload acknowledgement, fresh exact-revision heartbeat, and
fresh survival evidence beyond the 25 s host pending-verify gate.

## Result

Twenty fixtures were uploaded and verified:

```text
F40348 F2BCF4
F2BFE0 F2BDC4 F2BF7C F2BDD4 F402A8 F40314
F2B900 F4043C F3FD88 F3FD28
F403DC F2BCF0 F3FCAC F2BF74
9D7884 F40308 F4035C
F401DC
```

Three already carried the target revision and were not reflashed: Tidus
`F40424`, Magmar `F2BDFC`, and Swablu `F2BE70`. Thor `F40344` retained its
protected `net-bench-2026-08-19.1` magic-wand image. Clank `F2BF60` retained
`fx-260817-ec7f28d-b` because its reported 0.86 V battery is below ADR 0042's
2.20 V automatic-recovery floor.

Thus the final USB-powered census was:

```text
25 USB-powered
23 on fx-260826-51d1fe1-p
 1 protected net-bench magic wand (Thor)
 1 deliberately deferred unsafe low cell (Clank)
```

Post-OTA maintenance telemetry sampled every uploaded cohort. All sampled
fixtures reported `profile=field`, BQ precharge 300 mA, OTA valid, pending
false, and no charger fault. No profile mutation was required.

## Low-VBAT recovery

Daxter `9D7884` was the one-at-a-time canary at 2.324 V. It survived OTA and
pending verify, passed the BQ battery-presence test, entered recovery state 2,
and increased USB input from about 66 mA to about 294 mA. Its battery voltage
rose to 2.45 V in under a minute and later to 2.61 V; it then reported recovery
state 4 (graduated).

After that canary, Daisy `F40308` and Dratini `F4035C` were updated together.
Both survived pending verify, proved an attached battery, entered guarded
recovery, and showed a large input-current step. Daisy rose from 2.324 V to
2.61 V and graduated. Dratini rose from 2.342 V to about 2.46 V and remained in
active recovery at the final fresh maintenance sample. Leave it on USB until it
graduates above the 2.55 V/60 s recovery condition.

The MAX17260 battery-current channel stayed at 0 mA while these cells were below
its reliable cold-start voltage. That did not invalidate the recovery evidence:
the BQ state, input-current step, presence result, fault-free status, and rapid
VBAT rise all independently showed energy entering the cells.

Tidus `F40424` already had the production image but reported about 0.01 V and
recovery refused. Treat this as a missing/disconnected/failed battery path, not
as a successful USB recovery. Clank's 0.86 V cell should be isolated and
replaced or bench-diagnosed; do not lower the fleet recovery floor to charge it.

## OTA control-plane findings and fixes

The first two canary attempts correctly stopped before upload because bridge
campaign acknowledgement did not arrive in time. Root cause was deterministic:
the T-Deck printed a complete 100+ peer snapshot every second over 115200 baud,
which is more data than the link can carry, while the host requested campaign
status four times per second and added its own backlog.

The live bridge and host were corrected before any fixture upload:

- full T-Deck census emission changed from 1 s to 10 s;
- campaign status requests are bounded to one per 10 s;
- the campaign acknowledgement timeout changed from 8 s to 30 s;
- a ledger field-name collision exposed by the first immediate acknowledgement
  was fixed and regression-tested;
- an exact-target-only `F<ID>:<profile>:<persist>` bridge command was added for
  supervised profile correction. All-zero targets are refused. A runtime-only
  field command was hardware-smoked on Gambit; no fixture ultimately needed a
  persistent correction.

The final T-Deck dev-local image on exact bridge `8EB508` is 1,573,072 bytes,
SHA-256 `f42b4a1d40b151e07de1f569fd9cf50a095bf270f3d68547b450953d54ef4d51`.
The full embedded build passed, all 15 T-Deck native test binaries passed, and
29 dashboard/OTA host tests passed. Final live bridge telemetry showed channel
11, 7,888 successful sends, zero failures, and a frozen/inactive last campaign.

## Ledgers

Successful job ledgers are under `ops/bench/data/ca/`:

```text
20260827-usb-recovery-canary-r4.jsonl
20260827-usb-recovery-batch1.jsonl
20260827-usb-recovery-batch2a.jsonl
20260827-usb-recovery-batch2b.jsonl
20260827-usb-recovery-lowvbat-canary.jsonl
20260827-usb-recovery-lowvbat-batch.jsonl
20260827-usb-recovery-late-zubat.jsonl
```

The earlier canary, `-r2`, and `-r3` ledgers are intentional failed-closed
records: none reached upload. Per-target uploader results are under
`ops/bench/data/burning-man/` with the matching job IDs.

## Follow-up

- Keep Dratini on USB until a fresh sample reports recovery state 4 and a
  stable voltage above the graduation threshold.
- Remove Clank's cell from field service and diagnose/replace it; 0.86 V is not
  eligible for fleet recovery.
- Inspect Tidus's battery connector/cell; current firmware cannot prove a
  battery at about 0.01 V.
- Promote the T-Deck changes from `dev-local` to a clean immutable bridge
  artifact after the shared worktree is consolidated.
- Increase or split the T-Deck `nb-peer` text buffer: long full-tail lines can
  still truncate late profile/recovery fields, forcing maintenance telemetry
  for an authoritative audit.
