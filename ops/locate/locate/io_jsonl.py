"""Canonical pairwise-RSSI JSONL + roster contract, and net_bench adapters.

Pairwise contract (the interface a future firmware neighbor-table dump must emit;
one row per direction per aggregation window):

    {"ts_utc": "...", "tx": "9E5AF0", "rx": "9F2690", "rssi_dbm": -61,
     "n": 12, "n_expected": 50, "censored": false}

rssi_dbm is the window's CENSORING-CORRECTED median: packets below the receiver
floor are never received, so the plain survivor median is biased high (pairs look
closer than they are -- measured to warp the whole map). Since the sender beacons
at a fixed rate, the expected count is known and the true median is the
(n_expected/2)-th largest received sample; when the median rank itself was lost,
censored=true and rssi_dbm is only an upper bound (the solver treats the link as
"at least this far"). Emit rows through rows_from_directed() to get this right.

Roster JSON carries device identity, role, and any ToF-derived height:

    {"devices": [{"dev_id": "...", "role": "downlight",
                  "z_tof_m": 2.41, "z_sigma_m": 0.01}, ...]}

The net_bench adapter converts the existing star-topology bench logs (master <->
peer only; rows {src:"peer", peer_id, rssi_dbm, dl_rssi_dbm, ...}) into contract
rows -- format smoke test only, a star cannot feed a full localization.
"""

import json
from typing import Dict, List, Optional, Tuple

from .model import Device, LinkObs, ZAnchor
from .rssi import _directional_median, merge_pair_directions


def write_pairwise(path: str, rows: List[dict]):
    with open(path, "w") as fh:
        for r in rows:
            fh.write(json.dumps(r) + "\n")


def read_pairwise(path: str) -> List[dict]:
    rows = []
    with open(path) as fh:
        for line in fh:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    return rows


def pairwise_to_directed(
    rows: List[dict], id_to_idx: Dict[str, int]
) -> Dict[Tuple[int, int], List[float]]:
    """Contract rows -> directed sample dict for rssi.aggregate_directed().
    NOTE: drops censoring flags; prefer rows_to_links() for solving."""
    directed: Dict[Tuple[int, int], List[float]] = {}
    for r in rows:
        tx, rx = r.get("tx"), r.get("rx")
        if tx not in id_to_idx or rx not in id_to_idx:
            continue
        directed.setdefault((id_to_idx[tx], id_to_idx[rx]), []).append(float(r["rssi_dbm"]))
    return directed


def rows_from_directed(
    directed: Dict[Tuple[int, int], List[float]],
    dev_ids: List[str],
    expected: int = None,
    ts_utc: str = "",
) -> List[dict]:
    """Directed per-packet samples -> contract rows, one per direction, with the
    censoring-corrected median (see module docstring)."""
    rows = []
    for (tx, rx), samples in sorted(directed.items()):
        if not samples:
            continue
        med, cen = _directional_median(samples, expected)
        rows.append({
            "ts_utc": ts_utc, "tx": dev_ids[tx], "rx": dev_ids[rx],
            "rssi_dbm": round(med, 2), "n": len(samples),
            "n_expected": expected, "censored": cen,
        })
    return rows


def rows_to_links(rows: List[dict], id_to_idx: Dict[str, int]) -> List[LinkObs]:
    """Contract rows -> symmetrized LinkObs, honoring per-row censored flags.
    Each row is one directional aggregation window."""
    pair_dirs: Dict[Tuple[int, int], dict] = {}
    for r in rows:
        tx, rx = r.get("tx"), r.get("rx")
        if tx not in id_to_idx or rx not in id_to_idx:
            continue
        i, j = id_to_idx[tx], id_to_idx[rx]
        if i == j:
            continue
        key = (min(i, j), max(i, j))
        d = pair_dirs.setdefault(key, {})
        d.setdefault("medians", []).append(
            (float(r["rssi_dbm"]), bool(r.get("censored", False))))
        d["n"] = d.get("n", 0) + int(r.get("n", 1))
    return merge_pair_directions(pair_dirs)


def write_roster(path: str, devices: List[Device], anchors: List[ZAnchor]):
    z_by_idx = {a.idx: a for a in anchors}
    out = {"devices": []}
    for k, d in enumerate(devices):
        row = {"dev_id": d.dev_id, "role": d.role, "z_tof_m": None, "z_sigma_m": None}
        if k in z_by_idx:
            row["z_tof_m"] = round(z_by_idx[k].z_m, 4)
            row["z_sigma_m"] = round(z_by_idx[k].sigma_m, 4)
        out["devices"].append(row)
    with open(path, "w") as fh:
        json.dump(out, fh, indent=1)


def read_roster(path: str) -> Tuple[List[Device], List[ZAnchor]]:
    with open(path) as fh:
        doc = json.load(fh)
    devices, anchors = [], []
    for k, row in enumerate(doc["devices"]):
        devices.append(Device(dev_id=row["dev_id"], role=row["role"]))
        if row.get("z_tof_m") is not None:
            anchors.append(ZAnchor(idx=k, z_m=float(row["z_tof_m"]),
                                   sigma_m=float(row.get("z_sigma_m") or 0.05)))
    return devices, anchors


def from_net_bench(paths: List[str], master_id: str = "MASTER") -> List[dict]:
    """Adapter: net_bench star logs -> contract rows (smoke test only)."""
    out = []
    for path in paths:
        with open(path) as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                try:
                    r = json.loads(line)
                except json.JSONDecodeError:
                    continue
                peer = r.get("peer_id") or r.get("id")
                if not peer:
                    continue
                ts = r.get("ts_utc", "")
                if r.get("rssi_dbm") is not None or r.get("rssi") is not None:
                    out.append({"ts_utc": ts, "tx": peer, "rx": master_id,
                                "rssi_dbm": float(r.get("rssi_dbm", r.get("rssi"))), "n": 1})
                if r.get("dl_rssi_dbm") is not None:
                    out.append({"ts_utc": ts, "tx": master_id, "rx": peer,
                                "rssi_dbm": float(r["dl_rssi_dbm"]), "n": 1})
    return out
