# Oakland lantern inventory follow-up, 2026-09-14

Ben found a separate bin, physically far from the shipping container, and
brought it to Oakland: five canopy lanterns and one batteryless uplight with
its panel connected. Two older prototypes are also in the backyard; Ben thinks
the perimeter prototype is disconnected or batteryless. This gives eight
physical units, but enclosure-to-MAC mapping is not complete.

## Comparison with September 12

None of the five reported identities was in the 106 OTA/storage targets.
Kairi and Wooper were two of the eight unobserved members of the September 1
114-ID comparison roster. They are now accounted for by the Oakland sighting.
Groot, Kiki and prototype 9F26F8 were outside that 114-ID baseline.
Their present radio activity is not evidence that September 12 ship mode failed.
The other physical lanterns have not been identified, so they may overlap the
stored 106. The Nevada City radio census did not establish container location.

The six still-unlocated IDs from the September 12 missing list are Gambit
F2BCF4, Gengar F2BDD4, Magmar F2BDFC, Donkey F2BE10, Milotic F2BE94 and Skitty
F3FD28. This updates present whereabouts; it does not rewrite the original
106 receipts or the eight-ID historical absence record.

## Passive controller snapshot

Exact connected USB identity was TSwift 979604 / 44:1B:F6:97:96:04 on COM157.
A 22-second serial read at approximately 09:18 PDT sent no commands and closed
the port afterwards. No dashboard, logger or maintenance campaign was left
running. TSwift reports channel 11 and tdeck-dev-local, matching its older
controller lineage. The September 12 storage4 flash was on primary 8EB508,
not TSwift; do not assume the new Store/calendar UI is on this handheld.

| Callsign | ID | Last reported battery | Report age at snapshot | Reported firmware |
|---|---|---:|---:|---|
| Groot | 9F2724 | 2.880 V | 11.1 min | fx-260829-b0ff5db-b |
| Kairi | 9F26B4 | 3.303 V | 1.6 min | fx-260829-af1d4ec-p |
| Kiki | F2BF5C | 3.277 V | 13.6 min | fx-260828-658b7d2-p |
| Wooper | 9F0E30 | 3.321 V | 18 s | fx-260830-7ae6502-p |
| P105 prototype | 9F26F8 | 2.992 V | 10.0 min | net-bench-2026-07-14.1 |

These are retained last-listen reports, not five simultaneous fresh heartbeats
or cell-terminal measurements. All ages are shorter than the current TSwift
uptime. Groot, Kairi and Kiki report FIELD/PROTECT with a last 900-second sleep;
Wooper reports FIELD/NORMAL and 120 seconds. All four fixture LED rails report
off. Prototype 9F26F8 uses the older net-bench field-cycle schema.
Groot's low voltage despite a 99% SOC report reinforces that SOC is not a
standalone remaining-capacity diagnosis. Current hardware/cell condition and
charging paths have not been physically checked in this session.

The July outdoor bench record identifies 9F26F8 as the P105/4 W RGBW prototype,
and 9F2690 as the replacement P126/HEX prototype. 9F2690 is a historical lead
for the other backyard unit, not a confirmed present physical identity.

Evidence: `ops/bench/data/ca/20260914-091805-oakland-passive/`, including raw
serial records, snapshot, reconciliation and SHA-256 manifest. No firmware,
sleep, ship, reboot, NVS or charging-setting changes were made.

## Next service items

- Map the six newly found lantern enclosures and both prototypes to exact IDs.
- Check Groot's actual cell and charge path first given its 2.880 V report;
  inspect the old P105 prototype's 2.992 V report and present power setup.
- Treat Kairi/Wooper's older maintenance-discovery issues as service history,
  not evidence of dead cells; both now have radio reports above 3.3 V.
- If updating or storing these Oakland units, prepare exact targets and artifact
  identity for this local batch. Batteryless solar-only uplight power has not
  been established as a stable OTA/reboot supply.
