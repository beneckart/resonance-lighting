# 0007 — Electronics live in a hat, not inside the bamboo

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben + Steve, communicated to Elliot/Vishnu via WhatsApp

## Context

The bamboo lantern enclosure is designed by Vishnu (sealed shop drawing, in fab at Bamboo Pure). Interior diameter at the top of the bamboo lantern is 5.5 cm. Solar panel for ~1–3 W needs a footprint larger than 5.5 cm. Either:

1. Crowbar all electronics into the bamboo (sub-5.5 cm form factor, with a panel that doesn't extend beyond the bamboo lantern — limits panel to ~1 W, and is fragile to the bamboo dimensional variability).
2. Put electronics in a separate "hat" that sits partially in / partially over the top of the bamboo, like the solar-cap of a standard solar garden lantern.

Reference: common metal Moroccan-style solar garden lanterns (Amazon ASIN B0DKNLGCDM and similar) — separable solar top, decorative metal body, mechanical decoupling.

## Options considered

- **All-inside the bamboo lantern** (path 1). Constrains panel size, complicates mechanical design around the bamboo's natural variability.
- **Modular hat** (path 2). Standard solar-lantern pattern.

## Decision

**Modular hat.** Electronics, panel, battery, LEDs, antenna all live in a sealed 3D-printed enclosure on top of the bamboo. Hat clamps to the bamboo neck via set screws (3 at 120° initial plan) to absorb bamboo dimensional variability.

## Consequences

- **Fungibility:** any hat fits any bamboo lantern. Swap-broken-fixture in under a minute (drop in a spare hat or a spare bamboo). Caveat: rope attachment point decision (open) affects this.
- **Panel sizing flexibility:** hat can be larger than the bamboo top, so panel up to ~3 W is achievable within the structural 1 kg/fixture budget.
- **Sealed enclosure for the electronics:** dust ingress, UV, and rain handled at the hat level. Bamboo lantern body remains naturally porous.
- **Aesthetic alignment:** team has explicitly accepted plastic-when-it-earns-its-keep (per WhatsApp 04-23). Hat is plastic; filter is plastic; team knows.
- **Open mechanical question: rope attachment.** Hat-mounted (preferred for fungibility), bamboo-mounted (more "of a piece"), or hybrid (primary on hat, secondary safety tie around bamboo neck). Decision pending team call.
- **Open thermal question: vent gap or fully sealed.** Vent helps thermal but hurts dust ingress. Fully sealed means LEDs and CN3058 (linear charger, dissipating ~1 W) heat-soak the cavity. Probably manageable given small thermal load and big enclosure relative to load, but worth measuring on prototype.
