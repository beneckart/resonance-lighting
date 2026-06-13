# Headless export: real fixtures.json (78 Light_Sources) + a decimated tree .glb.
# Run: blender --background "<file>.blend" --python export_tree.py -- <outdir>
import bpy, json, sys, datetime, mathutils

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
outdir = argv[0] if argv else "."
FIX_OUT = f"{outdir}/fixtures.json"
GLB_OUT = f"{outdir}/tree.glb"

# ---- 1) FIXTURES from the Light_Sources collection (the 78 lanterns) -------------
col = bpy.data.collections.get("Light_Sources") or bpy.data.collections.get("Light_Bulbs")
objs = list(col.all_objects) if col else [o for o in bpy.data.objects if o.type == "LIGHT"]

# world-space bbox of light positions (for zone banding + scale)
zs = [o.matrix_world.translation.z for o in objs]
zmin, zmax = (min(zs), max(zs)) if zs else (0, 1)

def zone_for(z):
    if zmax == zmin:
        return "mid"
    t = (z - zmin) / (zmax - zmin)
    return "low" if t < 0.34 else ("mid" if t < 0.67 else "high")

fixtures = []
for i, o in enumerate(sorted(objs, key=lambda o: o.name)):
    p = o.matrix_world.translation
    L = o.data if o.type == "LIGHT" else None
    spot = round(L.spot_size * 57.2958, 1) if (L and getattr(L, "type", "") == "SPOT") else 120.0
    color = [round(v, 3) for v in (L.color if L else (1, 1, 1))]
    fixtures.append({
        "fixture_id": f"F{i:03d}",
        "name": o.name,
        "role": "canopy",
        "position": [round(p.x, 4), round(p.y, 4), round(p.z, 4)],
        "zone": zone_for(p.z),
        "led_type": "rgbw_4w",          # the ~5W RGBW point source (gobo / crisp beam)
        "lumens_max": 450,
        "beam_deg": spot,
        "design_color": color,
    })

# scene mesh bbox (full tree extent, for the twin's camera framing)
mn = [1e9] * 3; mx = [-1e9] * 3
for o in bpy.data.objects:
    if o.type == "MESH":
        for c in o.bound_box:
            w = o.matrix_world @ mathutils.Vector(c)
            for k in range(3):
                mn[k] = min(mn[k], w[k]); mx[k] = max(mx[k], w[k])

doc = {
    "meta": {
        "source": "blender:Tree_Resonance_packed_2026-06.13.ejf.blend",
        "exported": datetime.datetime.now().isoformat(timespec="seconds"),
        "up_axis": "Z", "units": "blender", "count": len(fixtures),
        "bbox": {"min": [round(v, 3) for v in mn], "max": [round(v, 3) for v in mx]},
        "schema": "resonance.fixtures/0.1",
    },
    "fixtures": fixtures,
}
with open(FIX_OUT, "w") as f:
    json.dump(doc, f, indent=1)
print(f"WROTE {FIX_OUT}: {len(fixtures)} fixtures, z[{round(zmin,2)},{round(zmax,2)}]")

# ---- 2) Decimated tree geometry -> .glb (structure only, draco-compressed) -------
try:
    bpy.ops.object.select_all(action="DESELECT")
    sel = 0
    struct = bpy.data.collections.get("01_Structure")
    if struct:
        for o in struct.all_objects:
            if o.type == "MESH":
                o.select_set(True); sel += 1
    if sel:
        bpy.ops.export_scene.gltf(
            filepath=GLB_OUT, export_format="GLB", use_selection=True,
            export_apply=True, export_draco_mesh_compression_enable=True,
            export_draco_mesh_compression_level=6, export_yup=True,
        )
        import os
        mb = os.path.getsize(GLB_OUT) / 1e6
        print(f"WROTE {GLB_OUT}: {sel} structure meshes, {mb:.1f} MB")
    else:
        print("NO 01_Structure meshes — skipped glb (fixtures still exported)")
except Exception as e:
    print(f"GLB export failed (non-fatal, fixtures still written): {e}")
