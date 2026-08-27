# Predictable fleet OTA runbook

This runbook starts with an already built, immutable ADR 0040 fixture binary.
Do not compile during the rollout and do not substitute a newer file by mtime.

## Preconditions

- One declared operator owns T-Deck and fixture OTA writes.
- Exact T-Deck, channel, artifact revision, binary SHA-256, and target short MACs
  are named.
- The shared maintenance WiFi is healthy and on the mesh channel.
- Production LFP batteries are installed for reboot ride-through.
- Use a strong-sun window. Do not mix known PROTECT fixtures into an ordinary
  pass merely to make the roster look complete.
- The dashboard and T-Deck both include the ADR 0062 campaign contract.
- The T-Deck build emits the full fleet snapshot no faster than every 10 s and
  the host status loop is bounded. A 1 Hz 100+ peer text snapshot saturates
  115200 baud and makes campaign acknowledgement nondeterministic.

## Ordinary 120-second cadence

```powershell
python ops/bench/fleet_dashboard_ota.py `
  --targets ID1,ID2,ID3 `
  --bin C:\absolute\path\to\fixture.ino.bin `
  --expect-fw fx-YYMMDD-RECIPE-p `
  --gather-cadence ordinary `
  --allow-partial-discovery
```

The default gather is 150 seconds: one 120-second sleep interval plus 30 seconds
for boot, WiFi association, and discovery. The bridge roster cycles all 130
production targets in about 1.3 seconds.

`--allow-partial-discovery` never invents or flashes a missing target. It lets a
healthy discovered cohort complete while the ledger records exact deferred IDs.
Omit it when every named fixture is required for the job to proceed.

## PROTECT pass

Run this separately, in strong sun:

```powershell
python ops/bench/fleet_dashboard_ota.py `
  --targets ID1,ID2 `
  --bin C:\absolute\path\to\fixture.ino.bin `
  --expect-fw fx-YYMMDD-RECIPE-p `
  --gather-cadence protect
```

The default gather is 930 seconds. The tool refuses a deadline shorter than the
selected sleep cadence plus margin.

## Required phase evidence

The tool performs and records:

```text
PLAN -> PREFLIGHT -> GATHER -> DISCOVER -> FREEZE -> UPLOAD -> VERIFY -> CLEANUP
```

Do not continue manually if FREEZE is unconfirmed. A valid completion names
every target as verified, deferred, or failed. HTTP upload ACK alone is not a
verified fixture.

The tool prints the exclusive-created job-ledger path at startup. Preserve that
JSONL file with the immutable artifact manifest and raw uploader result file.

After upload, audit the reported runtime profile. The artifact's field default
does not overwrite an existing NVS profile. If an old fixture remains in
commission mode, use exact-target `F<ID>:1:1` under the same declared writer,
then re-read the target. Never use a broadcast profile correction.

## USB low-voltage recovery

Treat these as separate cohorts:

- `>=2.50 V`: ordinary battery-backed OTA if identity and power are fresh;
- `2.20-2.50 V`: one supervised canary first, with good >=4.6 V USB input and
  an installed battery; require the new revision, pending-verify survival,
  recovery state 2, no BQ fault, an input-current step, and VBAT rise;
- `<2.20 V` or an approximately zero-volt/missing battery: no fleet OTA or
  automatic recovery. Isolate and bench-diagnose/replace the cell.

The 2026-08-27 recovery record is
`docs/tests/USB_POWERED_FIXTURE_RECOVERY_2026-08-27.md`.

## Timing target

For about 130 healthy ordinary-cadence fixtures:

- gather and association: about 2.5 minutes;
- parallel unicast upload: about 7-9 minutes at the measured field rate;
- reboot, verification, cleanup: about 30-45 seconds.

The goal is a predictable roughly 10-minute operation. A PROTECT cohort adds a
roughly 15-minute rendezvous and is not part of that target.
