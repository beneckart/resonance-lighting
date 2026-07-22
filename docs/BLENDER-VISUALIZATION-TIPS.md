# Lighting controller → Blender: reproducing the visualizations

Handoff from **lighting-architect** to **blender-architect** (2026-07-21). How to make the
lighting controller's looks — especially the new ☀️ Solar Ray — render inside Blender/Cycles.

Controller repo: `~/code/resonance-tree-lighting` (branch `elliots-controller`).
Live twin: https://resonanceart.github.io/resonance-lighting/

---

## The one idea that makes this easy

Every pattern in the controller is a **pure per-fixture function**. The engine is
`app/src/patterns.ts` → `litFor(t, fixture, control) → {r,g,b}`. It takes time + one
fixture's geometry + the control params, and returns that light's colour. It does **not**
stream pixels — each light computes its own colour from its own position. So you can port
the exact math to a `bpy` Python script and drive each Blender lantern's emission per
frame. **Same inputs → same picture.**

The shared contract you already own is the bridge: **`fixtures.json`** (you export it; the
controller consumes it). Each fixture has `position`, `aim`, `role`
(downlight/uplight/chandelier), `zone`, `ring` (0=inner,1=mid,2=outer), and azimuth/radialT
derivable from position. Those are the **only** inputs the patterns need.

---

## Blender setup (once)

1. One emissive object per fixture at `fixtures.json` positions — the bamboo lantern mesh
   with an Emission shader (or a Point/Spot light per fixture; spots + a gobo texture give
   the mandala ground projection).
2. Drive each lantern's Emission Color + Strength from a frame-change handler script
   (simplest) or drivers.
3. Cycles + volumetric world (thin Volume Scatter, density ~0.02) = the god-ray shafts.
   Top-down orthographic camera = the "tree as a sun" view.
4. Ground plane below so the gobo circles read (the mandala field).

---

## The per-frame script (the whole trick)

```python
import bpy, math, colorsys
def frac(x): return x - math.floor(x)
def clamp01(x): return max(0.0, min(1.0, x))
def hsl(h, s, l=0.5): return colorsys.hls_to_rgb(h % 1.0, l, s)  # three.js-style

# per fixture you need: rT (radialT 0..1), sunRay (int or None), is_core (bool)
def solar_ray(t, fx, speed=0.6):
    if fx['is_core']:
        return (*hsl(0.12, 0.66), 1.3)                       # molten gold core
    if fx['sunRay'] is None:
        return (0, 0, 0, 0.0)                                # not part of the sun → OFF
    rT = clamp01(fx['rT']); w = 0.5 + speed * 0.55
    jitter = (frac(math.sin(fx['sunRay'] * 12.9898) * 43758.5453) - 0.5) * 0.28
    phase = rT * 1.7 - t * w + jitter                        # crest travels OUTWARD
    crest = (0.5 + 0.5 * math.cos(phase * 2 * math.pi)) ** 2
    energy = clamp01((0.22 + 0.85 * crest) * (1.1 - 0.28 * rT))
    hue = 0.006 + 0.125 * (energy * energy)                  # deep red → gold
    sat = 1 - 0.34 * energy
    return (*hsl(hue, sat), energy)                          # rgb + strength

def on_frame(scene):
    t = scene.frame_current / scene.render.fps               # seconds
    for lantern in fixtures:                                  # your list built from fixtures.json
        r, g, b, strength = solar_ray(t, lantern.data)
        em = lantern.obj.material.node_tree.nodes['Emission']
        em.inputs['Color'].default_value = (r, g, b, 1)
        em.inputs['Strength'].default_value = strength * BASE_WATTS

bpy.app.handlers.frame_change_pre.append(on_frame)
```

---

## The Solar Ray stencil (`sunRay`)

The rays only read as rays because most lights stay dark. Port `assignSunRays()` from
`patterns.ts`: 12 ray spines around the trunk; for each spine × each ring, assign the ONE
nearest downlight (exclude radialT < 0.14 — those are the core). Result: ~36 of 78
downlights draw the sun; the other 42 are **always off** (the dark wedges between rays ARE
the drawing). `is_core` = chandelier fixtures OR downlights with radialT < 0.14.

---

## Why it looks right

- energy is a **continuous traveling wave** (a crest emitted at the core flows outward),
  not discrete on/off — that was the key fix; render enough frames (or add motion blur)
  and you'll see the flow.
- colour follows energy per Ben's watt colourbar: deep red trough → gold crest.
- for hero stills, Cycles volumetrics + a gobo texture (there are `gobo-mandala.png` /
  `real-gobo` refs in the repo) sell it far better than the WebGL twin.

---

## The other patterns

Same recipe — any of the 35 patterns is a `case` in `litFor`. `spectrum`, `godray`,
`aurora`, `chains`, `ca` port the same way (~10 lines of pure math each on the same fixture
fields). Ask lighting-architect for a specific one and it can be transliterated to Python.

**Offer:** lighting-architect can hand over (1) a flattened `fixtures.json` with
`ring`/`radialT`/`azimuth`/`sunRay` precomputed per fixture (so your script needs zero
geometry math), and (2) whichever `litFor` cases you want, transliterated to Python. Happy
to pair.
