#!/usr/bin/env python3
"""Pit-crew helper for targeted net_bench field-cycle OTA.

This wraps the bench steps that are easy to get subtly wrong:

1. Build a field-cycle peer image into a persistent, named build directory.
2. Send targeted shared-WiFi maintenance through the serial-bridge dashboard.
3. Discover the peer's maintenance IP by scanning /telemetry for fixture_id.
4. Wait out the sustained U<id> command tail before upload.
5. Run net_bench_ota.py and confirm the peer rejoins ESP-NOW.

It intentionally uses the existing shared-WiFi path, not maint-ap.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import ipaddress
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
SKETCH_DIR = ROOT / "firmware" / "net_bench"
BUILD_SH = SKETCH_DIR / "build.sh"
OTA_TOOL = ROOT / "ops" / "bench" / "net_bench_ota.py"
SOURCE = SKETCH_DIR / "net_bench.ino"


FIELD_DEFAULTS = {
    "field_charge_s": 300,
    "field_wait_s": 300,
    "field_protect_s": 900,
    "field_wake_ms": 8000,
    "field_cold_ms": 30000,
    "field_low_mv": 3100,
    "field_critical_mv": 3000,
    "field_low_confirm_s": 60,
    "capacity_mah": 6000,
    "charge_ma": 1500,
    "maintain_v": "4.6",
}


def normalize_peer_id(text: str) -> str:
    peer_id = text.strip().upper()
    if not re.fullmatch(r"[0-9A-F]{6}", peer_id):
        raise argparse.ArgumentTypeError("peer id must be 6 hex chars, e.g. 9F26F8")
    return peer_id


def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser(
        description="Build and OTA a targeted net_bench field-cycle peer image."
    )
    ap.add_argument("peer_id", type=normalize_peer_id)
    ap.add_argument("--site", default="ca")
    ap.add_argument("--channel", type=int, default=11)
    ap.add_argument("--hex-lit", type=int, default=18)
    ap.add_argument("--brightness", type=int, default=128)
    ap.add_argument("--field-charge-s", type=int, default=FIELD_DEFAULTS["field_charge_s"])
    ap.add_argument("--field-wait-s", type=int, default=FIELD_DEFAULTS["field_wait_s"])
    ap.add_argument("--field-protect-s", type=int, default=FIELD_DEFAULTS["field_protect_s"])
    ap.add_argument("--field-wake-ms", type=int, default=FIELD_DEFAULTS["field_wake_ms"])
    ap.add_argument("--field-cold-ms", type=int, default=FIELD_DEFAULTS["field_cold_ms"])
    ap.add_argument("--field-low-mv", type=int, default=FIELD_DEFAULTS["field_low_mv"])
    ap.add_argument("--field-critical-mv", type=int, default=FIELD_DEFAULTS["field_critical_mv"])
    ap.add_argument(
        "--field-low-confirm-s", type=int, default=FIELD_DEFAULTS["field_low_confirm_s"]
    )
    ap.add_argument("--capacity-mah", type=int, default=FIELD_DEFAULTS["capacity_mah"])
    ap.add_argument("--charge-ma", type=int, default=FIELD_DEFAULTS["charge_ma"])
    ap.add_argument("--maintain", default=FIELD_DEFAULTS["maintain_v"])
    ap.add_argument("--dashboard-url", default="http://127.0.0.1:8765")
    ap.add_argument(
        "--subnet",
        action="append",
        default=[],
        help="CIDR subnet to scan for /telemetry; repeatable. Default: auto + 192.168.4.0/24.",
    )
    ap.add_argument(
        "--ip",
        default=None,
        help="known maintenance IP; skips discovery but still validates fixture_id unless --trust-ip",
    )
    ap.add_argument("--discover-timeout", type=float, default=75.0)
    ap.add_argument("--probe-timeout", type=float, default=0.55)
    ap.add_argument("--probe-jobs", type=int, default=96)
    ap.add_argument(
        "--maint-tail-s",
        type=float,
        default=38.0,
        help="wait after sending U<id> before upload so the 35 s command tail expires",
    )
    ap.add_argument(
        "--maint-resend-s",
        type=float,
        default=30.0,
        help="re-send U<id> during discovery at this interval; 0 disables",
    )
    ap.add_argument(
        "--trust-ip",
        action="store_true",
        help="with --ip, skip fixture_id validation before OTA",
    )
    ap.add_argument("--verify-timeout", type=float, default=120.0)
    ap.add_argument("--fresh-age-ms", type=int, default=180000)
    ap.add_argument("--min-free-gb", type=float, default=2.0)
    ap.add_argument("--build-name", default=None)
    ap.add_argument("--bin", default=None, help="existing .bin; skips build")
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--build-only", action="store_true")
    ap.add_argument("--skip-maint", action="store_true")
    ap.add_argument("--skip-ota", action="store_true")
    ap.add_argument("--skip-verify", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument(
        "--expect-fw",
        default=None,
        help="firmware_rev expected after OTA; default parses NET_BENCH_VERSION",
    )
    ap.add_argument("--notes", default=None)
    return ap.parse_args()


def log(msg: str) -> None:
    print(msg, flush=True)


def require_file(path: Path) -> None:
    if not path.exists():
        raise SystemExit(f"missing required file: {path}")


def parse_source_version() -> str | None:
    text = SOURCE.read_text(encoding="utf-8", errors="replace")
    m = re.search(r'#define\s+NET_BENCH_VERSION\s+"([^"]+)"', text)
    return m.group(1) if m else None


def check_free_space(path: Path, min_gb: float) -> None:
    usage = shutil.disk_usage(path)
    free_gb = usage.free / (1024**3)
    if free_gb < min_gb:
        raise SystemExit(f"only {free_gb:.2f} GB free at {path}; need >= {min_gb:.2f} GB")
    log(f"disk preflight: {free_gb:.2f} GB free")


def build_name(args: argparse.Namespace) -> str:
    if args.build_name:
        return args.build_name
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    return f"field-cycle-peer-{stamp}-{args.peer_id}-hex{args.hex_lit}b{args.brightness}"


def build_args(args: argparse.Namespace) -> list[str]:
    return [
        "--role",
        "peer",
        "--channel",
        str(args.channel),
        "--field-cycle",
        "--field-charge-s",
        str(args.field_charge_s),
        "--field-wait-s",
        str(args.field_wait_s),
        "--field-protect-s",
        str(args.field_protect_s),
        "--field-wake-ms",
        str(args.field_wake_ms),
        "--field-cold-ms",
        str(args.field_cold_ms),
        "--field-low-mv",
        str(args.field_low_mv),
        "--field-critical-mv",
        str(args.field_critical_mv),
        "--field-low-confirm-s",
        str(args.field_low_confirm_s),
        "--field-led-load",
        "--drawdown-lit",
        str(args.hex_lit),
        "--drawdown-brightness",
        str(args.brightness),
        "--chem",
        "lfp",
        "--cap",
        str(args.capacity_mah),
        "--charge-ma",
        str(args.charge_ma),
        "--maintain",
        str(args.maintain),
    ]


def run(cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None, dry_run: bool = False) -> None:
    log("+ " + " ".join(str(x) for x in cmd))
    if dry_run:
        return
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env, check=True)


def build_image(args: argparse.Namespace) -> Path:
    require_file(BUILD_SH)
    name = build_name(args)
    build_rel = Path("build") / name
    build_abs = SKETCH_DIR / build_rel
    if build_abs.exists() and any(build_abs.iterdir()):
        raise SystemExit(f"build path already exists and is not empty: {build_abs}")

    env = os.environ.copy()
    env["ARDUINO_BUILD_PATH"] = build_rel.as_posix()
    cmd = ["bash", "./build.sh", *build_args(args)]
    run(cmd, cwd=SKETCH_DIR, env=env, dry_run=args.dry_run)
    if args.dry_run:
        return build_abs / "net_bench.ino.bin"

    bins = sorted(build_abs.rglob("*.ino.bin"), key=lambda p: p.stat().st_mtime, reverse=True)
    if not bins:
        raise SystemExit(f"build completed but no .ino.bin found under {build_abs}")
    log(f"build artifact: {bins[0]}")
    return bins[0]


def post_dashboard_command(dashboard_url: str, cmd: str, label: str, dry_run: bool) -> None:
    url = dashboard_url.rstrip("/") + "/api/cmd"
    payload = json.dumps({"cmd": cmd, "label": label}).encode("utf-8")
    log(f"dashboard command: {cmd} -> {url}")
    if dry_run:
        return
    req = urllib.request.Request(
        url,
        data=payload,
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=8) as resp:
        data = json.loads(resp.read().decode("utf-8"))
    if not data.get("ok"):
        raise SystemExit(f"dashboard rejected {cmd}: {data}")


def send_targeted_maintenance(args: argparse.Namespace) -> float:
    post_dashboard_command(
        args.dashboard_url,
        f"U{args.peer_id}",
        f"Target {args.peer_id} maintenance for field-cycle OTA",
        args.dry_run,
    )
    return time.time()


def fetch_json(url: str, timeout: float) -> dict[str, Any] | None:
    try:
        with urllib.request.urlopen(url, timeout=timeout) as resp:
            if resp.status != 200:
                return None
            return json.loads(resp.read().decode("utf-8", "replace"))
    except (OSError, ValueError, urllib.error.URLError, TimeoutError):
        return None


def local_ipv4s() -> set[str]:
    ips: set[str] = set()
    try:
        for ip in socket.gethostbyname_ex(socket.gethostname())[2]:
            ips.add(ip)
    except OSError:
        pass
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.settimeout(0.2)
        s.connect(("8.8.8.8", 80))
        ips.add(s.getsockname()[0])
        s.close()
    except OSError:
        pass
    return {ip for ip in ips if not ip.startswith("127.")}


def candidate_networks(subnets: list[str]) -> list[ipaddress.IPv4Network]:
    nets: list[ipaddress.IPv4Network] = []
    requested = subnets or ["auto"]
    for item in requested:
        if item == "auto":
            for ip in local_ipv4s():
                try:
                    nets.append(ipaddress.ip_network(f"{ip}/24", strict=False))
                except ValueError:
                    pass
            continue
        try:
            nets.append(ipaddress.ip_network(item, strict=False))
        except ValueError as exc:
            raise SystemExit(f"bad --subnet {item!r}: {exc}") from exc
    fallback = ipaddress.ip_network("192.168.4.0/24")
    if fallback not in nets:
        nets.append(fallback)

    unique: list[ipaddress.IPv4Network] = []
    seen: set[str] = set()
    for net in nets:
        key = str(net)
        if key not in seen:
            seen.add(key)
            unique.append(net)
    return unique


def telemetry_matches(peer_id: str, data: dict[str, Any]) -> bool:
    candidates = [
        data.get("fixture_id"),
        data.get("id"),
        data.get("peer_id"),
        data.get("node"),
    ]
    return any(str(v).strip().upper() == peer_id for v in candidates if v is not None)


def probe_ip(ip: str, peer_id: str, timeout: float) -> tuple[str, dict[str, Any]] | None:
    data = fetch_json(f"http://{ip}/telemetry", timeout)
    if data and telemetry_matches(peer_id, data):
        return ip, data
    return None


def discover_peer_ip(
    args: argparse.Namespace, last_maint_at: float | None
) -> tuple[str, dict[str, Any] | None, float | None]:
    if args.ip:
        log(f"using provided maintenance IP: {args.ip}")
        telemetry = fetch_json(f"http://{args.ip}/telemetry", args.probe_timeout)
        if args.trust_ip:
            if not telemetry:
                log("WARNING: --trust-ip set; /telemetry did not respond before OTA")
            elif not telemetry_matches(args.peer_id, telemetry):
                log(
                    "WARNING: --trust-ip set; /telemetry fixture_id did not match "
                    f"{args.peer_id}: {telemetry}"
                )
            return args.ip, telemetry, last_maint_at
        if not telemetry:
            raise SystemExit(f"{args.ip} did not serve /telemetry; use --trust-ip to override")
        if not telemetry_matches(args.peer_id, telemetry):
            raise SystemExit(
                f"{args.ip} /telemetry did not match {args.peer_id}; got {telemetry}"
            )
        return args.ip, telemetry, last_maint_at

    nets = candidate_networks(args.subnet)
    log("maintenance discovery subnets: " + ", ".join(str(n) for n in nets))
    deadline = time.time() + args.discover_timeout
    round_no = 0
    while time.time() < deadline:
        if (
            not args.skip_maint
            and args.maint_resend_s > 0
            and last_maint_at is not None
            and time.time() - last_maint_at >= args.maint_resend_s
        ):
            last_maint_at = send_targeted_maintenance(args)
        round_no += 1
        hosts = [str(host) for net in nets for host in net.hosts()]
        log(f"discovery pass {round_no}: probing {len(hosts)} hosts")
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.probe_jobs) as pool:
            futures = [pool.submit(probe_ip, ip, args.peer_id, args.probe_timeout) for ip in hosts]
            for fut in concurrent.futures.as_completed(futures):
                result = fut.result()
                if result:
                    ip, telemetry = result
                    log(f"found {args.peer_id} maintenance IP: {ip}")
                    return ip, telemetry, last_maint_at
        time.sleep(2.0)
    raise SystemExit(f"could not find {args.peer_id} /telemetry within {args.discover_timeout:.0f}s")


def wait_out_maintenance_tail(sent_at: float, tail_s: float, ip: str | None) -> None:
    remaining = sent_at + tail_s - time.time()
    if remaining <= 0:
        return
    log(f"waiting {remaining:.1f}s for targeted U command tail to expire before OTA")
    end = time.time() + remaining
    while time.time() < end:
        if ip:
            fetch_json(f"http://{ip}/telemetry", 0.5)
        time.sleep(min(2.0, max(0.0, end - time.time())))


def run_ota(args: argparse.Namespace, bin_path: Path, ip: str) -> None:
    require_file(OTA_TOOL)
    notes = args.notes or (
        f"{args.peer_id} field-cycle OTA hex-lit={args.hex_lit} brightness={args.brightness}"
    )
    cmd = [
        sys.executable,
        str(OTA_TOOL),
        "--bin",
        str(bin_path),
        "--nodes",
        f"{args.peer_id}={ip}",
        "--jobs",
        "1",
        "--reboot",
        "comms",
        "--site",
        args.site,
        "--notes",
        notes,
    ]
    run(cmd, cwd=ROOT, dry_run=args.dry_run)


def dashboard_state(dashboard_url: str) -> dict[str, Any] | None:
    return fetch_json(dashboard_url.rstrip("/") + "/api/state", 4.0)


def verify_rejoin(args: argparse.Namespace, expect_fw: str | None) -> None:
    if args.skip_verify:
        return
    log("waiting for peer to rejoin ESP-NOW via dashboard")
    deadline = time.time() + args.verify_timeout
    last: dict[str, Any] | None = None
    while time.time() < deadline:
        state = dashboard_state(args.dashboard_url)
        if state:
            peer = (state.get("peers") or {}).get(args.peer_id)
            if isinstance(peer, dict):
                last = peer
                age_ok = int(peer.get("age_ms", 10**9)) <= args.fresh_age_ms
                fw_ok = expect_fw is None or peer.get("firmware_rev") == expect_fw
                if age_ok and fw_ok:
                    log(
                        "verified rejoin: "
                        f"fw={peer.get('firmware_rev')} rr={peer.get('reset_reason')} "
                        f"bv={peer.get('battery_v')} phase={peer.get('field_phase')}"
                    )
                    return
        time.sleep(2.0)
    raise SystemExit(f"peer did not rejoin with expected state; last={last}")


def main() -> None:
    args = parse_args()
    require_file(SOURCE)
    check_free_space(ROOT, args.min_free_gb)

    expect_fw = args.expect_fw if args.expect_fw is not None else parse_source_version()
    if expect_fw:
        log(f"expected firmware_rev after OTA: {expect_fw}")

    if args.bin:
        bin_path = Path(args.bin).resolve()
        require_file(bin_path)
        if args.expect_fw is None and expect_fw:
            log(
                "WARNING: --bin uses NET_BENCH_VERSION from the current source for "
                "verification; pass --expect-fw when flashing an older/different bin"
            )
    elif args.skip_build:
        raise SystemExit("--skip-build requires --bin")
    else:
        bin_path = build_image(args)

    if args.build_only:
        log("build-only requested; stopping before maintenance/OTA")
        return
    if args.skip_ota:
        log("skip-ota requested; stopping before maintenance/OTA")
        return

    sent_at: float | None = None
    if not args.skip_maint:
        sent_at = send_targeted_maintenance(args)
    else:
        log("skip-maint requested; assuming peer is already in maintenance")
        sent_at = time.time() - args.maint_tail_s

    if args.dry_run:
        log("dry-run requested; stopping before discovery/OTA")
        return

    ip, telemetry, sent_at = discover_peer_ip(args, sent_at)
    if telemetry:
        log(
            "maintenance telemetry: "
            f"fw={telemetry.get('fw')} mode={telemetry.get('mode')} "
            f"bv={telemetry.get('battery_v')} reset={telemetry.get('reset_reason')}"
        )

    if sent_at is not None:
        wait_out_maintenance_tail(sent_at, args.maint_tail_s, ip)
    run_ota(args, bin_path, ip)
    verify_rejoin(args, expect_fw)


if __name__ == "__main__":
    main()
