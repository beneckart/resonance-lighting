#!/usr/bin/env /usr/bin/python3
"""
gobo_gen.py — parametric bamboo-branch gobo generator for the Bambu Lab P1S.

MODEL (corrected per Elliot 08-13):
  The PRINTED material is the ring + bamboo BRANCHES + LEAVES — the leaves are
  the dark silhouette in the projection, light floods the open field around
  them. Structure is honest: every stem runs ring-to-ring, every leaf grows
  from a stem node in a spray of 2-5, the way bamboo actually holds its
  leaves. Connectivity comes from the botany, not from welds.

LEAF (from the reference art):
  Lanceolate blade on a short petiole, widest ~1/3 from the base, long drawn
  tip, gentle sabre curve. Clusters fan from shared nodes.

PARAMETERS
  --sprays   density: leaf clusters per branch
  --scale    leaf size multiplier
  --flow     branch curvature/diagonality (0.2 gentle chord .. 1.0 strong sweep)
  --branches 1..3 culms crossing the lens
  --species  tali (broader blade) | asper (longer, narrower)
  --seed     unique repeatable variation

OUT: <name>.png (visual check) · <name>.stl (P1S) · <name>.svg (Bezier master)
The previous negative-space generator is kept as gobo_gen_v1_negative.py.
"""
import argparse, math, os, random
import numpy as np

NOZZLE   = 0.40
MIN_FEAT = 1.4          # mm min printed feature (Steve's rule)
STEM_W   = 1.7          # mm main stem width
PETIOLE_W= 1.8          # mm leaf stalk
RING_W   = 2.0          # mm

SPECIES = {
    "tali":  dict(aspect=0.20, apex=0.30, concave=0.38),
    "asper": dict(aspect=0.15, apex=0.28, concave=0.50),
}


def bez(p0, c1, c2, p1, n=22):
    p0, c1, c2, p1 = map(np.asarray, (p0, c1, c2, p1))
    t = np.linspace(0, 1, n)[:, None]
    return ((1-t)**3*p0 + 3*(1-t)**2*t*c1 + 3*(1-t)*t**2*c2 + t**3*p1)


def leaf_poly(L, W, sp, sabre):
    """One bamboo blade, base at origin pointing +x. Smooth Bezier edges."""
    a = sp["apex"]
    tipdrop = sabre * L * 0.22
    base   = np.array([0.0, 0.0])
    apex_u = np.array([L*a,  W/2 + sabre*L*0.05])
    apex_l = np.array([L*a, -W/2 + sabre*L*0.05])
    tip    = np.array([L, tipdrop])
    up = np.vstack([
        bez(base, base+[L*a*0.35, W*0.42], apex_u+[-L*a*0.4, 0.02*W], apex_u),
        bez(apex_u, apex_u+[L*(1-a)*0.45, -W*0.06], tip+[-L*0.28, W*0.20 - sabre*L*0.10], tip)[1:],
    ])
    lo = np.vstack([
        bez(tip, tip+[-L*0.22, -W*0.16 - sabre*L*0.02], apex_l+[L*(1-a)*0.40, -W*0.10], apex_l),
        bez(apex_l, apex_l+[-L*a*0.42, -0.05*W], base+[L*a*0.30, -W*0.38], base)[1:],
    ])
    return np.vstack([up, lo[1:-1]])


def rot(pts, th):
    c, s = math.cos(th), math.sin(th)
    return pts @ np.array([[c, s], [-s, c]])


def stem_path(rng, R, flow):
    """A culm crossing the disc, ring to ring, gently bowed.
    The chord must pass NEAR the centre - rim-hugging stems read as clutter."""
    for _ in range(40):
        a0 = rng.uniform(0, 2*math.pi)
        span = math.pi + rng.uniform(-0.9, 0.9) * (1.2 - flow)
        # midpoint distance from centre = R*cos(span/2); demand < 0.4R
        if abs(math.cos(span / 2.0)) < 0.40:
            break
    a1 = a0 + span
    p0 = np.array([math.cos(a0), math.sin(a0)]) * (R + 1.0)
    p1 = np.array([math.cos(a1), math.sin(a1)]) * (R + 1.0)
    mid = (p0 + p1) / 2
    perp = np.array([-(p1-p0)[1], (p1-p0)[0]])
    perp /= (np.linalg.norm(perp) + 1e-9)
    bow = rng.uniform(0.35, 1.0) * flow * R * 0.9 * rng.choice([-1, 1])
    c1 = mid + perp * bow * 0.8 + (p0 - mid) * 0.25
    c2 = mid + perp * bow * 0.8 + (p1 - mid) * 0.25
    return bez(p0, c1, c2, p1, n=90)


