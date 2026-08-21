# tdeck_bridge — Resonance Bridge OS (LilyGO T-Deck Plus)

App-launcher handheld per ADR 0047 (platform of record; constraints from ADR
0037): simultaneously an ESP-NOW mesh citizen on the fleet channel (census +
command TX) and a Wi-Fi STA (Claude API over TLS, laptop services).

**Status 2026-08-20:** M0-M4 complete and hardware-verified. Working apps:
**Claude** (streaming chat + 6-tool agent loop with the confirm rail),
**Fleet** (live census, reported-color chips, node detail + identify,
dark/release), **Zones** (class-targeted solid colors via sustained 8 Hz
direct-frame streaming, client-side dim), **Knocker** (single strike +
knock-all behind confirm; synced schedules stubbed pending ADR 0031),
**CA Studio** (program leases + GH-CA knob params via the release-re-lease
workaround), **Settings**, **SunTest**. Remaining: Patterns + ES7210 mic
(basic ambient audio mode), voice (whisperd), Sensors/Locate/RF Survey,
polish (M5 tail + M6).

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
firmware/tdeck_bridge/build.sh                      # build only
firmware/tdeck_bridge/build.sh --port /dev/ttyACM0  # build + USB flash
firmware/tdeck_bridge/tests/run_tests.sh            # native tests (plain g++)
```

- FQBN is pinned in `build.sh`: generic `esp32:esp32:esp32s3` with
  `FlashMode=qio,FlashSize=16M,PSRAM=opi` (ESP32-S3FN16R8 — a wrong PSRAM mode
  boot-loops), `PartitionScheme=app3M_fat9M_16MB`, USB CDC on boot.
- Every build gets a unique `--build-path`; never resume a killed build dir.
  Uncached ESP32-S3 builds take 2-3 minutes — wait, don't restart.
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
  AP — fix the AP (`docs/howto/CAMP_NETWORK_SETUP.md`), then `wifi retry`.

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
  and refuses broadcast; quick commands `i/I/K/B/b/t` are WAN-down safe.
- **1-hour soak: heap_min bit-identical (257,608 B) across 3,643 s** with STA
  associated + census running; PSRAM low-water drift ~5 KB (bounded ring
  high-water). Log: session scratchpad `m1_soak.log`.

## Milestones

M0 bring-up (this) → M1 mesh core (census port + nb-* emitters, dashboard
compatible) → M2 LVGL shell + Fleet/Settings → M3 Claude client → M4 agent tool
loop → M5 Patterns/Zones/Knocker/CA Studio → M6 voice + diagnostics + polish.
Plan of record: `docs/decisions/` Bridge OS ADR (pending) + ADR 0037.
