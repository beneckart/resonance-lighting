#!/usr/bin/env python3
"""One end-to-end auto-localization run: simulate (or ingest real data), solve,
report metrics, optionally emit figures + interactive HTML viewer + JSONL.

Usage:
  ./locate_run.py --sim --seed 7 --sigma-link 4 --beacons 3 --plots --html
  ./locate_run.py --pairwise pw.jsonl --roster roster.json [--beacons-map S001=F010,...]

Sim knobs mirror sim/rf.py (RfParams) and sim/tof.py (TofParams); solver knobs
mirror locate/pipeline.py (SolveConfig). Outputs land in --out-dir with a
run-id prefix (default: <date>-sim-sl<sigma>-s<seed>).
"""

import argparse
import json
import os
import sys
from datetime import date

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from locate.io_cad import load_cad                                   # noqa: E402
from locate.io_jsonl import (read_pairwise, read_roster, rows_from_directed,
                             rows_to_links, write_pairwise, write_roster)  # noqa: E402
from locate.metrics import assignment_accuracy, position_error_stats, verdict_triple  # noqa: E402
from locate.model import PathLossParams                              # noqa: E402
from locate.pipeline import SolveConfig, solve                       # noqa: E402
from locate.refine import RefineConfig                               # noqa: E402
from locate.rssi import aggregate_directed, distance_from_rssi       # noqa: E402
from sim.rf import RfParams, simulate_rssi                           # noqa: E402
from sim.scene import build_scene                                    # noqa: E402
from sim.tof import TofParams, make_anchors                          # noqa: E402
from viewer import write_viewer                                      # noqa: E402

# fixed categorical role colors (validated palette; same order everywhere)
ROLE_COLORS = {"downlight": "#2a78d6", "perimeter": "#1baf7a",
               "uplight": "#eda100", "chandelier": "#4a3aa7"}


def build_parser():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_argument_group("input")
    src.add_argument("--sim", action="store_true", help="simulate the fleet")
    src.add_argument("--pairwise", help="real pairwise-RSSI JSONL (contract rows)")
    src.add_argument("--roster", help="roster JSON for --pairwise")
    ap.add_argument("--cad",
                    default=os.path.join(HERE, "data", "fixtures-0.3.1-patched.json"),
                    help="fixtures.json; default = 0.3.1 with the 6 trunk strays "
                         "moved into the 6 ring holes (patch_cad_0.3.1.py)")
    ap.add_argument("--cad-scale", default="auto:downlights",
                    help='m/unit float, "auto:downlights" or "auto:tree=<H>"')
    ap.add_argument("--perimeter-n", type=int, default=40)
    ap.add_argument("--perimeter-radius", type=float, default=None)
    ap.add_argument("--seed", type=int, default=7)

    s = ap.add_argument_group("sim scene / RF / ToF")
    s.add_argument("--n-downlights", type=int, default=72)
    s.add_argument("--placement-jitter", type=float, default=0.15, help="m")
    s.add_argument("--sigma-link", type=float, default=4.0, help="per-link static bias dB")
    s.add_argument("--sigma-dev", type=float, default=3.0, help="per-device TX/RX offset dB")
    s.add_argument("--sigma-pkt", type=float, default=2.0, help="per-packet fading dB")
    s.add_argument("--sigma-asym", type=float, default=1.5)
    s.add_argument("--p-fade", type=float, default=0.05)
    s.add_argument("--panel-mode", choices=["spin", "frozen", "off"], default="spin")
    s.add_argument("--panel-depth", type=float, default=20.0)
    s.add_argument("--k-packets", type=int, default=50)
    s.add_argument("--rf-floor", type=float, default=-90.0, help="dBm (-110 = LR mode)")
    s.add_argument("--trunk-off", action="store_true")
    s.add_argument("--trunk-loss", type=float, default=10.0)
    s.add_argument("--two-ray", action="store_true", help="ground-reflection stressor")
    s.add_argument("--n-true", type=float, default=2.7, help="true path-loss exponent")
    s.add_argument("--tof-max-range", type=float, default=6.0)
    s.add_argument("--mount-downtilt", type=float, default=15.0)

    v = ap.add_argument_group("solver")
    v.add_argument("--p0-prior", type=float, default=-40.0)
    v.add_argument("--sigma-p0", type=float, default=10.0,
                   help="P0 prior width dB (2-3 = calibrated pair measured)")
    v.add_argument("--n-assumed", type=float, default=2.7)
    v.add_argument("--fit-n", action="store_true")
    v.add_argument("--init", choices=["auto", "plain", "smacof"], default="auto")
    v.add_argument("--beacons", type=int, default=0,
                   help="sim: N random surveyed devices pin the gauge")
    v.add_argument("--beacons-map", default="",
                   help="real: dev=fixture,dev=fixture pins")
    v.add_argument("--bootstrap", type=int, default=0, help="B re-solves for agreement")

    o = ap.add_argument_group("output")
    o.add_argument("--out-dir", default=os.path.join(HERE, "data", "sim"))
    o.add_argument("--run-id", default=None)
    o.add_argument("--plots", action="store_true")
    o.add_argument("--html", action="store_true")
    o.add_argument("--emit-jsonl", action="store_true",
                   help="sim: also write the pairwise/roster contract files")
    return ap


