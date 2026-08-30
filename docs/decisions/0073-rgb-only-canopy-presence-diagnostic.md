# ADR 0073: RGB-only canopy presence diagnostic

**Status:** Accepted for immediate fleet deployment, 2026-08-30

## Context

The newly promoted canopy interaction path can override an active LED Studio
color with dedicated warm white (`W=255`). A live green census proved DIRECT
frames were reaching affected downlights before local interaction replaced
them, but the resulting warm-white cohort was initially mistaken for fixtures
disregarding the bridge command. Exact reset improved roughly half of the stuck
warm-white lights, while the named `9E5AC8` canary re-latched after warmup.

Ben needs an immediate, visually unambiguous field discriminator and explicitly
directed fleet deployment without a separate firmware canary.

## Decision

While downlight presence is latched, render the one RGBW point source as full
RGB white (`R=G=B=255`, `W=0`) instead of dedicated warm white. Do not change
presence thresholds, baselines, debounce, interaction precedence, blackout
protection, power policy, packet layout, or persisted state.

This is a diagnostic rendering change, not the resolution of LED Studio/local
interaction authority or post-lowering presence recovery.

## Safety and rollout

Full RGB point-source output is already a supported production frame and was
exercised by the Gible presence sentinel. The existing power-tier brightness
cap, rail ramp and battery-sag abort, and hard rail-off veto remain authoritative.

Build one immutable variant-`p` artifact from clean source and offer it only to
an explicit, freshly qualified fleet roster through the shared-WiFi OTA path.
Completion still requires fresh exact-revision heartbeats after the pending-
verify window. Ben explicitly waived the usual named firmware canary for this
simple diagnostic change because of field time pressure.

## Consequences

- A downlight rendered cool RGB white is direct visual evidence that its local
  presence latch owns the visible frame.
- Unaffected fixtures retain the current bridge/program color.
- The change does not prevent a stationary scene from latching or re-latching.
- ADR 0069/0070 authority and re-baselining follow-ups remain open and must
  supersede this diagnostic posture deliberately.
