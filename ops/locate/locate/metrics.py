"""Error metrics, assignment scoring with duplicate-slot equivalence, verdict triple.

Truth-known metrics (sim): position RMSE/median/p95 per class, assignment accuracy
where a device assigned to any fixture in the same duplicate-position group as its
true slot counts as correct.

The headline feasibility triple: auto-correct % (correct and not flagged),
flagged % (sent to the manual fix-up list, right or wrong), silent-wrong %
(confidently misassigned -- the deployment killer).
"""

from typing import Dict, List, Optional

import numpy as np

from .model import CadModel


def _group_lookup(cad: CadModel) -> Dict[str, frozenset]:
    lut = {}
    for g in cad.duplicate_groups:
        for fid in g:
            lut[fid] = g
    return lut


def same_slot(fid_a: Optional[str], fid_b: Optional[str], cad: CadModel) -> bool:
    if fid_a is None or fid_b is None:
        return False
    if fid_a == fid_b:
        return True
    lut = _group_lookup(cad)
    return fid_b in lut.get(fid_a, frozenset())


def position_error_stats(est: np.ndarray, truth: np.ndarray, roles: List[str]) -> dict:
    err = np.linalg.norm(est - truth, axis=1)
    def stats(e):
        if len(e) == 0:
            return {"rmse_m": np.nan, "median_m": np.nan, "p95_m": np.nan, "n": 0}
        return {
            "rmse_m": float(np.sqrt(np.mean(e ** 2))),
            "median_m": float(np.median(e)),
            "p95_m": float(np.percentile(e, 95)),
            "n": int(len(e)),
        }
    out = {"overall": stats(err), "per_role": {}}
    for role in sorted(set(roles)):
        mask = np.array([r == role for r in roles])
        out["per_role"][role] = stats(err[mask])
    return out


def assignment_accuracy(
    assign: List[Optional[str]], truth: List[Optional[str]], roles: List[str], cad: CadModel
) -> dict:
    correct = np.array([same_slot(a, t, cad) for a, t in zip(assign, truth)])
    out = {
        "overall": float(np.mean(correct)),
        "wrong_idx": [int(k) for k in np.nonzero(~correct)[0]],
        "per_role": {},
    }
    for role in sorted(set(roles)):
        mask = np.array([r == role for r in roles])
        out["per_role"][role] = float(np.mean(correct[mask])) if mask.any() else np.nan
    return out


def verdict_triple(
    assign: List[Optional[str]],
    truth: List[Optional[str]],
    flagged: np.ndarray,
    cad: CadModel,
) -> dict:
    correct = np.array([same_slot(a, t, cad) for a, t in zip(assign, truth)])
    flagged = np.asarray(flagged, bool)
    n = len(correct)
    return {
        "auto_correct": float(np.sum(correct & ~flagged) / n),
        "flagged": float(np.sum(flagged) / n),
        "silent_wrong": float(np.sum(~correct & ~flagged) / n),
        "n": n,
    }
