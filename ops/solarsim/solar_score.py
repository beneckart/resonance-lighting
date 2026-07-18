#!/usr/bin/env python3
"""Score a light placement for solar access -- native Python solar sim.

Usage:
  ./solar_score.py --placement ../../solar-visualizer/canopy_positions_corrected.json
  ./solar_score.py --validate            # compare against shipped phase-2 data

Raytraces every placement position against the woven-tree mesh (84 shipped
10-min sun slots) and applies the v5 power chain. Positions default to
face-up panels at attach + 0.109 m (the phase-2 canopy convention); a
placement entry may carry "n": [x,y,z] to override.

Calibration status (see README.md): ranking vs the SketchUp reference is
excellent (Spearman 0.92-0.96 across all 94 panels); absolute lit runs ~10
points conservative -- use for RELATIVE layout comparison, and the SketchUp
rerun for bankable Wh.
"""

import argparse
import json
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from power import panel_power                                    # noqa: E402
from raytrace import load_tree_mesh, sky_view_factor, solar_access  # noqa: E402

MESH = os.path.join(HERE, "data", "tree_mesh.ply")
PHASE2 = os.path.join(HERE, "data", "solar_phase2_data.json")


def score(positions, normals, ids, suns, mesh):
    res = solar_access(mesh, positions + normals * 0.109, normals, suns)
    svf = sky_view_factor(mesh, positions)
    w, wh, rt = panel_power(res.lit, normals, svf, suns)
    return res.lit, svf, w, wh, rt


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--placement", help="placement JSON (canopy_positions_corrected"
                                        " format: [{fixture_id|id, pos_m, [n]}...])")
    ap.add_argument("--validate", action="store_true",
                    help="score the shipped 94-panel set and compare")
    ap.add_argument("--out", help="write per-light results JSON here")
    args = ap.parse_args()

    d = json.load(open(PHASE2))
    suns = np.array([s if s else [0, 0, -1] for s in d["suns"]])
    mesh = load_tree_mesh(MESH)

    if args.validate:
        panels = d["panels"]
        ids = list(panels.keys())
        P = np.array([panels[k]["pos"] for k in ids])
        N = np.array([panels[k]["n"] for k in ids])
        lit, svf, w, wh, rt = score(P, N, ids, suns, mesh)
        ship_wh = np.array([panels[k].get("wh_day_batt", np.nan) for k in ids], float)
        ours, theirs = wh, ship_wh
        ok = np.isfinite(theirs)
        r = np.corrcoef(ours[ok], theirs[ok])[0, 1]
        ra, rb = np.argsort(np.argsort(ours[ok])), np.argsort(np.argsort(theirs[ok]))
        print(f"wh_day_batt vs shipped: pearson {r:.3f} spearman "
              f"{np.corrcoef(ra, rb)[0, 1]:.3f}  ours median {np.median(ours[ok]):.1f} "
              f"vs shipped {np.median(theirs[ok]):.1f} Wh")
        rows = [(ids[i], wh[i], rt[i], svf[i]) for i in range(len(ids))]
    else:
        doc = json.load(open(args.placement))
        entries = doc["canopy"] if isinstance(doc, dict) and "canopy" in doc else doc
        entries = [e for e in entries if "duplicate_of" not in e]
        ids = [e.get("fixture_id") or e.get("id") for e in entries]
        P = np.array([e["pos_m"] for e in entries], float)
        N = np.array([e.get("n", [0, 0, 1]) for e in entries], float)
        lit, svf, w, wh, rt = score(P, N, ids, suns, mesh)
        rows = sorted(((ids[i], wh[i], rt[i], svf[i]) for i in range(len(ids))),
                      key=lambda x: x[1])
        print(f"{len(ids)} lights  wh_day_batt: median {np.median(wh):.1f}  "
              f"min {wh.min():.1f}  max {wh.max():.1f}  "
              f"runtime_full median {np.median(rt):.1f} h")
        print("weakest 8:")
        for i, (k, e, r_, s) in enumerate(rows[:8]):
            print(f"  {k:16s} {e:5.1f} Wh/day  {r_:4.1f} h full  svf {s:.2f}")

    if args.out:
        json.dump({"lights": [{"id": k, "wh_day_batt": round(float(e), 2),
                               "runtime_full_h": round(float(r_), 2),
                               "svf": round(float(s), 3)}
                              for k, e, r_, s in rows]},
                  open(args.out, "w"), indent=1)
        print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
