"""Native solar-access raytracer for the Resonance tree -- Python port of the
solar-visualizer SketchUp pipeline (branch solar-visualizer, Elliot).

Geometry: the woven-tree mesh extracted from the viewer's embedded Draco glb
(data/tree_mesh.ply, Y-up glTF frame) transformed to the study frame (meters,
tree-centered, Z-up, height 6.66 m). Sun: the 84 shipped 10-min slot unit
vectors from solar_phase2_data.json (point TOWARD the sun, Z-up). Panels:
0.5 x 0.35 m rectangles (the SketchUp SOLAR_LIGHT panel), sampled on an NxM
grid, each sample raytested toward the sun -- fraction unoccluded = lit.

Validation target: the shipped per-panel `lit` arrays (see validate_lit.py).
"""

from dataclasses import dataclass

import numpy as np
import trimesh

TREE_HEIGHT_M = 6.66
PANEL_W = 0.50          # m (SketchUp model panel: 2*0.25 x 2*0.175)
PANEL_H = 0.35
SAMPLE_GRID = (7, 5)    # across W, H -> 35 rays per panel per slot
EPS_ALONG_RAY = 0.004   # ~0.15 in self-hit offset, as the .rb does


def load_tree_mesh(path: str) -> trimesh.Trimesh:
    """Load the extracted mesh and transform glTF Y-up -> study Z-up meters."""
    m = trimesh.load(path, process=False)
    v = np.asarray(m.vertices)
    scale = TREE_HEIGHT_M / (v[:, 1].max() - v[:, 1].min())
    # glTF (x, y_up, z) -> study (x, -z, y_up), then scale to meters
    w = np.column_stack([v[:, 0], -v[:, 2], v[:, 1] - v[:, 1].min()]) * scale
    return trimesh.Trimesh(vertices=w, faces=m.faces, process=False)


def panel_sample_points(pos: np.ndarray, normal: np.ndarray) -> np.ndarray:
    """Sample grid on the panel rectangle centered at pos, oriented by normal."""
    n = normal / np.linalg.norm(normal)
    a = np.cross(n, [0.0, 0.0, 1.0])
    if np.linalg.norm(a) < 1e-6:
        a = np.array([1.0, 0.0, 0.0])
    a = a / np.linalg.norm(a)
    b = np.cross(n, a)
    gu, gv = SAMPLE_GRID
    u = (np.arange(gu) + 0.5) / gu - 0.5
    v = (np.arange(gv) + 0.5) / gv - 0.5
    uu, vv = np.meshgrid(u, v, indexing="ij")
    return (pos[None, :]
            + (uu.ravel() * PANEL_W)[:, None] * a[None, :]
            + (vv.ravel() * PANEL_H)[:, None] * b[None, :])


@dataclass
class AccessResult:
    lit: np.ndarray          # (n_panels, n_slots) fraction of face lit


def solar_access(mesh: trimesh.Trimesh, positions: np.ndarray,
                 normals: np.ndarray, suns: np.ndarray,
                 progress: bool = False) -> AccessResult:
    """lit[i, t] = fraction of panel i's sample points with a clear line to the
    sun at slot t (0 when the sun is below the panel's horizon plane)."""
    try:
        ray = trimesh.ray.ray_pyembree.RayMeshIntersector(mesh)
    except BaseException:
        ray = trimesh.ray.ray_triangle.RayMeshIntersector(mesh)

    n_p, n_s = len(positions), len(suns)
    pts = np.stack([panel_sample_points(p, n) for p, n in zip(positions, normals)])
    n_samp = pts.shape[1]
    lit = np.zeros((n_p, n_s))
    for t in range(n_s):
        s = np.asarray(suns[t], dtype=float)
        if s[2] <= 0:
            continue
        # panels can only be lit from their front side
        face_ok = (normals @ s) > 0
        idx = np.nonzero(face_ok)[0]
        if len(idx) == 0:
            continue
        origins = pts[idx].reshape(-1, 3) + s[None, :] * EPS_ALONG_RAY
        dirs = np.tile(s, (len(origins), 1))
        hit = ray.intersects_any(origins, dirs)
        frac = 1.0 - hit.reshape(len(idx), n_samp).mean(axis=1)
        lit[idx, t] = frac
        if progress and t % 12 == 0:
            print(f"  slot {t}/{n_s}")
    return AccessResult(lit=lit)


def sky_view_factor(mesh: trimesh.Trimesh, positions: np.ndarray,
                    n_rays: int = 128, seed: int = 0) -> np.ndarray:
    """Cosine-weighted upper-hemisphere visibility per position (the .rb's
    128-ray svf), sampled from 0.109 m above the attach point."""
    rng = np.random.default_rng(seed)
    u1, u2 = rng.random(n_rays), rng.random(n_rays)
    r = np.sqrt(u1)
    phi = 2 * np.pi * u2
    dirs = np.column_stack([r*np.cos(phi), r*np.sin(phi), np.sqrt(1-u1)])
    try:
        ray = trimesh.ray.ray_pyembree.RayMeshIntersector(mesh)
    except BaseException:
        ray = trimesh.ray.ray_triangle.RayMeshIntersector(mesh)
    out = np.zeros(len(positions))
    for i, p in enumerate(positions):
        o = np.tile(p + [0, 0, 0.109], (n_rays, 1)) + dirs * EPS_ALONG_RAY
        out[i] = 1.0 - ray.intersects_any(o, dirs).mean()
    return out
