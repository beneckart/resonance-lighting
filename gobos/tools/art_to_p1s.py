#!/usr/bin/env /usr/bin/python3
"""
art_to_p1s.py — convert clean black/white gobo ART (ChatGPT etc.) into a
printable P1S plate, preserving the smooth curves of the source.

The pipeline that killed leaf-open's curves was: low-res trace -> aggressive
polygon simplify. This one:
  1. traces at 40 px/mm (2000 px across the disc)
  2. detects the drawn ring, replaces it with the true 2 mm production ring
  3. thickens ONLY sub-1.4 mm strokes (stems/petioles) with a round kernel,
     so leaf shapes keep their outline
  4. welds any floating piece to its nearest neighbour with a curved-capped tie
  5. simplifies at 0.05 mm — invisible at print scale, curves survive
  6. emits PNG preview + verified STL + SVG

usage: art_to_p1s.py --png art.png --name chatgpt-01 [--diam 50]
"""
import argparse, math, os
import numpy as np, cv2

P = argparse.ArgumentParser()
P.add_argument("--png", required=True)
P.add_argument("--name", default=None)
P.add_argument("--out", default="out")
P.add_argument("--diam", type=float, default=50.0)
P.add_argument("--ring", type=float, default=2.0)
P.add_argument("--thick", type=float, default=3.0)
P.add_argument("--minfeat", type=float, default=1.4)
P.add_argument("--no-stl", action="store_true", help="emit SVG+PNG only (Steve does the 3D in Fusion)")
A = P.parse_args()
name = A.name or os.path.splitext(os.path.basename(A.png))[0]
os.makedirs(A.out, exist_ok=True)

# ---------- 1. load + binarize ----------
img = cv2.imread(A.png, cv2.IMREAD_UNCHANGED)
if img is None:
    raise SystemExit("cannot read " + A.png)
if img.ndim == 3 and img.shape[2] == 4:
    a = img[:, :, 3]
    g = cv2.cvtColor(img[:, :, :3], cv2.COLOR_BGR2GRAY)
    g = np.where(a > 127, g, 255).astype(np.uint8)
else:
    g = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY) if img.ndim == 3 else img
