# Container storage evidence, 2026-09-12

Operator: Ben + Codex; controller T-Deck 8EB508, channel 11.
Ben explicitly authorized fleet USB-wake storage and direct laptop commands.

The interpretation, battery estimates, wake instructions and service exceptions
are in `docs/tests/CONTAINER_STORAGE_CENSUS_2026-09-12.md` at the repository root.

## Retained records

- `expected-20260901.txt`: 114-ID installed comparison roster.
- `final/`: completed initial census, including 106 observed and eight missing.
- `ota-summary.json`, `all-106-ota-verified.txt`, `hellboy-canary-ota.jsonl`,
  `wave-*-ota.jsonl`: all 106 exact-revision OTA verification results.
- `storage-canary.jsonl`, `storage-01.jsonl` through `storage-07.jsonl`:
  explicit requests, source/sequence-matched receipts and per-job stop evidence.
- `storage-*-targets.json`, `storage-remaining-plan.json`: exact target plans.
- `storage-final.json` and CSV: final reconciliation once the observation passes.
  Requires all 106 PREPARED receipts, matching durable USB-storage audits,
  completed/stopped jobs, and at least 960 seconds since the final receipt with
  no rejoin and continuous live controller capture. This is radio evidence;
  assembled current and physical USB wake remain unmeasured.
- `tdeck-storage4-manifest.json`, `tdeck-storage4-flash.txt`: exact controller
  artifact and verified hardware/flash-region identity.
- `reconcile_storage.py`: read-only analysis used for this run. It does not
  transmit commands. It requires the live dashboard and original raw capture;
  it cannot recreate a past live state after pack-up.
- `evidence-manifest.json`: sizes and SHA-256 hashes, including raw captures
  retained locally in the original checkout. The large raw captures are not
  committed. Private device NVS/apps backups are not part of this evidence set.

The target-plan preflight snapshots are historical observations. Use matched
receipts and final reconciliation for storage outcomes, not cached online flags.
None of the eight unobserved installed fixtures was included in an OTA/storage
command. Silence is not a diagnosis of its battery or radio condition.
