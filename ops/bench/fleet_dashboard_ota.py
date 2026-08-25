#!/usr/bin/env python3
"""Flash an explicit fixture batch through the CoreS3 dashboard bridge.

This is the fleet counterpart to ``field_cycle_ota.py``. It sends one targeted
maintenance command per named fixture, discovers all matching maintenance HTTP
endpoints in one subnet pass, uploads one immutable binary in parallel, and
then requires fresh exact-revision heartbeats through the pending-verify window.

The safety preflight accepts either a fixture at or above 2.5 V, or a low-VBAT
fixture at or above 2.2 V with a healthy >=4.6 V / >=50 mA external input. It
never invents targets from discovery: every fixture MAC must be named.
"""

from __future__ import annotations

import argparse
import csv
import concurrent.futures
import hashlib
import ipaddress
import json
from pathlib import Path
import re
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request


ROOT = Path(__file__).resolve().parents[2]
OTA_TOOL = Path(__file__).resolve().with_name("net_bench_ota.py")
REGISTRY = ROOT / "ops" / "fleet" / "registry.csv"
MAC_RE = re.compile(r"^[0-9A-F]{6}$")
SPECIAL_OTA_ROLES = {"magic_wand"}


def log(message: str) -> None:
    print(message, flush=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="OTA one immutable image to an explicit dashboard fixture batch."
    )
    parser.add_argument("--targets", required=True, help="comma-separated short MACs")
    parser.add_argument("--bin", required=True, help="immutable fixture .bin")
    parser.add_argument("--expect-fw", required=True, help="exact post-OTA firmware_rev")
    parser.add_argument(
        "--allow-special-target",
        action="append",
        default=[],
        metavar="SHORT_MAC",
        help=(
            "explicitly acknowledge one protected one-off target; protected targets "
            "must be flashed alone"
        ),
    )
    parser.add_argument("--dashboard-url", default="http://127.0.0.1:8765")
    parser.add_argument("--subnet", action="append", default=[])
    parser.add_argument("--discovery-timeout", type=float, default=150.0)
    parser.add_argument("--probe-timeout", type=float, default=0.55)
    parser.add_argument("--probe-jobs", type=int, default=96)
    parser.add_argument("--maint-tail-s", type=float, default=38.0)
    parser.add_argument("--maint-resend-s", type=float, default=20.0)
    parser.add_argument("--ota-jobs", type=int, default=6)
    parser.add_argument("--verify-timeout", type=float, default=240.0)
    parser.add_argument("--fresh-age-ms", type=int, default=5000)
    parser.add_argument(
        "--allow-stale-preflight",
        action="store_true",
        help="accept explicit sleeping targets using their last safe power sample",
    )
    parser.add_argument(
        "--allow-partial-discovery",
        action="store_true",
        help="after the deadline, flash and verify only named targets actually discovered",
    )
    parser.add_argument("--pending-verify-s", type=float, default=25.0)
    parser.add_argument("--site", default="ca")
    parser.add_argument("--notes", default="")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def protected_ota_targets(registry: Path = REGISTRY) -> dict[str, str]:
    """Return registry IDs whose dedicated hardware must not take a fleet image."""
    try:
        with registry.open(newline="", encoding="utf-8-sig") as handle:
            rows = list(csv.DictReader(handle))
    except OSError as exc:
        raise SystemExit(f"cannot read fleet registry {registry}: {exc}") from exc

    protected: dict[str, str] = {}
    for row in rows:
        fixture_id = (row.get("fixture_id") or "").strip().upper()
        role = (row.get("role") or "").strip()
        if role in SPECIAL_OTA_ROLES:
            if not MAC_RE.fullmatch(fixture_id):
                raise SystemExit(
                    f"bad protected fixture id {fixture_id!r} in {registry}"
                )
            protected[fixture_id] = role
    return protected