def run_sim(args):
    cad = load_cad(args.cad, scale=args.cad_scale, perimeter_n=args.perimeter_n,
                   perimeter_radius_m=args.perimeter_radius, seed=args.seed)
    scene = build_scene(cad, n_downlights=args.n_downlights,
                        placement_jitter_m=args.placement_jitter, seed=args.seed)
    rf = RfParams(
        p0_dbm=-40.0, n=args.n_true, sigma_pkt_db=args.sigma_pkt,
        sigma_link_db=args.sigma_link, sigma_asym_db=args.sigma_asym,
        p_fade=args.p_fade, sigma_dev_db=args.sigma_dev,
        panel_depth_db=args.panel_depth, panel_mode=args.panel_mode,
        floor_dbm=args.rf_floor, k_packets=args.k_packets,
        trunk_on=not args.trunk_off, trunk_loss_db=args.trunk_loss,
        two_ray=args.two_ray,
    )
    directed = simulate_rssi(scene.truth_pos, rf, seed=args.seed + 1)
    tof = TofParams(tof_max_range_m=args.tof_max_range,
                    mount_downtilt_deg=args.mount_downtilt)
    anchors = make_anchors(scene, tof, seed=args.seed + 2)
    links = aggregate_directed(directed, expected=rf.k_packets)
    known = {}
    if args.beacons:
        rng = np.random.default_rng(args.seed + 3)
        picks = rng.choice(len(scene.devices), size=args.beacons, replace=False)
        known = {int(k): scene.truth_fixture[k] for k in picks}
    return cad, scene, links, anchors, known, directed, rf


def run_real(args):
    cad = load_cad(args.cad, scale=args.cad_scale, perimeter_n=args.perimeter_n,
                   perimeter_radius_m=args.perimeter_radius, seed=args.seed)
    devices, anchors = read_roster(args.roster)
    id_to_idx = {d.dev_id: k for k, d in enumerate(devices)}
    links = rows_to_links(read_pairwise(args.pairwise), id_to_idx)
    known = {}
    for pair in filter(None, args.beacons_map.split(",")):
        dev, fid = pair.split("=")
        known[id_to_idx[dev.strip()]] = fid.strip()
    return cad, devices, links, anchors, known


