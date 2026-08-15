# Atom Matrix reduced-access clicker

Proof-of-concept campmate remote for the Resonance solenoids. The pressable 5x5
Atom Matrix face sends only `NB_TARGET_SOLENOID` for one build-time fixture ID
and one bounded pulse width. It has no WiFi, OTA, serial command parser, fleet
maintenance, configuration, sleep, or show controls.

The first proof targets `9E5B8C`, channel 11, at 40 ms. The fixture remains the
safety authority: it checks exact target ID, lifecycle/power policy, armed state,
pulse clamp, rest time, timer cutoff, and loop failsafe. The clicker sends six
copies over 40 ms for RF robustness; the fixture's active-pulse/rest guards make
the burst one strike rather than six.

## Face

- Press and release are required; boot-held and held buttons do not repeat.
- 35 ms debounce and a 1 s click interval prevent bounce/double presses.
- Blue center: radio ready, target heartbeat not fresh.
- Green center: target heartbeat heard in the last 15 s.
- Amber face: command burst sent. This is not a target acknowledgement.
- Red center/face: radio failed or the click was refused locally.

## Build and flash

Use a unique build directory. The first Atom Matrix on this bench enumerated as
COM42 and identified as ESP32-PICO-D4 with MAC `14:08:08:54:AD:9C`.

```sh
bash build.sh --channel 11 --target 9E5B8C --pulse-ms 40 \
  --build-path build/nc-atom-clicker-9e5b8c-r1
arduino-cli upload --fqbn esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs \
  --port COM42 --build-path build/nc-atom-clicker-9e5b8c-r1 .
```

The build script can combine those steps with `--port COM42`; the split form is
preferred after auditing `build.options.json` and the binary.

## Not yet production-ready

The Atomic Battery Base is only 200 mAh. Before distributing clickers, add and
measure an explicit low-power policy, decide how a button wakes the radio, define
target provisioning/labels and lost-device revocation, add useful delivery
feedback without pretending ESP-NOW broadcast send status is an actuator ACK,
add real command authentication/fixture authorization (the current fleet packet
is not authenticated), and test RF coexistence/coverage with multiple clickers
and the installed fleet.
