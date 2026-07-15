#!/usr/bin/env python3
"""One-off, deterministic patch of the 0.3.1 CAD export's downlight artifacts.

Usage: ./patch_cad_0.3.1.py   # writes data/fixtures-0.3.1-patched.json

The vendored export's 78 downlights decompose into 66 distinct ring hang
positions (outer 24 complete / middle 22 of 24 / inner 20 of 24), 6 fixtures
stacked at duplicate coordinates, and 6 strays clumped within ~10 cm of the
trunk axis at odd heights (procedural-export glitches; Ben spotted them
top-down 2026-07-12). Per Ben (2026-07-13), until a refined Blender export
lands: move the 6 trunk strays into the 6 missing ring slots. Slot positions
are inferred from each ring's angular gaps (nominal 24 slots / 15 deg step):
a gap of ~k*15 deg gets k-1 slots evenly spaced, at the ring's median radius,
z linearly interpolated between the gap's edge members. Stacked duplicates are
left as-is (assignment scoring already treats them as equivalence groups).
Strays are assigned to holes in (ring, angle) order by ascending fixture_id.

Uplights are NOT touched: their elevated placement (two rings of 12 at
mid-height) may be intentional in this design iteration -- uplighting the
upper trunk (Ben, 2026-07-13).
"""

import json
import os

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "data", "fixtures-0.3.1.json")
DST = os.path.join(HERE, "data", "fixtures-0.3.1-patched.json")

NOMINAL = 24
STEP = 360.0 / NOMINAL


def main():
    with open(SRC) as fh:
        doc = json.load(fh)
    fixtures = doc["fixtures"]
    down = [f for f in fixtures if f["role"] == "downlight"]
    P = np.array([f["position"] for f in down])
    r = np.linalg.norm(P[:, :2], axis=1)

    strays = sorted((f for f, rv in zip(down, r) if rv < 5.0),
                    key=lambda f: f["fixture_id"])
    assert len(strays) == 6, [f["fixture_id"] for f in strays]

    holes = []
    for ring_name, lo, hi in (("inner", 5.0, 35.0), ("middle", 35.0, 46.0),
                              ("outer", 46.0, 60.0)):
        members = [tuple(np.round(p, 3)) for p, rv in zip(P, r) if lo < rv < hi]
        distinct = np.array(sorted(set(members)))
        radius = float(np.median(np.linalg.norm(distinct[:, :2], axis=1)))
        ang = np.degrees(np.arctan2(distinct[:, 1], distinct[:, 0]))
        order = np.argsort(ang)
        ang_s, z_s = ang[order], distinct[order, 2]
        n_missing = NOMINAL - len(distinct)
        inserted = 0
        for k in range(len(ang_s)):
            a0, a1 = ang_s[k], ang_s[(k + 1) % len(ang_s)]
            z0, z1 = z_s[k], z_s[(k + 1) % len(ang_s)]
            gap = (a1 - a0) % 360.0
            k_slots = int(round(gap / STEP)) - 1
            if k_slots <= 0 or gap < 1.5 * STEP:
                continue
            for m in range(1, k_slots + 1):
                frac = m / (k_slots + 1)
                a = np.deg2rad(a0 + gap * frac)
                z = z0 + (z1 - z0) * frac
                holes.append((ring_name, float(np.rad2deg(a) % 360),
                              [radius * np.cos(a), radius * np.sin(a), z]))
                inserted += 1
        assert inserted == n_missing, (ring_name, inserted, n_missing)

    holes.sort(key=lambda h: ({"inner": 0, "middle": 1, "outer": 2}[h[0]], h[1]))
    assert len(holes) == len(strays), (len(holes), len(strays))

    moves = []
    for f, (ring_name, ang, pos) in zip(strays, holes):
        moves.append({"fixture_id": f["fixture_id"],
                      "from": [round(v, 3) for v in f["position"]],
                      "to": [round(v, 3) for v in pos],
                      "ring": ring_name})
        f["position"] = [round(v, 4) for v in pos]

    doc["meta"]["patched_2026-07-13"] = {
        "by": "ops/locate/patch_cad_0.3.1.py (Ben + Claude)",
        "why": ("procedural export left 6 ring holes (2 middle + 4 inner) and "
                "6 strays at the trunk base; strays moved into the holes "
                "pending a refined Blender export. Stacked duplicates left "
                "as-is. Uplight heights intentionally untouched."),
        "moves": moves,
    }
    with open(DST, "w") as fh:
        json.dump(doc, fh, indent=1)
    print(f"wrote {DST}")
    for m in moves:
        print(f"  {m['fixture_id']}: {m['from']} -> {m['to']} ({m['ring']})")


if __name__ == "__main__":
    main()
