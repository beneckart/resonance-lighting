# Autonomous distributed choreography and fleet timing -- production firmware concept

**Date:** 2026-07-13
**Status:** EXPLORATORY DESIGN NOTE -- strong candidate direction, not an ADR or locked design
**Owners:** Ben + Codex discussion; artistic and production review still pending

## Why this note exists

The production fixtures need to feel like one artwork without depending on a permanently
installed master, bridge, or infrastructure network. The normal daily lightshow now
does depend on distributed wall time from sparse GPS/GNSS and external-RTC anchors
(ADR 0031), but not on one coordinator or continuing Starlink. At the same time, the
desired behavior is broader than a single cellular-automata (CA) show:

- autonomous light behavior after dusk;
- presence-modulated local and spatial effects;
- occasional synchronized preprogrammed light sequences;
- daytime solenoid knocks that can sound together or travel as a ripple;
- short autonomous solenoid compositions when energy permits;
- bridge-directed performances when a bridge is intentionally present;
- identification, photogrammetry, registration, diagnostics, and OTA maintenance;
- continued operation when nodes, links, sensors, or the bridge are missing;
- one common production firmware image with runtime capability and placement data,
  preserving software fungibility.

This note lays out a coherent candidate architecture that supports all of those goals.
It is intentionally more detailed than the current implementation, but none of the
packet formats, timing values, quorum rules, show types, or solenoid power arrangements
below are decisions. They are a design space and a proposed sequence of experiments.

The main design shift is:

> Treat CA as one program family inside a distributed choreography runtime, not as the
> top-level firmware architecture.

## Decision update -- 2026-07-26

ADR 0031 accepts a deterministic scheduled dusk/dawn lightshow as the production
direction. A subset of fixtures will carry onboard-antenna GPS/GNSS receivers for
absolute UTC and another subset will carry battery-backed external RTCs for holdover.
They distribute time quality over ESP-NOW; all 150 fixtures do not need their own RTC.

This supersedes this note's earlier assumption that wall time was unnecessary for the
daily lifecycle and demotes distributed dusk inference from the production trigger to
telemetry, sanity checking, and an explicitly designed degraded-mode option. The
coordinator-free logical-phase work remains relevant for tight choreography after the
scheduled show begins.

## Working design intent

The candidate production design follows these principles:

1. **Autonomous by default.** The installed tree produces its normal day and night
   behavior with no bridge present.
2. **No permanent coordinator.** Any fixture may originate an observation or event.
   Peer exchange aligns the fleet well enough for the artistic effect.
3. **Bridge as an optional leased participant.** A bridge may temporarily inject DJ
   modulation, select a show, synchronize a special moment, run registration, or open
   maintenance. Its disappearance must not stop the autonomous artwork.
4. **Scheduled wall time plus logical show phase.** Sparse GPS/RTC anchors establish
   the daily UTC schedule; most choreography after turn-on still uses shared phase and
   future event scheduling rather than repeatedly consulting civil time.
5. **One firmware image.** Hardware capabilities, fixture class, position, topology,
   and artistic role are runtime data rather than separately compiled personalities.
6. **Local power veto.** No mesh message or show program may force an unhealthy fixture
   to run LEDs, sensors, radio, or a solenoid.
7. **Graceful degradation is part of the artwork.** Partitions, missed packets, absent
   nodes, and low-energy fixtures should reduce or localize an effect rather than halt it.
8. **Assembly simplicity wins when performance is adequate.** Extra power paths,
   connectors, capacitors, per-unit soldering, and special harnesses need a measured
   artistic or reliability benefit to justify production complexity.

## System model

The expected fleet is roughly 150 PowerFeather V2 fixtures across multiple physical
classes. Classes differ in panel, LED, sensor, and possibly solenoid fit, but share the
same basic controller and ESP-NOW control plane. The production firmware should be able
to discover or load a capability record such as:

```text
fixture_id
board_type
fixture_class
led_type and pixel/emitter count
solenoid_present
sensor_set
panel_class
battery_class
position/topology record version
firmware and protocol versions
```

The design assumes no hostile network participants. It still must handle ordinary
distributed-system failures: packet loss, duplicate delivery, stale state, asymmetric
links, sleeping receivers, rebooted nodes, partitions, and differing local observations.

## Candidate architecture

```text
 local sensors       solar/power state       peer packets       optional bridge
      |                     |                     |                    |
      +---------------------+---------------------+--------------------+
                                      |
                           observation/event fabric
                     (dedupe, freshness, relay, scheduling)
                                      |
             +------------------------+------------------------+
             |                        |                        |
        fleet phase             trigger/rule engine      capability/site map
             |                        |                        |
             +------------------------+------------------------+
                                      |
                          choreography program runtime
              (CA, timelines, ripples, easter eggs, DJ mode)
                                      |
                  +-------------------+-------------------+
                  |                                       |
              LED renderer                         solenoid sequencer
                  |                                       |
                  +-------------------+-------------------+
                                      |
                               local power arbiter
                  (brightness, strike, sensor, radio vetoes)
```

