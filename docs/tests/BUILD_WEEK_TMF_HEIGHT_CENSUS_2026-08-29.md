# Build-week TMF installed-height census -- 2026-08-29 UTC

## Purpose

Determine whether live canopy TMF8820 sensors could distinguish close fixtures
still in shipping bins from fixtures already installed high in the tree, and
use the field geometry to tune canopy presence.

## Method

- Declared one operator on T-Deck bridge `979604`, mesh channel 11.
- Selected all 61 dashboard peers reporting downlight class plus TMF sensor bit.
- Began exact maintenance campaign `207B660F`; no OTA, reboot, persistence, NVS,
  profile, or firmware write was requested.
- Discovered shared-WiFi endpoints for 150 seconds across `192.168.1.0/24` and
  fallback `192.168.4.0/24`.
- Froze the campaign before sampling.
- Recorded six `/telemetry` reads per discovered fixture and then requested
  `/resume` on every endpoint.
- Confirmed all sampled fixtures later returned to mesh with fresh heartbeats.

The immutable JSONL evidence is
`ops/bench/data/Black Rock City/2026-08-29-tmf-installed-height-census-0218UTC.jsonl`.

## Results

- Selected: 61 exact downlights.
- Discovered and sampled: 46.
- Not discovered in the bounded window: 15; these are unsampled, not sensor
  failures.
- Healthy close-return group: 42 fixtures, raw median depth 145-575 mm. Most
  were highly stable across six reads; Robin and Midna had intermittent empty
  frames.
- Clear/no-return group: Panther `9E5A84`, Gible `9E5B34`, Sakura `F2BE0C`, and
  Leia `F40384`, each with raw depth `0,0,0,0,0,0`, TMF read OK, zero read
  errors, zero recoveries, and zero sensor-domain resets.
- No sampled fixture reported a multi-metre value because deployed firmware
  discarded all returns above 2,500 mm before telemetry.
- The census tool initially printed its retained filtered value even when the
  current raw sample was zero. Evidence analysis used `tof_depth_mm` as the
  validity authority; the tool is corrected for future runs.

## Interpretation

The four all-zero nodes are the strongest installed-height candidates, but the
census alone cannot prove physical location. A zero means no confident target
inside the deployed firmware's accepted 80-2,500 mm range, not a literal zero
distance. At the reported roughly 15 ft mounting height, ground is about
4,570 mm away and a standing head is commonly about 2,700-3,200 mm away. Both
were outside the deployed software window even though the TMF8820 hardware is
specified through 5,000 mm.

ADR 0070 records the resulting role-specific change. Perimeter keeps its close
chest-height distance mapping. Canopy accepts the full 5 m sensor range and uses
learned per-zone background delta plus a persistent presence latch.
