# tdeck_bridge — Resonance Bridge OS (LilyGO T-Deck Plus)

App-launcher handheld per ADR 0047 (platform of record; constraints from ADR
0037): simultaneously an ESP-NOW mesh citizen on the fleet channel (census +
command TX) and a Wi-Fi STA (Claude API over TLS, laptop services).

**Status 2026-08-24:** M0-M4 complete and hardware-verified. Working apps:
**Claude** (streaming chat + 6-tool agent loop with the confirm rail),
**Fleet** (live census, reported-color chips, node detail + identify,
Sleep/Dark entry point), **Health** (single-screen voltage health grid for the
production registry plus live node detail), **LED Studio** (class-targeted solid colors and 1 Hz
cohort blink via sustained 8 Hz direct-frame streaming, client-side dim),
**Sleep / Dark** (confirmed 10 min / 1 h / 4 h / 8 h / 12 h dark leases or
rails-off timer sleep), **Knocker** (single strike plus a confirmed,
deterministic targeted roll across the full fresh census; not synchronized),
**Time / Schedule** (GPS UTC status plus Auto / Day Dark / Night Show),
**CA Studio** (program leases + GH-CA knob params via the release-re-lease
workaround), **Patterns** (manual deterministic RGBW streaming),
**RF Diagnostics** (read-only mesh survey), **Settings**, **SunTest**.
Remaining: Locate, detailed sensor reports, ES7210 audio-reactive Patterns,
voice (whisperd), and polish (M5 tail + M6).

The combined Health, UTC/Schedule, Patterns v1, and RF Diagnostics image is
USB-flashed to exact T-Deck `8EB508` as `tdeck-dev-local`: 1,542,448 bytes,
binary SHA-256
`705119167e51ae8dff399a6c46cfd442b1610d14d0acb5d8a470c63461242b46`.
The upload verified, all onboard peripheral probes passed, and the bridge
rejoined channel 11 with live fleet receive and zero observed TX failures. Ben
reports the preceding Health/Schedule physical smoke check looked good; the
explicit acceptance matrices remain in `TODO.md`.

The current source checkpoint adds permanent operator callsigns and recognizes
`Thor [F40344]` as the protected one-off `magic_wand` role. Its 1,549,728-byte
binary, SHA-256
`b416c9642ceb1808dce66e84c21224cfce32d1d5685044fe81487eb35047a6eb`,
is USB-flashed to exact T-Deck `8EB508`. The upload verified, all onboard
peripheral probes passed, and the bridge rejoined the channel 11 mesh with zero
send failures. Callsign UI and named-command hardware acceptance remain open in
`TODO.md`.

Ben field-smoke-tested LED Studio, Sleep / Dark, Knocker, and CA Studio on the
night of 2026-08-23/24; all behaved as designed. That run exposed the old
Knocker `knock all` behavior: it selected at most 32 fresh fixtures in
heartbeat order and then dispatched a per-ID targeted request every 300 ms.
Current P0 source replaces it with an honestly labelled, deterministic 80 ms
targeted roll over the full 192-entry census. It still obeys every fixture's
daytime/power strike gates and is not synchronized. The current source passes
the full native suite and a complete T-Deck firmware build; only the hardware
recheck remains.

**Board: T-Deck Plus, LCD variant — NOT the T-Deck Pro** (e-paper; different
touch/keyboard drivers; the names are one word apart and that is the easiest
available mistake — ADR 0037 §10).

## Hardware verdicts (probed on real hardware, 2026-08-19)

| Peripheral | Verdict |
|---|---|
| PSRAM | 8 MB OPI, detected (`psram=1(8388608)`) |
| Keyboard aux MCU | present at I2C 0x55 |
| GT911 touch | present (0x5D) |
| ES7210 mic ADC | **present at 0x40** — Patterns audio reactivity is unblocked |
| GPS | **NMEA at 38400 baud**, pins as named in `pins_tdeck.h` (not swapped) |
| Display | ST7789 320x240 via hand-configured LovyanGFX, PSRAM canvas |
| First build | 1.04 MB flash (33% of 3 MB app), 50.8 KB static RAM, heap free ~263 KB, PSRAM free ~8.2 MB |

