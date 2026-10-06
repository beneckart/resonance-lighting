# Atom Conductor

One-button Atom Matrix controller for Ben's 2026 field use. It opens a bounded
fleet-control session, gathers the existing neighbor reports, and performs a
canopy-to-canopy chime or RGBW wave without a fixture-fleet flash.

This is deliberately separate from `atom_clicker/`. The clicker remains the
restricted one-target campmate proof. The conductor is a fleet controller and
must stay with Ben until command authentication and lost-device revocation are
implemented.

## Controls

The Atom has one pressable 5x5 face:

- From deep sleep, press once. That physical wake starts the six-minute gather.
- On a cold USB/power boot, short-press once to start the gather.
- During gather, the blue face fills over six minutes. A short press only shows
  that gathering is incomplete; it does not launch a partial-fleet effect.
- When ready, short-press to play the time-selected wave: chime before civil
  dusk, or the next RGBW color after civil dusk. Successful nighttime presses
  automatically advance red -> green -> blue -> white -> red.
- Hold for at least 0.7 seconds to preview/advance the next nighttime color
  without playing it.
- Hold for at least 2.5 seconds to send repeated Auto cleanup and sleep.

The ready face shows an amber bell before civil dusk or the next RGBW color
after civil dusk. A purple question mark means no trustworthy fleet UTC is
available; pressing then refuses both actuator families and flashes red rather
than guessing. Amber during gather means "not ready." Red also means the radio
failed or no fresh, class-confirmed canopy was available. A send is an attempt,
not proof that a mallet moved or a light rendered.

## Civil-dusk action gate

The fixture inspection image intentionally turns visible lighting on one hour
before evening civil dusk. The Atom does not call that overlap hour night:

- civil dawn through civil dusk -> chime wave;
- civil dusk through civil dawn -> RGBW wave.

The Atom has no trusted clock of its own. It listens for the fleet's existing
`NB_TIME_QUALITY` broadcasts from qualified GPS, RTC, or bridge sources, runs
them through the same consensus/expiry policy as the fixtures, and evaluates
the same Black Rock City solar calculation. The canonical schedule now exposes
true civil night separately from the inspection light's earlier `night` output.
If time is absent, malformed, conflicting, or stale, the Atom refuses to issue
either a strike or a light frame. Fixture lifecycle heartbeat consensus is not
a fallback because older Wake handling can force a reported daytime state and
the current inspection lifecycle enters its light window before civil dusk.

## Session contract

One session has three independent bounds:

1. The first six minutes send `NB_FORCE_LIFECYCLE` Wake every two seconds so a
   full 300-second field sleep cadence has a chance to hear it. During the same
   window, bounded `NB_LOCATE_CONTROL` requests gather fixture-to-fixture RSSI.
2. Every local button action -- playing a strike/color or advancing color --
   resets the Atom's 15-minute inactivity timer. Every five minutes while that
   timer remains live, the Atom sends a separate explicit Wake renewal. RGB and
   strike packet loops never send Wake themselves.
3. Fifteen minutes with no button input, or one hour from session start even
   with continuous use, enters a ten-second cleanup. Auto is repeated every two
   seconds, then the Atom deep-sleeps. Another face press starts a new session.

Fixture expiry remains the real fail-safe. If the Atom resets, loses power, or
misses cleanup, the current inspection firmware's control arm expires itself.
Older field images return through their ordinary inactivity sleep/reboot path.
An accepted strike can refresh that older generic receive hold, but only for a
bounded interval after the last received command; it does not extend the Atom's
session.

PROTECT remains fixture-owned. Wake does not keep a battery-powered PROTECT
fixture listening. A downlight that happens to be awake and fresh when a wave
is planned is included without a power-tier filter, so the deliberate strike is
attempted under ADR 0065. Local solenoid arm, pulse/rest, D7 collision, durable
load marker, and failsafe checks still decide whether the mechanism moves.

## Wave construction

The Atom does not ask fixtures to forward a new command. Existing updated
fixtures already support a bounded strongest-neighbor report. The Atom stores
the eight strongest directed observations per reporter, treats either direction
as a proximity edge, selects the canopy it hears most strongly as the origin,
and computes a breadth-first wave.

Only fresh, class-confirmed downlights are action targets. No battery tier is a
selection input. If survey data is disconnected, the next component starts in
a six-dB Atom-RSSI band. That fallback preserves fleet coverage while honestly
degrading to an Atom-centered radial ordering when the neighbor graph is sparse.

- Before civil dusk, a short press sends exact-target, six-copy, bounded
  `NB_TARGET_SOLENOID` requests in graph layers. Each receiving fixture attempts
  one pulse.
- After civil dusk, a short press sends `NB_DIRECT_FRAME` packets in graph
  layers, up to 18 targets per
  frame, with a hard-cut ten-second micro-lease. Red/green/blue use those RGB
  channels; white uses the downlight's dedicated W die. The next successful
  press advances R -> G -> B -> W. Fixture-side brightness and power caps remain
  authoritative.

The effect is centrally replayed from the Atom, so it works with the already
deployed packet contract. It is visually analogous to Color Wipe/Color Virus,
but it is not autonomous fixture-to-fixture forwarding.

## Build and test

Native policy/graph tests:

```sh
bash tests/run_tests.sh
```

Build for the known Atom Matrix target on channel 11. Build output must use a
fresh directory; the wrapper creates one when `--build-path` is omitted.

```sh
bash build.sh --channel 11 --pulse-ms 25 --color-value 64 \
  --build-path build/atom-conductor-field-25ms-r1
```

After inspecting `build.options.json` and the binary, optionally flash by adding
`--port COM42` or by using `arduino-cli upload` against the same build path. Do
not assume COM42 without checking the attached board.

## Hardware validation still required

- Prove GPIO39 face-button wake from deep sleep on the actual Atom + Atomic
  Battery Base and measure sleep current/runtime.
- Run one named canopy first, then a small visible roster, before a fleet chime.
- Prove trusted-time loss shows unknown and refuses both actions; inject UTC on
  both sides of civil dusk and confirm pre-dusk chime/post-dusk color selection.
- Compare the gathered origin/layers with physical neighbors at the tree.
- Confirm low/PROTECT awake nodes are best-effort and never held out of their
  power-owned sleep.
- Interrupt power before cleanup and prove fixture-side expiry recovers.
- Measure packet loss and queue drops during the 114-node neighbor-report burst.
- Keep this unit with Ben; do not hand it to campmates before authentication,
  authorization, labeling, and lost-device revocation are resolved.
