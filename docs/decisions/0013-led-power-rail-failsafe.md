# 0013 -- LED rail must be switchable and fail-safe; exact voltage rail chosen by test

**Date:** 2026-05-08
**Status:** Accepted
**Owners:** Ben
**Supersedes:** ADR 0008

## Context

The earlier design powered WS2812B LEDs directly from the LiFePO4 battery rail and relied on the logic-high threshold math to avoid a level shifter. That remains a useful technique, but it is not the most important requirement.

Addressable LEDs remember their last command. If the MCU hangs or crashes after commanding nonzero pixels, the LEDs can remain on and drain the battery. A deeply drained battery can then cause brownouts, reboot loops, failed OTA, or difficult field recovery. With 1-25 LEDs per fixture, this stuck-on failure mode matters more than saving a small boost converter or level shifter.

## Options considered

- **Direct Vbat, always connected:** simplest, but can leave LEDs on after MCU failure.
- **Direct Vbat through a load switch / high-side FET:** efficient and fail-safe if default-off.
- **Regulated 3.3 V LED rail:** simple logic levels, but current may exceed small MCU-board regulators.
- **4.5 V bq25185 load rail:** useful if using bq25185, but data-level and efficiency behavior must be tested.
- **5 V boost rail:** robust LED color/brightness and easy COTS USB power, but adds converter losses and startup behavior to test.

## Decision

The LED rail must be switchable and default-off. The exact LED voltage rail is chosen by bench testing and production architecture.

Minimum hardware requirements:

- LED power controlled by a dedicated enable signal, load switch, P-MOSFET, or regulator/boost enable pin.
- LED power defaults OFF during reset, boot, brownout, and unprogrammed MCU states.
- Firmware enables LED power only after boot, battery sanity check, and watchdog initialization.
- Firmware disables LED power in low-battery, shipping, fault, and watchdog-recovery modes.
- LED data line is forced low before LED rail shutdown.
- LED data line includes a small series resistor near the driver.
- LED rail includes local decoupling and bulk capacitance appropriate to the chosen LED count.
- Early custom boards reserve footprint/routing for a level shifter or buffer if voltage testing shows marginal data behavior.

## Consequences

- "No level shifter" is no longer a design goal. It is an optimization allowed only if testing supports it.
- Direct Vbat remains allowed when paired with a fail-safe switch.
- COTS boards with user-controlled NeoPixel/LED power are preferred for prototypes.
- Test plans must include a simulated MCU hang with LEDs on, watchdog reset, low-battery cutoff, and cold boot from a depleted battery.
- Current budgeting must assume accidental all-on LED modes are possible and must be bounded in firmware.
