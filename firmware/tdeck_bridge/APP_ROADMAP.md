# Bridge OS app roadmap

**Date:** 2026-08-24

**Status:** Working priority order for the T-Deck Plus app platform. This
supplements ADR 0047 and ADR 0048. The RTC/GPS dusk-to-dawn work is a separate
parallel workstream; this plan consumes its eventual read-only time-service seam
but does not change or duplicate it.

## Current baseline

Bridge OS is no longer a proposed handheld. M0-M4 are hardware-verified and the
launcher has thirteen tiles. Health and Schedule are integrated and flashed;
Patterns v1 and RF Diagnostics are integrated and flashed on `8EB508`. Locate
is the only remaining literal placeholder. Detailed Sensors work follows Health
when the packet contract can report measurements that heartbeats do not contain.

| App | State | Important remaining work |
|---|---|---|
| Claude | Working, hardware-verified | Census examines all 192 tracked slots and returns explicit 24-row pages; command authentication remains open. |
| Fleet | Working, hardware-verified | The visible table is capped at 64 peers; add paging/filtering before calling it a complete fleet view. |
| Health | Flashed on `8EB508`; broad physical smoke passed | Source roster covers 144 production fixtures. The last flash embeds 141 and treats the three restored IDs as foreign until rebuilt; complete that refresh plus the explicit color/off-air and memory matrix. |
| LED Studio | Working; field-smoke-tested | Ben reports the controls behaved as designed on 2026-08-23/24. The named HEX/RGBW color, class, blink, stop, and fleet-airtime matrix remains open. |
| Sleep / Dark | Working; field-smoke-tested | Ben reports both controls behaved as designed. Dark expiry and rails-off sleep/rejoin still need named-canary validation. |
| Knocker | Three fleet modes built; pending fixture/T-Deck hardware validation | Retains the deterministic 192-entry targeted roll and adds immediate multicast plus a shared +1.0 s multicast deadline. Every fixture rechecks its local strike gates at fire time. |
| CA Studio | Working; field-smoke-tested | Ben reports the controls behaved as designed. Same-program parameter changes still use a visible release/re-lease workaround. |
| Settings | Working | Secrets remain serial-only by design. |
| SunTest | Working diagnostic | Its direct-sun purpose is complete; retain as a service diagnostic. |
| Schedule | Flashed on `8EB508`; broad physical smoke passed | GPS publishes UTC quality; fixtures select bounded consensus time and apply Black Rock City civil twilight in field profile. Complete the explicit override/canary matrix. |
| Patterns | Flashed on `8EB508`; hardware check pending | Hardware-check the four modes, five palettes, class/cohort filters, owner handoff, and stop behavior on named canaries. ES7210 audio is v2. |
| Sensors Health | First slice implemented as Health | Battery/on-air triage plus existing heartbeat detail is implemented; detailed sensor samples still need `NB_SENSOR_REPORT`. |
| Locate | Placeholder | Survey packets exist, but the T-Deck does not retain/model neighbor reports for a UI. |
| RF | Flashed on `8EB508`; hardware check pending | Hardware-check both pages, counts, selection, ranking, frame tail, and channel-guard labels. |

## Priority rules

Rank work by:

1. Field usefulness before the 2026 event.
2. Honest fleet-scale behavior across about 130 fixtures.
3. Small blast radius and no unnecessary wire-format changes.
4. Ability to develop and native-test the feature in an isolated module.
5. Artistic value after the operator and safety baseline is trustworthy.

## P0 - close and harden the shared baseline

Do this before accepting more fleet-command surfaces. Read-only app development
may proceed in parallel after there is a clean common checkpoint.

1. Run ADR 0048's named-canary checks for LED Studio and Sleep / Dark on one HEX
   and one point-source fixture. Also physically exercise Knocker and CA Studio.
2. Fix misleading full-fleet bounds. In particular, `knock all` must not silently
   stop at 32 fixtures. Fleet and Claude may remain intentionally paged/bounded,
   but their UI/tool results must say so clearly.
3. Serialize mesh transmission across the UI, Claude, stream, and time tasks.
   Keeping packet emission in one translation unit does not alone protect packet
   sequence/order across callers.
4. Replace raw cross-task census reads with locked accessors. The Claude tool
   path currently reaches a census API documented as loop-context-only.
