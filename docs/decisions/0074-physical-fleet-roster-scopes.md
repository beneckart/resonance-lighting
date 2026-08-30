# 0074 -- Separate physical fleet membership from operational scope

**Status:** Accepted, 2026-08-30

## Context

The canonical electronics registry grew to include commissioned spares, bench
fixtures, demonstrations, protected one-offs, and retired or quarantined boards.
Bridge OS previously treated commissioning status as fleet membership. That made
Health and RF expect devices that were never part of the installed artwork and
made permanent callsigns look disposable when a fixture left service.

The authoritative 2026 physical build is 118 fixtures:

- 74 canopy/downlight fixtures;
- 24 perimeter fixtures; and
- 20 trunk/uplight fixtures.

Of those, 111 are expected at the art site. Four uplights are at camp: Yuffie
`9F0E4C`, Zidane `F2BEF4`, Psyduck `F3FC8C`, and Cream `F3FD50`. Three canopy
fixtures are in repair/offline scope: Olimar `F2BE8C`, Shuckle `F4031C`, and
Tidus `F40424`.

## Decision

`ops/fleet/roster.csv` is the authoritative physical-fleet roster. Each row has
one stable short MAC, one physical fixture type, and one operational scope:

- `site`: expected in art-site Health and RF accounting;
- `camp`: built fleet inventory intentionally offsite; or
- `repair`: built fleet inventory intentionally offline or unsafe.

`ops/fleet/registry.csv` remains the electronics and commissioning ledger.
`ops/fleet/callsigns.csv` remains the permanent operator-identity table. A
callsign is not deleted or reassigned when its fixture moves between site, camp,
repair, quarantine, or retirement.

Bridge OS generates its complete 118-entry Fleet inventory by joining all three
files. Health and RF use only the 111 `site` rows as their expected cohort. Fleet
keeps all 118 rows and exposes site/camp/repair filtering; camp and repair rows
do not become false off-air alarms. Fresh devices outside the physical roster
remain visible as foreign live peers.

## Consequences

- Commissioning a spare no longer silently enlarges the installed-fleet health
  denominator.
- Moving a built fixture offsite is a roster-scope edit, not an identity change.
- Repair fixtures remain discoverable by callsign and visible in Fleet without
  appearing broken in Health or RF.
- The checked-in generated header and native tests pin 118 total, 111/4/3 scope,
  and 74/24/20 type counts.
- Any future build or fleet flash must regenerate Bridge OS from these exact CSV
  inputs. This decision made no embedded build and changed no hardware.