The power arbiter is deliberately below every artistic path. An effect requests an
action; the local power policy decides whether and how strongly that fixture may
participate. ADR 0023 voltage/coulomb policy and the POR-safe rail sequence remain
authoritative even during a bridge-directed performance.

## Time model: three different needs

It is useful to distinguish three kinds of time instead of asking one clock to solve
every problem.

### 1. Wall time

Wall time means date, timezone, and civil time. It is useful for logs, named schedules,
or a bridge operator asking for a show at a known time. Per ADR 0031 it is now the
primary production authority for daily show start/stop. CA ticks, synchronized knocks,
and scene rendering still use monotonic/logical time once a show is active.

Wall time comes from a redundant sparse set of GPS/GNSS and battery-backed external-RTC
anchors, not from a permanent bridge and not from an RTC on every fixture. Starlink
during build week may seed and verify time, but the installed fleet must remain
autonomous after it disappears. Store show boundaries as UTC instants so timezone and
DST conversion is performed before deployment.

### 2. Local monotonic time

While awake, each ESP32-S3 has an accurate high-resolution monotonic clock. Espressif
documents the APB-clock deviation as less than +/-10 ppm, or less than about 0.864
seconds/day. That is more than adequate to render a show for hours, especially when
neighbor packets continuously correct relative phase.

### 3. Fleet logical phase

Fleet logical phase answers questions such as:

- when is the next daytime rendezvous window?
- which CA generation is current?
- execute this event two seconds from now;
- how old is this presence observation?
- has this trigger already run in the current epoch?

It does not need to equal Unix time. It needs to be monotonic within a boot/session,
approximately aligned among peers, explicitly invalidated or reacquired after a POR,
and good enough to schedule an event after it has propagated.

## What the current sleep clock actually does

The current Arduino-ESP32 3.3.7 build uses `CONFIG_RTC_CLK_SRC_INT_RC`, the default
internal low-frequency RC oscillator. It is power efficient but temperature sensitive.
The PowerFeather V2 schematic does not show a fitted 32.768 kHz crystal.

The outdoor July field log provides a useful, if non-laboratory, measurement. Sleep
duration was inferred from boot-origin changes and the previous boot's awake uptime;
adjacent host rows could not be used because the bridge repeats stale peer heartbeats
while a peer sleeps.

| Device / requested sleep | Samples | Median wake error | Equivalent daily gain |
|---|---:|---:|---:|
| original P126 `9E5B0C`, 300 s | 205 | -1.801 s | about 8.65 min/day fast |
| replacement P126 `9F2690`, 300 s | 31 | -2.861 s | about 13.74 min/day fast |
| P105 `9F26F8`, 300 s | 414 | -2.261 s | about 10.86 min/day fast |
| P105 `9F26F8`, 900 s | 67 | -9.224 s | about 14.76 min/day fast |

The measurement includes logger and boot-boundary uncertainty, but the 900-second data
confirms an error on the order of 0.6-1.0 percent under those outdoor conditions. A
fixture free-running for a week could therefore be ahead by roughly one to two hours,
and two fixtures need not drift by the same amount. Temperature changes on playa may
change the error again.

The RTC timer survives deep sleep and most resets, but not a true power-on reset. A
battery-protection disconnect or other full power loss therefore destroys any retained
absolute time or phase regardless of oscillator quality.

### Candidate sleep-clock experiment

ESP32-S3 also supports an internal 8.5-17.5 MHz oscillator, depending on chip, divided
by 256. Espressif describes it as more stable than the default low-frequency RC source
at a cost of about 5 uA additional deep-sleep current. The installed Arduino build names
the option `CONFIG_RTC_CLK_SRC_INT_8MD256`.

That cost is:

```text
5 uA * 24 h = 0.12 mAh/day
0.12 mAh/day / 5500 mAh = about 0.0022 percent of usable pack capacity/day
```

For comparison, the current field-cycle bench artifact wakes for about 8 seconds every
300 seconds. That is 38.4 radio-active minutes/day. At an illustrative 100-168 mA while
awake, it costs roughly 64-108 mAh/day. The optional oscillator cost is hundreds of
times smaller. Production firmware should shorten and adapt those radio windows, but
the relative conclusion remains: 5 uA is not a meaningful energy objection.

The candidate action is to A/B the alternative clock source, not assume it:

- verify that the Arduino build can select it reproducibly without a fragile custom
  toolchain;
