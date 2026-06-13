"""
export_context.py — headless Blender exporter for the tree-context backdrop.

Produces a LIGHTWEIGHT glTF (.glb) of the tree's recognizable form so the
lighting twin reads as a tree (structure + bamboo), NOT the LEDs themselves.

Includes ONLY meshes in collections 01_Structure + 02_Bamboo.
Excludes LED_Meshes / Lights / Cameras / Dust / QUARANTINE / 03_Decorative.

Each included mesh gets an aggressive Decimate modifier to crush poly count,
then the selection is exported as Draco-compressed GLB.

Usage:
    blender --background "<blend>" --python export_context.py -- <outpath> [decimate_ratio]
"""

import sys
import os
import bpy

# ---- route scratch to the external drive (main disk is nearly full) ----
SCRATCH = "/Volumes/SUNEAST/resonance-scratch/tmp"
try:
    os.makedirs(SCRATCH, exist_ok=True)
    os.environ["TMPDIR"] = SCRATCH
    bpy.context.preferences.filepaths.temporary_directory = SCRATCH
except Exception as e:
    print(f"[warn] could not set scratch dir: {e}")

# ---- parse args after the `--` ----
argv = sys.argv
if "--" in argv:
    argv = argv[argv.index("--") + 1:]
else:
    argv = []

if not argv:
    print("[error] no output path given")
    sys.exit(1)

OUTPATH = argv[0]
DECIMATE_RATIO = float(argv[1]) if len(argv) > 1 else 0.1

# Collections we want the recognizable tree form from.
INCLUDE_COLLECTIONS = {"01_Structure", "02_Bamboo"}
# Hard exclusions by collection-name substring (case-insensitive).
EXCLUDE_SUBSTR = ("led", "light", "camera", "dust", "quarantine", "decorat")


def collection_chain_names(obj):
    """All collection names this object belongs to (direct membership)."""
    return {c.name for c in obj.users_collection}


def is_in_included_collection(obj):
    """True if the object lives in an INCLUDE collection or a child of one."""
    # Build a quick parent map by walking the master collection tree.
    names = collection_chain_names(obj)
    # direct membership in an include collection
    if names & INCLUDE_COLLECTIONS:
        return True
    # membership in a child collection of an include collection
    for inc in INCLUDE_COLLECTIONS:
        coll = bpy.data.collections.get(inc)
        if coll is None:
            continue
        # recurse children
        stack = list(coll.children)
        child_names = {coll.name}
        while stack:
            ch = stack.pop()
            child_names.add(ch.name)
            stack.extend(ch.children)
        if names & child_names:
            return True
    return False


def looks_excluded(obj):
    nm = obj.name.lower()
    coll_names = " ".join(collection_chain_names(obj)).lower()
    blob = nm + " " + coll_names
    return any(s in blob for s in EXCLUDE_SUBSTR)


def main():
    scene = bpy.context.scene

    # deselect everything
    bpy.ops.object.select_all(action="DESELECT")

    selected = []
    total_objs = 0
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        total_objs += 1
        if looks_excluded(obj):
            continue
        if not is_in_included_collection(obj):
            continue
        selected.append(obj)

    print(f"[info] total meshes in file: {total_objs}")
    print(f"[info] meshes selected for context (structure+bamboo): {len(selected)}")

    if not selected:
        print("[error] no meshes matched include collections — aborting")
        sys.exit(2)

    # Some matched objects exist in bpy.data but are NOT linked into the
    # active ViewLayer (they live only in excluded/unlinked collections), so
    # select_set() throws. Link every selected object into a dedicated export
    # collection under the scene's master collection so they all become
    # selectable in the view layer.
    view_layer = bpy.context.view_layer
    master = scene.collection

    export_coll = bpy.data.collections.get("_ctx_export")
    if export_coll is None:
        export_coll = bpy.data.collections.new("_ctx_export")
        master.children.link(export_coll)

    linked = 0
    for obj in selected:
        try:
            export_coll.objects.link(obj)
            linked += 1
        except RuntimeError:
            # already linked somewhere in the view layer — fine
            pass
    print(f"[info] objects linked into export collection: {linked}")

    # refresh so the view layer sees the newly linked objects
    view_layer.update()

    # Now select + add decimate. Skip any object still not in the view layer.
    vl_objs = set(view_layer.objects)
    added_dec = 0
    sel_count = 0
    for obj in selected:
        if obj not in vl_objs:
            print(f"[warn] {obj.name} not in view layer after link — skipping")
            continue
        try:
            obj.hide_set(False)
            obj.hide_viewport = False
            obj.hide_select = False
        except Exception:
            pass
        try:
            obj.select_set(True)
            sel_count += 1
        except RuntimeError as e:
            print(f"[warn] could not select {obj.name}: {e}")
            continue
        # Add a decimate modifier (collapse) to crush polys.
        try:
            mod = obj.modifiers.new(name="ctx_decimate", type="DECIMATE")
            mod.decimate_type = "COLLAPSE"
            mod.ratio = DECIMATE_RATIO
            added_dec += 1
        except Exception as e:
            print(f"[warn] could not add decimate to {obj.name}: {e}")

    print(f"[info] objects selected: {sel_count}")
    print(f"[info] decimate modifiers added: {added_dec} (ratio={DECIMATE_RATIO})")

    if sel_count == 0:
        print("[error] nothing selectable after linking — aborting")
        sys.exit(4)

    # set an active object so the export op is happy
    view_layer.objects.active = next(o for o in selected if o in vl_objs)

    os.makedirs(os.path.dirname(OUTPATH), exist_ok=True)

    print(f"[info] exporting GLB to {OUTPATH} ...")
    bpy.ops.export_scene.gltf(
        filepath=OUTPATH,
        export_format="GLB",
        use_selection=True,
        export_yup=True,
        export_apply=True,  # apply modifiers (the decimate) on export
        export_draco_mesh_compression_enable=True,
        export_draco_mesh_compression_level=6,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
        export_animations=False,
    )

    if os.path.exists(OUTPATH):
        size = os.path.getsize(OUTPATH)
        print(f"[result] glb written: {OUTPATH}")
        print(f"[result] size_bytes={size}")
        print(f"[result] size_mb={size / (1024*1024):.2f}")
    else:
        print("[error] export finished but file not found")
        sys.exit(3)


if __name__ == "__main__":
    main()
