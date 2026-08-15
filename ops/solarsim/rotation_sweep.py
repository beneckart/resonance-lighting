#!/usr/bin/env python3
"""Precompute solar scores vs build rotation for the viewer's rotation slider.

Usage: ./rotation_sweep.py --placement <canopy json> [--step 5] --out rot_sweep.json

For each rotation angle (0..175 deg, the daily-integral metric is 2-fold
symmetric): rotate the shipped sun vectors about z (equivalent to rotating the
build), raytrace the canopy (inner ring at 3.26 m if the placement says so --
this tool scores whatever placement it is given) and the 16 ground/trunk
lights, apply the v5 power chain, and additionally pick each ground light's
best panel orientation from tilt {0,15,30,45,60} x azimuth {0..315 step 45}.

Output JSON: {"angles": [...], "canopy_ids": [...], "ground_ids": [...],
  "canopy_wh": [[per light]...per angle], "ground_wh": [...],
  "ground_best": [[[tilt, az], ...] ...],
  "suns": <shipped>, "layout": <note>}
"""

import argparse
import json
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from power import panel_power                                       # noqa: E402
from raytrace import load_tree_mesh, sky_view_factor, solar_access  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--placement", required=True)
    ap.add_argument("--inner-radius", type=float, default=3.26,
                    help="rescale inner-ring xy to this radius (0 = leave)")
    ap.add_argument("--step", type=int, default=5)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    ship = json.load(open(os.path.join(HERE, "data", "solar_phase2_data.json")))
    suns0 = np.array([s if s else [0, 0, -1] for s in ship["suns"]])
    mesh = load_tree_mesh(os.path.join(HERE, "data", "tree_mesh.ply"))

    doc = json.load(open(args.placement))
    ent = [e for e in (doc["canopy"] if "canopy" in doc else doc)
           if "duplicate_of" not in e]
    cids = [e.get("fixture_id") or e.get("id") for e in ent]
    P = np.array([e["pos_m"] for e in ent], float)
    if args.inner_radius > 0:
        r = np.hypot(P[:, 0], P[:, 1])
        inner = np.array([e.get("ring") == "inner" for e in ent])
        P[inner, :2] *= (args.inner_radius / r[inner])[:, None]
    N = np.tile([0.0, 0.0, 1.0], (len(P), 1))
    svf = sky_view_factor(mesh, P)

    g = {k: v for k, v in ship["panels"].items() if not k.startswith("CL-")}
    gids = list(g)
    GP = np.array([g[k]["pos"] for k in gids])
    tilts = [0, 15, 30, 45, 60]
    azs = list(range(0, 360, 45))
    cand = [np.array([np.sin(np.deg2rad(t)) * np.cos(np.deg2rad(a)),
                      np.sin(np.deg2rad(t)) * np.sin(np.deg2rad(a)),
                      np.cos(np.deg2rad(t))]) for t in tilts for a in azs]
    labels = [(t, a) for t in tilts for a in azs]
    GPs = np.repeat(GP, len(cand), axis=0)
    GNs = np.tile(np.array(cand), (len(GP), 1))
    gsvf = np.repeat(sky_view_factor(mesh, GP), len(cand))

    out = {"angles": [], "canopy_ids": cids, "ground_ids": gids,
           "canopy_wh": [], "canopy_rt": [], "ground_wh": [], "ground_best": [],
           "layout": f"{os.path.basename(args.placement)} inner@{args.inner_radius}"}
    for deg in range(0, 180, args.step):
        th = np.deg2rad(deg)
        c, s = np.cos(th), np.sin(th)
        su = suns0 @ np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]]).T
        res = solar_access(mesh, P + N * 0.109, N, su)
        _, wh, rt = panel_power(res.lit, N, svf, su)
        gres = solar_access(mesh, GPs + GNs * 0.109, GNs, su)
        _, gwh, _ = panel_power(gres.lit, GNs, gsvf, su)
        gwh = gwh.reshape(len(GP), len(cand))
        best_j = np.argmax(gwh, axis=1)
        out["angles"].append(deg)
        out["canopy_wh"].append([round(float(v), 2) for v in wh])
        out["canopy_rt"].append([round(float(v), 2) for v in rt])
        out["ground_wh"].append([round(float(gwh[i, j]), 2)
                                 for i, j in enumerate(best_j)])
        out["ground_best"].append([list(labels[j]) for j in best_j])
        print(f"rot {deg:3d}: canopy median {np.median(wh):.2f} Wh", flush=True)

    json.dump(out, open(args.out, "w"), separators=(",", ":"))
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
