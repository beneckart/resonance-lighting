# Marill presence-to-bells bench test

Marill F40348 is the perimeter controller. ADR 0086 describes this canary.
It keeps ADR 0085's bounded PROTECT recovery. No receiver reflash is required:
the initial nine-receiver qualification used fx-261014-65eeb39-b. Brian's
October 15 local-time USB flash records, pulled October 16, confirm all nine
subsequently received fx-261016-d572656-b. Refresh per-device identity/revision
before selecting another update.

## Start and stop

- Leave the nine receivers powered and within radio range on channel 11.
- Use only one controller for this test; stop any Atom/tone-lab playback first.
- Open a Marill session with framed USB `!TF40348:60` for one hour. Minutes
  may be 1..120; `!TF40348:0` ends the session.
- USER while idle opens one hour. From sleep, USER wakes it and opens the
  session once power sampling is ready. That first press gathers the receivers;
  it does not queue a bell for later. Press again once ready.
- Allow at least one full 120-second receiver sleep cycle plus boot time for
  gathering. A receiver in ship mode needs its ordinary physical wake first.
- While active, USER, `!JF40348:1`, or a qualified hand wave advances one step.
- Sessions do not extend on every wave. After the session ends, Marill returns
  to its normal two-minute timer sleeps, with no active presence detection.
  Receiver radio leases expire within five minutes if the controller leaves.

## Interaction

Wave 5..40 cm from the VL53 sensor. Two fresh frames qualify the approach.
Move out of the 5..60 cm band for at least 0.8 seconds to rearm; allow at least
2.5 seconds between bells. A hand held in place fires only once. Fixed objects
in this band can prevent rearming. The original canary used a 15 cm minimum;
the range-tuning revision defaults to 5 cm. The graph reads the actual cutoff
from telemetry, with a 15 cm fallback for older canaries.

Set the bounds in millimeters with framed USB `!LF40348:50` (minimum),
`!UF40348:400` (maximum), and `!VF40348:600` (clear/rearm boundary).
Values must satisfy `30 <= minimum < maximum < clear <= 4000` at every step.
For example, raise clear before raising maximum past the existing clear
boundary; lower maximum before lowering clear below the existing maximum.
Minimum `:30` tries 3 cm and `:150` restores the original lower cutoff.
The sensor pipeline already discards distances below 30 mm, so lowering only
this detector cannot enable closer returns. Changes are RAM-only and require
clear-space rearm; they do not start/extend a session or advance the bell cycle.
Reboot restores 50/400/600 mm. Invalid ordering is refused without changing
the detector. Pause the live reader before using a separate USB writer.

Keep the laptop, table edges, and other fixed objects outside the sensor's
field of view. In the first bench test the laptop produced a persistent
35..40 cm return and unwanted detections. Confirm Armed on the graph with
the intended interaction space empty, then introduce the hand.

The order is Ness, Eevee, Rikku, Leeloo, Bidoof, Pinsir, Tangela, Sakura, Lucas,
then all nine. Every interaction requests one bell strike with Brian's tuned
pulse width. A rejected input does not advance. The all-nine step requires
fresh READY receipts from all nine; it does not silently skip missing members.

One pixel is amber while the next step is unavailable, green when ready, and
blue for an accepted input. A bell is scheduled about one second after the
input. Group strike commands share a deadline, but existing receiver loop
jitter and mechanical latency remain; acoustic timing is a listening test,
not a claimed precision measurement. Radio queue success is not sound proof.

## Live hand-range graph

```
python ops/bench/marill_presence_live.py --port 8766
```

Open `http://127.0.0.1:8766/`. The graph shows the nearest return at or beyond
the current minimum, the closest valid zone return, the trigger band, the rearm
boundary, and the 16 individual zones. It also displays the controller's actual rearm state,
next bell, fresh READY count, and remaining session time. No simulated data.
An interrupted USB connection shows stale data explicitly, not a flat live
trace. The viewer works on the old firmware too, labelled Preview only.

The viewer exclusively owns Marill's serial port and sends only telemetry
queries. It does not reset, wake, arm, or chime the device. Its default lifetime
is two hours. For separate USB operations, create `tmp/marill-live.pause`,
wait for the viewer to report Paused, then do the operation. Remove that file
to resume. Create `tmp/marill-live.stop` to close it; remove the stop file before
the next launch. These controls are local files, not network endpoints.

Compact timestamped readings are exclusive-created under
`ops/bench/data/Oakland/*-F40348-live-presence.jsonl`. Keep these as observation
records; they are not a measurement of per-receiver acoustic latency.

## OTA support and qualification

October 16 source review, including Brian's branch through 3285583, confirms
the targeted ESP-NOW maintenance command, shared-WiFi HTTP update endpoint
and maintenance loop remain in fx-261016-d572656-b. The recorded source is
ffa4e1e9a00a8f2baff043b7e5f5f882fbb62980; the recorded binary SHA-256 is
fcda081f589bd06c1f0ee0ae75043f8f822eaf74119267291d63d580982223bd.
Both the registry and Brian's build entry agree. All nine individual flash
logs under `ops/bench/data/Brian bench/20261016T02*-flash.jsonl` show the new
revision, valid app0/app1 state, no pending verify, and successful unchanged
64 KiB prefix verification. These were USB updates. The immutable binary and
manifest were not included in the pull; obtain those exact artifacts before
any upload, rather than rebuilding and reusing this revision.

Its maintenance.cpp is byte-identical to the previous 65eeb39 canopy source
and 220d90a Marill source. Daisy/Zubat's retained f951ae9 and Skitty's b3e2738
also retain OTA. This establishes code support, not a new radio-discovery,
WiFi-association or update test. All nine canopies still have
ota_verified=false; Marill's older true flag is not fresh OTA proof either.
Brian's Android-hotspot provisioning and one-device OTA rehearsal remain a
TODO, not a capability already deployed in this image. Use a configured
maintenance network and verify one exact-target update before expanding.

Use a maintenance-capable bridge such as the T-Deck/CoreS3 and shared WiFi
for wireless gathering; the presence UI and Atom trigger are not established
fleet-OTA controllers. Ordinary day sleep may require a full 120-second
cadence; PROTECT gathering uses 900 seconds plus margin and needs suitable
power. Ship mode requires its normal physical wake before radio maintenance.
Refer to [the OTA runbook](FLEET_OTA_10_MINUTE_RUNBOOK.md). Keep Skitty out of
all maintenance commands while its natural-recovery experiment is active.

## Serial test-command compatibility

The October 16 merge preserves Brian's deployed `!R<id>:0|1` Atom relay
toggle. Future builds use `!X<id>:1|2|3` for the separate, target-gated PROTECT
fault-injection test. Older Marill canaries (including 220d90a) still interpret
`!R` as fault injection. Tone Lab now requires fresh identity and explicit
relay-capability telemetry before sending that verb; do not send it manually
to an old canary as a relay command. No device was updated by this merge.
