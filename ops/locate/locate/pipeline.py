"""End-to-end solve: links + anchors + CAD -> positions, assignment, confidence.

Stage order: aggregate (done by caller via rssi.aggregate_directed) -> distance
matrix at the prior path-loss model -> Floyd-Warshall completion -> classical MDS
-> anchor z-alignment (+ optional SMACOF) -> robust NLS refinement -> gauge search
registration + per-class assignment -> metrics/confidence.

Deliberately NO feedback of matched CAD positions into the RSSI refinement: CAD
must not leak into the estimated geometry or the feasibility verdict is corrupt.
"""

from dataclasses import dataclass, field, replace
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
    init_strategy: str = "auto"          # "auto" = run plain AND smacof arms, keep the
                                         # one with lower registration cost (each arm is
                                         # a different local-minimum basin; neither
                                         # dominates across noise draws)
    use_smacof: bool = False             # arm selector used internally by _solve_once
    smacof_iters: int = 100
    theta_step_deg: float = 2.0
    z_weight: float = 1.0
    known_assignments: Dict[int, str] = field(default_factory=dict)
    d_cap_m: float = 60.0
    min_anchors_for_2d_init: int = 12    # below this, fall back to 3D MDS + alignment
    reg_ambiguity_min: float = 1.5       # registration cost ratio (best competing
                                         # rotation / chosen) below which the WHOLE
                                         # registration is ambiguous -> flag everyone
    rescue_rounds: int = 2               # re-seed + warm-restart for stranded devices
    rescue_sigma_factor: float = 3.0     # sigma_pos > factor * median(sigma_pos) ...
    rescue_sigma_floor_m: float = 0.5    # ... AND > this absolute floor -> rescue
    flag_sigma_factor: float = 3.0       # same signal also feeds the manual-fix-up flag
    trim_rounds: int = 0                 # optional: drop gross per-link outliers, re-refine
    trim_k: float = 3.0                  # |link residual| > k * sigma_link -> drop ...
    trim_max_frac: float = 0.10          # ... but never more than this fraction
    cad_size_rescale: bool = True        # correspondence-free global size hint from CAD
    size_rescale_min_log: float = 0.05   # skip below ~5% size mismatch
    floor_dbm: float = -90.0             # receiver sensitivity floor
    censor_guard_db: float = 6.0         # links aggregated within this of the floor are
                                         # survivor-biased (packets below the floor were
                                         # never received) -> marked censored and treated
                                         # as one-sided "at least this far" constraints
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


_REFINE_KEYS = ("cost", "nfev", "success", "sigma_pos_m", "degree")


def _merge_diag(new_rdiag, old_rdiag):
    """Refine() returns a fresh diagnostics dict; carry stage annotations
    (rescued, trimmed, cad_size_rescale_s, ...) across re-refines."""
    for k, v in old_rdiag.items():
        if k not in _REFINE_KEYS and k not in new_rdiag:
            new_rdiag[k] = v
    return new_rdiag


def _cad_size_rescale(X, offsets, pl_fit, rdiag, links, anchors, cad, cfg):
    """Correct the map's absolute xy scale from the CAD's overall size.

    A fleet-uniform attenuation (e.g. the solar panel averaged over orientation:
    ~-20 dB on EVERY link) is exactly degenerate with P0, so absolute scale is
    near-unidentifiable from RSSI + near-planar z-anchors: the NLS settles at a
    self-consistent solution at whatever scale the init implied (measured on the
    bench: a scan along the scale manifold shows the wrong-scale optimum is a
    true local minimum, because the fit warps shape/offsets to match it). The
    missing scalar is genuinely absent from the data, so supply it from the one
    thing we trust about the CAD without using any correspondence information:
    its overall size. s = rms_xy_radius(CAD) / rms_xy_radius(estimate); rescale
    xy about the centroid, shift P0 by 10 n log10(s) to keep predictions
    consistent, warm re-refine. Registration downstream keeps ZERO scale
    freedom, so the feasibility verdict still measures RSSI shape quality.
    The applied s is reported: it IS the P0 miscalibration diagnostic
    (deployment alternative: calibrate P0 with one known-distance pair).
    """
    cad_xy = np.array([f.pos_m[:2] for f in cad.fixtures])
    r_cad = float(np.sqrt(np.mean(np.sum((cad_xy - cad_xy.mean(0)) ** 2, axis=1))))
    c = X[:, :2].mean(axis=0)
    r_dev = float(np.sqrt(np.mean(np.sum((X[:, :2] - c) ** 2, axis=1))))
    s = r_cad / max(r_dev, 1e-6)
    if abs(np.log(s)) < cfg.size_rescale_min_log:
        return X, offsets, pl_fit, rdiag
    X = X.copy()
    X[:, :2] = c + (X[:, :2] - c) * s
    p0_new = pl_fit.p0_dbm + 10 * pl_fit.n * np.log10(s)
    old = rdiag
    X, offsets, pl_fit, rdiag = refine(
        X, links, anchors, cfg.pl_prior, cfg.refine,
        offsets0=offsets, p0_init=p0_new)
    rdiag = _merge_diag(rdiag, old)
    rdiag["cad_size_rescale_s"] = s
    return X, offsets, pl_fit, rdiag


