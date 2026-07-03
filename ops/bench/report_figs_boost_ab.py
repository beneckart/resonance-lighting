#!/usr/bin/env python3
"""Generate the figures for docs/tests/BOOST_AB_BENCH_REPORT_2026-07-02.html.

Numbers are the vetted step summaries from the 2026-07-02 boost A/B bench campaign
(LOG entries of that date); raw samples for the stability figure are read from the
run JSONLs in ops/bench/data/boost_ab/. Re-run to regenerate the PNGs:
    python3 ops/bench/report_figs_boost_ab.py
"""

import json
import pathlib

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = pathlib.Path(__file__).resolve().parent
DATA = HERE / "data" / "boost_ab"
OUT = HERE.parent.parent / "docs" / "tests" / "figures" / "boost_ab"
OUT.mkdir(parents=True, exist_ok=True)

BARE = "#4878a8"
BOOST = "#c85450"
BARE2 = "#78a8d8"
BOOST2 = "#e8a090"

# ---------------------------------------------------------------- fig 1: HEX A/B
looks = ["white\n(1 px full)", "red\n(single)", "green\n(single)", "blue\n(single)",
         "7 px white\n(half bright)"]
bare_lux = [[215.6, 211.5], [33.3], [128.6], [60.4], [557.9]]
boost_lux = [[217.4, 216.7, 217.0], [33.5, 33.2], [129.5, 129.4], [63.6, 63.4],
             [596.0, 596.8]]
bare_w = [0.134, 0.065, 0.065, 0.062, 0.388]
boost_w = [0.216, 0.113, 0.113, 0.113, 0.63]

fig, ax = plt.subplots(figsize=(9, 4.6))
x = range(len(looks))
bm = [sum(v) / len(v) for v in bare_lux]
sm = [sum(v) / len(v) for v in boost_lux]
ax.bar([i - 0.19 for i in x], bm, 0.36, label="bare (no boost)", color=BARE)
ax.bar([i + 0.19 for i in x], sm, 0.36, label="boosted 4.2 V", color=BOOST)
for i in x:
    for v in bare_lux[i]:
        ax.plot(i - 0.19, v, "o", color="#123", ms=3.5)
    for v in boost_lux[i]:
        ax.plot(i + 0.19, v, "o", color="#123", ms=3.5)
    ax.annotate(f"{bare_w[i]:.2f} W", (i - 0.19, bm[i]), textcoords="offset points",
                xytext=(0, 12), ha="center", fontsize=8, color=BARE)
    ax.annotate(f"{boost_w[i]:.2f} W", (i + 0.19, sm[i]), textcoords="offset points",
                xytext=(0, 12), ha="center", fontsize=8, color=BOOST)
    gain = 100 * (sm[i] / bm[i] - 1)
    ax.annotate(f"{gain:+.0f}%", (i + 0.19, sm[i] / 2), ha="center", fontsize=9,
                color="white", fontweight="bold")
ax.set_xticks(list(x), looks)
ax.set_ylabel("light at the tube exit (lux)")
ax.set_title("HEX (37-px SK6812 RGB board): boost vs bare, per look\n"
             "dots = individual mounts (swap-to-swap repeatability); "
             "labels = electrical power into the LED branch")
ax.legend(loc="upper left")
ax.set_ylim(0, 700)
fig.tight_layout()
fig.savefig(OUT / "fig1_hex_ab.png", dpi=130)

# ------------------------------------------------- fig 2: RGBW brightness ladders
BRI = [32, 64, 128, 192, 255]
ladders = {
    # label: (color, style, W-only lux, RGB-white lux (None past a wall))
    "bare, from 3.3 V rail (r5)": (BARE, "-", [61, 120, 236, 353, 470],
                                   [165, 329, 655, 982, 1310]),
    "bare, from battery + fat wire (r8c)": (BARE2, "--", [59, 116, 226, 338, 448],
                                            [225, 444, 882, 1317, 1746]),
    "boosted, from 3.3 V rail (r6)": (BOOST, "-", [135, 265, 525, 786, 1044],
                                      [323, 641, None, None, None]),
    "boosted, from battery + fat wire (r9)": (BOOST2, "--", [129, 259, 513, 766, 1016],
                                              [396, 785, 1554, 2305, 3044]),
}
fig, (a1, a2) = plt.subplots(1, 2, figsize=(11, 4.8), sharex=True)
for label, (c, s, wl, rl) in ladders.items():
    a1.plot(BRI, wl, s, marker="o", color=c, label=label)
    rb = [b for b, v in zip(BRI, rl) if v is not None]
    rv = [v for v in rl if v is not None]
    a2.plot(rb, rv, s, marker="o", color=c, label=label)
    if len(rv) < len(BRI):
        a2.plot(rb[-1], rv[-1], "x", color=c, ms=14, mew=3)
        a2.annotate("supply collapsed\nat next step", (rb[-1], rv[-1]),
                    textcoords="offset points", xytext=(10, -6), fontsize=8, color=c)
