"""End-to-end solve: links + anchors + CAD -> positions, assignment, confidence.

Stage order: aggregate (done by caller via rssi.aggregate_directed) -> distance
matrix at the prior path-loss model -> Floyd-Warshall completion -> classical MDS
-> anchor z-alignment (+ optional SMACOF) -> robust NLS refinement -> gauge search
registration + per-class assignment -> metrics/confidence.

Deliberately NO feedback of matched CAD positions into the RSSI refinement: CAD
must not leak into the estimated geometry or the feasibility verdict is corrupt.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional

import numpy as np

from . import embed
from .metrics import _group_lookup
from .model import (CadModel, Device, DeviceReport, LinkObs, PathLossParams,
                    SolveResult, ZAnchor)
from .refine import RefineConfig, refine
from .register import cad_self_symmetry, register


@dataclass
class SolveConfig:
    pl_prior: PathLossParams = field(default_factory=PathLossParams)
    refine: RefineConfig = field(default_factory=RefineConfig)
    use_smacof: bool = False
    smacof_iters: int = 100
    theta_step_deg: float = 2.0
    z_weight: float = 1.0
    known_assignments: Dict[int, str] = field(default_factory=dict)
    d_cap_m: float = 60.0
    # flagging: margin below frac * (class median nn spacing)^2, or low degree
    flag_margin_frac: float = 0.25
    flag_degree_min: int = 8
    bootstrap_B: int = 0             # 0 = off (sweeps); 20 for single runs
    bootstrap_seed: int = 0


def _class_nn_spacing(cad: CadModel) -> Dict[str, float]:
    out = {}
    for role, idxs in cad.indices_by_role().items():
        P = np.array([cad.fixtures[k].pos_m for k in idxs])
        if len(P) < 2:
            out[role] = 1.0
            continue
        d = np.linalg.norm(P[:, None] - P[None, :], axis=2)
        np.fill_diagonal(d, np.inf)
        out[role] = float(np.median(d.min(axis=1)))
    return out


def _solve_once(devices, links, anchors, cad, cfg):
    n = len(devices)
    D, observed = embed.distance_matrix(n, links, cfg.pl_prior, d_cap_m=cfg.d_cap_m)
    Dc = embed.complete_distances(D)
    X0, eigvals = embed.classical_mds(Dc)
    X1, align_info = embed.align_to_anchors(X0, anchors)
    if cfg.use_smacof:
        X1 = embed.smacof(X1, links, cfg.pl_prior, iters=cfg.smacof_iters)
        X1, align_info = embed.align_to_anchors(X1, anchors)
    X2, offsets, pl_fit, rdiag = refine(X1, links, anchors, cfg.pl_prior, cfg.refine)
    roles = [d.role for d in devices]
    reg = register(X2, roles, cad, known=cfg.known_assignments,
                   theta_step_deg=cfg.theta_step_deg, z_weight=cfg.z_weight)
    X_cad = X2 @ reg.R.T + reg.t
    return X_cad, offsets, pl_fit, reg, rdiag, align_info, observed


def solve(
    devices: List[Device],
    links: List[LinkObs],
    anchors: List[ZAnchor],
    cad: CadModel,
    cfg: SolveConfig = None,
) -> SolveResult:
    cfg = cfg or SolveConfig()
    n = len(devices)
    X_cad, offsets, pl_fit, reg, rdiag, align_info, observed = _solve_once(
        devices, links, anchors, cad, cfg)

    # bootstrap agreement (optional): resample per-link samples, re-solve, compare
    agreement = np.full(n, np.nan)
    if cfg.bootstrap_B > 0:
        rng = np.random.default_rng(cfg.bootstrap_seed)
        agree = np.zeros(n)
        runs = 0
        lut = _group_lookup(cad)
        for _ in range(cfg.bootstrap_B):
            blinks = []
            for l in links:
                if l.samples:
                    s = rng.choice(l.samples, size=len(l.samples), replace=True)
                    r = float(np.median(s))
                else:
                    r = l.rssi_dbm + float(rng.normal(0, 1.0 / np.sqrt(max(l.n_samples, 1))))
                blinks.append(LinkObs(l.i, l.j, r, l.n_samples, l.asym_db))
            try:
                _, _, _, breg, _, _, _ = _solve_once(devices, blinks, anchors, cad, cfg)
            except Exception:
                continue
            runs += 1
            for k in range(n):
                a, b = reg.assignment[k], breg.assignment[k]
                same = a == b or (a in lut and b in lut.get(a, frozenset()))
                agree[k] += 1.0 if same else 0.0
        if runs:
            agreement = agree / runs

    spacing = _class_nn_spacing(cad)
    roles = [d.role for d in devices]
    reports = []
    for k, d in enumerate(devices):
        margin = reg.margins[k]
        thresh = cfg.flag_margin_frac * spacing[d.role] ** 2
        flagged = (
            (np.isfinite(margin) and margin < thresh)
            or rdiag["degree"][k] < cfg.flag_degree_min
            or (not np.isnan(agreement[k]) and agreement[k] < 0.9)
        )
        reports.append(DeviceReport(
            dev_id=d.dev_id, role=d.role, degree=int(rdiag["degree"][k]),
            fixture_id=reg.assignment[k], sigma_pos_m=float(rdiag["sigma_pos_m"][k]),
            margin_cost=float(margin), bootstrap_agreement=float(agreement[k]),
            flagged=bool(flagged),
        ))

    diagnostics = {
        "align": align_info,
        "refine": {k: v for k, v in rdiag.items() if k not in ("sigma_pos_m", "degree")},
        "register": {
            "theta_deg": reg.theta_deg, "mirror": reg.mirror, "cost": reg.cost,
            "theta_deg_grid": reg.diagnostics["theta_deg_grid"],
            "cost_curve_m1": reg.diagnostics["cost_curve_m1"],
            "cost_curve_m-1": reg.diagnostics["cost_curve_m-1"],
        },
        "n_links": len(links),
        "link_fill": float(2 * len(links) / (n * (n - 1))) if n > 1 else 0.0,
    }
    return SolveResult(
        pos_m=X_cad,
        assignment=list(reg.assignment),
        pathloss=pl_fit,
        offsets_db=offsets,
        per_device=reports,
        diagnostics=diagnostics,
    )