def build(args):
    rng = random.Random(args.seed)
    sp = SPECIES[args.species]
    R_out = args.diam / 2.0
    R_in = R_out - args.ring

    from shapely.geometry import Polygon, Point, LineString
    from shapely.ops import unary_union

    ring = Point(0, 0).buffer(R_out, quad_segs=256).difference(
           Point(0, 0).buffer(R_in, quad_segs=256))
    clip = Point(0, 0).buffer(R_out - 0.4, quad_segs=256)

    printed = [ring]
    blades = []          # accepted blade polys - used to stop blob-merging
    n_leaves = 0
    for b in range(args.branches):
        line = stem_path(rng, R_out, args.flow)
        ls = LineString(line)
        stem_poly = ls.buffer(STEM_W / 2.0, cap_style=1).intersection(clip)
        printed.append(stem_poly)
        n_nodes = max(2, args.sprays)
        for i in range(n_nodes):
            u = (i + 0.7) / (n_nodes + 0.7) + rng.uniform(-0.04, 0.04)
            u = min(max(u, 0.06), 0.94)
            idx = int(u * (len(line) - 2))
            node = line[idx]
            if math.hypot(*node) > R_in - 2.0:
                continue
            tv = line[idx+1] - line[idx-1]
            tang = math.atan2(tv[1], tv[0])
            # ---- sumi-e cluster: a short twig off the culm, then 3-5 leaves
            # radiating from ONE point at its tip, middle leaf longest,
            # the whole fan biased downward the way bamboo sprays hang
            side = rng.choice([-1, 1])
            twig_th = tang + side * rng.uniform(0.6, 1.1)
            # bias the twig (and so the fan) toward "down" (+y on the sheet)
            if math.sin(twig_th) < 0 and rng.random() < 0.7:
                twig_th = -twig_th
            twig_len = rng.uniform(4.0, 9.0)
            dirv = np.array([math.cos(twig_th), math.sin(twig_th)])
            tip = node + dirv * twig_len
            if math.hypot(*tip) > R_in - 3.0:
                continue
            printed.append(LineString([tuple(node), tuple(tip)])
                           .buffer(1.6/2.0, cap_style=1))
            k = rng.randint(3, 5 if args.sprays < 6 else 6)
            fan_span = rng.uniform(1.1, 1.7)          # 63-97 degrees total
            fan_c = twig_th + rng.uniform(-0.25, 0.25)
            cluster = []
            order = sorted(range(k), key=lambda j: abs(j - (k-1)/2))
            for j in order:
                frac = (j - (k-1)/2) / max((k-1)/2, 1)
                th = fan_c + frac * fan_span / 2 + rng.uniform(-0.06, 0.06)
                mid = 1.0 - 0.30 * abs(frac)          # middle leaf longest
                Ln = args.scale * mid * rng.uniform(0.9, 1.1) * R_in * 0.62
                Wd = max(Ln * sp["aspect"] * rng.uniform(0.92, 1.08), MIN_FEAT + 0.8)
                sabre = rng.uniform(0.10, 0.35) * (1 if frac >= 0 else -1)
                blade = rot(leaf_poly(Ln, Wd, sp, sabre), -th) + tip + np.array([math.cos(th), math.sin(th)]) * 1.0
                poly = Polygon(blade)
                if not poly.is_valid:
                    poly = poly.buffer(0)
                if poly.is_empty:
                    continue
                poly = poly.buffer(-0.45, join_style=1).buffer(0.95, join_style=1).buffer(-0.5, join_style=1)
                if not poly.is_valid: poly = poly.buffer(0)
                if poly.is_empty or poly.geom_type != "Polygon":
                    continue
                poly = poly.intersection(clip)
                if poly.is_empty or poly.area < 6.0:
                    continue
                # cluster mates share the base; only mid-blade crossing is a fault
                outer = poly.difference(Point(tuple(tip)).buffer(Ln * 0.38))
                if any(outer.intersection(q).area > 0.08 * poly.area for q in cluster):
                    continue
                # full air gap to every other cluster
                if any(poly.distance(q) < 0.9 for q in blades):
                    continue
                if poly.intersection(stem_poly).area > 0.15 * poly.area:
                    continue
                # anchor the blade base to the cluster tip
                printed.append(LineString([tuple(tip), tuple(tip + rot(np.array([[Ln*0.2, 0.0]]), -th)[0])])
                               .buffer(PETIOLE_W/2.0, cap_style=1))
                cluster.append(poly)
                printed.append(poly)
                n_leaves += 1
            blades.extend(cluster)

    solid = unary_union(printed)
    solid = solid.buffer(0.05).buffer(-0.05)
    return dict(solid=solid, R_out=R_out, R_in=R_in, n_leaves=n_leaves)


