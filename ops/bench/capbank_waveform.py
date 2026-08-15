#!/usr/bin/env python3
"""Arm and download radio-quiet capbank strike waveforms from net_bench.

The peer must already be in shared-WiFi maintenance and built with
--capbank-waveform. HTTP is used only before and after each strike; firmware
turns WiFi fully off while the ADC DMA capture is running.
"""

from __future__ import annotations

import argparse
import json
import re
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = ROOT / "ops" / "bench" / "data" / "ca" / "capbank"


def get_bytes(url: str, timeout: float = 4.0) -> bytes:
    with urllib.request.urlopen(url, timeout=timeout) as response:
        return response.read()


def get_json(url: str, timeout: float = 4.0) -> dict:
    return json.loads(get_bytes(url, timeout).decode("utf-8"))


def parse_widths(value: str) -> list[int]:
    widths: list[int] = []
    for item in value.split(","):
        try:
            width = int(item.strip())
        except ValueError as exc:
            raise argparse.ArgumentTypeError(f"invalid pulse width: {item}") from exc
        if not 5 <= width <= 50:
            raise argparse.ArgumentTypeError("pulse widths must be in the qualified 5..50 ms range")
        widths.append(width)
    if not widths:
        raise argparse.ArgumentTypeError("at least one pulse width is required")
    return widths


def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", required=True, help="peer maintenance IPv4 address")
    ap.add_argument("--fixture-id", default="F3FD7C")
    ap.add_argument("--widths", type=parse_widths, default=parse_widths("20"))
    ap.add_argument("--rest-s", type=float, default=15.0)
    ap.add_argument("--reconnect-timeout", type=float, default=45.0)
    ap.add_argument("--out-dir", type=Path, default=DEFAULT_OUT)
    ap.add_argument(
        "--label",
        default=None,
        help="optional filesystem-safe run label, e.g. boosted-12v or nonboosted-5v",
    )
    ap.add_argument("--yes", action="store_true", help="required: authorize physical strikes")
    return ap.parse_args()


def wait_for_capture(base: str, prior_id: int, pulse_ms: int, timeout: float) -> dict:
    deadline = time.monotonic() + timeout
    last_error = "peer has not gone offline yet"
    while time.monotonic() < deadline:
        try:
            status = get_json(f"{base}/capbank/waveform", timeout=1.5)
            last_error = str(status)
            if (
                status.get("state") == "ready"
                and int(status.get("capture_id", 0)) > prior_id
                and int(status.get("pulse_ms", -1)) == pulse_ms
            ):
                return status
            if status.get("state") == "error":
                raise RuntimeError(f"capture failed: {status}")
        except (OSError, urllib.error.URLError, TimeoutError, json.JSONDecodeError) as exc:
            last_error = str(exc)
        time.sleep(0.4)
    raise TimeoutError(f"peer did not return a ready {pulse_ms} ms capture: {last_error}")


def main() -> int:
    args = parse_args()
    if not args.yes:
        raise SystemExit("refusing to actuate: inspect the bench, then pass --yes")
    if args.label and not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,39}", args.label):
        raise SystemExit("--label must be 1-40 lowercase letters, digits, or hyphens")

    base = f"http://{args.ip}"
    feature = get_json(f"{base}/capbank/waveform")
    fixture = str(feature.get("fixture_id", "")).upper()
    if fixture != args.fixture_id.upper():
        raise SystemExit(f"fixture mismatch: wanted {args.fixture_id}, found {fixture or feature}")
    telemetry = get_json(f"{base}/telemetry")
    if not telemetry.get("solenoid_enabled"):
        raise SystemExit("peer does not report solenoid_enabled")
    if not telemetry.get("battery_present"):
        raise SystemExit("peer does not report a battery; waveform sweep requires battery ride-through")

    args.out_dir.mkdir(parents=True, exist_ok=True)
    run_stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    records: list[dict] = []
    print(
        f"peer {fixture} {feature.get('fw')} at {args.ip}; "
        f"widths={args.widths}, rest={args.rest_s:g}s",
        flush=True,
    )

    for index, pulse_ms in enumerate(args.widths):
        if index:
            print(f"recharge/rest {args.rest_s:g}s", flush=True)
            time.sleep(args.rest_s)
        before = get_json(f"{base}/capbank/waveform")
        prior_id = int(before.get("capture_id", 0))
        print(f"arming {pulse_ms} ms (capture after HTTP ack, radio then goes quiet)", flush=True)
        armed = get_json(f"{base}/capbank/arm?ms={pulse_ms}")
        if armed.get("state") != "armed":
            raise RuntimeError(f"arm rejected: {armed}")
        status = wait_for_capture(base, prior_id, pulse_ms, args.reconnect_timeout)
        sample_hz = float(status.get("vsns_hz", 0.0))
        expected_post_ms = pulse_ms + int(status.get("post_ms", 0))
        minimum_samples = int(sample_hz * expected_post_ms / 1000.0 * 0.8)
        if int(status.get("vsns_count", 0)) < minimum_samples:
            raise RuntimeError(
                f"capture too short: {status.get('vsns_count')} samples, "
                f"need at least {minimum_samples} for {expected_post_ms} ms post-trigger"
            )
        csv_blob = get_bytes(f"{base}/capbank/waveform.csv", timeout=30.0)
        if not csv_blob.startswith(b"# fixture_id,") or b"t_ms,bank_v" not in csv_blob:
            raise RuntimeError("download did not look like a capbank waveform CSV")
        capture_id = int(status["capture_id"])
        label_part = f"-{args.label}" if args.label else ""
        out = args.out_dir / (
            f"{run_stamp}-{fixture.lower()}{label_part}-{pulse_ms:02d}ms-c{capture_id}.csv"
        )
        with out.open("xb") as handle:
            handle.write(csv_blob)
        record = dict(status)
        record["path"] = str(out)
        records.append(record)
        print(
            f"saved {out.name}: {status.get('vsns_count')} VSNS samples at "
            f"{status.get('vsns_hz')} sps/channel, overflow={status.get('overflow')}",
            flush=True,
        )

    summary = args.out_dir / f"{run_stamp}-{fixture.lower()}-summary.json"
    if args.label:
        summary = args.out_dir / f"{run_stamp}-{fixture.lower()}-{args.label}-summary.json"
    with summary.open("x", encoding="utf-8") as handle:
        json.dump(
            {
                "fixture_id": fixture,
                "ip": args.ip,
                "run_utc": run_stamp,
                "label": args.label,
                "captures": records,
            },
            handle,
            indent=2,
        )
        handle.write("\n")
    print(f"summary: {summary}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
