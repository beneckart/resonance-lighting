# ADR 0045: Auto-waking transport sleep and bounded RSSI survey

- Status: Accepted for the 2026 pre-playa field pack
- Date: 2026-08-17
- Decider: Ben Eckart
- Extends: ADR 0004, ADR 0023, ADR 0040, ADR 0044

## Context

The commissioned fixtures must spend roughly four days in a shipping container
between Nevada City pack-out and playa unload. Measured dark-but-awake draw is
roughly 126-144 mA per fixture. At 130 mA, 96 hours consumes 12.48 Ah before any
LED load: fatal to the 6 Ah tier and nearly the full nominal 15 Ah tier.

The existing timer-sleep packet carries only 16-bit seconds, so its maximum
18.2-hour sleep would still leave about 78 hours awake. The normal production
day cycle is also unsafe as a transport contract: dense fleet traffic extends
the radio hold, and loss of supply can eventually enter autonomous night show.
Physical ship mode is rejected because it would require opening lids or touching
reset after unload.

The temporary rigs also provide a unique opportunity to record pairwise RSSI
before packing. Bridge telemetry alone is a star centered on the bridge, not a
fixture-to-fixture matrix. The successful two-strongest-neighbor presence wipe
is encouraging evidence for local adjacency, but it does not prove RSSI is a
metric Euclidean distance.

## Decision

Add two append-only protocol operations:

1. `NB_TRANSPORT_SLEEP` (type 27) carries a 32-bit duration and target. The
   bridge command `Q<hours>` accepts 1-168 hours. A fixture cuts all loads and
   rails, enters timer deep sleep, and automatically boots when the timer
   expires. An RTC-retained transport latch then keeps LED output electrically
   dark while ESP-NOW and telemetry are live. A valid program command, including
   the bridge's bare `b` release, clears the latch. No lid-open reset is needed.
2. `NB_LOCATE_CONTROL` (type 28) starts or stops a bounded survey. The bridge
   command `L[seconds]` defaults to 120 seconds and caps at 900; `L0` stops.
   During that window only, each updated fixture retains up to 160 heard peers
   and reports its full fresh heard roster in 16-entry `NB_NEIGHBOR_REPORT`
   fragments about every 20 seconds with randomized phase. Normal operation
   emits no neighbor reports.

The first field capture records ranked per-peer RSSI EWMA observations, not the
censoring-corrected medians required for a production-quality path-loss solve.
Host tooling preserves each raw directed observation in canonical-shaped JSONL
so repeated observations can be aggregated and the sparse/missing-link behavior
can be assessed honestly offline.

## Energy margin

Use 1 mA as a deliberately conservative timer-sleep planning estimate until a
shipping-duration external-current measurement is recorded:

- 96 hours asleep: 0.096 Ah (1.6 percent of 6 Ah; 0.64 percent of 15 Ah).
- 12 hours awake-dark after scheduled wake: about 1.56 Ah at 130 mA.
- 96 hours awake-dark: about 12.48 Ah and is not acceptable.

Schedule the timer to expire shortly before expected unload. Waking early is
safe only because the retained latch prevents a light show; it still restarts
the roughly 130 mA radio load, so excessive margin is costly for the 6 Ah tier.

## Consequences

- Updated, reachable fixtures can ship rails-off and wake without physical
  access. Old firmware does not understand type 27 and must not be counted as
  transport-safe merely because the command was broadcast.
- A true power removal clears RTC state. A software/watchdog reset after timer
  wake retains the dark latch, which is the safer container behavior.
- The capture can approach a full directed matrix among heard devices, but only
  updated fixtures act as receivers/reporters. Old fixtures may still appear as
  transmitters when their beacons are heard.
- Full-roster reports average less than 0.4 packet/s per 100-neighbor reporter
  at the 20-second period. The bounded survey still adds traffic and therefore
  must end before transport sleep is issued.
- Antenna orientation, solar-panel shadowing, enclosure variation, receiver
  censoring, and device-specific offsets can warp global distance. The dataset
  is evidence for offline feasibility, not automatic installation coordinates.