def make_figures(prefix, res, links, scene, roles):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    truth = scene.truth_pos if scene else None

    # fig 1: two orthographic panels (top view x/y, elevation x/z); the
    # interactive HTML viewer supplies the free 3D view
    fig, axes = plt.subplots(1, 2, figsize=(12, 5.5))
    for ax, (ix, iy, xl, yl, ttl) in zip(axes, [
            (0, 1, "x (m)", "y (m)", "top view"),
            (0, 2, "x (m)", "z (m)", "elevation")]):
        if truth is not None:
            for k in range(len(truth)):
                ax.plot([res.pos_m[k, ix], truth[k, ix]],
                        [res.pos_m[k, iy], truth[k, iy]],
                        color="#cc4444", lw=0.6, alpha=0.5, zorder=1)
            ax.scatter(truth[:, ix], truth[:, iy], s=4, color="#999999",
                       label="truth", zorder=2)
        for role, col in ROLE_COLORS.items():
            m = np.array([r == role for r in roles])
            if m.any():
                ax.scatter(res.pos_m[m, ix], res.pos_m[m, iy], s=14, color=col,
                           label=role, zorder=3)
        ax.set_xlabel(xl); ax.set_ylabel(yl); ax.set_title(ttl)
        ax.set_aspect("equal"); ax.grid(alpha=0.25, lw=0.5)
    axes[0].legend(loc="upper left", fontsize=8)
    fig.suptitle("estimated positions vs truth (red segments = error)")
    fig.tight_layout(); fig.savefig(prefix + "-positions.png", dpi=130); plt.close(fig)

    # fig 2: registration cost vs theta (gauge landscape)
    reg = res.diagnostics["register"]
    grid = np.asarray(reg["theta_deg_grid"])
    if len(grid):
        fig, ax = plt.subplots(figsize=(8, 3.4))
        best_x, best_c = None, np.inf
        for mkey, lbl, col in (("cost_curve_m1", "mirror +1", "#2a78d6"),
                               ("cost_curve_m-1", "mirror -1", "#eb6834")):
            c = np.asarray(reg[mkey])
            if len(c):
                x = grid % 360 if len(grid) > 50 else grid
                ax.plot(x, c, lw=1.2, color=col, label=lbl)
                if c.min() < best_c:
                    best_c, best_x = c.min(), x[int(np.argmin(c))]
        if best_x is not None:
            ax.axvline(best_x, color="#555555", lw=0.8, ls="--")
        ax.set_xlabel("theta (deg)"); ax.set_ylabel("assignment cost (m^2)")
        amb = reg.get("ambiguity_ratio")
        ax.set_title(f"registration gauge landscape "
                     f"(ambiguity ratio {amb:.2f})" if amb and np.isfinite(amb)
                     else "registration gauge landscape (beacon-pinned)")
        ax.legend(fontsize=8); ax.grid(alpha=0.25, lw=0.5)
        fig.tight_layout(); fig.savefig(prefix + "-theta.png", dpi=130); plt.close(fig)

    # fig 3: RSSI vs log true distance + fitted model (sim only)
    if truth is not None:
        d, r, cen = [], [], []
        for l in links:
            d.append(np.linalg.norm(truth[l.i] - truth[l.j]))
            r.append(l.rssi_dbm); cen.append(l.censored)
        d, r, cen = np.array(d), np.array(r), np.array(cen)
        fig, ax = plt.subplots(figsize=(8, 4.2))
        ax.scatter(d[~cen], r[~cen], s=3, alpha=0.25, color="#2a78d6", label="links")
        if cen.any():
            ax.scatter(d[cen], r[cen], s=3, alpha=0.4, color="#e34948",
                       label="censored (floor)")
        dd = np.geomspace(max(d.min(), 0.05), d.max(), 100)
        pl = res.pathloss
        ax.plot(dd, pl.p0_dbm - 10 * pl.n * np.log10(dd), color="#1b1b1a", lw=1.4,
                label=f"fit P0={pl.p0_dbm:.1f} n={pl.n:.2f}")
        ax.set_xscale("log")
        ax.set_xlabel("true distance (m)"); ax.set_ylabel("aggregated RSSI (dBm)")
        ax.set_title("RSSI vs distance, fitted path-loss model")
        ax.legend(fontsize=8); ax.grid(alpha=0.25, lw=0.5)
        fig.tight_layout(); fig.savefig(prefix + "-rssi.png", dpi=130); plt.close(fig)

    # fig 4: per-device confidence: LAP margin vs position error (or vs sigma)
    margins = np.array([p.margin_cost for p in res.per_device])
    flagged = np.array([p.flagged for p in res.per_device])
    fig, ax = plt.subplots(figsize=(8, 4.2))
    xs = (np.linalg.norm(res.pos_m - truth, axis=1) if truth is not None
          else np.array([p.sigma_pos_m for p in res.per_device]))
    finite = np.isfinite(margins)
    ax.scatter(xs[finite & ~flagged], margins[finite & ~flagged], s=12,
               color="#2a78d6", label="unflagged")
    ax.scatter(xs[finite & flagged], margins[finite & flagged], s=16,
               facecolors="none", edgecolors="#e34948", label="flagged")
    ax.set_yscale("symlog", linthresh=0.01)
    ax.set_xlabel("position error (m)" if truth is not None else "sigma_pos (m)")
    ax.set_ylabel("assignment margin (m^2)")
    ax.set_title("per-device confidence: margin vs error")
    ax.legend(fontsize=8); ax.grid(alpha=0.25, lw=0.5)
    fig.tight_layout(); fig.savefig(prefix + "-confidence.png", dpi=130); plt.close(fig)