5. Extract and native-test direct-frame wave planning: full 192-peer selection,
   18-entry packet chunking, freshness and class filters, dim scaling, blink
   edges, and owner replacement.
6. Measure T-Deck runtime with continuous census at normal backlight and with the
   screen off. Do not design later diagnostics around an assumed all-night run.
7. Close authenticated/authorized mesh commands before treating the handheld as
   a trusted event controller. Until then, keep the present limited tool surface
   and treat physical possession as an operational mitigation, not a protocol
   security property.
8. Checkpoint the current LED/Sleep/cache and RTC/GPS work before creating app
   worktrees. Branching from the current HEAD would omit important uncommitted
   baseline work.

Field update, 2026-08-24: Ben exercised LED Studio, Sleep / Dark, Knocker, and
CA Studio the prior night and reports that all four behaved as designed. This
closes the broad functional smoke pass, not the detailed named-canary matrix.
The exercise exposed that the old `knock all` implementation selected at most
32 fresh fixtures in heartbeat order and dispatched one targeted request every
300 ms. The current P0 source instead plans all fresh IDs (up to 192), sorts by
short ID, dispatches at an explicit 80 ms cadence, and labels the action a
targeted roll. Hardware revalidation of that revision remains open. True
synchronized fire was initially held for P3 behind the RTC/GPS and fixture
event seams.

Knocker update, 2026-08-24: those event seams are now implemented without a
wire-layout change. The picker offers the existing deterministic targeted roll,
one immediate fleet multicast, and one shared +1.0 s fleet deadline. Both new
modes use a repeated `NB_EVENT_SOLENOID_STRIKE` with a 32-bit dedupe ID. Later
copies decrement `fire_in_ms` toward the same bridge deadline, and fixtures use
radio-callback receipt time, arm only one pending event, ignore duplicates, and
drop a strike more than 250 ms late. All lifecycle, power, solenoid arm/rest,
and failsafe gates are re-evaluated when firing. Native suites and both embedded
development builds pass; isolated hardware timing remains open.

Integration update, 2026-08-24: items 2-5 and 8 are implemented in the common
baseline. The launcher now registers callbacks instead of comparing tile names.
Runtime measurement, named-canary validation, and authenticated commands remain
open P0 work.

## P1 - first parallel app batch

These are the best independent development silos after the P0 platform seam is
stable. They may be built concurrently.

### A. RF Diagnostics

First choice if only one lane is available. It is read-only, useful during
installation, and needs no fixture or packet change.

Initial scope:

- live/seen/unobserved counts and mesh-silent state;
- strongest and weakest fixtures by RSSI/PDR/age;
- receive-ring drops and mesh send success/failure;
- recent frame type/source/RSSI tail;
- WiFi/AP channel and channel-guard state; and
- serial export compatible with the existing host dashboard/log tools.

Implementation update, 2026-08-24: the read-only app and pure RF model are
integrated. It reports the production-roster and foreign-peer distinction,
labels unavailable/partial observation coverage, ranks fresh peers
deterministically, surfaces the existing counters and guard state, and exposes
the safe valid-frame tail on a second page. Native tests and the combined
embedded build pass; physical UI validation remains open.

### B. Sensors Health

Build the honest subset available from full heartbeats now: sensor-presence bits,
reported class, class mismatch, freshness, battery/power tier, environment fields
when present, and firmware identity. Label unavailable live measurements as
unavailable rather than inventing them.

Start as a Fleet health/detail view unless a dedicated tile clearly earns its
launcher space. The Schedule tile belongs to the concurrent RTC/GPS workstream.

Detailed tilt, ToF depth/zones, and sensor error counters remain a later phase
behind `NB_SENSOR_REPORT`.

Field-source update, 2026-08-24: the first slice now has a dedicated **Health**
tile. It keeps 141 commissioned/commission-failed production registry entries
in stable short-ID order, colors fresh nodes from raw VBAT at 3.20/3.10 V, greys
off-air nodes, and appends unexpected live IDs. The pure merge/band model and
registry-generation contract pass native tests. The final merged image is now
flashed on T-Deck `8EB508`; hardware layout, touch, trackball, detail, and
heap-watermark checks remain before acceptance.

### C. Patterns v1 - deterministic and manual

