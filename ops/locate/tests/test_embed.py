import numpy as np

from locate import embed
from locate.model import LinkObs, PathLossParams, ZAnchor
from locate.rssi import rssi_from_distance

from tests.helpers import procrustes_residual, random_config


def _links_from_positions(X, pl, drop=None, seed=0):
    n = len(X)
    rng = np.random.default_rng(seed)
    links = []
    for i in range(n):
        for j in range(i + 1, n):
            if drop and rng.random() < drop:
                continue
            d = np.linalg.norm(X[i] - X[j])
            links.append(LinkObs(i, j, float(rssi_from_distance(d, pl)), 10))
    return links


def test_mds_exact_recovery():
    for seed in (1, 2, 3):
        X = random_config(40, seed)
        pl = PathLossParams()
        links = _links_from_positions(X, pl)
        D, _ = embed.distance_matrix(40, links, pl)
        Dc = embed.complete_distances(D)
        X0, _ = embed.classical_mds(Dc)
        assert procrustes_residual(X0, X) < 1e-6, seed


def test_completion_with_missing_links():
    X = random_config(40, 7)
    pl = PathLossParams()
    links = _links_from_positions(X, pl, drop=0.2, seed=7)
    D, _ = embed.distance_matrix(40, links, pl)
    Dc = embed.complete_distances(D)
    assert np.isfinite(Dc).all()
    X0, _ = embed.classical_mds(Dc)
    # shortest-path completion overestimates missing distances; loose bound
    assert procrustes_residual(X0, X) < 0.5


def test_align_to_anchors_recovers_rotation_and_scale():
    X_true = random_config(50, 11)
    rng = np.random.default_rng(11)
    # random rotation + scale + translation, as MDS would hand us
    A = rng.normal(size=(3, 3))
    Q, _ = np.linalg.qr(A)
    if np.linalg.det(Q) < 0:
        Q[:, 0] *= -1
    k = 2.37
    X_obs = k * X_true @ Q.T + rng.normal(size=3)
    anchors = [ZAnchor(idx=i, z_m=float(X_true[i, 2]), sigma_m=0.5) for i in range(0, 50, 3)]
    Xa, info = embed.align_to_anchors(X_obs, anchors, snap_sigma_m=0.0)
    assert abs(info["scale_applied"] * k - 1.0) < 1e-9      # metric scale recovered
    za = np.array([X_true[a.idx, 2] for a in anchors])
    assert np.allclose(Xa[[a.idx for a in anchors], 2], za, atol=1e-9)
    # full config must now match truth in z everywhere (not just anchors)
    assert np.allclose(Xa[:, 2], X_true[:, 2], atol=1e-9)


def test_align_snaps_tight_anchors():
    X_true = random_config(30, 3)
    anchors = [ZAnchor(idx=i, z_m=float(X_true[i, 2]) + 0.3, sigma_m=0.001) for i in range(5)]
    Xa, _ = embed.align_to_anchors(X_true.copy(), anchors, snap_sigma_m=0.05)
    for a in anchors:
        assert abs(Xa[a.idx, 2] - a.z_m) < 1e-12
