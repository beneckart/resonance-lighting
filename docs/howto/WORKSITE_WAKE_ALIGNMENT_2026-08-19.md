# Worksite wake-alignment hand-off — 2026-08-19

**Audience:** Elliot + the agent driving his Mac mini (CoreS3 bridge on USB).
**Written by:** Ben + Claude, Oakland, Wed 2026-08-19 evening.

**Goal, in one paragraph:** Some fixtures at the worksite are awake early — six
we woke on purpose at pack-out, plus a few that have apparently trickle-charged
back to life. Inventory what is awake, flash the ones on old firmware that are
safe to flash, and then put everything awake back into transport sleep timed so
it wakes together with the rest of the fleet on **Friday night**. The sleep
command is the one dangerous step: it is a fleet-wide broadcast and its hour
count must be computed at send time. **Do not send it without Elliot's explicit
per-send confirmation** (hard gate in Step 4).

All times below are Pacific (PDT). Nevada City and Black Rock are both
America/Los_Angeles.

---

## 1. Ground truth from pack-out (LOG.md, 2026-08-17 entry)

- Fleet firmware image: **`fx-260818-f80f315-b`**, 1,177,264 bytes, binary
  SHA-256 `0f1119c6ba80f2280db2c04f478a59b6be0c407edf6c95c62248f89af90ad638`,
  built once from clean commit `29ebe2b5949bc52b03690986ea8ecbdcbecf4a65`.
  The binary is credential-bearing and is **not in the repo**. If the Mac mini
  does not already have a copy of `firmware/fixture/build/fx-260818-f80f315-b/`,
  get it from Ben. **Never rebuild it** — one `fw_rev` maps to exactly one
  SHA-256 (`docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md`). Verify before use:
  `shasum -a 256 fixture.ino.bin` must match the hash above.
- At **~20:11 PDT Mon 2026-08-17**, 84 fixtures on that exact image accepted a
  broadcast `Q99` (99 h transport sleep). Target timer wake:
  **~23:11 PDT Friday 2026-08-21**. They wake radio-on but **LED-dark** (an
  RTC-retained latch) until a bridge program release (`b`).
- "About 23:11" is a target, not a deadline: there was a few minutes of command
  spread, and the deep-sleep timer runs on the ESP32's internal RC clock —
  over 99 h with container temperature swings, tens of minutes of drift is
  plausible. Expect a ragged wake window centered near 23:11, and stragglers.
- 13 of the 97 observed identities did **not** get the exact image (old,
  intermittent, low, or unreachable). 10 of those took the legacy `S65535`
  sleep (~18.2 h) and therefore woke around **~14:20 PDT Tue 2026-08-18** on
  old firmware. **Old firmware does not understand transport sleep (type 27)
  and silently ignores `Q`.** These are the most likely identity of any
  "newly awake and lit" fixture.
- The six deliberately-woken test-cohort fixtures, all verified on the exact
  image (`9F2720` via USB commissioning on 8/17):

  ```
  9E5954  9F0E30  9F0E5C  9F26E4  F40174  9F2720
  ```

  These need **no flash** — they only need to be re-slept (Step 4).

## 2. Setup on the Mac mini

```sh
ls /dev/cu.usbmodem*          # find the bridge serial port
pip3 install pyserial          # dashboard's only non-stdlib dep
python3 ops/bench/net_bench_dashboard.py --port /dev/cu.usbmodemXXXX --http-port 8765
```

Then open `http://127.0.0.1:8765`. Raw bridge commands can go through the
`Detailed diagnostics` serial console in that UI, or
`curl -s -X POST http://127.0.0.1:8765/api/cmd -d '{"cmd":"Q52"}'` style POSTs
(the endpoint validates `Q1`–`Q168`).

**Bridge sanity check (do this first):** the boot banner must report
`cores3-bridge-2026-08-17` or newer, and the bridge's command help line must
list `Q<hours>`. The `Q` command was added 2026-08-17; an older bridge build
cannot issue transport sleep. If the banner is older, stop and call Ben — the
bridge needs a reflash before any of this works.

Per `FIRMWARE_ARTIFACT_HANDOFF.md`, state-changing work has one operator at a
time: Elliot owns this session; Ben will not issue commands from Oakland while
it runs.

## 3. Step 1 — inventory who is awake

Let the dashboard sit for **3–5 minutes** (fixtures heartbeat slowly in some
postures), then record for every fresh tile: short MAC, `fw_rev`, battery mV,
charger input, LED-rail/lit state, and low-VBAT recovery state (select a tile
for exact values). Classify:

| Bucket | Signature | Meaning | Action |
|---|---|---|---|
| A | one of the six IDs above | known early-wake cohort | re-sleep only |
| B | `fx-260818-f80f315-b`, **dark**, radio-on | woke early with the transport latch intact (reset in handling, or timer anomaly) | healthy; no flash; re-sleep |
| C | `fx-260818-f80f315-b`, **lit**, not in bucket A | true power loss cleared its RTC latch (battery hit zero, then recharged) — boots lit | no flash; re-sleep |
| D | old revision, **lit** | legacy `S65535` sleeper or trickle-charged holdback, running its normal cycle | flash candidate (Step 2) |
| E | old revision, **dark** | low battery, or dark posture on old firmware | triage battery (Step 2) |

Ben's guess "lit means OK to flash" is right, with the reasoning: the LED ramp
guards sit on the ADR 0046 ladder, so a lit fixture is above the ~3.15 V
load-compensated dim floor — far above the OTA tool's 2.5 V preflight.

## 4. Step 2 — flash triage (buckets D and E only)

- **Already on `fx-260818-f80f315-b` → do not flash.** There is nothing newer
  to put on it; the goal is homogeneity with the fleet image, not novelty.
- **Old revision, lit → OK to flash.**
- **Old revision, dark → check the tile's exact values:**
  - battery ≥ 2.5 V → OK to flash (the tool preflight enforces this anyway);
  - battery 2.2–2.5 V **and** healthy external input ≥ 4.6 V / ≥ 50 mA → OK;
  - below that, or ADR 0042 low-VBAT recovery active, or downlink anomalous →
    **do not flash**. Record the ID and leave it for Ben. (Pack-out precedent:
    the 8/17 waves excluded low-voltage, active-recovery, downlink-anomalous,
    and slot-anomalous fixtures.)
- An unflashable old-firmware fixture also **cannot be put to sleep** (it
  ignores `Q`, and the legacy `S` maxes out at ~18.2 h, which does not reach
  Friday night). Leave it awake, note the ID, and let Ben decide. Now that
  fixtures are out of the dark container and on solar, awake-drain is an
  annoyance, not the container death-spiral.

## 5. Step 3 — OTA the flashable old-revision units

Skip this whole step if buckets D/E produced no flashable IDs.

Prerequisites:

- The 2.4 GHz AP whose credentials are compiled into the image must be up
  (the artifact manifest's `wifi_profile` names the credential set; Ben can
  confirm the SSID). Fixtures leave ESP-NOW and join it in maintenance mode,
  so AP channel does not matter for this step (ADR 0036 §5).
- The Mac mini must be on that AP's subnet (GL.iNet Beryl default is
  `192.168.8.0/24`).
- The dashboard from Step 2 must still be running (the OTA tool drives it).

```sh
python3 ops/bench/fleet_dashboard_ota.py \
  --targets <COMMA-SEPARATED SHORT MACS — explicit, never "all"> \
  --bin  <path>/fx-260818-f80f315-b/fixture.ino.bin \
  --expect-fw fx-260818-f80f315-b \
  --dashboard-url http://127.0.0.1:8765 \
  --subnet 192.168.8.0/24 \
  --site brc --notes "worksite wake alignment 2026-08-19"
```

The tool hails each named fixture into maintenance, uploads in parallel, and
only counts success after a fresh exact-revision heartbeat survives the
pending-verify window. Post the OWNER/ARTIFACT/TARGETS callout from
`FIRMWARE_ARTIFACT_HANDOFF.md` before starting. Freshly flashed fixtures reboot
awake (lit or dark as they were) — that is fine; they now understand `Q` and
will be captured by Step 4.

## 6. Step 4 — re-sleep everything awake (HARD CONFIRMATION GATE)

Three facts to hold in your head:

1. **`Q` is broadcast-only.** The bridge sends target `00:00:00` — "one
   fleet-wide operation" — so **every** awake, radio-reachable fixture on new
   firmware sleeps, not a chosen subset. There is no per-fixture form. Here
   that is what we want (buckets A/B/C plus the freshly flashed), but confirm
   nothing in radio range should stay awake before sending.
2. Old-firmware fixtures ignore `Q` silently. Do not count a fixture as
   transport-slept unless it actually vanishes from fresh telemetry (ADR 0045).