def validate_special_targets(
    targets: list[str], allowed: list[str], registry: Path = REGISTRY
) -> None:
    acknowledgements = {
        item.strip().upper() for item in allowed if item and item.strip()
    }
    if any(not MAC_RE.fullmatch(item) for item in acknowledgements):
        raise SystemExit("--allow-special-target must be a six-digit short MAC")
    unrelated = acknowledgements - set(targets)
    if unrelated:
        raise SystemExit(
            "--allow-special-target names a non-target: " + ",".join(sorted(unrelated))
        )

    protected = protected_ota_targets(registry)
    selected = {target: protected[target] for target in targets if target in protected}
    unacknowledged = set(selected) - acknowledgements
    if unacknowledged:
        target = sorted(unacknowledged)[0]
        raise SystemExit(
            f"{target} is protected as role={selected[target]}; flash its dedicated "
            f"artifact alone and repeat --allow-special-target {target}"
        )
    if selected and len(targets) != 1:
        details = ",".join(f"{target}:{role}" for target, role in sorted(selected.items()))
        raise SystemExit(
            f"protected one-off target must be the only OTA target ({details})"
        )


def fetch_json(url: str, timeout: float = 4.0) -> dict | None:
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            if response.status != 200:
                return None
            value = json.loads(response.read().decode("utf-8", "replace"))
            return value if isinstance(value, dict) else None
    except (OSError, ValueError, urllib.error.URLError, TimeoutError):
        return None


def dashboard_state(url: str) -> dict:
    state = fetch_json(url.rstrip("/") + "/api/state")
    if not state:
        raise SystemExit(f"dashboard did not answer at {url}")
    return state


def post_dashboard_command(url: str, target: str) -> None:
    endpoint = url.rstrip("/") + "/api/cmd"
    payload = json.dumps(
        {"cmd": f"U{target}", "label": f"Fleet OTA maintenance {target}"}
    ).encode("utf-8")
    request = urllib.request.Request(
        endpoint,
        data=payload,
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(request, timeout=8) as response:
        result = json.loads(response.read().decode("utf-8", "replace"))
    if not result.get("ok"):
        raise SystemExit(f"dashboard rejected U{target}: {result}")


def local_ipv4s() -> set[str]:
    addresses: set[str] = set()
    try:
        addresses.update(socket.gethostbyname_ex(socket.gethostname())[2])
    except OSError:
        pass
    try:
        probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        probe.settimeout(0.2)
        probe.connect(("8.8.8.8", 80))
        addresses.add(probe.getsockname()[0])
        probe.close()
    except OSError:
        pass
    return {address for address in addresses if not address.startswith("127.")}


def candidate_networks(requested: list[str]) -> list[ipaddress.IPv4Network]:
    networks: list[ipaddress.IPv4Network] = []
    for item in requested or ["auto"]:
        if item == "auto":
            networks.extend(
                ipaddress.ip_network(f"{address}/24", strict=False)
                for address in local_ipv4s()
            )
        else:
            networks.append(ipaddress.ip_network(item, strict=False))
    fallback = ipaddress.ip_network("192.168.4.0/24")
    if fallback not in networks:
        networks.append(fallback)
    return list(dict.fromkeys(networks))


def telemetry_id(data: dict) -> str | None:
    for key in ("fixture_id", "id", "peer_id", "node"):
        value = data.get(key)
        if value is not None:
            candidate = str(value).strip().upper()
            if MAC_RE.fullmatch(candidate):
                return candidate
    return None


def probe_ip(ip: str, targets: set[str], timeout: float) -> tuple[str, str, dict] | None:
    telemetry = fetch_json(f"http://{ip}/telemetry", timeout)
    if telemetry:
        fixture_id = telemetry_id(telemetry)
        if fixture_id in targets:
            return fixture_id, ip, telemetry
    return None


def discover_batch(
    args: argparse.Namespace, targets: list[str], sent_at: dict[str, float]
) -> dict[str, tuple[str, dict]]:
    networks = candidate_networks(args.subnet)
    hosts = [str(host) for network in networks for host in network.hosts()]
    found: dict[str, tuple[str, dict]] = {}
    deadline = time.monotonic() + args.discovery_timeout
    pass_number = 0
    log("maintenance discovery subnets: " + ", ".join(map(str, networks)))
    while time.monotonic() < deadline and len(found) != len(targets):
        missing = [target for target in targets if target not in found]
        now = time.monotonic()
        for target in missing:
            if now - sent_at[target] >= args.maint_resend_s:
                post_dashboard_command(args.dashboard_url, target)
                sent_at[target] = time.monotonic()
                log(f"maintenance resend: U{target}")
        pass_number += 1
        log(
            f"discovery pass {pass_number}: probing {len(hosts)} hosts "
            f"for {len(missing)} missing targets"
        )
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=args.probe_jobs
        ) as executor:
            futures = [
                executor.submit(probe_ip, host, set(missing), args.probe_timeout)
                for host in hosts
            ]
            for future in concurrent.futures.as_completed(futures):
                result = future.result()
                if result:
                    fixture_id, ip, telemetry = result
                    if fixture_id not in found:
                        found[fixture_id] = (ip, telemetry)
                        log(
                            f"found {fixture_id} at {ip}: "
                            f"fw={telemetry.get('fw')} bv={telemetry.get('battery_v')}"
                        )
        if len(found) != len(targets):
            time.sleep(2.0)
    missing = [target for target in targets if target not in found]
    if missing:
        if not args.allow_partial_discovery:
            raise SystemExit("maintenance discovery timed out for: " + ",".join(missing))
        log("deferred; not discovered and not flashed: " + ",".join(missing))
    return found


