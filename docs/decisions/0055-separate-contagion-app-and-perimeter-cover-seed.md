# ADR 0055: Separate Contagion app and perimeter palm seed

**Date:** 2026-08-25

**Status:** Accepted; perimeter VL53 palm seed provisionally qualified on one
cloudy-daylight canary

## Context

Greenberg-Hastings wildfire is a cellular automaton with excitable and
refractory states. Color Virus, epidemic/recovery behavior, and future
infection-like pieces form a different artistic family: they carry identity
(color), may persist, and may recover or recur. Putting every idea behind CA
Studio would make the existing CA controls and language increasingly unclear.

The perimeter fixtures are manually reachable and carry an outward-facing
VL53L5CX. General human presence at that sensor is not yet trustworthy, but a
visitor deliberately holding a hand close to or over the sensor could be a
discoverable easter egg. This should be hardware-qualified in daylight without
allowing a noisy distance value to become repeated activation.

## Decision

1. Keep **CA Studio** focused on Greenberg-Hastings wildfire. Add a separate
   **Contagion** Bridge OS app and fixture program ID 5.
2. Contagion initially offers two models:
   - **Color Virus:** susceptible -> infected; infection and its hue persist
     until program reset, release, or lease expiry.
   - **Epidemic:** susceptible -> infected -> immune -> susceptible, with
     operator-selected tempo presets controlling the phase durations.
3. Each model offers lights, knocks, or lights + knocks. One transition into
   infected may request one 40 ms strike on a downlight-class node. Perimeter
   nodes seed and relay the same infection but never request a pulse on their
   unused D7. The program never grants actuator authority: lifecycle, solar
   surplus, battery, arming, rest, maintenance, and mechanism failsafes still
   decide whether a physical knock occurs. Knock-only output keeps the LED rail
   electrically dark on every class.
4. Start is a confirmed 10-minute fleet program lease with all nodes
   susceptible. Seed is a confirmed exact-target program update. Stop uses the
   existing fleet release. The ordinary repeated RF copies of one
   `NB_PROGRAM_SET` are de-duplicated at each fixture so one seed remains one
   infection edge.
5. Reuse the existing append-only packet layouts. `NbProgramSet.params` carries
   model, output, phase timing, hue, local-sensor enable, and seed flags.
   `NbChoreoState.state` carries two status bits plus a six-bit hue bucket.
   A fixture accepts infection only from a fresh neighbor reporting program 5.
6. Manual and local seeds may choose a random hue; infection adopts the
   infecting neighbor's transmitted hue. This makes color identity part of the
   propagation rather than a fleet-wide render setting.
7. Retain USB diagnostics for the perimeter VL53L5CX: fresh-frame count,
   closest return, raw-target zones, valid-return zones, 30-350 mm near-zone
   count, and all 16 nearest valid zone distances.
8. Route a deliberate perimeter palm gesture into the same one-shot program
   input. Require at least 4 of 16 zones in the 30-350 mm near band for two
   fresh sensor reports, latch after the edge, and require four clear reports
   before re-arming. This is a close-cover easter egg, not general presence.
9. Local ToF seeding uses the learned TMF approach detector from ADR 0044/0053
   on downlights and the broad VL53L5CX palm gate on perimeter fixtures. The
   older listener color wipe remains downlight-only.
10. Until the deployed fleet understands program 5, Bridge OS offers an
    explicit **legacy fleet roll** output. The operator selects one fresh
    Contagion source. On that source's susceptible -> infected state edge, the
    T-Deck sends one deterministic 40 ms `NB_TARGET_SOLENOID` roll to fresh
    downlight-class fixtures only. Repeated state/RF frames are de-duplicated,
    the adapter expires with the 10-minute lease, and Stop disables it. This is
    visibly distinct from native `knocks` so an updated fleet cannot be struck
    once natively and again by a hidden compatibility bridge.

## Parameter and state encoding

`NbProgramSet.params[8]` for program 5:

| Byte | Meaning |
|---|---|
| 0 | model: 0 Color Virus, 1 Epidemic |
| 1 | output: 0 lights, 1 knocks, 2 lights + knocks |
| 2 | infected duration in ticks |
| 3 | immune duration in ticks |
| 4 | tick period in deciseconds |
| 5 | local/fixed hue |
| 6 | local ToF seed enable |
| 7 | bit 0 seed now; bit 1 random local seed hue |

`NbChoreoState.state` uses bits 0-1 for susceptible/infected/immune and bits
2-7 for the propagating hue bucket. No packet size, field order, or protocol
version changes.

## Consequences

- CA remains a legible instrument while Contagion becomes a seam for Color
  Virus, Epidemic, and later related behaviors.
- Old fixture images reject unknown program 5 and remain on their prior
  behavior; a mixed fleet therefore does not produce a complete infection
  graph until updated.
- A refused knock does not stop infection state from propagating.
- A reachable perimeter node can therefore be the visitor-facing seed while
  nearby updated mallet downlights provide the audible response.
- The explicit legacy roll makes that same interaction usable with the current
  pre-program-5 fleet, but depends on the T-Deck remaining awake and receiving
  the selected source's state edge. Native fixture propagation remains the
  eventual bridge-independent design.
- Color Virus intentionally saturates the connected updated graph after a seed;
  release/restart is the reset. Epidemic creates recurring local waves instead.
- The reachable perimeter easter egg is enabled only when an operator opts a
  CA/Contagion lease into local ToF seeds. Manual exact-target seed remains the
  reliable fallback. Direct sun and final installed geometry still require
  qualification before relying on it as a show control.

## Initial perimeter hardware result

On cloudy evening 2026-08-25, exact canary `Magmar [F2BDFC]` was tested through
a synchronized USB trace. With the outward sensor facing open space and the
operator explicitly clear, repeated reports held at 0 of 16 valid/near zones.
A touching or extremely close palm briefly produced 11, 9, and 14 near zones
but could fall back to zero because it was inside the useful range. With a flat
palm held 5-10 cm above the sensor, 15-16 of 16 zones remained valid and near
for the rest of the capture, with distances approximately 60-93 mm. This is a
large separation from clear and comfortably supports the >=4-zone, two-report
gate. Earlier unsynchronized clear-only traces were not a hand test and are not
negative evidence.

## Validation required

1. With three updated light-only canaries, prove exact-target Color Virus hue
   propagation, persistence, release, and Epidemic recovery/reinfection.
2. On a sensor-verified downlight, prove ToF off cannot seed and ToF on can
   seed. On F2BDFC, prove the now-flashed gate originates exactly one seed per
   palm/clear/palm sequence. Repeat the perimeter trial in direct sun and final
   installed geometry. Prove an old/unknown program peer neither crashes nor
   joins the graph.
3. Only after a named solarnoid is explicitly armed, prove knock-only and both
   outputs request one bounded strike per infection edge and that a refused
   strike still relays infection.
4. In explicit legacy-roll mode, select F2BDFC, prove one palm starts exactly
   one downlight-only addressed roll, held/repeated infected frames do not start
   another, Stop/expiry disables the adapter, and a second clear/re-enter edge
   can start the next roll. The underlying old-protocol fleet roll is already
   field-used; this test qualifies only the new sensor-edge adapter.
