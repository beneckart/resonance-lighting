# Supplemental storage-session records

Preserved 2026-10-06 while consolidating the 2026-09-12 work. These files are
historical snapshots, process logs, and dry runs that remained local after the
original evidence curation. Their original bytes have not been changed.

`supplementary-evidence-manifest.json` records the exact size and SHA-256 of 25
files in this directory tree, plus the neighboring
`../20260912-233905-9690A585-fleet-ota-job.jsonl`. That neighboring file records
only a successful preflight and dry-run completion; it is not an OTA attempt.
The supplemental records total 2,632,997 bytes.

Use `final/` for the completed initial census and `storage-final.json` /
`storage-final.csv` for the completed storage reconciliation. Intermediate
snapshots and audits can contain incomplete counts, cached peer state, or
shorter observation windows. They do not supersede those final records.
`live-state.json` is byte-identical to the curated `pack-up-complete.json`.

The original `README.md` and `evidence-manifest.json` remain unchanged. Their
radio evidence establishes matched storage receipts and subsequent silence;
it does not establish assembled sleep current or a successful physical USB
wake test. See the repository report at
`docs/tests/CONTAINER_STORAGE_CENSUS_2026-09-12.md` for interpretation and service
instructions.

The three large raw captures -- `census.jsonl`, `ota-watch.jsonl`, and
`storage-rollout.jsonl` -- remain local only. Their sizes and hashes are already
recorded in the original evidence manifest. Private device NVS backups are
excluded from both evidence sets.
