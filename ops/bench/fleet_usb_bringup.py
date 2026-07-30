#!/usr/bin/env python3
"""Inventory and commission bare PowerFeather V2 boards over native USB.

The commissioning path deliberately uploads an existing Arduino build directory.
It never compiles, so every board receives the same inspected artifact.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import csv
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit("pyserial is required: python -m pip install pyserial") from exc


ROOT = Path(__file__).resolve().parents[2]
SKETCH_DIR = ROOT / "firmware" / "net_bench"
DEFAULT_REGISTRY = ROOT / "ops" / "fleet" / "registry.csv"
POWERFEATHER_VID = 0x303A
POWERFEATHER_PID = 0x1001
FQBN = "esp32:esp32:esp32s3_powerfeather"
REGISTRY_FIELDS = [
    "fixture_id",
    "mac",
    "board",
    "chip_revision",
    "flash_mb",
    "psram_mb",
    "status",
    "firmware_rev",
    "firmware_sha256",
    "battery_chemistry",
    "battery_capacity_mah",
    "charge_limit_ma",
    "ota_profile",
    "ota_verified",
    "role",
    "install_location",
    "first_seen_utc",
    "last_verified_utc",
    "notes",
]
PRINT_LOCK = threading.Lock()


@dataclass(frozen=True)
class UsbBoard:
    port: str
    mac: str
    fixture_id: str
    description: str
    hwid: str


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def log(message: str) -> None:
    with PRINT_LOCK:
        print(message, flush=True)


def normalize_mac(value: str) -> str:
    compact = re.sub(r"[^0-9A-Fa-f]", "", value).upper()
    if len(compact) != 12:
        raise ValueError(f"invalid MAC: {value!r}")
    return ":".join(compact[i : i + 2] for i in range(0, 12, 2))


def discover() -> list[UsbBoard]:
    boards: list[UsbBoard] = []
    for port in list_ports.comports():
        if port.vid != POWERFEATHER_VID or port.pid != POWERFEATHER_PID:
            continue
        if not port.serial_number:
            log(f"warning: ignoring {port.device}: native USB serial/MAC is absent")
            continue
        mac = normalize_mac(port.serial_number)
        boards.append(
            UsbBoard(
                port=port.device.upper(),
                mac=mac,
                fixture_id=mac.replace(":", "")[-6:],
                description=port.description or "",
                hwid=port.hwid or "",
            )
        )
    return sorted(boards, key=lambda item: int(re.sub(r"\D", "", item.port) or 0))


def load_registry(path: Path) -> dict[str, dict[str, str]]:
    if not path.exists():
        return {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != REGISTRY_FIELDS:
            raise SystemExit(f"unexpected registry schema in {path}")
        return {row["mac"]: row for row in reader}


def write_registry(path: Path, rows: dict[str, dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=REGISTRY_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(
            rows[key] for key in sorted(rows, key=lambda mac: rows[mac]["fixture_id"])
        )
    os.replace(temporary, path)


def inventory_rows(
    boards: list[UsbBoard], registry: dict[str, dict[str, str]], timestamp: str
) -> None:
    for board in boards:
        if board.mac in registry:
            continue
        registry[board.mac] = {
            "fixture_id": board.fixture_id,
            "mac": board.mac,
            "board": "PowerFeather V2",
            "chip_revision": "",
            "flash_mb": "",
            "psram_mb": "",
            "status": "enumerated",
            "firmware_rev": "",
            "firmware_sha256": "",
            "battery_chemistry": "",
            "battery_capacity_mah": "",
            "charge_limit_ma": "",
            "ota_profile": "",
            "ota_verified": "false",
            "role": "",
            "install_location": "",
            "first_seen_utc": timestamp,
            "last_verified_utc": "",
            "notes": "",
        }


def open_event_log(path: Path, append: bool):
    path.parent.mkdir(parents=True, exist_ok=True)
    if append:
        if not path.exists() or path.stat().st_size == 0:
            raise SystemExit(f"--append requires a non-empty existing log: {path}")
        return path.open("a", encoding="utf-8", newline="\n")
    try:
        return path.open("x", encoding="utf-8", newline="\n")
    except FileExistsError as exc:
        raise SystemExit(f"refusing existing log (use --append): {path}") from exc


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def find_esptool() -> str:
    direct = shutil.which("esptool") or shutil.which("esptool.exe")
    if direct:
        return direct
    local = os.environ.get("LOCALAPPDATA")
    if local:
        candidates = sorted(
            (Path(local) / "Arduino15" / "packages" / "esp32" / "tools" / "esptool_py").glob(
                "*/esptool.exe"
            ),
            reverse=True,
        )
        if candidates:
            return str(candidates[0])
    raise SystemExit("could not find esptool or Arduino's bundled esptool.exe")


def run_checked(command: list[str], timeout: float) -> str:
    completed = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )
    if completed.returncode:
        tail = "\n".join(completed.stdout.splitlines()[-20:])
        raise RuntimeError(f"exit {completed.returncode}: {' '.join(command)}\n{tail}")
    return completed.stdout


def parse_flash_id(output: str) -> dict[str, Any]:
    def match(pattern: str) -> str | None:
        found = re.search(pattern, output, flags=re.IGNORECASE)
        return found.group(1) if found else None

    chip = match(r"Chip type:\s*([^\r\n]+)")
    revision = match(r"(?:Chip revision:\s*v?|revision\s+v)([0-9.]+)")
    flash_mb = match(r"Detected flash size:\s*(\d+)\s*MB")
    psram_mb = match(r"(?:Embedded\s+)?PSRAM:?\s*(\d+)\s*MB")
    return {
        "chip": chip.strip() if chip else None,
        "chip_revision": revision,
        "flash_mb": int(flash_mb) if flash_mb else None,
        "psram_mb": int(psram_mb) if psram_mb else None,
    }


def serial_telemetry(port: str, timeout_s: float = 14.0) -> dict[str, Any]:
    deadline = time.monotonic() + timeout_s
    last_send = 0.0
    buffer = ""
    with serial.Serial(port, 115200, timeout=0.2, write_timeout=1.0) as handle:
        handle.dtr = False
        handle.rts = False
        handle.reset_input_buffer()
        while time.monotonic() < deadline:
            now = time.monotonic()
            if now - last_send >= 2.0:
                handle.write(b"t\n")
                last_send = now
            chunk = handle.read(handle.in_waiting or 1).decode("utf-8", errors="replace")
            if not chunk:
                continue
            buffer += chunk
            lines = buffer.splitlines()
            if not buffer.endswith(("\n", "\r")):
                buffer = lines.pop() if lines else buffer
            else:
                buffer = ""
            for line in reversed(lines):
                line = line.strip()
                if not line.startswith("{") or '"fixture_id"' not in line:
                    continue
                try:
                    return json.loads(line)
                except json.JSONDecodeError:
                    continue
    raise RuntimeError(f"{port}: no telemetry JSON within {timeout_s:.0f}s")


def wifi_telemetry(port: str, expected_id: str, timeout_s: float = 28.0) -> tuple[str, dict[str, Any]]:
    deadline = time.monotonic() + timeout_s
    ip_address: str | None = None
    buffer = ""
    with serial.Serial(port, 115200, timeout=0.25, write_timeout=1.0) as handle:
        handle.dtr = False
        handle.rts = False
        handle.reset_input_buffer()
        handle.write(b"u\n")
        while time.monotonic() < deadline and not ip_address:
            chunk = handle.read(handle.in_waiting or 1).decode("utf-8", errors="replace")
            if not chunk:
                continue
            buffer += chunk
            found = re.search(r"maintenance WiFi up,\s*ip=(\d+\.\d+\.\d+\.\d+)", buffer)
            if found:
                ip_address = found.group(1)
    if not ip_address:
        raise RuntimeError(f"{port}: shared-WiFi maintenance did not report an IP")

    url = f"http://{ip_address}/telemetry"
    with urllib.request.urlopen(url, timeout=3.0) as response:
        payload = json.load(response)
    if payload.get("fixture_id") != expected_id:
        raise RuntimeError(
            f"{port}: WiFi endpoint ID {payload.get('fixture_id')} != {expected_id}"
        )
    try:
        urllib.request.urlopen(f"http://{ip_address}/resume", timeout=2.0).read()
    except (urllib.error.URLError, TimeoutError):
        # Firmware deliberately tears down WiFi while serving this GET; Windows may
        # observe that as a timeout/reset even though the resume request succeeded.
        pass
    return ip_address, payload


def commission_one(
    board: UsbBoard,
    *,
    esptool: str,
    arduino_cli: str,
    build_path: Path,
    expect_fw: str,
    binary_hash: str,
    wifi_check: bool,
    battery_chemistry: str,
    capacity_mah: int,
    charge_ma: int,
    maintain_v: float,
    allow_battery_present: bool,
    wifi_semaphore: threading.Semaphore | None,
) -> dict[str, Any]:
    result: dict[str, Any] = {
        "event": "commission",
        "timestamp_utc": utc_now(),
        **asdict(board),
        "firmware_sha256": binary_hash,
        "preflight_ok": False,
        "upload_ok": False,
        "serial_verify_ok": False,
        "wifi_verify_ok": False,
    }
    try:
        log(f"{board.port} {board.fixture_id}: flash-ID preflight")
        flash_output = run_checked([esptool, "--port", board.port, "flash-id"], timeout=45)
        result.update(parse_flash_id(flash_output))
        if not str(result.get("chip") or "").startswith("ESP32-S3"):
            raise RuntimeError(f"chip is {result.get('chip')}, expected ESP32-S3")
        if result.get("flash_mb") != 8:
            raise RuntimeError(f"physical flash is {result.get('flash_mb')} MiB, expected 8")
        if result.get("psram_mb") != 2:
            raise RuntimeError(f"physical PSRAM is {result.get('psram_mb')} MiB, expected 2")
        result["preflight_ok"] = True

        log(f"{board.port} {board.fixture_id}: upload exact artifact")
        run_checked(
            [
                arduino_cli,
                "upload",
                "--fqbn",
                FQBN,
                "--port",
                board.port,
                "--build-path",
                str(build_path),
                str(SKETCH_DIR),
            ],
            timeout=180,
        )
        result["upload_ok"] = True
        time.sleep(4.0)

        telemetry = serial_telemetry(board.port)
        result["telemetry"] = telemetry
        if telemetry.get("fixture_id") != board.fixture_id:
            raise RuntimeError(
                f"serial ID {telemetry.get('fixture_id')} != USB ID {board.fixture_id}"
            )
        if telemetry.get("fw") != expect_fw:
            raise RuntimeError(f"firmware {telemetry.get('fw')} != expected {expect_fw}")
        if not telemetry.get("pf_ready"):
            raise RuntimeError("PowerFeather Board.init did not pass")
        if telemetry.get("flash_bytes") != 8 * 1024 * 1024:
            raise RuntimeError(f"flash size is {telemetry.get('flash_bytes')}, expected 8 MiB")
        # The normal PowerFeather FQBN leaves PSRAM uninitialized, so the runtime
        # value is zero even though esptool's ROM/eFuse probe above detects the
        # physical 2 MiB. Reject an unexpected partial size, not the expected zero.
        if telemetry.get("psram_bytes") not in (0, 2 * 1024 * 1024):
            raise RuntimeError(f"unexpected initialized PSRAM size {telemetry.get('psram_bytes')}")
        if telemetry.get("battery_type") != battery_chemistry:
            raise RuntimeError(f"battery profile is {telemetry.get('battery_type')}")
        if telemetry.get("battery_capacity_mah") != capacity_mah:
            raise RuntimeError(
                f"capacity is {telemetry.get('battery_capacity_mah')} mAh, "
                f"expected {capacity_mah}"
            )
        if telemetry.get("charge_limit_ma") != charge_ma:
            raise RuntimeError(
                f"charge cap is {telemetry.get('charge_limit_ma')} mA, expected {charge_ma}"
            )
        live_maintain = telemetry.get("maintain_v")
        if not isinstance(live_maintain, (int, float)) or abs(live_maintain - maintain_v) > 0.05:
            raise RuntimeError(f"VINDPM is {live_maintain} V, expected {maintain_v:.1f} V")
        if (
            not allow_battery_present
            and (telemetry.get("battery_present") or telemetry.get("charging_enabled"))
        ):
            raise RuntimeError("bare-board safety check failed: battery/charging reported active")
        result["serial_verify_ok"] = True

        if wifi_check:
            assert wifi_semaphore is not None
            with wifi_semaphore:
                log(f"{board.port} {board.fixture_id}: shared-WiFi OTA endpoint")
                ip_address, wifi_payload = wifi_telemetry(board.port, board.fixture_id)
                result["wifi_ip_observed"] = ip_address
                result["wifi_telemetry"] = wifi_payload
                result["wifi_verify_ok"] = True
        log(f"{board.port} {board.fixture_id}: PASS")
    except Exception as exc:  # Per-board evidence must survive a partial batch failure.
        result["error"] = str(exc)
        log(f"{board.port} {board.fixture_id}: FAIL: {exc}")
    result["timestamp_complete_utc"] = utc_now()
    return result


def select_boards(
    boards: list[UsbBoard],
    registry: dict[str, dict[str, str]],
    ports: list[str] | None,
    pending_only: bool,
) -> list[UsbBoard]:
    selected = boards
    if ports:
        wanted = {value.upper() for value in ports}
        selected = [board for board in selected if board.port in wanted]
        missing = sorted(wanted - {board.port for board in selected})
        if missing:
            raise SystemExit(f"requested ports not present as PowerFeathers: {', '.join(missing)}")
    if pending_only:
        selected = [
            board
            for board in selected
            if registry.get(board.mac, {}).get("status") != "commissioned"
        ]
    return selected


def update_commissioned_registry(
    registry: dict[str, dict[str, str]],
    results: list[dict[str, Any]],
    args: argparse.Namespace,
    binary_hash: str,
) -> None:
    for result in results:
        row = registry[result["mac"]]
        prior_ota_verified = (
            row.get("ota_verified") == "true"
            and row.get("ota_profile") == args.ota_profile
        )
        if not (
            result.get("preflight_ok")
            and result.get("upload_ok")
            and result.get("serial_verify_ok")
        ):
            row["status"] = "commission_failed"
            row["notes"] = result.get("error", "commissioning failed")
            continue
        row.update(
            {
                "chip_revision": str(result.get("chip_revision") or ""),
                "flash_mb": str(result.get("flash_mb") or ""),
                "psram_mb": str(result.get("psram_mb") or ""),
                "status": "commissioned",
                "firmware_rev": args.expect_fw,
                "firmware_sha256": binary_hash,
                "battery_chemistry": args.battery_chemistry,
                "battery_capacity_mah": str(args.capacity_mah),
                "charge_limit_ma": str(args.charge_ma),
                "ota_profile": args.ota_profile,
                # A transient later stress-test failure does not erase a prior
                # successful OTA qualification, but a credential/profile change
                # remains unverified until the new endpoint is actually reached.
                "ota_verified": str(
                    prior_ota_verified or bool(result.get("wifi_verify_ok"))
                ).lower(),
                "last_verified_utc": result["timestamp_complete_utc"],
                "notes": "Bare-board USB commissioning passed"
                + ("; shared-WiFi OTA endpoint verified" if result.get("wifi_verify_ok") else ""),
            }
        )


def add_common_output_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--registry", type=Path, default=DEFAULT_REGISTRY)
    parser.add_argument("--out", type=Path, required=True, help="append-only JSONL evidence")
    parser.add_argument("--append", action="store_true")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    inventory = commands.add_parser("inventory", help="record currently enumerated boards")
    add_common_output_args(inventory)

    commission = commands.add_parser(
        "commission", help="preflight, upload an existing artifact, and verify"
    )
    add_common_output_args(commission)
    commission.add_argument("--build-path", type=Path, required=True)
    commission.add_argument(
        "--sketch-dir",
        default="net_bench",
        help="firmware/<dir> sketch whose prebuilt artifact is uploaded (e.g. fixture)",
    )
    commission.add_argument("--expect-fw", required=True)
    commission.add_argument("--expect-count", type=int, required=True)
    commission.add_argument("--ports", nargs="+")
    commission.add_argument("--pending-only", action="store_true")
    commission.add_argument("--max-parallel", type=int, default=12)
    commission.add_argument("--wifi-check", action="store_true")
    commission.add_argument(
        "--wifi-parallel",
        type=int,
        default=4,
        help="maximum simultaneous serial-to-WiFi verification transitions",
    )
    commission.add_argument("--battery-chemistry", default="Generic_LFP")
    commission.add_argument("--capacity-mah", type=int, default=6000)
    commission.add_argument("--charge-ma", type=int, default=500)
    commission.add_argument("--maintain-v", type=float, default=4.6)
    commission.add_argument("--ota-profile", default="BubbyNet")
    commission.add_argument(
        "--allow-battery-present",
        action="store_true",
        help="allow a mixed batch containing installed batteries; bare-board safety remains default",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    global SKETCH_DIR
    if getattr(args, "sketch_dir", None):
        SKETCH_DIR = ROOT / "firmware" / args.sketch_dir
        if not SKETCH_DIR.is_dir():
            raise SystemExit(f"no such sketch dir: {SKETCH_DIR}")
    registry_path = args.registry.resolve()
    registry = load_registry(registry_path)
    boards = discover()
    timestamp = utc_now()
    inventory_rows(boards, registry, timestamp)

    if args.command == "inventory":
        events = [
            {"event": "inventory", "timestamp_utc": timestamp, **asdict(board)}
            for board in boards
        ]
        with open_event_log(args.out.resolve(), args.append) as handle:
            for event in events:
                handle.write(json.dumps(event, separators=(",", ":")) + "\n")
        write_registry(registry_path, registry)
        for board in boards:
            status = registry[board.mac]["status"]
            log(f"{board.port:>5}  {board.mac}  {board.fixture_id}  {status}")
        log(f"{len(boards)} PowerFeather(s); registry: {registry_path}")
        return 0

    selected = select_boards(boards, registry, args.ports, args.pending_only)
    if len(selected) != args.expect_count:
        raise SystemExit(
            f"refusing batch: selected {len(selected)} board(s), expected {args.expect_count}"
        )
    if not 1 <= args.max_parallel <= 16:
        raise SystemExit("--max-parallel must be 1..16")
    if not 1 <= args.wifi_parallel <= 8:
        raise SystemExit("--wifi-parallel must be 1..8")

    build_path = args.build_path.resolve()
    binary = build_path / f"{SKETCH_DIR.name}.ino.bin"
    options = build_path / "build.options.json"
    if not binary.is_file() or not options.is_file():
        raise SystemExit(
            f"build path lacks {SKETCH_DIR.name}.ino.bin/build.options.json: {build_path}"
        )
    binary_hash = sha256(binary)
    arduino_cli = shutil.which("arduino-cli")
    if not arduino_cli:
        raise SystemExit("arduino-cli is not on PATH")
    esptool = find_esptool()
    wifi_semaphore = threading.Semaphore(args.wifi_parallel) if args.wifi_check else None

    log(f"selected {len(selected)} board(s); max parallel uploads={args.max_parallel}")
    if args.wifi_check:
        log(f"max parallel WiFi checks={args.wifi_parallel}")
    log(f"artifact {binary} sha256={binary_hash}")
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.max_parallel) as pool:
        futures = [
            pool.submit(
                commission_one,
                board,
                esptool=esptool,
                arduino_cli=arduino_cli,
                build_path=build_path,
                expect_fw=args.expect_fw,
                binary_hash=binary_hash,
                wifi_check=args.wifi_check,
                battery_chemistry=args.battery_chemistry,
                capacity_mah=args.capacity_mah,
                charge_ma=args.charge_ma,
                maintain_v=args.maintain_v,
                allow_battery_present=args.allow_battery_present,
                wifi_semaphore=wifi_semaphore,
            )
            for board in selected
        ]
        results = [future.result() for future in futures]

    with open_event_log(args.out.resolve(), args.append) as handle:
        for result in results:
            handle.write(json.dumps(result, separators=(",", ":")) + "\n")
    update_commissioned_registry(registry, results, args, binary_hash)
    write_registry(registry_path, registry)

    passed = sum(
        bool(item.get("preflight_ok") and item.get("upload_ok") and item.get("serial_verify_ok"))
        for item in results
    )
    wifi_passed = sum(bool(item.get("wifi_verify_ok")) for item in results)
    log(f"batch result: {passed}/{len(results)} USB commissioned; {wifi_passed} WiFi verified")
    return 0 if passed == len(results) and (not args.wifi_check or wifi_passed == passed) else 1


if __name__ == "__main__":
    sys.exit(main())