## Build / flash

```bash
firmware/tdeck_bridge/build.sh --dev-cache           # fast locked local build
firmware/tdeck_bridge/build.sh --dev-cache --port COM152  # one named USB target
firmware/tdeck_bridge/build.sh                       # fresh retained build
firmware/tdeck_bridge/tests/run_tests.sh             # wrapper + native tests
```

- FQBN is pinned in `build.sh`: generic `esp32:esp32:esp32s3` with
  `FlashMode=qio,FlashSize=16M,PSRAM=opi` (ESP32-S3FN16R8 — a wrong PSRAM mode
  boot-loops), `PartitionScheme=app3M_fat9M_16MB`, USB CDC on boot.
- Fresh builds get a unique `--build-path`; an explicit retained path must be
  new or empty. Never resume a killed build directory.
  Uncached ESP32-S3 builds take 2-3 minutes -- wait, don't restart.
- `--dev-cache` is the opt-in local iteration path ported from the accepted
  fixture cache. It is single-writer, recipe-pinned to the FQBN, flags,
  Arduino/ESP32 versions, LVGL, and LovyanGFX, and always reports
  `tdeck-dev-local`. The first seed is still a cold build; subsequent no-op and
  leaf builds reuse completed objects. On the 2026-08-24 field laptop run, the
  cold seed took about 55 minutes under heavy host load and the first warm
  no-op took about 76 seconds with an identical SHA. Warm reuse is proven, but
  timings varied significantly during the later pre-flash check.
- If a cached build is interrupted, first confirm no Arduino, Xtensa, or
  esptool process remains, then run `build.sh --recover-dev-cache`. Recovery
  quarantines the cache and lock; it never resumes partial objects. Use
  `build.sh --clean-dev-cache` only for a healthy unlocked cache.
- The wire contract is included as `fixture/src/core/packet.h` via `-I` to the
  firmware root. One contract, one file; `tests/test_packet_include.cpp` pins
  the golden sizes on the native side.

## Provisioning (NVS only — no secret is compiled in or committed)

Serial 115200, line-based:

```
set wifi <ssid> <psk>     # ssid/psk must each be one token
set key <anthropic-key>
set model claude-sonnet-5
set channel 11            # mesh channel; 11 = commissioned fleet
set bl 200                # backlight 0-255
wifi retry | wifi off
show                      # api key redacted
probe | mem | reboot | help
```

## Channel guard (ADR 0036/0037)

One 2.4 GHz radio; in STA mode the AP picks the channel. On association the
firmware compares the AP channel to the stored mesh channel:

- match → ONLINE (SNTP starts, mesh stays up alongside)
- mismatch → **Wi-Fi dropped, mesh kept**, radio re-pinned, red
  `GUARD:ch-mismatch` on the status page. No auto-retry against a wrong-channel
AP - fix the AP (`docs/howto/CAMP_NETWORK_SETUP.md`), then `wifi retry`.

## Field LED Studio and rest controls (ADR 0048)

Open **LEDs** from the launcher (or **Sleep / Dark** from Fleet):

- LED Studio targets all fresh fixtures or one reported class: downlights,
  perimeter, uplights, or chandelier. Pick a labelled color, set dim, and use
  solid or 1 Hz blink. The class map comes from full heartbeats and can take
  about 60 seconds to fill after bridge boot. Streams cover the complete
  192-entry census, not only the first screenful.
- Dark is an expiring electrical-dark lease. The LED rail is off, but the
  ESP-NOW receiver stays awake and measured fleet draw remains roughly
  126-144 mA per fixture.