3. Entering transport sleep sets the dark latch, so everything re-slept — even
   the currently-lit six — wakes **dark** with the fleet, which is correct.

**Timing.** Target wake: **Friday 2026-08-21 23:11 PDT**. `Q` takes whole
hours (1–168). Compute the remaining hours **at the moment of sending** and
**round down** — waking early costs ~130 mA of dark-awake radio per hour
(~0.13 Ah, negligible); waking late means missing the coordinated release.

```sh
# Verify the Mac's clock first: `date` must print PDT and the correct time.
python3 - <<'EOF'
from datetime import datetime
import math
target = datetime(2026, 8, 21, 23, 11)   # fleet wake, local Pacific time
h = (target - datetime.now()).total_seconds() / 3600
print(f"remaining {h:.2f} h -> send Q{math.floor(h)}")
EOF
```

Worked examples (send time → command → that cohort's wake):

| Sent (PDT) | Remaining | Send | Wakes Fri (PDT) | Early by |
|---|---|---|---|---|
| Wed 8/19 21:11 | 50 h 00 m | `Q50` | 23:11 | 0 |
| Wed 8/19 22:00 | 49 h 11 m | `Q49` | 23:00 | 11 m |
| Thu 8/20 09:00 | 38 h 11 m | `Q38` | 23:00 | 11 m |
| Thu 8/20 15:30 | 31 h 41 m | `Q31` | 22:30 | 41 m |
| Fri 8/21 10:00 | 13 h 11 m | `Q13` | 23:00 | 11 m |

Sanity band: sent Wednesday evening the number is ~50; Thursday it is in the
40s–30s; Friday morning the teens. **If your computed number is outside what
this table implies for the current time, your clock math is wrong — stop.**
If you have slipped inside ~2 h of the fleet wake, skip re-sleep entirely and
call Ben instead — the granularity is too coarse to be worth it.

> **AGENT GATE — do not skip.** Before sending `Q`, print to Elliot: (a) the
> exact command (e.g. `Q49`), (b) the computed wake time it produces, (c) the
> fleet target 23:11 Fri and the early-by delta, and (d) the list of fixture
> IDs you expect to go silent. Then **wait for Elliot to explicitly type yes**.
> No timeout-default, no "he probably meant yes." This command puts hardware
> to sleep for two days; a wrong hour count strands fixtures asleep or wakes
> them into a dead container window. If anything about the computed number
> feels off, stop and reach Ben first.

Send the command (console or `/api/cmd`). The bridge echoes
`broadcast TRANSPORT_SLEEP <n>h (...)`. Within a minute or two the sleepers
drop out of fresh telemetry. Record which intended IDs went silent. For
stragglers still heartbeating: re-send is safe, but **recompute the hour count
fresh** — never reuse an old number.

## 7. What happens Friday night

- Around 23:11 PDT Friday (± the drift noted above) the container fleet and the
  re-slept cohort wake radio-on and LED-dark.
- Nothing lights until a bridge sends bare **`b`** (broadcast program release —
  it clears the retained latch fleet-wide). Whoever runs the wake-up chooses
  the moment; that is the whole point of the alignment.
- **Until then, do not send `b`** (or anything intended to push light —
  identify/tag/direct frames — at latched-dark fixtures) unless you mean to
  light them up early.

## 8. Gotchas and reporting

- Charge-only USB / solar does not wake a sleeping fixture; the BQ25628E keeps
  charging through transport sleep. A physical RESET press wakes it but keeps
  it dark (latch retained). A true battery pull clears RTC state — that
  fixture will boot **lit**.
- The dashboard's legacy `S<seconds>` sleep is 16-bit (max ~18.2 h). Do not
  use it for wake alignment.
- Do not run an `L` RSSI survey in the same session as `Q` — survey traffic
  must be finished before transport sleep (ADR 0045).
- Report back to Ben when done: which IDs slept (and the `Q` value + send
  timestamp), which were flashed (with OTA results), which were left awake and
  why (low battery / recovery / unreachable / old firmware unflashable).

Provenance for every claim above: `LOG.md` 2026-08-17 pack-out entry,
ADR 0045 (transport sleep), ADR 0046 (power ladder), ADR 0042 (low-VBAT
recovery), ADR 0036 (camp AP), `firmware/cores3_bridge/README.md` (command
set), `ops/bench/fleet_dashboard_ota.py` (preflight thresholds),
`docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md` (artifact + operator rules).