def _rescue_stranded(X, offsets, pl_fit, rdiag, links, anchors, cfg):
    """Detect and re-seed devices stranded by systematic per-device attenuation.

    A device whose EVERY link is extra-attenuated (canopy/trunk shadowing --
    NLOS positive bias) looks uniformly too far away. The optimizer strands it
    OUTSIDE the cloud in a locally-stable equilibrium: from far away all peer
    distances are nearly equal, a large positive offset absorbs the mean error,
    and the median residual reads ~0 -- so residual tests miss it. What does
    expose it is the Hessian covariance proxy: a stranded device's position is
    poorly determined (sigma_pos 5-15x the population median). Detection:
    sigma_pos > factor * median AND > an absolute floor. Re-seed: move the
    device to the inverse-square-weighted centroid of its strongest peers
    (distance from raw link RSSI corrected by peer offsets), re-fit its offset
    as the median residual at the new position, warm-restart the refinement.
    """
    from .rssi import distance_from_rssi

    n = len(X)
    links_of = [[] for _ in range(n)]
    for l in links:
        if l.censored:
            continue          # floor-biased: would look spuriously close in the re-seed
        links_of[l.i].append((l.rssi_dbm, l.j))
        links_of[l.j].append((l.rssi_dbm, l.i))
    anchored_z = {a.idx: a.z_m for a in anchors}

    for _ in range(cfg.rescue_rounds):
        sigma_pos = np.asarray(rdiag["sigma_pos_m"])
        med_sig = float(np.median(sigma_pos))
        bad = np.nonzero(
            (sigma_pos > cfg.rescue_sigma_factor * med_sig)
            & (sigma_pos > cfg.rescue_sigma_floor_m)
        )[0]
        if len(bad) == 0:
            break
        for k in bad:
            rs = np.array([r for r, _ in links_of[k]])
            peers = np.array([j for _, j in links_of[k]])
            d_hat = distance_from_rssi(rs - offsets[peers], pl_fit)
            w = 1.0 / np.maximum(d_hat, 0.1) ** 2
            order = np.argsort(-w)[:10]
            ww = w[order]
            X[k, :2] = (X[peers[order], :2] * ww[:, None]).sum(0) / ww.sum()
            X[k, 2] = anchored_z.get(k, float(
                (X[peers[order], 2] * ww).sum() / ww.sum()))
            d_new = np.maximum(np.linalg.norm(X[peers] - X[k], axis=1), 1e-3)
            pred_no_off = (pl_fit.p0_dbm - 10 * pl_fit.n * np.log10(d_new)
                           + offsets[peers])
            offsets[k] = float(np.median(rs - pred_no_off))
        old = rdiag
        X, offsets, pl_fit, rdiag = refine(
            X, links, anchors, cfg.pl_prior, cfg.refine,
            offsets0=offsets, p0_init=pl_fit.p0_dbm)
        rdiag = _merge_diag(rdiag, old)
        rdiag["rescued"] = rdiag.get("rescued", []) + [int(k) for k in bad]
    return X, offsets, pl_fit, rdiag


