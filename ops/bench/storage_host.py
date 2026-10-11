"""Exact USB-storage command validation and receipt provenance."""
import re
from datetime import datetime
from storage_events import parse_storage_event

IDS = re.compile(r"[0-9A-Fa-f]{6}(?:,[0-9A-Fa-f]{6}){0,19}")
JOB = re.compile(r"[0-9A-Fa-f]{8}")
CAMPAIGN = re.compile(
    r"nb-storage-campaign job=([0-9A-F]{8}) mode=([012]) active=([01]) "
    r"targets=(\d+) prepared=(\d+) refused=(\d+) remaining_ms=(\d+) dispatches=(\d+)")


def exact_targets(text):
    if not IDS.fullmatch(text):
        raise ValueError("need 1-20 comma-separated six-hex IDs")
    ids = text.upper().split(",")
    if len(set(ids)) != len(ids) or {"000000", "F40344"} & set(ids):
        raise ValueError("duplicate, broadcast, or protected target")
    return ids


def valid_storage_command(text):
    if text == "storage-status":
        return True
    args = text.split(" ")
    if len(args) == 2 and args[0] == "storage-stop":
        return bool(JOB.fullmatch(args[1]) and int(args[1], 16))
    if len(args) != 4 or args[0] != "storage-usb" or args[3] != "CONFIRM-USB-WAKE":
        return False
    if not JOB.fullmatch(args[1]) or not int(args[1], 16):
        return False
    try:
        exact_targets(args[2])
    except ValueError:
        return False
    return True


def campaign_status(line):
    m = CAMPAIGN.fullmatch(line)
    if not m:
        return None
    keys = ("mode", "active", "targets", "prepared", "refused", "remaining_ms", "dispatches")
    return dict(job=m[1], **dict(zip(keys, map(int, m.groups()[1:]))))


def observation_age_ms(peer, now_utc):
    """Age the bridge's last observation even between its ten-second emits."""
    try:
        elapsed = (datetime.fromisoformat(now_utc) -
                   datetime.fromisoformat(peer["ts_utc"])).total_seconds()
        return int(peer["age_ms"]) + max(0, round(elapsed * 1000))
    except (KeyError, TypeError, ValueError):
        return 10**12


class ReceiptTracker:
    def __init__(self, targets, bridge):
        self.targets = set(targets)
        self.bridge = bridge
        self.requests = {}
        self.statuses = {}
        self.pending = []

    def ingest(self, line):
        row = parse_storage_event(line)
        if not row or row["storage_mode"] != 2:
            return None
        if row["src"] == "storage_request":
            target = row["target"]
            if target not in self.targets or row["sleep_seconds"] != 0:
                return None
            self.requests[target] = row["request_seq"]
            pending, self.pending = self.pending, []
            for receipt in pending:
                self._receipt(receipt)
            return row
        return row if self._receipt(row) else None

    def _receipt(self, row):
        target = row["peer_id"]
        if target not in self.targets or row["request_source"] != self.bridge:
            return False
        if target not in self.requests:
            if len(self.pending) < 100:
                self.pending.append(row)
            return False
        if row["request_seq"] != self.requests[target]:
            return False
        old = self.statuses.get(target)
        if not old or old["storage_status"] == 1 or row["storage_status"] != 1:
            self.statuses[target] = row
        return True

    def complete(self):
        return self.targets == set(self.statuses)
