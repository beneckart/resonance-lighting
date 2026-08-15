"""Rectangular linear assignment (devices <= slots) + exact per-device margins.

Thin wrapper over scipy.optimize.linear_sum_assignment. The margin of an assigned
edge is the increase in total class cost when that edge is forbidden and the LAP
re-solved -- an exact, truth-free confidence signal (a small margin means a nearly
as-good competing assignment exists).
"""

import numpy as np
from scipy.optimize import linear_sum_assignment

BIG = 1e12


def solve_lap(cost: np.ndarray):
    """cost (n_rows, n_cols), n_rows <= n_cols. Returns (rows, cols, total)."""
    rows, cols = linear_sum_assignment(cost)
    return rows, cols, float(cost[rows, cols].sum())


def lap_margins(cost: np.ndarray, rows: np.ndarray, cols: np.ndarray, total: float):
    """Exact margin per assigned edge: forbid it, re-solve, delta total cost."""
    margins = np.zeros(len(rows))
    for k, (r, c) in enumerate(zip(rows, cols)):
        saved = cost[r, c]
        cost[r, c] = BIG
        try:
            rr, cc = linear_sum_assignment(cost)
            alt = float(cost[rr, cc].sum())
            margins[k] = alt - total if alt < BIG / 2 else np.inf
        finally:
            cost[r, c] = saved
    return margins
