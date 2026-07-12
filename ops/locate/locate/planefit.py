"""Robust LS plane fit over ToF zone points -> sensor height above ground.

Mirrors the hardware-validated math in firmware/sway_demo/sway_demo.ino
(planeFitLS + the single outlier-rejection pass, ADR 0027): fit z = a x + b y + c
by normal equations over zone hit points expressed in a gravity-aligned device
frame (device at origin, z up; the MSA311 supplies gravity on hardware), reject
residuals > k sigma once, refit. The sensor's height above the ground plane is
the perpendicular distance from the origin:

    h = |c| / sqrt(a^2 + b^2 + 1)
"""

from dataclasses import dataclass

import numpy as np


@dataclass
class PlaneFit:
    abc: np.ndarray        # (3,) for z = a x + b y + c
    height_m: float        # perpendicular distance origin -> plane
    resid_rms_m: float     # RMS residual of kept points (along z)
    n_used: int
    n_total: int
    ok: bool


def _lstsq_plane(pts: np.ndarray):
    A = np.column_stack([pts[:, 0], pts[:, 1], np.ones(len(pts))])
    abc, *_ = np.linalg.lstsq(A, pts[:, 2], rcond=None)
    resid = pts[:, 2] - A @ abc
    return abc, resid


def fit_ground_plane(points_m: np.ndarray, reject_k: float = 3.0, min_points: int = 6) -> PlaneFit:
    """points_m: (M,3) zone hits in the gravity-aligned device frame."""
    pts = np.asarray(points_m, dtype=float)
    if len(pts) < min_points:
        return PlaneFit(np.zeros(3), np.nan, np.nan, 0, len(pts), False)

    abc, resid = _lstsq_plane(pts)
    sigma = float(np.std(resid))
    keep = np.abs(resid) <= max(reject_k * sigma, 1e-9)
    if keep.sum() >= min_points and keep.sum() < len(pts):
        abc, resid = _lstsq_plane(pts[keep])
    else:
        keep = np.ones(len(pts), bool)

    a, b, c = abc
    height = abs(c) / np.sqrt(a * a + b * b + 1.0)
    return PlaneFit(
        abc=abc,
        height_m=float(height),
        resid_rms_m=float(np.sqrt(np.mean(resid ** 2))),
        n_used=int(keep.sum()),
        n_total=len(pts),
        ok=True,
    )
