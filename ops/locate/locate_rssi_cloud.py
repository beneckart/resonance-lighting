#!/usr/bin/env python3
"""Recover a relative point cloud from directed RSSI observations alone.

This is the deliberately unanchored sibling of ``locate_run.py``.  It consumes
only ``tx``, ``rx``, and ``rssi_dbm`` from pairwise JSONL; it does not read a
roster, fixture class, ToF, CAD, rig dimensions, known positions, or path-loss
calibration.  The result is therefore a dimensionless ordinal embedding, unique
only up to translation, rotation/reflection, and scale.

The solver turns each reporter's RSSI ordering into triplets of the form
``distance(near, reporter) < distance(far, reporter)``.  A regularized
per-transmitter bias absorbs stable radio-power differences.  Cross-validation
holds out directed links before triplet construction and compares dimensions by
their ability to predict the hidden RSSI after fitting additive radio biases.

Example:

  python ops/locate/locate_rssi_cloud.py capture.jsonl \
      --out ops/locate/data/field/capture-rssi-cloud.json
"""

import argparse
from collections import defaultdict
import hashlib
import json
import math
import os

import numpy as np
from scipy.linalg import orthogonal_procrustes
from scipy.optimize import minimize
from scipy.spatial.distance import pdist, squareform
from scipy.stats import spearmanr


TRIPLET_OFFSETS = (1, 2, 4, 8, 16, 32)


def read_observations(path):
    """Return metadata and valid directed RSSI rows from a JSONL capture."""
    metadata = []
    rows = []
    with open(path, encoding="utf-8") as fh:
        for line_no, line in enumerate(fh, 1):
            line = line.strip()
            if not line:
                continue
            try:
                row = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"{path}:{line_no}: invalid JSON: {exc}") from exc
            if all(k in row for k in ("tx", "rx", "rssi_dbm")):
                tx = str(row["tx"]).upper()
                rx = str(row["rx"]).upper()
                if tx == rx:
                    continue
                rows.append({
                    "tx": tx,
                    "rx": rx,
                    "rssi_dbm": float(row["rssi_dbm"]),
                    "elapsed_s": (None if row.get("elapsed_s") is None
                                  else float(row["elapsed_s"])),
                })
            else:
                metadata.append(row)
    if not rows:
        raise ValueError(f"{path}: no rows with tx/rx/rssi_dbm")
    return metadata, rows


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def aggregate_observations(rows):
    """Median repeated EWMA snapshots into one value per directed pair."""
    grouped = defaultdict(list)
    for row in rows:
        grouped[(row["tx"], row["rx"])].append(row["rssi_dbm"])
    records = []
    for (tx, rx), values in sorted(grouped.items()):
        arr = np.asarray(values, dtype=float)
        med = float(np.median(arr))
        mad = float(np.median(np.abs(arr - med)))
        records.append({
            "tx": tx,
            "rx": rx,
            "rssi_dbm": med,
            "n_snapshots": int(len(arr)),
            "mad_db": mad,
        })
    return records


def index_records(records):
    ids = sorted({r["tx"] for r in records} | {r["rx"] for r in records})
    id_to_idx = {dev_id: idx for idx, dev_id in enumerate(ids)}
    tx = np.asarray([id_to_idx[r["tx"]] for r in records], dtype=int)
    rx = np.asarray([id_to_idx[r["rx"]] for r in records], dtype=int)
    rssi = np.asarray([r["rssi_dbm"] for r in records], dtype=float)
    return ids, tx, rx, rssi


def make_holdout(tx, rx, fraction, seed):
    """Hold out links independently within each reporter's radial ordering."""
    rng = np.random.default_rng(seed)
    test = np.zeros(len(tx), dtype=bool)
    if fraction <= 0:
        return test
    for reporter in np.unique(rx):
        idx = np.flatnonzero(rx == reporter)
        rng.shuffle(idx)
        n_test = min(max(1, int(round(fraction * len(idx)))), max(0, len(idx) - 8))
        test[idx[:n_test]] = True

    # Every transmitter needs at least two training reporters.  Return arbitrary
    # held-out links to training when a sparse future capture violates that gate.
    for dev in np.unique(tx):
        idx = np.flatnonzero(tx == dev)
        while np.count_nonzero(~test[idx]) < min(2, len(idx)):
            candidates = idx[test[idx]]
            if not len(candidates):
                break
            test[candidates[0]] = False
    return test


