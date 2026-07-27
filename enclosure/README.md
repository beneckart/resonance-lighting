# Enclosure

Solar "hat" enclosure for the Resonance fixtures. Steve's workstream.

**STRATEGY UPDATE 2026-07-13: the enclosure bodies are BOUGHT, not printed.**
172x COTS sealed enclosures + screws ordered ~07-12/13 ($5,306.50 total; vendor/
part details TBC in `ops/PROCUREMENT.md`), split 22 to TN / 150 to CA:

- **LARGE (111):** hanging downlight hats only (<=110 deployed; 72 planned --
  healthy spares under the corrected 2026-07-15 mapping).
- **SMALL (61):** perimeter-light hats AND uplight "boots" (perimeter + boots
  <= 60 combined). The uplight boot gains a **hinged solar "wing"** (decided
  2026-07-15; likely carrying the P105 5 W panel for partial/shaded sun) -- wing
  hinge + panel mount are new mechanical design items.
- **Chandelier:** no hat -- a team carpenter builds a box housing the 16 lights
  (coordinate venting, access, and USB-charging reach).

Steve's workstream shifts from print-the-hat to **integrate-the-hat**: panel
mounting, bamboo clamp/set-screw attachment, gasketed USB-C rescue-port cutout
(all variants), ToF windows (downward on downlights, outward + cover on
perimeter), LED/gobo positioning, strain relief, and thermal/RF proof on the
real boxes. 3D printing remains for the gobo/filter program and custom internal
fittings. The original printed-hat architecture below is kept for reference --
its constraints (antenna keep-out, set-screw clamping, panel-over-bamboo
geometry) still apply to the bought boxes.

## Architecture

A sealed cap that mounts on top of the bamboo lantern:

- **Outer disc / ring** -- overhangs the bamboo top to host the solar panel (panel needs more area than the 76 mm bamboo cylinder affords).
- **Inner cylinder** -- extends down into the bamboo neck (~5.5 cm interior diameter, per Bamboo Pure spec). This is the "stem" of the mushroom.
- **Set screws** -- through the inner cylinder, clamping outward against the bamboo wall to absorb dimensional variability of natural bamboo lanterns. Plan: 3 set screws at 120 deg .
- **Internal cavity** -- houses the carrier PCB, LiFePO4 cell, and downward-facing LEDs. Sealed for dust ingress.
- **Hanging point** -- TBD whether on the hat (preferred) or on the bamboo. See `BACKGROUND.md` for the trade-off discussion.

## Print pipeline

(2026-07-13: the hat BODIES are now bought COTS boxes -- this pipeline serves the
gobo/filter program and custom internal fittings, not hat production.)

- **Prototyping:** Bambu Labs printer (Steve's workshop). Iterate fit and look.
- **Batch parts if needed:** MJF nylon at JLC3DP, PCBWay, or Xometry. ~$3-10 per part.

## Filter / gobo (separate consumable component)

A thin 3D-printed disc with patterned cutouts that sits at the bamboo node notch, ~15 cm down from the top. Casts mandala shadows on the ground when the LED above shines through. Friction-fit at the node -- replaceable, swappable per fixture.

A more recent iteration extrudes the 2D pattern into a translucent cone via projective geometry -- looks like a glowing cone-shaped bulb from above, casts undistorted mandala below.

MVP direction (Ben + Steve, 2026-07-08): for time and robustness, production gobos
are likely FLAT DISCS -- the cone extrusion adds print complexity and makes the
"bulb" more brittle, so cones may be reserved for a few designs (or none),
depending on the pattern.

Pattern program update (2026-07-08): community submissions were pulled for time;
the plan is in-house designs plus generative-AI-modulated bamboo-leaf patterns per
bamboo species (see `BACKGROUND.md`).

## Uplight "boot" (new fixture class, tentative -- design input for Steve)

The 24 uplights are simple bamboo cylinders (no lower splay, no gobo). Updated
concept (2026-07-15): the 4 W RGBW sits near the lit end, and the small Polycase
"boot" at the base carries the PowerFeather, the standard 6 Ah cell, a gasketed
panel-mount USB-C charge/flash port, and a **hinged solar "wing"** (likely the
P105 5 W) that folds out to catch partial/shaded sun. Wing hinge + panel mount are
the new design items; the show runs a low-brightness budget tuned at the NC
prebuild. Same internals as every other fixture; sleeps during the day. (The
earlier fill-the-cylinder 20 Ah concept was cancelled on sourcing -- ADR 0025
annotation.) The 16 chandelier lights use the carpenter-built box instead.

## Reference

- `references/DOWN LIGHTS DRAWINGS.pdf` -- Vishnu's bamboo lantern shop drawing,
  2026-04-22. (Not committed to the repo yet -- lives with Steve/Drive; TODO to
  commit or re-point.)

## Open mechanical decisions

- Rope attachment point (hat / bamboo / hybrid). See `BACKGROUND.md`.
- Vent gap vs sealed (thermal vs IP rating trade-off).
- Set screw count and placement (currently planned 3 at 120 deg , may revise based on prototype fit).
- Uplight boot: battery retention in-cylinder, USB-C port gasketing, LED mount at
  the lit end.
- USB-C rescue port is now UNIVERSAL (150 panel-mount extension cables bought
  2026-07-10): every hat variant needs a gasketed panel-mount USB-C cutout wired
  to the PowerFeather -- rescue/charging without opening the enclosure.
- ToF apertures: downward beside the gobo (downlights), outward window with
  protective cover (perimeter).
