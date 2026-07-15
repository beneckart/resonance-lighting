"""Registration onto CAD + device->fixture assignment.

Because z is metrically pinned by the ToF anchors, the residual gauge after
refinement is only: rotation about z (theta), an optional xy mirror, and
translation. That is globally searchable -- no ICP-style local alternation with
random restarts:

    T(x) = Rz(theta) . diag(m, 1, 1) . x + t        m in {+1, -1}

Grid theta x mirror; at each grid point a per-class rectangular LAP gives the
best assignment cost; the argmin is polished by alternating [per-class LAP] <->
[closed-form 2D Procrustes fit of (theta, t)] to a fixed point. Deliberately NO
scale freedom: scale is anchor-fixed, and letting the registration fit scale
would silently absorb P0 calibration error and corrupt the feasibility verdict.

cad_self_symmetry() runs the same grid CAD-vs-CAD to quantify rotational/mirror
near-symmetry of the layout -- the failure shape where assignment locks into a
rotated solution with high confidence (correlated silent-wrong). Beacon devices
(--known-assignments) pin LAP rows and close that risk.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional

import numpy as np

from .assign import BIG, lap_margins, solve_lap
from .model import CadModel


@dataclass
class RegisterResult:
    R: np.ndarray                    # (3,3) rotation (incl. mirror)
    t: np.ndarray                    # (3,)
    theta_deg: float
    mirror: int                      # +1 / -1
    assignment: List[Optional[str]]  # device idx -> fixture_id
    margins: np.ndarray              # per device, LAP margin (m^2 units), inf if pinned
    cost: float
    diagnostics: dict = field(default_factory=dict)


def _rz(theta: float, mirror: int = 1) -> np.ndarray:
    c, s = np.cos(theta), np.sin(theta)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]]) @ np.diag([float(mirror), 1.0, 1.0])


def _class_lap_cost(Xt, roles, cad: CadModel, z_weight, known_fixture_by_dev,
                    compute_margins: bool = False):
    """Per-class LAPs on squared distances. Returns (total, assignment idx list, margins).
    Margins (n extra LAP solves per class) only on request -- skip during grid search."""
    fx_by_role = cad.indices_by_role()
    assign = [None] * len(Xt)
    margins = np.full(len(Xt), np.nan)
    total = 0.0
    for role, fx_idx in fx_by_role.items():
        dev_idx = [k for k, r in enumerate(roles) if r == role]
        if not dev_idx:
            continue
        if len(dev_idx) > len(fx_idx):
            raise ValueError(f"more devices than CAD slots for role {role}")
        P = Xt[dev_idx]
        Q = np.array([cad.fixtures[k].pos_m for k in fx_idx])
        d2 = ((P[:, None, :2] - Q[None, :, :2]) ** 2).sum(-1) \
            + z_weight * (P[:, None, 2] - Q[None, :, 2]) ** 2
        for row, dk in enumerate(dev_idx):
            fid = known_fixture_by_dev.get(dk)
            if fid is not None:
                col_map = {cad.fixtures[k].fixture_id: c for c, k in enumerate(fx_idx)}
                if fid in col_map:
                    # pin the pairing but keep the REAL displacement cost: a
                    # zero-cost pin would let a wrong rotation park the beacon
                    # anywhere for free -- beacons must anchor the gauge
                    keep = d2[row, col_map[fid]]
                    d2[row, :] = BIG
                    d2[row, col_map[fid]] = keep
        rows, cols, tot = solve_lap(d2)
        m = lap_margins(d2, rows, cols, tot) if compute_margins else np.full(len(rows), np.nan)
        for r, c, mg in zip(rows, cols, m):
            assign[dev_idx[r]] = fx_idx[c]
            margins[dev_idx[r]] = mg
        total += tot
    return total, assign, margins


def _procrustes_2d(P: np.ndarray, Q: np.ndarray):
    """Closed-form rotation-about-z + translation minimizing ||Rz P + t - Q||^2
    (mirror already applied to P). Returns (theta, t3)."""
    pc, qc = P.mean(axis=0), Q.mean(axis=0)
    a = P[:, :2] - pc[:2]
    b = Q[:, :2] - qc[:2]
    num = float(np.sum(a[:, 0] * b[:, 1] - a[:, 1] * b[:, 0]))
    den = float(np.sum(a[:, 0] * b[:, 0] + a[:, 1] * b[:, 1]))
    theta = np.arctan2(num, den)
    R = _rz(theta)
    t = qc - R @ pc
    return theta, R, t


def register(
    X: np.ndarray,
    roles: List[str],
    cad: CadModel,
    known: Optional[Dict[int, str]] = None,
    theta_step_deg: float = 2.0,
    z_weight: float = 1.0,
    tz_bound_m: float = 0.5,
    max_polish_iters: int = 10,
) -> RegisterResult:
    known = known or {}
    cx = X.mean(axis=0)
    cy = np.array([f.pos_m for f in cad.fixtures]).mean(axis=0)
    fid_pos = {f.fixture_id: f.pos_m for f in cad.fixtures}

    # 3+ beacons pin the gauge in closed form. NOT 2: two points in the plane
    # are fit exactly by EITHER mirror with a suitable rotation, so the mirror
    # choice is a coin flip (measured: the 2-beacon arm collapsed to ~0.44
    # accuracy on mirror flips). 1-2 beacons instead inform the full grid
    # search through their real displacement cost in the pinned LAP rows.
    beacon_gauge = len(known) >= 3
    if beacon_gauge:
        # surveyed devices pin the gauge COARSELY in closed form (their own
        # position error maps to ~10 deg of theta -- one slot on the perimeter
        # ring), so keep a restricted fine grid around the beacon estimate and
        # let the full-cloud data pick the precise rotation inside it.
        P = np.array([X[dk] for dk in known])
        Q = np.array([fid_pos[fid] for fid in known.values()])
        bbest = None
        for mirror_ in (1, -1):
            Pm = P @ np.diag([float(mirror_), 1.0, 1.0]).T
            theta_, Rz_only, t_ = _procrustes_2d(Pm, Q)
            R_ = Rz_only @ np.diag([float(mirror_), 1.0, 1.0])
            resid = float(np.sum((P @ R_.T + t_ - Q)[:, :2] ** 2))
            if bbest is None or resid < bbest[0]:
                bbest = (resid, theta_, mirror_)
        _, theta0, mirror0 = bbest
        mirrors = (mirror0,)
        thetas = theta0 + np.deg2rad(np.arange(-20.0, 20.0 + 1e-9, 1.0))
    else:
        mirrors = (1, -1)
        thetas = np.deg2rad(np.arange(0.0, 360.0, theta_step_deg))

    curves = {}
    best = None
    for mirror in mirrors:
        costs = np.empty(len(thetas))
        for k, th in enumerate(thetas):
            R = _rz(th, mirror)
            Xt = (X - cx) @ R.T + cy
            costs[k], _, _ = _class_lap_cost(Xt, roles, cad, z_weight, known)
            if best is None or costs[k] < best[0]:
                best = (costs[k], th, mirror)
        curves[mirror] = costs

    _, theta, mirror = best
    R = _rz(theta, mirror)
    t = cy - R @ cx

    # polish: alternate LAP <-> closed-form gauge fit to a fixed point
    prev_assign = None
    for _ in range(max_polish_iters):
        Xt = X @ R.T + t
        cost, assign, margins = _class_lap_cost(Xt, roles, cad, z_weight, known)
        if assign == prev_assign:
            break
        prev_assign = assign
        P = X @ np.diag([float(mirror), 1.0, 1.0]).T
        Q = np.array([cad.fixtures[a].pos_m for a in assign])
        theta, Rz_only, t = _procrustes_2d(P, Q)
        R = Rz_only @ np.diag([float(mirror), 1.0, 1.0])
        tz = float(np.clip(np.mean(Q[:, 2] - (X @ R.T)[:, 2]), -tz_bound_m, tz_bound_m))
        t = np.array([t[0], t[1], tz])

    Xt = X @ R.T + t
    cost, assign, margins = _class_lap_cost(Xt, roles, cad, z_weight, known,
                                            compute_margins=True)
    for dk in known:
        margins[dk] = np.inf

    # global registration ambiguity: the per-device LAP margin cannot see a
    # WHOLESALE rotation alternative (a wrong theta is self-consistent inside
    # its own assignment). Compare, ON THE GRID (same footing), the best cost
    # near the chosen theta/mirror against the best cost away from it -- a
    # ratio near 1 means a competing rotation explains the cloud almost as
    # well (correlated silent-wrong risk; beacons or asymmetry close it).
    if beacon_gauge:
        ambiguity_ratio = np.inf     # gauge externally fixed by surveyed devices
        competing = np.inf
    else:
        grid_best, competing = np.inf, np.inf
        final_theta = np.rad2deg(theta) % 360.0
        for m_, costs in curves.items():
            for k, th_deg in enumerate(np.rad2deg(thetas)):
                dth = abs((th_deg - final_theta + 180) % 360 - 180)
                if m_ == mirror and dth < 15.0:
                    grid_best = min(grid_best, costs[k])
                else:
                    competing = min(competing, costs[k])
        ambiguity_ratio = float(competing / max(grid_best, 1e-9))

    return RegisterResult(
        R=R,
        t=t,
        theta_deg=float(np.rad2deg(theta) % 360.0),
        mirror=mirror,
        assignment=[cad.fixtures[a].fixture_id if a is not None else None for a in assign],
        margins=margins,
        cost=float(cost),
        diagnostics={
            "theta_deg_grid": np.rad2deg(thetas),
            "cost_curve_m1": curves.get(1, np.zeros(0)),
            "cost_curve_m-1": curves.get(-1, np.zeros(0)),
            "competing_cost": float(competing),
            "ambiguity_ratio": ambiguity_ratio,
            "beacon_gauge": beacon_gauge,
        },
    )


def cad_self_symmetry(cad: CadModel, theta_step_deg: float = 2.0, exclusion_deg: float = 10.0):
    """Register the CAD layout onto itself over the theta grid (both mirrors) and
    report the best competing minimum outside the trivial identity. Returns dict
    with the curves and competing_rms_m = sqrt(best competing cost / N): if this
    is comparable to expected position error, the layout is dangerously symmetric."""
    pos = np.array([f.pos_m for f in cad.fixtures])
    roles = [f.role for f in cad.fixtures]
    c = pos.mean(axis=0)
    thetas = np.deg2rad(np.arange(0.0, 360.0, theta_step_deg))
    out = {"theta_deg_grid": np.rad2deg(thetas)}
    competing = np.inf
    for mirror in (1, -1):
        costs = np.empty(len(thetas))
        for k, th in enumerate(thetas):
            R = _rz(th, mirror)
            Xt = (pos - c) @ R.T + c
            costs[k], _, _ = _class_lap_cost(Xt, roles, cad, 1.0, {})
            trivial = mirror == 1 and (
                np.rad2deg(th) < exclusion_deg or np.rad2deg(th) > 360 - exclusion_deg
            )
            if not trivial:
                competing = min(competing, costs[k])
        out[f"cost_curve_m{mirror}"] = costs
    out["competing_cost"] = float(competing)
    out["competing_rms_m"] = float(np.sqrt(competing / len(pos)))
    return out