def make_triplets(tx, rx, rssi, edge_mask, min_gap_db=1.0):
    """Build ordinal constraints from reporter-local RSSI ranks."""
    near, reporter, far = [], [], []
    for center in np.unique(rx[edge_mask]):
        idx = np.flatnonzero(edge_mask & (rx == center))
        order = idx[np.argsort(rssi[idx])[::-1]]
        for offset in TRIPLET_OFFSETS:
            if offset >= len(order):
                continue
            a = order[:-offset]
            b = order[offset:]
            keep = (rssi[a] - rssi[b]) >= min_gap_db
            near.extend(tx[a[keep]])
            reporter.extend(np.full(np.count_nonzero(keep), center, dtype=int))
            far.extend(tx[b[keep]])
    if not near:
        raise ValueError("no ordinal triplets; lower --min-gap-db or inspect capture")
    return (np.asarray(near, dtype=int), np.asarray(reporter, dtype=int),
            np.asarray(far, dtype=int))


def _fingerprint_init(n_dev, ndim, tx, rx, rssi, train_mask, seed):
    """SVD of double-centered RSSI fingerprints; random fill for spare axes."""
    reporters = np.unique(rx)
    col = {dev: k for k, dev in enumerate(reporters)}
    M = np.full((n_dev, len(reporters)), np.nan)
    for k in np.flatnonzero(train_mask):
        M[tx[k], col[rx[k]]] = rssi[k]

    col_med = np.nanmedian(M, axis=0)
    overall = float(np.nanmedian(M))
    col_med = np.where(np.isfinite(col_med), col_med, overall)
    missing = ~np.isfinite(M)
    M[missing] = np.broadcast_to(col_med, M.shape)[missing]
    M -= M.mean(axis=1, keepdims=True)
    M -= M.mean(axis=0, keepdims=True)
    U, S, _ = np.linalg.svd(M, full_matrices=False)
    take = min(ndim, len(S))
    X = np.zeros((n_dev, ndim))
    X[:, :take] = U[:, :take] * np.sqrt(np.maximum(S[:take], 1e-9))
    if take < ndim:
        X[:, take:] = np.random.default_rng(seed).normal(scale=0.05,
                                                          size=(n_dev, ndim - take))
    X -= X.mean(axis=0, keepdims=True)
    rms = math.sqrt(float(np.mean(np.sum(X * X, axis=1))))
    return X / max(rms, 1e-9)


def _normalize_with_pullback(flat, n_dev, ndim):
    raw = flat.reshape(n_dev, ndim)
    centered = raw - raw.mean(axis=0, keepdims=True)
    rms = math.sqrt(float(np.sum(centered * centered) / n_dev) + 1e-12)
    return centered / rms, centered, rms


def _pullback_normalization(grad_x, centered, rms):
    n_dev = len(centered)
    dot = float(np.sum(grad_x * centered))
    grad = grad_x / rms - centered * dot / (n_dev * rms ** 3)
    return grad - grad.mean(axis=0, keepdims=True)


def _ordinal_objective(params, n_dev, ndim, triplets, bias_ridge, margin):
    n_pos = n_dev * ndim
    X, centered, rms = _normalize_with_pullback(params[:n_pos], n_dev, ndim)
    bias_raw = params[n_pos:]
    bias = bias_raw - bias_raw.mean()
    near, reporter, far = triplets

    dn = X[near] - X[reporter]
    df = X[far] - X[reporter]
    delta = margin + np.sum(dn * dn, axis=1) - np.sum(df * df, axis=1) \
        - bias[near] + bias[far]
    loss_each = np.logaddexp(0.0, delta)
    prob = np.exp(-np.logaddexp(0.0, -delta)) / len(delta)

    grad_x = np.zeros_like(X)
    np.add.at(grad_x, near, 2.0 * prob[:, None] * dn)
    np.add.at(grad_x, far, -2.0 * prob[:, None] * df)
    np.add.at(grad_x, reporter, 2.0 * prob[:, None] * (df - dn))

    grad_b = np.zeros(n_dev)
    np.add.at(grad_b, near, -prob)
    np.add.at(grad_b, far, prob)
    loss = float(np.mean(loss_each) + bias_ridge * np.mean(bias * bias))
    grad_b += (2.0 * bias_ridge / n_dev) * bias
    grad_b -= grad_b.mean()

    grad_raw = _pullback_normalization(grad_x, centered, rms)
    grad = np.concatenate([grad_raw.ravel(), grad_b])
    return loss, grad