def preflight(args: argparse.Namespace, targets: list[str], binary: Path) -> None:
    if not binary.is_file() or binary.stat().st_size == 0:
        raise SystemExit(f"missing or empty binary: {binary}")
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    log(f"artifact: {binary} ({binary.stat().st_size} bytes, sha256={digest})")
    state = dashboard_state(args.dashboard_url)
    peers = state.get("peers") or {}
    errors: list[str] = []
    for target in targets:
        peer = peers.get(target)
        if not isinstance(peer, dict):
            errors.append(f"{target}: absent from dashboard")
            continue
        age_ms = int(peer.get("age_ms", 10**9))
        battery_v = float(peer.get("battery_v") or 0.0)
        supply_v = float(peer.get("supply_v") or 0.0)
        supply_ma = int(peer.get("supply_ma") or 0)
        supply_good = bool(peer.get("supply_good"))
        powered = battery_v >= 2.5 or (
            battery_v >= 2.2
            and supply_good
            and supply_v >= 4.6
            and supply_ma >= 50
        )
        if age_ms > args.fresh_age_ms and not args.allow_stale_preflight:
            errors.append(f"{target}: stale ({age_ms} ms)")
        if not powered:
            errors.append(
                f"{target}: unsafe power evidence "
                f"(VBAT={battery_v:.3f}, VBUS={supply_v:.3f}/{supply_ma} mA)"
            )
        log(
            f"preflight {target}: age={age_ms} ms VBAT={battery_v:.3f} V "
            f"VBUS={supply_v:.3f} V/{supply_ma} mA fw={peer.get('firmware_rev')}"
        )
    if errors:
        raise SystemExit("preflight refused:\n  " + "\n  ".join(errors))


def run_ota(
    args: argparse.Namespace, targets: list[str], binary: Path, found: dict
) -> None:
    nodes = ",".join(f"{target}={found[target][0]}" for target in targets)
    stamp = time.strftime("%Y%m%d-%H%M%S", time.gmtime())
    output = (
        Path(__file__).resolve().parent
        / "data"
        / args.site
        / f"{stamp}-fleet-ota-results.jsonl"
    )
    command = [
        sys.executable,
        str(OTA_TOOL),
        "--bin",
        str(binary),
        "--nodes",
        nodes,
        "--jobs",
        str(args.ota_jobs),
        "--reboot",
        "comms",
        "--site",
        args.site,
        "--out",
        str(output),
        "--notes",
        args.notes or f"explicit fleet batch {','.join(targets)}",
    ]
    log("uploading exact targets: " + nodes)
    completed = subprocess.run(command, cwd=ROOT, check=False)
    if completed.returncode != 0 or not output.is_file():
        raise SystemExit(f"OTA uploader failed with exit {completed.returncode}")
    results = [json.loads(line) for line in output.read_text().splitlines() if line.strip()]
    failures = [
        str(result.get("node"))
        for result in results
        if not result.get("recovered")
    ]
    if len(results) != len(targets) or failures:
        raise SystemExit(
            f"OTA upload acknowledgement failure: results={len(results)} "
            f"expected={len(targets)} failed={failures}"
        )