def write_png(path, res, args, px=1400):
    import cv2
    R = res["R_out"]; sc = px / (2*R)
    img = np.full((px, px, 3), 255, np.uint8)
    solid = res["solid"]
    geoms = list(solid.geoms) if solid.geom_type == "MultiPolygon" else [solid]
    for g in geoms:
        ext = (np.array(g.exterior.coords) * sc + px/2).astype(np.int32)
        cv2.fillPoly(img, [ext], (25, 25, 25), cv2.LINE_AA)
        for hole in g.interiors:
            h = (np.array(hole.coords) * sc + px/2).astype(np.int32)
            cv2.fillPoly(img, [h], (255, 255, 255), cv2.LINE_AA)
    cv2.imwrite(path, img)


def write_svg(path, res, args):
    U = 96.0/25.4
    solid = res["solid"]
    geoms = list(solid.geoms) if solid.geom_type == "MultiPolygon" else [solid]
    d = []
    for g in geoms:
        for ringc in [g.exterior, *g.interiors]:
            pts = np.array(ringc.coords) * U
            d.append("M " + " L ".join(f"{p[0]:.3f} {p[1]:.3f}" for p in pts[:-1]) + " Z")
    half = res["R_out"] * U
    open(path, "w").write(
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="{args.diam:g}mm" '
        f'height="{args.diam:g}mm" viewBox="{-half:.5f} {-half:.5f} {2*half:.5f} {2*half:.5f}">\n'
        f'  <path id="printed-solid" fill="#000000" fill-rule="evenodd" d="{" ".join(d)}"/>\n'
        '</svg>\n')


def write_stl(path, res, args):
    import trimesh
    solid = res["solid"].buffer(0).simplify(0.02).buffer(0)
    geoms = list(solid.geoms) if solid.geom_type == "MultiPolygon" else [solid]
    meshes = [trimesh.creation.extrude_polygon(g, height=args.thick, engine="earcut")
              for g in geoms if g.area > 0.5]
    mesh = trimesh.util.concatenate(meshes) if len(meshes) > 1 else meshes[0]
    mesh.merge_vertices()
    mesh.update_faces(mesh.nondegenerate_faces())
    mesh.fix_normals()
    mesh.export(path)
    return mesh, solid


def report(res, mesh, solid, args):
    from shapely.geometry import Point
    disc = Point(0, 0).buffer(res["R_out"], quad_segs=128)
    blocked = 100.0 * solid.area / disc.area
    feat = None
    n0 = 1 if solid.geom_type == "Polygon" else len(solid.geoms)
    for w in np.arange(0.3, 3.01, 0.1):
        er = solid.buffer(-w/2.0)
        n1 = 0 if er.is_empty else (1 if er.geom_type == "Polygon" else len(er.geoms))
        if n1 > n0 or er.is_empty:
            feat = w
            break
    bodies_2d = n0
    return dict(leaves=res["n_leaves"], blocked_pct=round(blocked, 1),
                min_feature_mm=(round(feat, 2) if feat else ">3"),
                one_piece=(bodies_2d == 1),
                watertight=bool(mesh.is_watertight), tris=int(len(mesh.faces)))


def main():
    P = argparse.ArgumentParser()
    P.add_argument("--name", default="gobo")
    P.add_argument("--out", default="out")
    P.add_argument("--diam", type=float, default=50.0)
    P.add_argument("--ring", type=float, default=2.0)
    P.add_argument("--thick", type=float, default=3.0)
    P.add_argument("--branches", type=int, default=2)
    P.add_argument("--sprays", type=int, default=4)
    P.add_argument("--scale", type=float, default=1.0)
    P.add_argument("--flow", type=float, default=0.7)
    P.add_argument("--species", default="tali", choices=list(SPECIES))
    P.add_argument("--seed", type=int, default=1)
    A = P.parse_args()
    os.makedirs(A.out, exist_ok=True)
    res = build(A)
    base = os.path.join(A.out, A.name)
    write_png(base + ".png", res, A)
    write_svg(base + ".svg", res, A)
    mesh, solid = write_stl(base + ".stl", res, A)
    print(dict(name=A.name, **report(res, mesh, solid, A)))


if __name__ == "__main__":
    main()
