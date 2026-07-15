#!/usr/bin/env python3
"""Parameter sweeps -> breakage curves for the auto-localization feasibility study.

Usage:
  ./locate_sweep.py --suite core --trials 6 --workers 4
  ./locate_sweep.py --suite core,beacons,anchors --trials 6 --workers 4 --plots

Suites (all use the vendored CAD, 152 devices, censoring-aware aggregation):
  core     sigma_link 0..12 dB at 3 beacons -- the primary breakage axis
  beacons  0..4 surveyed devices at sigma_link 4 and 6 dB -- gauge anchoring value
  anchors  ToF anchor coverage (downlight max range) at sigma_link 4 dB
  mismatch model-mismatch stress arms: n_true vs n_assumed, frozen panel, two-ray
  grid     sigma_link x sigma_dev heatmap

Rows are appended to data/sim/<date>-sweep-<suite>.jsonl (one JSON object per
run: params + seed + git head + metrics). --plots renders the curves next to
the data. Deterministic per (suite, config, trial): seed = base + trial.
"""

import os

# cap BLAS threading BEFORE numpy loads: N workers x unbounded OpenBLAS pools
# oversubscribe the box into thrash (measured: load 60 on 16 cores, zero
# completed runs). 2 threads/worker x default 4 workers stays comfortably under.
for _v in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS",
           "NUMEXPR_NUM_THREADS"):
    os.environ.setdefault(_v, "2")

import argparse
import itertools
import json
import multiprocessing as mp
import subprocess
import sys
from datetime import date

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from locate.io_cad import load_cad                                    # noqa: E402
from locate.metrics import assignment_accuracy, position_error_stats, verdict_triple  # noqa: E402
from locate.model import PathLossParams                               # noqa: E402
from locate.pipeline import SolveConfig, solve                        # noqa: E402
from locate.refine import RefineConfig                                # noqa: E402
from locate.rssi import aggregate_directed                            # noqa: E402
from sim.rf import RfParams, simulate_rssi                            # noqa: E402
from sim.scene import build_scene                                     # noqa: E402
from sim.tof import TofParams, make_anchors                           # noqa: E402

CAD_PATH = os.path.join(HERE, "data", "fixtures-0.3.1-patched.json")

# realistic-band annotation for the report figures: open-playa short-window
# estimate vs the measured indoor placement-shift band
# (docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md)
PLAYA_BAND_DB = (2.0, 6.0)
INDOOR_BAND_DB = (8.0, 17.0)


def suite_configs(suite):
    if suite == "core":
        return [{"sigma_link": s, "beacons": 3} for s in (0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0)]
    if suite == "beacons":
        return [{"sigma_link": s, "beacons": b}
                for s in (4.0, 6.0) for b in (0, 1, 2, 3, 4)]
    if suite == "anchors":
        return [{"sigma_link": 4.0, "beacons": 3, "tof_max_range": r}
                for r in (0.0, 2.5, 4.5, 6.0)]
    if suite == "mismatch":
        return [
            {"sigma_link": 4.0, "beacons": 3, "n_true": 2.2},
            {"sigma_link": 4.0, "beacons": 3, "n_true": 3.2},
            {"sigma_link": 4.0, "beacons": 3, "panel_mode": "frozen"},
            {"sigma_link": 4.0, "beacons": 3, "two_ray": True},
            {"sigma_link": 4.0, "beacons": 3, "p_fade": 0.15},
        ]
    if suite == "grid":
        return [{"sigma_link": s, "sigma_dev": d, "beacons": 3}
                for s in (2.0, 4.0, 6.0, 8.0) for d in (0.0, 2.0, 4.0, 6.0)]
    raise ValueError(f"unknown suite {suite}")


