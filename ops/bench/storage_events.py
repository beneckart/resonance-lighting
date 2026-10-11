"""Read-only decoding of T-Deck storage provenance (ADR 0080)."""
import re

REQUEST = re.compile(
    r"nb-storage-request target=([0-9A-F]{6}) seq=(\d+) mode=([012]) seconds=(\d+)")
RECEIPT = re.compile(
    r"nb-storage id=([0-9A-F]{6}) source=([0-9A-F]{6}) seq=(\d+) mode=([12]) status=([1-5])")
STATUSES = {1: "prepared", 2: "refused_power", 3: "refused_verify",
            4: "refused_audit", 5: "entry_failed"}


def parse_storage_event(text):
    match = REQUEST.fullmatch(text.strip())
    if match:
        target, seq, mode, seconds = match.groups()
        return dict(src="storage_request", target=target, request_seq=int(seq),
                    storage_mode=int(mode), sleep_seconds=int(seconds))
    match = RECEIPT.fullmatch(text.strip())
    if match:
        peer, source, seq, mode, status = match.groups()
        return dict(src="storage_receipt", peer_id=peer, request_source=source,
                    request_seq=int(seq), storage_mode=int(mode),
                    storage_status=int(status), storage_status_name=STATUSES[int(status)],
                    electrical_shutdown_verified=False)
    return None
