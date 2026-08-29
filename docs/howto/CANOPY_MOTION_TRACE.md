# Canopy motion trace field run

Use this runbook to measure a hanging downlight's wind-driven two-axis motion
against TMF presence and visible output. The test image is a short-lived,
exact-target instrument. It is never a fleet artifact.

## What the recorder captures

At the fixture's existing 25 Hz cooperative MSA311 cadence:

- raw X/Y/Z acceleration and the low-pass gravity vector, in mg;
- tilt from boot rest, in centidegrees, and the current sway envelope, in mg;
- TMF report sequence, closest depth/confidence, and all nine zone depths and
  confidences;
- the production presence latch and rising edge;
- lifecycle, program, power tier, and the rendered LED rail/color/pixel count.

The recorder stores 8,192 samples in PSRAM, about 5.5 minutes. If PSRAM
allocation fails, it explicitly reports a 1,024-sample, roughly 41-second
internal-RAM fallback. Recording occurs during ordinary mesh/show behavior.
Maintenance mode is used only afterward to drain the retained history, and it
keeps the LED rail off.

## Safety contract

1. Use Fleet Identify and physically confirm the named fixture is hanging and
   free to swing. Do not infer location from telemetry.
2. Record its exact six-digit short MAC, current firmware revision, profile,
   class, power state, and the exact prior artifact/SHA before any write.
3. Declare one OTA writer. The trace build must use
   `--msa-trace-target <SHORT_MAC>` and an immutable `-t` revision. The firmware
   disables recording if the physical short MAC differs from the compiled one.
4. Target that one MAC only. Require a production battery or another proven
   stable supply, exact endpoint identity, fresh post-reboot heartbeat, exact
   test revision, and survival through pending verify.
5. Never select either test or restore binaries by newest mtime or `latest`.
6. After downloading, restore the exact pre-trace fleet artifact unless Ben
   explicitly promotes a different named fleet artifact. Prove fresh rejoin,
   exact revision, pending-verify survival, profile, class, and sensor health.

## Suggested five-minute scene

After the test image has rebooted and TMF has had at least 30 seconds to learn
the hanging background, call out the wall-clock time at each transition:

1. 60 seconds: wind only; nobody under or near the chosen cone.
2. 60 seconds: one person stands still under the nominal fixture position.
3. 60 seconds: that person walks slowly across and back through the swept cone.
4. 60 seconds: person leaves; observe white hold and release.
5. Up to 60 seconds: a second person or a deliberately started gentle swing,
   only if physically safe and useful.

Keep the scene under five minutes so the PSRAM ring retains the whole run.
Write the transition times into the field note immediately; the trace itself
uses monotonic fixture uptime, while the OTA and capture ledgers provide UTC
bookends.

## Download

Enter exact-target maintenance only after the scene. Discover and freeze the
one target, then download the history with `ops/bench/capture_motion_trace.py`.
The tool requires the maintenance IP, exact short MAC, and exact `-t` revision;
it refuses identity/revision/role/MSA/target mismatches and exclusive-creates
the JSONL output.

Example shape (replace every value with the current named run):

```text
python ops/bench/capture_motion_trace.py \
  --host 192.168.1.123 \
  --target ABCDEF \
  --expect-fw fx-260829-1234567-t \
  --history-s 300 \
  --label wind-still-walk-leave \
  --out "ops/bench/data/Black Rock City/2026-08-29-abcdef-canopy-motion.jsonl"
```

Do not use `--live-s` for the primary artistic trace: behavior is paused in
maintenance, so live samples describe a dark maintenance fixture. It exists
only for sensor/readout diagnostics.

## First analysis questions

- What are the two dominant swing periods and how stable are they?
- Does raw/gravity-vector direction predict which TMF zones see a person?
- How often does empty wind create a 300 mm per-zone approach signature?
- How long is a stationary person visible during each cone sweep?
- Does visible white chatter, or does the current latch bridge adjacent sweeps?
- Can sway drive color/phase without attenuating brightness or vetoing the
  desirable wind-assisted scanner behavior?

Prefer modulation from swing phase, direction, hue, or temporal accents. Do not
make MSA motion a blanket brightness reduction or ToF veto.
