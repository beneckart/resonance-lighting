#!/usr/bin/env python3
"""Download an exact-target fixture motion flight recorder as JSONL.

The trace image records continuously while the fixture remains in its ordinary
ESP-NOW/show posture. Enter maintenance only after the desired windy/person
interaction window, then run this tool to recover the retained history. Output
is exclusive-created so a field trace can never be silently overwritten.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re
import time
import urllib.error
import urllib.parse
import urllib.request


MAC_RE = re.compile(r"^[0-9A-F]{6}$")


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
        raise ValueError("motion trace response has no meta row")
    samples = rows[1:]
    if any(row.get("kind") != "sample" for row in samples):
        raise ValueError("motion trace response contains an unknown row")
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
    if not data.get("motion_trace_build"):
        raise ValueError("fixture is not running a motion-trace test image")
    if str(data.get("motion_trace_target", "")).upper() != target:
        raise ValueError("trace image is compiled for a different target")
    if not data.get("motion_trace_target_match"):
        raise ValueError("fixture reports that the compiled trace target does not match")
    fixture_class = data.get("fixture_class")
    if fixture_class not in {"downlight", "perimeter"}:
        raise ValueError(
            f"fixture is {fixture_class!r}, not a traceable downlight/perimeter"
        )
    if not data.get("msa311_present") or not data.get("msa_read_ok"):
        raise ValueError("MSA311 is absent or not producing valid samples")
    if fixture_class == "downlight" and (
        not data.get("tmf8820_present") or not data.get("tmf_read_ok")
    ):
        raise ValueError("downlight TMF8820 is absent or not producing valid frames")
    if fixture_class == "perimeter" and (
        not data.get("vl53l5cx_present") or not data.get("vl_read_ok")
    ):
        raise ValueError("perimeter VL53L5CX is absent or not producing valid frames")
    if int(data.get("mode", -1)) != 1 or int(data.get("maint_status", -1)) != 1:
        raise ValueError("fixture is not identity-ready in active maintenance mode")
    if int(data.get("motion_trace_capacity", 0)) <= 0:
        raise ValueError("fixture motion trace buffer is unavailable")


def history_cursor(meta: dict, history_s: float) -> tuple[int, int]:
    oldest = int(meta.get("oldest_seq", 0))
    newest = int(meta.get("newest_seq", 0))
    hz = int(meta.get("sample_hz", 0))
    if oldest <= 0 or newest < oldest or hz <= 0:
        raise ValueError("trace has no retained samples")
    wanted = max(1, int(math.ceil(history_s * hz)))
    first = max(oldest, newest - wanted + 1)
    return first - 1, newest


def endpoint(host: str, after: int, maximum: int) -> str:
    query = urllib.parse.urlencode({"after": after, "max": maximum})
    return f"http://{host}/motion-trace?{query}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Recover retained raw MSA/TMF/output samples from one trace image."
    )
    parser.add_argument("--host", required=True, help="fixture maintenance IP")
    parser.add_argument("--target", required=True, help="exact six-digit fixture ID")
    parser.add_argument("--expect-fw", required=True, help="exact test firmware revision")
    parser.add_argument("--out", required=True, help="new JSONL output path")
    parser.add_argument(
        "--history-s", type=float, default=300.0, help="retained pre-maint history to recover"
    )
    parser.add_argument(
        "--live-s", type=float, default=0.0, help="also follow new maintenance-mode samples"
    )
    parser.add_argument("--label", default="wind-field-trace")
    parser.add_argument("--timeout", type=float, default=4.0)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    target = args.target.strip().upper()
    if not MAC_RE.fullmatch(target):
        raise SystemExit("--target must be a six-digit short MAC")
    if not args.expect_fw.endswith("-t"):
        raise SystemExit("--expect-fw must name a test-class (-t) artifact")
    if args.history_s <= 0 or args.live_s < 0:
        raise SystemExit("--history-s must be >0 and --live-s must be >=0")

    base = f"http://{args.host}"
    try:
        telemetry = fetch_json(base + "/telemetry", args.timeout)
        preflight_fixture(telemetry, target, args.expect_fw)
        meta, _ = parse_trace_ndjson(fetch_text(endpoint(args.host, 0, 0), args.timeout))
        if str(meta.get("fixture_id", "")).upper() != target:
            raise ValueError("trace endpoint identity differs from telemetry")
        cursor, initial_newest = history_cursor(meta, args.history_s)
    except (OSError, ValueError, json.JSONDecodeError, urllib.error.URLError) as exc:
        raise SystemExit(f"trace preflight failed: {exc}") from exc

    output = Path(args.out).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    started_utc = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    written = 0
    gaps = 0
    live_deadline: float | None = None

    try:
        with output.open("x", encoding="utf-8", newline="\n") as handle:
            header = {
                "kind": "capture_meta",
                "schema": 1,
                "captured_utc": started_utc,
                "label": args.label,
                "host": args.host,
                "target": target,
                "expected_fw": args.expect_fw,
                "history_s_requested": args.history_s,
                "live_s_requested": args.live_s,
                "trace_meta": meta,
            }
            handle.write(json.dumps(header, sort_keys=True) + "\n")
            handle.flush()

            while True:
                batch_meta, samples = parse_trace_ndjson(
                    fetch_text(endpoint(args.host, cursor, 16), args.timeout)
                )
                for sample in samples:
                    seq = int(sample["seq"])
                    if seq <= cursor:
                        continue
                    if cursor and seq != cursor + 1:
                        gaps += seq - cursor - 1
                    cursor = seq
                    sample["capture_label"] = args.label
                    handle.write(json.dumps(sample, separators=(",", ":")) + "\n")
                    written += 1
                handle.flush()

                if cursor >= initial_newest and live_deadline is None:
                    if args.live_s == 0:
                        break
                    live_deadline = time.monotonic() + args.live_s
                if live_deadline is not None and time.monotonic() >= live_deadline:
                    break
                if not samples:
                    time.sleep(0.08)

            footer = {
                "kind": "capture_summary",
                "completed_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "samples_written": written,
                "sequence_gaps": gaps,
                "last_seq": cursor,
                "fixture_overwrites": int(batch_meta.get("overwrites", 0)),
            }
            handle.write(json.dumps(footer, sort_keys=True) + "\n")
    except FileExistsError as exc:
        raise SystemExit(f"refusing existing output: {output}") from exc
    except (OSError, ValueError, json.JSONDecodeError, urllib.error.URLError) as exc:
        raise SystemExit(f"trace capture failed after {written} samples: {exc}") from exc

    if written == 0:
        raise SystemExit(f"trace capture wrote no samples: {output}")
    span_s = written / int(meta["sample_hz"])
    print(
        f"captured {written} samples (~{span_s:.1f}s), gaps={gaps}, "
        f"overwrites={batch_meta.get('overwrites', 0)} -> {output}",
        flush=True,
    )


if __name__ == "__main__":
    main()