- measure deep-sleep current on actual PowerFeather V2 hardware;
- measure 300-second and longer sleep errors across indoor, outdoor, hot-sun, and cool
  night conditions;
- confirm radio, charger, gauge, and wake behavior are unchanged;
- use it if the stability improvement is real and the integration remains boring.

Even if adopted, fleet correctness must not depend on it. It improves holdover and
rendezvous efficiency; peer resynchronization and POR recovery remain required.

## Coordinator-free fleet alignment

A permanent leader still conflicts with autonomy and fungibility. Sparse GPS/RTC
anchors do not create one: they are replicated capabilities, any recent high-quality
source may teach time, and loss of one anchor must not stop the fleet. Wall-time
distribution establishes the daily schedule; the candidate model below aligns the
logical phase used inside a show.

The candidate model is a peer-corrected logical phase:

1. Each awake packet includes the sender's logical phase, phase-quality estimate, boot
   or session ID, and sequence number.
2. A receiver compares the phase of several recent peers with its own.
3. It gently slews toward a robust neighborhood estimate rather than jumping backward.
4. Regular traffic continually removes awake-clock error during the show.
5. The sleep oscillator holds the phase through daytime naps.
6. A node that reports a new POR/session has low phase quality until it hears peers.

The exact estimator is open. Candidates include a neighborhood median of phase error,
a pulse-coupled/firefly-style synchronizer, or a small phase-locked-loop model. Circular
phase math, asymmetric links, and stale samples need native tests before hardware work.

This is not Byzantine or formal distributed consensus. There are no adversaries, and
the artwork does not need every node to have the same bit at the same microsecond. The
goal is convergent evidence and bounded artistic skew under ordinary loss and sleep.

### Bootstrap and reacquisition

Randomly phased fixtures cannot be assumed to discover one another if every device
wakes for a very short, permanently offset window. Bootstrap therefore needs an
acquisition policy:

- after POR or first installation, use randomized/frequent or occasional extended
  listen windows;
- when solar surplus is strong, spend more freely on acquisition;
- once phase quality is high, shrink to narrow scheduled rendezvous windows;
- increase listening again as local evidence approaches twilight;
- preserve a randomized backoff so 150 aligned transmitters do not all collide;
- let any synchronized peer teach a newly replaced fixture the current phase.

The dense fleet should help acquisition, but that is a hypothesis to validate rather
than a guarantee.

## Event fabric

Immediate action on packet receipt produces network- and task-jitter-dependent effects.
Instead, events that should align are announced early and executed at a future logical
time.

A candidate event envelope is:

```text
protocol_version
event_type
event_id
origin_id
origin_boot_id
origin_sequence
observed_or_created_time
scheduled_start_time
expiry_time or max_age
hop_limit
program_id
program_version
seed
small parameter payload
energy_class
```

Required properties:

- **Idempotent:** duplicates do not repeat an event.
- **Fresh:** stale events expire and cannot replay after a reboot.
- **Relayable:** peers may forward show-control events even though firmware images are
  never mesh-gossiped.
- **Future scheduled:** lead time covers propagation and local preparation.
- **Locally vetoable:** the power arbiter may decline or reduce participation.
- **Versioned:** an unknown program or packet fails closed without breaking the normal
  autonomous show.

For deterministic autonomous rules, several nodes may independently discover the same
trigger. An event ID derived from the rule and a coarse fleet epoch can collapse those
discoveries into one event:

```text
event_id = hash(rule_id, site_id, fleet_epoch_bucket)
```

The first valid copy supplies the future start; later duplicates reinforce propagation
but do not retrigger it. The details need care when nodes have different phase estimates.

### Observations are not conclusions

Peers should share original, timestamped observations rather than recursively repeating
only a conclusion such as `DUSK=true`. Otherwise one shaded or faulty node can create a
self-amplifying rumor.

A compact dusk observation might be:

```text
origin_id
observation_sequence
solar_confidence
panel/input features
optional lux confidence
observation_age
```

Each receiver counts distinct recent origins. The same principle applies to presence:
share a fresh presence observation from a known location/sector, then let the trigger
engine evaluate the rule.

## Candidate daily lifecycle

The production lifecycle can be thought of as states with adaptive radio duty rather
than the field-cycle sketch's fixed eight-second/five-minute bench behavior.

### DAY_SLEEP

- LEDs off and their rail safely parked.
- Solenoid gate LOW.
- Long deep sleeps with brief telemetry, solar, phase, and event windows.
- Radio duty adapts to energy surplus and phase quality.
- A scheduled daytime knock window may justify one aligned rendezvous.
- Local power and reset telemetry remain available for later health collection.

### DAY_RENDEZVOUS

