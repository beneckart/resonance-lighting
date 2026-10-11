#!/usr/bin/env python3
"""Explicit exact-roster USB-wake storage through a storage4 T-Deck.

Requires operator authorization and a prior verified OTA for every target.
Never describes PREPARED or radio silence as measured electrical shutdown.
"""
import argparse
from collections import deque
from datetime import datetime, timezone
import json
from pathlib import Path
import secrets
import time
import urllib.request

from storage_host import (ReceiptTracker, campaign_status, exact_targets,
                          observation_age_ms, valid_storage_command)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--targets", required=True)
    ap.add_argument("--expect-fw", required=True)
    ap.add_argument("--job-out", required=True)
    ap.add_argument("--dashboard-url", default="http://127.0.0.1:8765")
    ap.add_argument("--bridge", default="8EB508")
    ap.add_argument("--confirm-usb-wake", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    targets = exact_targets(args.targets)
    if not args.confirm_usb_wake and not args.dry_run:
        ap.error("explicit --confirm-usb-wake is required")
    job = secrets.token_hex(4).upper()
    while job == "00000000":
        job = secrets.token_hex(4).upper()
    Path(args.job_out).parent.mkdir(parents=True, exist_ok=True)
    ledger = open(args.job_out, "x", encoding="utf-8", newline="\n")
    started = time.monotonic()

    def emit(event, **fields):
        row = dict(ts_utc=datetime.now(timezone.utc).isoformat(),
                   elapsed_s=round(time.monotonic() - started, 3), job_id=job,
                   event=event, **fields)
        ledger.write(json.dumps(row, sort_keys=True) + "\n")
        ledger.flush()

    def state():
        with urllib.request.urlopen(args.dashboard_url + "/api/state", timeout=5) as r:
            s = json.load(r)
        master = s.get("master", {})
        if (not s.get("serial", {}).get("connected") or master.get("id") != args.bridge or
                master.get("channel") != 11 or
                master.get("firmware_rev") != "tdeck-0.3.0-storage4"):
            raise RuntimeError("expected connected storage4 bridge on channel 11")
        return s

    def command(text):
        if not valid_storage_command(text):
            raise ValueError("invalid storage command")
        request = urllib.request.Request(args.dashboard_url + "/api/cmd",
            data=json.dumps(dict(cmd=text, label="Authorized USB storage " + job)).encode(),
            headers={"Content-Type": "application/json"}, method="POST")
        with urllib.request.urlopen(request, timeout=5) as response:
            result = json.load(response)
        if not result.get("ok"):
            raise RuntimeError("command was not queued")
        emit("command_queued", command=text)

    seen, order = set(), deque()

    def new_lines(s):
        for row in s.get("raw", []):
            key = (row["ts_utc"], row["line"])
            if key in seen:
                continue
            seen.add(key); order.append(key)
            while len(order) > 4096:
                seen.discard(order.popleft())
            yield row

    attempted = False
    stopped = False
    tracker = ReceiptTracker(targets, args.bridge)
    try:
        initial = state()
        for target in targets:
            peer = initial.get("peers", {}).get(target, {})
            if (peer.get("firmware_rev") != args.expect_fw or
                    observation_age_ms(peer, initial["ts_utc"]) > 1800000):
                raise RuntimeError("missing fresh expected fixture revision: " + target)
        emit("preflight", targets=targets, expected_firmware=args.expect_fw,
             bridge=initial["master"], authorized=args.confirm_usb_wake,
             peers={target: initial["peers"][target] for target in targets})
        if args.dry_run:
            print("Dry run passed; no commands sent.", flush=True)
            return 0
        list(new_lines(initial))  # never accept a cached acknowledgement
        command("storage-status")
        deadline = time.monotonic() + 30
        status = None
        while time.monotonic() < deadline and status is None:
            for row in new_lines(state()):
                status = campaign_status(row["line"]) or status
            if status is None:
                time.sleep(0.4)
        if status is None or status["active"]:
            raise RuntimeError("storage controller not freshly confirmed idle")
        attempted = True
        command(f"storage-usb {job} {','.join(targets)} CONFIRM-USB-WAKE")
        deadline = time.monotonic() + 1005
        complete_since = None
        next_status = 0
        accepted = False
        while time.monotonic() < deadline:
            s = state()
            for row in new_lines(s):
                line = row["line"]
                if line.startswith("nb-storage"):
                    emit("bridge_storage_line", bridge_ts_utc=row["ts_utc"], line=line)
                if line.startswith(f"nb-storage-host job={job} "):
                    if " accepted=0 " in line:
                        raise RuntimeError("bridge refused storage: " + line)
                    accepted = " accepted=1 " in line
                matched = tracker.ingest(line)
                if matched:
                    emit("matched_provenance", provenance=matched)
                    if matched["src"] == "storage_receipt":
                        print(matched["peer_id"], matched["storage_status_name"], flush=True)
                status = campaign_status(line)
                if status and status["job"] == job:
                    if not status["active"] and not tracker.complete():
                        raise RuntimeError("campaign stopped or expired with unresolved targets")
            if not accepted and time.monotonic() - started > 45:
                raise RuntimeError("missing fresh host acceptance")
            if accepted and tracker.complete():
                if complete_since is None:
                    complete_since = time.monotonic()
                if time.monotonic() - complete_since >= 15:
                    break
            if time.monotonic() >= next_status:
                command("storage-status")
                next_status = time.monotonic() + 5
            time.sleep(0.4)
        command("storage-stop " + job)
        stop_deadline = time.monotonic() + 15
        while time.monotonic() < stop_deadline and not stopped:
            for row in new_lines(state()):
                line = row["line"]
                if line.startswith("nb-storage"):
                    emit("bridge_storage_line", bridge_ts_utc=row["ts_utc"], line=line)
                tracker.ingest(line)
                status = campaign_status(line)
                if status and status["job"] == job and not status["active"]:
                    stopped = True
            if not stopped:
                time.sleep(0.4)
        final = state()
        prepared = sorted(k for k, v in tracker.statuses.items() if v["storage_status"] == 1)
        failed = {k: v["storage_status_name"] for k, v in tracker.statuses.items() if v["storage_status"] != 1}
        unresolved = sorted(set(targets) - set(tracker.statuses))
        still_recent = sorted(k for k in prepared if
            observation_age_ms(final["peers"].get(k, {}), final["ts_utc"]) < 10000)
        emit("job_finished", prepared=prepared, failed=failed, unresolved=unresolved,
             still_recent=still_recent, sends_stopped=stopped,
             electrical_shutdown_verified=False,
             peers={k: final["peers"].get(k) for k in targets})
        print(f"Storage {job}: prepared={len(prepared)} failed={len(failed)} "
              f"unresolved={len(unresolved)} still_recent={len(still_recent)} stopped={stopped}", flush=True)
        return 0 if not failed and not unresolved and not still_recent and stopped else 2
    except Exception as exc:
        emit("job_error", error=str(exc), statuses=tracker.statuses)
        print("Storage stopped with error:", exc, flush=True)
        return 2
    finally:
        if attempted and not stopped:
            try:
                command("storage-stop " + job)
            except Exception as exc:
                emit("stop_unconfirmed", error=str(exc))
        ledger.close()


if __name__ == "__main__":
    raise SystemExit(main())
