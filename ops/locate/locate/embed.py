"""Initial 3D embedding: distance completion, classical MDS, anchor z-alignment.

Pipeline: RSSI -> distance estimates at the prior path-loss model -> Floyd-Warshall
shortest-path completion of missing links -> classical MDS -> anchor-driven
orientation + metric scale fix.

Anchor alignment math: after MDS, X relates to truth by an unknown rotation,
reflection, translation, and a scale error k inherited from the (P0, n) prior:
X ~= k * X_true R^T. Least-squares u = argmin ||Xc u - zc||^2 over anchored rows
then satisfies u = R e_z / k, so ||u|| = 1/k recovers the metric scale from the
anchors and u/||u|| is the embedding direction that is truly "up" (the LS sign
resolves the z-flip automatically; a residual xy mirror may remain and is resolved
later in registration). We apply scale ||u||, rotate u/||u|| -> e_z, and translate
z to the anchor mean. The anchor fit RMS is reported: a large value means the
embedding is too warped for the anchors to define a plane -- an early breakage
indicator.
"""

from typing import List

import numpy as np

from .model import LinkObs, PathLossParams, ZAnchor
from .rssi import distance_from_rssi


def distance_matrix(n: int, links: List[LinkObs], pl: PathLossParams, d_cap_m: float = None):
    """Build (D, observed) from symmetrized links at the prior path-loss model."""
    D = np.full((n, n), np.inf)
    np.fill_diagonal(D, 0.0)
    observed = np.zeros((n, n), bool)
    for l in links:
        if l.censored:
            continue          # survivor-biased: only a lower bound, let FW complete it
        d = float(distance_from_rssi(l.rssi_dbm, pl))
        if d_cap_m is not None:
            d = min(d, d_cap_m)
        D[l.i, l.j] = D[l.j, l.i] = d
        observed[l.i, l.j] = observed[l.j, l.i] = True
    return D, observed


def complete_distances(D: np.ndarray) -> np.ndarray:
    """Floyd-Warshall shortest-path completion of an inf-padded distance matrix."""
    D = D.copy()
    n = len(D)
    for k in range(n):
        np.minimum(D, D[:, k, None] + D[None, k, :], out=D)
    if np.isinf(D).any():
        # disconnected components: fall back to the largest finite distance
        finite_max = np.max(D[np.isfinite(D)])
        D[np.isinf(D)] = finite_max
    return D


def classical_mds(D: np.ndarray, ndim: int = 3):
    """Torgerson MDS: double centering + top-ndim eigenvectors."""
    n = len(D)
    D2 = D ** 2
    J = np.eye(n) - np.ones((n, n)) / n
    B = -0.5 * J @ D2 @ J
    w, V = np.linalg.eigh(B)
    order = np.argsort(w)[::-1][:ndim]
    w_top = np.clip(w[order], 0.0, None)
    X = V[:, order] * np.sqrt(w_top)
    return X, w[np.argsort(w)[::-1]]


def _rotation_to_ez(u_hat: np.ndarray) -> np.ndarray:
    """Proper rotation Q with Q @ u_hat = e_z (Rodrigues)."""
    ez = np.array([0.0, 0.0, 1.0])
    v = np.cross(u_hat, ez)
    c = float(u_hat @ ez)
    s = float(np.linalg.norm(v))
    if s < 1e-12:
        return np.eye(3) if c > 0 else np.diag([1.0, -1.0, -1.0])
    vx = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + vx + vx @ vx * ((1 - c) / (s * s))


def align_to_anchors(X: np.ndarray, anchors: List[ZAnchor], snap_sigma_m: float = 0.05):
    """Scale + orient the MDS embedding so anchored devices match measured heights.

    Returns (X_aligned, info). Anchors with sigma_m < snap_sigma_m get their z
    overwritten with the measurement (init nicety; refinement re-balances).
    """
    if len(anchors) < 3:
        raise ValueError("need >=3 z-anchors to orient the embedding")
    idx = np.array([a.idx for a in anchors])
    za = np.array([a.z_m for a in anchors])

    mean_a = X[idx].mean(axis=0)
    Xc = X - mean_a
    zc = za - za.mean()
    u, *_ = np.linalg.lstsq(Xc[idx], zc, rcond=None)
    nu = float(np.linalg.norm(u))
    if nu < 1e-12:
        raise ValueError("anchor z-fit degenerate (no vertical spread?)")

    # fit-quality gate: if the anchors' z barely correlates with any embedding
    # direction (near-planar scene + noise -- the z axis drowned), ||u|| is
    # meaningless and would rescale the whole embedding by garbage. The MDS
    # embedding is already metric by construction, so fall back to scale 1.
    fit_resid = Xc[idx] @ u - zc
    ss_tot = float(np.sum(zc ** 2))
    r2 = 1.0 - float(np.sum(fit_resid ** 2)) / ss_tot if ss_tot > 0 else 0.0
    degraded = r2 < 0.3
    if degraded:
        nu = 1.0

    Q = _rotation_to_ez(u / np.linalg.norm(u))
    Xa = (nu * Xc) @ Q.T
    Xa[:, 2] += za.mean()

    resid = Xa[idx, 2] - za
    for a in anchors:
        if a.sigma_m < snap_sigma_m:
            Xa[a.idx, 2] = a.z_m

    info = {
        "scale_applied": nu,
        "anchor_fit_r2": r2,
        "degraded": degraded,
        "anchor_fit_rms_m": float(np.sqrt(np.mean(resid ** 2))),
        "n_anchors": len(anchors),
        "anchor_z_spread_m": float(np.std(za)),
    }
    return Xa, info


