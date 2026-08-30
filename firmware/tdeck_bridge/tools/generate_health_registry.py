#!/usr/bin/env python3
"""Generate the Bridge OS physical-fleet roster from canonical CSV inputs."""

from __future__ import annotations

import csv
import hashlib
import json
import re
import sys
from pathlib import Path


SCRIPT = Path(__file__).resolve()
REPO = SCRIPT.parents[3]
DEFAULT_REGISTRY = REPO / "ops" / "fleet" / "registry.csv"
DEFAULT_CALLSIGNS = REPO / "ops" / "fleet" / "callsigns.csv"
DEFAULT_ROSTER = REPO / "ops" / "fleet" / "roster.csv"
REGISTRY_STATUSES = {
    "commissioned": "HealthRegistryStatus::COMMISSIONED",
    "commission_failed": "HealthRegistryStatus::COMMISSION_FAILED",
    "enumerated": "HealthRegistryStatus::ENUMERATED",
    "quarantined": "HealthRegistryStatus::QUARANTINED",
}
ROSTER_SCOPES = {
    "site": "HealthRosterScope::SITE",
    "camp": "HealthRosterScope::CAMP",
    "repair": "HealthRosterScope::REPAIR",
}
FIXTURE_TYPES = {
    "canopy": "downlight",
    "perimeter": "perimeter",
    "uplight": "uplight",
}


def fail(message: str) -> None:
    raise SystemExit(message)


