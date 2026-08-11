# Enclosure

Solar "hat" enclosure for the Resonance fixtures. Steve's workstream.

**STRATEGY UPDATE 2026-07-13: the enclosure bodies are BOUGHT, not printed.**
172x COTS sealed enclosures + screws ordered ~07-12/13 ($5,306.50 total; vendor/
part details TBC in `ops/PROCUREMENT.md`), split 22 to TN / 150 to CA:

- **LARGE (111):** 72 hanging downlight hats, leaving 39 units of gross inventory
  headroom (including the transparent-lid demo unit).
- **SMALL (61):** 24 perimeter-light hats plus candidate enclosures for about 16
  trunk lights. Final trunk mounting and power geometry are being settled during
  Nevada City integration.
- **Chandelier:** no hat -- a team carpenter builds a box housing the 18 lights
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

## Trunk-light variant (supersedes the uplight-boot allocation)

The Nevada City layout now targets about 16 trunk lights with no gobo (ADR 0032).
They are moving toward all 4 W RGBW, while a smaller lensed 3 W RGB module is being
tested for extra throw. Candidate hardware includes the small Polycase, standard
6 Ah cell, gasketed panel-mount USB-C charge/flash port, and available P105 panels,
but the final power arrangement, optic protection, and attachment to the trunk are
open until the comparison is complete. The previous 24-uplight hinged-solar-wing
concept is a superseded July plan, not the production baseline. The 18 chandelier
lights use the carpenter-built box instead.

## Reference

- `references/DOWN LIGHTS DRAWINGS.pdf` -- Vishnu's bamboo lantern shop drawing,
  2026-04-22. (Not committed to the repo yet -- lives with Steve/Drive; TODO to
  commit or re-point.)

## Open mechanical decisions

- Rope attachment point (hat / bamboo / hybrid). See `BACKGROUND.md`.
- Vent gap vs sealed (thermal vs IP rating trade-off).
- Set screw count and placement (currently planned 3 at 120 deg , may revise based on prototype fit).
- Trunk light: attachment to the tree, battery/panel arrangement, USB-C gasketing,
  optic protection, and 4 W RGBW vs lensed 3 W RGB mounting.
- USB-C rescue port is now UNIVERSAL (150 panel-mount extension cables bought
  2026-07-10): every hat variant needs a gasketed panel-mount USB-C cutout wired
  to the PowerFeather -- rescue/charging without opening the enclosure.
- ToF apertures: downward beside the gobo (downlights), outward window with
  protective cover (perimeter).
