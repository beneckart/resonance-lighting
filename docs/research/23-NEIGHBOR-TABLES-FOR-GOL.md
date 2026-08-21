# Neighbor tables → real-tree Game of Life (saved for later)

2026-08-15, lighting-architect, at Elliot's direction: "save these for later as
a game of life interactivity." Everything needed to wire the twin's Game of
Light onto the REAL fleet's own neighbor machinery, captured while it's fresh.

## What Ben built (verbatim contract, firmware/fixture/src/core/neighbor_table.h)

Every fixture keeps a **24-entry neighbor table** for a "150-node broadcast
domain where only the RF-nearest ~16 matter," with TWO adjacency sources for
the on-fixture CA (Ben, 2026-07-30):

- **RSSI mode (default):** strongest fresh heartbeat sources. Ben's own note:
  "expected to be marginal indoors (8–17 dB placement variance) but free to
  try" — matches our selfmap finding exactly (RF-only adjacency is noise at
  true tree scale).
- **Pinned mode:** host-pushed explicit adjacency via **NB_NEIGHBOR_SET
  (type 24)** — up to **8 neighbors per fixture**, `flags bit0 = persist to
  NVS`, `bit1 = clear/revert to RSSI`. Pinning overrides RSSI for CA purposes
  while the table keeps tracking everything for telemetry.

Each neighbor entry carries `choreoState / programId / generation / tier /
rssiEwma / age` — i.e., **a fixture can already see its neighbors' CA state**.
The CA-facing `neighborSnapshot()` returns only FRESH neighbors: "a pinned
neighbor never heard from is simply absent — loss looks organic, never
blocking." Death of a lantern degrades the game gracefully, by design.

Related, reserved but unsent: NB_NEIGHBOR_REPORT (22, censored-median RSSI for
locate) and NB_EVENT (23, the event fabric).

## The convergence (why this is a gift)

The twin's Interactive mode ALREADY runs the Game of Light on a pre-baked
k-nearest-neighbor list computed from fixtures.json's true geometry. That list
is exactly what NB_NEIGHBOR_SET wants:

```
fixtures.json (true positions, 130)
      │  twin already computes spatial kNN (field.ts adjacency)
      ▼
per-fixture 8-neighbor list  ──(MAC↔slot join)──►  NB_NEIGHBOR_SET per lantern
                                                   flags bit0 = persist
      ▼
REAL lanterns run Ben's on-fixture CA over TRUE TREE ADJACENCY —
decentralized, bridge-optional, "dies invisibly" preserved.
```

Taps (ToF presence, when NB_EVENT ships) seed life; light spreads lantern to
lantern over real geometry with no host in the loop. That IS the Game of Light
as chartered — running on the tree itself, not the twin.

## The phasing (Elliot, 08-15 — the strategy this doc serves)

> "For now we want to simplify the default mode so we can easily control the
> lights — and when we get them all hung up and in place later, we can flash
> the interactivity mode."

**Phase 1 (now, bench→build week):** simple, always-awake, directly-drivable
firmware (the led_studio direction) — no parks, no gates, obedient lights.
**Phase 2 (hung + placed):** flash the fixture image, run the MAC↔slot join +
Constellate sweep, push pinned adjacency (below), and the tree becomes the
self-running Game of Life. This doc is the Phase-2 recipe.

## Build plan (deferred, ~half a day when picked up)

1. **Exporter** (app or script): for each fixture, its 8 nearest by the same
   metric the twin's CA uses (hop-distance along rings beats raw euclidean for
   ring topology — reuse `field.ts` adjacency, don't reinvent).
2. **cambium builder + route** for type 24 (same pattern as the knock builder
   on branch `lighting/dev-mode-endpoints`): `neighbor_set(h, target, ids[≤8],
   persist, clear)` + `/debug/neighbors?mac=&ids=&persist=1`. Note packet.h
   line ~296 for the exact packed layout; entries are 3-byte short ids.
3. **Prereq: MAC↔slot join** (commissioning/Constellate) — neighbor ids are
   MAC-derived; pushing slot-geometry neighbors requires knowing which MAC
   hangs where. This is the same join that gates everything else.
4. **Verify**: pin two bench lanterns as mutual neighbors, seed one (tap or
   NB_PROGRAM_SET GH_CA), watch state propagate in the other's heartbeat
   (`caState`/choreo generation climbing) — provable over the air, no eyes
   needed. Mario+Luigi are the natural first pair.
5. **Persist** (bit0) only after the layout is final — pinned NVS adjacency on
   a re-hung lantern is a lie until re-pushed.

## Caveats saved with it

- RSSI mode is free to A/B against pinned on the real tree; Ben expects it
  marginal, our selfmap math agrees, but outdoors-on-tree is unmeasured.
- The 2x10 rig "color-ordering workflow" (host side of pinning) exists
  somewhere in Ben's bench tooling — find before rebuilding.
- 8-neighbor cap shapes the CA: rings of 24 mean pinning left/right ±2 along
  the ring + 2 vertical gives richer topology than raw kNN clumps.