def run_one(task):
    suite, config, trial, git_head = task
    seed = 100 * (trial + 1)
    cad = load_cad(CAD_PATH, seed=1)
    scene = build_scene(cad, seed=seed)
    rf = RfParams(
        sigma_link_db=config.get("sigma_link", 4.0),
        sigma_dev_db=config.get("sigma_dev", 3.0),
        p_fade=config.get("p_fade", 0.05),
        panel_mode=config.get("panel_mode", "spin"),
        n=config.get("n_true", 2.7),
        two_ray=config.get("two_ray", False),
    )
    directed = simulate_rssi(scene.truth_pos, rf, seed=seed + 1)
    tof = TofParams(tof_max_range_m=config.get("tof_max_range", 6.0))
    anchors = make_anchors(scene, tof, seed=seed + 2)
    links = aggregate_directed(directed, expected=rf.k_packets)

    known = {}
    b = config.get("beacons", 0)
    if b:
        rng = np.random.default_rng(seed + 3)
        picks = rng.choice(len(scene.devices), size=b, replace=False)
        known = {int(k): scene.truth_fixture[k] for k in picks}

    cfg = SolveConfig(pl_prior=PathLossParams(),
                      refine=RefineConfig(sigma_p0_db=10.0),
                      known_assignments=known)
    roles = [d.role for d in scene.devices]
    try:
        res = solve(scene.devices, links, anchors, cad, cfg)
    except Exception as exc:                                   # record, don't kill the sweep
        return {"suite": suite, **config, "trial": trial, "seed": seed,
                "git": git_head, "error": repr(exc)}
    acc = assignment_accuracy(res.assignment, scene.truth_fixture, roles, cad)
    err = position_error_stats(res.pos_m, scene.truth_pos, roles)
    flags = np.array([p.flagged for p in res.per_device])
    v = verdict_triple(res.assignment, scene.truth_fixture, flags, cad)
    return {
        "suite": suite, **config, "trial": trial, "seed": seed, "git": git_head,
        "acc": acc["overall"], "acc_per_role": acc["per_role"],
        "auto_correct": v["auto_correct"], "flagged": v["flagged"],
        "silent_wrong": v["silent_wrong"],
        "err_median_m": err["overall"]["median_m"],
        "err_p95_m": err["overall"]["p95_m"],
        "arm": res.diagnostics["align"].get("arm"),
        "n_anchors": len(anchors),
        "n_censored": sum(1 for l in links if l.censored),
    }


