# COTS bench test plan -- 2026-05-10

Purpose: turn the R&D shopping spree into actionable data for the production decision. The goal is not to crown a board by intuition; it is to measure power, charging, sleep, optics, RF, robustness, and assembly time.

## Test articles

### Track A -- PowerFeather V2 + IS31FL3741

- PowerFeather board from Elecrow.
- LiFePO4 18650 sample cell.
- Solar panels: 1 W, 2 W, 3 W, 5 W samples.
- Adafruit IS31FL3741 13x9 RGB matrix via STEMMA-QT.

### Track B -- PowerFeather V2 + NeoHEX

- PowerFeather board from Elecrow.
- LiFePO4 18650 sample cell.
- M5Stack NeoHEX via GPIO/data and suitable LED power rail.

### Track C -- FeatherS2 Neo + DFR0559

- DFRobot DFR0559 Solar Power Manager 5V.
- LiPo cell.
- FeatherS2 Neo powered from DFR0559 USB output.
- FeatherS2 Neo battery connector left empty.

### Track D -- Atom Matrix + DFR0559

- DFRobot DFR0559 Solar Power Manager 5V.
- LiPo cell.
- M5Stack Atom Matrix powered from DFR0559 USB output.

## Phase 1 -- incoming inspection and identity

For each board:

- Photograph front/back with scale.
- Record SKU, vendor, order date, received date, and visible revision text.
- Inspect connectors and solder joints.
- Check for shipping damage.
- Check continuity for power rails before applying battery/solar.
- Label each board with a short ID.

PowerFeather-specific:

- Determine whether the board is V1 or V2 before attaching LiFePO4.
- Inspect chip markings for TPS631013, MAX17260, BQ25628E, LC709204F, and XC6220.
- Run an I2C scan over the internal bus and record detected devices.
- Only use LiFePO4 on confirmed V2 hardware or hardware explicitly cleared by the designer.

## Phase 2 -- bring-up and firmware sanity

For each MCU board:

- Flash a minimal firmware image.
- Blink/status LED test.
- Print reset reason and board ID over serial.
- Read MAC address and derive runtime fixture ID.
- Test deep sleep and wake.
- Confirm watchdog reset behavior.
- Confirm standard OTA maintenance-mode update on local WiFi.
- Confirm no custom mesh OTA firmware transfer code is present.

## Phase 3 -- sleep/current measurements

Use a USB power meter for USB-powered setups and a low-current profiler or DMM/shunt for battery-powered setups.

Measure:

- Board active idle, no LEDs.
- Board deep sleep, external LED module disconnected.
- Board deep sleep, external LED module connected but power rail off.
- Board deep sleep, external LED module connected and rail accidentally left on.
- WiFi/ESP-NOW active receive/listen behavior.
- ESP-NOW transmit burst current.
- Standard OTA update current.
- Fuel gauge / charger telemetry polling overhead if measurable.

Record:

- Board type.
- Firmware commit/hash.
- Power source.
- Battery voltage/SOC.
- LED module attached.
- Rail states.
- Measured current.
- Notes.

## Phase 4 -- charger and solar tests

For PowerFeather V2:

- Configure battery chemistry for LiFePO4 in firmware/SDK.
- Set conservative charge current initially.
- Set panel MPP/VINDPM values appropriate for each test panel.
- Log charger state, input voltage, input current if available, battery voltage, charge current, battery temperature, faults, and fuel-gauge SOC.

For DFR0559:

- Use LiPo only.
- Record panel input voltage/current if measurable externally.
- Record USB output voltage/current.
- Record battery voltage and charge behavior.

Solar conditions to test:

- Full sun.
- Partial shade.
- Panel tilted poorly.
- Panel dusty/covered lightly.
- Hot panel after sun exposure.
- Morning/evening low-angle sun.

Recovery tests:

- Battery low + sun appears.
- Battery absent + USB/VDC present, if safe for the board.
- Battery present + sudden LED load spike.
- Solar input removed during operation.
- USB input added/removed during operation.

## Phase 5 -- LED module electrical tests

For each LED module:

- Confirm voltage requirement and interface.
- Confirm module power can be switched off cleanly.
- Confirm no back-powering through data or I2C lines when LED rail is off.
- Measure current for center-only, 3-pixel, 9-pixel/crop, and show modes.
- Measure current for all-white worst-case at capped brightness.
- Implement firmware current caps before any long-duration test.

### IS31FL3741 13x9 matrix

Test:

