# Archived One-Off Perimeter Gobo USB Controller

**Status:** Historical stopgap, retained for posterity. This utility was host
self-tested but was not hardware-validated through the installed panel-mount
USB-C extension. It is not a supported production workflow; prefer current
fleet/bridge controls when they are available.

Use this Windows utility at the tree when there is no Starlink, T-Deck, or other
network path. It controls one USB-connected production fixture directly through
the fixture firmware's existing serial commands. It does not flash firmware,
change NVS, join WiFi, or need internet access.

## Start it

1. Connect exactly one perimeter fixture to the laptop by USB.
2. Double-click `perimeter_gobo_usb.cmd` in this directory.
3. Wait for the green line naming the exact six-hex fixture ID and reporting
   class `perimeter`.
4. Press **START DANCING GOBO**.

The app automatically selects the only serial port when there is exactly one.
If Windows exposes more than one COM port, select the fixture explicitly and
press Connect.

## Buttons and exact effects

- **START DANCING GOBO** sends `L1`. On a perimeter fixture, the production
  smoke renderer walks one white pixel through the HEX spiral, giving the
  intended moving-shadow gobo test.
- **STOP / LED RAIL OFF** sends `L0`. This immediately turns the LED rail off
  and deliberately leaves the current boot forced dark.
- **RETURN TO NORMAL (REBOOT)** sends `L0`, waits for rail-off processing, then
  applies the documented USB RTS reset pulse. RAM-only bench state clears and
  normal field behavior resumes after boot.
- **Read status** sends only the read-only `t` telemetry query.

The controller also requests `t` periodically while connected so its identity
and safety display remain fresh.

## Safety gates

Start remains disabled unless fresh telemetry proves all of the following:

- a valid fixture short ID;
- production fixture peer firmware;
- class `perimeter`;
- PowerFeather initialization ready;
- not a deep-recovery build;
- boot guard below PROTECT; and
- power tier FULL or DIM.

Fixture-side boot, battery, rail-ramp, and load-marker vetoes remain
authoritative even after the app sends `L1`.

If USB reset does not restore telemetry, use the fixture's physical reset or a
power cycle. Do not leave after pressing only Stop: `L0` intentionally remains
dark until reset.
