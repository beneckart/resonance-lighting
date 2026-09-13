# ADR 0081: Explicit host operation of USB-wake storage

Date: 2026-09-12
Status: Implemented for validation
Supersedes: ADR 0048 and ADR 0080's serial exclusion for this narrow operation

Ben explicitly authorized fleet USB-wake storage, then asked Codex to issue it
directly while the laptop/T-Deck remain with a campmate for unattended work.
This authorization supersedes the former requirement for a physical T-Deck
confirmation tap for this operation. It does not authorize generic agent sleep
tools or change the local UI confirmation flow.

Add `storage-usb <job8> <comma-separated-IDs> CONFIRM-USB-WAKE` to the USB CLI.
Accept 1-20 unique, nonzero exact fixture IDs. Reject the protected magic wand,
malformed/overlong commands, zero job IDs, and missing confirmation text.
The text is an operator intent marker, not cryptographic authentication.
Require fresh advertised USB-storage capability for every target before any
send. The existing mesh campaign freezes the roster, audits each request before
RF, matches receipts to its exact source/sequence/target, and expires after
16 minutes. A reboot never resumes it. Fixture power and pending-OTA gates
remain authoritative. Host operation refuses an active maintenance gather.

`storage-status` reports the current job, mode and receipt counts.
`storage-stop <job8>` stops only the matching host job. An unrelated/stale job
cannot stop a different campaign. Local Back/Stop remains able to halt further
sends. Neither operation recalls fixtures already asleep.

Host tooling must require the exact verified fixture revision, preserve a
request/receipt ledger, and distinguish PREPARED from measured electrical
shutdown. A successful upload alone is not a verified OTA, and silence alone
is not proof of ship current or successful storage. Complete the canary before
the storage rollout and retain unresolved targets for physical service.

Native tests exercise parser bounds and malformed/duplicate/zero targets.
Embedded build, USB identity/boot verification, canary receipt and subsequent
radio observations are required for the deployed record. USB wake and actual
assembled storage current still need physical qualification at service.
