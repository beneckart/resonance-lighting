# 0006 -- Custom carrier PCB, not dev-board-on-carrier

**Date:** 2026-05-06
**Status:** Superseded by ADR 0012 -- Dual-track production architecture: COTS fallback plus custom PCBA optimization
**Owners:** Ben

## Context

Two paths exist for getting an ESP32 onto a production fixture:

1. Use dev modules / COTS boards mechanically integrated into the hat.
2. Reflow-solder a pre-certified Espressif module directly onto a custom carrier board.

The 2018 LoRa-pendant project used socketed Wemos D1 Mini boards and hand-soldered headers. Hand-soldering headers for 16 devices took ~40 hours. That experience must not be repeated at 100 units.

## Original decision

Use a custom carrier PCB only; no dev-board-on-carrier; no headers anywhere except optional debug pads.

## Why this is superseded

The operational constraint is not "never use dev boards." The real constraint is "no skilled, slow, error-prone per-fixture work." Factory-soldered headers, screw-mounted COTS boards, USB/JST connections, daughterboards, and pre-crimped cables can satisfy the no-solder / low-skill assembly goal. A COTS path should be built as a production-credible fallback while the custom PCBA path proceeds in parallel. See ADR 0012.
