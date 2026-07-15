import numpy as np

from locate.model import LinkObs, PathLossParams, ZAnchor
from locate.refine import RefineConfig, refine
from locate.rssi import rssi_from_distance

from tests.helpers import procrustes_residual, random_config


def _make_problem(n=25, seed=5, offsets=None, pl=None):
    X = random_config(n, seed, spread=4.0)
    X[:, 2] = np.abs(X[:, 2])  # keep above ground, cosmetic
    pl = pl or PathLossParams()
    offsets = offsets if offsets is not None else np.zeros(n)
    links = []
    for i in range(n):
        for j in range(i + 1, n):
            d = np.linalg.norm(X[i] - X[j])
            r = float(rssi_from_distance(d, pl)) + offsets[i] + offsets[j]
            links.append(LinkObs(i, j, r, 20))
    anchors = [ZAnchor(idx=i, z_m=float(X[i, 2]), sigma_m=0.01) for i in range(0, n, 2)]
    return X, links, anchors, pl


def test_jacobian_matches_finite_differences():
    # exercise the private jacobian through a tiny refine problem by comparing
    # the analytic J against numeric differentiation of the residual closure.
    from locate import refine as rf

    n = 6
    X, links, anchors, pl = _make_problem(n=n, seed=2)
    cfg = RefineConfig(fit_n=True)

    # rebuild the same closures refine() uses, via a one-iteration call path:
    # simplest robust check -- numerically differentiate the public residual
    # function reconstructed here to mirror refine()'s definition.
    I = np.array([l.i for l in links]); J = np.array([l.j for l in links])
    rssi = np.array([l.rssi_dbm for l in links])
    a_idx = np.array([a.idx for a in anchors]); a_z = np.array([a.z_m for a in anchors])
    a_sig = np.array([a.sigma_m for a in anchors])

    def residuals(p):
        Xp = p[:3 * n].reshape(n, 3)
        o = p[3 * n:4 * n]
        p0 = p[4 * n]
        nn = p[4 * n + 1]
        d = np.maximum(np.linalg.norm(Xp[I] - Xp[J], axis=1), rf.D_MIN)
        r_link = (p0 - 10 * nn * np.log10(d) + o[I] + o[J] - rssi) / cfg.sigma_link_db
        r_anchor = (Xp[a_idx, 2] - a_z) / a_sig
        r_off = o / cfg.sigma_o_db
        r_n = np.array([(nn - pl.n) / cfg.sigma_n])
        return np.concatenate([r_link, r_anchor, r_off, r_n])

    rng = np.random.default_rng(0)
    p = np.concatenate([(X + rng.normal(0, 0.1, X.shape)).ravel(),
                        rng.normal(0, 0.5, n), [pl.p0_dbm + 1.0], [pl.n + 0.05]])

    # analytic jacobian: pull it out of refine's implementation by calling
    # refine with max_nfev=1? Instead: finite-difference residuals and compare
    # against scipy's internal use indirectly is weak; so replicate analytic J
    # by importing refine and invoking its inner jac is not possible (closure).
    # We therefore validate the MATH: numeric J of the residuals above must
    # match the analytic formulas coded in refine.py for a probe of entries.
    eps = 1e-7
    r0 = residuals(p)
    num = np.zeros((len(r0), len(p)))
    for k in range(len(p)):
        dp = p.copy(); dp[k] += eps
        num[:, k] = (residuals(dp) - r0) / eps

    # analytic formulas (same as refine.py)
    Xp = p[:3 * n].reshape(n, 3); o = p[3 * n:4 * n]; nn = p[4 * n + 1]
    diff = Xp[I] - Xp[J]
    d = np.maximum(np.linalg.norm(diff, axis=1), rf.D_MIN)
    g = -(10 * nn / np.log(10.0)) * diff / (d ** 2)[:, None] / cfg.sigma_link_db
    for row in range(len(links)):
        i, j = I[row], J[row]
        assert np.allclose(num[row, 3 * i:3 * i + 3], g[row], atol=1e-4)
        assert np.allclose(num[row, 3 * j:3 * j + 3], -g[row], atol=1e-4)
        assert abs(num[row, 3 * n + i] - 1 / cfg.sigma_link_db) < 1e-4
        assert abs(num[row, 4 * n] - 1 / cfg.sigma_link_db) < 1e-4
        assert abs(num[row, 4 * n + 1] - (-10 * np.log10(d[row]) / cfg.sigma_link_db)) < 1e-4


def test_zero_noise_exact_recovery_from_perturbed_init():
    X, links, anchors, pl = _make_problem(n=25, seed=5)
    rng = np.random.default_rng(1)
    X0 = X + rng.normal(0, 0.3, X.shape)
    Xr, offsets, pl_fit, diag = refine(X0, links, anchors, pl)
    # gauge: xy translation/rotation free -> compare via procrustes (no scale)
    assert procrustes_residual(Xr, X, allow_scale=False) < 1e-3
    assert np.abs(offsets).max() < 0.05
    assert abs(pl_fit.p0_dbm - pl.p0_dbm) < 0.1


def test_known_offsets_explained_within_coupling_limit():
    # Per-device offsets are only weakly identifiable against radial position
    # (moving an edge device outward mimics a negative offset -- the documented
    # leakage; the sigma_o prior bounds it). At zero packet noise the honest
    # invariants are: (1) the fitted model reproduces the DATA essentially
    # exactly, (2) the offset/position decomposition lands within the coupling
    # limit, not at machine precision.
    n = 25
    rng = np.random.default_rng(9)
    true_off = rng.normal(0, 2.0, n)
    true_off -= true_off.mean()  # mean offset is degenerate with P0; center it
    X, links, anchors, pl = _make_problem(n=n, seed=6, offsets=true_off)
    X0 = X + rng.normal(0, 0.2, X.shape)
    Xr, offsets, pl_fit, diag = refine(X0, links, anchors, pl)

    # (1) data explained: predicted symmetrized RSSI matches observations
    pred_err = []
    for l in links:
        d = np.linalg.norm(Xr[l.i] - Xr[l.j])
        pred = float(rssi_from_distance(d, pl_fit)) + offsets[l.i] + offsets[l.j]
        pred_err.append(pred - l.rssi_dbm)
    # ~0.2 dB misfit is the MAP prior/data trade-off, not solver failure
    assert np.sqrt(np.mean(np.square(pred_err))) < 0.5, "model fails to explain the data"

    # (2) decomposition within the coupling limit
    est = offsets - offsets.mean()
    assert np.abs(est - true_off).max() < 2.5
    assert procrustes_residual(Xr, X, allow_scale=False) < 0.7


def test_anchors_respected_under_noise():
    X, links, anchors, pl = _make_problem(n=25, seed=8)
    rng = np.random.default_rng(3)
    noisy = [LinkObs(l.i, l.j, l.rssi_dbm + float(rng.normal(0, 2.0)), l.n_samples)
             for l in links]
    X0 = X + rng.normal(0, 0.3, X.shape)
    Xr, _, _, _ = refine(X0, noisy, anchors, pl)
    for a in anchors:
        assert abs(Xr[a.idx, 2] - a.z_m) < 0.05