- Wake slightly before the nominal event/telemetry phase.
- Listen, exchange phase and health summaries, and relay pending events.
- Optionally charge/precondition a solenoid energy reservoir if that hardware path is
  eventually selected.
- Execute a chorus or ripple only after local power permission.
- Return to deep sleep unless a show or maintenance lease says otherwise.

### SCHEDULE_ACQUIRE / TWILIGHT_DIAGNOSTICS

- Wake before the next scheduled transition and increase listen duration/frequency so
  fixtures reacquire fresh time from several anchors or recently synchronized peers.
- Enter night mode at the configured UTC instant, subject to the local power veto.
- Broadcast time source, age, uncertainty, and next-transition version rather than
  treating one panel-current sample as authoritative dusk.
- Continue collecting panel/lux evidence as diagnostics and as an input to whatever
  invalid-time degraded mode is explicitly selected.
- Never allow missing/stale wall time to recreate an accidental unbounded 13-15-hour
  P126 show.

There need not be one official fleet-start packet. Each fixture evaluates the same
versioned schedule against a qualified time estimate. Regions may still differ by the
bounded uncertainty of their current estimate, and the logical-phase/event layer can
schedule a tighter synchronized opening once the show is active.

### NIGHT_AUTONOMOUS

- Radio and sensors remain active at the cadence required by the selected choreography.
- The accurate awake clock plus ongoing peer packets maintains shared phase.
- The program runtime may run CA, timelines, spatial effects, sensor-triggered scenes,
  solenoid accents, or hybrids.
- Local battery state may reduce brightness, strike participation, sensor cadence, or
  radio rate before the ADR 0023 hard thresholds.

### LOW_ENERGY / PROTECT

- Power policy overrides all artistic requests.
- LEDs and solenoid are safely off before sleep or reset-prone transitions.
- Durable state prevents POR from re-enabling the load into a boot loop.
- A recovering node reacquires phase and current events; it does not replay old events.

These are conceptual names, not a required enum or final transition table.

## Distributed choreography runtime

The renderer should consume a generic choreography state rather than assume CA is the
only autonomous mode. Candidate program families include:

1. **Neighborhood CA:** excitable media, reaction-diffusion approximations, forest-fire
   rules, Lenia-like continuous fields, and other local-state systems.
2. **Asynchronous local systems:** rules that intentionally tolerate or exploit
   non-simultaneous neighbor updates.
3. **Deterministic timelines:** a preloaded light/solenoid sequence keyed by fleet phase.
4. **Spatial ripples and waves:** delay/intensity derived from registered position,
   graph distance, RSSI topology, or hop count.
5. **Sensor-seeded programs:** presence, sway, touch, wind, or a carried wand injects
   energy or state into an otherwise autonomous process.
6. **Distributed easter eggs:** a fleet-level sensor condition selects a known program.
7. **Bridge-modulated programs:** a bridge streams tempo, energy, color, seed, or sparse
   events while fixtures still render locally.
8. **Bridge-directed frames or cues:** available for special performances, but not the
   only way the tree works.

The likely common interface is conceptually:

```text
render(local_state,
       recent_neighbor_state,
       fresh_observations,
       fleet_phase,
       active_program,
       bridge_modulation,
       local_power_budget)
```

## CA synchronization choices remain open

Classical CA assumes simultaneous generations, but the artwork need not commit to that
model. Two useful options should remain available:

- **Generation-based:** packets carry generation and state. A node advances after
  enough neighbor states arrive or a timeout. Shared phase defines nominal tick edges.
- **Asynchronous:** nodes update from the most recent neighbor state without requiring
  a common edge. This may be more organic and inherently robust to loss.

The first minimum-viable CA bench should compare both visually. Architecture should not
hard-code synchronous generations before that comparison.

## Daytime solenoid chorus and ripple

Daylight cannot support the intended lightshow, but it can support an acoustic network.
A telemetry/listen rendezvous can double as an artistic moment.

The daytime-only constraint may be an artistic advantage rather than a compromise. A
tree that has a solar-percussion personality by day and a light personality by night
rewards a second visit and makes the energy source legible in the experience. The
daytime show can therefore remain exclusive even if a future electrical design could
technically strike after dark.

Two distinct effects should be first-class:

### Chorus

All eligible fixtures strike at one future fleet phase. Radio propagation is negligible
at tree scale; task scheduling, phase error, MOSFET timing, solenoid mechanics, bamboo
coupling, and sound travel dominate the perceived spread. A reasonable initial bench
target is tens of milliseconds, but no production number should be chosen before a
multi-node acoustic measurement.

### Ripple

One origin starts an event and each fixture adds a deliberate delay based on:

- graph hop count;
- registered physical distance;
- radial/tree layer;
- fixture class;
- deterministic ID/topology order;
- local sensor location.

