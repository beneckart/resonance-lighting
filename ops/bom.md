# BOM - Working Procurement Skeleton

**Status:** Current working BOM shape, 2026-06-17. This is not a buy sheet. Prices, lead
times, and exact SKUs must be re-verified before procurement. The old ESP32-C3 + CN3058 +
AP2112K first-pass BOM is superseded by the PowerFeather V2 feasibility results.

## BOM Strategy

The 2026 production decision is still COTS vs custom vs hybrid, but all paths now derive
from the same measured architecture:

- PowerFeather V2 or PowerFeather-derived controller/power board.
- One larger LiFePO4 cell per fixture, likely 32700 class if hat geometry allows.
- Role-specific direct-GPIO LED modules:
  - HEX for close-range animation / glow.
  - 4 W RGBW point source for crisp long-throw gobos.
- Role-specific solar panel may be rational:
  - P126-class 2 W ETFE for lower-power HEX fixtures if the budget closes.
  - P105-class 5 W ETFE for RGBW point-source fixtures or margin-heavy placements.

Avoid per-unit skilled operations: no hand-soldering 100 header sets, no hand-crimping
100 harnesses, no per-unit pairing/config rituals.

## Track A - COTS / Hybrid Production BOM

| Item | Function | Current direction | Notes |
|---|---|---|---|
| PowerFeather V2 | MCU, charger, gauge, buck-boost, telemetry, USB | Leading COTS/reference board | ADR 0021 go decision. Check supply and connector assembly options before 100+ buy. |
| LiFePO4 cell | Energy storage | One large cell; 32700 6 Ah candidate leading | One sample measured 5726 mAh. Spot-check more before bulk. |
| Solar panel, HEX role | Daily harvest | Voltaic P126 2 W ETFE candidate | Mechanically elegant. Use only if HEX role budget closes. |
| Solar panel, RGBW role | Daily harvest / storm margin | Voltaic P105 5 W ETFE candidate | Larger, mounting holes, better margin. |
| HEX LED module | Close-range animation / glow | SK6812 direct-GPIO HEX | 4.2 V boost under test; cap all-pixel full-white modes. |
| RGBW point-source LED | Crisp long-throw gobo | 4 W RGBW direct-GPIO | Needs role-specific current budget and thermal/mechanical placement. |
| LED adapter PCB | Connectorization / rail option | NeoHEX passive Rev A now; future boosted adapter possible | Use keyed/polarized connectors and production-safe power path. |
| Panel lead / VDC connector | Solar input | Pre-crimped or factory-installed pigtail | Strain relief required; do not rely on bare soldered panel wires. |
| Battery lead / holder | Cell retention/service | Holder or spot-welded lead with keyed connector | Must survive vibration/heat; avoid fragile spring/contact assumptions. |
| Hat enclosure | Sealed electronics + panel mount | Steve design, likely MJF nylon for production | Must respect antenna keep-out and thermal constraints. |
| Gobo/filter | Patterned aperture | Steve printed cone/flat variants | Role-specific optical test photos still needed. |
| Fasteners/standoffs | Mechanical retention | Off-the-shelf | Include set screws, board standoffs, panel backup retention. |
| Flashing jig/cable | Production QA/recovery | USB-C or pogo | Needed even with OTA. |

## Track B - Custom Board Candidate Blocks

Use these only if the COTS/connector path fails cost, assembly, packaging, or availability.

| Block | Current reference | Notes |
|---|---|---|
| MCU/RF | ESP32-S3-WROOM-class module | Pre-certified module, PCB antenna, strict keep-out. |
| Charger/power path | BQ25628E-class | Must set VBUS_OVP=1 and implement HIZ requalification guard. |
| Fuel gauge | MAX17260-class | Treat LFP SOC as advisory until learned; expose raw current/voltage. |
| 3.3 V regulator | TPS631013-class buck-boost | LFP plateau sits near crossover at light loads; measure real efficiency. |
| LED rail | Switchable/default-off rail or boost with EN | HEX boost candidate: 4.2 V, not 5 V, unless level shifting is added. |
| Connectors | Keyed solar, battery, LED, test pads | Production operations matter more than schematic elegance. |
| Temperature | Battery NTC / charger TS strategy | Needed for sealed-hat LFP charge-temperature limits. |

## Explicitly Superseded

Do not use this as the current production BOM:

```
ESP32-C3-MINI-1 + CN3058 + AP2112K + always-live direct-Vbat WS2812B
```

That old sketch is useful history only. Later ADRs and bench data moved the project to the
PowerFeather V2 reference architecture, switchable rails, telemetry, direct-GPIO LED roles,
and a measured panel/cell sizing campaign.

## Open Procurement Inputs

- PowerFeather V2 supply/cost at 100-150 units, including factory connector options.
- Voltaic P105/P126 real outdoor harvest after firmware OVP/HIZ guard.
- HEX/RGBW type mix and placement by tree height / sightline.
- Battery sample count beyond the single passing 32700 capacity run.
- Hat envelope: panel size, battery retention, antenna keep-out, and thermal result.
- Custom vs COTS go/no-go date based on actual lead times.
- Cost decomposition for `INV_2026_00401`, still needed as a comparison baseline.

## Costing Guidance

Do not publish a precise per-fixture total until the architecture mix is chosen. The big
drivers are now:

- controller path: COTS PowerFeather vs custom assembly;
- solar panel role: P126-class vs P105-class;
- battery format and sourcing;
- LED role mix and any HEX boost adapter;
- hat production method and panel retention;
- labor removed by factory soldering / pre-crimped harnesses.

For each candidate BOM, compute both dollars and operations:

```
total_cost = parts + shipping + spares + assembly labor + QA/rework allowance
ops_risk = solder joints + crimps + one-off configs + fragile connectors + field access
```

The winning BOM is the one that closes energy and reliability while keeping 100-unit
assembly boring.
