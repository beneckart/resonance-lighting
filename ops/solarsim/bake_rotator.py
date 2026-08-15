#!/usr/bin/env python3
"""Rebake the rotator viewer's embedded sim data in place.

Usage: ./bake_rotator.py --html <Resonance_Solar_3D_rotator.html>
           [--offsets 0.25,0.5,1.0] [--skip-rot]

Parses the viewer's `const DATA/ROT/HINGE` blobs, recomputes every sim-derived
field (lit, w, svf, wh, runtimes) with the current raytrace.py panel footprint
against the positions/normals already baked in the viewer (the design
candidate is the source of truth for geometry), and reinjects:

- DATA.panels: all 112 (72 V-hang lanterns, 24 perimeter, 16 trunk-base)
- HINGE: the 72 lanterns at their tangential-pitch normals
- ROT: 36-angle build-rotation sweep (canopy wh/rt + per-angle best ground aim)
- DATA.hang_variants (new): per-lantern field-sets at each --offsets drop
  below the modeled hang, clearance-checked vs the 7 ft panel-bottom rule
  (viewer's hang-offset selector swaps these in, HINGE-style)

Slot conventions match the phase-2 bake: w rounded 2dp / lit percent 1dp,
null on below-horizon slots; runtime = batt Wh / 1.364 W full, / 0.5 W dim.
"""

import argparse
import json
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from power import panel_power                                       # noqa: E402
from raytrace import load_tree_mesh, sky_view_factor, solar_access  # noqa: E402

CLEARANCE_M = 2.134   # hang rule: panel bottom >= 7 ft
FULL_W, DIM_W = 1.364, 0.5


def slot_arrays(lit_row, w_row, null_mask):
    lit = [None if m else round(float(v) * 100, 1) for v, m in zip(lit_row, null_mask)]
    w = [None if m else round(float(v), 2) for v, m in zip(w_row, null_mask)]
    return lit, w


