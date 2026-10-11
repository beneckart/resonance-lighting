# Magic Wand Conductor

Dedicated no-button fleet conductor for Steve's 20-NeoHex Magic Wand. This is
an exact-device image for WiFi MAC `68:EE:8F:F4:03:44` / short ID `F40344`.
The firmware refuses LED, power-configuration, and fleet-transmit behavior on
any other board.

Keep the conductor under Steve's or Ben's supervision while fleet commands are
unauthenticated. The two-sensor gesture rejects accidents; it is not an access
control mechanism for an unattended or freely handed-out wand.

The working `net-bench-2026-08-19.1` image remains installed until this target
passes a physical gesture and one-canopy canary. This source does not require a
fixture-fleet flash: it uses the packets already understood by the installed
fleet.

The 2026-10-06 source review fixed shock rejection during the final high hold.
Earlier retained builds predate this fix. The updated gesture tests pass
natively; build a new, separately identified artifact and complete the physical
canary before deploying this source. Historical binaries have not been changed.

## Deliberate gesture

There is no reliable software view of the existing Pololu LED toggle. The 5.1 V
LED supply branches from the BatterySpace PCM in parallel with the PowerFeather,
so its current bypasses the PowerFeather MAX17260 sense resistor. The common
battery voltage may sag slightly when the LEDs turn on, but a 15 Ah LFP makes
that an unsafe and environment-dependent control signal. This image does not
guess from voltage.

One deliberately corroborated gesture replaces the Atom button:

1. Hold the wand still for 1.6 seconds.
2. Within six seconds, raise it by at least 0.45 m. The MSA311 must observe a
   real translation; pressure drift alone cannot qualify.
3. Hold it still at least 0.35 m above the baseline for 0.8 seconds. The BMP581
   must retain the height evidence; motion alone cannot qualify.
4. Lower it within 0.20 m of the starting height before the next gesture. If it
   is deliberately kept at a new height, five still seconds rebase it there.

A hard shock at or above 0.90 g in the gravity-removed envelope cancels the
candidate. Missing or stale MSA311/BMP581 data fails closed. These are safe
source defaults, not hardware-qualified thresholds; record a trace while Steve
performs natural carries, pauses, and lifts before treating them as final.

The NeoHexes give gesture feedback: blue centers mean armed, the first two
rings show lift/high-hold progress, and cyan confirms acceptance.

## Fleet session

The first accepted gesture starts the same bounded session policy as the Atom:

- six minutes of Wake plus neighbor-report gathering;
- a five-minute explicit Wake renewal only while accepted gestures keep the
  15-minute local inactivity timer fresh;
- one-hour absolute maximum;
- ten seconds of repeated Auto cleanup.

The wand cannot deep-sleep after cleanup because neither its controller power
nor its LED toggle is wired to a wake-capable input. It returns to an idle local
animation with the radio listening; the next accepted gesture starts a new
session. Fixture-side expiry remains authoritative if the wand loses power.

During gather, all 20 NeoHex boards load from center to edge in the canonical
37-pixel spiral order. A bright head circles each board's current frontier ring,
so the slow six-minute progress remains visibly animated.

When ready, the next accepted gesture is selected by trusted fleet UTC:

- civil dawn through civil dusk -> nearest-neighbor canopy chime wave;
- civil dusk through civil dawn -> nearest-neighbor color wave.

The inspection fixture's one-hour pre-dusk lighting overlap remains chime time.
Missing, stale, malformed, or conflicting time refuses both actions. The target
set is fresh, class-confirmed downlights, with no battery-tier filter, exactly as
in ADR 0078.

At night the wand shows the next selection across all 740 pixels and advances
after each successful attempt:

```
red -> green -> blue -> dim RGB -> red
```

The first three local fills use channel value 6. Dim RGB uses `2,2,2`, keeping
the same total RGB sum and staying below Steve's validated all-pixel value-8
test. The fleet receives value 64 for a single channel and `21,21,21` for dim
RGB, with the existing hard-cut ten-second direct-frame micro-lease. The RGB
selection intentionally uses the RGB dies rather than the point-source W die.

## Build and test

Native gesture and visual-policy tests:

```sh
bash tests/run_tests.sh
```

Compile into a fresh retained directory:

```sh
bash build.sh --channel 11 --pulse-ms 40 \
  --tree-color-value 64 --tree-rgb-value 21 \
  --build-path build/wand-conductor-canary-r1
```

USB flashing is optional via `--port`, but the operator must first prove that
port is the exact `68:EE:8F:F4:03:44` board. The binary also checks the complete
MAC at boot. Do not OTA this source through a normal fleet batch; ADR 0050 still
requires a dedicated immutable artifact, the special-target acknowledgement,
and the wand as the only OTA target.

## Hardware validation required

- Record raw filtered pressure, MSA311 motion envelope, recognizer state, false
  triggers, and missed triggers during realistic handling; tune thresholds.
- Confirm every one of the 20 local spiral loaders follows the intended board
  orientation and that R/G/B/RGB fills remain thermally and electrically safe.
- Verify behavior with the LED toggle both on and off. With it off, the gesture
  controller still operates because switch state is not observable.
- Canary Wake/gather, trusted-time failure, one daytime chime, all four night
  colors, inactivity cleanup, absolute cleanup, and wand power loss against one
  explicitly named canopy before widening.
- If switch-as-input remains desirable, add a reviewed logic-level sense from
  the 5.1 V rail or Pololu enable state to a spare GPIO. Do not connect the 5.1 V
  rail directly to an ESP32 input.
- Keep the conductor supervised until command authentication, authorization,
  labeling, and lost-device revocation exist for portable fleet controllers.
