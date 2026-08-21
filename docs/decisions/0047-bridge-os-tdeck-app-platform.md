# 0047 -- Bridge OS: the T-Deck app platform

**Date:** 2026-08-19/20

**Status:** Accepted and in active development. Supersedes the scope (not the
constraints) of ADR 0037: the "cricket console" chat handheld grew into an
app-launcher OS. Every ADR 0037 constraint remains binding and is restated
below where it shaped an implementation.

**Owners:** Ben + Claude

## Context

ADR 0037 recorded a Claude chat handheld as post-2026-event work. Ben
re-prioritized it on 2026-08-19 and expanded the scope: one pocket device that
is simultaneously a passive fleet observer, a mesh commander, and a Claude
terminal — with apps for fleet health, Hue-style zone control, knock control,
pattern streaming with audio reactivity, CA tuning, sensors, locate, and
settings. Target: `firmware/tdeck_bridge/` on the T-Deck Plus (LCD variant).

## Decisions

1. **LVGL 9 on a hand-configured LovyanGFX ST7789 driver.** First UI-framework
   dependency in the repo (installed via arduino-cli, version pinned in the
   README; vendoring under `lib/` remains open). Rationale: an app OS with
   three input devices and a dozen screens is exactly what the hand-rolled
   sprite pattern (cores3) does not scale to. Draw buffers (2x 320x40 RGB565)
   live in internal DMA RAM; the 256 KB LVGL pool lives in PSRAM
   (`lv_conf.h`). All `lv_*` calls stay on one UI task.
2. **Field UI text floor is "size 2"** (M0 sun verdict, re-tested after
   removing the shipped screen film: full sun readable with the panel tilted
   off-normal; size-1 reserved for dense diagnostic tables).
3. **Input model: touch-first, trackball first-class** (gloves/dust make
   touch unreliable on playa). The trackball is a context-aware keypad indev:
   left/right = focus step, up/down = per-screen nav hooks (launcher row-jump,
   table row-scroll), click = per-screen enter hook; in edit mode pulses become
   arrow keys. Calibration a=UP b=RIGHT c=DOWN d=LEFT lives in `pins_tdeck.h`.
4. **The census is the cores3 port plus honesty upgrades:** RSSI EWMA
   (alpha 1/8), windowed PDR beside the donor's cumulative ratio, eviction
   with a 6 dB newcomer hysteresis, class latching across hb-short frames,
   and an observation ledger so duty-cycled listening reads "unobserved",
   never "quiet" (ADR 0037 §11). `nb-master`/`nb-peer` serial emitters remain
   byte-compatible with `ops/bench/net_bench_dashboard.py`.
5. **Single-writer doctrine as code:** `mesh_tx.cpp` is the only translation
   unit that emits Nb packets; burst-repeat follows the fleet convention
   (4x/5 ms broadcast, 6x/8 ms targeted). Strikes require a real target id at
   every layer (fixture already refuses broadcast strikes).
6. **The confirm rail is one component for humans and the agent alike**
   (`ui_confirm`): fleet-wide actions show a modal naming the ORIGIN ("Fleet
   app asks:" / "Claude asks:"), focus lands on cancel, and the cross-task
   path blocks the requesting task with a 30 s TTL. A timeout or denial
   round-trips to the model as an `is_error` tool result it must verbalize.
7. **Claude client:** raw TLS (embedded GTS trust anchors,
   `src/net/anthropic_root_ca.h`, refresh procedure in the header), manual
   chunked-transfer decode feeding a pure native-tested SSE parser, 12-turn
   PSRAM chat log, `thinking: disabled` + `output_config.effort: low`, model
   NVS-overridable (default `claude-sonnet-5`). SNTP-before-TLS is enforced by
   the state machine; offline messages queue (amber), never error.
8. **Agent tool surface = exactly the six ADR 0037 tools** (`tool_schema.h`):
   mesh_census, node_status, identify, strike, set_program, sniffer_tail.
   No OTA/reboot/profile/lifecycle/sleep/capacity opcode is representable.
   Tool turns are stored as raw content-array turns in the chat log; the
   iteration cap is 8.
9. **Voice (M6): swappable STT interface** — laptop `whisperd` first, cloud
   STT later behind the same seam. Audio-source roles per Ben: PUCA (ADR 0035)
   and the CoreS3 audio build are the real sound-reactivity bridges; the
   T-Deck's ES7210 is a basic ambient mode that must defer to a foreign
   streamer on air.
10. **Fixture gaps do not limit the handheld** (Ben 2026-08-19): Bridge OS
    sends the full intended contract; the six fixture-side gaps are tracked in
    `TODO.md` → Firmware track (params re-lease no-op, inert
    bright/beat_phase/energy, NB_SENSOR_REPORT, fire_in_ms strike events,
    CA→strike seam, NB_NEIGHBOR_SET persistence).

## Status at time of writing

M0-M4 complete and hardware-verified (see `firmware/tdeck_bridge/README.md`
and LOG 2026-08-19/20): bring-up + channel guard + coexistence, census +
dashboard-compatible emitters + 1 h zero-creep soak, LVGL shell + Fleet (live
table with reported-color chips) + Settings + confirm rail, streaming chat,
and the agent loop with all three acceptance tests green (census answer,
typed-English identify on a physical fixture, fleet-wide confirm timeout
refused honestly). M5 (Patterns/Zones/Knocker/CA Studio) and M6 (voice +
diagnostics) remain.

## Consequences

- The unauthenticated-command exposure (ADR 0037 §8) now includes a
  conversational commander; unchanged mitigation set, same tracked work item,
  still a precondition for event trust.
- Two T-Decks exist; the second stays a spare until the port matters.
- The Cardputer ADV port waits behind the `hal/` seam (unchanged).
- Battery runtime remains the one open M0 number (`bat_mv` rides the 10 s
  `nb-mem` line; run unplugged and read the log).

## References

- `docs/decisions/0037-claude-mesh-bridge-handheld.md` (constraints of record)
- `docs/research/CLAUDE_MESH_BRIDGE_DESIGN_2026-08-15.md`
- `firmware/tdeck_bridge/README.md` (acceptance evidence per milestone)
- `TODO.md` → Firmware track → Bridge OS fixture gaps
