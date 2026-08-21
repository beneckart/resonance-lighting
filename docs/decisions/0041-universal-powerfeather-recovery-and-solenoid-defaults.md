# ADR 0041: Universal PowerFeather recovery and solenoid defaults

- Status: Accepted
- Date: 2026-08-16
- Deciders: Ben Eckart
- Supersedes: canopy-only solenoid build scoping in the 2026-08-16 operational rollout
- Extends: ADR 0009, ADR 0023, ADR 0030, ADR 0033

## Context

The installed fleet now mixes rev-1 capboards, rev-2 capboards without 433 MHz
receivers, and bare PowerFeathers whose eventual role is not reliably known.
Role-specific firmware therefore creates avoidable operational ambiguity: an
operator cannot infer from physical placement whether an addressed solenoid
strike is available.

The same fleet also demonstrated that the BQ25628E 30 mA precharge POR value can
leave deeply depleted production LFP fixtures near energy-neutral even with a
valid panel source. A corrected 300 mA configuration was canary-tested and then
validated across both low-voltage and healthy installed fixtures. The charger's
fixed trickle threshold, input DPM, thermal regulation, and transition to normal
fast charge remain independent protections.

## Decision

Every production PowerFeather fixture image uses these defaults:

1. BQ25628E precharge current is 300 mA, programmed through the full 16-bit
   REG0x10 read/modify/write transaction and verified by readback. OTA validity
   requires the requested value to match the decoded readback.
2. Solenoid capability is enabled for every fixture class. A one-time NVS policy
   migration changes historical absent or disabled state to enabled. After that
   migration, an explicit runtime disarm is preserved.
3. Armed idle on D7/GPIO37 remains INPUT/high-Z. This preserves the shared rev-1
   receiver/manual source and makes the setting an electrical no-op on a Feather
   without a connected capboard or solenoid load.
4. Enablement does not imply autonomous actuation. MCU strikes still require an
   addressed command or deliberate local input and retain lifecycle,
   solar-surplus, battery-tier, pulse-width, rest-time, maintenance, and failsafe
   gates. The local USER-button path remains available as a manual service input.
5. `--canopy-solenoid` remains a deprecated build-script no-op for recipe
   compatibility. Artifact manifests must describe universal solenoid capability
   directly rather than infer it from that flag. `--solenoid-test` remains a
   targeted bench-only override and is not a fleet posture.

## Consequences

- One inspected artifact can be used across the mixed rev-1, rev-2, and bare
  PowerFeather fleet, consistent with ADR 0009.
- Any fixture can accept a targeted strike if matching capboard hardware is
  present; boards without that hardware do not actuate anything.
- Rev-1 receiver operation is not clamped between MCU strikes.
- Historical intentional disarms cannot be distinguished from the old default
  zero and are migrated once. Operators who need a fixture disarmed must issue
  the runtime disarm after this policy lands; that choice then persists.
- The 300 mA setting improves recovery opportunity but does not create input
  power. A shaded, disconnected, current-limited, or thermally regulated source
  may still deliver less, and unsafe OTA targets still require proven power
  ride-through.

## Verification required for a fleet artifact

- Native tests cover the solenoid NVS policy migration and explicit post-policy
  disarm behavior, plus the BQ register codec and OTA readback predicate.
- A battery-backed canary must report the exact immutable artifact revision,
  OTA state `valid`, `solenoid_enabled=true`, gate off, zero uncommanded strikes,
  and a 300 mA precharge readback after the 20-second pending-verify window.
- Fleet rollout uses exact short-MAC targets and the shared-WiFi OTA path under
  one declared writer. USB connected by other operators is power-only unless
  writer ownership is explicitly transferred.