def field_sets(mesh, suns, null_mask, pos, nrm, svf=None):
    """Full per-panel field dicts for positions/normals (phase-2 conventions)."""
    res = solar_access(mesh, pos + nrm * 0.109, nrm, suns)
    if svf is None:
        svf = sky_view_factor(mesh, pos)
    w, wh_batt, rt = panel_power(res.lit, nrm, svf, suns)
    out = []
    for i in range(len(pos)):
        lit_i, w_i = slot_arrays(res.lit[i], w[i], null_mask)
        wh_dc = float(w[i].sum()) * (10.0 / 60.0)
        out.append({
            "svf": round(float(svf[i]), 3),
            "w": w_i, "lit": lit_i,
            "wh_day": round(wh_dc, 1),
            "wh_day_batt": round(float(wh_batt[i]), 1),
            "runtime_full_h": round(float(wh_batt[i]) / FULL_W, 1),
            "runtime_dim_h": round(float(wh_batt[i]) / DIM_W, 1),
        })
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--html", required=True)
    ap.add_argument("--offsets", default="0.25,0.5,1.0")
    ap.add_argument("--skip-rot", action="store_true",
                    help="keep the existing ROT blob (fast iteration)")
    args = ap.parse_args()

    html = open(args.html).read()

    def blob(name):
        m = re.search(r'const %s = (\{.*?\});?\n' % name, html)
        return json.loads(m.group(1)), m.group(1)

    DATA, _ = blob("DATA")
    ROT, _ = blob("ROT")
    HINGE, _ = blob("HINGE")

    suns = np.array([s if s else [0, 0, -1] for s in DATA["suns"]])
    null_mask = [s is None for s in DATA["suns"]]
    mesh = load_tree_mesh(os.path.join(HERE, "data", "tree_mesh.ply"))

    P = DATA["panels"]
    ids = list(P)
    pos = np.array([P[k]["pos"] for k in ids], float)
    nrm = np.array([P[k]["n"] for k in ids], float)

    print(f"base: {len(ids)} panels", flush=True)
    base = field_sets(mesh, suns, null_mask, pos, nrm)
    for k, f in zip(ids, base):
        P[k].update(f)

    lids = [k for k in ids if P[k]["kind"] == "lantern"]
    lpos = np.array([P[k]["pos"] for k in lids], float)
    lnrm = np.array([P[k]["n"] for k in lids], float)

    print(f"hinge: {len(HINGE)} lanterns", flush=True)
    hids = list(HINGE)
    hpos = np.array([P[k]["pos"] for k in hids], float)
    hnrm = np.array([HINGE[k]["n"] for k in hids], float)
    hsvf = np.array([P[k]["svf"] for k in hids], float)  # svf is aim-independent
    for k, f in zip(hids, field_sets(mesh, suns, null_mask, hpos, hnrm, svf=hsvf)):
        HINGE[k] = {"pitch_deg": HINGE[k]["pitch_deg"], "n": HINGE[k]["n"], **f}

    offsets = [float(o) for o in args.offsets.split(",") if o.strip()]
    variants = {}
    for off in offsets:
        print(f"hang offset -{off} m: {len(lids)} lanterns", flush=True)
        vpos = lpos - np.array([0, 0, off])
        fs = field_sets(mesh, suns, null_mask, vpos, lnrm)
        variants[f"{off:g}"] = {
            k: {**f, "clearance_ok": bool(P[k]["pos"][2] - off >= CLEARANCE_M)}
            for k, f in zip(lids, fs)}
    DATA["hang_variants"] = {"offsets_m": [f"{o:g}" for o in offsets],
                             "clearance_rule": DATA.get("hang_rule"),
                             "panels": variants}

    if not args.skip_rot:
        gids = ROT["ground_ids"]
        gpos = np.array([P[k]["pos"] for k in gids], float)
        tilts, azs = [0, 15, 30, 45, 60], list(range(0, 360, 45))
        cand = np.array([[np.sin(np.deg2rad(t)) * np.cos(np.deg2rad(a)),
                          np.sin(np.deg2rad(t)) * np.sin(np.deg2rad(a)),
                          np.cos(np.deg2rad(t))] for t in tilts for a in azs])
        labels = [(t, a) for t in tilts for a in azs]
        GPs = np.repeat(gpos, len(cand), axis=0)
        GNs = np.tile(cand, (len(gpos), 1))
        gsvf = np.repeat(sky_view_factor(mesh, gpos), len(cand))
        lsvf = sky_view_factor(mesh, lpos)
        ROT.update({"canopy_ids": lids, "ground_ids": gids, "angles": [],
                    "canopy_wh": [], "canopy_rt": [], "ground_wh": [],
                    "ground_best": []})
        for deg in range(0, 180, 5):
            th = np.deg2rad(deg)
            c, s = np.cos(th), np.sin(th)
            su = suns @ np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]]).T
            res = solar_access(mesh, lpos + lnrm * 0.109, lnrm, su)
            _, wh, rt = panel_power(res.lit, lnrm, lsvf, su)
            gres = solar_access(mesh, GPs + GNs * 0.109, GNs, su)
            _, gwh, _ = panel_power(gres.lit, GNs, gsvf, su)
            gwh = gwh.reshape(len(gpos), len(cand))
            best_j = np.argmax(gwh, axis=1)
            ROT["angles"].append(deg)
            ROT["canopy_wh"].append([round(float(v), 2) for v in wh])
            ROT["canopy_rt"].append([round(float(v), 2) for v in rt])
            ROT["ground_wh"].append([round(float(gwh[i, j]), 2)
                                     for i, j in enumerate(best_j)])
            ROT["ground_best"].append([list(labels[j]) for j in best_j])
            print(f"rot {deg:3d}: canopy median {np.median(wh):.2f} Wh", flush=True)

    DATA["model"] += (" · REBAKED 2026-07-27: true 5\"x3.5\" panel sampling "
                      "footprint (was 0.50x0.35 m stand-in); python sim, "
                      "~15% conservative vs SketchUp reference (occluder mesh)")

    for name, obj in (("DATA", DATA), ("ROT", ROT), ("HINGE", HINGE)):
        html = re.sub(r'const %s = \{.*?\};?\n' % name,
                      lambda m: "const %s = %s;\n"
                      % (name, json.dumps(obj, separators=(",", ":"))),
                      html, count=1)
    open(args.html, "w").write(html)
    med = np.median([P[k]["wh_day_batt"] for k in ids])
    print(f"wrote {args.html}  (fleet median {med:.1f} Wh/day batt)")


if __name__ == "__main__":
    main()
