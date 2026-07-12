"""Robust nonlinear least squares in dB space: positions + device offsets + P0.

Parameters theta = {x_i in R^3} + {o_i} + P0 (+ n with fit_n), residuals:

    link:    r_ij = [P0 - 10 n log10||x_i - x_j|| + o_i + o_j - rssi_ij] / sigma_link
    anchor:  r_i  = (x_i.z - z_meas_i) / sigma_z_i
    offset:  r_i  = o_i / sigma_o          (kills the o <-> P0 gauge and bounds
                                            offset <-> radial-position leakage)
    n prior: (n - n_prior) / sigma_n       (only with fit_n)

Huber loss (scipy least_squares loss='huber'). The xy-gauge null space (global
translation/rotation about z, irrelevant to the cost) is handled by the trust
region + lsmr solver; registration fixes the gauge afterwards. Analytic sparse
Jacobian; ~610 params at N=152 solves in well under a second.

The (unrobustified) J^T J at the solution provides a per-device covariance proxy
(Cramer-Rao flavored) at negligible cost.
"""

from dataclasses import dataclass
from typing import List

import numpy as np
from scipy import sparse
from scipy.optimize import least_squares

from .model import LinkObs, PathLossParams, ZAnchor

LN10_OVER_10 = np.log(10.0) / 10.0
D_MIN = 1e-3


@dataclass
class RefineConfig:
    sigma_link_db: float = 3.0
    sigma_o_db: float = 3.0
    huber_delta: float = 1.5
    fit_n: bool = False
    sigma_n: float = 0.3
    max_nfev: int = 300


def refine(X0: np.ndarray, links: List[LinkObs], anchors: List[ZAnchor],
           pl0: PathLossParams, cfg: RefineConfig = None):
    """Returns (X, offsets_db, pl_fit, diagnostics)."""
    cfg = cfg or RefineConfig()
    n_dev = len(X0)
    I = np.array([l.i for l in links])
    J = np.array([l.j for l in links])
    rssi = np.array([l.rssi_dbm for l in links])
    a_idx = np.array([a.idx for a in anchors], dtype=int)
    a_z = np.array([a.z_m for a in anchors])
    a_sig = np.array([a.sigma_m for a in anchors])

    n_links = len(links)
    n_extra = 2 if cfg.fit_n else 1          # P0 (+ n)
    n_par = 3 * n_dev + n_dev + n_extra
    off0 = 3 * n_dev                          # offsets start
    p0_col = 4 * n_dev                        # P0 column
    n_col = 4 * n_dev + 1                     # n column (if fit)

    def unpack(p):
        X = p[: 3 * n_dev].reshape(n_dev, 3)
        o = p[off0: off0 + n_dev]
        p0 = p[p0_col]
        n_exp = p[n_col] if cfg.fit_n else pl0.n
        return X, o, p0, n_exp

    def residuals(p):
        X, o, p0, n_exp = unpack(p)
        diff = X[I] - X[J]
        d = np.maximum(np.linalg.norm(diff, axis=1), D_MIN)
        r_link = (p0 - 10 * n_exp * np.log10(d) + o[I] + o[J] - rssi) / cfg.sigma_link_db
        r_anchor = (X[a_idx, 2] - a_z) / a_sig if len(a_idx) else np.zeros(0)
        r_off = o / cfg.sigma_o_db
        parts = [r_link, r_anchor, r_off]
        if cfg.fit_n:
            parts.append(np.array([(n_exp - pl0.n) / cfg.sigma_n]))
        return np.concatenate(parts)

    # static sparsity: link rows (3+3+1+1 pos/offset cols + P0 (+n)), anchor rows, prior rows
    def jacobian(p):
        X, o, p0, n_exp = unpack(p)
        diff = X[I] - X[J]
        d = np.maximum(np.linalg.norm(diff, axis=1), D_MIN)
        g = -(10 * n_exp / np.log(10.0)) * diff / (d ** 2)[:, None] / cfg.sigma_link_db

        rows, cols, data = [], [], []
        lr = np.arange(n_links)
        for ax in range(3):
            rows.append(lr); cols.append(3 * I + ax); data.append(g[:, ax])
            rows.append(lr); cols.append(3 * J + ax); data.append(-g[:, ax])
        one = np.full(n_links, 1.0 / cfg.sigma_link_db)
        rows.append(lr); cols.append(off0 + I); data.append(one)
        rows.append(lr); cols.append(off0 + J); data.append(one)
        rows.append(lr); cols.append(np.full(n_links, p0_col)); data.append(one)
        if cfg.fit_n:
            rows.append(lr); cols.append(np.full(n_links, n_col))
            data.append(-10 * np.log10(d) / cfg.sigma_link_db)

        r0 = n_links
        if len(a_idx):
            ar = r0 + np.arange(len(a_idx))
            rows.append(ar); cols.append(3 * a_idx + 2); data.append(1.0 / a_sig)
            r0 += len(a_idx)
        orow = r0 + np.arange(n_dev)
        rows.append(orow); cols.append(off0 + np.arange(n_dev))
        data.append(np.full(n_dev, 1.0 / cfg.sigma_o_db))
        r0 += n_dev
        if cfg.fit_n:
            rows.append(np.array([r0])); cols.append(np.array([n_col]))
            data.append(np.array([1.0 / cfg.sigma_n]))
            r0 += 1

        rows = np.concatenate([np.asarray(r, dtype=int) for r in rows])
        cols = np.concatenate([np.asarray(c, dtype=int) for c in cols])
        data = np.concatenate(data)
        return sparse.csr_matrix((data, (rows, cols)), shape=(r0, n_par))

    p_init = np.concatenate([
        X0.ravel(),
        np.zeros(n_dev),
        [pl0.p0_dbm],
        [pl0.n] if cfg.fit_n else [],
    ])
    res = least_squares(
        residuals, p_init, jac=jacobian, method="trf", tr_solver="lsmr",
        loss="huber", f_scale=cfg.huber_delta, max_nfev=cfg.max_nfev, x_scale="jac",
    )

    X, o, p0, n_exp = unpack(res.x)
    pl_fit = PathLossParams(p0_dbm=float(p0), n=float(n_exp), d0_m=pl0.d0_m)

    # covariance proxy from the unrobustified normal matrix
    Jm = jacobian(res.x)
    H = (Jm.T @ Jm).toarray()
    cov = np.linalg.pinv(H)
    var_pos = np.array([np.trace(cov[3 * k: 3 * k + 3, 3 * k: 3 * k + 3]) / 3 for k in range(n_dev)])
    sigma_pos = np.sqrt(np.clip(var_pos, 0, None))

    degree = np.zeros(n_dev, int)
    for l in links:
        degree[l.i] += 1
        degree[l.j] += 1

    diag = {
        "cost": float(res.cost),
        "nfev": int(res.nfev),
        "success": bool(res.success),
        "sigma_pos_m": sigma_pos,
        "degree": degree,
    }
    return X, o, pl_fit, diag
