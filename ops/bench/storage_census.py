#!/usr/bin/env python3
"""Summarize a passive net_bench JSONL census; never opens a radio/serial port.

Bridge snapshots repeat cached peers. A row counts as observed during the run
only when its inferred RF reception (elapsed - age) is after the run start.
SOC and short-wake current are deliberately not used to estimate capacity.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import statistics
from datetime import datetime
from collections import Counter
from pathlib import Path


def summarize(rows, expected=(), callsigns=None):
    callsigns = callsigns or {}
    cached, fresh = {}, {}
    duration_s = 0.0
    started_utc = None
    last_master = None
    segment_durations = {}
    for row in rows:
        elapsed = row.get("elapsed_s")
        if not isinstance(elapsed, (int, float)):
            continue
        started_utc = started_utc or row.get("segment_started_utc")
        segment = row.get("segment_index", 1)
        segment_durations[segment] = max(segment_durations.get(segment, 0), elapsed)
        if started_utc and row.get("ts_utc"):
            elapsed = (datetime.fromisoformat(row["ts_utc"]) -
                       datetime.fromisoformat(started_utc)).total_seconds()
        duration_s = max(duration_s, elapsed)
        if row.get("src") == "master":
            last_master = row
        if row.get("src") != "peer":
            continue
        pid = row.get("peer_id", "")
        age_ms = row.get("age_ms")
        if not re.fullmatch(r"[0-9A-Fa-f]{6}", pid):
            continue
        pid = pid.upper()
        cached[pid] = row
        if not isinstance(age_ms, (int, float)) or age_ms < 0:
            continue
        heard_s = elapsed - age_ms / 1000.0
        # A two-second margin rejects pre-run cache entries despite rounded
        # logger elapsed time and the bridge's serial snapshot latency.
        if heard_s < 2.0:
            continue
        prior = fresh.get(pid)
        if prior is None or heard_s >= prior[0]:
            # Optional tails can disappear on a damaged/truncated serial line.
            # Retain prior explicit fields, but expose their observation age.
            fields = dict(prior[1]) if prior else {}
            field_times = dict(prior[2]) if prior else {}
            for key, value in row.items():
                if value is not None:
                    fields[key] = value
                    field_times[key] = heard_s
            fresh[pid] = (heard_s, fields, field_times)

    peers = []
    for pid, (heard_s, row, field_times) in sorted(fresh.items()):
        peer = {"id": pid, "callsign": callsigns.get(pid, ""),
                "last_heard_elapsed_s": round(heard_s, 3),
                "age_at_end_s": round(duration_s - heard_s, 3)}
        for field in ("battery_v", "firmware_rev", "power_tier", "profile",
                      "last_sleep_cause", "last_sleep_s", "uptime_ms",
                      "supply_good", "led_rail_on", "reset_reason",
                      "rssi_dbm", "config_capacity_mah"):
            peer[field] = row.get(field)
            peer[field + "_age_s"] = (
                round(duration_s - field_times[field], 3)
                if field in field_times else None)
        peers.append(peer)

    volts = [p["battery_v"] for p in peers
             if isinstance(p["battery_v"], (int, float))
             and 0.5 < p["battery_v"] < 4.5]
    expected = set(expected)
    ids = set(fresh)
    return {
        "started_utc": started_utc, "duration_s": duration_s,
        "full_16_minute_window": max(segment_durations.values(), default=0) >= 960,
        "segment_durations_s": segment_durations,
        "bridge_id": (last_master or {}).get("master_id"),
        "channel": (last_master or {}).get("channel"),
        "cached_count": len(cached), "fresh_count": len(peers),
        "expected_count": len(expected),
        "expected_seen_count": len(ids & expected),
        "expected_not_observed": sorted(expected - ids),
        "unexpected_observed": sorted(ids - expected) if expected else [],
        "cache_only": sorted(set(cached) - ids),
        "battery_min_v": min(volts) if volts else None,
        "battery_median_v": statistics.median(volts) if volts else None,
        "battery_max_v": max(volts) if volts else None,
        "reported_0_to_0_6v": [p["id"] for p in peers
                               if isinstance(p["battery_v"], (int, float))
                               and 0 <= p["battery_v"] <= 0.6],
        "below_3v": [p["id"] for p in peers
                      if isinstance(p["battery_v"], (int, float))
                      and 0.5 < p["battery_v"] < 3.0],
        "below_3_05v": [p["id"] for p in peers
                         if isinstance(p["battery_v"], (int, float))
                         and 0.5 < p["battery_v"] < 3.05],
        "power_tiers": dict(Counter(str(p["power_tier"]) for p in peers)),
        "revisions": dict(Counter(str(p["firmware_rev"]) for p in peers)),
        "profiles": dict(Counter(str(p["profile"]) for p in peers)),
        "sleep_seconds": dict(Counter(str(p["last_sleep_s"]) for p in peers)),
        "peers": peers,
        "limitations": [
            "Not observed does not mean dead or discharged; RF may be blocked.",
            "Repeated bridge snapshots are not fresh RF observations.",
            "Optional fields are retained only from in-run observations; see field ages.",
            "Bridge may itself retain old rich heartbeat tails across short heartbeats.",
            "No SOC-to-capacity or awake-current-to-sleep-current inference is made.",
            "This is a census, not proof that any storage command was accepted.",
        ],
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("log", type=Path)
    ap.add_argument("--expected", type=Path, help="Whitespace/comma-separated exact short IDs")
    ap.add_argument("--callsigns", type=Path,
                    default=Path(__file__).parents[1] / "fleet" / "callsigns.csv")
    ap.add_argument("--out", type=Path, help="Exclusive-created result directory")
    args = ap.parse_args()
    expected = []
    if args.expected:
        expected = re.split(r"[\s,]+", args.expected.read_text().strip())
        if any(not re.fullmatch(r"[0-9A-Fa-f]{6}", x) for x in expected):
            ap.error("expected roster must contain only six-hex short IDs")
        expected = [x.upper() for x in expected]
    with args.callsigns.open(newline="", encoding="utf-8-sig") as f:
        callsigns = {r["fixture_id"]: r["callsign"] for r in csv.DictReader(f)}
    rows = []
    partial_tail = False
    with args.log.open(encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError:
                if line.endswith("\n"):
                    raise
                partial_tail = True
    result = summarize(rows, expected, callsigns)
    result["source_log"] = str(args.log.resolve())
    result["partial_final_line_ignored"] = partial_tail
    result["source_rows"] = len(rows)
    if args.out:
        args.out.mkdir(parents=True, exist_ok=False)
        (args.out / "summary.json").write_text(
            json.dumps(result, indent=2) + "\n", encoding="ascii")
        if result["peers"]:
            with (args.out / "peers.csv").open("x", newline="", encoding="ascii") as f:
                writer = csv.DictWriter(f, fieldnames=list(result["peers"][0]))
                writer.writeheader()
                writer.writerows(result["peers"])
        (args.out / "observed-targets.txt").write_text(
            "\n".join(p["id"] for p in result["peers"]) + "\n", encoding="ascii")
        (args.out / "not-observed-targets.txt").write_text(
            "\n".join(result["expected_not_observed"]) + "\n", encoding="ascii")
    print(json.dumps({k: v for k, v in result.items() if k != "peers"}, indent=2))


if __name__ == "__main__":
    main()
