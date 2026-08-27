#!/usr/bin/env python3
"""Flash an explicit fixture batch through the T-Deck dashboard bridge.

This is the fleet counterpart to ``field_cycle_ota.py``. One bridge-owned roster
gathers every named target, an explicit FREEZE acknowledgement ends all gather
traffic, and only then does the host upload one immutable binary in parallel.
Fresh exact-revision heartbeats must survive the pending-verify window.

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
import secrets
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
CAMPAIGN_CAPACITY = 160
MAINT_PHASE_GATHER = 1
MAINT_PHASE_FROZEN = 2


class JobLedger:
    def __init__(self, path: Path, job_id: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        self.path = path
        self.job_id = job_id
        self.started = time.monotonic()
        self.handle = path.open("x", encoding="utf-8", newline="\n")

    def emit(
        self, phase: str, event: str, target: str | None = None, **fields: object
    ) -> None:
        row: dict[str, object] = {
            "ts_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "elapsed_s": round(time.monotonic() - self.started, 3),
            "job_id": self.job_id,
            "phase": phase,
            "event": event,
        }
        if target is not None:
            row["target"] = target
        row.update(fields)
        self.handle.write(json.dumps(row, sort_keys=True, default=str) + "\n")
        self.handle.flush()

    def close(self) -> None:
        self.handle.close()


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
    parser.add_argument(
        "--gather-cadence",
        choices=("ordinary", "protect"),
        default="ordinary",
        help="sleep cadence the gather must completely span",
    )
    parser.add_argument("--ordinary-sleep-s", type=float, default=120.0)
    parser.add_argument("--protect-sleep-s", type=float, default=900.0)
    parser.add_argument("--gather-margin-s", type=float, default=30.0)
    parser.add_argument(
        "--discovery-timeout",
        type=float,
        default=None,
        help="defaults to selected sleep cadence plus gather margin",
    )
    parser.add_argument("--probe-timeout", type=float, default=0.55)
    parser.add_argument("--probe-jobs", type=int, default=96)
    parser.add_argument("--campaign-status-timeout", type=float, default=30.0)
    parser.add_argument("--ota-jobs", type=int, default=6)
    parser.add_argument("--verify-timeout", type=float, default=240.0)
    parser.add_argument("--fresh-age-ms", type=int, default=5000)
    parser.add_argument(
        "--allow-stale-preflight",
        action="store_true",
        help="accept power evidence older than --max-preflight-age-s",
    )
    parser.add_argument("--max-preflight-age-s", type=float, default=3600.0)
    parser.add_argument(
        "--allow-partial-discovery",
        action="store_true",
        help="after the deadline, flash and verify only named targets actually discovered",
    )
    parser.add_argument("--pending-verify-s", type=float, default=25.0)
    parser.add_argument("--site", default="ca")
    parser.add_argument("--notes", default="")
    parser.add_argument("--job-out", help="exclusive-create path for unified job JSONL")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def gather_timing(args: argparse.Namespace) -> tuple[float, int]:
    sleep_s = (
        args.protect_sleep_s
        if args.gather_cadence == "protect"
        else args.ordinary_sleep_s
    )
    minimum_s = sleep_s + args.gather_margin_s
    timeout_s = minimum_s if args.discovery_timeout is None else args.discovery_timeout
    if sleep_s <= 0 or args.gather_margin_s < 0:
        raise SystemExit("sleep cadence must be positive and margin non-negative")
    if timeout_s < minimum_s:
        raise SystemExit(
            f"discovery timeout {timeout_s:.0f}s cannot span "
            f"{args.gather_cadence} cadence; need at least {minimum_s:.0f}s"
        )
    duration_s = int(timeout_s + args.campaign_status_timeout + 10.0 + 0.999)
    if duration_s > 3600:
        raise SystemExit("bridge campaign duration exceeds 3600 s")
    return timeout_s, duration_s


def default_ledger_path(args: argparse.Namespace, job_id: str) -> Path:
    if args.job_out:
        return Path(args.job_out).resolve()
    stamp = time.strftime("%Y%m%d-%H%M%S", time.gmtime())
    return (
        Path(__file__).resolve().parent
        / "data"
        / args.site
        / f"{stamp}-{job_id}-fleet-ota-job.jsonl"
    )


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


def post_dashboard_command(url: str, command: str, label: str | None = None) -> None:
    endpoint = url.rstrip("/") + "/api/cmd"
    payload = json.dumps(
        {"cmd": command, "label": label or command}
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
        raise SystemExit(f"dashboard rejected {command}: {result}")


def wait_campaign_status(
    args: argparse.Namespace,
    job_id: str,
    *,
    phase: int,
    active: bool,
    target_count: int,
) -> dict:
    deadline = time.monotonic() + args.campaign_status_timeout
    last: dict | None = None
    next_status_request = 0.0
    while time.monotonic() < deadline:
        now = time.monotonic()
        if now >= next_status_request:
            post_dashboard_command(
                args.dashboard_url,
                f"uS{job_id}",
                f"Fleet OTA status {job_id}",
            )
            # A 100+ fixture census can occupy the 115200-baud bridge link for
            # several seconds.  Do not turn a status wait into a serial-input
            # flood: one request per 10 seconds is ample and stays bounded.
            next_status_request = now + 10.0
        state = dashboard_state(args.dashboard_url)
        candidate = state.get("maintenance_campaign")
        if isinstance(candidate, dict):
            last = candidate
            if (
                candidate.get("job_id") == job_id
                and candidate.get("phase") == phase
                and bool(candidate.get("active")) == active
                and int(candidate.get("target_count") or 0) == target_count
                and int(
                    candidate.get("status_age_ms")
                    if candidate.get("status_age_ms") is not None
                    else 10**9
                )
                <= 2000
            ):
                return candidate
        time.sleep(0.25)
    raise SystemExit(
        f"bridge did not acknowledge campaign {job_id} phase={phase} "
        f"active={int(active)} targets={target_count}; last={last}"
    )


def begin_campaign(
    args: argparse.Namespace,
    job_id: str,
    duration_s: int,
    targets: list[str],
    ledger: JobLedger,
) -> dict:
    post_dashboard_command(
        args.dashboard_url,
        f"uB{job_id}:{duration_s}",
        f"Fleet OTA begin {job_id}",
    )
    for target in targets:
        post_dashboard_command(
            args.dashboard_url,
            f"uA{job_id}:{target}",
            f"Fleet OTA add {target}",
        )
        ledger.emit("GATHER", "target_added", target)
    status = wait_campaign_status(
        args,
        job_id,
        phase=MAINT_PHASE_GATHER,
        active=True,
        target_count=len(targets),
    )
    if int(status.get("cycle_ms") or 10**9) >= 3000:
        raise SystemExit(
            f"campaign cycle {status.get('cycle_ms')} ms does not fit the 3000 ms wake window"
        )
    ledger.emit("GATHER", "bridge_acknowledged", campaign=status)
    return status


def freeze_campaign(
    args: argparse.Namespace, job_id: str, target_count: int, ledger: JobLedger
) -> dict:
    post_dashboard_command(
        args.dashboard_url,
        f"uF{job_id}",
        f"Fleet OTA freeze {job_id}",
    )
    status = wait_campaign_status(
        args,
        job_id,
        phase=MAINT_PHASE_FROZEN,
        active=False,
        target_count=target_count,
    )
    ledger.emit("FREEZE", "bridge_acknowledged", campaign=status)
    return status


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
    args: argparse.Namespace,
    targets: list[str],
    timeout_s: float,
    ledger: JobLedger,
) -> dict[str, tuple[str, dict]]:
    networks = candidate_networks(args.subnet)
    hosts = [str(host) for network in networks for host in network.hosts()]
    found: dict[str, tuple[str, dict]] = {}
    deadline = time.monotonic() + timeout_s
    pass_number = 0
    log("maintenance discovery subnets: " + ", ".join(map(str, networks)))
    while time.monotonic() < deadline and len(found) != len(targets):
        missing = [target for target in targets if target not in found]
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
                        ledger.emit(
                            "DISCOVER",
                            "endpoint_found",
                            fixture_id,
                            ip=ip,
                            firmware_rev=telemetry.get("fw"),
                            battery_v=telemetry.get("battery_v"),
                        )
        if len(found) != len(targets):
            time.sleep(2.0)
    missing = [target for target in targets if target not in found]
    if missing:
        if not args.allow_partial_discovery:
            raise SystemExit("maintenance discovery timed out for: " + ",".join(missing))
        log("deferred; not discovered and not flashed: " + ",".join(missing))
        for target in missing:
            ledger.emit("DISCOVER", "deferred_not_discovered", target)
    return found


def preflight(
    args: argparse.Namespace, targets: list[str], binary: Path
) -> dict[str, dict]:
    if not binary.is_file() or binary.stat().st_size == 0:
        raise SystemExit(f"missing or empty binary: {binary}")
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    log(f"artifact: {binary} ({binary.stat().st_size} bytes, sha256={digest})")
    state = dashboard_state(args.dashboard_url)
    peers = state.get("peers") or {}
    errors: list[str] = []
    baselines: dict[str, dict] = {}
    for target in targets:
        peer = peers.get(target)
        if not isinstance(peer, dict):
            errors.append(f"{target}: absent from dashboard")
            continue
        baselines[target] = dict(peer)
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
        if (
            age_ms > int(args.max_preflight_age_s * 1000)
            and not args.allow_stale_preflight
        ):
            errors.append(
                f"{target}: power evidence too old ({age_ms} ms; "
                f"limit {args.max_preflight_age_s:.0f} s)"
            )
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
    return baselines


def maintenance_power_preflight(
    found: dict[str, tuple[str, dict]], ledger: JobLedger
) -> None:
    errors: list[str] = []
    for target, (ip, _) in list(found.items()):
        telemetry = fetch_json(f"http://{ip}/telemetry", 2.0)
        if not telemetry or telemetry_id(telemetry) != target:
            errors.append(f"{target}: maintenance endpoint no longer identity-ready at {ip}")
            continue
        found[target] = (ip, telemetry)
        battery_v = float(telemetry.get("battery_v") or 0.0)
        supply_v = float(telemetry.get("supply_v") or 0.0)
        supply_ma = int(telemetry.get("supply_ma") or 0)
        supply_good = bool(telemetry.get("supply_good"))
        powered = battery_v >= 2.5 or (
            battery_v >= 2.2
            and supply_good
            and supply_v >= 4.6
            and supply_ma >= 50
        )
        if not powered:
            errors.append(
                f"{target}: unsafe fresh maintenance power "
                f"(VBAT={battery_v:.3f}, VBUS={supply_v:.3f}/{supply_ma} mA)"
            )
        ledger.emit(
            "PREFLIGHT",
            "fresh_maintenance_power",
            target,
            ip=ip,
            battery_v=battery_v,
            supply_v=supply_v,
            supply_ma=supply_ma,
            supply_good=supply_good,
            maint_status=telemetry.get("maint_status"),
        )
    if errors:
        raise SystemExit("fresh maintenance preflight refused:\n  " + "\n  ".join(errors))


def run_ota(
    args: argparse.Namespace,
    targets: list[str],
    binary: Path,
    found: dict,
    ledger: JobLedger,
) -> tuple[list[dict], Path]:
    nodes = ",".join(f"{target}={found[target][0]}" for target in targets)
    stamp = time.strftime("%Y%m%d-%H%M%S", time.gmtime())
    output = (
        Path(__file__).resolve().parent
        / "data"
        / args.site
        / f"{stamp}-{ledger.job_id}-fleet-ota-results.jsonl"
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
    for target in targets:
        ledger.emit("UPLOAD", "started", target, ip=found[target][0])
    completed = subprocess.run(command, cwd=ROOT, check=False)
    if completed.returncode != 0 or not output.is_file():
        raise SystemExit(f"OTA uploader failed with exit {completed.returncode}")
    results = [json.loads(line) for line in output.read_text().splitlines() if line.strip()]
    for result in results:
        target = str(result.get("node") or "UNKNOWN")
        ledger.emit(
            "UPLOAD",
            "http_result",
            target,
            recovered=bool(result.get("recovered")),
            t_ack_s=result.get("t_ack_s"),
            error=result.get("error"),
            result_file=str(output),
        )
    returned = {str(result.get("node")) for result in results}
    for target in targets:
        if target not in returned:
            ledger.emit("UPLOAD", "missing_result", target, result_file=str(output))
    return results, output


def resume_endpoint(ip: str, timeout: float = 2.0) -> bool:
    try:
        with urllib.request.urlopen(f"http://{ip}/resume", timeout=timeout) as response:
            response.read()
            return response.status == 200
    except (OSError, urllib.error.URLError, TimeoutError):
        return False


def reconcile_and_resume(
    args: argparse.Namespace,
    targets: list[str],
    found: dict[str, tuple[str, dict]],
    results: list[dict],
    ledger: JobLedger,
) -> None:
    result_by_target = {str(result.get("node")): result for result in results}
    for target in targets:
        ip = found[target][0]
        result = result_by_target.get(target) or {}
        telemetry = fetch_json(f"http://{ip}/telemetry", 1.0)
        if telemetry and telemetry_id(telemetry) == target:
            firmware = telemetry.get("fw")
            ledger.emit(
                "UPLOAD",
                "endpoint_reconciled",
                target,
                ip=ip,
                firmware_rev=firmware,
                ota_state=telemetry.get("ota_state"),
                mode=telemetry.get("mode"),
                http_recovered=bool(result.get("recovered")),
            )
            if firmware == args.expect_fw and not result.get("recovered"):
                log(f"reconciled upload timeout from exact endpoint: {target}")
            resumed = resume_endpoint(ip)
            ledger.emit(
                "CLEANUP",
                "resume_requested" if resumed else "resume_unconfirmed",
                target,
                ip=ip,
                firmware_rev=firmware,
            )
        elif not result.get("recovered"):
            ledger.emit(
                "UPLOAD",
                "awaiting_mesh_reconciliation",
                target,
                ip=ip,
                error=result.get("error") or "no uploader result",
            )


def post_job_reset_seen(peer: dict, baseline: dict | None) -> bool:
    if baseline is None:
        return True
    try:
        if int(peer.get("uptime_ms")) < int(baseline.get("uptime_ms")):
            return True
    except (TypeError, ValueError):
        pass
    try:
        if int(peer.get("seq")) < int(baseline.get("seq")):
            return True
    except (TypeError, ValueError):
        pass
    return False


def verify_pending_window(
    args: argparse.Namespace,
    targets: list[str],
    baselines: dict[str, dict],
    ledger: JobLedger,
) -> tuple[dict[str, dict], list[str]]:
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
            firmware_age = 10**9
            if isinstance(peer, dict) and peer.get("firmware_rev_age_ms") is not None:
                firmware_age = int(peer["firmware_rev_age_ms"])
            good = (
                isinstance(peer, dict)
                and int(peer.get("age_ms", 10**9)) <= args.fresh_age_ms
                and firmware_age <= args.fresh_age_ms
                and peer.get("firmware_rev") == args.expect_fw
            )
            baseline = baselines.get(target)
            if good and baseline and baseline.get("firmware_rev") == args.expect_fw:
                good = post_job_reset_seen(peer, baseline)
            if good:
                if target not in first_exact:
                    first_exact.add(target)
                    log(f"fresh exact revision: {target}")
                    ledger.emit(
                        "VERIFY",
                        "fresh_exact_revision",
                        target,
                        uptime_ms=peer.get("uptime_ms"),
                        reset_reason=peer.get("reset_reason"),
                    )
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
                    ledger.emit(
                        "VERIFY",
                        "verified",
                        target,
                        uptime_ms=uptime_ms,
                        reset_reason=peer.get("reset_reason"),
                    )
        if len(proven) == len(targets):
            for target in targets:
                peer = proven[target]
                log(
                    f"verified {target}: age={peer.get('age_ms')} ms "
                    f"uptime={peer.get('uptime_ms')} ms class={peer.get('fixture_class')} "
                    f"recovery={peer.get('recovery_state')}"
                )
            return proven, []
        time.sleep(2.0)
    missing = [target for target in targets if target not in proven]
    for target in missing:
        ledger.emit("VERIFY", "failed", target)
    return proven, missing


def main() -> None:
    args = parse_args()
    targets = list(
        dict.fromkeys(part.strip().upper() for part in args.targets.split(",") if part.strip())
    )
    if not targets or any(not MAC_RE.fullmatch(target) for target in targets):
        raise SystemExit("--targets must contain comma-separated six-digit short MACs")
    if len(targets) > CAMPAIGN_CAPACITY:
        raise SystemExit(
            f"target roster has {len(targets)} fixtures; bridge capacity is {CAMPAIGN_CAPACITY}"
        )
    validate_special_targets(targets, args.allow_special_target)
    discovery_timeout, campaign_duration = gather_timing(args)
    binary = Path(args.bin).resolve()
    job_id = secrets.token_hex(4).upper()
    if job_id == "00000000":
        job_id = "00000001"
    ledger = JobLedger(default_ledger_path(args, job_id), job_id)
    log(f"OTA job {job_id}; ledger={ledger.path}")
    found: dict[str, tuple[str, dict]] = {}
    campaign_may_be_active = False
    campaign_frozen = False
    baselines: dict[str, dict] = {}
    selected: list[str] = []
    try:
        digest = hashlib.sha256(binary.read_bytes()).hexdigest() if binary.is_file() else None
        ledger.emit(
            "PLAN",
            "job_started",
            targets=targets,
            target_count=len(targets),
            artifact=str(binary),
            artifact_sha256=digest,
            expected_revision=args.expect_fw,
            gather_cadence=args.gather_cadence,
            discovery_timeout_s=discovery_timeout,
            campaign_duration_s=campaign_duration,
        )
        baselines = preflight(args, targets, binary)
        ledger.emit("PREFLIGHT", "passed", target_count=len(targets))
        if args.dry_run:
            ledger.emit("CLEANUP", "dry_run_complete")
            log("dry-run: preflight passed; no maintenance or OTA commands sent")
            return

        campaign_may_be_active = True
        begin_campaign(args, job_id, campaign_duration, targets, ledger)
        found = discover_batch(
            args, targets, discovery_timeout, ledger
        )
        freeze_campaign(args, job_id, len(targets), ledger)
        campaign_frozen = True

        selected = [target for target in targets if target in found]
        if not selected:
            raise SystemExit("no named targets were discovered; no OTA attempted")
        selected_found = {target: found[target] for target in selected}
        maintenance_power_preflight(selected_found, ledger)
        found.update(selected_found)
        ledger.emit("PREFLIGHT", "fresh_maintenance_power_passed", count=len(selected))

        results, result_file = run_ota(args, selected, binary, found, ledger)
        ledger.emit("UPLOAD", "batch_finished", result_file=str(result_file))
        reconcile_and_resume(args, selected, found, results, ledger)

        proven, missing = verify_pending_window(
            args, selected, baselines, ledger
        )
        deferred = [target for target in targets if target not in selected]
        ledger.emit(
            "CLEANUP",
            "job_finished",
            verified=sorted(proven),
            deferred=deferred,
            failed=missing,
        )
        log(
            f"fleet batch complete: verified={len(proven)} "
            f"deferred={len(deferred)} failed={len(missing)}"
        )
        if missing:
            raise SystemExit("post-OTA pending-verify failed for: " + ",".join(missing))
    except BaseException as exc:
        ledger.emit("CLEANUP", "job_error", error=str(exc))
        raise
    finally:
        if campaign_may_be_active and not campaign_frozen:
            try:
                freeze_campaign(args, job_id, len(targets), ledger)
                campaign_frozen = True
            except BaseException as exc:
                ledger.emit("CLEANUP", "freeze_unconfirmed", error=str(exc))
        for target, (ip, _) in found.items():
            if resume_endpoint(ip, 0.75):
                ledger.emit("CLEANUP", "final_resume_requested", target, ip=ip)
        ledger.close()


if __name__ == "__main__":
    main()
