#!/usr/bin/env python3
"""Run a guarded single-cell discharge on an EastTester ET5406A+.

The ET5406A+ provides the primary voltage cutoff in its battery-test mode. This
tool also records independent host-side Ah/Wh integrals and always attempts to
turn the channel off on exit.

Typical workflow:

  python ops/bench/et5406_discharge.py --port COM41 --status
  python ops/bench/et5406_discharge.py --port COM41 --arm
  python ops/bench/et5406_discharge.py --port COM41 --run --yes \
      --battery gotion-33140-15ah-sample-1 --ambient-c 25.0

The default test is 1.000 A CC to 2.500 V. At 15 Ah it takes roughly 15 hours.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import serial


COMMAND_DELAY_S = 0.25
LOW_RANGE_MAX_A = 3.0
ET5406_MAX_A = 20.0


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def finite_float(value: str, command: str) -> float:
    try:
        number = float(value)
    except ValueError as exc:
        raise RuntimeError(f"unexpected response to {command!r}: {value!r}") from exc
    if not math.isfinite(number):
        raise RuntimeError(f"non-finite response to {command!r}: {value!r}")
    return number


class ET5406:
    def __init__(self, port: str, baud: int, timeout_s: float = 1.5):
        self.port = port
        self.baud = baud
        self.serial = serial.Serial(port, baud, timeout=timeout_s)
        self.serial.reset_input_buffer()

    def close(self) -> None:
        self.serial.close()

    @staticmethod
    def _clean_response(raw: str) -> str:
        response = raw.strip()
        # Numeric query responses use an R prefix; strings usually do not.
        if response.startswith("R") and len(response) > 1:
            if response[1].isdigit() or response[1] in " +-.":
                response = response[1:].strip()
        return response

    def command(self, command: str) -> str:
        self.serial.write((command + "\n").encode("ascii"))
        self.serial.flush()
        time.sleep(COMMAND_DELAY_S)
        raw = self.serial.readline().decode("ascii", errors="replace")
        if not raw:
            raise RuntimeError(f"ET5406A+ did not respond to {command!r}")
        return self._clean_response(raw)

    def set(self, command: str) -> None:
        response = self.command(command)
        if "execu success" not in response.lower():
            raise RuntimeError(f"ET5406A+ rejected {command!r}: {response!r}")

    def query(self, command: str) -> str:
        return self.command(command)

    def query_float(self, command: str) -> float:
        return finite_float(self.query(command), command)

    def output_off(self) -> None:
        self.set("CH1:SW OFF")

    def output_is_on(self) -> bool:
        state = self.query("CH1:SW?").upper()
        if state not in {"ON", "OFF"}:
            raise RuntimeError(f"unexpected channel state: {state!r}")
        return state == "ON"

    def measurements(self) -> dict[str, float]:
        return {
            "voltage_v": self.query_float("MEAS1:VOLTAGE?"),
            "current_a": self.query_float("MEAS1:CURRENT?"),
            "power_w": self.query_float("MEAS1:POWER?"),
        }

    def status(self) -> dict[str, Any]:
        return {
            "id": self.query("*IDN?"),
            "output": self.query("CH1:SW?"),
            "mode": self.query("CH1:MODE?"),
            "voltage_range": self.query("LOAD1:VRANGE?"),
            "current_range": self.query("LOAD1:CRANGE?"),
            "abnormal": self.query("LOAD1:ABNO?"),
            "battery_mode": self.query("BATT1:MODE?"),
            "battery_cutoff_type": self.query("BATT1:BCUT?"),
            "battery_stages": self.query("BATT1:BAEN?"),
            "set_current_a": self.query_float("CURR1:BCC1?"),
            "cutoff_v": self.query_float("VOLT1:BCC1?"),
            "instrument_ah": self.query_float("BATT1:CAPA?"),
            "instrument_wh": self.query_float("BATT1:ENER?"),
            **self.measurements(),
        }


@dataclass
class Integrator:
    last_t: float | None = None
    last_v: float | None = None
    last_a: float | None = None
    ah: float = 0.0
    wh: float = 0.0
    ah_above_3v: float = 0.0
    wh_above_3v: float = 0.0

    def add(self, now: float, voltage_v: float, current_a: float) -> None:
        if self.last_t is None or self.last_v is None or self.last_a is None:
            self.last_t = now
            self.last_v = voltage_v
            self.last_a = current_a
            return

        dt_h = max(0.0, now - self.last_t) / 3600.0
        avg_a = (self.last_a + current_a) / 2.0
        avg_w = (self.last_v * self.last_a + voltage_v * current_a) / 2.0
        self.ah += avg_a * dt_h
        self.wh += avg_w * dt_h

        if self.last_v >= 3.0 and voltage_v >= 3.0:
            fraction = 1.0
        elif self.last_v < 3.0 and voltage_v < 3.0:
            fraction = 0.0
        elif voltage_v != self.last_v:
            crossing = (3.0 - self.last_v) / (voltage_v - self.last_v)
            fraction = crossing if self.last_v < 3.0 else 1.0 - crossing
            fraction = min(1.0, max(0.0, fraction))
        else:
            fraction = 0.0
        self.ah_above_3v += avg_a * dt_h * fraction
        self.wh_above_3v += avg_w * dt_h * fraction

        self.last_t = now
        self.last_v = voltage_v
        self.last_a = current_a


def configure(
    load: ET5406,
    current_a: float,
    cutoff_v: float,
    tail_current_a: float | None = None,
    tail_trigger_v: float | None = None,
) -> dict[str, Any]:
    current_range = "LOW" if current_a <= LOW_RANGE_MAX_A else "HIGH"
    ocp_a = min(ET5406_MAX_A, max(current_a * 1.20, current_a + 0.10))
    opp_w = max(10.0, current_a * 5.0)
    staged = tail_current_a is not None and tail_trigger_v is not None
    stages = 2 if staged else 1
    first_cutoff_v = tail_trigger_v if staged else cutoff_v
    load.output_off()
    load.set("LOAD1:VRANGE LOW")
    load.set(f"LOAD1:CRANGE {current_range}")
    load.set("VOLT1:VMAX 4.000")
    load.set(f"CURR1:IMAX {ocp_a:.3f}")
    load.set(f"POWE1:PMAX {opp_w:.2f}")
    load.set("BATT1:MODE CC")
    load.set("BATT1:BCUT V")
    load.set(f"BATT1:BAEN {stages}")
    load.set(f"CURR1:BCC1 {current_a:.3f}")
    load.set(f"VOLT1:BCC1 {first_cutoff_v:.3f}")
    if staged:
        load.set(f"CURR1:BCC2 {tail_current_a:.3f}")
        load.set(f"VOLT1:BCC2 {cutoff_v:.3f}")
    load.set("CH1:MODE BATT")

    checks = {
        "output": load.query("CH1:SW?"),
        "mode": load.query("CH1:MODE?"),
        "voltage_range": load.query("LOAD1:VRANGE?"),
        "current_range": load.query("LOAD1:CRANGE?"),
        "battery_mode": load.query("BATT1:MODE?"),
        "battery_cutoff_type": load.query("BATT1:BCUT?"),
        "battery_stages": load.query("BATT1:BAEN?"),
        "set_current_a": load.query_float("CURR1:BCC1?"),
        "cutoff_v": load.query_float("VOLT1:BCC1?"),
        "ovp_v": load.query_float("VOLT1:VMAX?"),
        "ocp_a": load.query_float("CURR1:IMAX?"),
        "opp_w": load.query_float("POWE1:PMAX?"),
    }
    expected = {
        "output": "OFF",
        "mode": "BATT",
        "voltage_range": "LOW",
        "current_range": current_range,
        "battery_mode": "CC",
        "battery_stages": str(stages),
    }
    for key, value in expected.items():
        if str(checks[key]).upper() != value:
            raise RuntimeError(f"configuration verification failed: {key}={checks[key]!r}")
    if not str(checks["battery_cutoff_type"]).lower().startswith("volt"):
        raise RuntimeError(
            "configuration verification failed: "
            f"battery_cutoff_type={checks['battery_cutoff_type']!r}"
        )
    if abs(checks["set_current_a"] - current_a) > 0.002:
        raise RuntimeError("configured current did not read back correctly")
    if abs(checks["cutoff_v"] - first_cutoff_v) > 0.002:
        raise RuntimeError("configured first cutoff did not read back correctly")
    if staged:
        checks["tail_current_a"] = load.query_float("CURR1:BCC2?")
        checks["final_cutoff_v"] = load.query_float("VOLT1:BCC2?")
        if abs(checks["tail_current_a"] - tail_current_a) > 0.002:
            raise RuntimeError("configured tail current did not read back correctly")
        if abs(checks["final_cutoff_v"] - cutoff_v) > 0.002:
            raise RuntimeError("configured final cutoff did not read back correctly")
    return checks


def default_output(site: str, battery: str) -> Path:
    here = Path(__file__).resolve().parent
    now = datetime.now(timezone.utc)
    safe_battery = "".join(c if c.isalnum() or c in "-_" else "-" for c in battery)
    filename = now.strftime("%Y-%m-%d-et5406-discharge-%H%M%SZ-") + safe_battery + ".jsonl"
    return here / "data" / site / filename


def print_status(status: dict[str, Any]) -> None:
    for key, value in status.items():
        print(f"{key:>24}: {value}")


def write_jsonl(handle: Any, row: dict[str, Any]) -> None:
    handle.write(json.dumps(row, sort_keys=True) + "\n")
    handle.flush()


def run_discharge(load: ET5406, args: argparse.Namespace, config: dict[str, Any]) -> int:
    initial = load.measurements()
    abnormal = load.query("LOAD1:ABNO?").upper()
    if abnormal not in {"NONE", "UN"}:
        raise RuntimeError(f"load reports {abnormal}; check polarity and wiring")
    if not (args.min_start_v <= initial["voltage_v"] <= args.max_start_v):
        raise RuntimeError(
            f"start voltage {initial['voltage_v']:.3f} V is outside "
            f"{args.min_start_v:.3f}..{args.max_start_v:.3f} V; "
            "do not start until a fully charged, rested single LFP cell is connected"
        )
    if initial["current_a"] > 0.02 or load.output_is_on():
        raise RuntimeError("channel is not safely off before start")

    output = Path(args.out).resolve() if args.out else default_output(args.site, args.battery)
    output.parent.mkdir(parents=True, exist_ok=True)
    run_id = output.stem
    metadata = {
        "type": "metadata",
        "ts_utc": utc_now(),
        "run_id": run_id,
        "instrument": load.query("*IDN?"),
        "port": args.port,
        "baud": args.baud,
        "battery": args.battery,
        "site": args.site,
        "operator": args.operator,
        "ambient_c": args.ambient_c,
        "notes": args.notes,
        "current_a": args.current_a,
        "cutoff_v": args.cutoff_v,
        "tail_current_a": args.tail_current_a,
        "tail_trigger_v": args.tail_trigger_v,
        "min_start_v": args.min_start_v,
        "max_start_v": args.max_start_v,
        "sample_s": args.sample_s,
        "max_hours": args.max_hours,
        "initial_open_circuit_v": initial["voltage_v"],
        "configuration": config,
    }

    if args.tail_current_a is None:
        print(f"starting {args.current_a:.3f} A CC discharge to {args.cutoff_v:.3f} V")
    else:
        print(
            f"starting {args.current_a:.3f} A CC to {args.tail_trigger_v:.3f} V, "
            f"then {args.tail_current_a:.3f} A to {args.cutoff_v:.3f} V"
        )
    print(f"output: {output}")
    start_mono = time.monotonic()
    integrator = Integrator()
    stop_reason = "unknown"
    initial_dcir_mohm: float | None = None
    instrument_ah: float | None = None
    instrument_wh: float | None = None
    samples = 0

    # Exclusive creation protects previous long-run data from accidental overwrite.
    with output.open("x", encoding="ascii", newline="\n") as handle:
        write_jsonl(handle, metadata)
        try:
            load.set("CH1:SW ON")
            time.sleep(1.5)
            if not load.output_is_on():
                raise RuntimeError("channel did not turn on")

            while True:
                sample_mono = time.monotonic()
                measured = load.measurements()
                instrument_ah = load.query_float("BATT1:CAPA?")
                instrument_wh = load.query_float("BATT1:ENER?")
                abnormal = load.query("LOAD1:ABNO?").upper()
                output_on = load.output_is_on()
                integrator.add(sample_mono, measured["voltage_v"], measured["current_a"])
                elapsed_s = sample_mono - start_mono

                if initial_dcir_mohm is None and measured["current_a"] >= 0.1:
                    initial_dcir_mohm = max(
                        0.0,
                        (initial["voltage_v"] - measured["voltage_v"])
                        / measured["current_a"]
                        * 1000.0,
                    )

                stage = "bulk"
                if args.tail_current_a is not None and measured["current_a"] <= (
                    args.current_a + args.tail_current_a
                ) / 2.0:
                    stage = "tail"

                row = {
                    "type": "sample",
                    "ts_utc": utc_now(),
                    "run_id": run_id,
                    "elapsed_s": round(elapsed_s, 3),
                    **measured,
                    "instrument_ah": instrument_ah,
                    "instrument_wh": instrument_wh,
                    "host_ah": integrator.ah,
                    "host_wh": integrator.wh,
                    "host_ah_above_3v": integrator.ah_above_3v,
                    "host_wh_above_3v": integrator.wh_above_3v,
                    "initial_dcir_mohm": initial_dcir_mohm,
                    "stage": stage,
                    "output_on": output_on,
                    "abnormal": abnormal,
                }
                write_jsonl(handle, row)
                samples += 1
                print(
                    f"{elapsed_s / 3600:6.3f} h  "
                    f"{measured['voltage_v']:5.3f} V  {measured['current_a']:5.3f} A  "
                    f"ET={instrument_ah:7.3f} Ah  host={integrator.ah:7.3f} Ah  "
                    f"state={'ON' if output_on else 'OFF'}  abnormal={abnormal}",
                    flush=True,
                )

                if abnormal not in {"NONE", "UN"}:
                    stop_reason = f"instrument-protection-{abnormal.lower()}"
                    break
                if not output_on:
                    stop_reason = "instrument-cutoff"
                    break
                if measured["voltage_v"] <= args.cutoff_v:
                    stop_reason = "host-voltage-cutoff"
                    break
                if elapsed_s >= args.max_hours * 3600.0:
                    stop_reason = "host-time-limit"
                    break
                sleep_s = max(0.0, args.sample_s - (time.monotonic() - sample_mono))
                time.sleep(sleep_s)
        except KeyboardInterrupt:
            stop_reason = "keyboard-interrupt"
        except Exception:
            stop_reason = "error"
            raise
        finally:
            off_error = None
            try:
                load.output_off()
            except Exception as exc:  # The internal voltage cutoff remains primary.
                off_error = repr(exc)
            final_measurement = {}
            try:
                final_measurement = load.measurements()
            except Exception as exc:
                final_measurement = {"measurement_error": repr(exc)}
            end = {
                "type": "end",
                "ts_utc": utc_now(),
                "run_id": run_id,
                "stop_reason": stop_reason,
                "elapsed_s": round(time.monotonic() - start_mono, 3),
                "samples": samples,
                "instrument_ah": instrument_ah,
                "instrument_wh": instrument_wh,
                "host_ah": integrator.ah,
                "host_wh": integrator.wh,
                "host_ah_above_3v": integrator.ah_above_3v,
                "host_wh_above_3v": integrator.wh_above_3v,
                "initial_dcir_mohm": initial_dcir_mohm,
                "output_off_error": off_error,
                "final_measurement": final_measurement,
            }
            write_jsonl(handle, end)

    print(
        f"done: {stop_reason}; ET={instrument_ah or 0.0:.3f} Ah, "
        f"host={integrator.ah:.3f} Ah, above 3.0 V={integrator.ah_above_3v:.3f} Ah"
    )
    print(f"channel forced OFF; data: {output}")
    return 0 if stop_reason in {"instrument-cutoff", "host-voltage-cutoff"} else 2


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="ET5406A+ serial port, e.g. COM41")
    parser.add_argument("--baud", type=int, default=9600)
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--status", action="store_true", help="read status only (default)")
    action.add_argument("--arm", action="store_true", help="configure test and leave output OFF")
    action.add_argument("--run", action="store_true", help="configure and start the discharge")
    parser.add_argument("--yes", action="store_true", help="required with --run")
    parser.add_argument("--current-a", type=float, default=1.0)
    parser.add_argument("--cutoff-v", type=float, default=2.5)
    parser.add_argument(
        "--tail-current-a",
        type=float,
        default=None,
        help="optional hardware stage-2 current after --tail-trigger-v",
    )
    parser.add_argument(
        "--tail-trigger-v",
        type=float,
        default=None,
        help="optional hardware stage-1 cutoff that transitions to tail current",
    )
    parser.add_argument("--sample-s", type=float, default=5.0)
    parser.add_argument("--max-hours", type=float, default=20.0)
    parser.add_argument("--min-start-v", type=float, default=3.30)
    parser.add_argument("--max-start-v", type=float, default=3.70)
    parser.add_argument("--battery", default="gotion-33140-15ah-sample-1")
    parser.add_argument("--site", default="ca")
    parser.add_argument("--operator", default="ben")
    parser.add_argument("--ambient-c", type=float, default=None)
    parser.add_argument("--notes", default="Gotion 33140 15 Ah qualification")
    parser.add_argument("--out", default=None)
    args = parser.parse_args()

    if not (0.05 <= args.current_a <= ET5406_MAX_A):
        parser.error(f"--current-a must be within 0.05..{ET5406_MAX_A:.0f} A")
    if (args.tail_current_a is None) != (args.tail_trigger_v is None):
        parser.error("--tail-current-a and --tail-trigger-v must be provided together")
    if args.tail_current_a is not None:
        if not (0.05 <= args.tail_current_a < args.current_a):
            parser.error("--tail-current-a must be >=0.05 A and below --current-a")
        if not (args.cutoff_v < args.tail_trigger_v < args.min_start_v):
            parser.error("--tail-trigger-v must be between cutoff and minimum start voltage")
    if not (2.0 <= args.cutoff_v <= 3.2):
        parser.error("--cutoff-v must be 2.0..3.2 V for a single LFP cell")
    if args.cutoff_v >= args.min_start_v:
        parser.error("--cutoff-v must be below --min-start-v")
    if args.min_start_v >= args.max_start_v:
        parser.error("--min-start-v must be below --max-start-v")
    if args.sample_s < 2.0:
        parser.error("--sample-s must be at least 2 seconds")
    if args.max_hours <= 0:
        parser.error("--max-hours must be positive")
    if args.run and not args.yes:
        parser.error("--run requires --yes after wiring and polarity have been checked")
    return args


def main() -> int:
    args = parse_args()
    load: ET5406 | None = None
    try:
        load = ET5406(args.port, args.baud)
        if not args.arm and not args.run:
            print_status(load.status())
            return 0

        config = configure(
            load,
            args.current_a,
            args.cutoff_v,
            args.tail_current_a,
            args.tail_trigger_v,
        )
        print("configured and verified; channel is OFF")
        print_status(config)
        if args.arm:
            return 0
        return run_discharge(load, args, config)
    except (serial.SerialException, RuntimeError, OSError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        if load is not None:
            try:
                load.output_off()
            except Exception:
                pass
        return 1
    finally:
        if load is not None:
            load.close()


if __name__ == "__main__":
    raise SystemExit(main())
