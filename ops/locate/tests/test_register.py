import numpy as np

from locate.model import CadFixture, CadModel
from locate.register import _rz, cad_self_symmetry, register

from tests.helpers import random_config


def _cad_from_positions(pos, roles):
    fixtures = [CadFixture(fixture_id=f"F{k:03d}", role=r, pos_m=p)
                for k, (p, r) in enumerate(zip(pos, roles))]
    return CadModel(fixtures=fixtures)


def _asymmetric_layout(n=60, seed=3):
    """Blob with no rotational symmetry: random cluster + a distinctive arm."""
    rng = np.random.default_rng(seed)
    pos = rng.uniform(-4, 4, size=(n, 3))
    pos[:, 2] = rng.uniform(1, 3, n)
    pos[: n // 4, 0] += 6.0  # arm
    return pos


def test_known_gauge_recovered_with_noise():
    pos = _asymmetric_layout()
    roles = ["downlight"] * 40 + ["perimeter"] * 20
    cad = _cad_from_positions(pos, roles)
    rng = np.random.default_rng(1)
    for mirror in (1, -1):
        theta = np.deg2rad(117.0)
        R = _rz(theta, mirror)
        # devices = CAD slots transformed by the INVERSE gauge + small noise
        X = (pos - pos.mean(0)) @ R + pos.mean(0)  # note: @R == @ (R^-1)^T for rotations
        X = X + rng.normal(0, 0.05, X.shape)
        res = register(X, roles, cad, theta_step_deg=2.0)
        truth = [f"F{k:03d}" for k in range(len(pos))]
        acc = np.mean([a == t for a, t in zip(res.assignment, truth)])
        assert acc == 1.0, (mirror, acc)
        assert res.mirror == mirror


def test_beacon_pinning_forces_assignment():
    pos = _asymmetric_layout(n=30, seed=8)
    roles = ["downlight"] * 30
    cad = _cad_from_positions(pos, roles)
    X = pos + np.random.default_rng(2).normal(0, 0.05, pos.shape)
    res = register(X, roles, cad, known={0: "F005"})
    assert res.assignment[0] == "F005"
    assert np.isinf(res.margins[0])


def test_two_beacons_never_flip_mirror():
    # regression: two beacon points in the plane are fit exactly by EITHER
    # mirror + a suitable rotation, so a closed-form gauge from 2 beacons is a
    # mirror coin flip (measured collapse to ~0.44 accuracy). With 2 beacons
    # the registration must use the grid (beacons pay real displacement cost)
    # and still recover the true gauge.
    pos = _asymmetric_layout()
    roles = ["downlight"] * len(pos)
    cad = _cad_from_positions(pos, roles)
    rng = np.random.default_rng(5)
    for mirror in (1, -1):
        theta = np.deg2rad(203.0)
        R = _rz(theta, mirror)
        X = (pos - pos.mean(0)) @ R + pos.mean(0)
        X = X + rng.normal(0, 0.05, X.shape)
        res = register(X, roles, cad, known={0: "F000", 7: "F007"})
        truth = [f"F{k:03d}" for k in range(len(pos))]
        acc = np.mean([a == t for a, t in zip(res.assignment, truth)])
        assert acc == 1.0, (mirror, acc)
        assert res.mirror == mirror
        assert not res.diagnostics["beacon_gauge"]

    # and 3 beacons take the closed-form path
    X3 = pos + rng.normal(0, 0.05, pos.shape)
    res3 = register(X3, roles, cad, known={0: "F000", 7: "F007", 20: "F020"})
    assert res3.diagnostics["beacon_gauge"]
    truth = [f"F{k:03d}" for k in range(len(pos))]
    assert np.mean([a == t for a, t in zip(res3.assignment, truth)]) == 1.0


def test_symmetry_diagnostic_fires_on_symmetric_ring():
    # 6-fold symmetric ring: competing minima must be ~as good as identity
    n = 36
    ang = np.arange(n) * 2 * np.pi / n
    pos = np.column_stack([5 * np.cos(ang), 5 * np.sin(ang), np.full(n, 1.5)])
    cad = _cad_from_positions(pos, ["perimeter"] * n)
    sym = cad_self_symmetry(cad, theta_step_deg=2.0)
    assert sym["competing_rms_m"] < 1e-6  # perfectly symmetric -> zero-cost alias

    # asymmetric blob: competing minimum must be expensive
    pos2 = _asymmetric_layout()
    cad2 = _cad_from_positions(pos2, ["downlight"] * len(pos2))
    sym2 = cad_self_symmetry(cad2, theta_step_deg=10.0)
    assert sym2["competing_rms_m"] > 0.5
