"""Load the fixtures.json CAD export, normalize units, synthesize the perimeter ring.

Scale handling: the vendored export (resonance.fixtures/0.3, Blender, Z-up) claims
units "m" but spans +-50 units; the real tree is ~7.5 m tall and per fleet spec the
downlights hang at 7-10 ft REGARDLESS of what the CAD says (Ben, 2026-07-12: the
7-10 ft band is ground truth; do not treat the CAD scale as truth). Scale modes:

    "auto:downlights"   scale so the highest downlight sits at 10 ft (3.048 m);
                        the bulk of CAD downlight z (28.6..40.5 units) then lands
                        at ~2.15..3.05 m = 7..10 ft.  DEFAULT.
    "auto:tree=<H>"     scale so the highest fixture of any class sits at <H> m.
    "<float>"           explicit meters-per-unit ("slider").

The export has no perimeter class (38-40 fixtures on 5 ft shepherd hooks around the
tree); synthesize a parametric ring. It also contains groups of exactly-duplicated
positions; those become duplicate_groups so assignment scoring can treat within-group
swaps as correct.
"""

import json
from typing import Optional

import numpy as np

from .model import CadFixture, CadModel

FT = 0.3048
DOWNLIGHT_TOP_M = 10 * FT          # top of the 7-10 ft downlight hang band
PERIMETER_Z_M = 5 * FT             # 5 ft shepherd hooks


def _resolve_scale(spec, fixtures_raw) -> float:
    if isinstance(spec, (int, float)):
        return float(spec)
    spec = str(spec)
    try:
        return float(spec)
    except ValueError:
        pass
    zs_all = [f["position"][2] for f in fixtures_raw]
    if spec == "auto:downlights":
        zs = [f["position"][2] for f in fixtures_raw if f.get("role") == "downlight"]
        return DOWNLIGHT_TOP_M / max(zs)
    if spec.startswith("auto:tree="):
        return float(spec.split("=", 1)[1]) / max(zs_all)
    raise ValueError(f"unknown cad scale spec: {spec!r}")


def _duplicate_groups(fixtures, eps_m: float):
    """Group fixture_ids whose positions coincide within eps_m (union-find-lite)."""
    n = len(fixtures)
    pos = np.array([f.pos_m for f in fixtures])
    parent = list(range(n))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for a in range(n):
        d = np.linalg.norm(pos[a + 1:] - pos[a], axis=1)
        for off in np.nonzero(d < eps_m)[0]:
            ra, rb = find(a), find(a + 1 + int(off))
            if ra != rb:
                parent[rb] = ra
    groups = {}
    for a in range(n):
        groups.setdefault(find(a), []).append(fixtures[a].fixture_id)
    return [frozenset(g) for g in groups.values() if len(g) > 1]


def load_cad(
    path: str,
    scale="auto:downlights",
    perimeter_n: int = 40,
    perimeter_radius_m: Optional[float] = None,   # None -> 1.15 x max downlight xy radius
    perimeter_z_m: float = PERIMETER_Z_M,
    perimeter_jitter_m: float = 0.0,
    seed: int = 0,
    dup_eps_m: float = 0.01,
) -> CadModel:
    with open(path) as fh:
        doc = json.load(fh)
    raw = doc["fixtures"]
    s = _resolve_scale(scale, raw)

    fixtures = [
        CadFixture(
            fixture_id=f["fixture_id"],
            role=f["role"],
            pos_m=np.asarray(f["position"], dtype=float) * s,
        )
        for f in raw
    ]

    if perimeter_n > 0:
        down_xy = np.array([f.pos_m[:2] for f in fixtures if f.role == "downlight"])
        radius = (
            perimeter_radius_m
            if perimeter_radius_m is not None
            else 1.15 * float(np.max(np.linalg.norm(down_xy, axis=1)))
        )
        center = down_xy.mean(axis=0)
        rng = np.random.default_rng(seed)
        for k in range(perimeter_n):
            ang = 2 * np.pi * k / perimeter_n
            p = np.array(
                [center[0] + radius * np.cos(ang), center[1] + radius * np.sin(ang), perimeter_z_m]
            )
            if perimeter_jitter_m > 0:
                p[:2] += rng.normal(0, perimeter_jitter_m, size=2)
            fixtures.append(
                CadFixture(fixture_id=f"P{k:03d}", role="perimeter", pos_m=p, synthetic=True)
            )

    return CadModel(
        fixtures=fixtures,
        duplicate_groups=_duplicate_groups(fixtures, dup_eps_m),
        scale_m_per_unit=s,
        source=f"{path} scale={scale}->{s:.5f} m/unit",
    )
