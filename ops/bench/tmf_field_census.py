#!/usr/bin/env python3
"""Read exact-target TMF8820 telemetry without flashing fixtures.

The normal ESP-NOW heartbeat reports sensor presence but not range. This tool
briefly gathers selected downlights into shared-WiFi maintenance, records real
TMF telemetry, freezes the campaign, and asks every discovered endpoint to
resume mesh operation. Output is exclusive-created JSONL field evidence.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import datetime as dt
import json
from pathlib import Path
import random
import statistics
import sys
import time

from fleet_dashboard_ota import (
    candidate_networks,
    dashboard_state,
    fetch_json,
    post_dashboard_command,
    probe_ip,
    resume_endpoint,
)


def log(message: str) -> None:
    print(message, flush=True)


def emit(handle, event: str, **fields) -> None:
    record = {
        "ts_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "event": event,
        **fields,
    }
    handle.write(json.dumps(record, separators=(",", ":")) + "\n")
    handle.flush()


def wait_campaign(url: str, job: str, phase: int, active: bool,
                  target_count: int, timeout_s: float = 30.0) -> dict:
    deadline = time.monotonic() + timeout_s
    last = None
    next_status = 0.0
    while time.monotonic() < deadline:
        if time.monotonic() >= next_status:
            post_dashboard_command(url, f"uS{job}", f"TMF census status {job}")
            next_status = time.monotonic() + 5.0
        candidate = dashboard_state(url).get("maintenance_campaign")
        if isinstance(candidate, dict):
            last = candidate
            if (
                candidate.get("job_id") == job
                and candidate.get("phase") == phase
                and bool(candidate.get("active")) == active
                and int(candidate.get("target_count") or 0) == target_count
                and int(candidate.get("status_age_ms") or 10**9) <= 2500
            ):
                return candidate
        time.sleep(0.25)
    raise RuntimeError(f"campaign acknowledgement timed out; last={last}")


def select_targets(state: dict, requested: list[str]) -> tuple[list[str], dict]:
    peers = state.get("peers") or {}
    if not isinstance(peers, dict):
        raise RuntimeError("dashboard peer table is unavailable")
    by_id = {str(key).upper(): value for key, value in peers.items()}
    if requested:
        targets = list(dict.fromkeys(item.upper() for item in requested))
    else:
        targets = sorted(
            fixture_id
            for fixture_id, peer in by_id.items()
            if int(peer.get("fixture_class") or 0) == 1
            and int(peer.get("sensor_bits") or 0) & 0x01
        )
    missing = [target for target in targets if target not in by_id]
    if missing:
        raise RuntimeError("targets absent from dashboard: " + ",".join(missing))
    return targets, by_id


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dashboard-url", default="http://127.0.0.1:8765")
    parser.add_argument("--target", action="append", default=[])
    parser.add_argument("--discover-seconds", type=float, default=150.0)
    parser.add_argument("--campaign-seconds", type=int, default=240)
    parser.add_argument("--subnet", action="append", default=["auto"])
    parser.add_argument("--probe-timeout", type=float, default=0.4)
    parser.add_argument("--probe-jobs", type=int, default=64)
    parser.add_argument("--samples", type=int, default=6)
    parser.add_argument("--sample-period", type=float, default=0.45)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    if args.campaign_seconds <= args.discover_seconds + 20:
        parser.error("campaign must outlast discovery by at least 20 seconds")
    args.out.parent.mkdir(parents=True, exist_ok=True)

    state = dashboard_state(args.dashboard_url)
    targets, peers = select_targets(state, args.target)
    if not targets:
        raise SystemExit("no live TMF-bearing downlights selected")
    job = f"{random.getrandbits(32):08X}"
    networks = candidate_networks(args.subnet)
    hosts = [str(host) for network in networks for host in network.hosts()]
    found: dict[str, tuple[str, dict]] = {}
    frozen = False

    with args.out.open("x", encoding="utf-8", newline="\n") as output:
        emit(output, "start", job_id=job, targets=targets,
             networks=[str(network) for network in networks])
        log(f"TMF census {job}: {len(targets)} exact downlights")
        post_dashboard_command(
            args.dashboard_url, f"uB{job}:{args.campaign_seconds}",
            f"TMF census begin {job}")
        for target in targets:
            post_dashboard_command(
                args.dashboard_url, f"uA{job}:{target}",
                f"TMF census add {target}")
        status = wait_campaign(args.dashboard_url, job, 1, True, len(targets))
        emit(output, "campaign_gather", status=status)
        log(f"campaign acknowledged; scanning {', '.join(map(str, networks))}")

        try:
            deadline = time.monotonic() + args.discover_seconds
            pass_number = 0
            while time.monotonic() < deadline and len(found) < len(targets):
                missing = set(targets) - set(found)
                pass_number += 1
                with concurrent.futures.ThreadPoolExecutor(
                    max_workers=args.probe_jobs
                ) as executor:
                    futures = [
                        executor.submit(
                            probe_ip, host, missing, args.probe_timeout
                        )
                        for host in hosts
                    ]
                    for future in concurrent.futures.as_completed(futures):
                        result = future.result()
                        if result and result[0] not in found:
                            fixture_id, ip, telemetry = result
                            found[fixture_id] = (ip, telemetry)
                            peer = peers[fixture_id]
                            emit(output, "endpoint_found", fixture_id=fixture_id,
                                 callsign=peer.get("callsign"), ip=ip,
                                 telemetry=telemetry)
                            log(
                                f"found {fixture_id} {peer.get('callsign')} at {ip} "
                                f"({len(found)}/{len(targets)})"
                            )
                remaining = max(0, int(deadline - time.monotonic()))
                log(f"scan pass {pass_number}: {len(found)}/{len(targets)}; "
                    f"{remaining}s left")
                if len(found) < len(targets):
                    time.sleep(1.0)
        finally:
            post_dashboard_command(
                args.dashboard_url, f"uF{job}", f"TMF census freeze {job}")
            status = wait_campaign(
                args.dashboard_url, job, 2, False, len(targets))
            frozen = True
            emit(output, "campaign_frozen", status=status)

            rows = []
            for fixture_id, (ip, first) in sorted(found.items()):
                samples = [first]
                for _ in range(max(0, args.samples - 1)):
                    time.sleep(args.sample_period)
                    sample = fetch_json(f"http://{ip}/telemetry", 1.5)
                    if sample:
                        samples.append(sample)
                positive = [
                    int(sample.get("tof_depth_mm") or 0)
                    for sample in samples
                    if int(sample.get("tof_depth_mm") or 0) > 0
                ]
                latest = samples[-1]
                row = {
                    "fixture_id": fixture_id,
                    "callsign": peers[fixture_id].get("callsign"),
                    "ip": ip,
                    "sample_count": len(samples),
                    "positive_count": len(positive),
                    "depth_median_mm": (
                        round(statistics.median(positive), 1) if positive else 0
                    ),
                    "depth_min_mm": min(positive) if positive else 0,
                    "depth_max_mm": max(positive) if positive else 0,
                    "latest_depth_mm": int(latest.get("tof_depth_mm") or 0),
                    "latest_confidence": int(latest.get("tof_confidence") or 0),
                    "tmf_read_ok": bool(latest.get("tmf_read_ok")),
                    "tmf_reads": int(latest.get("tmf_reads") or 0),
                    "tmf_errors": int(latest.get("tmf_errors") or 0),
                    "tmf_recoveries": int(latest.get("tmf_recoveries") or 0),
                    "tmf_domain_resets": int(latest.get("tmf_domain_resets") or 0),
                }
                rows.append(row)
                emit(output, "tmf_samples", **row, samples=samples)
                log(
                    f"{fixture_id} {row['callsign']}: "
                    f"median={row['depth_median_mm']} mm "
                    f"valid={row['positive_count']}/{row['sample_count']} "
                    f"conf={row['latest_confidence']}"
                )

            for fixture_id, (ip, _) in sorted(found.items()):
                resumed = resume_endpoint(ip, timeout=1.5)
                emit(output, "resume_requested", fixture_id=fixture_id, ip=ip,
                     confirmed=resumed)
            missing = sorted(set(targets) - set(found))
            emit(output, "complete", selected=len(targets), found=len(found),
                 missing=missing, rows=rows, frozen=frozen)
            log(f"complete: {len(found)}/{len(targets)} endpoints sampled; "
                f"{len(missing)} not discovered")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        raise SystemExit(130)