def fit_embedding(n_dev, ndim, tx, rx, rssi, train_mask, seed=7,
                  min_gap_db=1.0, bias_ridge=0.02, margin=0.05,
                  maxiter=700, starts=2, initial=None):
    triplets = make_triplets(tx, rx, rssi, train_mask, min_gap_db=min_gap_db)
    best = None
    for start in range(starts):
        if initial is not None and start == 0:
            X0 = np.asarray(initial, dtype=float)
            if X0.shape != (n_dev, ndim):
                raise ValueError("initial embedding shape does not match")
        elif start == 0:
            X0 = _fingerprint_init(n_dev, ndim, tx, rx, rssi, train_mask, seed)
        else:
            X0 = np.random.default_rng(seed + 1009 * start).normal(size=(n_dev, ndim))
        p0 = np.concatenate([X0.ravel(), np.zeros(n_dev)])
        result = minimize(
            _ordinal_objective, p0,
            args=(n_dev, ndim, triplets, bias_ridge, margin),
            jac=True, method="L-BFGS-B",
            options={"maxiter": maxiter, "ftol": 1e-11, "gtol": 1e-7,
                     "maxls": 40},
        )
        if best is None or result.fun < best.fun:
            best = result
    n_pos = n_dev * ndim
    X, _, _ = _normalize_with_pullback(best.x[:n_pos], n_dev, ndim)
    bias = best.x[n_pos:] - np.mean(best.x[n_pos:])
    near, reporter, far = triplets
    score_gap = (bias[near] - np.sum((X[near] - X[reporter]) ** 2, axis=1)
                 - bias[far] + np.sum((X[far] - X[reporter]) ** 2, axis=1))
    info = {
        "loss": float(best.fun),
        "success": bool(best.success),
        "message": str(best.message),
        "iterations": int(best.nit),
        "n_triplets": int(len(near)),
        "train_triplet_accuracy": float(np.mean(score_gap > 0)),
    }
    return X, bias, info


def _design_matrix(n_dev, reporters, tx, rx, log_distance, edge_mask,
                   include_distance=True):
    reporter_to_col = {dev: k for k, dev in enumerate(reporters)}
    idx = np.flatnonzero(edge_mask)
    # Intercept + TX effects + RX effects.  Ridge regularization handles the
    # deliberately redundant gauges and makes prediction code straightforward.
    width = 1 + n_dev + len(reporters) + int(include_distance)
    A = np.zeros((len(idx), width))
    A[:, 0] = 1.0
    A[np.arange(len(idx)), 1 + tx[idx]] = 1.0
    A[np.arange(len(idx)), 1 + n_dev + np.asarray([reporter_to_col[x]
                                                   for x in rx[idx]])] = 1.0
    if include_distance:
        A[:, -1] = log_distance[idx]
    return idx, A


def fit_link_predictor(X, tx, rx, rssi, train_mask, include_distance=True,
                       ridge=1.0):
    dist = np.linalg.norm(X[tx] - X[rx], axis=1)
    log_distance = np.log(np.maximum(dist, 1e-4))
    reporters = np.unique(rx)
    idx, A = _design_matrix(len(X), reporters, tx, rx, log_distance, train_mask,
                            include_distance=include_distance)
    penalty = np.eye(A.shape[1]) * ridge
    penalty[0, 0] = 0.0
    if include_distance:
        penalty[-1, -1] = 1e-6
    coef = np.linalg.solve(A.T @ A + penalty, A.T @ rssi[idx])
    return coef, reporters, log_distance


def predict_links(coef, reporters, log_distance, n_dev, tx, rx,
                  include_distance=True):
    mask = np.ones(len(tx), dtype=bool)
    _, A = _design_matrix(n_dev, reporters, tx, rx, log_distance, mask,
                          include_distance=include_distance)
    return A @ coef