a1.set_title("W channel only (clean white)")
a2.set_title("R+G+B all on (bright white, color-fringed)")
for a in (a1, a2):
    a.set_xlabel("commanded brightness (0-255)")
    a.set_xticks(BRI)
    a.grid(alpha=0.25)
a1.set_ylabel("light at the tube exit (lux)")
a1.legend(fontsize=8, loc="upper left")
fig.suptitle("RGBW 4 W point source: brightness ladders by power topology "
             "(all at the same mounting position)", y=1.0)
fig.tight_layout()
fig.savefig(OUT / "fig2_rgbw_ladders.png", dpi=130)

# --------------------------------------------------------- fig 3: full-power matrix
configs = ["bare\n3.3 V rail", "bare\nbattery+fat", "boosted\n3.3 V rail",
           "boosted\nbattery+thin*", "boosted\nbattery+fat"]
w_lux = [470, 448, 1044, 1060, 1016]
rgb_lux = [1310, 1746, None, None, 3044]
fig, ax = plt.subplots(figsize=(9, 4.6))
x = range(len(configs))
ax.bar([i - 0.19 for i in x], w_lux, 0.36, label="W-only white (clean)", color="#888")
for i in x:
    if rgb_lux[i] is None:
        ax.bar(i + 0.19, 130, 0.36, color="#ddd", hatch="//")
        ax.annotate("collapses at\n1/2 brightness", (i + 0.19, 150), ha="center",
                    fontsize=8, color="#666")
    else:
        ax.bar(i + 0.19, rgb_lux[i], 0.36, color="#c8a020")
for i in x:
    ax.annotate(str(w_lux[i]), (i - 0.19, w_lux[i]), ha="center",
                textcoords="offset points", xytext=(0, 3), fontsize=9)
    if rgb_lux[i]:
        ax.annotate(str(rgb_lux[i]), (i + 0.19, rgb_lux[i]), ha="center",
                    textcoords="offset points", xytext=(0, 3), fontsize=9)
ax.bar(0, 0, color="#c8a020", label="R+G+B white (fringed)")
ax.set_xticks(list(x), configs)
ax.set_ylabel("light at full brightness (lux)")
ax.set_title("RGBW full-brightness output by power topology\n"
             "*battery+thin harness sat at a different (more favorable) aim; "
             "W value shown aim-corrected")
ax.legend(loc="upper left")
fig.tight_layout()
fig.savefig(OUT / "fig3_matrix.png", dpi=130)

# ------------------------------------------------------------- fig 4: efficacy
labels = ["bare W-only\n(rail plane)", "bare R+G+B\n(rail plane)",
          "boosted W-only\nvia rail (rail plane)",
          "boosted W-only\nvia battery (battery plane)"]
vals = [470 / 0.208, 1310 / 0.836, 1044 / 0.731, 1060 / 0.723]
cols = [BARE, "#c8a020", BOOST, BOOST2]
fig, ax = plt.subplots(figsize=(8.5, 4.4))
ax.bar(labels, vals, 0.55, color=cols)
for i, v in enumerate(vals):
    ax.annotate(f"{v:.0f}", (i, v), ha="center", textcoords="offset points",
                xytext=(0, 3), fontsize=10)
ax.set_ylabel("lux per watt of measured input")
ax.set_title("Efficacy at full brightness (stable-current steps only)\n"
             "'rail plane' excludes the board's 3.3 V converter loss (~10% est.);\n"
             "'battery plane' is true battery draw -- see report section 5",
             fontsize=11)
fig.tight_layout()
fig.savefig(OUT / "fig4_efficacy.png", dpi=130)

# ----------------------------------------------- fig 5: measurement stability
src = DATA / "2026-07-02_113956_rgbw-ramp-bare-r5.jsonl"
tr = {128: [], 255: []}
for line in open(src):
    r = json.loads(line)
    if (r.get("type") == "ina" and r.get("ch") == "0x41"
            and r.get("look") == "rgbwhite" and r.get("bri") in tr):
        tr[r["bri"]].append(r["ma"])
fig, ax = plt.subplots(figsize=(9, 4))
ax.plot(tr[128], color="#b06020", label="half brightness (bri=128): current READING wanders")
ax.plot(tr[255], color="#207040", label="full brightness (bri=255): rock stable")
ax.set_xlabel("sample number within the 15 s step (10 Hz)")
ax.set_ylabel("LED-branch current reading (mA)")
ax.set_title("Why partial-brightness current readings were declared unreliable\n"
             "(bare RGBW run r5 -- light output was steady in BOTH steps shown;\n"
             "the swings are a measurement artifact, see report section 6)",
             fontsize=11)
ax.legend()
ax.grid(alpha=0.25)
fig.tight_layout()
fig.savefig(OUT / "fig5_stability.png", dpi=130)

print("wrote figures to", OUT)