Highest new artistic value. Keep v1 independent of the microphone and RTC/GPS.
Put pattern math in a pure core module and expose palette, speed, intensity,
class/cohort selection, start, and stop.

Use fields that fixtures honor today, or emit final RGBW direct frames. Do not
base v1 on `NbShowFrame.bright`, `beat_phase`, or `energy` until `ProgBridge`
actually consumes them. Preserve the single active stream owner and the normal
three-second stale fallback.

Implementation update, 2026-08-24: v1 is integrated with Wash, Chase, Wave,
and Twinkle; five palettes; speed and intensity; class and stable short-ID
cohort filters; and final per-fixture RGBW planning. Patterns and LED Studio
replace one another through the shared stream owner. The pure model passes 190
checks and the combined embedded build passes. Named-canary hardware validation
remains open; microphone capture remains v2.

## P2 - active survey, audio, and existing-app completion

1. **Locate / active RSSI survey:** add a bounded `NB_LOCATE_CONTROL` sender,
   retain decoded `NB_NEIGHBOR_REPORT` fragments in a native-tested model, and
   show survey progress/quality. Keep the actual coordinate solve host-side
   initially; RSSI topology is evidence, not a defensible map by itself.
2. **Patterns v2 / ES7210 ambient audio:** add and hardware-qualify the mic/I2S
   HAL, then feed the deterministic pattern engine. A T-Deck stream must defer
   when a PUCA or CoreS3 performance-audio publisher is already active.
3. **Fleet completion:** paging/filtering/search, explicit truncation labels,
   and faster access to identify and health views.
4. **CA Studio completion:** remove the release/re-lease blip after the fixture
   runtime correctly reapplies parameters to the active program.

## P3 - dependency-bound features

- **Full Sensors:** requires a new append-only `NB_SENSOR_REPORT` contract plus a
  fixture producer and rate/window policy.
- **Synchronized Knocker hardware acceptance:** source now uses the canonical
  `NbEvent.fire_in_ms` seam rather than a second time model. Measure immediate
  multicast spread and +1.0 s deadline skew on an isolated, explicitly armed
  cohort before any fleet promotion.
- **CA-to-strike choreography:** requires a clamped fixture `ProgramOutputs`
  request routed through the normal power/lifecycle strike permission.
- **Voice:** typed Claude already works. Laptop `whisperd` requires an explicit
  STT transport and failure contract; cloud STT should sit behind the same seam.

## P4 - later portability and experiments

- Cardputer ADV port behind the existing display/input HAL.
- General launcher/theme polish after the field apps settle.
- SX1262 LoRa experiments only under a separate decision; LoRa is not the fleet
  link and is out of current scope.

## Parallel development contract

After one clean baseline checkpoint, use isolated worktrees/branches with these
ownership boundaries:

- RF lane: new `app_rf.*` and pure RF view/model tests.
- Sensors lane: `app_health.*`, pure health-model tests, and later
  `app_sensors.*` work behind the existing heartbeat/snapshot boundary.
- Patterns lane: new `app_patterns.*`, pure pattern core, and explicitly owned
  changes to the stream service.
- Integration lane only: callback-based launcher registration,
  `tdeck_bridge.ino`, shared
  service wiring, version identity, README/ADR/LOG/TODO, retained builds, and
  USB flashing.

The integration lane owns collision hotspots including `ui_task.cpp`,
`mesh_tx.cpp`, `census_svc.cpp`, and `stream_svc.cpp` unless a hotspot is
explicitly handed to one feature lane. All LVGL calls stay on the UI task.
Every feature lane runs native tests in its own worktree. Final retained builds
and USB flashes remain single-operator actions against an explicit T-Deck MAC.

## RTC/GPS boundary

The T-Deck may display time quality or use the completed service for scheduled
commands later. It must not become the production dusk/dawn authority: a show
clock that depends on someone carrying the handheld is not a production clock.
The separate sparse GPS/RTC anchor work remains authoritative per ADR 0031.

## Documentation precedence

For Bridge OS, read in this order:

1. `firmware/tdeck_bridge/README.md`
2. this roadmap
3. ADR 0048
4. ADR 0047
5. ADR 0037 for constraints not superseded by ADR 0047

Older statements in the root `TODO.md` and ADR 0037 that say no bridge firmware
exists or ask whether to build it are historical and no longer describe current
state.
