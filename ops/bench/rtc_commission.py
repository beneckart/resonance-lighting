#!/usr/bin/env python3
"""Set one DS3231 from fresh Bridge GPS during exact-target maintenance."""

from __future__ import annotations

import argparse
import json
import math
import re
import time
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


def request_resume(ip: str) -> None:
    # The ESP32 switches modes immediately after serving this response. Force
    # the HTTP client to close so an idle keep-alive cannot leave the server in
    # maintenance even though the operator saw a successful status code.
    request = urllib.request.Request(
        f"http://{ip}/resume",
        method="GET",
        headers={"Connection": "close"},
    )
    try:
        with urllib.request.urlopen(request, timeout=5.0) as response:
            response.read()
    except OSError:
        # Closing WiFi as the response drains can look like a client error.
        # Only the fresh mesh evidence below is authoritative.
        pass


def rtc_mesh_sample(state: dict, target: str, expected_fw: str) -> tuple[str, int] | None:
    peers = state.get("peers") or {}
    peer = peers.get(target)
    if not isinstance(peer, dict):
        return None
    if peer.get("firmware_rev") != expected_fw:
        return None
    if int(peer.get("age_ms", 10**9)) > 5000:
        return None

    sources = state.get("time_sources") or {}
    row = sources.get(target)
    if not isinstance(row, dict):
        return None
    if (
        row.get("source") != 2
        or row.get("valid") is not True
        or row.get("date_valid") is not True
        or row.get("gps_valid") is not True
        or int(row.get("observation_age_ms", 10**9)) > 5000
    ):
        return None
    observed = str(row.get("ts_utc") or "")
    if not observed:
        return None
    delta_ms = int(row.get("gps_delta_ms", 10**9))
    if abs(delta_ms) > 3000:
        return None
    return observed, delta_ms


def wait_for_rtc_mesh(
    dashboard_url: str,
    target: str,
    expected_fw: str,
    previous_observation: str,
    required: int = 3,
    timeout_s: float = 45.0,
) -> list[int]:
    seen = {previous_observation} if previous_observation else set()
    deltas: list[int] = []
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        state = fetch_json(dashboard_url.rstrip("/") + "/api/state")
        sample = rtc_mesh_sample(state, target, expected_fw)
        if sample and sample[0] not in seen:
            seen.add(sample[0])
            deltas.append(sample[1])
            if len(deltas) >= required:
                return deltas
        time.sleep(1.0)
    raise ValueError(
        f"only {len(deltas)}/{required} fresh RTC mesh observations after resume"
    )


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
        previous = str(
            ((state.get("time_sources") or {}).get(target) or {}).get("ts_utc") or ""
        )
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

    rtc_written = False
    try:
        response = post_rtc(args.ip, target, requested)
        rtc_written = True
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
        if rtc_written:
            request_resume(args.ip)
        raise SystemExit(f"commission failed: {exc}") from exc
    request_resume(args.ip)
    try:
        deltas = wait_for_rtc_mesh(
            args.dashboard_url, target, args.expect_fw, previous
        )
    except (OSError, ValueError) as exc:
        raise SystemExit(
            f"RTC was written, but mesh resume verification failed: {exc}"
        ) from exc
    print(
        f"commissioned {target}: rtc={response['rtc_utc_s']} "
        f"readback={readback} gps={gps_s} delta={readback - gps_s}s "
        f"mesh_deltas_ms={','.join(str(value) for value in deltas)}",
        flush=True,
    )


if __name__ == "__main__":
    main()
