# AGENTS.md

Read this if you are an AI agent picking up work in this repo for the first time.

## What this repo is

The power, electronics, firmware, and electronics-enclosure ("hat") workstream for the **Resonance Tree** -- a bamboo art installation for Burning Man 2026 + 2027. This repo is one slice of a larger project. The other slices (bamboo structure, structural engineering, parametric lighting design, project management) live elsewhere, mostly with the project lead Elliot Fabri and his Co-Work agent.

## Read order at session start

1. `README.md` -- orientation.
2. `LOG.md` -- what changed recently.
3. `TODO.md` -- what's open.
4. `BACKGROUND.md` -- full project context, team, history. Long but worth it on first session.
5. `docs/block-diagram/SYSTEM.md` -- the canonical system architecture and power budget.
6. `docs/decisions/` -- ADRs for every major architectural decision so far. Numbered. Read in order if you have time, otherwise look up by topic.

After session, append to `LOG.md` with a dated entry summarizing what changed and why. Append to `TODO.md` for new open items. Add to `docs/decisions/` when you make a new architectural call.

## High-priority bench gotchas

- **OTA fleet path:** default to shared-WiFi / portable-router maintenance mode plus
  `ops/bench/net_bench_ota.py` parallel uploads. Do **not** build or recommend
  `--maint-ap` unless Ben explicitly asks for the deprecated one-board AP fallback;
  self-hosted AP mode is not scalable and has confused recent OTA debugging.
- **Arduino compile cache:** do **not** run parallel `arduino-cli compile` commands
  against the same sketch/cache. Either build sequentially or pass a unique
  `--build-path` per compile. `firmware/net_bench/build.sh` already does this; direct
  `arduino-cli` calls must do it manually. Parallel builds against the default Arduino
  sketch cache can collide with `unlinkat ... directory is not empty` errors and can
  corrupt mixed build artifacts.

## Who's working in this repo

