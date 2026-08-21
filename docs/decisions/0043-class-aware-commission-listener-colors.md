# ADR 0043: Class-aware commission listener colors

- Status: Accepted
- Date: 2026-08-16
- Decider: Ben Eckart
- Supersedes: ADR 0039's universal low-red beacon color only
- Extends: ADR 0022, ADR 0029, ADR 0039, ADR 0041 (fixture class)

## Context

The basic commission listener originally rendered every fixture at linear red
128. That is useful on a single RGB point source, but it wastes the dedicated
warm-white die on canopy/downlight RGBW fixtures and multiplies the idle load
across all 37 pixels of a perimeter HEX module.

## Decision

With no active bridge or dashboard tag lease, the basic listener renders:

- canopy/downlight: dedicated warm-white channel at linear 128;
- perimeter HEX: all pixels red at linear 16;
- trunk/uplight RGB: red at linear 128; and
- other non-canopy classes: red at linear 128 until their final optical profile
  is decided.

An explicit identify/tag or bridge lease overrides this posture. All local
power-tier brightness caps and rail vetoes remain authoritative.

## Consequences

- Canopy's default is both pleasant and efficient in lux per watt.
- The 37-pixel HEX listener is one eighth of its former per-channel level.
- RGB trunk lights do not attempt to use a nonexistent white channel.
- Reported LED-output telemetry, rather than the requested class default, is the
  dashboard's source of truth for the visible color bar.
