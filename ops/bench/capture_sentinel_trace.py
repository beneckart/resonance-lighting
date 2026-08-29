#!/usr/bin/env python3
"""Recover one exact-target radio-off + perimeter-ToF power A/B/A trace.

The fixture checkpoints the complete campaign into a read-verified flash
journal before entering maintenance WiFi. Output is exclusive-created.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import statistics
import time
import urllib.error
import urllib.parse
import urllib.request


MAC_RE = re.compile(r"^[0-9A-F]{6}$")
MEASUREMENT_PHASES = ("baseline-a", "tof-active", "baseline-b")


def fetch_text(url: str, timeout: float = 4.0) -> str:
    with urllib.request.urlopen(url, timeout=timeout) as response:
        if response.status != 200:
            raise ValueError(f"HTTP {response.status} from {url}")
        return response.read().decode("utf-8", "strict")


def fetch_json(url: str, timeout: float = 4.0) -> dict:
    value = json.loads(fetch_text(url, timeout))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object from {url}")
    return value


def parse_trace_ndjson(text: str) -> tuple[dict, list[dict]]:
    rows = [json.loads(line) for line in text.splitlines() if line.strip()]
    if not rows or rows[0].get("kind") != "meta":
        raise ValueError("sentinel trace response has no meta row")
    samples = rows[1:]
    if any(row.get("kind") != "sample" for row in samples):
        raise ValueError("sentinel trace response contains an unknown row")
    return rows[0], samples


def preflight_fixture(data: dict, target: str, expect_fw: str) -> None:
    if str(data.get("fixture_id", "")).upper() != target:
        raise ValueError(
            f"identity mismatch: expected {target}, got {data.get('fixture_id')!r}"
        )
    if data.get("fw") != expect_fw:
        raise ValueError(
            f"revision mismatch: expected {expect_fw}, got {data.get('fw')!r}"
        )
    if not data.get("sentinel_trace_build"):
        raise ValueError("fixture is not running a sentinel-trace test image")
    if str(data.get("sentinel_trace_target", "")).upper() != target:
        raise ValueError("sentinel image is compiled for a different target")
    if not data.get("sentinel_trace_target_match"):
        raise ValueError("compiled sentinel target does not match the fixture")
    if data.get("sentinel_trace_smoke") is True:
        raise ValueError("short sentinel smoke evidence is not a power campaign")
    if data.get("fixture_class") != "perimeter":
        raise ValueError(f"fixture is {data.get('fixture_class')!r}, not perimeter")
    if data.get("sentinel_trace_phase") != "retrieval":
        raise ValueError(
            f"campaign is not complete: phase={data.get('sentinel_trace_phase')!r}"
        )
    if int(data.get("mode", -1)) != 1 or int(data.get("maint_status", -1)) != 1:
        raise ValueError("fixture is not identity-ready in active maintenance mode")
    if int(data.get("sentinel_trace_capacity", 0)) < 1900:
        raise ValueError("fixture sentinel trace buffer is too small for the full campaign")
    if data.get("sentinel_trace_recovery_only") is True:
        raise ValueError(
            "fixture is in fail-closed recovery without a valid checkpoint: "
            f"{data.get('sentinel_trace_persistence_state')!r}"
        )
    if data.get("sentinel_trace_persisted") is not True:
        raise ValueError("completed trace is not positively flash-persisted")


def endpoint(host: str, after: int, maximum: int) -> str:
    query = urllib.parse.urlencode({"after": after, "max": maximum})
    return f"http://{host}/sentinel-trace?{query}"


def phase_summary(samples: list[dict]) -> dict[str, dict]:
    summary: dict[str, dict] = {}
    for phase in MEASUREMENT_PHASES:
        rows = [row for row in samples if row.get("phase") == phase]
        battery = [int(row["battery_ma"]) for row in rows if int(row.get("power_flags", 0)) & 1]
        supply = [int(row["supply_ma"]) for row in rows]
        summary[phase] = {
            "samples": len(rows),
            "battery_valid_samples": len(battery),
            "battery_ma_mean": round(statistics.fmean(battery), 3) if battery else None,
            "battery_ma_median": statistics.median(battery) if battery else None,
            "supply_ma_mean": round(statistics.fmean(supply), 3) if supply else None,
            "presence_edges": sum(int(row.get("presence_rising", 0)) for row in rows),
            "vl_read_delta": (
                int(rows[-1].get("vl_reads", 0)) - int(rows[0].get("vl_reads", 0))
                if len(rows) >= 2
                else 0
            ),
        }
    return summary


def validate_campaign(samples: list[dict]) -> None:
    phases = {str(row.get("phase")) for row in samples}
    missing = [phase for phase in MEASUREMENT_PHASES if phase not in phases]
    if missing:
        raise ValueError(f"trace is missing measurement phases: {', '.join(missing)}")
    for row in samples:
        phase = row.get("phase")
        if phase in MEASUREMENT_PHASES and int(row.get("radio_on", 1)) != 0:
            raise ValueError(f"radio was on during {phase} at seq {row.get('seq')}")
        expected_rail = 1 if phase == "tof-active" else 0
        if phase in MEASUREMENT_PHASES and int(row.get("sensor_rail_on", -1)) != expected_rail:
            raise ValueError(
                f"sensor rail state is wrong during {phase} at seq {row.get('seq')}"
            )
    tof = [row for row in samples if row.get("phase") == "tof-active"]
    if len(tof) < 2 or int(tof[-1].get("vl_reads", 0)) <= int(tof[0].get("vl_reads", 0)):
        raise ValueError("VL53L5CX did not produce fresh frames during tof-active")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Recover and validate one exact-target sentinel power campaign."
    )
    parser.add_argument("--host", required=True, help="fixture maintenance IP")
    parser.add_argument("--target", required=True, help="exact six-digit perimeter ID")
    parser.add_argument("--expect-fw", required=True, help="exact test firmware revision")
    parser.add_argument("--out", required=True, help="new JSONL output path")
    parser.add_argument("--label", default="radio-off-vl53-aba")
    parser.add_argument("--timeout", type=float, default=4.0)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    target = args.target.strip().upper()
    if not MAC_RE.fullmatch(target):
        raise SystemExit("--target must be a six-digit short MAC")
    if not args.expect_fw.endswith("-t"):
        raise SystemExit("--expect-fw must name a test-class (-t) artifact")

    base = f"http://{args.host}"
    try:
        telemetry = fetch_json(base + "/telemetry", args.timeout)
        preflight_fixture(telemetry, target, args.expect_fw)
        meta, _ = parse_trace_ndjson(fetch_text(endpoint(args.host, 0, 0), args.timeout))
        if str(meta.get("fixture_id", "")).upper() != target:
            raise ValueError("trace endpoint identity differs from telemetry")
        if meta.get("persisted") is not True:
            raise ValueError("trace endpoint does not confirm flash persistence")
        oldest = int(meta.get("oldest_seq", 0))
        newest = int(meta.get("newest_seq", 0))
        if oldest <= 0 or newest < oldest:
            raise ValueError("trace has no retained samples")
        if oldest != 1 or int(meta.get("overwrites", 0)) != 0:
            raise ValueError("trace overwrote the start of the A/B/A campaign")
        cursor = oldest - 1
    except (OSError, ValueError, json.JSONDecodeError, urllib.error.URLError) as exc:
        raise SystemExit(f"sentinel preflight failed: {exc}") from exc

    output = Path(args.out).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    samples: list[dict] = []
    gaps = 0
    try:
        while cursor < newest:
            _, batch = parse_trace_ndjson(
                fetch_text(endpoint(args.host, cursor, 32), args.timeout)
            )
            if not batch:
                raise ValueError(f"trace stalled after seq {cursor}, expected {newest}")
            for sample in batch:
                seq = int(sample["seq"])
                if seq <= cursor:
                    continue
                if cursor and seq != cursor + 1:
                    gaps += seq - cursor - 1
                cursor = seq
                samples.append(sample)
        validate_campaign(samples)
        summary = phase_summary(samples)
        with output.open("x", encoding="utf-8", newline="\n") as handle:
            header = {
                "kind": "capture_meta",
                "schema": 1,
                "captured_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "label": args.label,
                "host": args.host,
                "target": target,
                "expected_fw": args.expect_fw,
                "trace_meta": meta,
            }
            handle.write(json.dumps(header, sort_keys=True) + "\n")
            for sample in samples:
                sample["capture_label"] = args.label
                handle.write(json.dumps(sample, separators=(",", ":")) + "\n")
            footer = {
                "kind": "capture_summary",
                "completed_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "samples_written": len(samples),
                "sequence_gaps": gaps,
                "phase_summary": summary,
            }
            handle.write(json.dumps(footer, sort_keys=True) + "\n")
    except FileExistsError as exc:
        raise SystemExit(f"refusing existing output: {output}") from exc
    except (OSError, ValueError, json.JSONDecodeError, urllib.error.URLError) as exc:
        raise SystemExit(f"sentinel capture failed after {len(samples)} samples: {exc}") from exc

    print(
        f"captured {len(samples)} samples, gaps={gaps} -> {output}",
        flush=True,
    )
    for phase, values in summary.items():
        print(
            f"  {phase}: n={values['samples']} battery_mean={values['battery_ma_mean']}mA "
            f"supply_mean={values['supply_ma_mean']}mA edges={values['presence_edges']}",
            flush=True,
        )


if __name__ == "__main__":
    main()