def _heldout_rank_score(X, tx, rx, rssi, test_mask):
    vals = []
    for reporter in np.unique(rx[test_mask]):
        idx = np.flatnonzero(test_mask & (rx == reporter))
        if len(idx) < 4 or np.ptp(rssi[idx]) == 0:
            continue
        d = np.linalg.norm(X[tx[idx]] - X[reporter], axis=1)
        corr = spearmanr(rssi[idx], -d).statistic
        if np.isfinite(corr):
            vals.append(float(corr))
    return float(np.median(vals)) if vals else float("nan")


def evaluate_embedding(X, tx, rx, rssi, train_mask, test_mask):
    coef, reporters, logd = fit_link_predictor(X, tx, rx, rssi, train_mask, True)
    pred = predict_links(coef, reporters, logd, len(X), tx, rx, True)
    base_coef, base_reporters, base_logd = fit_link_predictor(
        X, tx, rx, rssi, train_mask, False)
    base_pred = predict_links(base_coef, base_reporters, base_logd, len(X), tx, rx,
                              False)
    if not np.any(test_mask):
        test_mask = train_mask
    rmse = float(np.sqrt(np.mean((pred[test_mask] - rssi[test_mask]) ** 2)))
    base_rmse = float(np.sqrt(np.mean((base_pred[test_mask] - rssi[test_mask]) ** 2)))
    corr = spearmanr(rssi[test_mask], pred[test_mask]).statistic
    return {
        "heldout_rmse_db": rmse,
        "additive_baseline_rmse_db": base_rmse,
        "distance_rmse_gain_db": base_rmse - rmse,
        "heldout_prediction_spearman": float(corr),
        "heldout_reporter_rank_spearman_median": _heldout_rank_score(
            X, tx, rx, rssi, test_mask),
        "fitted_log_distance_slope_db": float(coef[-1]),
    }


def dimension_sweep(n_dev, dims, tx, rx, rssi, train_mask, test_mask, args):
    out = []
    for ndim in dims:
        X, _, fit = fit_embedding(
            n_dev, ndim, tx, rx, rssi, train_mask,
            seed=args.seed + 37 * ndim, min_gap_db=args.min_gap_db,
            bias_ridge=args.bias_ridge, margin=args.margin,
            maxiter=args.maxiter, starts=args.starts,
        )
        metrics = evaluate_embedding(X, tx, rx, rssi, train_mask, test_mask)
        out.append({"dimensions": int(ndim), **fit, **metrics})
    return out


def align_embedding(reference, candidate):
    """Orthogonally align a centered, normalized candidate to a reference."""
    ref = reference - reference.mean(axis=0, keepdims=True)
    cand = candidate - candidate.mean(axis=0, keepdims=True)
    ref /= max(math.sqrt(float(np.mean(np.sum(ref * ref, axis=1)))), 1e-12)
    cand /= max(math.sqrt(float(np.mean(np.sum(cand * cand, axis=1)))), 1e-12)
    rotation, _ = orthogonal_procrustes(cand, ref)
    aligned = cand @ rotation
    displacement = np.linalg.norm(aligned - ref, axis=1)
    return aligned, displacement


def temporal_stability(rows, ids, X_full, args):
    elapsed = np.asarray([r["elapsed_s"] for r in rows
                          if r.get("elapsed_s") is not None], dtype=float)
    if len(elapsed) < 10:
        return None, None
    split = float(np.median(elapsed))
    halves = []
    aligned_halves = []
    for label, predicate in (("early", lambda x: x <= split),
                             ("late", lambda x: x > split)):
        part = [r for r in rows if r.get("elapsed_s") is not None
                and predicate(r["elapsed_s"])]
        records = aggregate_observations(part)
        if len(records) < len(ids) * 4:
            return None, None
        id_to_idx = {dev_id: idx for idx, dev_id in enumerate(ids)}
        tx = np.asarray([id_to_idx[r["tx"]] for r in records], dtype=int)
        rx = np.asarray([id_to_idx[r["rx"]] for r in records], dtype=int)
        rssi = np.asarray([r["rssi_dbm"] for r in records], dtype=float)
        mask = np.ones(len(records), dtype=bool)
        X, _, fit = fit_embedding(
            len(ids), 3, tx, rx, rssi, mask,
            seed=args.seed + (101 if label == "early" else 211),
            min_gap_db=args.min_gap_db, bias_ridge=args.bias_ridge,
            margin=args.margin, maxiter=args.maxiter, starts=args.starts,
        )
        aligned, displacement = align_embedding(X_full, X)
        halves.append({
            "half": label,
            "n_directed_pairs": int(len(records)),
            "fit": fit,
            "median_aligned_displacement": float(np.median(displacement)),
            "p90_aligned_displacement": float(np.percentile(displacement, 90)),
            "pair_distance_spearman_vs_full": float(spearmanr(
                pdist(X_full), pdist(aligned)).statistic),
        })
        aligned_halves.append(aligned)
    disagreement = np.linalg.norm(aligned_halves[0] - aligned_halves[1], axis=1)
    return halves, disagreement