An initial artistic range might be 50-200 ms between layers, but this is only a test
range. The correct value depends on the installed tree, bamboo acoustics, and ambient
sound.

## Brief autonomous solenoid shows

A short audio show should distribute a compact program rather than radio every strike
in real time:

```text
program_id
event_id
scheduled_start_time
seed
tempo
duration
intensity
spatial mode
```

Each fixture renders its own part. Before accepting the program, it advertises or
internally checks readiness. A low-energy fixture may abstain without blocking the
others. Candidate admission inputs are:

- battery voltage under representative load;
- corrected coulomb budget rather than flaky gauge SOC percent;
- present charge/input power and recent solar trend;
- local strike count/cooldown budget;
- recent resets or power-path faults;
- any reservoir/capacitor readiness if that option exists.

## Solenoid energy and protection

The total energy of one short strike is likely small; peak current and rail integrity
are the important problems. As a rough scale, a 7 ohm coil at 3.2 V draws about 0.46 A.
A 40 ms pulse is about 0.005 mAh and 0.059 J before driver, wiring, magnetic, and
mechanical losses. A sequence of many strikes still needs a measured budget, but a
single strike is not comparable to hours of LED operation.

Every production path should preserve the already validated safety shape:

- MOSFET driver with flyback protection; never drive the coil from a GPIO;
- signal pulldown and explicit LOW before rail enable/board initialization;
- timer-bounded pulse plus an independent software deadline;
- hard pulse-width bounds;
- minimum rest/cooldown interval;
- forced LOW before OTA, maintenance transitions, sleep, and reset-prone work;
- no boot strike;
- per-window strike/energy budget;
- battery, rail, fault, and recovery vetoes;
- telemetry counters for requested, executed, blocked, and failsafe-ended strikes.

## Solenoid power and harness stance -- VDC + 10,000 uF now leads, still not locked

The July 14 first-capacitor trial materially changed the trade. A 10,000 uF, 16 V
electrolytic placed directly across the solar V+/GND feed transformed the previously
weak no-capacitor P126-panel strike into a qualitatively strong result. Ben's immediate
assessment was that it "works so well." Exact pulse width, coil SKU, VDC droop,
recharge time, and telemetry still need a controlled capture, so this remains a leading
candidate rather than a production decision.

The tested panel-input harness is unusually assembly-friendly:

```text
Voltaic P105/P126 3.5 x 1.1 mm male DC plug
  -> Voltaic female 3.5 x 1.1 mm DC to male USB-C adapter
  -> small female USB-C to four-pin JST-XH breakout
       V+ and GND alone are moved into a two-pin XH housing
       the other two breakout conductors are left disconnected
  -> PowerFeather VDC/GND

10,000 uF capacitor:
  + lead -> V+ through-hole on the USB-C/XH breakout
  - lead -> GND through-hole on the USB-C/XH breakout
```

The capacitor diameter and lead spacing happen to place its leads directly over the
breakout's V+ and GND holes (roughly a 2.54 x 3 mm spacing pattern). The result is about
one minute of straightforward soldering, no bare wire soldered to the PowerFeather, and
an observed Amazon prototype price of roughly $1/capacitor. Volume sourcing should be
investigated only after the electrical/mechanical rating is qualified.

This means the VDC/capacitor branch has provisionally earned its extra component:

- the strike improvement is large and immediately audible, not marginal;
- the capacitor attaches to an existing adapter board rather than creating a new
  PowerFeather solder operation or mixed PH/XH hand-crimp workflow;
- the solar rail supplies the high instantaneous strike energy without placing that
  pulse on the battery/3V3 rail;
- solar availability naturally supports a distinct daytime percussion personality;
- the added assembly appears consistent with the O(1)-operations/MVP philosophy.

At 5.8 V, an ideal 10,000 uF capacitor stores about 0.168 J. That is the right order of
magnitude for one short solenoid pulse, explaining why a 2 W panel can recharge slowly
but still deliver a convincing instantaneous kick. Actual usable energy is lower because
the driver/coil stops benefiting before the capacitor reaches zero volts and because
ESR, wiring, and the panel/charger share the rail.

The previous 3V3 design remains the fallback. It is electrically proven over 815 strikes
and may still be useful for a class without solar/VDC, but it no longer wins merely on
assembly simplicity. The five-pin XH Y-splitter remains useful elsewhere and remains a
fallback way to share 3V3/GND with LED signal A0 and solenoid signal A1.

Open checks before treating VDC + 10,000 uF as production-ready:

- record the exact coil (3 V/5 V and resistance), command pulse width, light conditions,
  pre-strike VDC, minimum VDC during strike, recharge curve, and physical result;
