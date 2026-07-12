import numpy as np

from locate.model import PathLossParams
from locate.rssi import aggregate_directed, distance_from_rssi, rssi_from_distance


def test_round_trip():
    pl = PathLossParams(p0_dbm=-40, n=2.7)
    d = np.array([0.5, 1.0, 3.7, 12.0, 45.0])
    r = rssi_from_distance(d, pl)
    back = distance_from_rssi(r, pl)
    assert np.allclose(back, d, rtol=1e-12), (back, d)


def test_symmetrization_cancels_txrx_asymmetry():
    # RSSI(i->j) = base + t_i + r_j ; symmetric average must equal
    # base + o_i + o_j with o = (t+r)/2, EXACTLY, whatever t/r are.
    t = {0: 5.0, 1: -3.0}
    r = {0: -1.0, 1: 2.0}
    base = -60.0
    directed = {
        (0, 1): [base + t[0] + r[1]] * 7,
        (1, 0): [base + t[1] + r[0]] * 5,
    }
    links = aggregate_directed(directed)
    assert len(links) == 1
    o = {k: (t[k] + r[k]) / 2 for k in t}
    expected = base + o[0] + o[1]
    assert abs(links[0].rssi_dbm - expected) < 1e-12
    assert links[0].n_samples == 12
    assert links[0].asym_db is not None and links[0].asym_db > 0


def test_one_directional_link_kept():
    links = aggregate_directed({(2, 5): [-70.0, -71.0, -69.0]})
    assert len(links) == 1
    assert links[0].i == 2 and links[0].j == 5
    assert links[0].asym_db is None
    assert abs(links[0].rssi_dbm - (-70.0)) < 1e-12  # median


def test_median_robust_to_outlier():
    links = aggregate_directed({(0, 1): [-60.0, -60.0, -60.0, -60.0, -95.0]})
    assert abs(links[0].rssi_dbm - (-60.0)) < 1e-12