def main():
    args = build_parser().parse_args()
    if not args.sim and not (args.pairwise and args.roster):
        sys.exit("need --sim or (--pairwise + --roster)")

    scene = None
    if args.sim:
        cad, scene, links, anchors, known, directed, rf = run_sim(args)
        devices = scene.devices
    else:
        cad, devices, links, anchors, known = run_real(args)

    cfg = SolveConfig(
        pl_prior=PathLossParams(p0_dbm=args.p0_prior, n=args.n_assumed),
        refine=RefineConfig(sigma_p0_db=args.sigma_p0, fit_n=args.fit_n),
        init_strategy=args.init,
        known_assignments=known,
        bootstrap_B=args.bootstrap,
    )
    res = solve(devices, links, anchors, cad, cfg)
    roles = [d.role for d in devices]

    report = {
        "args": {k: v for k, v in vars(args).items()},
        "n_devices": len(devices),
        "n_links": len(links),
        "n_censored": sum(1 for l in links if l.censored),
        "n_anchors": len(anchors),
        "pathloss_fit": {"p0_dbm": res.pathloss.p0_dbm, "n": res.pathloss.n},
        "register": {k: v for k, v in res.diagnostics["register"].items()
                     if not hasattr(v, "__len__")},
        "arm": res.diagnostics["align"].get("arm"),
        "flagged": int(sum(1 for p in res.per_device if p.flagged)),
    }
    if scene is not None:
        acc = assignment_accuracy(res.assignment, scene.truth_fixture, roles, cad)
        flags = np.array([p.flagged for p in res.per_device])
        report["assignment_accuracy"] = acc
        report["position_error"] = position_error_stats(res.pos_m, scene.truth_pos, roles)
        report["verdict"] = verdict_triple(res.assignment, scene.truth_fixture, flags, cad)

    os.makedirs(args.out_dir, exist_ok=True)
    run_id = args.run_id or (f"{date.today().isoformat()}-sim-sl{args.sigma_link:g}"
                             f"-s{args.seed}" if args.sim
                             else f"{date.today().isoformat()}-real")
    prefix = os.path.join(args.out_dir, run_id)
    report["per_device"] = [
        {"dev_id": p.dev_id, "role": p.role, "fixture_id": p.fixture_id,
         "degree": p.degree, "sigma_pos_m": round(p.sigma_pos_m, 4),
         "margin": (None if not np.isfinite(p.margin_cost) else round(p.margin_cost, 4)),
         "flagged": p.flagged,
         "pos_m": [round(float(v), 3) for v in res.pos_m[k]]}
        for k, p in enumerate(res.per_device)
    ]
    with open(prefix + "-report.json", "w") as fh:
        json.dump(report, fh, indent=1, default=str)

    if args.emit_jsonl and args.sim:
        dev_ids = [d.dev_id for d in devices]
        write_pairwise(prefix + "-pairwise.jsonl",
                       rows_from_directed(directed, dev_ids, expected=rf.k_packets))
        write_roster(prefix + "-roster.json", devices, anchors)

    if args.plots:
        make_figures(prefix, res, links, scene, roles)

    if args.html:
        fid_pos = {f.fixture_id: f.pos_m for f in cad.fixtures}
        payload = {
            "title": run_id,
            "meta_text": json.dumps({k: report[k] for k in
                                     ("n_devices", "n_links", "flagged")}, indent=0),
            "devices": [], "cad_all": [
                {"id": f.fixture_id, "role": f.role,
                 "pos": [round(float(v), 3) for v in f.pos_m]}
                for f in cad.fixtures],
        }
        for k, p in enumerate(res.per_device):
            truth_k = (scene.truth_pos[k] if scene is not None else None)
            correct = None
            if scene is not None:
                from locate.metrics import same_slot
                correct = same_slot(p.fixture_id, scene.truth_fixture[k], cad)
            payload["devices"].append({
                "id": p.dev_id, "role": p.role,
                "est": [round(float(v), 3) for v in res.pos_m[k]],
                "truth": ([round(float(v), 3) for v in truth_k]
                          if truth_k is not None else None),
                "cad": ([round(float(v), 3) for v in fid_pos[p.fixture_id]]
                        if p.fixture_id else None),
                "err": (round(float(np.linalg.norm(res.pos_m[k] - truth_k)), 3)
                        if truth_k is not None else None),
                "flagged": bool(p.flagged), "correct": correct,
            })
        write_viewer(prefix + "-viewer.html", payload)

    # console summary
    print(f"run {run_id}: {len(devices)} devices, {len(links)} links "
          f"({report['n_censored']} censored), {report['n_anchors']} anchors, "
          f"arm={report['arm']}, flagged={report['flagged']}")
    if scene is not None:
        acc = report["assignment_accuracy"]
        v = report["verdict"]
        e = report["position_error"]["overall"]
        print(f"  assignment: overall {acc['overall']:.3f}  "
              + "  ".join(f"{r} {a:.2f}" for r, a in acc["per_role"].items()))
        print(f"  verdict: auto-correct {v['auto_correct']:.2%}  flagged {v['flagged']:.2%}"
              f"  SILENT-WRONG {v['silent_wrong']:.2%}")
        print(f"  position: median {e['median_m']:.2f} m  p95 {e['p95_m']:.2f} m")
    print(f"  outputs: {prefix}-*")


if __name__ == "__main__":
    main()
