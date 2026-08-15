#!/usr/bin/env python3
"""Watch PowerFeather telemetry until an LFP charge has genuinely tapered.

This is read-only with respect to charger configuration. It sends the existing
serial ``t`` telemetry command, records every accepted sample to an exclusive
JSONL file, and exits after the completion condition remains true for a sustained
hold interval.

Example:

  python ops/bench/charge_taper_watch.py --port COM4

The default completion condition is:

  - a real bulk-charge sample of at least 500 mA has been observed first;
  - battery type is Generic_LFP;
  - external supply remains good at >=4.5 V;
  - battery voltage is >=3.58 V; and
  - absolute battery current is <=120 mA continuously for 5 minutes.

On Windows, completion produces audible alerts and a local ``msg.exe`` popup.
The JSONL trace and a sibling ``.done.json`` marker remain as evidence.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import serial


def iso_now() -> str:
    return datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")


def safe_port_name(port: str) -> str:
    return "".join(c if c.isalnum() or c in "-_" else "_" for c in port)


def default_output(port: str) -> Path:
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    return (
        Path(__file__).resolve().parent
        / "data"
        / "ca"
        / f"{stamp}-charge-taper-{safe_port_name(port)}.jsonl"
    )


def write_json_line(stream: Any, event: dict[str, Any]) -> None:
    stream.write(json.dumps(event, sort_keys=True, separators=(",", ":")) + "\n")
    stream.flush()


def telemetry_from_line(line: str) -> dict[str, Any] | None:
    if not line.startswith("{"):
        return None
    try:
        value = json.loads(line)
    except json.JSONDecodeError:
        return None
    if not isinstance(value, dict) or "battery_v" not in value or "battery_ma" not in value:
        return None
    return value


def open_serial(port: str, baud: int) -> serial.Serial:
    link = serial.Serial(port, baud, timeout=0.25, write_timeout=2)
    link.reset_input_buffer()
    return link


def query_telemetry(link: serial.Serial, timeout_s: float) -> dict[str, Any]:
    link.write(b"t\n")
    link.flush()
    deadline = time.monotonic() + timeout_s
    lines: list[str] = []
    while time.monotonic() < deadline:
        raw = link.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").strip()
        if not line:
            continue
        lines.append(line)
        telemetry = telemetry_from_line(line)
        if telemetry is not None:
            return telemetry
    tail = " | ".join(lines[-3:]) if lines else "no serial output"
    raise TimeoutError(f"no telemetry JSON before timeout ({tail})")


def send_completion_alert(message: str) -> list[str]:
    errors: list[str] = []
    if os.name != "nt":
        return errors
    try:
        import winsound

        for _ in range(3):
            winsound.MessageBeep(winsound.MB_ICONEXCLAMATION)
            time.sleep(0.4)
    except Exception as exc:  # pragma: no cover - platform alert only
        errors.append(f"beep: {exc}")
    try:
        result = subprocess.run(
            ["msg.exe", "*", "/TIME:300", message],
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )
        if result.returncode:
            detail = (result.stderr or result.stdout).strip()
            errors.append(f"msg.exe exit {result.returncode}: {detail}")
    except Exception as exc:  # pragma: no cover - platform alert only
        errors.append(f"msg.exe: {exc}")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Alert when a PowerFeather LFP charge tapers to completion."
    )
    parser.add_argument("--port", required=True, help="PowerFeather serial port, e.g. COM4")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--interval-s", type=float, default=15.0)
    parser.add_argument("--query-timeout-s", type=float, default=4.0)
    parser.add_argument("--min-v", type=float, default=3.58)
    parser.add_argument("--taper-ma", type=float, default=120.0)
    parser.add_argument("--armed-ma", type=float, default=500.0)
    parser.add_argument("--hold-s", type=float, default=300.0)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--done-file", type=Path)
    parser.add_argument("--once", action="store_true", help="read one sample and exit")
    parser.add_argument("--no-alert", action="store_true", help="skip Windows beep/popup")
    args = parser.parse_args()
    if args.interval_s <= 0 or args.query_timeout_s <= 0 or args.hold_s < 0:
        parser.error("interval/query timeout must be >0 and hold must be >=0")
    if args.taper_ma < 0 or args.armed_ma <= args.taper_ma:
        parser.error("armed-ma must be greater than the nonnegative taper-ma")
    return args


def main() -> int:
    args = parse_args()
    out = (args.out or default_output(args.port)).resolve()
    done_file = (args.done_file or out.with_suffix(".done.json")).resolve()
    out.parent.mkdir(parents=True, exist_ok=True)

    try:
        log = out.open("x", encoding="utf-8", newline="\n")
    except FileExistsError:
        print(f"refusing existing output: {out}", file=sys.stderr)
        return 2

    print(f"charge sentinel PID {os.getpid()} -> {out}", flush=True)
    write_json_line(
        log,
        {
            "event": "meta",
            "at": iso_now(),
            "pid": os.getpid(),
            "port": args.port,
            "baud": args.baud,
            "interval_s": args.interval_s,
            "query_timeout_s": args.query_timeout_s,
            "min_v": args.min_v,
            "taper_ma": args.taper_ma,
            "armed_ma": args.armed_ma,
            "hold_s": args.hold_s,
            "done_file": str(done_file),
        },
    )

    link: serial.Serial | None = None
    seen_bulk = False
    peak_ma = float("-inf")
    candidate_since: float | None = None
    last_sample: dict[str, Any] | None = None

    try:
        while True:
            loop_started = time.monotonic()
            try:
                if link is None or not link.is_open:
                    link = open_serial(args.port, args.baud)
                    write_json_line(log, {"event": "serial_open", "at": iso_now()})
                telemetry = query_telemetry(link, args.query_timeout_s)
            except Exception as exc:
                if link is not None:
                    try:
                        link.close()
                    except Exception:
                        pass
                    link = None
                candidate_since = None
                event = {"event": "read_error", "at": iso_now(), "error": str(exc)}
                write_json_line(log, event)
                print(f"{event['at']} READ ERROR: {exc}", flush=True)
                if args.once:
                    return 1
                time.sleep(max(1.0, args.interval_s))
                continue

            at = iso_now()
            try:
                battery_v = float(telemetry["battery_v"])
                battery_ma = float(telemetry["battery_ma"])
                supply_v = float(telemetry.get("supply_v", 0.0))
            except (TypeError, ValueError, KeyError) as exc:
                candidate_since = None
                write_json_line(
                    log, {"event": "invalid_sample", "at": at, "error": str(exc), "raw": telemetry}
                )
                if args.once:
                    return 1
                time.sleep(args.interval_s)
                continue

            supply_good = telemetry.get("supply_good") is True
            battery_type = telemetry.get("battery_type")
            if supply_good and battery_ma >= args.armed_ma:
                seen_bulk = True
            peak_ma = max(peak_ma, battery_ma)

            candidate = (
                seen_bulk
                and battery_type == "Generic_LFP"
                and supply_good
                and supply_v >= 4.5
                and battery_v >= args.min_v
                and abs(battery_ma) <= args.taper_ma
            )
            now_mono = time.monotonic()
            if candidate:
                if candidate_since is None:
                    candidate_since = now_mono
                held_s = now_mono - candidate_since
            else:
                candidate_since = None
                held_s = 0.0

            state = "TAPER" if candidate else ("CHARGING" if seen_bulk else "ARMING")
            sample = {
                "event": "sample",
                "at": at,
                "state": state,
                "seen_bulk": seen_bulk,
                "peak_battery_ma": round(peak_ma, 1),
                "candidate": candidate,
                "held_s": round(held_s, 1),
                "battery_type": battery_type,
                "battery_v": battery_v,
                "battery_ma": battery_ma,
                "supply_good": supply_good,
                "supply_v": supply_v,
                "supply_ma": telemetry.get("supply_ma"),
                "fixture_id": telemetry.get("fixture_id"),
                "fw": telemetry.get("fw"),
                "soc_pct": telemetry.get("soc_pct"),
            }
            last_sample = sample
            write_json_line(log, sample)
            print(
                f"{at} {state:8s} batt={battery_v:.3f} V {battery_ma:+.1f} mA "
                f"supply={supply_v:.3f} V good={int(supply_good)} "
                f"hold={held_s:.0f}/{args.hold_s:.0f} s",
                flush=True,
            )

            if args.once:
                return 0

            if candidate and held_s >= args.hold_s:
                complete = {
                    "event": "complete",
                    "at": at,
                    "reason": "sustained_lfp_cv_taper",
                    "sample": sample,
                    "thresholds": {
                        "min_v": args.min_v,
                        "taper_ma": args.taper_ma,
                        "armed_ma": args.armed_ma,
                        "hold_s": args.hold_s,
                    },
                }
                write_json_line(log, complete)
                done_file.parent.mkdir(parents=True, exist_ok=True)
                try:
                    with done_file.open("x", encoding="utf-8", newline="\n") as done:
                        json.dump(complete, done, sort_keys=True, indent=2)
                        done.write("\n")
                except FileExistsError:
                    write_json_line(
                        log,
                        {"event": "done_file_exists", "at": iso_now(), "path": str(done_file)},
                    )
                message = (
                    f"Gotion charge tapered: {battery_v:.3f} V, {battery_ma:+.0f} mA "
                    f"for {args.hold_s:.0f} s. Disconnect and rest the cell >=1 hour."
                )
                print("CHARGE COMPLETE: " + message, flush=True)
                if not args.no_alert:
                    errors = send_completion_alert(message)
                    for error in errors:
                        write_json_line(log, {"event": "alert_error", "at": iso_now(), "error": error})
                return 0

            elapsed = time.monotonic() - loop_started
            time.sleep(max(0.1, args.interval_s - elapsed))
    except KeyboardInterrupt:
        write_json_line(
            log,
            {"event": "interrupted", "at": iso_now(), "last_sample": last_sample},
        )
        print("charge sentinel interrupted", flush=True)
        return 130
    finally:
        if link is not None:
            try:
                link.close()
            except Exception:
                pass
        log.close()


if __name__ == "__main__":
    raise SystemExit(main())
