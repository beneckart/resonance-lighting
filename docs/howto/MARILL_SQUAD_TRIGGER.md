# Marill presence-to-bells bench test

Marill F40348 is the perimeter controller. ADR 0086 describes this canary.
It keeps ADR 0085's bounded PROTECT recovery. No receiver reflash is required:
the nine receivers were last recorded on fx-261014-65eeb39-b.

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

Wave 15..70 cm from the VL53 sensor. Two fresh frames qualify the approach.
Move out of the 15..80 cm band for at least 0.8 seconds to rearm; allow at least
2.5 seconds between bells. A hand held in place fires only once. Fixed objects
in this band can prevent rearming. Close returns under 15 cm are excluded to
avoid Marill's observed ~10 cm housing return.

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
15 cm, the raw closest return, the trigger band, the rearm boundary, and the
16 individual zones. It also displays the controller's actual rearm state,
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