_, bw = cv2.threshold(g, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
art0 = (bw > 0).astype(np.uint8)

# ---------- 2. find the drawn ring -> px scale ----------
cnts, _ = cv2.findContours(art0, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
big = max(cnts, key=cv2.contourArea)
(cx, cy), r_px = cv2.minEnclosingCircle(big)
# work at 40 px/mm on a clean canvas
MM = 40.0
RES = int(A.diam * MM)
scale = (RES / 2.0 - 2) / r_px
Mtx = np.float32([[scale, 0, RES/2 - cx*scale], [0, scale, RES/2 - cy*scale]])
art = cv2.warpAffine(art0*255, Mtx, (RES, RES), flags=cv2.INTER_CUBIC)
art = cv2.GaussianBlur(art, (0, 0), 2.2)          # sub-pixel edges
art = (art > 110).astype(np.uint8)

R_out = A.diam / 2.0
R_in = R_out - A.ring
yy, xx = np.mgrid[0:RES, 0:RES]
rr = np.sqrt((xx - RES/2)**2 + (yy - RES/2)**2) / MM
ring = ((rr <= R_out) & (rr >= R_in)).astype(np.uint8)
interior = (rr < R_in).astype(np.uint8)

# strip the drawn ring: keep only art clear of the rim band, then add ours
inner_art = (art & (rr < R_in + 0.6).astype(np.uint8))

# heal the art BEFORE anything else:
#  - closing fills edge nicks/notches up to ~0.8 mm (anti-aliasing scars,
#    half-drawn midrib lines that Otsu turns into cuts)
#  - hole-fill removes small white slivers trapped inside a leaf
k_heal = int(0.8 * MM) | 1
inner_art = cv2.morphologyEx(inner_art, cv2.MORPH_CLOSE,
             cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (k_heal, k_heal)))
hc, hh = cv2.findContours(inner_art, cv2.RETR_CCOMP, cv2.CHAIN_APPROX_SIMPLE)
if hh is not None:
    for j, c in enumerate(hc):
        if hh[0][j][3] != -1 and cv2.contourArea(c) < (2.0 * MM) ** 2:
            cv2.drawContours(inner_art, [c], -1, 1, -1)

# ---------- 3. (thickening now happens in vector space, uniformly) ----------
full = ((inner_art & (interior | ring)) | ring).astype(np.uint8)

# ---------- 4. weld floating pieces (curved ties to nearest mass) ----------
welds = 0
for _ in range(6):
    n, lbl = cv2.connectedComponents(full)
    if n <= 2:
        break
    main = lbl[ring.astype(bool)].flat[0]
    sizes = [(i, (lbl == i).sum()) for i in range(1, n) if i != main]
    moved = False
    for i, sz in sizes:
        comp = np.argwhere(lbl == i)
        if sz < (0.5 * MM)**2:                    # dust
            full[lbl == i] = 0
            continue
        cs = comp[::max(1, len(comp)//400)]
        mp = np.argwhere((lbl == main))[::37]
        d = ((cs[:, None, :] - mp[None])**2).sum(-1)
        ai, bi = np.unravel_index(d.argmin(), d.shape)
        cv2.line(full, tuple(cs[ai][::-1]), tuple(mp[bi][::-1]), 1,
                 max(3, int(A.minfeat * MM)))
        welds += 1
        moved = True
    if not moved:
        break

# ---------- 4b. anchor enforcement (Steve 08-13): every art island needs ----------
# >=2 ties to the ring or to other art. One contact = snaps in handling.
def _art_comps():
    art_only = (full & interior).astype(np.uint8)
    return cv2.connectedComponents(art_only)

def _contacts(comp_mask):
    """(n_arcs, arc_mm): separate ring-contact arcs + total rim seam length."""
    dil = cv2.dilate(comp_mask, np.ones((7, 7), np.uint8))
    band = (dil & ring).astype(np.uint8)
    n, _ = cv2.connectedComponents(band)
    px = np.argwhere(band)
    if len(px) == 0:
        return 0, 0.0
    ang = np.degrees(np.arctan2(px[:, 0] - RES/2, px[:, 1] - RES/2)).astype(int)
    arc_mm = len(np.unique(ang)) * math.pi / 180.0 * R_in
    return n - 1, arc_mm

def _sound(comp_mask):
    n, arc = _contacts(comp_mask)
    # two separate ties OR one broad fused seam (>=4mm of rim) is structural
    return n >= 2 or arc >= 4.0

anchor_ext = 0
for _pass in range(30):
    ncomp, lbl = _art_comps()
    deficient = []
    for ci in range(1, ncomp):
        cm = (lbl == ci).astype(np.uint8)
        if cm.sum() < (0.8 * MM) ** 2:
            continue
        if not _sound(cm):
            deficient.append((ci, cm))
    if not deficient:
        break
    ci, cm = deficient[0]
    # try a stem weld to another art island first (Steve's fix for 32)
    others = ((lbl > 0) & (lbl != ci)).astype(np.uint8)
    welded = False
    if others.any():
        cs = np.argwhere(cm)[::max(1, int(cm.sum()) // 500)]
        os_ = np.argwhere(others)[::max(1, int(others.sum()) // 800)]
        dmat = ((cs[:, None, :] - os_[None]) ** 2).sum(-1)
        ai, bi = np.unravel_index(dmat.argmin(), dmat.shape)
        if dmat[ai, bi] < (4.0 * MM) ** 2:
            cv2.line(full, tuple(cs[ai][::-1]), tuple(os_[bi][::-1]), 1, int(1.6 * MM))
            welded = True
    if not welded:
        # extend the leaf tip nearest the ring outward until it touches
        # (radial direction = the way a rim-side leaf already points);
        # thin stroke so it reads as the leaf's drawn-out point
        dil = cv2.dilate(cm, np.ones((7, 7), np.uint8))
        contact_px = np.argwhere(dil & ring)
        th0 = None
        if len(contact_px):
            cyx = contact_px.mean(0)
            th0 = np.arctan2(cyx[0] - RES/2, cyx[1] - RES/2)
        pts = np.argwhere(cm)
        rad = np.sqrt(((pts - RES/2) ** 2).sum(1)) / MM
        ang = np.arctan2(pts[:, 0] - RES/2, pts[:, 1] - RES/2)
        score = rad.copy()
        if th0 is not None:
            dth = np.abs(np.angle(np.exp(1j * (ang - th0))))
            score = rad + dth * 6.0          # prefer far side from the anchor
        p = pts[int(score.argmax())]
        v = p - np.array([RES/2, RES/2])
        v = v / (np.linalg.norm(v) + 1e-9)
        q = (np.array([RES/2, RES/2]) + v * (R_out - 0.3) * MM).astype(int)
        cv2.line(full, tuple(p[::-1]), tuple(q[::-1]), 1, int(1.1 * MM))
    anchor_ext += 1

# final audit: count art islands still under 2 anchors (must be 0)
_n, _lbl = _art_comps()
still_deficient = 0
for _ci in range(1, _n):
    _cm = (_lbl == _ci).astype(np.uint8)
    if _cm.sum() < (0.8 * MM) ** 2:
        continue
    if not _sound(_cm):
        still_deficient += 1

# ---------- 5. vectorise gently ----------
from shapely.geometry import Polygon
from shapely.ops import unary_union
cnts, hier = cv2.findContours(full, cv2.RETR_CCOMP, cv2.CHAIN_APPROX_NONE)
hier = hier[0] if hier is not None else []
outers, holes = [], []
for i, c in enumerate(cnts):
    if len(c) < 8:
        continue
    pts = c[:, 0, :].astype(float) / MM - A.diam/2.0     # centred mm
    poly = Polygon(pts)
    if not poly.is_valid:
        poly = poly.buffer(0)
    if poly.is_empty or poly.area < 0.15:
        continue
    if hier[i][3] != -1 and poly.area < 3.0:
        continue                       # micro-slivers inside leaves
    (holes if hier[i][3] != -1 else outers).append(poly)
solid = unary_union(outers)
if holes:
    solid = solid.difference(unary_union(holes))
# separate the ring from the art, offset the art uniformly (smooth "ink bleed"
# that takes every stroke past minfeat without lumps), then re-union
from shapely.geometry import Point as _Pt
_ring_v = _Pt(0, 0).buffer(R_out, quad_segs=512).difference(
          _Pt(0, 0).buffer(R_in, quad_segs=512))
_art_v = solid.difference(_Pt(0, 0).buffer(R_in + 0.05, quad_segs=512).exterior.buffer(0)) \
         if False else solid
_art_v = solid.difference(_ring_v.buffer(0.02))
_art_v = _art_v.buffer(0.58, join_style=1, quad_segs=8)
solid = unary_union([_art_v.intersection(_Pt(0, 0).buffer(R_out - 0.05, quad_segs=512)), _ring_v])
def _chaikin(pts, iters=2):
    P = np.asarray(pts)[:-1]
    for _ in range(iters):
        Q = np.empty((2*len(P), 2))
        R = np.roll(P, -1, axis=0)
        Q[0::2] = 0.75*P + 0.25*R
        Q[1::2] = 0.25*P + 0.75*R
        P = Q
    return np.vstack([P, P[:1]])

def _smooth(poly):
    try:
        ext = _chaikin(np.array(poly.exterior.coords))
        ins = [_chaikin(np.array(r.coords)) for r in poly.interiors
               if len(r.coords) > 8]
        q = Polygon(ext, ins)
        return q if q.is_valid and not q.is_empty else poly
    except Exception:
        return poly

_geoms = list(solid.geoms) if solid.geom_type == "MultiPolygon" else [solid]
solid = unary_union([_smooth(g) for g in _geoms])
solid = solid.simplify(0.03).buffer(0)
# close hairline pinches + heal self-touches so the extrusion is manifold
solid = solid.buffer(0.06, join_style=1).buffer(-0.06, join_style=1).buffer(0)

# ---------- 6. outputs ----------
if A.no_stl:
    import math as _m
    geoms = list(solid.geoms) if solid.geom_type == "MultiPolygon" else [solid]
    mesh = None
    stl = None
else:
    import trimesh
if not A.no_stl:
    geoms = list(solid.geoms) if solid.geom_type == "MultiPolygon" else [solid]
    meshes = [trimesh.creation.extrude_polygon(gm, height=A.thick, engine="earcut")
              for gm in geoms if gm.area > 0.4]
    mesh = trimesh.util.concatenate(meshes) if len(meshes) > 1 else meshes[0]
    mesh.merge_vertices()
    mesh.update_faces(mesh.nondegenerate_faces())
    mesh.fix_normals()
    if not mesh.is_watertight:
        mesh.merge_vertices(digits_vertex=2)
        mesh.update_faces(mesh.nondegenerate_faces())
        trimesh.repair.fill_holes(mesh)
        mesh.fix_normals()
    stl = os.path.join(A.out, f"{name}.stl")
    mesh.export(stl)

# preview rendered from the FINAL smoothed vectors - what actually prints
PVX = 1600
psc = PVX / A.diam
pv = np.full((PVX, PVX, 3), 255, np.uint8)
for gm in geoms:
    ex = ((np.array(gm.exterior.coords) + A.diam/2) * psc).astype(np.int32)
    cv2.fillPoly(pv, [ex], (25, 25, 25), cv2.LINE_AA)
    for hole in gm.interiors:
        hh2 = ((np.array(hole.coords) + A.diam/2) * psc).astype(np.int32)
        cv2.fillPoly(pv, [hh2], (255, 255, 255), cv2.LINE_AA)
cv2.imwrite(os.path.join(A.out, f"{name}.png"), pv)

U = 96.0/25.4
d = []
for gm in geoms:
    for ringc in [gm.exterior, *gm.interiors]:
        pts = np.array(ringc.coords) * U
        d.append("M " + " L ".join(f"{p[0]:.3f} {p[1]:.3f}" for p in pts[:-1]) + " Z")
half = R_out * U
open(os.path.join(A.out, f"{name}.svg"), "w").write(
    '<?xml version="1.0" encoding="UTF-8"?>\n'
    f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="{A.diam:g}mm" '
    f'height="{A.diam:g}mm" viewBox="{-half:.5f} {-half:.5f} {2*half:.5f} {2*half:.5f}">\n'
    f'  <path id="printed-solid" fill="#000000" fill-rule="evenodd" d="{" ".join(d)}"/>\n</svg>\n')

# ---------- verify ----------
disc_a = math.pi * R_out**2
blocked = 100.0 * solid.area / disc_a
n0 = 1 if solid.geom_type == "Polygon" else len(solid.geoms)
feat = None
for w in np.arange(0.3, 3.01, 0.1):
    er = solid.buffer(-w/2.0)
    n1 = 0 if er.is_empty else (1 if er.geom_type == "Polygon" else len(er.geoms))
    if n1 > n0 or er.is_empty:
        feat = round(float(w), 2)
        break
print(dict(name=name, blocked_pct=round(blocked, 1), welds=welds, anchors_added=anchor_ext, weak_islands_left=still_deficient,
           min_feature_mm=(feat if feat else ">3"), one_piece=(n0 == 1),
           watertight=(bool(mesh.is_watertight) if mesh is not None else "n/a (svg only)"),
           stl=stl))