def _kmeans(X, k, seed, iterations=200):
    rng = np.random.default_rng(seed)
    centers = [X[rng.integers(len(X))]]
    for _ in range(1, k):
        d2 = np.min(squareform(pdist(np.vstack([X, centers])))[:len(X), len(X):] ** 2,
                    axis=1)
        if d2.sum() <= 1e-12:
            centers.append(X[rng.integers(len(X))])
        else:
            centers.append(X[rng.choice(len(X), p=d2 / d2.sum())])
    centers = np.asarray(centers)
    labels = np.zeros(len(X), dtype=int)
    for _ in range(iterations):
        new_labels = np.argmin(np.linalg.norm(X[:, None, :] - centers[None, :, :],
                                             axis=2), axis=1)
        new_centers = np.asarray([
            X[new_labels == j].mean(axis=0) if np.any(new_labels == j) else centers[j]
            for j in range(k)
        ])
        if np.array_equal(new_labels, labels) and np.allclose(new_centers, centers):
            labels = new_labels
            break
        labels, centers = new_labels, new_centers
    return labels


def _silhouette(X, labels):
    D = squareform(pdist(X))
    values = []
    for i in range(len(X)):
        own = labels == labels[i]
        if np.count_nonzero(own) <= 1:
            values.append(0.0)
            continue
        a = float(D[i, own].sum() / (np.count_nonzero(own) - 1))
        b = min(float(D[i, labels == other].mean()) for other in np.unique(labels)
                if other != labels[i])
        values.append((b - a) / max(a, b, 1e-12))
    return float(np.mean(values))


def infer_clusters(X, seed):
    candidates = []
    for k in range(2, min(8, len(X) - 1) + 1):
        best = None
        for restart in range(8):
            labels = _kmeans(X, k, seed + 97 * k + restart)
            score = _silhouette(X, labels)
            if best is None or score > best[0]:
                best = (score, labels)
        candidates.append({"k": k, "silhouette": float(best[0]),
                           "labels": best[1]})
    chosen = max(candidates, key=lambda item: item["silhouette"])
    return chosen, candidates


def build_parser():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="directed pairwise RSSI JSONL")
    ap.add_argument("--out", required=True, help="output point-cloud report JSON")
    ap.add_argument("--dimensions", default="1,2,3,4,5",
                    help="cross-validation dimension sweep (default: 1,2,3,4,5)")
    ap.add_argument("--holdout", type=float, default=0.20,
                    help="directed links held out within each reporter")
    ap.add_argument("--seed", type=int, default=7)
    ap.add_argument("--min-gap-db", type=float, default=1.0,
                    help="minimum RSSI gap used as an ordinal comparison")
    ap.add_argument("--bias-ridge", type=float, default=0.02,
                    help="per-transmitter ordinal-bias penalty")
    ap.add_argument("--margin", type=float, default=0.05,
                    help="dimensionless ordinal separation margin")
    ap.add_argument("--maxiter", type=int, default=700)
    ap.add_argument("--starts", type=int, default=2,
                    help="optimizer starts per embedding")
    return ap


