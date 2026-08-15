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


def _directional_median(samples: List[float], expected: int = None):
    """Median of one direction's packets, corrected for floor censoring.

    Packets below the receiver floor are never received, so the survivor
    median is biased high (the pair looks closer than it is). When the SENT
    count K is known (fixed beacon rate x capture window), the received
    samples are approximately the top-m of K draws, so the true median is the
    (K/2)-th largest received sample -- recoverable whenever enough packets
    survive. Returns (median_dbm, censored): censored=True when the median
    rank fell below the floor (the value returned is then the deepest usable
    upper bound on RSSI, i.e. a lower bound on distance).
    """
    m = len(samples)
    # near-complete reception: the survivor bias is < ~0.5 dB and the rank
    # trick would OVERcorrect (reception near the floor is soft-logistic, not a
    # hard threshold, so received != exactly top-m)
    if not expected or m >= 0.95 * expected:
        return float(np.median(samples)), False
    desc = np.sort(np.asarray(samples, dtype=float))[::-1]
    med_rank = expected // 2
    if med_rank <= m - 1 and m >= 0.6 * expected:
        return float(desc[med_rank]), False
    return float(desc[-1]), True


def merge_pair_directions(pair_dirs: Dict[Tuple[int, int], dict]) -> List[LinkObs]:
    """Shared merge: pair_dirs[(i,j)] = {"medians": [(med_dbm, censored), ...],
    "n": total_packets, "samples": optional raw} -> symmetrized LinkObs list.
    Uncensored directional medians are averaged; if every direction is censored
    the link keeps the tightest RSSI upper bound and stays censored."""
    links = []
    for (i, j), d in sorted(pair_dirs.items()):
        meds = d["medians"]
        clean = [m for m, cen in meds if not cen]
        if clean:
            rssi = float(np.mean(clean))
            censored = False
            asym = float(max(clean) - min(clean)) if len(clean) >= 2 else None
        else:
            rssi = float(min(m for m, _ in meds))
            censored = True
            asym = None
        links.append(
            LinkObs(
                i=i, j=j, rssi_dbm=rssi, n_samples=d.get("n", 0), asym_db=asym,
                samples=d.get("samples"), censored=censored,
            )
        )
    return links


def aggregate_directed(
    directed: Dict[Tuple[int, int], List[float]],
    keep_samples: bool = False,
    expected: int = None,
) -> List[LinkObs]:
    """Collapse directed per-packet RSSI samples into symmetrized LinkObs.

    directed[(tx, rx)] = list of dB samples received at rx from tx.
    Links observed in only one direction are kept (asym_db=None).
    expected = packets SENT per direction over the capture window, if known
    (enables floor-censoring correction, see _directional_median).
    """
    pair_dirs: Dict[Tuple[int, int], dict] = {}
    for (tx, rx), samples in directed.items():
        if tx == rx or not samples:
            continue
        key = (min(tx, rx), max(tx, rx))
        med, cen = _directional_median(samples, expected)
        d = pair_dirs.setdefault(key, {})
        d.setdefault("medians", []).append((med, cen))
        d["n"] = d.get("n", 0) + len(samples)
        if keep_samples:
            d.setdefault("samples", []).extend(float(s) for s in samples)
    return merge_pair_directions(pair_dirs)