- Low-power sleep cuts both switchable rails and the radio for the selected
  duration, then auto-wakes into normal behavior. It cannot be cancelled while
  the fixture is asleep. It uses the existing `NB_SLEEP_FOR` contract, not the
  transport-dark latch.
- Both actions show live/seen counts and require the on-device confirmation
  modal, with focus on cancel. Only currently listening fixtures can receive a
  broadcast. Starting either action stops any suspended LED Studio stream.
- Sleep remains absent from the Claude tool schema and serial quick commands;
  it is reachable only from the local physical UI.

## Fleet Health

Open **Health** for the read-only, no-scroll fleet triage view. The normal
134-device production-health roster fits as fixed squares on one 320x240 screen;
the grid automatically compacts if unexpected live IDs expand it toward the
192-entry census limit. Registry positions stay stable in short-MAC order.

- green: raw reported VBAT >3.20 V;
- yellow: raw reported VBAT >3.10 V and <=3.20 V;
- red: raw reported VBAT <=3.10 V;
- grey: a rostered fixture is not currently fresh/on-air; and
- blue: a fresh heartbeat has no plausible battery voltage.

These are operator triage bands, not ADR 0023 lifecycle thresholds. The app
deliberately does not use gauge SOC as its primary color. Tap a square, or use
the trackball and click, for voltage/current, age, RSSI/PDR, supply, advisory
SOC, class/program/lifecycle, sensor signature, firmware, and registry details.
The app is read-only and adds no packet type or fixture command.

The embedded roster is generated from `ops/fleet/registry.csv`; it includes
PowerFeathers whose status is `commissioned` or `commission_failed`, and omits
quarantined, bench-only, merely enumerated/demo, and bridge hardware. Any live
ID outside that roster is appended with a cyan border. The native test wrapper
regenerates the header and fails if the checked-in snapshot is stale.

Source plus native tests and the merged embedded build pass. Health is flashed
on `8EB508` in the 1,542,448-byte image documented above. Physical layout,
input, detail, and memory-watermark checks remain open in `TODO.md`.

## UTC and schedule controls (ADR 0049)

The T-Deck parses checksum-valid active GPS RMC date/time and broadcasts one
`NB_TIME_QUALITY` UTC anchor every two seconds. This cadence gives a field
fixture several chances to hear time during either its 15-second production
listen window or a future 8-second build-week window.

Open **Schedule** for GPS/UTC status and three fleet baselines:

- **Auto** returns to UTC civil twilight, with fixture solar/power fallback if
  trustworthy time expires;
- **Day Dark** temporarily forces the daylight baseline;
- **Night Show** temporarily forces the nighttime baseline.

All three are RAM-only and repeat for six minutes to span the full 300-second
field sleep cadence. LED Studio and program leases can override the baseline;
dark remains higher authority. Knock stays one-shot and locally safety-gated,
so it is not promised as a wake command for a sleeping fixture.

## Patterns v1

Open **Patterns** for microphone-independent, deterministic artistic control.
The first version offers Wash, Chase, Wave, and Twinkle with Ember, Forest,
Ocean, Aurora, and Moon palettes. Speed, intensity, fixture class, and stable
short-ID cohort A-D are adjustable from the handheld.

Patterns computes final per-fixture RGBW values and sends them through the same
bounded 8 Hz direct-frame stream used by LED Studio. Only one stream owner is
active: starting Patterns replaces LED Studio, and starting LED Studio replaces
Patterns. Leaving the app does not stop the selected look; use Stop explicitly.
If frames stop arriving, fixtures retain the normal three-second stale fallback.
Audio reactivity remains a separate v2 feature behind ES7210/I2S qualification.

## RF Diagnostics

Open **RF** for a read-only installation and mesh-health view. The summary page
shows live, seen, stale, production-roster unobserved, and foreign-live counts;
observation coverage; strongest and weakest fresh peers; receive drops; TX
success/failure counters; and mesh/WiFi/AP/channel-guard state. PDR is labelled
unknown until enough observations exist rather than being invented from RSSI.