- **Ben Eckart** (`ben.eckart@gmail.com`) -- power systems, firmware, mesh networking, project lead for the lighting workstream within Resonance. Primary committer to `/firmware/` and `/hardware/`.
- **Steve Eckart** (Ben's dad) -- enclosure design, 3D printing, mechanical integration. Primary committer to `/enclosure/`.
- **Claude** (this) -- pair-programmer for both Ben and Steve. Cowork instance handles project management and review (this side). Claude Code instances handle daily implementation iteration.

The wider Resonance project team is in `BACKGROUND.md` -- read it for names and roles. Don't message them or assume their context; coordinate through Ben, who interfaces with them via WhatsApp.

## What's known vs assumed

**Decided** (see ADRs):
- ESP32-C3-MINI-1 module for production (ADR 0001).
- LiFePO4 battery chemistry (ADR 0002).
- CN3058 LiFePO4 charger IC (ADR 0003).
- ESP-NOW mesh, no infrastructure required (ADR 0004).
- FreeRTOS task architecture, not Arduino loop() (ADR 0005).
- Custom PCB with reflowed module, not dev-board-on-carrier (ADR 0006).
- Electronics in a separable hat on top of the bamboo lantern, not crammed inside (ADR 0007).
- WS2812B powered direct from Vbat, no level shifter (ADR 0008).
- Minimize per-fixture operations at scale: no soldering on receipt, no per-unit configuration, jig-automated flashing (ADR 0009).
- PowerFeather V2 (ESP32-S3) confirmed as the COTS reference after feasibility de-risking -- networking, solar, and battery-only no-touch OTA all validated (ADR 0021).

**Open** (see TODO.md and ROADMAP.md):
- Rope attachment point: hat / bamboo / hybrid. Pending team input.
- Hat dimensions: placeholder, awaiting Vishnu input.
- Cost decomposition of `INV_2026_00401` invoice.
- Whether the Community Mandala Program goes ahead.

**Validated on hardware** (2026-06, PowerFeather V2 COTS bench -- see ADR 0021 +
`docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md` + LOG 2026-06-07/08):
- **ESP-NOW networking** scales to ~100 fixtures (5-node bench ~99% PDR, clean rate-knee) and
  the radio reaches well past tree scale (held through a house + yard + oak, ~100 steps). The
  lantern enclosure is RF-transparent; the solar panel is the main ~20 dB attenuator (antenna
  keep-out matters).
- **Battery-only, no-touch OTA + A/B rollback** (the "never take a lantern off the tree"
  requirement): software-reset OTA recovered ~17/17 incl. worst-case LFP voltage; a
  self-test-failing image auto-reverts to last-good. Watchdog + autosleep recovery validated.
- **Solar charge path** end-to-end: net-positive into an LFP even in weak/partial light.

**Assumed** but not yet validated on hardware:
- The exact **nightly power budget** -- the old ~120 mAh/night napkin number is RETIRED
  (2026-07-02): pre-hardware math that crisp-gobo light levels invalidate. Derive
  bottom-up from measured LED draw (400-500 mA at full) x show duty cycle, then size
  cell/panel. Full-sun harvest number + LFP re-verify of the battery/stability runs
  still pending.
- WS2812B-from-Vbat on LiFePO4 -- superseded direction: LED axis is now direct-GPIO --
  data on a free GPIO (e.g. GPIO10/A0), V+ from the regulated switchable 3V3 header
  rail, deliberately NOT on the I2C/STEMMA bus the IS31 shared with the charger/gauge
  (ADR 0018). Note the rail is not stiff at show loads: the 2026-06-10 discharge
  measured ~2.96-2.97 V at the LED at ~290 mA (see LOG 2026-07-02).
- 1-3 LEDs at ~10% brightness gives the desired ambient look. Gobo + ambient tuning pending
  (note the 8-bit dimming low-end limit -- ADR 0018 / POWERFEATHER_NOTES).

## What this repo does NOT cover

- Bamboo lantern fabrication (Bamboo Pure / Vishnu, Bali).
- Tree structural design (Ed Wilkes, Bristol).
- Wind chime cluster electronics (separate workstream, Vishnu).
- Project-wide logistics, budget, container shipping (Elliot, Co-Work agent).
- The Resonance project's grant strategy / fundraising (Elliot).

If a task touches one of these areas, do not assume; ask Ben to relay it through the right channel.

## Style for this repo

- Markdown for everything that isn't code or CAD. Plain text, no emojis.
- Keep Markdown/docs ASCII-only unless there is a project-critical reason not to. Use
  `--`, `->`, `>=`, `<=`, `deg C`, `ohm`, `uA`, etc. instead of Unicode punctuation or
  symbols; Windows shells have repeatedly rendered those as mojibake.
- ASCII diagrams beat external image files. Easier to diff, easier for agents to read.
- Schematics-as-code via atopile. Layout in KiCad. No proprietary CAD source.
- Firmware split: platform-independent C++ in `firmware/core/` (compiles native, has unit tests), platform glue in `firmware/esp32/` (links to ESP-IDF / Arduino-ESP32). See `firmware/ARCHITECTURE.md`.
- ADRs are the contract for any decision worth remembering. Append, don't edit. Supersede with a new ADR.

## Cross-references with other tools

- **Project's WhatsApp threads** ("Resonance Tree", "Resonance Agentic Wiki") -- primary team comms, not directly accessible from this repo. Ben relays.
- **Co-Work agent** (Elliot's PM agent) -- maintains a separate wiki Co-Work syncs from WhatsApp + Fireflies meeting transcripts. Plan: get read access to Co-Work's wiki folder once Elliot has it cloud-hosted, then this repo's `LOG.md` and Co-Work's wiki cross-reference each other.
- **`beneckart/future-robotics`** GitHub repo -- Ben's prior Burning Man projects (Talisman v1/v2/v2rev2, Marquee, MaraudersMap, Winduino). Source of reusable code (TalismanPatterns.cpp, packet codec patterns) and lessons (see BACKGROUND.md "Lessons from 2018 Talisman v2" section).
- **Drive folder** with original Talisman v2 design docs and measured power numbers -- referenced in `BACKGROUND.md`.

## When in doubt

Ask Ben. Don't assume context. The project has a real timeline (BM 2026 ships in August) and real costs ($K of bamboo + electronics already in-flight). Wrong decisions are recoverable but expensive.