- repeat across bright sun, cloud, shade, P105/P126, charging versus full-battery taper,
  and relevant panel/cable orientations;
- confirm no MCU reset, BQ input fault/requalification loop, charge-state upset, false
  dusk transition, or corrupted telemetry during single and repeated strikes;
- measure hot-plug/inrush behavior with a discharged 10,000 uF capacitor;
- measure how long VDC/cap energy remains after shade, sunset, or panel disconnect.
  "Daytime only" must remain an explicit firmware policy unless a bleeder or measured
  discharge makes it a physical guarantee;
- decide whether a high-value bleeder is useful for predictable discharge/service and
  include its continuous solar loss in the calculation;
- qualify voltage rating against worst-case cold bright-sun Voc. The successful bench
  part is 16 V, while P126's published nominal Voc is about 8.59 V. Measure cold Voc
  and production tolerance to document the actual derating margin;
- qualify capacitance tolerance, ESR, ripple/pulse current, temperature rating, lifetime,
  polarity marking, supplier consistency, and behavior after long storage;
- add mechanical retention so the capacitor leads and breakout solder joints are not
  the structural mount under transport, wind, or vibration;
- verify USB-C adapter and XH pin order/keying on production samples and prevent reverse
  insertion or reversed capacitor population;
- compare total parts/assembly time against 3V3 one final time after strain relief and
  enclosure mounting are included.

Firmware should keep the actuator power source abstract enough that the first hardware
choice does not contaminate the event/choreography protocol.

## Autonomous distributed triggers and easter eggs

The autonomous night show must not be limited to CA. A useful example is a perimeter
presence condition that launches a preprogrammed synchronized sequence.

Requiring literally every outer fixture to report presence simultaneously would be
brittle: one missed detection, occluded sensor, low-energy node, or packet loss would
make the feature appear broken. A stronger candidate is spatial coverage:

1. Registration labels perimeter fixtures or divides the perimeter into sectors.
2. Fixtures broadcast fresh presence observations with origin and sector.
3. Each node maintains a rolling set of recently occupied sectors.
4. A rule such as `at least one confident observation in 7 of 8 sectors for 3 seconds`
   becomes true.
5. Multiple nodes may independently derive the same deterministic event ID.
6. The event schedules a known light/solenoid program several seconds in the future.
7. A cooldown prevents continuous retriggering while the crowd remains in place.

The exact sector count, threshold, hold, cooldown, and program are artistic parameters.
The architectural point is that a distributed condition can launch a synchronized
non-CA timeline without a master.

Other autonomous triggers could include:

- a person progressing around the perimeter;
- several people converging beneath the tree;
- an unusually strong wind/sway event;
- a long period with no visitors followed by one detection;
- repeated touches/knocks forming a rhythm;
- a locally seeded CA wave crossing a fleet-wide threshold;
- solar/energy abundance unlocking a richer mode.

## Optional bridge behavior

The bridge is an intentional temporary participant, not infrastructure. Candidate uses:

- DJ-deck or live-performance modulation;
- sparse cue/event injection;
- temporary global program selection;
- identification and locate operations;
- photogrammetry and spatial registration;
- health and diagnostic collection;
- fleet-time/wall-time observation for logs;
- entry into shared-WiFi OTA maintenance mode.

Bridge authority should be a lease with an expiry. While valid, it can override or
modulate autonomous program selection within local safety limits. If packets stop, nodes
return smoothly to autonomous choreography rather than freezing, blanking, or waiting.

### Catching a sleeping fleet

A bridge need not contact all 150 fixtures directly in one listen window. It can:

1. announce a future performance or maintenance-acquisition epoch repeatedly;
2. have the first receivers remain awake and relay the small control event;
3. allow later waking nodes to learn it from peers;
4. use a generous pre-roll before the scheduled start;
5. optionally collect acknowledgements/coverage summaries before a performance;
6. keep nodes awake for the duration of the show lease.

This mesh relay applies to small control/program metadata only. OTA images remain on the
standard shared-WiFi path and are never carried as ESP-NOW chunks.

## Fungibility and site data

Software fungibility does not require pretending all physical fixture classes are
identical. It means one production image can run on every unit and behavior is selected
from runtime facts:

- detected/declared hardware capabilities;
- fixture class;
- registered position and neighbor map;
- current power budget;
- currently active program and site configuration.

`Outermost`, `tree layer 3`, and `neighbor set` should be site data, not compiled into a
device-specific firmware binary. Registration may use the temporary bridge and
photogrammetry, then distribute/store a versioned site map. Replacing a fixture may
require assigning or learning its location, but should not require compiling or flashing
a different image.

The replacement and unregistered states need defined graceful behavior. A new device can
advertise its capabilities, join generic autonomous effects, and omit position-specific
parts until registration completes.