def main() -> None:
    # Keep the checked-in header byte-stable when Windows Python is launched
    # from Git Bash by the native test wrapper.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(newline="\n")
    registry_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else DEFAULT_REGISTRY
    callsigns_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else DEFAULT_CALLSIGNS
    roster_path = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else DEFAULT_ROSTER
    registry_raw = registry_path.read_bytes()
    callsigns_raw = callsigns_path.read_bytes()
    roster_raw = roster_path.read_bytes()
    with registry_path.open(newline="", encoding="utf-8-sig") as handle:
        source_rows = list(csv.DictReader(handle))
    with callsigns_path.open(newline="", encoding="utf-8-sig") as handle:
        source_callsigns = list(csv.DictReader(handle))
    with roster_path.open(newline="", encoding="utf-8-sig") as handle:
        source_roster = list(csv.DictReader(handle))

    assignments: dict[str, dict[str, str]] = {}
    seen_callsigns: set[str] = set()
    for row in source_callsigns:
        callsign = row.get("callsign", "").strip()
        folded = callsign.casefold()
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9]{2,6}", callsign):
            fail(f"bad callsign: {callsign!r}")
        if folded in seen_callsigns:
            fail(f"duplicate callsign: {callsign!r}")
        seen_callsigns.add(folded)
        if row.get("category", "").strip() not in {"game", "pokemon", "pop"}:
            fail(f"bad callsign category for {callsign!r}")
        assignment = row.get("assignment", "").strip()
        fixture_id = row.get("fixture_id", "").strip().upper()
        if assignment == "spare":
            if fixture_id:
                fail(f"spare callsign has fixture id: {callsign!r}")
            continue
        if assignment != "assigned" or not re.fullmatch(r"[0-9A-F]{6}", fixture_id):
            fail(f"bad callsign assignment for {callsign!r}")
        if fixture_id in assignments:
            fail(f"duplicate callsign fixture_id: {fixture_id}")
        assignments[fixture_id] = row

    registry_by_id: dict[str, dict[str, str]] = {}
    known_fixture_ids: set[str] = set()
    for row in source_rows:
        if row.get("board") != "PowerFeather V2":
            continue
        fixture_id = row.get("fixture_id", "").strip().upper()
        mac = row.get("mac", "").replace(":", "").upper()
        if not re.fullmatch(r"[0-9A-F]{6}", fixture_id):
            fail(f"bad fixture_id in registry: {fixture_id!r}")
        if len(mac) != 12 or not mac.endswith(fixture_id):
            fail(f"MAC/fixture_id mismatch for {fixture_id}")
        if fixture_id in registry_by_id:
            fail(f"duplicate fixture_id in registry: {fixture_id}")
        known_fixture_ids.add(fixture_id)
        registry_by_id[fixture_id] = row

    rows: list[tuple[dict[str, str], dict[str, str]]] = []
    roster_ids: set[str] = set()
    for roster_row in source_roster:
        fixture_id = roster_row.get("fixture_id", "").strip().upper()
        fixture_type = roster_row.get("fixture_type", "").strip()
        scope = roster_row.get("roster_scope", "").strip()
        if not re.fullmatch(r"[0-9A-F]{6}", fixture_id):
            fail(f"bad fixture_id in roster: {fixture_id!r}")
        if fixture_id in roster_ids:
            fail(f"duplicate fixture_id in roster: {fixture_id}")
        if fixture_id not in registry_by_id:
            fail(f"roster fixture absent from registry: {fixture_id}")
        if fixture_type not in FIXTURE_TYPES:
            fail(f"bad fixture_type for {fixture_id}: {fixture_type!r}")
        if scope not in ROSTER_SCOPES:
            fail(f"bad roster_scope for {fixture_id}: {scope!r}")
        status = registry_by_id[fixture_id].get("status", "").strip()
        if status not in REGISTRY_STATUSES:
            fail(f"unsupported registry status for roster fixture {fixture_id}: {status!r}")
        roster_ids.add(fixture_id)
        rows.append((roster_row, registry_by_id[fixture_id]))

    rows.sort(key=lambda pair: int(pair[0]["fixture_id"], 16))
    missing = roster_ids - set(assignments)
    # Callsigns are permanent operator identity, including for quarantined and
    # retired physical fixtures. The roster selects current physical inventory;
    # only an assignment absent from the complete PowerFeather registry is
    # invalid.
    extra = set(assignments) - known_fixture_ids
    if missing:
        fail(f"physical roster fixtures missing callsigns: {sorted(missing)}")
    if extra:
        fail(f"callsigns assigned outside fixture registry: {sorted(extra)}")
    registry_digest = hashlib.sha256(registry_raw).hexdigest()
    callsigns_digest = hashlib.sha256(callsigns_raw).hexdigest()
    roster_digest = hashlib.sha256(roster_raw).hexdigest()

    print("#pragma once")
    print()
    print('#include "health_model.h"')
    print()
    print("// Generated by tools/generate_health_registry.py from")
    print("// ops/fleet/registry.csv, roster.csv, and callsigns.csv. Do not edit by hand.")
    print(f'static constexpr char kHealthRegistryCsvSha256[] = "{registry_digest}";')
    print(f'static constexpr char kCallsignsCsvSha256[] = "{callsigns_digest}";')
    print(f'static constexpr char kFleetRosterCsvSha256[] = "{roster_digest}";')
    print("static const HealthRegistryEntry kHealthRegistry[] = {")
    for roster_row, registry_row in rows:
        fixture_id = roster_row["fixture_id"].upper()
        octets = ", ".join(f"0x{fixture_id[i:i + 2]}" for i in range(0, 6, 2))
        status = REGISTRY_STATUSES[registry_row["status"].strip()]
        scope = ROSTER_SCOPES[roster_row["roster_scope"].strip()]
        cap_text = registry_row.get("battery_capacity_mah", "").strip()
        capacity = int(cap_text) if cap_text else 0
        callsign = json.dumps(assignments[fixture_id]["callsign"].strip(), ensure_ascii=True)
        role = json.dumps(FIXTURE_TYPES[roster_row["fixture_type"].strip()], ensure_ascii=True)
        print(
            f"    {{{{{octets}}}, {status}, {scope}, {capacity}, {callsign}, {role}}},"
        )
    print("};")
    print("static constexpr size_t kHealthRegistryCount =")
    print("    sizeof(kHealthRegistry) / sizeof(kHealthRegistry[0]);")


if __name__ == "__main__":
    main()