def verify_pending_window(args: argparse.Namespace, targets: list[str]) -> dict[str, dict]:
    log(
        "requiring fresh exact-revision heartbeats beyond the "
        f"{args.pending_verify_s:.0f} s pending-verify gate"
    )
    first_exact: set[str] = set()
    proven: dict[str, dict] = {}
    deadline = time.monotonic() + args.verify_timeout
    while time.monotonic() < deadline:
        state = dashboard_state(args.dashboard_url)
        peers = state.get("peers") or {}
        for target in targets:
            peer = peers.get(target)
            good = (
                isinstance(peer, dict)
                and int(peer.get("age_ms", 10**9)) <= args.fresh_age_ms
                and peer.get("firmware_rev") == args.expect_fw
            )
            if good:
                if target not in first_exact:
                    first_exact.add(target)
                    log(f"fresh exact revision: {target}")
                # Pending images are forbidden to deep-sleep. Therefore either
                # a fresh exact-revision heartbeat with uptime beyond the gate,
                # or a later boot whose reset reason is deepsleep, proves that
                # this image marked itself valid before sleeping.
                uptime_ms = int(peer.get("uptime_ms") or 0)
                slept_after_validation = peer.get("reset_reason") == "deepsleep"
                if (
                    target not in proven
                    and (
                        uptime_ms >= int(args.pending_verify_s * 1000)
                        or slept_after_validation
                    )
                ):
                    proven[target] = peer
                    log(
                        f"past pending-verify gate: {target} "
                        f"uptime={uptime_ms} rr={peer.get('reset_reason')}"
                    )
        if len(proven) == len(targets):
            for target in targets:
                peer = proven[target]
                log(
                    f"verified {target}: age={peer.get('age_ms')} ms "
                    f"uptime={peer.get('uptime_ms')} ms class={peer.get('fixture_class')} "
                    f"recovery={peer.get('recovery_state')}"
                )
            return proven
        time.sleep(2.0)
    missing = [target for target in targets if target not in proven]
    raise SystemExit("post-OTA pending-verify failed for: " + ",".join(missing))


def main() -> None:
    args = parse_args()
    targets = list(
        dict.fromkeys(part.strip().upper() for part in args.targets.split(",") if part.strip())
    )
    if not targets or any(not MAC_RE.fullmatch(target) for target in targets):
        raise SystemExit("--targets must contain comma-separated six-digit short MACs")
    validate_special_targets(targets, args.allow_special_target)
    binary = Path(args.bin).resolve()
    preflight(args, targets, binary)
    if args.dry_run:
        log("dry-run: preflight passed; no maintenance or OTA commands sent")
        return
    sent_at: dict[str, float] = {}
    for target in targets:
        post_dashboard_command(args.dashboard_url, target)
        sent_at[target] = time.monotonic()
        log(f"maintenance command sent: U{target}")
    found = discover_batch(args, targets, sent_at)
    targets = [target for target in targets if target in found]
    if not targets:
        raise SystemExit("no named targets were discovered; no OTA attempted")
    remaining = max(
        0.0,
        max(sent_at[target] + args.maint_tail_s for target in targets)
        - time.monotonic(),
    )
    if remaining:
        log(f"waiting {remaining:.1f} s for all maintenance command tails to expire")
        time.sleep(remaining)
    run_ota(args, targets, binary, found)
    verify_pending_window(args, targets)
    log(f"fleet batch complete: {len(targets)}/{len(targets)}")


if __name__ == "__main__":
    main()