## Expected failure behavior

| Condition | Candidate behavior |
|---|---|
| bridge absent | autonomous program continues normally |
| bridge lease expires mid-show | fade/transition to autonomous mode |
| network partition | each connected region continues from local evidence/state |
| isolated fixture with fresh-enough held time | follow the stored schedule and run a generic local program |
| isolated fixture with invalid/stale time | use the explicitly bounded ADR 0031 fallback; never infer an unbounded long show |
| missed event announcement | do not execute late unless event explicitly permits catch-up |
| duplicate event | dedupe by event ID; never repeat the strike/sequence |
| stale event after POR | reject by boot/session/freshness rules |
| one shaded panel | scheduled lifecycle is unchanged; retain the observation as diagnostic telemetry |
| one missing presence sensor | sector/quorum rule degrades instead of requiring unanimity |
| new POR with unknown phase | acquire before joining tightly synchronized events |
| low battery or power fault | local power veto; optionally advertise abstention |
| solenoid/rail transient | block further strikes, park gate/rail, retain diagnostics |
| unknown program version | reject that event and retain safe autonomous behavior |
| broadcast storm/collision | jitter, bounded relay count/hops, duplicate suppression |

## Candidate implementation sequence

The architecture is large enough that it should be validated in narrow slices.

### Phase 1 -- external time anchors, holdover, and wake-window experiment

- Bench candidate onboard-antenna GPS/GNSS modules through the real hat and near the
  production panel/battery geometry; measure acquisition time and energy.
- Bench candidate battery-backed external RTCs for drift, backup current, reset
  survival, and hot-day/cold-night holdover.
- Define time-quality messages and verify that non-anchor fixtures acquire UTC from
  multiple peers without Starlink.
- In parallel, build otherwise identical default-RC and divided-fast-RC images and
  decide whether the optional sleep oscillator improves bounded holdover enough to
  retain.

### Phase 2 -- host-testable event and phase core

- Add platform-independent packet/event structs and codecs.
- Implement event ID, dedupe, expiry, session/boot handling, and future scheduling.
- Simulate phase error, packet loss, partitions, duplicates, POR, and delayed peers.
- Keep this logic under `firmware/core/` with native tests.

### Phase 3 -- five-node rootless synchronization bench

- Start nodes with deliberately different phase and injected clock error.
- Measure convergence time and residual phase skew.
- Exercise sleeping, replacement, and POR reacquisition.
- Confirm aligned wake windows do not create a collision wall.

### Phase 4 -- multi-node solenoid timing bench

- Start with two, then five solenoids on the likely 3V3/XH path.
- Record command time, GPIO edge, mechanical strike, and audible arrival if practical.
- Compare chorus versus hop/spatial ripple.
- Measure rail voltage, reset/fault behavior, and energy per strike/show.
- Quantify and durability-test the now-leading VDC/10,000 uF path before locking the
  harness; retain 3V3 as the measured fallback.

### Phase 5 -- scheduled day/night lifecycle

- Replace fixed field-cycle wake windows with schedule- and time-quality-aware windows.
- Verify deterministic start/stop using sparse GPS/RTC anchors with Starlink removed.
- Exercise stale time, partitions, conflicting anchors, true power loss, and the chosen
  invalid-time fallback.
- Verify that clouds, shade, full-battery taper, and temporary panel changes do not
  start or extend the scheduled show.

### Phase 6 -- generic choreography runtime

- Port at least one CA, one deterministic timeline, one spatial ripple, and one
  sensor-triggered easter egg through the same program interface.
- Confirm bridge modulation is an overlay, not a separate firmware personality.
- Confirm bridge removal returns to autonomy.

### Phase 7 -- registration and fleet-scale rehearsal

- Define versioned capability and site-map records.
- Exercise identify/photogrammetry/registration with replacement fixtures.
- Re-run network airtime/collision projections at the actual 150-device scale.
- Conduct a full power-day, twilight, night, low-battery, sunrise cycle.

## Measurements that should decide the design

Avoid locking attractive algorithms or packet fields until the following are measured:

- onboard-antenna GPS/GNSS acquisition time and energy through the production hat;
- external-RTC drift, backup current, and reset/power-loss survival across temperature;
- wall-time acquisition, disagreement, holdover, and stale-time behavior without
  Starlink;
- alternative sleep-clock drift versus temperature and added current;
- acquisition time from random phases and after POR;
- rendezvous capture rate and radio energy per day;
- fleet-phase skew distribution while awake and after long sleeps;
- chorus GPIO and mechanical strike skew;
- visually/audibly best ripple delay;
- packet airtime/PDR at 150-node-equivalent traffic;
- proof that panel/charger confounds cannot move a valid scheduled boundary, plus
  separate characterization of any explicitly bounded degraded-time fallback;
