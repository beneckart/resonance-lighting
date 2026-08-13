# ChatGPT image-generation prompt — bamboo leaf gobo art

Paste this into ChatGPT (image generation). Ask for 4 variations per run.

---

Design a circular stencil pattern for a projection gobo, 1:1 square image.

A perfect black circle fills the frame on a pure white background. Cut through
the black disc are the silhouettes of bamboo leaves in pure white — slender,
lanceolate leaves that taper to a fine point at BOTH ends, with gently concave
edges near the tip, like sumi-e brushstrokes of bamboo tali leaves.

Composition: the leaves flow in a natural diagonal current THROUGH the circle,
like leaves drifting on a stream — clusters and gaps, some leaves overlapping
the path of others, a few small leaves trailing at the edges of the current.
Do NOT arrange the leaves in a ring or wreath around the border. Do NOT place
a border of leaves. The current should enter one side of the circle and exit
the other.

Constraints (this will be 3D printed at 50 mm diameter):
- pure black and pure white only, no grey, no gradients, no outlines
- every leaf fully inside the circle, none touching the rim
- leaves never touch or overlap each other; keep clear black space between them
- between 9 and 20 leaves total, varied sizes, the largest about 1/3 of the
  circle's width, the smallest about 1/10
- smooth flowing curves only — no jagged or sketchy edges

Style: Japanese calligraphy minimalism; the elegance is in the spacing.

---

Variation levers (change one line per batch):
- "between 9 and 12 leaves, large and bold" → sparse set
- "between 18 and 24 leaves, small and lively" → dense set
- "the current curves in a gentle S through the circle" → S-flow set
- "two currents crossing at a shallow angle" → crossing set
- "leaves of bamboo asper: very long, very narrow" → asper set

Then: save as PNG, send it to me, and I run it through art_to_gobo.py
(raster path) or trace it parametrically for the smooth-curve version.