def main():
    args = build_parser().parse_args()
    dims = tuple(int(x) for x in args.dimensions.split(",") if x.strip())
    if not dims or any(x < 1 or x > 8 for x in dims):
        raise SystemExit("--dimensions must contain integers from 1 through 8")
    if not 0.0 <= args.holdout < 0.8:
        raise SystemExit("--holdout must be in [0, 0.8)")

    metadata, raw_rows = read_observations(args.input)
    records = aggregate_observations(raw_rows)
    ids, tx, rx, rssi = index_records(records)
    test_mask = make_holdout(tx, rx, args.holdout, args.seed)
    train_mask = ~test_mask

    sweep = dimension_sweep(len(ids), dims, tx, rx, rssi, train_mask, test_mask, args)
    all_mask = np.ones(len(records), dtype=bool)
    X, bias, fit = fit_embedding(
        len(ids), 3, tx, rx, rssi, all_mask,
        seed=args.seed + 3001, min_gap_db=args.min_gap_db,
        bias_ridge=args.bias_ridge, margin=args.margin,
        maxiter=args.maxiter, starts=max(2, args.starts),
    )
    full_eval_raw = evaluate_embedding(X, tx, rx, rssi, all_mask, all_mask)
    full_eval = {
        key.replace("heldout", "in_sample"): value
        for key, value in full_eval_raw.items()
    }
    stability, disagreement = temporal_stability(raw_rows, ids, X, args)
    chosen, cluster_candidates = infer_clusters(X, args.seed)
    labels = chosen["labels"]

    neighbors = [set() for _ in ids]
    reporter = np.zeros(len(ids), dtype=bool)
    for i, j in zip(tx, rx):
        neighbors[i].add(int(j))
        neighbors[j].add(int(i))
        reporter[j] = True
    if disagreement is None:
        disagreement = np.full(len(ids), np.nan)

    report = {
        "schema": "resonance-rssi-ordinal-cloud-v1",
        "input": os.path.abspath(args.input),
        "input_sha256": sha256_file(args.input),
        "method": {
            "uses": ["tx", "rx", "rssi_dbm"],
            "does_not_use": ["capture notes", "roster", "fixture class", "ToF",
                             "CAD", "rig dimensions", "known positions",
                             "path-loss calibration"],
            "coordinate_gauge": "dimensionless; translation/rotation/reflection/scale arbitrary",
            "aggregation": "median of repeated directed EWMA snapshots",
            "embedding": "reporter-local ordinal triplets with learned TX bias",
        },
        "capture": {
            "n_raw_rows": len(raw_rows),
            "n_directed_pairs": len(records),
            "n_devices": len(ids),
            "n_reporters": int(len(np.unique(rx))),
            "metadata_rows_ignored_by_solver": len(metadata),
            "snapshot_count_quantiles": {
                str(q): float(np.percentile([r["n_snapshots"] for r in records], q))
                for q in (0, 25, 50, 75, 100)
            },
            "within_pair_mad_db_median": float(np.median([r["mad_db"] for r in records])),
            "within_pair_mad_db_p90": float(np.percentile([r["mad_db"] for r in records], 90)),
        },
        "cross_validation": sweep,
        "full_3d_fit": {**fit, **full_eval},
        "temporal_stability": stability,
        "unsupervised_clusters": {
            "chosen_k": int(chosen["k"]),
            "chosen_silhouette": float(chosen["silhouette"]),
            "candidates": [{"k": int(x["k"]), "silhouette": float(x["silhouette"])}
                           for x in cluster_candidates],
            "sizes": {str(k): int(np.count_nonzero(labels == k))
                      for k in np.unique(labels)},
        },
        "points": [
            {
                "id": dev_id,
                "xyz": [round(float(v), 7) for v in X[k]],
                "reporter": bool(reporter[k]),
                "observed_degree": int(len(neighbors[k])),
                "tx_bias": round(float(bias[k]), 7),
                "temporal_displacement": (None if not np.isfinite(disagreement[k])
                                           else round(float(disagreement[k]), 7)),
                "cluster": int(labels[k]),
            }
            for k, dev_id in enumerate(ids)
        ],
    }

    out = os.path.abspath(args.out)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "x", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2)
        fh.write("\n")

    print(f"RSSI-only cloud: {len(ids)} devices, {len(records)} directed pairs")
    for row in sweep:
        print(f"  {row['dimensions']}D: held-out RMSE {row['heldout_rmse_db']:.2f} dB, "
              f"gain {row['distance_rmse_gain_db']:+.2f} dB, "
              f"rank rho {row['heldout_reporter_rank_spearman_median']:.3f}")
    if stability:
        print("  early/late pair-distance rho:",
              ", ".join(f"{x['half']}={x['pair_distance_spearman_vs_full']:.3f}"
                        for x in stability))
    print(f"  unsupervised clusters: k={chosen['k']} "
          f"silhouette={chosen['silhouette']:.3f}")
    print(f"  wrote {out}")


if __name__ == "__main__":
    main()