- presence-sector false positive/negative rate in actual lantern geometry;
- 3V3 shared-rail solenoid strength and combined LED transient;
- VDC-plus-capacitor acoustic gain versus assembly/power cost;
- number of autonomous audio strikes supportable on poor- and good-harvest days;
- recovery after bridge loss, partition, low battery, and POR.

## Open design questions

- Is one logical phase shared fleet-wide, or is neighborhood phase sufficient for most
  modes with an optional global layer?
- Which peer estimator converges fastest without visible or audible phase jumps?
- How long and how often should unsynchronized acquisition windows be?
- Should daytime chorus epochs be periodic, pseudo-random, solar-surplus-triggered, or
  a mix?
- Should all 150 fixtures knock together, or should audio density be capped by role,
  region, or energy?
- What acknowledgment/coverage level is useful before a bridge performance starts?
- How many GPS/GNSS and RTC anchors are required, and how should they be distributed
  across fixture classes and physical regions?
- What time-age/uncertainty threshold makes a schedule valid, and what bounded behavior
  applies after it expires?
- Should any fixture carry both GPS and RTC, or should those capabilities remain
  separate and cross-check each other over ESP-NOW?
- What exact UTC start/stop instants and offsets apply to each event date and role?
- How much program data is preloaded versus supplied as compact parameters?
- How is the site map distributed, versioned, and recovered after flash replacement?
- What is the exact 5-pin XH header order and branch harness pinout?
- Does the shared 3V3 rail remain stable under the worst LED-plus-solenoid coincidence?
- Does the VDC/10,000 uF result remain strong across weather, coil samples, panel class,
  recharge cadence, cold Voc, and production mounting?
- What local metric best grants a solenoid show: recent solar surplus, corrected coulomb
  budget, voltage margin, or a combination?

## Provisional direction, not a decision

The strongest current candidate is:

- one common firmware image and generic choreography runtime;
- deterministic site/date show windows from a redundant sparse set of GPS/GNSS and
  battery-backed external-RTC anchors (ADR 0031);
- time-quality distribution over ESP-NOW so ordinary fixtures need neither module;
- rootless peer-corrected fleet phase;
- optional divided-fast-RC sleep clock if the A/B validates it;
- schedule-aware daytime sleep/listen windows and pre-transition time reacquisition;
- raw panel/lux evidence retained for diagnostics, sanity checks, and a deliberately
  bounded degraded mode rather than primary production dusk authority;
- independent presence observations plus distributed trigger evaluation;
- future-scheduled idempotent events for aligned effects;
- CA, timelines, spatial effects, solenoids, easter eggs, and bridge modulation as peers
  in the same runtime;
- bridge behavior controlled by expiring leases;
- local power policy always authoritative;
- VDC/GND plus a directly breakout-mounted 10,000 uF capacitor as the new leading
  daytime-solenoid candidate because its strike and one-minute assembly both passed the
  first qualitative test;
- 3V3/GND through a five-pin JST-XH Y-splitter retained as the proven fallback until
  VDC/capacitor voltage, recharge, durability, and production mounting are qualified.

This direction preserves the most important property: removing the bridge, replacing a
fixture, losing a packet, or declining an energy-expensive action does not stop the tree
from being alive.

## References

- `BACKGROUND.md` -- autonomy, fungibility, mesh creative directions, and prior art.
- `firmware/ARCHITECTURE.md` -- target task/layer split and safety constraints.
- `docs/decisions/0004-mesh-esp-now.md` -- ESP-NOW control plane.
- `docs/decisions/0005-firmware-rtos-tasks.md` -- task architecture.
- `docs/decisions/0009-minimize-per-fixture-ops.md` -- production operations constraint.
- `docs/decisions/0023-lfp-power-policy-thresholds.md` -- local energy and low-voltage policy.
- `docs/decisions/0027-sensor-architecture-msa311-and-downward-tof.md` -- sensor allocation.
- `docs/decisions/0028-power-management-bus-integrity.md` -- I2C/power safety constraints.
- `docs/decisions/0029-led-electrical-drive-by-role.md` -- 3V3 rail and harness context.
- `docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md` -- presence as a mesh seed.
- `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` -- measured ESP-NOW behavior.
- `docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md` -- field lifecycle, dusk, and POR findings.
- `ops/bench/data/ca/2026-07-10-ca-field-cycle-9E5B0C-p126-production-cabling.jsonl`
  -- source field log for the sleep-drift estimate.
- Espressif, ESP32-S3 System Time:
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/system_time.html
- PowerFeather V2 schematic:
  https://docs.powerfeather.dev/assets/files/esp32-s3-powerfeather-7620cc4fefa671436564aefb91d09158.pdf
