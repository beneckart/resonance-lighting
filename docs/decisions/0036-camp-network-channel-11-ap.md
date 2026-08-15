# 0036 -- Camp network on a channel-11-pinned AP

**Date:** 2026-08-15

**Status:** Accepted; router ordered, not yet received or configured

**Owners:** Ben + Claude

## Context

Every Resonance device to date sidesteps a constraint that a camp WiFi network
makes unavoidable. The ESP32-S3 has **one 2.4 GHz radio**. WiFi STA mode and
ESP-NOW coexist on it, but they must share a single channel, and in STA mode the
channel is dictated by the access point. The production fleet is pinned to
**channel 11** (ADR 0004; every commissioned fixture, every bench artifact).

The existing bridges avoid the collision by never associating:
`firmware/cores3_bridge` explicitly "stays unassociated from infrastructure WiFi
and pins ESP-NOW to channel 11". Fixtures associate only in a deliberate
maintenance mode, during which they have already left ESP-NOW -- so the two
have never had to run at once.

That changes as soon as a device wants to be on the mesh and on the internet
simultaneously: a Claude-backed handheld (ADR 0037), a laptop running
`net_bench_dashboard.py` against a bridge while the fleet is live, or any future
telemetry uplink. It also changes for OTA at camp scale, where the shared-WiFi
parallel path (`ops/bench/net_bench_ota.py`) is the recommended fleet mechanism
and the AP is whatever the camp brought.

An AP left on automatic channel selection will land on 1 or 6 most of the time.
A device that associates to it drags its radio off channel 11 and goes silently
deaf to the entire fleet. There is no error and no log line -- the mesh simply
stops arriving. This is a failure mode worth designing out before the event
rather than debugging in dust at 2 AM.

## Decision

1. **The camp 2.4 GHz AP is pinned to channel 11, 20 MHz width (HT20), WPA2-PSK,
   on a dedicated SSID separate from any 5 GHz SSID.** Automatic channel
   selection is prohibited on the 2.4 GHz radio. 5 GHz is unconstrained and is
   the preferred band for laptops and phones.
2. **Channel 11 is the fixed point, not the AP.** If a future site constraint
   forces a different channel, the fleet channel moves first, deliberately, by
   the migration path already built for it -- never by reconfiguring an AP and
   hoping.
3. **Network topology:** Starlink in bypass mode feeding a GL.iNet Beryl AX
   (GL-MT3000) travel router over Ethernet. The Beryl serves the pinned 2.4 GHz
   SSID and is USB-C powered (about 5 W) from camp batteries.
4. **Firmware channel guard (required on any Resonance device that associates to
   an AP while also using ESP-NOW).** After STA association, read the actual
   operating channel. If it does not equal the compiled mesh channel, **drop the
   WiFi association and keep the mesh**, then surface the mismatch on whatever
   display or serial the device has.
   The mesh is the primary function; internet is an enhancement. A device that
   silences its mesh to hold a WiFi link has the priority backwards.
   Devices with no display log it and continue mesh-only.
5. **The guard does not apply to maintenance mode.** A fixture entering
   shared-WiFi maintenance has already left ESP-NOW by design and may associate
   on any channel. That is the existing, validated OTA path and is unchanged.
6. **Rehearse the Starlink bypass-mode switch at home before the event.** Leaving
   bypass mode requires a factory reset, so it is not a field-improvisable step.
   **Dish generation resolved 2026-08-15: the project has Gen 3 and Gen 4 units
   (possibly all Gen 4), so Ethernet is built in and no Starlink Ethernet Adapter
   is needed.** That closes the only lead-time-sensitive item here. Still confirm
   the port physically on the specific unit that travels, and that bypass mode is
   present in its app settings.

## Consequences

- One config line on one router removes an entire class of silent, hard-to-
  diagnose mesh failure.
- The channel guard converts "someone factory-reset the router" from a dead mesh
  into a legible message, and keeps the fleet working while it says so.
- The camp network becomes a named dependency of any simultaneous
  mesh-plus-internet feature. It is not a dependency of the fleet itself:
  fixtures run autonomous shows with no infrastructure at all (ADR 0004), and
  that stays true.
- 2.4 GHz camp clients share a channel with about 130 fixtures. ESP-NOW frames
  are short and the fleet's duty cycle is low, but heavy 2.4 GHz client traffic
  and the mesh now contend. Push laptops and phones to 5 GHz.
- A second AP on the art site (versus camp) must use the same channel and, if it
  is to serve OTA, the same SSID/PSK. The one-virtual-SSID-across-two-Starlinks
  question is already queued in `TODO.md` and is now also gated by this ADR.

## Validation required

1. Configure the Beryl on arrival; verify with a scan (phone, or the bridge's
   own `NB_SCANAP` path) that the 2.4 GHz SSID reports channel 11 and HT20.
2. Rehearse Starlink bypass mode at home, end to end, including the recovery
   path if bypass has to be undone. (Ethernet adapter: not needed -- Gen 3/4.)
3. Associate one device to the pinned AP while a fixture is beaconing, and
   confirm ESP-NOW RX continues uninterrupted -- both directions, for at least
   an hour.
4. Deliberately set the AP to channel 1 and confirm the guard fires: WiFi
   dropped, mesh retained, mismatch surfaced.
5. Run one parallel shared-WiFi OTA (`ops/bench/net_bench_ota.py`) over the
   Beryl to confirm the maintenance path works on the camp router, not just on
   a house network.
6. Measure the Beryl's actual draw against the camp battery budget, powered the
   way it will actually be powered.

## References

- `docs/howto/CAMP_NETWORK_SETUP.md` -- the runbook for steps 1-6
- `docs/decisions/0037-claude-mesh-bridge-handheld.md` -- the first consumer
- `docs/decisions/0004-mesh-esp-now.md` -- ESP-NOW, no infrastructure required
- `docs/decisions/0010-standard-ota-no-mesh-firmware-gossip.md` -- deliberate
  maintenance mode
- `firmware/cores3_bridge/README.md` -- the unassociated-bridge precedent
- `AGENTS.md` -- OTA fleet path gotcha (shared WiFi, not per-board AP)
