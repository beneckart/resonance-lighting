# Headless Blender inspector — dumps scene structure so we can target the export.
# Run: blender --background "<file>.blend" --python inspect_blend.py
import bpy, json, sys
from collections import Counter

out = {"collections": [], "type_counts": {}, "lights": [], "light_like": [],
       "big_meshes": [], "scene_bbox": None}

type_counts = Counter()
for o in bpy.data.objects:
    type_counts[o.type] += 1
out["type_counts"] = dict(type_counts)

for c in bpy.data.collections:
    out["collections"].append({"name": c.name, "objects": len(c.objects),
                               "all_objects": len(c.all_objects)})

# Actual LIGHT datablocks
for o in bpy.data.objects:
    if o.type == "LIGHT":
        L = o.data
        loc = o.matrix_world.translation
        out["lights"].append({
            "name": o.name, "light_type": getattr(L, "type", "?"),
            "x": round(loc.x, 4), "y": round(loc.y, 4), "z": round(loc.z, 4),
            "energy": getattr(L, "energy", None),
            "color": [round(v, 3) for v in getattr(L, "color", [1, 1, 1])],
        })

# Objects whose name hints they represent lights/lanterns/leds (could be meshes/empties)
KW = ("light", "led", "lantern", "lamp", "canopy", "fixture", "bulb")
for o in bpy.data.objects:
    n = o.name.lower()
    if any(k in n for k in KW) and o.type != "LIGHT":
        loc = o.matrix_world.translation
        out["light_like"].append({"name": o.name, "type": o.type,
                                   "x": round(loc.x, 3), "y": round(loc.y, 3), "z": round(loc.z, 3)})

# Largest meshes (for geometry + scale sense)
meshes = [(o, len(o.data.vertices)) for o in bpy.data.objects if o.type == "MESH"]
meshes.sort(key=lambda t: -t[1])
for o, vc in meshes[:15]:
    out["big_meshes"].append({"name": o.name, "verts": vc})

# Scene world-space bbox
import mathutils
mn = [1e9, 1e9, 1e9]; mx = [-1e9, -1e9, -1e9]
for o in bpy.data.objects:
    if o.type == "MESH":
        for corner in o.bound_box:
            w = o.matrix_world @ mathutils.Vector(corner)
            for i in range(3):
                mn[i] = min(mn[i], w[i]); mx[i] = max(mx[i], w[i])
out["scene_bbox"] = {"min": [round(v, 3) for v in mn], "max": [round(v, 3) for v in mx]}

print("RESONANCE_INSPECT_JSON_START")
print(json.dumps(out, indent=2))
print("RESONANCE_INSPECT_JSON_END")