- I2C discovery and initialization after VSQT power cycling.
- Center LED only.
- 3-pixel RGB/fringing pattern.
- 9x9 center crop.
- Full-matrix low-brightness animation.
- Brightness vs current at 3.3 V and, if available, 5 V.
- Visible PWM/multiplex artifacts through gobo/filter.

### M5Stack NeoHEX

Test:

- GPIO data output and pixel ordering.
- Behavior at 3.3 V, if attempted, but do not assume production suitability below spec.
- Behavior at 5 V or suitable boosted rail.
- Diffuser on vs removed/off, if mechanically possible.
- Center LED/ring addressing.
- Gobo washout from rings vs center-only.

### FeatherS2 Neo / Atom Matrix

Test:

- Center pixel mapping.
- Power draw for center-only and multi-pixel modes.
- Ability to power down LED matrix in sleep.
- Whether the integrated board geometry can place the center LED on the optical axis inside the hat.

## Phase 6 -- optics / gobo tests

Use Steve's existing filter/gobo rig or a simple repeatable setup.

For each LED candidate:

- Fix LED-to-filter distance.
- Measure/photograph projection at several ground distances.
- Test center-only white.
- Test center-only monochrome RGB.
- Test 3-pixel RGB chromatic fringing.
- Test 9-pixel / center crop.
- Test full-array animation mode.
- Test diffuser/no diffuser where applicable.
- Test matte-painted filter interior vs unpainted glossy PLA.
- Record subjective notes: crispness, washout, brightness, color fringing, visual beauty.

Acceptance target:

- Center-only mode must produce a recognizably crisp mandala at realistic height.
- Multi-pixel modes may be intentionally showy, but must not make the default mode look broken or washed out.

## Phase 7 -- RF tests

For each candidate MCU/hat arrangement:

- Bare-board ESP-NOW range/RSSI baseline.
- Mock-hat range/RSSI with panel installed.
- Mock-hat range/RSSI with battery installed.
- Mock-hat range/RSSI with screws/wiring in final-ish positions.
- Test multiple orientations.
- Test within a cluster of 5+ nodes if possible.

Record:

- RSSI distribution.
- Packet loss.
- Latency/jitter.
- Effect of solar panel and battery placement.
- Peer discovery stability.

Acceptance target:

- The final hat geometry must not bury the antenna under a solar panel, battery, or metal hardware.
- PCB antenna should remain the default. Do not switch to u.FL/external antenna unless RF tests fail.

## Phase 8 -- fault and fail-safe tests

For each plausible production architecture:

- Turn LEDs on, then deliberately hang firmware.
- Confirm watchdog reset occurs.
- Confirm LED rail can be shut off on reboot or low-battery detection.
- Confirm low-battery mode dims or disables LEDs before brownout loop.
- Confirm boot from low battery behaves gracefully.
- Confirm solar recovery from low/depleted battery.
- Confirm OTA failure rollback/recovery path.
- Confirm USB/pogo flashing recovery path.

Acceptance target:

- A stuck LED command must not be able to quietly drain a battery into an unrecoverable field state.
- Hardware default should bias external LED rails off until firmware intentionally enables them.

## Phase 9 -- mechanical / assembly tests

For each candidate stack:

- Place in a mock hat volume.
- Time assembly from loose parts to powered module.
- Count connectors/screws/solder joints.
- Identify operations requiring skill.
- Strain-relief solar panel leads.
- Shake/vibration test gently.
- Check serviceability: battery replacement, panel replacement, board replacement.
- Check connector retention.
- Check access to USB/pogo flashing.

Acceptance target:

- The production architecture can be assembled by a careful non-expert using a written checklist.
- Any soldering must be minimal, fixture-jigged, and not repeated across long header rows.

## Phase 10 -- data logging for 2027

For PowerFeather-like architectures, log enough to inform 2027 solar sizing:

- timestamp / uptime
- board ID
- firmware version
- battery voltage
- battery current
- SOC / remaining capacity
- battery temperature
- charger state
- solar/VDC input voltage
- charge current
- fault flags
- VSQT / LED rail state
- LED mode and brightness cap
- reset reason / brownout count
- ESP-NOW peer count
- RSSI summary
- enclosure temperature if available

Goal:

- Turn BM 2026 deployment into a dataset for BM 2027: real sun exposure, panel shading, dust impact, nightly drain, thermal behavior, and failure modes.

## Deliverables from this test plan

- Measured current table for each architecture.
- Measured solar/charge table for each panel.
- Gobo photo set for each LED module.
- RF/RSSI summary.
- Assembly-time summary.
- Production recommendation: COTS, custom, or hybrid.
- Open risks list before production decision.
