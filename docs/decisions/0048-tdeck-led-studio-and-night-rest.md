# 0048 -- T-Deck LED Studio and night-rest controls

**Date:** 2026-08-24

**Status:** Accepted; dev-local boot/mesh validated, physical action validation pending

**Owner:** Ben

**Extends:** ADR 0047

**Supersedes:** ADR 0037/0047's blanket exclusion of sleep from the T-Deck,
only for the local, physically confirmed UI path defined here. The Claude tool
surface and serial CLI still exclude sleep.

## Context

At Burning Man, the installed lanterns became operational before the team had
time to tune shows or establish a nightly energy routine. Bridge OS already had
a partial Zones app and a fixed 600-second fleet-dark action. The missing field
operations were simple class colors, visible cohort blinking, and an easy way
to avoid a full night of unnecessary battery draw.

An electrically dark but radio-awake fixture still draws roughly 126-144 mA.
The proven rails-off deep-sleep path is sub-mA by external measurement, so a
long dark lease and a long low-power sleep must be presented as different
actions rather than treated as synonyms.

## Decision

1. Rename and finish Zones as **LED Studio**. It offers labelled color swatches,
   client-side dim, and solid or 1 Hz blink for all fresh fixtures or one
   reported class: downlight, perimeter, uplight, or chandelier.
2. Class-targeted streams use the full 192-entry T-Deck census. A 64-entry UI
   snapshot must not silently limit an approximately 130-fixture command.
3. Blink remains an `NB_DIRECT_FRAME` stream. The 500 ms edges carry the
   existing hard-cut flag so fixture-side slew does not blur them. No new wire
   type or second protocol header is introduced.
4. Add a **Sleep / Dark** local app with 10-minute, 1-hour, 4-hour, 8-hour, and
   12-hour presets:
   - dark sends an expiring hard-cut `PROG_COMMISSION_DARK` lease and leaves the
     radio awake;
   - low-power sleep sends the existing `NB_SLEEP_FOR` command, cuts rails and
     radio, cannot be cancelled while asleep, and automatically resumes normal
     behavior at timer wake.
5. Do not use `NB_TRANSPORT_SLEEP` for an ordinary night. Its retained dark
   latch intentionally survives timer wake until a later program command;
   night sleep should resume automatically.
6. Both fleet-wide actions pass through the on-device confirmation rail. The
   modal names the action and duration, shows live/seen counts, warns when the
   radio will be unavailable, and focuses cancel by default.
7. Starting a rest action stops any suspended LED Studio stream. A sleeping or
   otherwise non-listening fixture cannot receive the broadcast and is not
   claimed as changed.
8. Keep sleep out of the Claude agent schema and serial quick commands. This
   narrow local-UI exception does not expand OTA, profile, capacity, reboot,
   maintenance-voltage, or other persistent mutation authority.

## Consequences

- A field operator can set basic fleet/class colors and blink cohorts without
  a laptop or internet connection.
- Dark remains reversible and radio-reachable but is not sold as a battery
  sleep mode.
- Low-power sleep gives the desired overnight energy saving, but the fleet is
  intentionally unreachable until the selected timer expires (or a physical
  power cycle wakes a fixture early).
- The command is best-effort broadcast. Field use must compare the live count
  before confirmation with the subsequent census disappearance/rejoin rather
  than assuming every historical fixture heard it.

## Validation required

Field status on 2026-08-24: `tdeck-0.2.0-field1` was exact-binary USB-flashed
to T-Deck `8EB508` on `COM152`. It booted with the expected main banner, all
known peripherals, stable memory, and live fleet receive. No fixture command
was sent. Its machine telemetry retained a separate stale `tdeck-0.1.0` label;
current `field2` source replaces that duplicate with one shared version header
but has not yet been accepted as a retained build or flashed.

The same source was subsequently built through the locked development cache
as `tdeck-dev-local` (SHA-256
`90fd45e257a92d61c94ae1cd87e23fa7383ee2cd009916f673dc219ac57cbae5`) and
USB-flashed to `8EB508`. Both boot and telemetry identities were correct; the
peripheral probe, memory check, and live mesh receive passed with no panic. No
fixture command was sent, so the physical action checks below remain open.

1. Test touch plus trackball navigation on the flashed T-Deck.
2. With explicit canary fixtures from both LED roles, verify red/green/blue,
   class filtering, dim, 1 Hz blink, stop/staleness, and reported LED color.
3. Confirm a 10-minute dark lease self-expires and Release Dark is immediate.
4. Confirm a short low-power sleep cuts both rails/radio, cannot be released by
   the bridge while asleep, and rejoins normal operation on timer wake.
5. Before an overnight fleet sleep, compare live/seen counts and account for
   any already sleeping or silent fixtures separately.
