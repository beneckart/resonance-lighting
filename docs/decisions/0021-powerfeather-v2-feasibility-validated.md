# 0021 -- PowerFeather V2 feasibility validated (networking, solar, field-OTA): proceed

**Date:** 2026-06-08
**Status:** Accepted -- go. PowerFeather V2 survived the COTS feasibility de-risking on its
highest-risk axes; continue building the COTS prototype track on it (confirms ADR 0015/0016).
**Owners:** Ben

## Context

ADR 0015/0016 chose the **PowerFeather V2 (ESP32-S3)** as the leading COTS reference, but
basing ~100 fixtures on it carried real unknowns that had to be retired *before* buying at
scale or freezing the architecture. The two genuine risks were **radio performance** (the
board isn't optimized for RF; custom-ESP antenna/back-off mistakes are a known horror story)
and **whether a deployed, battery-only lantern can be maintained without ever being taken off
the tree** (a hard operational requirement). Solar was expected to "just work" (it's the
board's purpose) but was unmeasured. This ADR records the bench results and the go decision.

## Decision

**Proceed with PowerFeather V2 as the COTS reference architecture.** It passed feasibility
testing on networking, field-OTA, and solar. Remaining work is *spec/refinement*, not
viability (listed below); none blocks committing to the board.

## Evidence (2026-06-07/08 bench -- `docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md`, LOG)

- **Networking (ESP-NOW) -- strong.** 5-node bench: ~99% PDR; rate sweep 1->50 Hz with a clean
  airtime trend extrapolating to **~98-99% PDR at 100 nodes** at a 1-2 Hz heartbeat (broadcast,
  unencrypted -- sidesteps the ~6-17 encrypted-peer cap). Range held **through a house + full
  backyard + behind an oak (~100 steps)**; the **lantern enclosure is RF-transparent**, and the
  **solar panel is the main ~20 dB attenuator** (antenna keep-out matters). No brownouts on a
  healthy cell.
- **Field-OTA (the no-touch requirement) -- met.** Battery-only, no-physical-access OTA recovered
  **~17/17** (incl. 3/3 on an LFP at the ~3.2 V buck-boost crossover) via software reset.
  **A/B rollback validated**: a self-test-failing image auto-reverts to the last-good image,
  no touch. Watchdog (auto-restart on hang) and the autosleep guard also validated.
- **Solar -- path validated.** Charger -> LFP is net-positive even in partly-cloudy, through-glass
  light; full-sun + the board's purpose-built charging give ample margin.

## Consequences

- The COTS prototype track continues on PowerFeather V2; a future custom board uses it as the
  reference (ADR 0012/0015). Antenna placement must respect keep-out from the solar panel.
- Production firmware must implement the validated recovery patterns: standard WiFi OTA
  (ADR 0010), `extern "C" verifyOta()` self-test + `verifyRollbackLater()`/watchdog for
  rollback on a late crash/hang, and the autosleep guard. Set the fuel-gauge `DesignCap`
  once at first boot (don't change in the field).
- Security is **open by design** (unencrypted broadcast); app-layer auth only if a future
  public/grant install needs it.

## Open follow-ups (refinements, not viability blockers)

- Re-derive the nightly **power budget** bottom-up from measured LED draw (SYSTEM.md's
  ~120 mAh/night is an optimistic floor); then size cell + panel.
- **LFP re-verify** of the battery/stability runs (most bench work was on Li-ion); full-sun
  harvest number; `--maintain` (MPP) sweep for the shaded canopy.
- OTA over a **marginal WiFi link**; 20+ node confirmation if the production rate nears the loss knee.
- **Mock-hat RF** with panel + battery installed (Steve) -- the real antenna-detuning case.
- Implement the production rollback/health + watchdog pattern in the real firmware.
