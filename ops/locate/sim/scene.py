"""Ground-truth scene: devices placed at (a seeded subset of) CAD slots + jitter.

Reality will not match CAD exactly -- lanterns are hand-hung on limbs and hooks
placed by eye -- so devices get placement jitter around their slot. The jittered
position is the ground truth (the downward ToF measures the REAL height, not the
CAD height). 72 production downlights populate a seeded subset of the 78 CAD
downlight slots; other classes fill all slots.
"""

from dataclasses import dataclass
from typing import List

import numpy as np

from locate.model import CadModel, Device


@dataclass
class Scene:
    devices: List[Device]
    truth_pos: np.ndarray        # (N,3) meters
    truth_fixture: List[str]     # device idx -> fixture_id actually occupied
    cad: CadModel


def build_scene(
    cad: CadModel,
    n_downlights: int = 72,
    placement_jitter_m: float = 0.15,
    seed: int = 0,
) -> Scene:
    rng = np.random.default_rng(seed)
    by_role = cad.indices_by_role()

    chosen = []
    for role, idxs in sorted(by_role.items()):
        if role == "downlight" and n_downlights < len(idxs):
            pick = sorted(rng.choice(len(idxs), size=n_downlights, replace=False))
            chosen.extend(idxs[k] for k in pick)
        else:
            chosen.extend(idxs)

    devices, pos, fids = [], [], []
    for k, fx_idx in enumerate(chosen):
        f = cad.fixtures[fx_idx]
        p = f.pos_m + rng.normal(0, placement_jitter_m, size=3)
        p[2] = max(p[2], 0.05)     # nothing below ground
        devices.append(Device(dev_id=f"S{k:03d}", role=f.role))
        pos.append(p)
        fids.append(f.fixture_id)

    return Scene(devices=devices, truth_pos=np.array(pos), truth_fixture=fids, cad=cad)
