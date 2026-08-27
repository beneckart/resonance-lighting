# 0063 -- PUCA safe arming and exact-target shared-WiFi OTA

**Date:** 2026-08-27

**Status:** Accepted; safe-idle bootstrap and exact-target OTA hardware-
validated, remaining show/rollback gates open

**Owner:** Ben

**Extends:** ADR 0010, ADR 0035, ADR 0040

## Context

PUCA is an optional high-authority publisher. Once powered, the previous
`0.4.1-dev` image immediately entered HEARTBEAT and could send zero-valued
direct frames while its input was silent. That could seize the directed-frame
lease and darken the tree merely because the Pod20 was plugged in or the board
rebooted after maintenance.

The installed module's USB connector is behind the Eurorack faceplate. USB must
remain the emergency recovery path, but routine firmware work should not require
opening the Pod20. PUCA shares the same one-radio constraint as every Resonance
ESP32: show COMMS owns unassociated channel-11 ESP-NOW, while firmware bytes must
travel through standard WiFi OTA rather than a custom mesh transport.

The DJ/per-slot color look is expected to be the common performance mode.
HEARTBEAT remains useful for the deterministic ceremony waveform, but should not
be the first armed mode.

## Decision

1. PUCA boots SAFE-IDLE after every power, software, watchdog, or OTA reset. It
   initializes codec, I2S, and ESP-NOW and advertises identity, but sends no
   `NB_DIRECT_FRAME` until deliberately armed.
2. A continuous 1.2-second capacitive-paw hold during boot is the physical arm
   gesture. A successful hold arms line input + DJ and opens the existing
   20-second setup window. No hold leaves the publisher locked and silent.
3. The live paw cycle is DJ -> HEARTBEAT -> EMBER -> HUE -> DJ. OFF remains
   outside the paw cycle. USB service may still explicitly pause/enable audio.
4. PUCA emits a tail-7 `NB_HEARTBEAT` with short ID `A4EB10` and firmware prefix
   `puca-bridge-*`. Bridge OS can census and exact-target it, while audio bridges
   exclude that non-fixture identity from their light-output census.
5. PUCA accepts `NB_TARGET_ENTER_MAINT` only when `target_id` exactly equals its
   own short ID. It deliberately ignores `00:00:00` fleet-wide maintenance.
6. Entering maintenance stops publishing without sending a blackout frame,
   tears down ESP-NOW, and joins the shared maintenance WiFi. It exposes the
   existing `/telemetry`, `/update`, and `/resume` HTTP contract. It never starts
   the factory `PUCA DSP` softAP and never transports firmware over ESP-NOW.
7. `/resume`, maintenance timeout, and an OTA reboot return to dark COMMS. A
   later paw-held boot is required before performance publishing resumes.
8. PUCA uses the ESP32-PICO-D4 4 MB default dual-app partition layout. A pending
   image remains unconfirmed for 20 seconds, then must pass codec, I2S, network,
   and NVS checks or roll back. Host acceptance still requires a fresh exact
   revision and survival beyond the 25-second observation gate.
9. USB remains the guaranteed recovery path. The one-time USB bootstrap must
   include the gitignored shared-WiFi credentials; a credential-less build
   refuses maintenance rather than creating an AP.

## Consequences

- Plugging in, power-cycling, or OTA-rebooting PUCA cannot silently take control
  of the tree.
- Performance activation now requires a deliberate physical action after each
  boot. An unattended OTA cannot automatically resume a live publisher.
- Bridge OS can initiate PUCA maintenance with `UA4EB10`, but the laptop remains
  the ordinary-WiFi uploader, matching the fixture architecture.
- Generic fleet-wide maintenance cannot remove the one-off audio publisher from
  COMMS. Operators must use the exact PUCA identity and binary.
- The internal USB connector can remain behind the faceplate during routine
  updates, but access must remain possible for recovery.

## Validation required

1. USB-flash a credentialed `0.5.0-dev` image to exact PUCA `A4EB10` and verify
   flash readback.
2. Boot without touching the paw and prove zero direct frames while Bridge OS
   receives the PUCA identity/revision heartbeat.
3. Paw-hold boot and prove DJ is first, setup cycling/lock works, and normal
   locked touches cannot arm or change the publisher.
4. Send both fleet-wide and exact-target maintenance. Prove only `UA4EB10`
   starts shared-WiFi maintenance and telemetry identity matches before upload.
5. Complete one exact-target OTA, observe SAFE-IDLE rejoin with the exact new
   revision after 25 seconds, then test `/resume` and the 10-minute timeout.
6. Flash a forced-self-test-failure canary and prove automatic A/B rollback.
7. Confirm no `PUCA DSP` or other PUCA softAP exists in COMMS or maintenance.

## Validation record

On 2026-08-27, exact PUCA `A4EB10` was USB-bootstrapped with credentialed
`puca-bridge-0.5.0-dev`. The retained application was 1,024,128 bytes with
SHA-256
`1e90f6f1731a622b11274fa91abbc6eeebb17c35abe90bd86337c915cb99e8da`.
Esptool verified all written regions. Fourteen consecutive no-hold samples
reported `active=0`, `bootarmed=0`, locked controls, healthy codec, and zero
direct frames while Bridge OS `8EB508` received fresh PUCA heartbeats.

The host then sent exact command `UA4EB10`, identity-matched the shared-WiFi
`/telemetry` endpoint, uploaded that same retained application, and observed a
fresh same-revision heartbeat with software-reset uptime/sequence after the 25 s
survival gate. A later ordinary USB-induced reset again returned SAFE-IDLE with
zero direct frames. This closes the no-hold bootstrap, identity, exact-target
maintenance, OTA, and post-OTA safe-rejoin parts of items 1, 2, 4, and 5.

Still open: physical paw-held DJ-first/setup behavior, `/resume`, 10-minute
timeout, fleet-wide-maintenance rejection on live hardware, forced-self-test
rollback, and explicit softAP absence. The live fleet made a broadcast
maintenance rejection test inappropriate during this acceptance run.

## References

- `firmware/puca_bridge/README.md`
- `hardware/puca-audio-bridge/README.md`
- `docs/howto/BRIDGE_OS_FIELD_MANUAL.md`
- `docs/decisions/0010-standard-ota-no-mesh-firmware-gossip.md`
- `docs/decisions/0035-puca-performance-audio-bridge.md`
