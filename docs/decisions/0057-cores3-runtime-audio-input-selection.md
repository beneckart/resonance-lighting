# 0057 -- CoreS3 runtime Ambient/Aux audio input selection

**Date:** 2026-08-25

**Status:** Accepted; paused handoff hardware-validated

**Owner:** Ben

## Context

ADR 0054 combined Listener and Audio into one touch-first CoreS3 Bridge OS, but
still treated built-in microphones versus Module Audio as a build-time choice.
The Module Audio build already configures both independent CoreS3 and M-Bus I2S
paths. An operator should be able to use the local microphones for ambient sound
and the LINE/MIC input for a directional RODE or other analog source without a
laptop or firmware change.

USB reset testing also showed that the externally powered module controller can
occasionally be unavailable during the CoreS3's first initialization attempt.
Ambient fallback worked, but readiness at one instant must not be confused with
whether the installed image supports Aux.

## Decision

1. A Module Audio Bridge OS image exposes two runtime sources in the Audio app:
   **Ambient Mic** is the CoreS3 dual microphone input; **Aux Input** is Module
   Audio's TRS LINE/MIC input. The built-in-only image remains Ambient-only.
2. The Audio footer is Start/Pause, Input, and Look. Input alternates Ambient
   and Aux. Optional USB `N` invokes the same action for diagnostics; USB is not
   required for operation.
3. A source handoff is an artistic ownership boundary. If publishing, the bridge
   sends a zero direct frame and pauses before changing hardware. A successful
   handoff resets the envelope and two-second noise-floor calibration, then
   restores the prior publishing state. A failed handoff keeps the previous
   working source.
4. The Module build prefers Aux at boot and falls back to Ambient when Aux is not
   ready. Build support and current readiness are distinct. If the module
   controller did not answer at boot, Input may retry it later.
5. Once the controller is detected, an I2S/codec initialization failure is not
   repeatedly retried in place. The current upstream Module Audio library has no
   teardown method, so the safe recovery is a full power cycle rather than
   accumulating partial I2S state.
6. Module LEDs indicate the selected path: green for Aux, dark blue for Ambient.
   The screen remains authoritative and says `AUX INPUT`, `AMBIENT MIC`, or
   `INPUT FAILED` plus `PUBLISHING`/`PAUSED`.
7. No mesh packet changes are required. Both sources feed the same existing
   envelope, look, chunking, and `NB_DIRECT_FRAME` publisher.

This supersedes ADR 0054 decision 4 only where it called built-in versus Module
Audio solely a hardware build variant. Including the module driver is still a
build choice; selecting Ambient versus Aux inside that image is now runtime.

## Consequences

- One field artifact covers nearby ambient response and cabled directional audio.
- Every source change recalibrates, so the first two seconds should contain
  representative background sound rather than intentional peaks.
- A late module does not take down Audio or permanently disable the Aux control.
- Active-stream fixture behavior still needs named-canary acceptance; the safe
  paused hardware handoff is already proven.

## Validation

On CoreS3 `4D5DB0` (`80:45:6B:4D:5D:B0`), the exact r6 Module Audio artifact
completed Aux -> Ambient -> Aux through the shared Input action. Each state
reported ready with `active=0`, `frames=0`, and `readfail=0`. The installed
binary is 1,168,208 bytes with SHA-256
`01F99A5167C5DE01C6FC75BD5781F4F8AB337D2095B3870122858909B206EE12`.

## References

- `docs/decisions/0054-cores3-wireless-two-app-bridge-os.md`
- `firmware/cores3_bridge/cores3_bridge.ino`
- `firmware/cores3_bridge/app_model.h`
- `docs/howto/CORES3_AUDIO_REACTIVE.md`
- `docs/howto/BRIDGE_OS_FIELD_MANUAL.md`
