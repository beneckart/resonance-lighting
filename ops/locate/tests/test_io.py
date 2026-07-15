import os
import tempfile

import numpy as np

from locate.io_cad import DOWNLIGHT_TOP_M, load_cad
from locate.io_jsonl import (from_net_bench, pairwise_to_directed, read_pairwise,
                             read_roster, write_pairwise, write_roster)
from locate.model import Device, ZAnchor

HERE = os.path.dirname(os.path.abspath(__file__))
CAD_PATH = os.path.join(HERE, "..", "data", "fixtures-0.3.1.json")


def test_load_cad_default_scale_puts_downlights_in_band():
    cad = load_cad(CAD_PATH)
    roles = [f.role for f in cad.fixtures]
    assert roles.count("downlight") == 78
    assert roles.count("uplight") == 24
    assert roles.count("chandelier") == 16
    assert roles.count("perimeter") == 40          # synthesized
    dz = np.array([f.pos_m[2] for f in cad.fixtures if f.role == "downlight"])
    assert abs(dz.max() - DOWNLIGHT_TOP_M) < 1e-9   # top of band = 10 ft
    # bulk of the downlights inside 7-10 ft (the export has a few low outliers)
    frac_in_band = np.mean((dz > 7 * 0.3048 * 0.95) & (dz < 10 * 0.3048 * 1.05))
    assert frac_in_band > 0.85, frac_in_band
    # duplicate-position groups found (known in export 0.3.1)
    assert len(cad.duplicate_groups) >= 5
    # perimeter ring geometry
    per = np.array([f.pos_m for f in cad.fixtures if f.role == "perimeter"])
    assert np.allclose(per[:, 2], 5 * 0.3048)


def test_patched_cad_rings_complete():
    # patch_cad_0.3.1.py moves the 6 trunk strays into the 6 ring holes
    # (Ben, 2026-07-13); each downlight ring must be 24 distinct positions,
    # nothing near the trunk axis, stacked duplicates still present
    import json
    path = os.path.join(HERE, "..", "data", "fixtures-0.3.1-patched.json")
    doc = json.load(open(path))
    P = np.array([f["position"] for f in doc["fixtures"] if f["role"] == "downlight"])
    assert len(P) == 78
    r = np.linalg.norm(P[:, :2], axis=1)
    assert (r > 5).all()
    for lo, hi in ((5, 35), (35, 46), (46, 60)):
        m = (r > lo) & (r < hi)
        distinct = {tuple(np.round(p, 3)) for p in P[m]}
        assert len(distinct) == 24, (lo, hi, len(distinct))
    assert "patched_2026-07-13" in doc["meta"]
    cad = load_cad(path)
    assert len(cad.duplicate_groups) >= 5    # stacks intentionally left


def test_load_cad_explicit_scale():
    cad = load_cad(CAD_PATH, scale=0.1, perimeter_n=0)
    assert cad.scale_m_per_unit == 0.1
    assert all(f.role != "perimeter" for f in cad.fixtures)


def test_pairwise_roster_round_trip():
    devices = [Device("A1", "downlight"), Device("B2", "perimeter"), Device("C3", "uplight")]
    anchors = [ZAnchor(idx=0, z_m=2.44, sigma_m=0.01), ZAnchor(idx=1, z_m=1.51, sigma_m=0.02)]
    rows = [
        {"ts_utc": "t0", "tx": "A1", "rx": "B2", "rssi_dbm": -55.0, "n": 10},
        {"ts_utc": "t0", "tx": "B2", "rx": "A1", "rssi_dbm": -57.0, "n": 9},
        {"ts_utc": "t0", "tx": "C3", "rx": "A1", "rssi_dbm": -61.0, "n": 4},
    ]
    with tempfile.TemporaryDirectory() as td:
        pp, rp = os.path.join(td, "p.jsonl"), os.path.join(td, "r.json")
        write_pairwise(pp, rows)
        write_roster(rp, devices, anchors)
        rows2 = read_pairwise(pp)
        devices2, anchors2 = read_roster(rp)
    assert rows2 == rows
    assert devices2 == devices
    assert len(anchors2) == 2 and anchors2[0].z_m == 2.44

    id_to_idx = {d.dev_id: k for k, d in enumerate(devices2)}
    directed = pairwise_to_directed(rows2, id_to_idx)
    assert directed[(0, 1)] == [-55.0]
    assert directed[(1, 0)] == [-57.0]
    assert directed[(2, 0)] == [-61.0]


def test_net_bench_adapter_smoke():
    with tempfile.TemporaryDirectory() as td:
        p = os.path.join(td, "nb.jsonl")
        with open(p, "w") as fh:
            fh.write('{"src":"peer","ts_utc":"t1","peer_id":"9F2690","rssi_dbm":-42,"dl_rssi_dbm":-44}\n')
            fh.write('{"src":"master","ts_utc":"t1","sendok":1}\n')      # ignored
            fh.write('{"t":0.93,"id":"9F2690","rssi":-19}\n')            # rangewalk row
        rows = from_net_bench([p])
    assert {"tx": "9F2690", "rx": "MASTER"}.items() <= rows[0].items()
    assert rows[0]["rssi_dbm"] == -42.0
    assert any(r["tx"] == "MASTER" and r["rssi_dbm"] == -44.0 for r in rows)
    assert any(r["rssi_dbm"] == -19.0 for r in rows)