def plot_suite(suite, rows, out_png):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    rows = [r for r in rows if "error" not in r]

    def curve(xkey, filt=None):
        xs = sorted({r[xkey] for r in rows if not filt or filt(r)})
        out = {}
        for m in ("acc", "auto_correct", "silent_wrong", "flagged"):
            med, lo, hi = [], [], []
            for x in xs:
                v = [r[m] for r in rows if r[xkey] == x and (not filt or filt(r))]
                med.append(np.median(v)); lo.append(np.min(v)); hi.append(np.max(v))
            out[m] = (np.array(med), np.array(lo), np.array(hi))
        return np.array(xs), out

    fig, ax = plt.subplots(figsize=(8.5, 5))
    series = [("acc", "assignment accuracy", "#2a78d6"),
              ("auto_correct", "auto-correct", "#1baf7a"),
              ("silent_wrong", "silent-wrong", "#e34948")]
    if suite in ("core", "anchors"):
        xkey = "sigma_link" if suite == "core" else "tof_max_range"
        xs, out = curve(xkey)
        for m, lbl, col in series:
            med, lo, hi = out[m]
            ax.plot(xs, med, color=col, lw=2, marker="o", ms=4, label=lbl)
            ax.fill_between(xs, lo, hi, color=col, alpha=0.12, lw=0)
        if suite == "core":
            ax.axvspan(*PLAYA_BAND_DB, color="#888888", alpha=0.12, lw=0)
            ax.text(np.mean(PLAYA_BAND_DB), 1.02, "playa est.", ha="center",
                    fontsize=8, color="#555555")
            if max(xs) >= INDOOR_BAND_DB[0]:
                ax.axvspan(INDOOR_BAND_DB[0], min(INDOOR_BAND_DB[1], max(xs)),
                           color="#888888", alpha=0.07, lw=0)
                ax.text(INDOOR_BAND_DB[0] + 1, 1.02, "indoor band", ha="center",
                        fontsize=8, color="#777777")
            ax.set_xlabel("per-link multipath bias sigma (dB)")
        else:
            ax.set_xlabel("downlight ToF max range (m); 0 = no downlight anchors")
    elif suite == "beacons":
        for sl, ls in ((4.0, "-"), (6.0, "--")):
            xs, out = curve("beacons", filt=lambda r, s=sl: r["sigma_link"] == s)
            for m, lbl, col in series:
                med, lo, hi = out[m]
                ax.plot(xs, med, color=col, lw=2, ls=ls, marker="o", ms=4,
                        label=f"{lbl} ({sl:g} dB)" if ls == "-" else None)
                ax.fill_between(xs, lo, hi, color=col, alpha=0.10, lw=0)
        ax.set_xlabel("surveyed beacon devices (solid 4 dB, dashed 6 dB)")
        ax.set_xticks([0, 1, 2, 3, 4])
    elif suite == "grid":
        fig2, ax2 = plt.subplots(figsize=(6.5, 5))
        sls = sorted({r["sigma_link"] for r in rows})
        sds = sorted({r["sigma_dev"] for r in rows})
        M = np.full((len(sds), len(sls)), np.nan)
        for i, sd in enumerate(sds):
            for j, sl in enumerate(sls):
                v = [r["acc"] for r in rows
                     if r["sigma_link"] == sl and r["sigma_dev"] == sd]
                if v:
                    M[i, j] = np.median(v)
        im = ax2.imshow(M, origin="lower", cmap="Blues", vmin=0, vmax=1, aspect="auto")
        ax2.set_xticks(range(len(sls)), [f"{v:g}" for v in sls])
        ax2.set_yticks(range(len(sds)), [f"{v:g}" for v in sds])
        ax2.set_xlabel("sigma_link (dB)"); ax2.set_ylabel("sigma_dev (dB)")
        for i in range(len(sds)):
            for j in range(len(sls)):
                if np.isfinite(M[i, j]):
                    ax2.text(j, i, f"{M[i, j]:.2f}", ha="center", va="center",
                             fontsize=9,
                             color="#1b1b1a" if M[i, j] < 0.6 else "#ffffff")
        ax2.set_title("median assignment accuracy")
        fig2.colorbar(im, ax=ax2, shrink=0.85)
        fig2.tight_layout(); fig2.savefig(out_png, dpi=130); plt.close(fig2)
        plt.close(fig)
        return
    else:   # mismatch: grouped dots
        labels, vals = [], []
        for cfg_key in sorted({json.dumps({k: r[k] for k in r
                                           if k in ("n_true", "panel_mode",
                                                    "two_ray", "p_fade")},
                                          sort_keys=True) for r in rows}):
            sel = [r for r in rows if json.dumps(
                {k: r[k] for k in r if k in ("n_true", "panel_mode", "two_ray",
                                             "p_fade")}, sort_keys=True) == cfg_key]
            labels.append(cfg_key.replace('"', "")); vals.append([r["acc"] for r in sel])
        ax.boxplot(vals, tick_labels=labels)
        ax.tick_params(axis="x", labelrotation=20, labelsize=7)
        ax.set_ylabel("assignment accuracy")
        ax.set_title("model-mismatch stress arms (sigma_link 4 dB, 3 beacons)")
        ax.grid(alpha=0.25, lw=0.5, axis="y")
        fig.tight_layout(); fig.savefig(out_png, dpi=130); plt.close(fig)
        return

    ax.set_ylim(-0.02, 1.08)
    ax.set_ylabel("fraction of 152 devices")
    ax.set_title(f"suite {suite}: breakage curves (band = min..max over trials)")
    ax.legend(fontsize=8, loc="center left")
    ax.grid(alpha=0.25, lw=0.5)
    fig.tight_layout(); fig.savefig(out_png, dpi=130); plt.close(fig)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--suite", default="core", help="comma-separated suite names")
    ap.add_argument("--trials", type=int, default=6)
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--plots", action="store_true")
    ap.add_argument("--out-dir", default=os.path.join(HERE, "data", "sim"))
    args = ap.parse_args()

    git_head = subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                              capture_output=True, text=True, cwd=HERE).stdout.strip()
    os.makedirs(args.out_dir, exist_ok=True)

    for suite in args.suite.split(","):
        suite = suite.strip()
        tasks = [(suite, cfg, t, git_head)
                 for cfg in suite_configs(suite) for t in range(args.trials)]
        print(f"suite {suite}: {len(tasks)} runs on {args.workers} workers")
        path = os.path.join(args.out_dir, f"{date.today().isoformat()}-sweep-{suite}.jsonl")
        rows = []
        with mp.Pool(args.workers) as pool, open(path, "a") as fh:
            for row in pool.imap_unordered(run_one, tasks):
                rows.append(row)
                fh.write(json.dumps(row) + "\n")
                fh.flush()
                tag = {k: v for k, v in row.items()
                       if k in ("sigma_link", "sigma_dev", "beacons",
                                "tof_max_range", "trial")}
                print(f"  {tag} -> acc {row.get('acc', 'ERR'):.3f}"
                      if "acc" in row else f"  {tag} -> {row.get('error')}")
        if args.plots:
            png = path.replace(".jsonl", ".png")
            plot_suite(suite, rows, png)
            print(f"  wrote {png}")
        print(f"  wrote {path} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
