"""ToF anchor synthesis: downlight TMF8820 (downward) + perimeter VL53L5CX (outward).

Downlights: the downward 3x3 ToF reads height above ground directly; per the
2026-07-12 direction the (nighttime) range comfortably covers the 7-10 ft hang
band, so every downlight anchors by default; tof_max_range_m remains a stress
knob for the `anchors` sweep suite.

Perimeter: VL53L5CX 8x8 (~45 deg FoV) faces outward at people height; only its
lower zones see the ground, and only because the mount is ~5 ft up with a wide
FoV. Zone rays are built on a tangent-plane grid (same construction as
firmware/sway_demo tofBuildRays), pitched down by mount_downtilt_deg, intersected
with the ground plane, range-gated, noised, then locate.planefit recovers the
height exactly the way the hardware does -- fit residual propagates to the anchor
sigma.
"""

from dataclasses import dataclass
from typing import List

import numpy as np

from locate.model import ZAnchor
from locate.planefit import fit_ground_plane


@dataclass
class TofParams:
    # downlight TMF8820
    down_sigma_m: float = 0.010
    tof_max_range_m: float = 6.0       # generous: nighttime range covers the hang band
    # perimeter VL53L5CX
    grid: int = 8
    fov_deg: float = 45.0
    mount_downtilt_deg: float = 15.0
    range_max_m: float = 4.0
    sigma_pct: float = 0.01            # 1 % of range
    sigma_floor_m: float = 0.003
    zone_dropout: float = 0.05
    min_ground_zones: int = 6


def downlight_anchors(scene, params: TofParams, seed: int = 0) -> List[ZAnchor]:
    rng = np.random.default_rng(seed)
    out = []
    for k, dev in enumerate(scene.devices):
        if dev.role != "downlight":
            continue
        z = float(scene.truth_pos[k][2])
        if z > params.tof_max_range_m:
            continue
        out.append(ZAnchor(idx=k, z_m=z + float(rng.normal(0, params.down_sigma_m)),
                           sigma_m=params.down_sigma_m))
    return out


def _zone_rays(grid: int, fov_deg: float, downtilt_deg: float) -> np.ndarray:
    """Zone-center rays on a tangent-plane grid across the FoV (mirrors sway_demo
    tofBuildRays), boresight +x pitched down by downtilt. Device frame, z up."""
    half = np.tan(np.deg2rad(fov_deg / 2))
    u = (np.arange(grid) + 0.5) / grid * 2 - 1     # zone centers
    c, s = np.cos(np.deg2rad(downtilt_deg)), np.sin(np.deg2rad(downtilt_deg))
    rays = []
    for uy in u * half:
        for uz in u * half:
            ray = np.array([1.0, uy, uz])
            ray /= np.linalg.norm(ray)
            rays.append([c * ray[0] + s * ray[2], ray[1], -s * ray[0] + c * ray[2]])
    return np.array(rays)


def perimeter_anchors(scene, params: TofParams, seed: int = 0) -> List[ZAnchor]:
    rng = np.random.default_rng(seed)
    rays = _zone_rays(params.grid, params.fov_deg, params.mount_downtilt_deg)
    out = []
    for k, dev in enumerate(scene.devices):
        if dev.role != "perimeter":
            continue
        h = float(scene.truth_pos[k][2])
        pts = []
        for ray in rays:
            if ray[2] >= -1e-9:
                continue                       # ray never reaches the ground
            rng_m = h / -ray[2]
            if rng_m > params.range_max_m:
                continue
            if rng.random() < params.zone_dropout:
                continue
            noise = rng.normal(0, max(params.sigma_pct * rng_m, params.sigma_floor_m))
            pts.append((rng_m + noise) * ray)
        if len(pts) < params.min_ground_zones:
            continue
        fit = fit_ground_plane(np.array(pts), min_points=params.min_ground_zones)
        if not fit.ok:
            continue
        sigma = max(fit.resid_rms_m / np.sqrt(max(fit.n_used, 1)), 0.005)
        out.append(ZAnchor(idx=k, z_m=fit.height_m, sigma_m=float(sigma)))
    return out


def make_anchors(scene, params: TofParams = None, seed: int = 0) -> List[ZAnchor]:
    params = params or TofParams()
    return downlight_anchors(scene, params, seed) + perimeter_anchors(scene, params, seed + 1)
