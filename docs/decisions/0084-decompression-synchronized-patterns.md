# ADR 0084: Synchronized decompression patterns with an elected conductor

Date: 2026-10-14
Status: Proposed (Brian + Claude; Ben approved starting firmware work
2026-10-14). Not accepted for deployment until bench-qualified.

## Context

Brian is composing repeating rhythmic patterns for the decompression
lanterns in the tone lab (`ops/bench/decompression_tone_lab.py`, Patterns
tab). Today those patterns play only over USB: the laptop sends one immediate
`K<id>:<ms>` per hit. Measured send-to-sound latency is 48-89 ms per lantern
with occasional +70-110 ms outliers, because plain `K` waits on the firmware
main loop (LOG 2026-10-13). ADR 0082 gives one lantern a precise local phrase
(about 60 us onset error) and ADR 0083 starts that phrase by radio, but both
explicitly leave synchronized multi-lantern timing to a separate contract.

Brian wants a pattern set up from the tone lab to keep playing in time after
the lanterns are unplugged, with no laptop or dedicated controller, and the
design to scale beyond the current nine lanterns.

## Decision

Add a pattern mode to decompression builds (`RES_DECOMPRESSION`), with three
parts.

### 1. Each lantern stores only its own part

A part is: pattern id (nonzero, shared by every lantern in the pattern), loop
length (0.25-60 s), solenoid pulse (5-100 ms), a mechanical lead (0-200 ms,
fire this much early so slower mechanisms land on the beat) and up to 32 hit
offsets within the loop. A lantern with zero hits is a silent member: it still
keeps time and may conduct. Hits on one lantern must be at least 250 ms apart
including the loop wrap, the tightest spacing the signature already uses.

The part is uploaded over USB with one bounded, exact-target line and stored in
NVS, so it survives resets and power loss. The firmware echoes a CRC32 of the
stored part so the host can verify every lantern holds what was sent. Radio
upload is deferred.

    !P<id> SET <pattern-id-hex> <loop-us> <pulse-ms> <lead-us> <offsets-us|->
    !P<id> PLAY <pattern-id-hex> <minutes 1..120>
    !P<id> STOP
    !P<id> CLEAR

### 2. One shared group clock, kept by an elected conductor

Every lantern in a pattern session keeps a group clock: its own microsecond
timer plus an offset. One lantern, the conductor, broadcasts an
`NB_PATTERN_BEAT` (new type 36) every 500 ms on the existing ESP-NOW broadcast
channel. Each beat carries the pattern id, loop length, the conductor's boot
nonce, a beat sequence number and the group time at which the *previous* beat
actually left the radio (taken in the send-complete callback). Followers
timestamp each beat on receipt and, when the next beat arrives, compute
offset = previous-beat tx time - previous-beat rx time. The median of the last
five samples sets the follower's offset. This two-step ("follow-up") scheme
removes the variable queueing and channel-access delay before transmission;
what remains is a near-constant air/callback latency common to all followers.

Election needs no extra traffic:

- A lantern starting a session listens for 1.5 s (plus an ID-derived stagger).
  If it hears no beat for its pattern it becomes the conductor, keeping its
  current group clock.
- Followers follow the lowest short ID that is beating. A conductor that hears
  a lower ID beating for the same pattern yields and follows it.
- A follower that hears no beat for 1.6 s (plus stagger) takes over, keeping
  its group clock, so the loop phase continues seamlessly. Briefly overlapping
  conductors share the same clock and resolve to the lowest ID.
- A lantern out of range conducts for itself and re-merges, snapping to the
  lowest-ID group, when it hears it again.

Beats from a different pattern id or loop length are ignored, so separate
groups can run different patterns side by side.

### 3. Hits are fired from a local timer on group time

Loop phase is group time modulo loop length. A 1 ms `esp_timer` (the ADR 0082
mechanism, no I2C, NVS, allocation or logging in the callback) fires each hit
at (hit time - lead) in group time. A hit more than 20 ms late is skipped, never
replayed or compressed; a clock correction can never re-fire a hit already
played. A lantern plays only once it is locked: a conductor after 1.2 s of
uncontested conducting, a follower after two offset samples.

## Safety and power

- Pattern sessions are RAM-only, last 1-120 minutes, and do not resume after a
  reset, matching ADR 0082. The stored part persists; playback must be
  restarted.
- Start and every tick require the same readiness as the signature: battery
  present, FULL/DIM tier, comms mode, no pending OTA, boot guard not parked.
  Existing power vetoes stop playback.
- The durable load marker is armed in loop context before and during playback,
  never in the timer callback.
- While a pattern plays, single strikes, the signature and USER phrases are
  refused, and sleep is suppressed for the session.
- Radio is unauthenticated like the rest of the mesh; a beat can only shift a
  lantern's timing for a pattern it already holds and was told to play.

## Consequences and open items

- New canonical packet type 36 `NB_PATTERN_BEAT` (append-only, layout pinned in
  `test_packet_layout.cpp`). Existing types are unchanged.
- Expected cross-lantern alignment is about 1 ms, bounded by beat receipt
  latency spread and crystal drift (tens of ppm, about 10 us per beat period).
  This must be measured: microphone timing in the tone lab and, ideally, GPIO
  capture on two lanterns.
- Mechanical lead must be calibrated from the locally timed path, not USB `K`
  latency. Until then the host may use relative USB latencies as an estimate.
- Continuous playback costs event energy (radio awake, strikes, capbank refill);
  runtime at realistic pattern density is unmeasured.
- Open: radio upload of parts, starting/stopping a pattern from the ADR 0083
  Atom, whether a session should survive a reset, and conductor behavior on
  very large groups (beat airtime is small; election is O(1) per lantern).

## Qualification

Native tests for parsing, validation, scheduling, synchronization and election
(simulated clocks with skew, delay and dropped beats). Then one lantern on USB,
two lanterns on battery measured by microphone and GPIO, conductor loss and
re-merge, and finally all nine unplugged for a sustained run with energy
measured. A successful build or flash is not qualification.
