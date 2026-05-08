# Enclosure

Solar "hat" enclosure for the Resonance downlight. Steve's workstream.

## Architecture

A sealed cap that mounts on top of the bamboo lantern:

- **Outer disc / ring** — overhangs the bamboo top to host the solar panel (panel needs more area than the 76 mm bamboo cylinder affords).
- **Inner cylinder** — extends down into the bamboo neck (~5.5 cm interior diameter, per Bamboo Pure spec). This is the "stem" of the mushroom.
- **Set screws** — through the inner cylinder, clamping outward against the bamboo wall to absorb dimensional variability of natural bamboo lanterns. Plan: 3 set screws at 120°.
- **Internal cavity** — houses the carrier PCB, LiFePO4 cell, and downward-facing LEDs. Sealed for dust ingress.
- **Hanging point** — TBD whether on the hat (preferred) or on the bamboo. See `BACKGROUND.md` for the trade-off discussion.

## Print pipeline

- **Prototyping:** Bambu Labs printer (Steve's workshop). Iterate fit and look.
- **Production (100 units):** MJF nylon at JLC3DP, PCBWay, or Xometry. ~$3–10 per part. Better dust/UV/heat tolerance than FDM PLA, and 100 units takes hours not weeks.

## Filter / gobo (separate consumable component)

A thin 3D-printed disc with patterned cutouts that sits at the bamboo node notch, ~15 cm down from the top. Casts mandala shadows on the ground when the LED above shines through. Friction-fit at the node — replaceable, swappable per fixture.

A more recent iteration extrudes the 2D pattern into a translucent cone via projective geometry — looks like a glowing cone-shaped bulb from above, casts undistorted mandala below.

If the Community Mandala Program goes ahead, each fixture gets a unique filter sourced from a contributor sketch. See `docs/decisions/` for the program design.

## Reference

- `references/DOWN LIGHTS DRAWINGS.pdf` — Vishnu's bamboo lantern shop drawing, 2026-04-22.

## Open mechanical decisions

- Rope attachment point (hat / bamboo / hybrid). See `BACKGROUND.md`.
- Vent gap vs sealed (thermal vs IP rating trade-off).
- Set screw count and placement (currently planned 3 at 120°, may revise based on prototype fit).
