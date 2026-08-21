import json
import os
import tempfile

import numpy as np
from scipy.spatial.distance import pdist
from scipy.stats import spearmanr

import locate_rssi_cloud as cloud


def _synthetic_records(seed=4):
    rng = np.random.default_rng(seed)
    theta = np.linspace(0, 2 * np.pi, 16, endpoint=False)
    truth = np.column_stack([np.cos(theta), 0.65 * np.sin(theta)])
    truth[::2, 0] *= 0.72
    tx_bias = rng.normal(0, 0.8, len(truth))
    rx_bias = rng.normal(0, 0.6, 10)
    rows = []
    for j in range(10):
        for i in range(len(truth)):
            if i == j:
                continue
            distance = np.linalg.norm(truth[i] - truth[j])
            rssi = -40 - 18 * np.log10(max(distance, 0.04)) + tx_bias[i] + rx_bias[j]
            rows.append({"tx": f"D{i:02d}", "rx": f"D{j:02d}",
                         "rssi_dbm": float(rssi)})
    return truth, rows


def test_read_and_aggregate_ignore_metadata():
    handle, path = tempfile.mkstemp(suffix=".jsonl")
    os.close(handle)
    try:
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(json.dumps({"schema": "metadata-only"}) + "\n")
            fh.write(json.dumps({"tx": "aa", "rx": "bb", "rssi_dbm": -50}) + "\n")
            fh.write(json.dumps({"tx": "AA", "rx": "BB", "rssi_dbm": -54}) + "\n")
        metadata, rows = cloud.read_observations(path)
        records = cloud.aggregate_observations(rows)
        assert len(metadata) == 1
        assert records == [{"tx": "AA", "rx": "BB", "rssi_dbm": -52.0,
                            "n_snapshots": 2, "mad_db": 2.0}]
    finally:
        os.unlink(path)


def test_ordinal_embedding_recovers_synthetic_relative_geometry():
    truth, records = _synthetic_records()
    ids, tx, rx, rssi = cloud.index_records(records)
    assert ids == [f"D{i:02d}" for i in range(len(truth))]
    mask = np.ones(len(records), dtype=bool)
    estimate, _, info = cloud.fit_embedding(
        len(ids), 2, tx, rx, rssi, mask, seed=12, min_gap_db=0.2,
        maxiter=300, starts=1,
    )
    rho = spearmanr(pdist(truth), pdist(estimate)).statistic
    assert info["success"]
    assert info["train_triplet_accuracy"] > 0.8
    assert rho > 0.7


def test_holdout_keeps_each_transmitter_constrained():
    _, records = _synthetic_records()
    _, tx, rx, _ = cloud.index_records(records)
    test = cloud.make_holdout(tx, rx, 0.4, seed=9)
    assert test.any()
    for dev in np.unique(tx):
        assert np.count_nonzero((tx == dev) & ~test) >= 2
