"""Log-distance path-loss model and per-link RSSI aggregation/symmetrization.

Model (all dB):
    RSSI(i->j) = P0 - 10 n log10(d_ij / d0) + b_ij + t_i + r_j + eps

Aggregation takes the median per direction (robust to per-packet fading and
heavy tails), then averages the two directions. Under the model above the
symmetric average turns the four device terms into (t_i+r_j+t_j+r_i)/2 =
o_i + o_j with o_k = (t_k + r_k)/2, i.e. TX/RX asymmetry cancels exactly and
the solver only needs one scalar offset per device (unit-tested in
tests/test_rssi.py). |median_ij - median_ji| is retained as a diagnostic:
large values indicate directional effects the scalar model cannot absorb.
"""

from typing import Dict, List, Tuple

import numpy as np

from .model import LinkObs, PathLossParams


def rssi_from_distance(d_m, pl: PathLossParams):
    """Forward model. Accepts scalars or arrays; clamps d to avoid log(0)."""
    d = np.maximum(np.asarray(d_m, dtype=float), 1e-6)
    return pl.p0_dbm - 10.0 * pl.n * np.log10(d / pl.d0_m)


def distance_from_rssi(rssi_dbm, pl: PathLossParams):
    """Inverse model."""
    r = np.asarray(rssi_dbm, dtype=float)
    return pl.d0_m * 10.0 ** ((pl.p0_dbm - r) / (10.0 * pl.n))


def aggregate_directed(
    directed: Dict[Tuple[int, int], List[float]],
    keep_samples: bool = False,
) -> List[LinkObs]:
    """Collapse directed per-packet RSSI samples into symmetrized LinkObs.

    directed[(tx, rx)] = list of dB samples received at rx from tx.
    Links observed in only one direction are kept (asym_db=None).
    """
    pair_dirs: Dict[Tuple[int, int], dict] = {}
    for (tx, rx), samples in directed.items():
        if tx == rx or not samples:
            continue
        key = (min(tx, rx), max(tx, rx))
        d = pair_dirs.setdefault(key, {})
        d.setdefault("medians", []).append(float(np.median(samples)))
        d["n"] = d.get("n", 0) + len(samples)
        if keep_samples:
            d.setdefault("samples", []).extend(float(s) for s in samples)

    links = []
    for (i, j), d in sorted(pair_dirs.items()):
        meds = d["medians"]
        asym = abs(meds[0] - meds[1]) if len(meds) == 2 else None
        links.append(
            LinkObs(
                i=i,
                j=j,
                rssi_dbm=float(np.mean(meds)),
                n_samples=d["n"],
                asym_db=asym,
                samples=d.get("samples") if keep_samples else None,
            )
        )
    return links