The second page shows the existing newest-first valid-frame tail. Ranking is
deterministic by RSSI, then known PDR, age, and short ID. The app sends no mesh
packet and changes no fixture state or wire format.

Join timeouts / lost links fall back to mesh-only and retry with backoff. The
mesh is the primary function; Claude is the enhancement.

## Status page (M0)

Wi-Fi state / guard, SNTP UTC, mesh up + frame counter + last-frame
src/type/RSSI/age (+ MESH SILENT flag >15 s), battery mV/%, last key, trackball
pulse counts + click, touch point, probe verdicts, GPS fix summary, heap/PSRAM
watermarks. Redraw ~5 Hz into a PSRAM sprite, single pushSprite (no flicker).
`nb-mem heap_free=... psram_min=...` prints on serial every 10 s.

Press **`s`** on the keyboard to toggle the **direct-sun test pattern**
(max-contrast bars, chat-density text, RGB patches, backlight 255).

## M0 field checklist

- [x] **Direct-sun readability** (2026-08-19, Ben; re-tested same day):
      first pass looked near-illegible in direct sun, but the shipped
      **protective film was a glare confound** — with it removed and the panel
      tilted so the sun is off-perpendicular, full-sun reading is *fairly
      clear*; size 2 is comfortable, size 1 workable if a dense view needs it.
      **Field-UI consequence: size 2 is the default UI text size (M2);
      size 1 allowed for dense diagnostic tables. Operator habit: tilt the
      panel away from normal incidence in direct sun.**
- [ ] **Battery runtime**: `bat_mv` rides the 10 s `nb-mem` serial line — run
      unplugged (screen on, census running) and record hours; repeat with
      backlight 0. (Expectation: radio-RX-dominated ~168 mA + backlight →
      order 6-9 h; measure, don't assume.)
- [x] **Trackball direction map** (2026-08-19, Ben): a=UP b=RIGHT c=DOWN
      d=LEFT — recorded in `pins_tdeck.h`.
- [x] **Channel-guard live demo** (2026-08-19): mesh channel temporarily set
      to 6 vs BubbyNet on 11 → `guard: AP on ch 11 != mesh ch 6 -> wifi
      DROPPED, mesh kept`, status `GUARD:ch-mismatch`, mesh stayed up;
      restored to 11 → ONLINE again, SNTP synced, frames ticking.
- [x] **Coexistence**: ONLINE on BubbyNet (ch 11, -57 dBm) with mesh frames
      advancing 34→45 over ~7 s while associated.

## M1 acceptance (2026-08-19)

- Census port native-tested (donor-faithful tails + EWMA/windowed-PDR/eviction/
  observation ledger); live bench feathers tracked with plausible EWMA.
- `nb-master`/`nb-peer` emitters: 30/30 + 60/60 captured lines parse against
  `net_bench_dashboard.py`'s own regexes (full optional tails).
- TX proven end-to-end: identify-all was received by bench fixtures (their
  heartbeats report our downlink at `dlpdr=1.000`); strike path clamps 5-300 ms
  and refuses broadcast. Quick commands `i/I/K/U/B/b/t` are WAN-down safe;
  `U<6-hex-ID>` sustains exact-target OTA maintenance for 35 seconds and has no
  broadcast form.
- **1-hour soak: heap_min bit-identical (257,608 B) across 3,643 s** with STA
  associated + census running; PSRAM low-water drift ~5 KB (bounded ring
  high-water). Log: session scratchpad `m1_soak.log`.

## Milestones

M0 bring-up (this) → M1 mesh core (census port + nb-* emitters, dashboard
compatible) → M2 LVGL shell + Fleet/Settings → M3 Claude client → M4 agent tool
loop → M5 Patterns/Zones/Knocker/CA Studio → M6 voice + diagnostics + polish.
Plan of record: `APP_ROADMAP.md` + ADR 0047/0048, with ADR 0037 retained for
constraints not superseded by the app-platform decision.
