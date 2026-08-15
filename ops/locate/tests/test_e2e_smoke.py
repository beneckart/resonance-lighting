"""End-to-end smoke: 152-device sim at benign noise -> files -> ingest -> solve.

Proves the sim/real abstraction boundary (the solver sees only the JSONL contract)
and pins the benign-regime acceptance bar. Chandelier is asserted separately: its
CAD slot spacing (~0.24 m) sits at the RSSI resolution floor even at benign noise,
so 100% there would be luck, not capability.
"""

import os
import tempfile
import time

import numpy as np

from locate.io_cad import load_cad
from locate.io_jsonl import (read_pairwise, read_roster, rows_from_directed,
                             rows_to_links, write_pairwise, write_roster)
from locate.metrics import assignment_accuracy, position_error_stats
from locate.model import PathLossParams
from locate.pipeline import SolveConfig, solve
from sim.rf import RfParams, simulate_rssi
from sim.scene import build_scene
from sim.tof import TofParams, make_anchors

HERE = os.path.dirname(os.path.abspath(__file__))
CAD_PATH = os.path.join(HERE, "..", "data", "fixtures-0.3.1.json")


def test_e2e_benign_noise_via_jsonl_round_trip():
    t0 = time.time()
    cad = load_cad(CAD_PATH, seed=1)
    scene = build_scene(cad, seed=1)
    assert len(scene.devices) == 152

    rf = RfParams(sigma_link_db=2.0, sigma_dev_db=2.0, p_fade=0.0, panel_mode="off")
    directed = simulate_rssi(scene.truth_pos, rf, seed=2)
    anchors_true = make_anchors(scene, TofParams(), seed=3)
    assert sum(1 for a in anchors_true if scene.devices[a.idx].role == "downlight") == 72
    assert sum(1 for a in anchors_true if scene.devices[a.idx].role == "perimeter") == 40

    # ---- round-trip through the on-disk contract (sim -> files -> ingest) ----
    dev_ids = [d.dev_id for d in scene.devices]
    rows = rows_from_directed(directed, dev_ids, expected=rf.k_packets)
    with tempfile.TemporaryDirectory() as td:
        pp, rp = os.path.join(td, "pairwise.jsonl"), os.path.join(td, "roster.json")
        write_pairwise(pp, rows)
        write_roster(rp, scene.devices, anchors_true)
        devices, anchors = read_roster(rp)
        id_to_idx = {d.dev_id: k for k, d in enumerate(devices)}
        links = rows_to_links(read_pairwise(pp), id_to_idx)

    # 3 surveyed beacons: the deployment plan's gauge anchor (without them the
    # rotational gauge rests on a ~2% cost margin and is flagged ambiguous)
    rng = np.random.default_rng(99)
    picks = rng.choice(len(devices), size=3, replace=False)
    known = {int(k): scene.truth_fixture[k] for k in picks}
    cfg = SolveConfig(pl_prior=PathLossParams(p0_dbm=-40.0, n=2.7),
                      known_assignments=known)
    res = solve(devices, links, anchors, cad, cfg)

    roles = [d.role for d in devices]
    acc = assignment_accuracy(res.assignment, scene.truth_fixture, roles, cad)
    err = position_error_stats(res.pos_m, scene.truth_pos, roles)
    dt = time.time() - t0

    # benign-regime bars (downlight nn spacing 0.77 m vs ~0.45 m median error
    # leaves a few neighbor swaps even here -- that is physics, not a defect)
    assert acc["per_role"]["downlight"] >= 0.94, acc["per_role"]["downlight"]
    for role in ("perimeter", "uplight"):
        assert acc["per_role"][role] >= 0.97, (role, acc["per_role"][role])
    assert acc["per_role"]["chandelier"] >= 0.3, acc["per_role"]["chandelier"]
    assert err["overall"]["median_m"] < 0.6, err["overall"]
    # generous: the workstation is shared and sweep campaigns load it heavily
    assert dt < 600, dt