def _solve_once(devices, links, anchors, cad, cfg):
    n = len(devices)
    if cfg.censor_guard_db > 0:
        thresh = cfg.floor_dbm + cfg.censor_guard_db
        links = [
            l if l.rssi_dbm > thresh else LinkObs(
                l.i, l.j, l.rssi_dbm, l.n_samples, l.asym_db, l.samples, censored=True)
            for l in links
        ]
    n_anchored = len({a.idx for a in anchors})

    def _init_embed(pl):
        if n_anchored >= cfg.min_anchors_for_2d_init:
            # mainline: anchor-aware init (2D MDS on the anchored subset, z measured)
            X1 = embed.anchored_init(n, links, anchors, pl, d_cap_m=cfg.d_cap_m)
            info = {"init": "anchored_2d", "n_anchors": n_anchored}
        else:
            # generic fallback: 3D MDS + anchor alignment (few anchors)
            D, _ = embed.distance_matrix(n, links, pl, d_cap_m=cfg.d_cap_m)
            Dc = embed.complete_distances(D)
            X0, eigvals = embed.classical_mds(Dc)
            X1, info = embed.align_to_anchors(X0, anchors)
            info["init"] = "mds3d_aligned"
        if cfg.use_smacof:
            X1 = embed.smacof(X1, links, pl, iters=cfg.smacof_iters)
        return X1, info

    # outer init <-> calibration loop: a badly wrong P0 prior (e.g. the fleet-
    # uniform ~-20 dB panel shadow nobody calibrated for) inflates the init,
    # and the refine bakes an irreversible shape warp in at that scale. Once
    # the CAD size hint has revealed the calibration error, redo the init from
    # scratch at the corrected path loss instead of massaging the warped map.
    pl_cur = cfg.pl_prior
    for _ in range(2):
        X1, align_info = _init_embed(pl_cur)
        X2, offsets, pl_fit, rdiag = refine(
            X1, links, anchors, cfg.pl_prior, cfg.refine, p0_init=pl_cur.p0_dbm)
        if cfg.cad_size_rescale:
            X2, offsets, pl_fit, rdiag = _cad_size_rescale(
                X2, offsets, pl_fit, rdiag, links, anchors, cad, cfg)
        s = rdiag.get("cad_size_rescale_s", 1.0)
        p0_corrected = pl_fit.p0_dbm
        if abs(np.log(s)) < 0.15 and abs(p0_corrected - pl_cur.p0_dbm) < 3.0:
            break
        pl_cur = PathLossParams(p0_dbm=p0_corrected, n=cfg.pl_prior.n,
                                d0_m=cfg.pl_prior.d0_m)
        align_info["reinit_p0_dbm"] = p0_corrected

    X2, offsets, pl_fit, rdiag = _rescue_stranded(
        X2, offsets, pl_fit, rdiag, links, anchors, cfg)
    if cfg.cad_size_rescale:
        # collapse guard: later stages can drift the size again (heavily
        # censored regimes have a point-collapse failure mode)
        X2, offsets, pl_fit, rdiag = _cad_size_rescale(
            X2, offsets, pl_fit, rdiag, links, anchors, cad, cfg)

    # trim pass: gross per-link outliers (trunk/canopy-occluded links -- an
    # unmodeled +10 dB the huber loss only dampens) get dropped outright, then
    # a warm re-refine cleans the geometry they warped
    active = list(links)
    for _ in range(cfg.trim_rounds):
        resid = np.zeros(len(active))
        for kk, l in enumerate(active):
            if l.censored:
                continue      # one-sided constraints are never trimmed
            d = max(float(np.linalg.norm(X2[l.i] - X2[l.j])), 1e-3)
            pred = (pl_fit.p0_dbm - 10 * pl_fit.n * np.log10(d)
                    + offsets[l.i] + offsets[l.j])
            resid[kk] = l.rssi_dbm - pred
        bad = np.abs(resid) > cfg.trim_k * cfg.refine.sigma_link_db
        if not bad.any():
            break
        max_drop = int(cfg.trim_max_frac * len(active))
        if bad.sum() > max_drop:
            worst = np.argsort(-np.abs(resid))[:max_drop]
            bad = np.zeros(len(active), bool)
            bad[worst] = True
        active = [l for l, b in zip(active, bad) if not b]
        X2, offsets, pl_fit, rdiag2 = refine(
            X2, active, anchors, cfg.pl_prior, cfg.refine,
            offsets0=offsets, p0_init=pl_fit.p0_dbm)
        rdiag2 = _merge_diag(rdiag2, rdiag)
        rdiag2["trimmed"] = rdiag.get("trimmed", 0) + int(bad.sum())
        rdiag = rdiag2

    roles = [d.role for d in devices]
    reg = register(X2, roles, cad, known=cfg.known_assignments,
                   theta_step_deg=cfg.theta_step_deg, z_weight=cfg.z_weight)
    X_cad = X2 @ reg.R.T + reg.t
    return X_cad, offsets, pl_fit, reg, rdiag, align_info


def solve(
    devices: List[Device],
    links: List[LinkObs],
    anchors: List[ZAnchor],
    cad: CadModel,
    cfg: SolveConfig = None,
) -> SolveResult:
    cfg = cfg or SolveConfig()
    n = len(devices)
    arms = {"auto": (False, True), "plain": (False,), "smacof": (True,)}[cfg.init_strategy]
    best = None
    for sm in arms:
        cfg_arm = replace(cfg, use_smacof=sm)
        out = _solve_once(devices, links, anchors, cad, cfg_arm)
        if best is None or out[3].cost < best[0][3].cost:
            best = (out, sm)
    (X_cad, offsets, pl_fit, reg, rdiag, align_info), winning_arm = best
    cfg = replace(cfg, use_smacof=winning_arm)   # bootstrap re-solves use the winner
    align_info["arm"] = "smacof" if winning_arm else "plain"

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
                _, _, _, breg, _, _ = _solve_once(devices, blinks, anchors, cad, cfg)
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
    med_sigma = float(np.median(rdiag["sigma_pos_m"]))
    # with 1-2 beacons the ambiguity ratio already includes their displacement
    # cost on the grid, so it is trusted as-is; only a >=3-beacon closed-form
    # gauge returns inf (externally pinned). No blanket suppression: a single
    # beacon must NOT silence a genuinely thin rotation margin.
    reg_ambiguous = (
        reg.diagnostics.get("ambiguity_ratio", np.inf) < cfg.reg_ambiguity_min
    )
    reports = []
    for k, d in enumerate(devices):
        margin = reg.margins[k]
        thresh = cfg.flag_margin_frac * spacing[d.role] ** 2
        flagged = (
            reg_ambiguous
            or (np.isfinite(margin) and margin < thresh)
            or rdiag["degree"][k] < cfg.flag_degree_min
            or rdiag["sigma_pos_m"][k] > cfg.flag_sigma_factor * med_sigma
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
            "ambiguity_ratio": reg.diagnostics.get("ambiguity_ratio"),
            "ambiguous": reg_ambiguous,
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