def anchored_init(
    n_dev: int,
    links: List[LinkObs],
    anchors: List[ZAnchor],
    pl: PathLossParams,
    d_cap_m: float = 60.0,
    min_dxy_m: float = 0.05,
) -> np.ndarray:
    """Anchor-aware initialization -- use the z-anchors where they are strong.

    Rationale (learned the hard way, see tests/test_e2e_smoke.py history): the
    tree scene is nearly planar (z spread ~0.7 m vs xy spread ~8 m), so in plain
    3D MDS the height axis drowns in RSSI noise, and align_to_anchors' scale
    estimate collapses with it. Instead: for the anchored subset (>=2/3 of the
    fleet) z is MEASURED, so subtract it out -- d_xy^2 = d_hat^2 - dz^2 -- and
    solve a clean 2D MDS for xy, where the signal variance actually is. The
    unanchored devices (uplights/chandelier) are seeded by inverse-square-
    weighted centroids of their nearest anchored neighbors; the NLS refinement
    separates them from there.
    """
    idx_a = np.array(sorted({a.idx for a in anchors}))
    z_meas = np.zeros(n_dev)
    z_sig = np.full(n_dev, np.inf)
    for a in anchors:
        if a.sigma_m < z_sig[a.idx]:
            z_meas[a.idx] = a.z_m
            z_sig[a.idx] = a.sigma_m

    D, observed = distance_matrix(n_dev, links, pl, d_cap_m=d_cap_m)

    # 2D MDS over the anchored subset with z removed from the metric
    DA = D[np.ix_(idx_a, idx_a)]
    dz = z_meas[idx_a][:, None] - z_meas[idx_a][None, :]
    Dxy = np.sqrt(np.maximum(DA ** 2 - dz ** 2, min_dxy_m ** 2))
    np.fill_diagonal(Dxy, 0.0)
    Dxy[np.isinf(DA)] = np.inf
    Dxy = complete_distances(Dxy)
    XY, _ = classical_mds(Dxy, ndim=2)

    X = np.zeros((n_dev, 3))
    X[idx_a, :2] = XY
    X[idx_a, 2] = z_meas[idx_a]

    # unanchored: inverse-square-weighted centroid of nearest anchored devices
    anchored = np.zeros(n_dev, bool)
    anchored[idx_a] = True
    for u in range(n_dev):
        if anchored[u]:
            continue
        d_u = D[u, idx_a]
        order = np.argsort(d_u)[:8]
        w = 1.0 / np.maximum(d_u[order], 1e-2) ** 2
        if not np.isfinite(w).any() or w.sum() <= 0:
            w = np.ones(len(order))
        w = np.where(np.isfinite(w), w, 0.0)
        X[u] = (X[idx_a[order]] * w[:, None]).sum(axis=0) / w.sum()
    return X


def smacof(X0: np.ndarray, links: List[LinkObs], pl: PathLossParams,
           iters: int = 100, tol: float = 1e-8):
    """Optional weighted stress majorization intermediate (Sammon-style weights
    w = 1/d_hat^2 approximates log-space error; missing links get zero weight,
    removing completion bias before the NLS stage). Off the mainline by default."""
    n = len(X0)
    D_hat = np.zeros((n, n))
    W = np.zeros((n, n))
    for l in links:
        d = float(distance_from_rssi(l.rssi_dbm, pl))
        D_hat[l.i, l.j] = D_hat[l.j, l.i] = d
        W[l.i, l.j] = W[l.j, l.i] = 1.0 / max(d, 1e-3) ** 2

    V = np.diag(W.sum(axis=1)) - W
    Vp = np.linalg.pinv(V)
    X = X0.copy()
    prev = np.inf
    for _ in range(iters):
        diff = X[:, None, :] - X[None, :, :]
        dist = np.linalg.norm(diff, axis=2)
        np.fill_diagonal(dist, 1.0)
        ratio = np.where(dist > 1e-12, D_hat / dist, 0.0) * W
        B = np.diag(ratio.sum(axis=1)) - ratio
        X = Vp @ (B @ X)
        stress = float(np.sum(W * (dist - D_hat) ** 2) / 2)
        if abs(prev - stress) < tol * max(prev, 1.0):
            break
        prev = stress
    return X
