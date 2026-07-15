import itertools

import numpy as np

from locate.assign import lap_margins, solve_lap


def _brute_force(cost):
    nr, nc = cost.shape
    best, best_perm = np.inf, None
    for perm in itertools.permutations(range(nc), nr):
        c = sum(cost[r, p] for r, p in enumerate(perm))
        if c < best:
            best, best_perm = c, perm
    return best, best_perm


def test_lap_matches_brute_force_square_and_rect():
    rng = np.random.default_rng(0)
    for trial in range(100):
        nr = int(rng.integers(2, 7))
        nc = int(rng.integers(nr, 8))
        cost = rng.uniform(0, 10, size=(nr, nc))
        rows, cols, total = solve_lap(cost.copy())
        bf_total, _ = _brute_force(cost)
        assert abs(total - bf_total) < 1e-9, trial


def test_margins_match_brute_force():
    rng = np.random.default_rng(4)
    for trial in range(30):
        n = int(rng.integers(3, 6))
        cost = rng.uniform(0, 10, size=(n, n))
        rows, cols, total = solve_lap(cost.copy())
        margins = lap_margins(cost.copy(), rows, cols, total)
        for k, (r, c) in enumerate(zip(rows, cols)):
            forbidden = cost.copy()
            forbidden[r, c] = 1e12
            bf_total, _ = _brute_force(forbidden)
            assert abs(margins[k] - (bf_total - total)) < 1e-9, (trial, k)
