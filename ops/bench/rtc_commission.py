#!/usr/bin/env python3
"""Set one DS3231 from fresh Bridge GPS during exact-target maintenance."""

from __future__ import annotations

import argparse
import json
import math
import re
import urllib.parse
import urllib.request


MAC_RE = re.compile(r"[0-9A-F]{6}")


def fetch_json(url: str, timeout: float = 5.0) -> dict:
    with urllib.request.urlopen(url, timeout=timeout) as response:
        value = json.loads(response.read().decode("utf-8", "replace"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object from {url}")
    return value


def preflight_fixture(telemetry: dict, target: str, expected_fw: str) -> None:
    fixture_id = str(telemetry.get("fixture_id", "")).upper()
    if fixture_id != target:
        raise ValueError(f"fixture identity mismatch: expected {target}, got {fixture_id}")
    if telemetry.get("fw") != expected_fw:
        raise ValueError(
            f"firmware mismatch: expected {expected_fw}, got {telemetry.get('fw')}"
        )
    if telemetry.get("mode") != 1 or telemetry.get("maint_status") != 1:
        raise ValueError("fixture is not in active maintenance mode")
    if telemetry.get("ds3231_present") is not True:
        raise ValueError("fixture does not report a DS3231")
    battery_v = float(telemetry.get("battery_v") or 0.0)
    supply_v = float(telemetry.get("supply_v") or 0.0)
    supply_ma = float(telemetry.get("supply_ma") or 0.0)
    powered = battery_v >= 2.5 or (
        battery_v >= 2.2
        and telemetry.get("supply_good") is True
        and supply_v >= 4.6
        and supply_ma >= 50
    )
    if not powered:
        raise ValueError(
            f"unsafe power evidence: VBAT={battery_v:.3f} V, "
            f"VBUS={supply_v:.3f} V/{supply_ma:.0f} mA"
        )


def gps_now_ms(state: dict) -> int:
    sources = state.get("time_sources") or {}
    candidates = [
        row
        for row in sources.values()
        if isinstance(row, dict)
        and row.get("source") == 1
        and row.get("valid") is True
        and row.get("date_valid") is True
        and int(row.get("observation_age_ms", 10**9)) <= 5000
    ]
    if not candidates:
        raise ValueError("no fresh valid Bridge GPS observation")
    row = min(candidates, key=lambda value: int(value["observation_age_ms"]))
    return (
        int(row["utc_s"]) * 1000
        + int(row.get("sub_ms") or 0)
        + int(row.get("gps_age_ms") or 0)
        + int(row["observation_age_ms"])
    )


def commission_utc_s(state: dict) -> int:
    # Schedule the write just ahead of the observed clock so HTTP/I2C latency
    # lands near the next DS3231 second edge, then verify against fresh GPS.
    return math.ceil(gps_now_ms(state) / 1000) + 1


def post_rtc(ip: str, target: str, utc_s: int) -> dict:
    body = urllib.parse.urlencode(
        {"fixture_id": target, "utc_s": str(utc_s), "confirm": "SET_RTC_UTC"}
    ).encode("ascii")
    request = urllib.request.Request(
        f"http://{ip}/rtc",
        data=body,
        method="POST",
        headers={"Content-Type": "application/x-www-form-urlencoded"},
    )
    with urllib.request.urlopen(request, timeout=8.0) as response:
        value = json.loads(response.read().decode("utf-8", "replace"))
    if not isinstance(value, dict) or value.get("ok") is not True:
        raise ValueError(f"RTC endpoint refused: {value}")
    if str(value.get("fixture_id", "")).upper() != target:
        raise ValueError(f"RTC response identity mismatch: {value}")
    return value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Commission one exact-target DS3231 from fresh Bridge GPS."
    )
    parser.add_argument("--target", required=True, help="six-digit fixture short MAC")
    parser.add_argument("--ip", required=True, help="identity-matched maintenance IP")
    parser.add_argument("--expect-fw", required=True, help="exact fixture firmware revision")
    parser.add_argument("--dashboard-url", default="http://127.0.0.1:8765")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    target = args.target.strip().upper()
    if not MAC_RE.fullmatch(target):
        raise SystemExit("--target must be one six-digit short MAC")
    fixture_url = f"http://{args.ip}/telemetry"
    telemetry = fetch_json(fixture_url)
    try:
        preflight_fixture(telemetry, target, args.expect_fw)
        state = fetch_json(args.dashboard_url.rstrip("/") + "/api/state")
        requested = commission_utc_s(state)
    except ValueError as exc:
        raise SystemExit(f"preflight refused: {exc}") from exc
    print(
        f"preflight {target}: fw={telemetry.get('fw')} "
        f"VBAT={float(telemetry.get('battery_v') or 0):.3f} V "
        f"rtc_valid={telemetry.get('rtc_valid')} set_utc={requested}",
        flush=True,
    )
    if args.dry_run:
        print("dry-run: exact target and GPS reference passed; RTC was not written")
        return

    try:
        response = post_rtc(args.ip, target, requested)
        post = fetch_json(fixture_url)
        preflight_fixture(post, target, args.expect_fw)
        if post.get("rtc_valid") is not True:
            raise ValueError("RTC did not become valid after write")
        readback = int(post.get("rtc_utc_s") or 0)
        state = fetch_json(args.dashboard_url.rstrip("/") + "/api/state")
        gps_s = round(gps_now_ms(state) / 1000)
        if abs(readback - gps_s) > 3:
            raise ValueError(
                f"RTC/GPS verification failed: rtc={readback}, gps={gps_s}"
            )
    except (OSError, ValueError) as exc:
        raise SystemExit(f"commission failed: {exc}") from exc
    print(
        f"commissioned {target}: rtc={response['rtc_utc_s']} "
        f"readback={readback} gps={gps_s} delta={readback - gps_s}s",
        flush=True,
    )


if __name__ == "__main__":
    main()
